#include "ModelTransactionHost.hpp"
#include "ModelGraphTransaction.hpp"
#include "UiExclusionHost.hpp"
#include "ScriptDiagnostic.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace starfield::adapter {
namespace {
constexpr std::size_t page_bytes=32768,mesh_limit=8*1024*1024,total_limit=64*1024*1024;
SPBasicSuite* basic{};AEGP_PluginID plugin{};std::thread::id ui_thread;
bool stopped{},queued{},running{};
unsigned request_failures{};
struct Asset {ModelAssetRestore restore;std::vector<std::uint8_t> bytes;std::size_t length{};};
struct Session {
    std::string id;A_long project{},comp{},layer{};AEGP_LayerH target{};
    std::vector<ModelTransactionId> desired;std::vector<Asset> assets;
    std::size_t index{},metadata{},total{};
    std::chrono::steady_clock::time_point expires{};
};
Session session;
template<class T> struct Suite {
    const char* name;int version;const T* value{};
    Suite(const char* n,int v):name(n),version(v){if(basic)basic->AcquireSuite(n,v,reinterpret_cast<const void**>(&value));}
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
};
bool call(const char* name,const std::string& args,std::string& output,std::size_t limit=8192) {
    output.clear();Suite<AEGP_UtilitySuite6> utility(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    Suite<AEGP_MemorySuite1> memory(kAEGPMemorySuite,kAEGPMemorySuiteVersion1);
    if(!utility.value || !memory.value)return false;
    A_Boolean available{};if(utility.value->AEGP_IsScriptingAvailable(&available) || !available)return false;
    const auto script="(typeof "+std::string(name)+"==='function'?"+name+"("+args+"):'0')";
    AEGP_MemHandle result{},error{};
    struct Handles {const AEGP_MemorySuite1* suite;AEGP_MemHandle& result;AEGP_MemHandle& error;
        ~Handles(){if(error)suite->AEGP_FreeMemHandle(error);if(result)suite->AEGP_FreeMemHandle(result);}} handles{memory.value,result,error};
    if(utility.value->AEGP_ExecuteScript(plugin,script.c_str(),FALSE,&result,&error) ||
       !script_diagnostic_empty(memory.value,error) || !result)return false;
    AEGP_MemSize size{};if(memory.value->AEGP_GetMemHandleSize(result,&size) || !size || size>limit+1)return false;
    void* data{};if(memory.value->AEGP_LockMemHandle(result,&data) || !data)return false;
    struct Lock {const AEGP_MemorySuite1* suite;AEGP_MemHandle value;~Lock(){suite->AEGP_UnlockMemHandle(value);}} lock{memory.value,result};
    const auto* chars=static_cast<const char*>(data);const auto* end=static_cast<const char*>(std::memchr(chars,0,size));
    if(!end)return false;output.assign(chars,end);return true;
}
std::string quoted_id(){return "'"+session.id+"'";}
bool id(std::string_view hex,ModelTransactionId& value,bool output=false) noexcept {
    if(hex.size()!=32)return false;
    for(std::size_t i=0;i<16;++i){unsigned byte{};const auto field=hex.substr(i*2,2);
        if(!std::all_of(field.begin(),field.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}))return false;
        const auto parsed=std::from_chars(field.data(),field.data()+2,byte,16);if(parsed.ec!=std::errc{})return false;
        value[i]=static_cast<std::uint8_t>(byte);}
    ModelTransactionId fixed{};fixed.back()=0xff;
    return std::any_of(value.begin(),value.end(),[](auto b){return b!=0;}) && (output || value!=fixed);
}
template<class T>bool integer(std::string_view field,T& out) noexcept {
    const auto parsed=std::from_chars(field.data(),field.data()+field.size(),out);
    return !field.empty() && parsed.ec==std::errc{} && parsed.ptr==field.data()+field.size();
}
template<std::size_t N>bool split(std::string_view text,std::array<std::string_view,N>& fields) noexcept {
    for(std::size_t i=0;i<N;++i){const auto end=text.find('|');
        if((i+1==N)!=(end==std::string_view::npos))return false;
        fields[i]=text.substr(0,end);if(end!=std::string_view::npos)text.remove_prefix(end+1);}
    return true;
}
bool request(std::string_view text) {
    std::array<std::string_view,6> fields{};ModelTransactionId transaction{};std::size_t count{};
    if(!split(text,fields) || !id(fields[0],transaction))return false;
    session.id=fields[0];
    // AE's project root item can have ID zero. Every target lookup still compares
    // it with the current root; composition and layer IDs remain positive.
    if(!integer(fields[1],session.project) || session.project<0 || !integer(fields[2],session.comp) || session.comp<=0 ||
       !integer(fields[3],session.layer) || session.layer<=0 || !integer(fields[4],count) || count>63)return false;
    auto ids=fields[5];
    while(!ids.empty()) {const auto comma=ids.find(',');ModelTransactionId desired{};
        if(session.desired.size()==64 || !id(ids.substr(0,comma),desired,true) ||
           std::find(session.desired.begin(),session.desired.end(),desired)!=session.desired.end())return false;
        session.desired.push_back(desired);
        if(comma==std::string_view::npos)break;
        ids.remove_prefix(comma+1);if(ids.empty())return false;
    }
    if(session.desired.empty())return false;
    session.assets.resize(count);session.expires=std::chrono::steady_clock::now()+std::chrono::minutes(5);return true;
}
bool descriptor(std::string_view text,Asset& asset) {
    std::array<std::string_view,10> fields{};
    if(!split(text,fields) || !id(fields[0],asset.restore.node_id) ||
       std::find(session.desired.begin(),session.desired.end(),asset.restore.node_id)==session.desired.end() ||
       !integer(fields[1],asset.restore.source) || (asset.restore.source!=1 && asset.restore.source!=2) ||
       !integer(fields[2],asset.restore.revision) || asset.restore.revision>2147483647u ||
       !integer(fields[3],asset.length) || asset.length>mesh_limit || asset.length>total_limit-session.total ||
       (asset.restore.revision==0?asset.length!=0:asset.length<32))return false;
    for(std::size_t i=0;i<session.metadata;++i)if(session.assets[i].restore.node_id==asset.restore.node_id)return false;
    for(unsigned axis=0;axis<6;++axis){auto field=fields[4+axis];double value{};
        const auto parsed=std::from_chars(field.data(),field.data()+field.size(),value,std::chars_format::general);
        if(field.empty() || parsed.ec!=std::errc{} || parsed.ptr!=field.data()+field.size() || !std::isfinite(value) || std::abs(value)>1e9 ||
           (axis>=3 && value<asset.restore.bounds[axis-3]))return false;
        asset.restore.bounds[axis]=value;}
    session.total+=asset.length;return true;
}
bool append_hex(Asset& asset,std::string_view text) {
    const auto count=std::min(page_bytes,asset.length-asset.bytes.size());
    if(text.size()!=count*2 || !count)return false;
    for(std::size_t i=0;i<text.size();i+=2){unsigned byte{};auto field=text.substr(i,2);
        if(!std::all_of(field.begin(),field.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}))return false;
        const auto parsed=std::from_chars(field.data(),field.data()+2,byte,16);if(parsed.ec!=std::errc{})return false;
        asset.bytes.push_back(static_cast<std::uint8_t>(byte));}return true;
}
bool target(AEGP_LayerH& out,A_Time& time) {
    Suite<AEGP_ProjSuite6> projects(kAEGPProjSuite,kAEGPProjSuiteVersion6);
    Suite<AEGP_ItemSuite9> items(kAEGPItemSuite,kAEGPItemSuiteVersion9);
    Suite<AEGP_CompSuite11> comps(kAEGPCompSuite,kAEGPCompSuiteVersion11);
    Suite<AEGP_LayerSuite9> layers(kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    if(!projects.value || !items.value || !comps.value || !layers.value)return false;
    AEGP_ProjectH project{};AEGP_ItemH root{},item{};A_long root_id{};
    if(projects.value->AEGP_GetProjectByIndex(0,&project) || !project || projects.value->AEGP_GetProjectRootFolder(project,&root) ||
       !root || items.value->AEGP_GetItemID(root,&root_id) || root_id!=session.project ||
       items.value->AEGP_GetFirstProjItem(project,&item))return false;
    AEGP_CompH comp{};
    for(unsigned visits=0;item && visits<65536;++visits){A_long item_id{};AEGP_ItemType type{};
        if(items.value->AEGP_GetItemID(item,&item_id))return false;
        if(item_id==session.comp){if(items.value->AEGP_GetItemType(item,&type) || type!=AEGP_ItemType_COMP ||
            comps.value->AEGP_GetCompFromItem(item,&comp))return false;break;}
        AEGP_ItemH next{};if(items.value->AEGP_GetNextProjItem(project,item,&next))return false;item=next;}
    if(!comp || layers.value->AEGP_GetLayerFromLayerID(comp,session.layer,&out) || !out ||
       layers.value->AEGP_GetLayerCurrentTime(out,AEGP_LTimeMode_LayerTime,&time) || !time.scale)return false;
    return true;
}
std::int32_t cancelled(void*) noexcept{return stopped || std::chrono::steady_clock::now()>session.expires;}
A_Err invoke(const char* name) {
    if(cancelled(nullptr))return PF_Interrupt_CANCEL;
    AEGP_LayerH current{};A_Time time{};
    if(!target(current,time) || current!=session.target)return PF_Err_BAD_CALLBACK_PARAM;
    std::string ack;return call(name,quoted_id(),ack) && ack=="1"?0:PF_Err_BAD_CALLBACK_PARAM;
}
A_Err prepare(void*){return invoke("SFLD_modelTransactionHostPrepare");}
A_Err commit(void*){return invoke("SFLD_modelTransactionHostCommit");}
void finish(const ModelGraphTransactionResult& result) {
    if(!session.id.empty()){std::string ack;
        const auto numbers="["+std::to_string(result.committed?1:0)+","+std::to_string(static_cast<unsigned>(result.stage))+","+
            std::to_string(result.error)+","+std::to_string(result.rollback_error)+","+std::to_string(result.asset_rollback_error)+","+
            std::to_string(result.cleanup_error)+","+std::to_string(result.undo_error)+","+
            std::to_string(static_cast<unsigned>(result.asset_error))+","+std::to_string(result.asset_index)+"]";
        (void)call("SFLD_modelTransactionHostResult",quoted_id()+","+numbers,ack);}
    session=Session{};
}
void failed(A_Err error) {ModelGraphTransactionResult result;result.error=error;finish(result);}
void apply() {
    ModelGraphTransactionPlan plan;plan.basic=basic;plan.plugin=plugin;
    if(!target(plan.layer,plan.time) || !id(session.id,plan.id)){failed(PF_Err_BAD_CALLBACK_PARAM);return;}
    session.target=plan.layer;std::vector<ModelAssetRestore> assets;assets.reserve(session.assets.size());
    for(auto& asset:session.assets){asset.restore.mesh=asset.bytes;assets.push_back(asset.restore);}
    plan.desired_ids=session.desired;plan.assets=assets;plan.prepare=prepare;plan.commit=commit;plan.is_cancelled=cancelled;
    // Fence the entire executor, including its backup writes before prepare.
    // Polling/reloading must never discard a session while rollback is possible.
    if(invoke("SFLD_modelTransactionHostBegin")){failed(PF_Err_BAD_CALLBACK_PARAM);return;}
    finish(apply_model_graph_transaction(plan));
}
}
void initialize_model_transaction_host(SPBasicSuite* suites,AEGP_PluginID id) noexcept {
    basic=suites;plugin=id;ui_thread=std::this_thread::get_id();stopped=queued=running=false;request_failures=0;session=Session{};
}
void queue_model_graph_transaction() noexcept {if(!stopped && std::this_thread::get_id()==ui_thread){if(!queued)request_failures=0;queued=true;}}
void stop_model_transaction_host() noexcept {stopped=true;queued=false;if(!running)session=Session{};}
bool step_model_transaction_host() noexcept {
    if(stopped || running || std::this_thread::get_id()!=ui_thread || (!queued && session.id.empty()))return false;
    HostUiExclusion exclusion;if(!exclusion)return true;
    try {
    running=true;struct Running {~Running(){running=false;}} scope;
    std::string text;
    if(session.id.empty()){
        // Request is read-only. A failed SDK result must not consume a staged
        // job whose identity we never received; its script deadline bounds retry.
        if(!call("SFLD_modelTransactionHostRequest","",text))return queued=++request_failures<3;
        queued=false;
        if(text=="0" || text.empty())return false;
        if(!request(text)){failed(PF_Err_BAD_CALLBACK_PARAM);return queued;}
        // From here onward the ID is owned, so even a lost claim acknowledgement
        // can receive a terminal failure without guessing another job's ID.
        if(!call("SFLD_modelTransactionHostClaim",quoted_id(),text) || text!="1"){
            failed(PF_Err_BAD_CALLBACK_PARAM);return queued;}}
    if(cancelled(nullptr) || !call("SFLD_modelTransactionHostContinue",quoted_id(),text) || text!="1"){
        failed(PF_Interrupt_CANCEL);return queued;}
    const auto started=std::chrono::steady_clock::now();
    // Validate all descriptors and the aggregate budget before copying a single
    // payload page. Each idle callback still has a bounded work/time slice.
    for(unsigned work=0;work<8 && (session.metadata<session.assets.size() || session.index<session.assets.size());++work){
        if(session.metadata<session.assets.size()){
            auto& asset=session.assets[session.metadata];
            if(!call("SFLD_modelTransactionHostAsset",quoted_id()+","+std::to_string(session.metadata),text) || !descriptor(text,asset)){
                failed(PF_Err_BAD_CALLBACK_PARAM);return queued;}++session.metadata;
        }else {auto& asset=session.assets[session.index];
        if(asset.bytes.size()<asset.length){
            if(asset.bytes.empty())asset.bytes.reserve(asset.length);
            if(!call("SFLD_modelTransactionHostPage",quoted_id()+","+std::to_string(session.index)+","+
                std::to_string(asset.bytes.size()/page_bytes),text,page_bytes*2) || !append_hex(asset,text)){
                failed(PF_Err_BAD_CALLBACK_PARAM);return queued;}}
        if(asset.bytes.size()==asset.length)++session.index;
        }
        if(std::chrono::steady_clock::now()-started>std::chrono::milliseconds(25))break;
    }
    if(session.index==session.assets.size()){apply();return queued;}
    return true;
    } catch(...){try{failed(PF_Err_OUT_OF_MEMORY);}catch(...){session=Session{};}running=false;return queued;}
}
}
