#include "ModelAssetHost.hpp"
#include "ModelAssetMessage.hpp"
#include "NodeRecord.hpp"
#include "Parameters.hpp"
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

using namespace starfield::adapter;
namespace {
unsigned checks{},suites{},refs{},values{},handles{},locks{},script_calls{},generic_calls{},chunk_calls{};
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"check "<<checks<<" at "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
constexpr char transfer[]="0123456789abcdef0123456789abcdef",uuid[]="fedcba98765432100123456789abcdef";
std::string request_line,missing_suite,received,failed;
bool available=true,continue_ack=true,wrong_uuid{},duplicate{},multiple_renderers{},wrong_project{},wrong_comp{},wrong_layer{},
    generic_error{},unacknowledged{},bad_payload_size{},sink_overflow{},script_error{},unterminated{},big_result{},wrong_type{};
bool backup_renderer{},backup_model{};double renderer_guard{},model_guard{};
std::size_t payload_size=70001;
int reject_page=-1;
unsigned begins{},finishes{};
struct Memory {std::string text;bool locked{};};
struct Effect {A_long index;};
struct Stream {A_long index,effect;};
AEGP_UtilitySuite6 utility{};AEGP_MemorySuite1 memory{};AEGP_ProjSuite6 projects{};
AEGP_ItemSuite9 items{};AEGP_CompSuite11 comps{};AEGP_LayerSuite9 layers{};
AEGP_EffectSuite4 effects{};AEGP_StreamSuite6 streams{};SPBasicSuite basic{};
template<class T>T opaque(std::uintptr_t value){return reinterpret_cast<T>(value);}
AEGP_MemHandle mem(std::string text){++handles;return reinterpret_cast<AEGP_MemHandle>(new Memory{std::move(text)});}
std::string arguments(const std::string& body,const char* name){const std::string marker=std::string(name)+"(";
    const auto start=body.find(marker);CHECK(start!=std::string::npos);const auto end=body.rfind("):'0')");CHECK(end!=std::string::npos);
    return body.substr(start+marker.size(),end-start-marker.size());}
void no_owned(){CHECK(suites==0);CHECK(refs==0);CHECK(values==0);CHECK(handles==0);CHECK(locks==0);}
void reset(){stop_model_asset_host();no_owned();initialize_model_asset_host(&basic,7);
    request_line=std::string(transfer)+"|101|202|303|"+uuid+"|1|17";
    missing_suite.clear();received.clear();failed.clear();payload_size=70001;reject_page=-1;
    available=continue_ack=true;wrong_uuid=duplicate=multiple_renderers=wrong_project=wrong_comp=wrong_layer=false;
    generic_error=unacknowledged=bad_payload_size=sink_overflow=script_error=unterminated=big_result=wrong_type=false;
    backup_renderer=backup_model=false;renderer_guard=model_guard=0;
    script_calls=generic_calls=chunk_calls=begins=finishes=0;}
void drain(){queue_model_asset_export();unsigned passes{};while(step_model_asset_host()){CHECK(++passes<300);no_owned();}no_owned();}
void initialize(){
    utility.AEGP_IsScriptingAvailable=[](A_Boolean* out)->A_Err{*out=available;return 0;};
    utility.AEGP_ExecuteScript=[](AEGP_PluginID id,const A_char* body,A_Boolean platform,AEGP_MemHandle* out,AEGP_MemHandle* error)->A_Err{
        CHECK(id==7);CHECK(platform==FALSE);++script_calls;CHECK(refs==0);CHECK(values==0);CHECK(locks==0);
        if(script_error){*error=mem("error");*out=mem("discarded");return 512;}
        std::string text(body),reply="1";
        if(text.find("SFLD_modelAssetHostRequest")!=std::string::npos)reply=request_line;
        else if(text.find("SFLD_modelAssetHostContinue")!=std::string::npos)reply=continue_ack?"1":"0";
        else if(text.find("SFLD_modelAssetHostBegin")!=std::string::npos){++begins;
            CHECK(arguments(text,"SFLD_modelAssetHostBegin")==std::string("'")+transfer+"',"+std::to_string(payload_size)+",[-2,-3,-4,2,3,4]");}
        else if(text.find("SFLD_modelAssetHostChunk")!=std::string::npos){
            auto args=arguments(text,"SFLD_modelAssetHostChunk");const auto prefix=std::string("'")+transfer+"',"+std::to_string(chunk_calls)+",'";
            CHECK(args.starts_with(prefix));CHECK(args.ends_with("'"));const auto hex=args.substr(prefix.size(),args.size()-prefix.size()-1);
            CHECK(hex.size()>0 && hex.size()<=65536 && hex.size()%2==0);
            CHECK(hex.find_first_not_of("0123456789abcdef")==std::string::npos);
            received+=hex;reply=static_cast<int>(chunk_calls)==reject_page?"0":"1";++chunk_calls;
        } else if(text.find("SFLD_modelAssetHostFinish")!=std::string::npos)++finishes;
        else if(text.find("SFLD_modelAssetHostFail")!=std::string::npos)failed=arguments(text,"SFLD_modelAssetHostFail");
        else CHECK(false);
        if(big_result)reply=std::string(2048,'x');
        if(!unterminated)reply.push_back(0);
        *out=mem(reply);return 0;
    };
    memory.AEGP_FreeMemHandle=[](AEGP_MemHandle h)->A_Err{auto* m=reinterpret_cast<Memory*>(h);CHECK(!m->locked);delete m;CHECK(handles>0);--handles;return 0;};
    memory.AEGP_GetMemHandleSize=[](AEGP_MemHandle h,AEGP_MemSize* out)->A_Err{*out=static_cast<AEGP_MemSize>(reinterpret_cast<Memory*>(h)->text.size());return 0;};
    memory.AEGP_LockMemHandle=[](AEGP_MemHandle h,void** out)->A_Err{auto* m=reinterpret_cast<Memory*>(h);CHECK(!m->locked);m->locked=true;++locks;*out=m->text.data();return 0;};
    memory.AEGP_UnlockMemHandle=[](AEGP_MemHandle h)->A_Err{auto* m=reinterpret_cast<Memory*>(h);CHECK(m->locked);m->locked=false;--locks;return 0;};
    projects.AEGP_GetProjectByIndex=[](A_long index,AEGP_ProjectH* out)->A_Err{CHECK(index==0);*out=opaque<AEGP_ProjectH>(1);return 0;};
    projects.AEGP_GetProjectRootFolder=[](AEGP_ProjectH,AEGP_ItemH* out)->A_Err{*out=opaque<AEGP_ItemH>(1);return 0;};
    items.AEGP_GetFirstProjItem=[](AEGP_ProjectH,AEGP_ItemH* out)->A_Err{*out=opaque<AEGP_ItemH>(1);return 0;};
    items.AEGP_GetNextProjItem=[](AEGP_ProjectH,AEGP_ItemH item,AEGP_ItemH* out)->A_Err{*out=item==opaque<AEGP_ItemH>(1)?opaque<AEGP_ItemH>(2):nullptr;return 0;};
    items.AEGP_GetItemID=[](AEGP_ItemH item,A_long* out)->A_Err{*out=item==opaque<AEGP_ItemH>(1)?(wrong_project?102:101):202;return 0;};
    items.AEGP_GetItemType=[](AEGP_ItemH,AEGP_ItemType* out)->A_Err{*out=static_cast<AEGP_ItemType>(wrong_comp?AEGP_ItemType_FOLDER:AEGP_ItemType_COMP);return 0;};
    comps.AEGP_GetCompFromItem=[](AEGP_ItemH,AEGP_CompH* out)->A_Err{*out=opaque<AEGP_CompH>(2);return 0;};
    layers.AEGP_GetLayerFromLayerID=[](AEGP_CompH,AEGP_LayerIDVal id,AEGP_LayerH* out)->A_Err{CHECK(id==303);*out=wrong_layer?nullptr:opaque<AEGP_LayerH>(3);return 0;};
    layers.AEGP_GetLayerCurrentTime=[](AEGP_LayerH,AEGP_LTimeMode mode,A_Time* out)->A_Err{CHECK(mode==AEGP_LTimeMode_LayerTime);*out={12,24};return 0;};
    effects.AEGP_GetLayerNumEffects=[](AEGP_LayerH,A_long* out)->A_Err{*out=(duplicate || multiple_renderers || backup_renderer || backup_model)?3:2;return 0;};
    effects.AEGP_GetLayerEffectByIndex=[](AEGP_PluginID id,AEGP_LayerH,A_long index,AEGP_EffectRefH* out)->A_Err{CHECK(id==7);++refs;*out=reinterpret_cast<AEGP_EffectRefH>(new Effect{index});return 0;};
    effects.AEGP_DisposeEffect=[](AEGP_EffectRefH h)->A_Err{delete reinterpret_cast<Effect*>(h);--refs;return 0;};
    effects.AEGP_GetInstalledKeyFromLayerEffect=[](AEGP_EffectRefH h,AEGP_InstalledEffectKey* out)->A_Err{*out=reinterpret_cast<Effect*>(h)->index;return 0;};
    effects.AEGP_GetEffectMatchName=[](AEGP_InstalledEffectKey key,A_char* out)->A_Err{
        std::strcpy(out,key==0 || (key==2 && (multiple_renderers || backup_renderer))?"org.starfieldfx.particle":"org.starfieldfx.node.model");return 0;};
    streams.AEGP_GetNewEffectStreamByIndex=[](AEGP_PluginID,AEGP_EffectRefH effect,A_long index,AEGP_StreamRefH* out)->A_Err{
        ++refs;*out=reinterpret_cast<AEGP_StreamRefH>(new Stream{index,reinterpret_cast<Effect*>(effect)->index});return 0;};
    streams.AEGP_GetStreamType=[](AEGP_StreamRefH,AEGP_StreamType* out)->A_Err{*out=wrong_type?AEGP_StreamType_ARB:AEGP_StreamType_OneD;return 0;};
    streams.AEGP_GetNewStreamValue=[](AEGP_PluginID,AEGP_StreamRefH ref,AEGP_LTimeMode,const A_Time*,A_Boolean,AEGP_StreamValue2* out)->A_Err{
        const auto key=*reinterpret_cast<Stream*>(ref);out->streamH=ref;
        if(key.index==kGraphSyncGuardId)out->val.one_d=key.effect==2 && backup_renderer?2:renderer_guard;
        else if(key.index==native_nodes::sync_guard_index(native_nodes::Kind::model))out->val.one_d=key.effect==2 && backup_model?2:model_guard;
        else {const auto index=key.index-native_nodes::uuid_first_index(native_nodes::Kind::model);CHECK(index>=0 && index<8);
            out->val.one_d=std::stoul(std::string(uuid).substr(index*4,4),nullptr,16)+(wrong_uuid?1:0);}++values;return 0;};
    streams.AEGP_DisposeStreamValue=[](AEGP_StreamValue2*)->A_Err{CHECK(values>0);--values;return 0;};
    streams.AEGP_DisposeStream=[](AEGP_StreamRefH ref)->A_Err{delete reinterpret_cast<Stream*>(ref);--refs;return 0;};
    effects.AEGP_EffectCallGeneric=[](AEGP_PluginID id,AEGP_EffectRefH effect,const A_Time* time,PF_Cmd command,void* extra)->A_Err{
        CHECK(id==7);CHECK(reinterpret_cast<Effect*>(effect)->index==1);CHECK(time->value==12 && time->scale==24);CHECK(command==PF_Cmd_COMPLETELY_GENERAL);
        ++generic_calls;auto& request=*static_cast<ModelAssetExportRequest*>(extra);CHECK(request.magic==0x53464d58 && request.bytes==sizeof(request) && request.version==1);
        CHECK(request.expected_source==1 && request.expected_revision==17);
        if(generic_error)return 516;if(unacknowledged)return 0;
        request.acknowledged=1;request.error=ModelAssetError::none;
        std::vector<std::uint8_t> bytes(payload_size);for(std::size_t i=0;i<bytes.size();++i)bytes[i]=static_cast<std::uint8_t>(i);
        const auto result=request.write_bytes(request.sink_context,bytes.data(),static_cast<std::uint32_t>(sink_overflow?8*1024*1024+1:bytes.size()));
        if(result){request.error=ModelAssetError::sink_rejected;return 0;}
        request.payload_bytes=static_cast<std::uint32_t>(bytes.size()+(bad_payload_size?1:0));
        const double bounds[]{-2,-3,-4,2,3,4};std::copy(std::begin(bounds),std::end(bounds),std::begin(request.bounds));return 0;
    };
    basic.AcquireSuite=[](const char* name,int32,const void** out)->A_Err{
        if(missing_suite==name){*out=nullptr;return 1;}
        const std::map<std::string,const void*> table{{kAEGPUtilitySuite,&utility},{kAEGPMemorySuite,&memory},{kAEGPProjSuite,&projects},
            {kAEGPItemSuite,&items},{kAEGPCompSuite,&comps},{kAEGPLayerSuite,&layers},{kAEGPEffectSuite,&effects},{kAEGPStreamSuite,&streams}};
        const auto found=table.find(name);CHECK(found!=table.end());*out=found->second;++suites;return 0;};
    basic.ReleaseSuite=[](const char*,int32)->A_Err{CHECK(suites>0);--suites;return 0;};
}
}
int main(){initialize();reset();CHECK(!step_model_asset_host());CHECK(script_calls==0);
    std::thread worker([]{queue_model_asset_export();CHECK(!step_model_asset_host());});worker.join();CHECK(script_calls==0);
    drain();CHECK(generic_calls==1 && begins==1 && finishes==1 && chunk_calls==3);CHECK(received.size()==payload_size*2);
    constexpr char hex[]="0123456789abcdef";for(std::size_t i=0;i<payload_size;++i){CHECK(received[i*2]==hex[(i&255)>>4]);CHECK(received[i*2+1]==hex[i&15]);}
    CHECK(!step_model_asset_host());CHECK(generic_calls==1);
    for(unsigned which=0;which<2;++which){reset();if(which==0)backup_renderer=true;else backup_model=true;drain();CHECK(generic_calls==1 && finishes==1);}
    for(unsigned which=0;which<2;++which){reset();if(which==0)renderer_guard=1;else model_guard=1;drain();CHECK(generic_calls==0 && finishes==0);}
    for(const char* suite:{kAEGPUtilitySuite,kAEGPMemorySuite,kAEGPProjSuite,kAEGPItemSuite,kAEGPCompSuite,kAEGPLayerSuite,kAEGPEffectSuite,kAEGPStreamSuite}){
        reset();missing_suite=suite;drain();CHECK(generic_calls==0 && finishes==0);}
    for(unsigned which=0;which<7;++which){reset();switch(which){case 0:wrong_uuid=true;break;case 1:duplicate=true;break;case 2:multiple_renderers=true;break;
        case 3:wrong_project=true;break;case 4:wrong_comp=true;break;case 5:wrong_layer=true;break;case 6:wrong_type=true;break;}drain();CHECK(generic_calls==0 && finishes==0);}
    for(const std::string& line:std::vector<std::string>{"0",std::string(transfer)+"|101|202|303|"+uuid+"|0|17",std::string(transfer)+"|101|202|303|"+uuid+"|1|2147483648",
        std::string(transfer)+"|101|202|303|"+uuid+"|1|17|extra",std::string(transfer)+"|101|202|303|"+uuid+"|1|17');alert(1)",
        std::string(transfer)+"|101|202|303|00000000000000000000000000000000|1|17",std::string(transfer)+"|101|202|-1|"+uuid+"|1|17"}){
        reset();request_line=line;drain();CHECK(generic_calls==0 && chunk_calls==0);}
    for(unsigned which=0;which<4;++which){reset();switch(which){case 0:generic_error=true;break;case 1:unacknowledged=true;break;case 2:bad_payload_size=true;break;case 3:sink_overflow=true;break;}
        drain();CHECK(generic_calls==1 && begins==0 && finishes==0 && !failed.empty());}
    reset();continue_ack=false;drain();CHECK(begins==1 && chunk_calls==0 && finishes==0);
    reset();reject_page=1;drain();CHECK(chunk_calls==2 && finishes==0);
    for(unsigned which=0;which<4;++which){reset();switch(which){case 0:script_error=true;break;case 1:unterminated=true;break;case 2:big_result=true;break;case 3:available=false;break;}
        drain();CHECK(generic_calls==0);}
    reset();payload_size=8*1024*1024;drain();CHECK(chunk_calls==256 && finishes==1 && received.size()==payload_size*2);
    reset();queue_model_asset_export();stop_model_asset_host();CHECK(!step_model_asset_host());CHECK(script_calls==0);no_owned();
    std::cout<<"model_asset_host_tests: "<<checks<<" checks passed; UI-idle fake host, no AE qualification\n";
}
