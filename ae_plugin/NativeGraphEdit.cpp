#include "NativeGraphEdit.hpp"
#include "NativeClearRequest.hpp"
#include "GraphCommitRequest.hpp"
#include "GraphBindingRefreshRequest.hpp"
#include "GraphDiagnosticsRequest.hpp"
#include "NodeRecord.hpp"
#include "Parameters.hpp"
#include "UiExclusionHost.hpp"
#include "GraphRenderJournal.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>

namespace starfield::adapter {
namespace {
constexpr A_Err bad=PF_Err_BAD_CALLBACK_PARAM;
constexpr A_Err damaged=PF_Err_INTERNAL_STRUCT_DAMAGED;
bool clear_dispatch_active{};
using EditClock=std::chrono::steady_clock;
std::atomic_uint64_t edit_operation{};
std::uint64_t elapsed_us(EditClock::time_point start) noexcept {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(EditClock::now()-start).count());
}
template<class F> auto timed(std::uint64_t& value,F&& call) {
    struct Timer {std::uint64_t& value;EditClock::time_point start{EditClock::now()};~Timer(){value+=elapsed_us(start);}} timer{value};
    return call();
}
using Kind=native_nodes::Kind;
template<class T> struct Suite {
    SPBasicSuite* basic;const char* name;A_long version;const T* value{};
    Suite(SPBasicSuite* b,const char* n,A_long v):basic(b),name(n),version(v){if(b)b->AcquireSuite(n,v,reinterpret_cast<const void**>(&value));}
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
    const T* operator->()const{return value;}explicit operator bool()const{return value!=nullptr;}
};
struct EffectRef {const AEGP_EffectSuite4* suite;AEGP_EffectRefH value{};~EffectRef(){if(value)suite->AEGP_DisposeEffect(value);}};
struct StreamRef {const AEGP_StreamSuite6* suite;AEGP_StreamRefH value{};~StreamRef(){if(value)suite->AEGP_DisposeStream(value);}};
int kind(const char* name) {
    if(!std::strcmp(name,"org.starfieldfx.particle"))return -1;
    for(const auto [label,k]:{std::pair{"org.starfieldfx.node.emitter",Kind::emitter},
        {"org.starfieldfx.node.particle",Kind::particle},{"org.starfieldfx.node.force",Kind::force},
        {"org.starfieldfx.node.transform",Kind::transform},{"org.starfieldfx.node.model",Kind::model},
        {"org.starfieldfx.node.material",Kind::material},{"org.starfieldfx.node.motion",Kind::motion},
        {"org.starfieldfx.node.turbulence",Kind::turbulence}})if(!std::strcmp(name,label))return static_cast<int>(k);
    return -2;
}
bool valid_id(const AuthorNodeId& id){AuthorNodeId output{};output.back()=255;
    return id!=AuthorNodeId{} && id!=output;}
core::NodeId core_id(const AuthorNodeId& id){core::NodeId r;for(std::size_t i=0;i<8;++i){r.value.bytes[i*2]=static_cast<std::uint8_t>(id[i]>>8);r.value.bytes[i*2+1]=static_cast<std::uint8_t>(id[i]);}return r;}
bool integer(double v,double maximum){return std::isfinite(v) && v>=0 && v<=maximum && std::floor(v)==v;}
struct Node {
    int type{};A_long index{};AuthorNodeId id{};
    A_long count{},next_count{};
    std::array<std::uint16_t,native_nodes::kMaxOutgoingEdges*native_nodes::kConnectionRecordChunks> words{},next{};
    bool removed{},touched{};
};
struct Renderer {A_long index{-1};double revision{},high{},low{},nonce{},receipt{};};
struct Access {
    SPBasicSuite* basic;AEGP_PluginID plugin;AEGP_LayerH layer;
    Suite<AEGP_EffectSuite4> effects;Suite<AEGP_StreamSuite6> streams;
    Access(SPBasicSuite* b,AEGP_PluginID p,AEGP_LayerH l):basic(b),plugin(p),layer(l),
        effects(b,kAEGPEffectSuite,kAEGPEffectSuiteVersion4),streams(b,kAEGPStreamSuite,kAEGPStreamSuiteVersion6){}
    A_Err effect(A_long index,AEGP_EffectRefH& ref){return effects->AEGP_GetLayerEffectByIndex(plugin,layer,index,&ref);}
    A_Err type(AEGP_EffectRefH ref,int& result){AEGP_InstalledEffectKey key{};char label[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
        auto e=effects->AEGP_GetInstalledKeyFromLayerEffect(ref,&key);if(!e)e=effects->AEGP_GetEffectMatchName(key,label);
        if(!e)result=kind(label);return e;}
    A_Err read(AEGP_EffectRefH fx,A_long index,double& number){StreamRef ref{streams.value};
        auto e=streams->AEGP_GetNewEffectStreamByIndex(plugin,fx,index,&ref.value);if(e || !ref.value)return e?e:bad;
        AEGP_StreamType t{};e=streams->AEGP_GetStreamType(ref.value,&t);if(e || t!=AEGP_StreamType_OneD)return e?e:bad;
        AEGP_StreamValue2 v{};const A_Time zero{0,1};e=streams->AEGP_GetNewStreamValue(plugin,ref.value,AEGP_LTimeMode_LayerTime,&zero,TRUE,&v);
        if(e)return e;number=v.val.one_d;return streams->AEGP_DisposeStreamValue(&v);}
    A_Err write(AEGP_EffectRefH fx,A_long index,double number){StreamRef ref{streams.value};
        auto e=streams->AEGP_GetNewEffectStreamByIndex(plugin,fx,index,&ref.value);if(e || !ref.value)return e?e:bad;
        AEGP_StreamValue2 v{};v.streamH=ref.value;v.val.one_d=number;e=streams->AEGP_SetStreamValue(plugin,ref.value,&v);
        if(e)return e;double saved{};e=read(fx,index,saved);return e?e:saved==number?0:damaged;}
    A_Err id(AEGP_EffectRefH fx,int type,AuthorNodeId& out){for(A_long i=0;i<8;++i){double value{};
        auto e=read(fx,native_nodes::uuid_first_index(static_cast<Kind>(type))+i,value);if(e || !integer(value,65535))return e?e:bad;
        out[i]=static_cast<std::uint16_t>(value);}return valid_id(out)?0:bad;}
    A_Err renderer(AEGP_EffectRefH fx,Renderer& r){auto e=read(fx,kGraphRevisionId,r.revision);
        if(!e)e=read(fx,kGraphChecksumHighId,r.high);if(!e)e=read(fx,kGraphChecksumLowId,r.low);
        if(!e)e=read(fx,kGraphEditCommitId,r.nonce);if(!e)e=read(fx,kGraphEditReceiptId,r.receipt);return e;}
    A_Err ref_node(const Node& n,AEGP_EffectRefH& fx){auto e=effect(n.index,fx);if(e || !fx)return e?e:bad;
        int t{};e=type(fx,t);AuthorNodeId actual{};if(!e && t==n.type)e=id(fx,t,actual);
        return e?e:t==n.type && actual==n.id?0:bad;}
    A_Err connections(Node& n,bool restore){EffectRef fx{effects.value};auto e=ref_node(n,fx.value);if(e)return e;
        const auto k=static_cast<Kind>(n.type);e=write(fx.value,native_nodes::sync_guard_index(k),1);
        const auto& data=restore?n.words:n.next;const auto count=restore?n.count:n.next_count;
        for(A_long i=0;!e && i<std::max(n.count,n.next_count)*native_nodes::kConnectionRecordChunks;++i)
            e=write(fx.value,native_nodes::connection_first_index(k)+i,data[i]);
        if(!e)e=write(fx.value,native_nodes::connection_count_index(k),count);
        if(!e)e=write(fx.value,native_nodes::sync_guard_index(k),0);return e;}
};
bool decimal(std::wstring_view s,A_long maximum,A_long& out){if(s.empty() || s.size()>10 || (s.size()>1 && s.front()==L'0'))return false;
    std::uint64_t v{};for(auto c:s){if(c<L'0'||c>L'9')return false;v=v*10+unsigned(c-L'0');if(v>static_cast<unsigned>(maximum))return false;}
    out=static_cast<A_long>(v);return true;}
int hex(wchar_t c){return c>=L'0'&&c<=L'9'?c-L'0':c>=L'a'&&c<=L'f'?c-L'a'+10:-1;}
NativeConnection connection(const Node& n,A_long edge){NativeConnection c;const auto at=n.words.begin()+edge*native_nodes::kConnectionRecordChunks;std::copy_n(at,8,c.target.begin());std::copy_n(at+8,8,c.edge.begin());return c;}
std::vector<NativeConnection> connections(const Node& n){std::vector<NativeConnection> result;for(A_long i=0;i<n.count;++i)result.push_back(connection(n,i));return result;}
bool changed(const Node& n){return n.count!=n.next_count || !std::equal(n.words.begin(),n.words.begin()+n.next_count*native_nodes::kConnectionRecordChunks,n.next.begin());}
A_Err desired(Node& n,const std::vector<NativeConnection>& list){if(list.size()>native_nodes::kMaxOutgoingEdges)return bad;n.next={};n.next_count=static_cast<A_long>(list.size());
    for(std::size_t i=0;i<list.size();++i){auto at=n.next.begin()+i*native_nodes::kConnectionRecordChunks;std::copy(list[i].target.begin(),list[i].target.end(),at);std::copy(list[i].edge.begin(),list[i].edge.end(),at+8);}return 0;}
A_Err bridge_connections(std::vector<Node>& nodes,const std::vector<AuthorNodeId>& removed,const std::vector<Node>& original) {
    const auto selected=[&](const auto& id){return std::find(removed.begin(),removed.end(),id)!=removed.end();};
    std::vector<AuthorNodeId> occupied;for(const auto& n:original)for(A_long i=0;i<n.count;++i)occupied.push_back(connection(n,i).edge);
    AuthorNodeId allocated{};
    const auto new_edge=[&](){for(unsigned attempt=0;attempt<512;++attempt){for(auto i=allocated.rbegin();i!=allocated.rend();++i)if(++*i)break;
        if(valid_id(allocated) && std::find(occupied.begin(),occupied.end(),allocated)==occupied.end()){occupied.push_back(allocated);return allocated;}}return AuthorNodeId{};};
    for(auto& n:nodes){std::vector<NativeConnection> list;const auto old=connections(n);
        for(const auto& c:old)if(!selected(c.target))list.push_back(c);
        for(const auto& c:old)if(selected(c.target)) {
            std::vector<AuthorNodeId> waiting{c.target},visited;
            while(!waiting.empty()){const auto id=waiting.back();waiting.pop_back();if(std::find(visited.begin(),visited.end(),id)!=visited.end())continue;visited.push_back(id);
                const auto found=std::find_if(original.begin(),original.end(),[&](const auto& source){return source.id==id;});if(found==original.end())return bad;
                for(A_long i=0;i<found->count;++i){const auto out=connection(*found,i);if(selected(out.target)){waiting.push_back(out.target);continue;}
                    if(out.target==n.id || std::any_of(list.begin(),list.end(),[&](const auto& v){return v.target==out.target;}))continue;
                    if(list.size()>=native_nodes::kMaxOutgoingEdges)return bad;const auto edge_id=new_edge();if(!valid_id(edge_id))return bad;list.push_back({out.target,edge_id});}
            }
        }
        if(auto e=desired(n,list);e)return e;
    }
    return 0;
}
A_Err prepare_tail(Access& a,Node& n){if(n.next_count<=n.count)return 0;EffectRef fx{a.effects.value};auto e=a.ref_node(n,fx.value);if(e)return e;
    for(A_long i=n.count*native_nodes::kConnectionRecordChunks;i<n.next_count*native_nodes::kConnectionRecordChunks;++i){double value{};e=a.read(fx.value,native_nodes::connection_first_index(static_cast<Kind>(n.type))+i,value);
        if(e || !integer(value,65535))return e?e:bad;n.words[i]=static_cast<std::uint16_t>(value);}return 0;
}
A_Err patch_connections(std::vector<Node>& nodes,const NativeDeleteRequest& request){
    for(std::size_t i=0;i<request.rows.size();++i)for(std::size_t j=0;j<i;++j)if(request.rows[i].source==request.rows[j].source)return bad;
    for(auto& n:nodes){auto e=desired(n,connections(n));if(e)return e;}
    for(const auto& row:request.rows){const auto n=std::find_if(nodes.begin(),nodes.end(),[&](const auto& v){return v.id==row.source;});
        if(n==nodes.end() || n->removed || connections(*n)!=row.expected)return bad;auto e=desired(*n,row.desired);if(e)return e;}
    AuthorNodeId output{};output.back()=255;std::vector<AuthorNodeId> ids;
    for(const auto& n:nodes)if(!n.removed)for(A_long i=0;i<n.next_count;++i){AuthorNodeId target{},edge{};const auto at=n.next.begin()+i*native_nodes::kConnectionRecordChunks;std::copy_n(at,8,target.begin());std::copy_n(at+8,8,edge.begin());
        if(edge==AuthorNodeId{} || std::find(ids.begin(),ids.end(),edge)!=ids.end() || (target!=output && std::none_of(nodes.begin(),nodes.end(),[&](const auto& v){return !v.removed && v.id==target;})))return bad;ids.push_back(edge);}
    return 0;
}
A_Err clear_bridge_capacity(const std::vector<Node>& original,const std::vector<AuthorNodeId>& selected) {
    // AE may Clear a subset (or just keyframes). Bound every possible retained
    // frontier before dispatch, rather than discover row overflow after removal.
    const auto removable=[&](const auto& id){return std::find(selected.begin(),selected.end(),id)!=selected.end();};
    for(const auto& source:original){std::vector<AuthorNodeId> path;unsigned work=0;
        std::function<unsigned(const AuthorNodeId&)> cost=[&](const AuthorNodeId& id)->unsigned {
            if(++work>20000)return 5;if(id==source.id)return 0;if(!removable(id))return 1;
            if(std::find(path.begin(),path.end(),id)!=path.end())return 1;
            const auto node=std::find_if(original.begin(),original.end(),[&](const auto& n){return n.id==id;});if(node==original.end())return 5;
            path.push_back(id);unsigned total=0;
            for(A_long i=0;i<node->count && total<5;++i)total+=cost(connection(*node,i).target);
            path.pop_back();return std::max(1u,std::min(5u,total));
        };
        unsigned total=0;for(A_long i=0;i<source.count && total<=4;++i){const auto target=connection(source,i).target;total+=target==source.id?1:cost(target);}
        if(total>4)return bad;
    }
    return 0;
}
A_Err scan(Access& a,const NativeDeleteRequest& request,Renderer& renderer,std::vector<Node>& nodes,A_long& total,
    bool allow_missing=false,double renderer_guard=0){
    auto e=a.effects->AEGP_GetLayerNumEffects(a.layer,&total);if(e || total<1 || total>4096)return e?e:bad;
    for(A_long i=0;i<total;++i){EffectRef fx{a.effects.value};e=a.effect(i,fx.value);if(e || !fx.value)return e?e:bad;
        int t{};e=a.type(fx.value,t);if(e)return e;if(t==-2)continue;double guard{};
        e=a.read(fx.value,t<0?kGraphSyncGuardId:native_nodes::sync_guard_index(static_cast<Kind>(t)),guard);if(e || guard!=(t<0?renderer_guard:0))return e?e:bad;
        if(t<0){if(renderer.index>=0)return bad;renderer.index=i;e=a.renderer(fx.value,renderer);
            double ready{},source{};if(!e)e=a.read(fx.value,kNodeEffectsReadyId,ready);if(!e)e=a.read(fx.value,kControlSourceId,source);
            if(e || ready!=1 || source!=kNodeControlSource)return e?e:bad;continue;}
        if(nodes.size()>=63)return bad;Node n;n.type=t;n.index=i;e=a.id(fx.value,t,n.id);if(e)return e;
        if(std::any_of(nodes.begin(),nodes.end(),[&](const auto& v){return v.id==n.id;}))return bad;
        n.removed=std::find(request.nodes.begin(),request.nodes.end(),n.id)!=request.nodes.end();
        double count{};e=a.read(fx.value,native_nodes::connection_count_index(static_cast<Kind>(t)),count);
        if(e || !integer(count,native_nodes::kMaxOutgoingEdges))return e?e:bad;n.count=static_cast<A_long>(count);
        for(A_long j=0;j<n.count*native_nodes::kConnectionRecordChunks;++j){double value{};
            e=a.read(fx.value,native_nodes::connection_first_index(static_cast<Kind>(t))+j,value);if(e || !integer(value,65535))return e?e:bad;
            n.words[j]=static_cast<std::uint16_t>(value);}
        nodes.push_back(n);
    }
    if(renderer.index<0 || !integer(renderer.revision,16777214) || renderer.revision<1 ||
        !integer(renderer.high,65535) || !integer(renderer.low,65535) || !integer(renderer.nonce,1000000) ||
        renderer.receipt!=renderer.nonce)return bad;
    if(request.revision && (renderer.revision!=request.revision ||
        (static_cast<std::uint32_t>(renderer.high)*65536+static_cast<std::uint32_t>(renderer.low))!=request.checksum))return bad;
    if(!allow_missing && std::count_if(nodes.begin(),nodes.end(),[](const auto& n){return n.removed;})!=static_cast<std::ptrdiff_t>(request.nodes.size()))return bad;
    return 0;
}
}
static bool parse_native_request(std::wstring_view text,NativeDeleteRequest& result,bool allow_empty){
    if(text.size()>6400)return false;std::array<std::wstring_view,5> fields{};std::size_t at{};
    for(std::size_t i=0;i<4;++i){const auto end=text.find(L'|',at);if(end==text.npos)return false;fields[i]=text.substr(at,end-at);at=end+1;}fields[4]=text.substr(at);
    NativeDeleteRequest r;const auto token=fields[0];const auto c=token.find(L"-c"),l=token.find(L"-l");
    if(token.empty() || token.front()!=L'p' || c==token.npos || l==token.npos || l<=c+2 ||
        !decimal(token.substr(1,c-1),0,r.project) || !decimal(token.substr(c+2,l-c-2),0x7fffffff,r.comp) || !r.comp ||
        !decimal(token.substr(l+2),0x7fffffff,r.layer) || !r.layer || !decimal(fields[1],16777214,r.revision) || !r.revision || fields[2].size()!=8)return false;
    for(auto ch:fields[2]){const auto d=hex(ch);if(d<0)return false;r.checksum=r.checksum*16+unsigned(d);}
    if(fields[3].size()>4096)return false;for(auto ch:fields[3])if(ch<32)return false;r.project_scope=fields[3];
    at=0;while(at<fields[4].size()){const auto end=fields[4].find(L',',at);const auto word=fields[4].substr(at,end==text.npos?end:end-at);
        if(word.size()!=32 || r.nodes.size()>=63)return false;AuthorNodeId id{};
        for(std::size_t i=0;i<32;++i){const auto d=hex(word[i]);if(d<0)return false;id[i/4]=static_cast<std::uint16_t>(id[i/4]*16+d);}
        if(!valid_id(id) || std::find(r.nodes.begin(),r.nodes.end(),id)!=r.nodes.end())return false;r.nodes.push_back(id);
        if(end==text.npos)break;at=end+1;if(at==fields[4].size())return false;}
    if(r.nodes.empty() && !allow_empty)return false;result=std::move(r);return true;
}
bool parse_native_delete(std::wstring_view text,NativeDeleteRequest& result){return parse_native_request(text,result,false);}
bool parse_native_patch(std::wstring_view text,NativeDeleteRequest& result){
    if(text.size()>48000)return false;const auto end=text.rfind(L'|');if(end==text.npos)return false;
    NativeDeleteRequest r;if(!parse_native_request(text.substr(0,end),r,true))return false;r.patch=true;
    const auto uuid=[](std::wstring_view word,AuthorNodeId& id){if(word.size()!=32)return false;for(std::size_t i=0;i<32;++i){const auto d=hex(word[i]);if(d<0)return false;id[i/4]=static_cast<std::uint16_t>(id[i/4]*16+d);}return id!=AuthorNodeId{};};
    const auto list=[&](std::wstring_view input,std::vector<NativeConnection>& output){std::size_t at=0;while(at<input.size()){
        const auto stop=input.find(L',',at);const auto word=input.substr(at,stop==input.npos?input.npos:stop-at);NativeConnection c;
        if(word.size()!=64 || output.size()>=4 || !uuid(word.substr(0,32),c.target) || !uuid(word.substr(32),c.edge))return false;
        output.push_back(c);if(stop==input.npos)break;at=stop+1;if(at==input.size())return false;}return true;};
    std::size_t at=end+1;while(at<text.size()){const auto stop=text.find(L';',at);const auto row=text.substr(at,stop==text.npos?text.npos:stop-at);const auto arrow=row.find(L'>');
        NativeConnectionRow value;if(r.rows.size()>=63 || row.size()<34 || row[32]!=L':' || arrow==row.npos || arrow<33 || !uuid(row.substr(0,32),value.source) ||
            !list(row.substr(33,arrow-33),value.expected) || !list(row.substr(arrow+1),value.desired) ||
            std::any_of(r.rows.begin(),r.rows.end(),[&](const auto& old){return old.source==value.source;}))return false;
        r.rows.push_back(std::move(value));if(stop==text.npos)break;at=stop+1;if(at==text.size())return false;}
    if(r.rows.empty() && r.nodes.empty())return false;result=std::move(r);return true;
}
std::wstring native_delete_reply(const NativeDeleteResult& r){const bool ok=(r.committed || r.unchanged) && !r.error && !r.cleanup_error && !r.undo_error;
    std::wstring s=L"{\"ok\":";s+=ok?L"true":L"false";s+=L",\"committed\":";s+=r.committed?L"true":L"false";
    s+=L",\"guarded\":";s+=r.guarded?L"true":L"false";
    s+=L",\"unchanged\":";s+=r.unchanged?L"true":L"false";
    s+=L",\"clearDispatched\":";s+=r.clear_dispatched?L"true":L"false";
    s+=L",\"outcomeUnknown\":";s+=r.outcome_unknown?L"true":L"false";
    s+=L",\"operationId\":"+std::to_wstring(r.operation)+L",\"timingUs\":{\"total\":"+std::to_wstring(r.timing_us.total)+
        L",\"resolve\":"+std::to_wstring(r.timing_us.resolve)+L",\"preflight\":"+std::to_wstring(r.timing_us.preflight)+
        L",\"capture\":"+std::to_wstring(r.timing_us.capture)+L",\"publish\":"+std::to_wstring(r.timing_us.publish)+
        L",\"physical\":"+std::to_wstring(r.timing_us.physical)+L",\"refresh\":"+std::to_wstring(r.timing_us.refresh)+L"}";
    s+=L",\"revision\":"+std::to_wstring(r.revision)+L",\"diagnostics\":{\"error\":"+std::to_wstring(r.error)+
        L",\"rollbackError\":"+std::to_wstring(r.rollback_error)+L",\"cleanupError\":"+std::to_wstring(r.cleanup_error)+L",\"undoError\":"+std::to_wstring(r.undo_error)+
        L",\"commandError\":"+std::to_wstring(r.command_error)+L",\"clearStage\":"+std::to_wstring(r.clear_stage)+L"}";
    s+=L",\"target\":{\"token\":\"p0-c"+std::to_wstring(r.comp)+L"-l"+std::to_wstring(r.layer)+L"\"}";
    if(!ok){s+=L",\"error\":{\"code\":";
        s+=r.capacity_rejected?L"\"connection_capacity\",\"message\":\"The bridge may exceed four outgoing wires. Disconnect another wire first. No source was removed.\"}":r.committed?L"\"native_graph_delete_cleanup_failed\",\"message\":\"The graph edit was committed, but deletion cleanup or Undo finalization failed. Check guarded and diagnostics before continuing.\"}":
            L"\"native_graph_delete_failed\",\"message\":\"Native deletion could not be confirmed. Check outcomeUnknown, guarded and diagnostics before continuing; no automatic retry was made.\"}";}
    return s+L"}";
}
static NativeDeleteResult delete_native_nodes_impl(SPBasicSuite* basic,AEGP_PluginID plugin,AEGP_LayerH layer,const NativeDeleteRequest& request) noexcept {
    NativeDeleteResult r;r.comp=request.comp;r.layer=request.layer;
    bool started{},commit_called{};Renderer before;std::vector<Node> nodes;
    try{
    if(!basic || !plugin || !layer || !SFLD_UiExclusionActiveV1() || (!request.patch && request.nodes.empty()) || request.nodes.size()>63 || request.rows.size()>63){r.error=bad;return r;}
    for(std::size_t i=0;i<request.nodes.size();++i)if(!valid_id(request.nodes[i]) || std::find(request.nodes.begin(),request.nodes.begin()+i,request.nodes[i])!=request.nodes.begin()+i){r.error=bad;return r;}
    Access a(basic,plugin,layer);Suite<AEGP_UtilitySuite6> utility(basic,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    Suite<AEGP_LayerSuite9> layers(basic,kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    if(!a.effects || !a.streams || !utility || !layers){r.error=bad;return r;}
    A_long total{};r.error=timed(r.timing_us.preflight,[&]{return scan(a,request,before,nodes,total);});if(r.error)return r;
    r.error=request.patch?patch_connections(nodes,request):bridge_connections(nodes,request.nodes,nodes);if(r.error)return r;
    for(auto& n:nodes)if(!n.removed && changed(n)){r.error=prepare_tail(a,n);if(r.error)return r;}
    A_Time time{};r.error=layers->AEGP_GetLayerCurrentTime(layer,AEGP_LTimeMode_LayerTime,&time);if(r.error || time.scale<=0){if(!r.error)r.error=bad;return r;}
    HostGraphMutation mutation;if(!mutation){r.error=bad;return r;}
    r.error=utility->AEGP_StartUndoGroup(request.nodes.empty()?"Starfield: edit connections":"Starfield: delete nodes");if(r.error)return r;
    struct UndoGroup {const AEGP_UtilitySuite6* suite;NativeDeleteResult& result;bool open{true};
        void close() noexcept {if(open){result.undo_error=suite->AEGP_EndUndoGroup();open=false;}}
        ~UndoGroup(){close();}} undo{utility.value,r};
    const auto renderer=[&](AEGP_EffectRefH& fx){auto e=a.effect(before.index,fx);if(e || !fx)return e?e:bad;int t{};e=a.type(fx,t);return e?e:t==-1?0:bad;};
    auto work=[&]() -> A_Err {
        {EffectRef fx{a.effects.value};auto e=renderer(fx.value);if(e)return e;started=true;r.guarded=true;e=a.write(fx.value,kGraphSyncGuardId,1);if(e)return e;}
        for(auto& n:nodes){if(n.removed){n.touched=true;EffectRef fx{a.effects.value};auto e=a.ref_node(n,fx.value);if(!e)e=a.write(fx.value,native_nodes::sync_guard_index(static_cast<Kind>(n.type)),3);if(e)return e;}
            else if(changed(n)){n.touched=true;const auto e=a.connections(n,false);if(e)return e;}}
        const A_long nonce=before.nonce>=1000000?1:static_cast<A_long>(before.nonce)+1;
        {EffectRef fx{a.effects.value};auto e=renderer(fx.value);if(!e)e=a.write(fx.value,kGraphEditCommitId,nonce);if(e)return e;
            GraphCommitRequest commit;commit.plugin=plugin;commit.renderer=fx.value;commit.layer=layer;commit.nonce=nonce;commit_called=true;r.outcome_unknown=true;
            e=timed(r.timing_us.publish,[&]{return a.effects->AEGP_EffectCallGeneric(plugin,fx.value,&time,PF_Cmd_COMPLETELY_GENERAL,&commit);});
            // The publisher has already verified its saved graph before returning
            // a positive acknowledgement. Do not repeat its reads on success.
            const bool accepted=commit.acknowledged==1 && commit.status==0;
            bool published=accepted;
            if(!accepted){Renderer saved;const auto read=a.renderer(fx.value,saved);
                published=!read && saved.revision==before.revision+1 && saved.nonce==nonce && saved.receipt==nonce;}
            r.committed=published;
            r.outcome_unknown=!r.committed && commit.acknowledged!=1;
            r.revision=r.committed?static_cast<A_long>(before.revision+1):0;
            if(e || commit.acknowledged!=1 || commit.status || !published)return e?e:commit.status?commit.status:bad;}
        for(auto i=nodes.rbegin();i!=nodes.rend();++i)if(i->removed){EffectRef fx{a.effects.value};auto e=a.ref_node(*i,fx.value);if(e)return e;
            double guard{};e=a.read(fx.value,native_nodes::sync_guard_index(static_cast<Kind>(i->type)),guard);if(e || guard!=3)return e?e:bad;
            e=timed(r.timing_us.physical,[&]{return a.effects->AEGP_DeleteLayerEffect(fx.value);});if(e)return e;A_long count{};e=a.effects->AEGP_GetLayerNumEffects(layer,&count);
            if(e || count!=total-1)return e?e:damaged;--total;if(i->index<before.index)--before.index;}
        {EffectRef fx{a.effects.value};auto e=renderer(fx.value);if(e)return e;
            if(!request.nodes.empty()){
                GraphBindingRefreshRequest refresh;refresh.plugin=plugin;refresh.renderer=fx.value;refresh.layer=layer;
                e=timed(r.timing_us.refresh,[&]{return a.effects->AEGP_EffectCallGeneric(plugin,fx.value,&time,PF_Cmd_COMPLETELY_GENERAL,&refresh);});
                if(e || refresh.acknowledged!=1 || refresh.status)return e?e:refresh.status?refresh.status:bad;}
            e=a.write(fx.value,kGraphSyncGuardId,0);if(!e)r.guarded=false;return e;}
    };
    r.error=work();
    if(r.error && r.committed){r.cleanup_error=r.error;r.error=0;}
    else if(r.error && started){
        // Restore only this edit's author metadata. Never use the user's Undo
        // stack as a private rollback log. An unverified publisher stays guarded.
        for(auto i=nodes.rbegin();i!=nodes.rend();++i)if(i->touched){A_Err e{};
            if(i->removed){EffectRef fx{a.effects.value};e=a.ref_node(*i,fx.value);if(!e)e=a.write(fx.value,native_nodes::sync_guard_index(static_cast<Kind>(i->type)),0);}
            else e=a.connections(*i,true);if(e && !r.rollback_error)r.rollback_error=e;}
        EffectRef fx{a.effects.value};auto e=renderer(fx.value);Renderer saved;if(!e)e=a.renderer(fx.value,saved);
        const bool unchanged=!e && saved.revision==before.revision && saved.high==before.high && saved.low==before.low;
        if(!e && !r.outcome_unknown)e=a.write(fx.value,kGraphEditCommitId,before.nonce);
        if(!e && !r.outcome_unknown)e=a.write(fx.value,kGraphEditReceiptId,before.receipt);
        if(e && !r.rollback_error)r.rollback_error=e;
        if(!unchanged && !r.rollback_error)r.rollback_error=damaged;
        // The legacy publisher does not certify every destructor rollback.
        // After a failed generic call keep derived state guarded, even if its
        // revision is unchanged. This is not a claim of complete rollback.
        if(!r.rollback_error && !commit_called){e=a.write(fx.value,kGraphSyncGuardId,0);if(!e)r.guarded=false;else r.rollback_error=e;}
    }
    undo.close();return r;
    }catch(...){if(r.committed){r.error=0;r.cleanup_error=damaged;}else if(!r.error)r.error=damaged;
        // Destructors release local refs. Do not guess rollback after an exception.
        r.guarded=started;return r;}
}
NativeDeleteResult delete_native_nodes(SPBasicSuite* basic,AEGP_PluginID plugin,AEGP_LayerH layer,const NativeDeleteRequest& request) noexcept {
    const auto start=EditClock::now();const auto id=++edit_operation;
    auto result=delete_native_nodes_impl(basic,plugin,layer,request);result.operation=id;result.timing_us.total=elapsed_us(start);return result;
}
std::wstring execute_native_delete(SPBasicSuite* basic,AEGP_PluginID plugin,std::wstring_view text,bool patch,bool diagnostics){
    const auto start=EditClock::now();
    NativeDeleteRequest request;if(!(diagnostics?parse_native_request(text,request,true):patch?parse_native_patch(text,request):parse_native_delete(text,request))){NativeDeleteResult r;r.error=bad;return native_delete_reply(r);}
    Suite<AEGP_ProjSuite6> projects(basic,kAEGPProjSuite,kAEGPProjSuiteVersion6);
    Suite<AEGP_ItemSuite9> items(basic,kAEGPItemSuite,kAEGPItemSuiteVersion9);
    Suite<AEGP_CompSuite11> comps(basic,kAEGPCompSuite,kAEGPCompSuiteVersion11);
    Suite<AEGP_LayerSuite9> layers(basic,kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    Suite<AEGP_MemorySuite1> memory(basic,kAEGPMemorySuite,kAEGPMemorySuiteVersion1);
    AEGP_ProjectH project{};AEGP_ItemH item{};AEGP_LayerH layer{};NativeDeleteResult failed;failed.error=bad;
    if(!projects || !items || !comps || !layers || !memory || projects->AEGP_GetProjectByIndex(request.project,&project) || !project)return native_delete_reply(failed);
    {AEGP_MemHandle path{};auto e=projects->AEGP_GetProjectPath(project,&path);
        struct Memory{const AEGP_MemorySuite1* s;AEGP_MemHandle h;~Memory(){if(h)s->AEGP_FreeMemHandle(h);}} owned{memory.value,path};
        AEGP_MemSize bytes{};if(e || !path || memory->AEGP_GetMemHandleSize(path,&bytes) || bytes<2 || bytes>8194 || bytes%2)return native_delete_reply(failed);
        void* data{};if(memory->AEGP_LockMemHandle(path,&data) || !data)return native_delete_reply(failed);
        const auto* text_path=static_cast<const A_UTF16Char*>(data);const auto units=bytes/sizeof(A_UTF16Char);
        const auto* end=std::find(text_path,text_path+units,A_UTF16Char{});
        const bool matches=end!=text_path+units && static_cast<std::size_t>(end-text_path)==request.project_scope.size() &&
            std::equal(text_path,end,request.project_scope.begin());
        e=memory->AEGP_UnlockMemHandle(path);if(e || !matches)return native_delete_reply(failed);}
    if(items->AEGP_GetFirstProjItem(project,&item))return native_delete_reply(failed);
    for(unsigned count=0;item && count<65536;++count){A_long id{};if(items->AEGP_GetItemID(item,&id))return native_delete_reply(failed);
        if(id==request.comp){AEGP_ItemType type{};AEGP_CompH comp{};
            if(items->AEGP_GetItemType(item,&type) || type!=AEGP_ItemType_COMP || comps->AEGP_GetCompFromItem(item,&comp) || !comp ||
                layers->AEGP_GetLayerFromLayerID(comp,request.layer,&layer) || !layer)return native_delete_reply(failed);
            if(diagnostics) {
                Access a(basic,plugin,layer);A_long effect_count{};if(!a.effects || !a.streams || a.effects->AEGP_GetLayerNumEffects(layer,&effect_count) || effect_count<1 || effect_count>4096)return native_delete_reply(failed);
                EffectRef main{a.effects.value};
                for(A_long i=0;i<effect_count;++i){EffectRef fx{a.effects.value};int effect_type{};if(a.effect(i,fx.value) || !fx.value || a.type(fx.value,effect_type))return native_delete_reply(failed);
                    if(effect_type==-1){if(main.value)return native_delete_reply(failed);main.value=fx.value;fx.value=nullptr;}}
                if(!main.value)return native_delete_reply(failed);
                auto out=std::make_unique<GraphDiagnosticsRequest>();out->plugin=plugin;out->renderer=main.value;out->revision=request.revision;out->checksum=request.checksum;const A_Time time{0,1};
                auto e=a.effects->AEGP_EffectCallGeneric(plugin,main.value,&time,PF_Cmd_COMPLETELY_GENERAL,out.get());
                if(e || out->acknowledged!=1 || out->status || out->count>out->issues.size())return L"{\"ok\":false,\"error\":{\"code\":\"graph_diagnostics_unavailable\",\"message\":\"Node diagnostics are unavailable for this graph receipt.\"}}";
                const auto uuid_text=[](const auto& id){constexpr wchar_t chars[]=L"0123456789abcdef";std::wstring s;for(auto b:id.value.bytes){s+=chars[b>>4];s+=chars[b&15];}return s;};
                std::wstring reply=L"{\"ok\":true,\"revision\":"+std::to_wstring(request.revision)+L",\"checksum\":"+std::to_wstring(request.checksum)+L",\"issues\":[";
                for(std::uint32_t i=0;i<out->count;++i){const auto& issue=out->issues[i];if(i)reply+=L",";reply+=L"{\"edge\":\""+uuid_text(issue.edge)+L"\",\"source\":\""+uuid_text(issue.source)+L"\",\"destination\":\""+uuid_text(issue.destination)+L"\",\"code\":\"SF-G"+std::to_wstring(issue.code)+L"\",\"message\":\"";
                    for(auto c:issue.detail){if(!c)break;if(c=='"' || c=='\\')reply+=L'\\';reply+=static_cast<wchar_t>(static_cast<unsigned char>(c));}reply+=L"\"}";}
                return reply+L"]}";
            }
            const auto resolved=elapsed_us(start);auto result=delete_native_nodes(basic,plugin,layer,request);
            result.timing_us.resolve=resolved;result.timing_us.total+=resolved;
            // This transport entry runs on safe Host idle, never inside Clear.
            char message[512]{};
            std::snprintf(message,sizeof(message),"Native delta timing: op=%llu, c=%ld, l=%ld, rev=%ld, totalUs=%llu, resolveUs=%llu, scanUs=%llu, publishUs=%llu, physicalUs=%llu, finalBindingsUs=%llu.",
                static_cast<unsigned long long>(result.operation),long(result.comp),long(result.layer),long(result.revision),
                static_cast<unsigned long long>(result.timing_us.total),static_cast<unsigned long long>(result.timing_us.resolve),
                static_cast<unsigned long long>(result.timing_us.preflight),static_cast<unsigned long long>(result.timing_us.publish),
                static_cast<unsigned long long>(result.timing_us.physical),static_cast<unsigned long long>(result.timing_us.refresh));
            journal_graph_command(0,result.error?result.error:result.cleanup_error,message);
            return native_delete_reply(result);}
        AEGP_ItemH next{};if(items->AEGP_GetNextProjItem(project,item,&next))return native_delete_reply(failed);item=next;}
    return native_delete_reply(failed);
}
namespace {
A_Err project_scope(SPBasicSuite* basic,std::wstring& result) {
    Suite<AEGP_ProjSuite6> projects(basic,kAEGPProjSuite,kAEGPProjSuiteVersion6);
    Suite<AEGP_MemorySuite1> memory(basic,kAEGPMemorySuite,kAEGPMemorySuiteVersion1);
    if(!projects || !memory)return bad;AEGP_ProjectH project{};AEGP_MemHandle path{};
    auto e=projects->AEGP_GetProjectByIndex(0,&project);if(e || !project)return e?e:bad;
    e=projects->AEGP_GetProjectPath(project,&path);
    struct Memory{const AEGP_MemorySuite1* s;AEGP_MemHandle h;~Memory(){if(h)s->AEGP_FreeMemHandle(h);}} owned{memory.value,path};
    AEGP_MemSize bytes{};if(e || !path || (e=memory->AEGP_GetMemHandleSize(path,&bytes)))return e?e:bad;
    if(bytes<2 || bytes>8194 || bytes%2)return bad;void* data{};
    e=memory->AEGP_LockMemHandle(path,&data);if(e || !data)return e?e:bad;
    const auto* text=static_cast<const A_UTF16Char*>(data);const auto* end=std::find(text,text+bytes/2,A_UTF16Char{});
    if(end==text+bytes/2){memory->AEGP_UnlockMemHandle(path);return bad;}
    try{result.assign(text,end);}catch(...){memory->AEGP_UnlockMemHandle(path);throw;}
    return memory->AEGP_UnlockMemHandle(path);
}
A_Err reacquire_target(SPBasicSuite* basic,const NativeDeleteRequest& request,AEGP_LayerH& layer,std::wstring& path) {
    layer=nullptr;auto e=project_scope(basic,path);if(e || path!=request.project_scope)return e?e:bad;
    Suite<AEGP_ProjSuite6> projects(basic,kAEGPProjSuite,kAEGPProjSuiteVersion6);
    Suite<AEGP_ItemSuite9> items(basic,kAEGPItemSuite,kAEGPItemSuiteVersion9);
    Suite<AEGP_CompSuite11> comps(basic,kAEGPCompSuite,kAEGPCompSuiteVersion11);
    Suite<AEGP_LayerSuite9> layers(basic,kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    if(!projects || !items || !comps || !layers)return bad;AEGP_ProjectH project{};AEGP_ItemH item{};
    e=projects->AEGP_GetProjectByIndex(0,&project);if(e || !project)return e?e:bad;
    e=items->AEGP_GetFirstProjItem(project,&item);if(e)return e;
    for(unsigned count=0;item && count<65536;++count){A_long id{};e=items->AEGP_GetItemID(item,&id);if(e)return e;
        if(id==request.comp){AEGP_ItemType type{};AEGP_CompH comp{};
            e=items->AEGP_GetItemType(item,&type);if(!e && type!=AEGP_ItemType_COMP)e=bad;
            if(!e)e=comps->AEGP_GetCompFromItem(item,&comp);if(!e && !comp)e=bad;
            return e?e:layers->AEGP_GetLayerFromLayerID(comp,request.layer,&layer);}
        AEGP_ItemH next{};e=items->AEGP_GetNextProjItem(project,item,&next);if(e)return e;item=next;}
    return item?bad:0;
}
NativeDeleteResult clear_native_selection(SPBasicSuite* basic,AEGP_PluginID plugin,AEGP_LayerH layer,
    NativeDeleteRequest request,AEGP_Command command) noexcept {
    NativeDeleteResult r;r.comp=request.comp;r.layer=request.layer;r.clear_stage=1;
    try {
        Access a(basic,plugin,layer);Suite<AEGP_UtilitySuite6> utility(basic,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
        Suite<AEGP_LayerSuite9> layers(basic,kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
        Suite<AEGP_CommandSuite1> commands(basic,kAEGPCommandSuite,kAEGPCommandSuiteVersion1);
        if(!a.effects || !a.streams || !utility || !layers || !commands || !commands->AEGP_DoCommand){r.error=bad;return r;}
        Renderer before;std::vector<Node> original,current;current.reserve(63);
        std::vector<AuthorNodeId> removed;removed.reserve(63);std::wstring path_check;path_check.reserve(4096);A_long total{};
        r.error=timed(r.timing_us.preflight,[&]{return scan(a,request,before,original,total);});if(r.error)return r;
        r.error=clear_bridge_capacity(original,request.nodes);if(r.error){r.capacity_rejected=true;return r;}
        request.revision=static_cast<A_long>(before.revision);request.checksum=static_cast<std::uint32_t>(before.high)*65536+static_cast<std::uint32_t>(before.low);
        r.error=project_scope(basic,request.project_scope);if(r.error)return r;
        auto snapshot=std::make_unique<NativeClearSnapshot>();snapshot->revision=request.revision;
        snapshot->high=static_cast<A_long>(before.high);snapshot->low=static_cast<A_long>(before.low);
        snapshot->selected_count=static_cast<std::uint32_t>(request.nodes.size());
        for(std::size_t i=0;i<request.nodes.size();++i)snapshot->selected[i]=core_id(request.nodes[i]);
        A_Time time{};r.error=layers->AEGP_GetLayerCurrentTime(layer,AEGP_LTimeMode_LayerTime,&time);if(r.error || time.scale<=0){if(!r.error)r.error=bad;return r;}
        r.clear_stage=2;
        {EffectRef fx{a.effects.value};r.error=a.effect(before.index,fx.value);if(r.error || !fx.value){if(!r.error)r.error=bad;return r;}
            NativeClearRequest capture;capture.plugin=plugin;capture.renderer=fx.value;capture.layer=layer;capture.snapshot=snapshot.get();
            r.error=timed(r.timing_us.capture,[&]{return a.effects->AEGP_EffectCallGeneric(plugin,fx.value,&time,PF_Cmd_COMPLETELY_GENERAL,&capture);});
            if(r.error || capture.acknowledged!=1 || capture.status || !snapshot->captured){if(!r.error)r.error=capture.status?capture.status:bad;return r;}}
        HostGraphMutation mutation;if(!mutation){r.error=bad;return r;}
        snapshot->epoch=SFLD_GraphMutationEpochV1();
        r.error=utility->AEGP_StartUndoGroup("Starfield: clear nodes");if(r.error)return r;
        struct Undo{const AEGP_UtilitySuite6* s;NativeDeleteResult& r;bool open{true};void close(){if(open){r.undo_error=s->AEGP_EndUndoGroup();open=false;}}~Undo(){close();}} undo{utility.value,r};
        const auto finish=[&](){undo.close();return r;};
        r.clear_stage=3;
        {EffectRef fx{a.effects.value};r.error=a.effect(before.index,fx.value);
            if(!r.error && !fx.value)r.error=bad;
            if(!r.error){r.guarded=true;r.error=a.write(fx.value,kGraphSyncGuardId,1);}if(r.error)return finish();}
        // No effect/stream/collection reference spans application command dispatch.
        a.layer=nullptr;layer=nullptr;
        A_Err command_error{};r.clear_stage=4;r.clear_dispatched=true;
        {struct Dispatch{Dispatch(){clear_dispatch_active=true;}~Dispatch(){clear_dispatch_active=false;}} dispatch;
            command_error=timed(r.timing_us.physical,[&]{return commands->AEGP_DoCommand(command);});r.command_error=command_error;}
        r.clear_stage=5;r.error=reacquire_target(basic,request,layer,path_check);if(r.error)return finish();
        if(!layer){r.guarded=false;r.unchanged=true;r.error=command_error;return finish();}
        a.layer=layer;Renderer after;A_long after_total{};r.clear_stage=6;
        r.error=timed(r.timing_us.preflight,[&]{return scan(a,request,after,current,after_total,true,1);});if(r.error)return finish();
        if(after.nonce!=before.nonce || after.receipt!=before.receipt){r.error=bad;return finish();}
        for(const auto& old:original){const auto found=std::find_if(current.begin(),current.end(),[&](const auto& n){return n.id==old.id;});
            if(found==current.end()){
                if(std::find(request.nodes.begin(),request.nodes.end(),old.id)==request.nodes.end()){r.error=bad;return finish();}removed.push_back(old.id);
            }else if(found->type!=old.type || found->count!=old.count || found->words!=old.words){r.error=bad;return finish();}}
        for(const auto& n:current)if(std::none_of(original.begin(),original.end(),[&](const auto& old){return old.id==n.id;})){r.error=bad;return finish();}
        if(after_total!=total-static_cast<A_long>(removed.size())){r.error=bad;return finish();}
        if(removed.empty()) {
            EffectRef fx{a.effects.value};r.error=a.effect(after.index,fx.value);if(!r.error && !fx.value)r.error=bad;
            if(!r.error)r.error=a.write(fx.value,kGraphSyncGuardId,0);
            if(!r.error){r.guarded=false;r.unchanged=true;r.revision=request.revision;r.error=command_error;r.clear_stage=10;}return finish();
        }
        snapshot->removed_count=static_cast<std::uint32_t>(removed.size());
        for(std::size_t i=0;i<removed.size();++i)snapshot->removed[i]=core_id(removed[i]);
        r.error=bridge_connections(current,removed,original);if(r.error)return finish();
        for(auto& n:current)if(changed(n)){r.error=prepare_tail(a,n);if(r.error)return finish();}
        r.clear_stage=7;
        for(auto& n:current)if(changed(n)){n.touched=true;r.error=a.connections(n,false);if(r.error)return finish();}
        {EffectRef fx{a.effects.value};r.error=a.effect(after.index,fx.value);if(!r.error && !fx.value)r.error=bad;if(r.error)return finish();
            const A_long nonce=before.nonce>=1000000?1:static_cast<A_long>(before.nonce)+1;
            r.error=a.write(fx.value,kGraphEditCommitId,nonce);if(r.error)return finish();
            NativeClearRequest publish;publish.magic=0x53464332;publish.plugin=plugin;publish.renderer=fx.value;publish.layer=layer;
            publish.snapshot=snapshot.get();publish.nonce=nonce;r.outcome_unknown=true;r.clear_stage=8;
            r.error=timed(r.timing_us.publish,[&]{return a.effects->AEGP_EffectCallGeneric(plugin,fx.value,&snapshot->time,PF_Cmd_COMPLETELY_GENERAL,&publish);});
            r.committed=publish.acknowledged==1 && publish.status==0;r.outcome_unknown=publish.acknowledged!=1;
            r.revision=r.committed?request.revision+1:0;
            if(!r.error && !r.committed)r.error=publish.status?publish.status:bad;
            if(r.error){if(r.committed){r.cleanup_error=r.error;r.error=0;}return finish();}
            r.revision=request.revision+1;
            GraphBindingRefreshRequest refresh;refresh.plugin=plugin;refresh.renderer=fx.value;refresh.layer=layer;
            r.clear_stage=9;
            auto e=timed(r.timing_us.refresh,[&]{return a.effects->AEGP_EffectCallGeneric(plugin,fx.value,&snapshot->time,PF_Cmd_COMPLETELY_GENERAL,&refresh);});
            if(!e && (refresh.acknowledged!=1 || refresh.status))e=refresh.status?refresh.status:bad;
            if(!e)e=a.write(fx.value,kGraphSyncGuardId,0);
            if(e){r.cleanup_error=e;return finish();}r.guarded=false;r.cleanup_error=command_error;r.clear_stage=10;
        }
        return finish();
    }catch(...){if(r.committed)r.cleanup_error=damaged;else r.error=damaged;return r;}
}
}
bool native_clear_dispatch_active() noexcept{return clear_dispatch_active;}
bool delete_selected_native_nodes(SPBasicSuite* basic,AEGP_PluginID plugin,NativeDeleteResult& result,bool execute,
    AEGP_Command clear_command,A_Boolean* handled) noexcept try {
    result={};
    struct Total {
        NativeDeleteResult& result;std::uint64_t id{++edit_operation};EditClock::time_point start{EditClock::now()};
        ~Total(){result.operation=id;result.timing_us.total=elapsed_us(start);}
    } total{result};
    // Clear can reenter an existing UI transaction. Drop it before inspecting a
    // partially mutated parade; never hand it back to AE for physical deletion.
    HostUiExclusion exclusion;if(!exclusion){result.error=bad;return true;}
    Suite<AEGP_ItemSuite9> items(basic,kAEGPItemSuite,kAEGPItemSuiteVersion9);Suite<AEGP_CompSuite11> comps(basic,kAEGPCompSuite,kAEGPCompSuiteVersion11);
    Suite<AEGP_CollectionSuite2> collections(basic,kAEGPCollectionSuite,kAEGPCollectionSuiteVersion2);
    Suite<AEGP_LayerSuite9> layers(basic,kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    Suite<AEGP_DynamicStreamSuite4> dynamic(basic,kAEGPDynamicStreamSuite,kAEGPDynamicStreamSuiteVersion4);
    if(!items || !comps || !collections || !layers || !dynamic)return false;
    AEGP_ItemH item{};AEGP_ItemType type{};AEGP_CompH comp{};AEGP_Collection2H raw{};
    if(items->AEGP_GetActiveItem(&item) || !item || items->AEGP_GetItemType(item,&type) || type!=AEGP_ItemType_COMP ||
        comps->AEGP_GetCompFromItem(item,&comp) || !comp || comps->AEGP_GetNewCollectionFromCompSelection(plugin,comp,&raw) || !raw)return false;
    struct Collection{const AEGP_CollectionSuite2* s;AEGP_Collection2H h;
        A_Err close(){if(!h)return 0;const auto value=h;h=nullptr;return s->AEGP_DisposeCollection(value);}
        ~Collection(){(void)close();}} owned{collections.value,raw};
    A_u_long count{};if(collections->AEGP_GetCollectionNumItems(raw,&count) || !count || count>128)return false;
    AEGP_LayerH layer{},enclosing{};NativeDeleteRequest request;
    Access a(basic,plugin,nullptr);if(!a.effects || !a.streams)return false;
    for(A_u_long i=0;i<count;++i){AEGP_CollectionItemV2 selected{};if(collections->AEGP_GetCollectionItemByIndex(raw,i,&selected))return false;
        if(selected.type==AEGP_CollectionItemType_LAYER){if(enclosing && enclosing!=selected.u.layer.layerH)return false;enclosing=selected.u.layer.layerH;continue;}
        AEGP_EffectCollectionItem effect{};
        if(selected.type==AEGP_CollectionItemType_EFFECT)effect=selected.u.effect;
        else if(clear_command && selected.type==AEGP_CollectionItemType_STREAM && selected.u.stream.type==AEGP_StreamCollectionItemType_EFFECT)
            effect=selected.u.stream.u.effect_stream.effect;
        else if(clear_command && selected.type==AEGP_CollectionItemType_KEYFRAME && selected.u.keyframe.stream_coll.type==AEGP_StreamCollectionItemType_EFFECT)
            effect=selected.u.keyframe.stream_coll.u.effect_stream.effect;
        else if(selected.type==AEGP_CollectionItemType_STREAMREF && selected.stream_refH){
            char label[AEGP_MAX_STREAM_MATCH_NAME_SIZE]{};AEGP_StreamGroupingType grouping{};
            auto group=selected.stream_refH;
            struct Parents{const AEGP_StreamSuite6* s;std::array<AEGP_StreamRefH,8> refs{};
                ~Parents(){for(auto r:refs)if(r)s->AEGP_DisposeStream(r);}} parents{a.streams.value};
            unsigned depth{};
            for(;;){if(dynamic->AEGP_GetMatchName(group,label))return false;
                if(kind(label)>=0)break;
                if(!clear_command || depth==parents.refs.size() ||
                    dynamic->AEGP_GetNewParentStreamRef(plugin,group,&parents.refs[depth]) || !parents.refs[depth])return false;
                group=parents.refs[depth++];}
            if(dynamic->AEGP_GetStreamGroupingType(group,&grouping) || grouping==AEGP_StreamGroupingType_LEAF)return false;
            StreamRef parade{a.streams.value},root{a.streams.value};
            if(dynamic->AEGP_GetNewParentStreamRef(plugin,group,&parade.value) || !parade.value ||
                dynamic->AEGP_GetMatchName(parade.value,label) || std::strcmp(label,"ADBE Effect Parade") ||
                dynamic->AEGP_GetStreamIndexInParent(group,&effect.index) ||
                dynamic->AEGP_GetNewParentStreamRef(plugin,parade.value,&root.value) || !root.value)return false;
            std::int32_t root_id{};if(a.streams->AEGP_GetUniqueStreamID(root.value,&root_id))return false;A_long number{};
            if(layers->AEGP_GetCompNumLayers(comp,&number) || number<0 || number>4096)return false;
            for(A_long n=0;n<number;++n){AEGP_LayerH candidate{};StreamRef stream{a.streams.value};std::int32_t id{};
                if(layers->AEGP_GetCompLayerByIndex(comp,n,&candidate) || !candidate ||
                    dynamic->AEGP_GetNewStreamRefForLayer(plugin,candidate,&stream.value) || !stream.value || a.streams->AEGP_GetUniqueStreamID(stream.value,&id))return false;
                if(id==root_id){if(effect.layerH)return false;effect.layerH=candidate;}}
        }else return false;
        if(!effect.layerH || (layer && layer!=effect.layerH))return false;layer=effect.layerH;a.layer=layer;
        EffectRef fx{a.effects.value};int t{};AuthorNodeId id{};
        if(a.effect(effect.index,fx.value) || !fx.value || a.type(fx.value,t) || t<0 || a.id(fx.value,t,id))return false;
        if(std::find(request.nodes.begin(),request.nodes.end(),id)==request.nodes.end())request.nodes.push_back(id);
    }
    if(!layer || (enclosing && enclosing!=layer) || request.nodes.empty() || request.nodes.size()>63)return false;
    // The complete owned set is already identified. Notification-ID failures
    // must consume the command, not expose those sources to AE's default Clear.
    auto target_error=items->AEGP_GetItemID(item,&request.comp);
    if(!target_error)target_error=layers->AEGP_GetLayerID(layer,&request.layer);
    if(target_error){result.error=target_error;return true;}
    result.comp=request.comp;result.layer=request.layer;
    // Collections own their STREAMREFs. Dispose before any parade mutation,
    // not merely before returning from this outer command handler.
    if(const auto error=owned.close();error){result.error=error;return true;}
    if(clear_command){if(handled)*handled=TRUE;result=clear_native_selection(basic,plugin,layer,std::move(request),clear_command);return true;}
    if(!execute){result.error=bad;return true;}
    // A recognized set is consumed even if busy/failed, never default-deleted.
    result=delete_native_nodes(basic,plugin,layer,request);return true;
}catch(...){return false;}
}
