#include "ModelTransactionHost.hpp"
#include "ModelGraphTransaction.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace starfield::adapter;
namespace {
unsigned checks{},suites{},handles{},locks{},script_calls{},executor_calls{},prepares{},commits{},results{},pages{};
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"check "<<checks<<" at "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
constexpr char transfer[]="0123456789abcdef0123456789abcdef",node[]="fedcba98765432100123456789abcdef";
std::string request_line,descriptor_line,result_line,missing_suite;
std::vector<std::size_t> lengths;
bool available{},continue_ok{},prepare_ok{},commit_ok{},script_error{},unterminated{},oversize{},reenter{},
    bad_project{},bad_comp{},bad_layer{},change_after_prepare{},bad_hex{},short_page{},stop_in_prepare{},committed_warning{},unique_assets{};
unsigned script_depth{};
struct Memory {std::string text;bool locked{};};
AEGP_UtilitySuite6 utility{};AEGP_MemorySuite1 memory{};AEGP_ProjSuite6 projects{};
AEGP_ItemSuite9 items{};AEGP_CompSuite11 comps{};AEGP_LayerSuite9 layers{};SPBasicSuite basic{};
template<class T>T opaque(std::uintptr_t value){return reinterpret_cast<T>(value);}
AEGP_MemHandle mem(std::string text){++handles;return reinterpret_cast<AEGP_MemHandle>(new Memory{std::move(text)});}
void clean(){CHECK(suites==0);CHECK(handles==0);CHECK(locks==0);CHECK(script_depth==0);}
std::string arguments(const std::string& body,const char* name){const std::string marker=std::string(name)+"(";
    const auto start=body.find(marker),end=body.rfind("):'0')");CHECK(start!=std::string::npos && end!=std::string::npos);
    return body.substr(start+marker.size(),end-start-marker.size());}
void reset(){stop_model_transaction_host();clean();initialize_model_transaction_host(&basic,7);
    request_line=std::string(transfer)+"|101|202|303|1|"+node+",000000000000000000000000000000ff";
    lengths={70001};descriptor_line.clear();result_line.clear();missing_suite.clear();
    available=continue_ok=prepare_ok=commit_ok=true;
    script_error=unterminated=oversize=reenter=bad_project=bad_comp=bad_layer=change_after_prepare=bad_hex=short_page=stop_in_prepare=committed_warning=unique_assets=false;
    script_calls=executor_calls=prepares=commits=results=pages=0;}
void drain(){queue_model_graph_transaction();unsigned passes{};while(step_model_transaction_host()){CHECK(++passes<300);clean();}clean();}
void initialize(){
    utility.AEGP_IsScriptingAvailable=[](A_Boolean* out)->A_Err{*out=available;return 0;};
    utility.AEGP_ExecuteScript=[](AEGP_PluginID plugin,const char* body,A_Boolean platform,AEGP_MemHandle* out,AEGP_MemHandle* error)->A_Err{
        CHECK(plugin==7 && platform==FALSE);CHECK(handles==0 && locks==0);CHECK(script_depth++==0);++script_calls;
        if(reenter)CHECK(!step_model_transaction_host());
        std::string text(body),reply="1";
        if(text.find("SFLD_modelTransactionHostRequest")!=std::string::npos)reply=request_line;
        else if(text.find("SFLD_modelTransactionHostContinue")!=std::string::npos)reply=continue_ok?"1":"0";
        else if(text.find("SFLD_modelTransactionHostAsset")!=std::string::npos){
            const auto args=arguments(text,"SFLD_modelTransactionHostAsset");
            const auto index=std::stoul(args.substr(args.find(',')+1));CHECK(index<lengths.size());
            std::string asset_id=node;if(unique_assets)asset_id.back()="0123456789abcdef"[index];
            reply=descriptor_line.empty()?asset_id+"|2|17|"+std::to_string(lengths[index])+"|-2|-3|-4|2|3|4":descriptor_line;
        } else if(text.find("SFLD_modelTransactionHostPage")!=std::string::npos){
            const auto args=arguments(text,"SFLD_modelTransactionHostPage");const auto first=args.find(','),last=args.rfind(',');
            const auto index=std::stoul(args.substr(first+1,last-first-1)),page=std::stoul(args.substr(last+1));
            CHECK(index<lengths.size() && page*32768<lengths[index]);++pages;
            const auto bytes=std::min(std::size_t{32768},lengths[index]-page*32768);
            reply=std::string(bytes*2,'a');if(bad_hex)reply[0]='Z';if(short_page)reply.pop_back();
        } else if(text.find("SFLD_modelTransactionHostPrepare")!=std::string::npos){++prepares;reply=prepare_ok?"1":"0";
            if(change_after_prepare)bad_project=true;if(stop_in_prepare)stop_model_transaction_host();
        } else if(text.find("SFLD_modelTransactionHostCommit")!=std::string::npos){++commits;reply=commit_ok?"1":"0";
        } else if(text.find("SFLD_modelTransactionHostResult")!=std::string::npos){++results;result_line=arguments(text,"SFLD_modelTransactionHostResult");}
        else CHECK(false);
        if(oversize)reply=std::string(65537,'x');
        if(!unterminated)reply.push_back(0);
        *out=mem(reply);if(script_error)*error=mem("error");CHECK(--script_depth==0);return script_error?512:0;
    };
    memory.AEGP_FreeMemHandle=[](AEGP_MemHandle value)->A_Err{auto* m=reinterpret_cast<Memory*>(value);CHECK(!m->locked);delete m;CHECK(handles-- >0);return 0;};
    memory.AEGP_GetMemHandleSize=[](AEGP_MemHandle value,AEGP_MemSize* out)->A_Err{*out=static_cast<AEGP_MemSize>(reinterpret_cast<Memory*>(value)->text.size());return 0;};
    memory.AEGP_LockMemHandle=[](AEGP_MemHandle value,void** out)->A_Err{auto* m=reinterpret_cast<Memory*>(value);CHECK(!m->locked);m->locked=true;++locks;*out=m->text.data();return 0;};
    memory.AEGP_UnlockMemHandle=[](AEGP_MemHandle value)->A_Err{auto* m=reinterpret_cast<Memory*>(value);CHECK(m->locked);m->locked=false;CHECK(locks-- >0);return 0;};
    projects.AEGP_GetProjectByIndex=[](A_long index,AEGP_ProjectH* out)->A_Err{CHECK(index==0);*out=opaque<AEGP_ProjectH>(1);return 0;};
    projects.AEGP_GetProjectRootFolder=[](AEGP_ProjectH,AEGP_ItemH* out)->A_Err{*out=opaque<AEGP_ItemH>(1);return 0;};
    items.AEGP_GetItemID=[](AEGP_ItemH item,A_long* out)->A_Err{*out=item==opaque<AEGP_ItemH>(1)?(bad_project?102:101):(bad_comp?203:202);return 0;};
    items.AEGP_GetFirstProjItem=[](AEGP_ProjectH,AEGP_ItemH* out)->A_Err{*out=opaque<AEGP_ItemH>(2);return 0;};
    items.AEGP_GetNextProjItem=[](AEGP_ProjectH,AEGP_ItemH,AEGP_ItemH* out)->A_Err{*out=nullptr;return 0;};
    items.AEGP_GetItemType=[](AEGP_ItemH,AEGP_ItemType* out)->A_Err{*out=AEGP_ItemType_COMP;return 0;};
    comps.AEGP_GetCompFromItem=[](AEGP_ItemH,AEGP_CompH* out)->A_Err{*out=opaque<AEGP_CompH>(2);return 0;};
    layers.AEGP_GetLayerFromLayerID=[](AEGP_CompH,A_long id,AEGP_LayerH* out)->A_Err{CHECK(id==303);*out=bad_layer?nullptr:opaque<AEGP_LayerH>(3);return 0;};
    layers.AEGP_GetLayerCurrentTime=[](AEGP_LayerH,AEGP_LTimeMode mode,A_Time* out)->A_Err{CHECK(mode==AEGP_LTimeMode_LayerTime);*out={12,24};return 0;};
    basic.AcquireSuite=[](const char* name,int version,const void** out)->SPErr{
        CHECK(version>0);if(missing_suite==name)return 1;
        if(!std::strcmp(name,kAEGPUtilitySuite))*out=&utility;else if(!std::strcmp(name,kAEGPMemorySuite))*out=&memory;
        else if(!std::strcmp(name,kAEGPProjSuite))*out=&projects;else if(!std::strcmp(name,kAEGPItemSuite))*out=&items;
        else if(!std::strcmp(name,kAEGPCompSuite))*out=&comps;else if(!std::strcmp(name,kAEGPLayerSuite))*out=&layers;
        else return 1;++suites;return 0;};
    basic.ReleaseSuite=[](const char*,int)->SPErr{CHECK(suites-- >0);return 0;};
}
}
namespace starfield::adapter {
// Transport seam only; the actual whole-graph executor has a separate suite.
ModelGraphTransactionResult apply_model_graph_transaction(const ModelGraphTransactionPlan& plan) noexcept {
    ++executor_calls;CHECK(plan.basic==&basic && plan.plugin==7 && plan.layer==opaque<AEGP_LayerH>(3));
    CHECK(plan.time.value==12 && plan.time.scale==24);CHECK(plan.desired_ids.size()==2);CHECK(plan.assets.size()==lengths.size());
    CHECK(suites==0 && handles==0 && locks==0);auto script_before=script_calls;CHECK(!plan.is_cancelled(plan.context));CHECK(script_calls==script_before);
    for(std::size_t i=0;i<plan.assets.size();++i){const auto& a=plan.assets[i];CHECK(a.mesh.size()==lengths[i]);
        CHECK(a.source==2 && a.revision==17);for(auto b:a.mesh)CHECK(b==0xaa);}
    ModelGraphTransactionResult result;result.stage=ModelTransactionStage::prepare;result.error=plan.prepare(plan.context);
    if(!result.error){result.stage=ModelTransactionStage::commit;result.error=plan.is_cancelled(plan.context)?PF_Interrupt_CANCEL:plan.commit(plan.context);}
    if(!result.error){result.committed=true;result.stage=ModelTransactionStage::complete;result.cleanup_error=committed_warning?516:0;}
    return result;
}
}
int main(){initialize();reset();CHECK(!step_model_transaction_host());CHECK(script_calls==0);
    drain();CHECK(executor_calls==1 && prepares==1 && commits==1 && results==1 && pages==3);CHECK(result_line.find(",[1,7,0,0,0,0,0,0,-1]")!=std::string::npos);
    reset();reenter=true;drain();CHECK(executor_calls==1 && commits==1);
    reset();committed_warning=true;drain();CHECK(result_line.find(",[1,7,0,0,0,516,0,0,-1]")!=std::string::npos);
    for(auto which:{0,1}){reset();if(which==0)prepare_ok=false;else commit_ok=false;drain();CHECK(executor_calls==1);CHECK(result_line.find(",[0,")!=std::string::npos);}
    reset();change_after_prepare=true;drain();CHECK(prepares==1 && commits==0 && results==1);
    reset();stop_in_prepare=true;drain();CHECK(prepares==1 && commits==0 && results==1);
    for(auto which:{0,1,2,3,4,5,6}){reset();if(which==0)bad_project=true;if(which==1)bad_comp=true;if(which==2)bad_layer=true;
        if(which==3)bad_hex=true;if(which==4)short_page=true;if(which==5)continue_ok=false;if(which==6)missing_suite=kAEGPLayerSuite;
        drain();CHECK(executor_calls==0 && prepares==0 && commits==0 && results==1);}
    for(const auto& line:{std::string("0"),std::string(transfer)+"|0|202|303|1|"+node,
        std::string(transfer)+"|101|202|303|64|"+node,std::string(transfer)+"|101|202|303|1|"+node+","+node,
        std::string(transfer)+"|101|202|303|1|"+node+","}){reset();request_line=line;drain();CHECK(executor_calls==0 && pages==0);}
    for(const auto& line:{std::string(node)+"|3|17|32|-2|-3|-4|2|3|4",std::string(node)+"|2|2147483648|32|-2|-3|-4|2|3|4",
        std::string(node)+"|2|17|8388609|-2|-3|-4|2|3|4",std::string(node)+"|2|17|32|nan|-3|-4|2|3|4",
        std::string(node)+"|2|17|32|-2|-3|-4|-3|3|4",std::string(node)+"|2|0|32|-2|-3|-4|2|3|4"}){
        reset();descriptor_line=line;drain();CHECK(executor_calls==0 && pages==0 && results==1);}
    reset();request_line=std::string(transfer)+"|101|202|303|2|"+node+",000000000000000000000000000000ff";lengths={32,32};drain();CHECK(executor_calls==0 && pages==0 && results==1);
    reset();unique_assets=true;lengths=std::vector<std::size_t>(9,8*1024*1024);
    request_line=std::string(transfer)+"|101|202|303|9|000000000000000000000000000000ff";
    for(unsigned i=0;i<9;++i){std::string asset_id=node;asset_id.back()="0123456789abcdef"[i];request_line+=","+asset_id;}
    drain();CHECK(executor_calls==0 && pages==0 && results==1);
    for(auto which:{0,1,2,3,4}){reset();if(which==0)available=false;if(which==1)script_error=true;if(which==2)unterminated=true;
        if(which==3)oversize=true;if(which==4)missing_suite=kAEGPMemorySuite;drain();CHECK(executor_calls==0 && prepares==0);}
    reset();std::thread worker([]{queue_model_graph_transaction();CHECK(!step_model_transaction_host());});worker.join();CHECK(script_calls==0);clean();
    reset();lengths={8*1024*1024};drain();CHECK(executor_calls==1 && pages==256 && results==1);
    reset();queue_model_graph_transaction();stop_model_transaction_host();CHECK(!step_model_transaction_host() && script_calls==0);clean();
    std::cout<<"Model graph Host transport: "<<checks<<" checks, 0 failures (fake executor/May2023 suites; no AE qualification)\n";
}
