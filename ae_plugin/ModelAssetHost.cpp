#include "ModelAssetHost.hpp"
#include "UiExclusionHost.hpp"
#include "ScriptDiagnostic.hpp"
#include "ModelAssetMessage.hpp"
#include "NodeRecord.hpp"
#include "EffectGraphBackup.hpp"
#include "Parameters.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <string_view>
#include <stdexcept>
#include <thread>
#include <vector>

namespace starfield::adapter {
namespace {
constexpr std::size_t mesh_limit=8*1024*1024,page_bytes=32768;
SPBasicSuite* basic{};
AEGP_PluginID plugin{};
std::thread::id ui_thread;
bool queued{},stopped{},running{};
struct Session {
    std::string id;
    std::vector<std::uint8_t> bytes;
    std::size_t page{};
    std::chrono::steady_clock::time_point expires{};
};
Session session;
template<class T> struct Suite {
    const char* name;A_long version;const T* value{};
    Suite(const char* n,A_long v):name(n),version(v){if(basic)basic->AcquireSuite(n,v,reinterpret_cast<const void**>(&value));}
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
};
// Execute only fixed entry points with validated hex IDs/bytes and numeric
// arguments. Script results/errors are copied and disposed in this callback.
bool script(const std::string& body,std::string& output) {
    output.clear();
    Suite<AEGP_UtilitySuite6> utility(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    Suite<AEGP_MemorySuite1> memory(kAEGPMemorySuite,kAEGPMemorySuiteVersion1);
    if(!utility.value || !memory.value)return false;
    A_Boolean available{};
    if(utility.value->AEGP_IsScriptingAvailable(&available) || !available)return false;
    AEGP_MemHandle result{},error{};
    struct Handles {const AEGP_MemorySuite1* suite;AEGP_MemHandle& result;AEGP_MemHandle& error;
        ~Handles(){if(error)suite->AEGP_FreeMemHandle(error);if(result)suite->AEGP_FreeMemHandle(result);}} handles{memory.value,result,error};
    const auto code=utility.value->AEGP_ExecuteScript(plugin,body.c_str(),FALSE,&result,&error);
    if(code || !script_diagnostic_empty(memory.value,error) || !result)return false;
    AEGP_MemSize size{};
    if(memory.value->AEGP_GetMemHandleSize(result,&size) || size==0 || size>2048)return false;
    void* data{};
    if(memory.value->AEGP_LockMemHandle(result,&data) || !data)return false;
    struct Lock {const AEGP_MemorySuite1* suite;AEGP_MemHandle handle;~Lock(){suite->AEGP_UnlockMemHandle(handle);}} lock{memory.value,result};
    const auto* chars=static_cast<const char*>(data);
    const auto* end=static_cast<const char*>(std::memchr(chars,0,size));
    if(!end)return false;
    output.assign(chars,end);return true;
}
bool call_script(const char* name,const std::string& arguments,std::string& output) {
    return script("(typeof "+std::string(name)+"==='function'?"+name+"("+arguments+"):'0')",output);
}
std::string hex_literal(const std::string& hex){return "'"+hex+"'";}
void clear_session(){session=Session{};}
void fail(const std::string& id,ModelAssetError error,std::int32_t host_error=0) {
    std::string ignored;
    (void)call_script("SFLD_modelAssetHostFail",hex_literal(id)+","+std::to_string(static_cast<std::uint32_t>(error))+","+std::to_string(host_error),ignored);
    clear_session();
}
bool is_hex(std::string_view value) {
    return value.size()==32 && std::all_of(value.begin(),value.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});
}
template<class T> bool integer(std::string_view field,T& value) {
    const auto parsed=std::from_chars(field.data(),field.data()+field.size(),value);
    return !field.empty() && parsed.ec==std::errc{} && parsed.ptr==field.data()+field.size();
}
struct Target {std::string id;A_long project{},comp{},layer{};ModelAssetExportRequest request;};
bool parse(std::string_view line,Target& target) {
    std::array<std::string_view,7> fields{};
    for(std::size_t i=0;i<fields.size();++i){const auto next=line.find('|');
        if((i+1==fields.size())!=(next==std::string_view::npos))return false;
        fields[i]=line.substr(0,next);if(next!=std::string_view::npos)line.remove_prefix(next+1);}
    if(!is_hex(fields[0]) || !is_hex(fields[4]) || fields[4]==std::string(32,'0') ||
        !integer(fields[1],target.project) || target.project<0 || !integer(fields[2],target.comp) || target.comp<=0 ||
        !integer(fields[3],target.layer) || target.layer<=0 || !integer(fields[5],target.request.expected_source) ||
        target.request.expected_source<1 || target.request.expected_source>2 || !integer(fields[6],target.request.expected_revision) ||
        target.request.expected_revision>2147483647u)return false;
    target.id=fields[0];
    for(std::size_t i=0;i<16;++i){unsigned byte{};const auto field=fields[4].substr(i*2,2);
        const auto parsed=std::from_chars(field.data(),field.data()+2,byte,16);if(parsed.ec!=std::errc{})return false;
        target.request.expected_uuid[i]=static_cast<std::uint8_t>(byte);}
    return true;
}
std::int32_t receive(void* context,const std::uint8_t* bytes,std::uint32_t count) noexcept try {
    auto& copy=*static_cast<std::vector<std::uint8_t>*>(context);
    if(!bytes || count==0 || count>mesh_limit || !copy.empty())return 1;
    copy.assign(bytes,bytes+count);return 0;
} catch(...){return 1;}
bool read_guard(const AEGP_StreamSuite6* streams,AEGP_EffectRefH effect,const A_Time& time,A_long index,double& guard) {
    AEGP_StreamRefH ref{};
    const auto acquired=streams->AEGP_GetNewEffectStreamByIndex(plugin,effect,index,&ref);
    struct Ref {const AEGP_StreamSuite6* suite;AEGP_StreamRefH ref;~Ref(){if(ref)suite->AEGP_DisposeStream(ref);}} owned{streams,ref};
    if(acquired || !ref)return false;
    AEGP_StreamType type{};AEGP_StreamValue2 value{};
    if(streams->AEGP_GetStreamType(ref,&type) || type!=AEGP_StreamType_OneD ||
       streams->AEGP_GetNewStreamValue(plugin,ref,AEGP_LTimeMode_LayerTime,&time,TRUE,&value))return false;
    guard=value.val.one_d;return !streams->AEGP_DisposeStreamValue(&value) && std::isfinite(guard);
}
bool read_uuid(const AEGP_StreamSuite6* streams,AEGP_EffectRefH effect,const A_Time& time,const Target& target) {
    for(A_long part=0;part<8;++part){
        AEGP_StreamRefH ref{};
        if(streams->AEGP_GetNewEffectStreamByIndex(plugin,effect,native_nodes::uuid_first_index(native_nodes::Kind::model)+part,&ref) || !ref)return false;
        struct Ref {const AEGP_StreamSuite6* suite;AEGP_StreamRefH ref;~Ref(){suite->AEGP_DisposeStream(ref);}} owned{streams,ref};
        AEGP_StreamType type{};AEGP_StreamValue2 value{};
        if(streams->AEGP_GetStreamType(ref,&type) || type!=AEGP_StreamType_OneD ||
            streams->AEGP_GetNewStreamValue(plugin,ref,AEGP_LTimeMode_LayerTime,&time,TRUE,&value))return false;
        const auto number=value.val.one_d;
        const auto code=streams->AEGP_DisposeStreamValue(&value);
        const auto expected=(std::uint32_t(target.request.expected_uuid[part*2])<<8)|target.request.expected_uuid[part*2+1];
        if(code || number!=expected)return false;
    }return true;
}
bool export_target(Target& target,std::vector<std::uint8_t>& copy) {
    Suite<AEGP_ProjSuite6> projects(kAEGPProjSuite,kAEGPProjSuiteVersion6);
    Suite<AEGP_ItemSuite9> items(kAEGPItemSuite,kAEGPItemSuiteVersion9);
    Suite<AEGP_CompSuite11> comps(kAEGPCompSuite,kAEGPCompSuiteVersion11);
    Suite<AEGP_LayerSuite9> layers(kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    Suite<AEGP_EffectSuite4> effects(kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
    Suite<AEGP_StreamSuite6> streams(kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
    if(!projects.value || !items.value || !comps.value || !layers.value || !effects.value || !streams.value)return false;
    AEGP_ProjectH project{};AEGP_ItemH root{},item{};A_long root_id{};
    if(projects.value->AEGP_GetProjectByIndex(0,&project) || !project ||
       projects.value->AEGP_GetProjectRootFolder(project,&root) || !root || items.value->AEGP_GetItemID(root,&root_id) ||
       root_id!=target.project || items.value->AEGP_GetFirstProjItem(project,&item))return false;
    AEGP_CompH comp{};
    for(unsigned visits=0;item && visits<65536;++visits){A_long id{};AEGP_ItemType type{};
        if(items.value->AEGP_GetItemID(item,&id))return false;
        if(id==target.comp){if(items.value->AEGP_GetItemType(item,&type) || type!=AEGP_ItemType_COMP ||
            comps.value->AEGP_GetCompFromItem(item,&comp))return false;break;}
        AEGP_ItemH next{};if(items.value->AEGP_GetNextProjItem(project,item,&next))return false;item=next;}
    AEGP_LayerH layer{};A_long count{};
    if(!comp || layers.value->AEGP_GetLayerFromLayerID(comp,target.layer,&layer) || !layer ||
        effects.value->AEGP_GetLayerNumEffects(layer,&count) || count<1 || count>4096)return false;
    A_Time time{};if(layers.value->AEGP_GetLayerCurrentTime(layer,AEGP_LTimeMode_LayerTime,&time) || !time.scale)return false;
    // Keep only numeric indices after this scan. All owning refs are released
    // before the chosen effect is reacquired for the synchronous message.
    A_long model_index=-1;unsigned renderers{};
    for(A_long index=0;index<count;++index){AEGP_EffectRefH effect{};
        if(effects.value->AEGP_GetLayerEffectByIndex(plugin,layer,index,&effect) || !effect)return false;
        struct Ref {const AEGP_EffectSuite4* suite;AEGP_EffectRefH ref;~Ref(){suite->AEGP_DisposeEffect(ref);}} owned{effects.value,effect};
        AEGP_InstalledEffectKey key{};A_char match[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
        if(effects.value->AEGP_GetInstalledKeyFromLayerEffect(effect,&key) || effects.value->AEGP_GetEffectMatchName(key,match))return false;
        const bool renderer=!std::strcmp(match,"org.starfieldfx.particle"),model=!std::strcmp(match,"org.starfieldfx.node.model");
        if(renderer || model){double guard{};
            if(!read_guard(streams.value,effect,time,renderer?kGraphSyncGuardId:native_nodes::sync_guard_index(native_nodes::Kind::model),guard))return false;
            if(guard==transaction_backup_guard)continue;
            if(guard!=0)return false;
            if(renderer)++renderers;
            if(model && read_uuid(streams.value,effect,time,target)){if(model_index!=-1)return false;model_index=index;}}
    }
    if(renderers!=1 || model_index<0)return false;
    AEGP_EffectRefH effect{};
    if(effects.value->AEGP_GetLayerEffectByIndex(plugin,layer,model_index,&effect) || !effect)return false;
    struct Ref {const AEGP_EffectSuite4* suite;AEGP_EffectRefH ref;~Ref(){suite->AEGP_DisposeEffect(ref);}} owned{effects.value,effect};
    target.request.sink_context=&copy;target.request.write_bytes=receive;
    const auto code=effects.value->AEGP_EffectCallGeneric(plugin,effect,&time,PF_Cmd_COMPLETELY_GENERAL,&target.request);
    if(code){target.request.error=ModelAssetError::host_error;target.request.host_error=code;return false;}
    if(target.request.acknowledged!=1){target.request.error=ModelAssetError::unavailable;return false;}
    if(target.request.error!=ModelAssetError::none)return false;
    if(target.request.payload_bytes!=copy.size() || (target.request.expected_revision!=0 && copy.empty())){
        target.request.error=ModelAssetError::invalid_geometry;return false;}
    return true;
}
std::string number(double value){char buffer[64]{};const auto result=std::to_chars(buffer,buffer+sizeof(buffer),value,std::chars_format::general,17);
    if(result.ec!=std::errc{} || !std::isfinite(value))throw std::runtime_error("invalid Model bounds");return {buffer,result.ptr};}
void begin() {
    std::string line;
    if(!call_script("SFLD_modelAssetHostRequest","",line) || line=="0" || line.empty())return;
    Target target;
    if(!parse(line,target))return;
    // Preserve the validated ID before any allocation/host call so exceptions
    // can fail the matching volatile request instead of leaving it queued.
    session.id=target.id;
    std::vector<std::uint8_t> copy;
    if(!export_target(target,copy)){fail(target.id,target.request.error,target.request.host_error);return;}
    std::string bounds="[";
    for(unsigned i=0;i<6;++i){if(i)bounds+=",";bounds+=number(target.request.bounds[i]);}bounds+="]";
    std::string ack;
    if(!call_script("SFLD_modelAssetHostBegin",hex_literal(target.id)+","+std::to_string(copy.size())+","+bounds,ack) || ack!="1"){
        clear_session();return;}
    session.id=std::move(target.id);session.bytes=std::move(copy);session.page=0;
    session.expires=std::chrono::steady_clock::now()+std::chrono::seconds(60);
}
}
void initialize_model_asset_host(SPBasicSuite* suites,AEGP_PluginID id) noexcept {
    basic=suites;plugin=id;ui_thread=std::this_thread::get_id();stopped=false;queued=false;running=false;clear_session();
}
void queue_model_asset_export() noexcept {if(!stopped && std::this_thread::get_id()==ui_thread)queued=true;}
void stop_model_asset_host() noexcept {stopped=true;queued=false;if(!running)clear_session();}
bool step_model_asset_host() noexcept {
    if(stopped || running || std::this_thread::get_id()!=ui_thread || (!queued && session.id.empty()))return false;
    HostUiExclusion exclusion;if(!exclusion)return true;
    running=true;struct Running {~Running(){running=false;if(stopped)clear_session();}} active;
    try {
    if(session.id.empty()){queued=false;begin();}
    if(stopped)return false;
    if(session.id.empty())return queued;
    std::string ack;
    if(std::chrono::steady_clock::now()>session.expires ||
       !call_script("SFLD_modelAssetHostContinue",hex_literal(session.id),ack) || ack!="1"){clear_session();return queued;}
    const auto started=std::chrono::steady_clock::now();
    for(unsigned pages=0;pages<8 && session.page*page_bytes<session.bytes.size();++pages){
        const auto start=session.page*page_bytes,count=std::min(page_bytes,session.bytes.size()-start);
        constexpr char hex[]="0123456789abcdef";std::string text(count*2,'0');
        for(std::size_t i=0;i<count;++i){const auto byte=session.bytes[start+i];text[i*2]=hex[byte>>4];text[i*2+1]=hex[byte&15];}
        if(!call_script("SFLD_modelAssetHostChunk",hex_literal(session.id)+","+std::to_string(session.page)+","+hex_literal(text),ack) || ack!="1"){
            clear_session();return queued;}
        if(stopped)return false;
        ++session.page;
        if(std::chrono::steady_clock::now()-started>std::chrono::milliseconds(25))break;
    }
    if(session.page*page_bytes>=session.bytes.size()){
        (void)call_script("SFLD_modelAssetHostFinish",hex_literal(session.id),ack);clear_session();return queued;}
    return true;
    } catch(...){if(!session.id.empty())try{fail(session.id,ModelAssetError::allocation_failed);}catch(...){clear_session();}return queued;}
}
}
