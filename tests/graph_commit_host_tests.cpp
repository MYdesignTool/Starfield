// Actual resident command hook; fake public SDK services, no AE process.
#include "../ae_plugin/StarfieldHost.cpp"
#include <iostream>
#include <array>
namespace starfield::adapter {
void initialize_model_asset_host(SPBasicSuite*,AEGP_PluginID) noexcept {}
void queue_model_asset_export() noexcept {}
bool step_model_asset_host() noexcept {return false;}
void stop_model_asset_host() noexcept {}
void initialize_model_transaction_host(SPBasicSuite*,AEGP_PluginID) noexcept {}
void queue_model_graph_transaction() noexcept {}
bool step_model_transaction_host() noexcept {return false;}
void stop_model_transaction_host() noexcept {}
void initialize_preset_file_host(SPBasicSuite*,AEGP_PluginID) noexcept {}
void queue_preset_file_dialog() noexcept {}
bool step_preset_file_host() noexcept {return false;}
void stop_preset_file_host() noexcept {}
}
namespace {
unsigned checks{},failures{},acquisitions{},releases{},effect_gets{},effect_disposes{},stream_gets{},stream_disposes{},generic_calls{},writes{};
bool reject{},miss{},wrong_type{},reenter{};
std::array<std::array<double,43>,2> values{};
AEGP_ProjSuite6 projects{};AEGP_ItemSuite9 item_suite{};AEGP_CompSuite11 comp_suite{};
AEGP_LayerSuite9 layer_suite{};AEGP_EffectSuite4 effect_suite{};AEGP_StreamSuite6 stream_suite{};
AEGP_UtilitySuite6 utility_suite{};
bool no_active_layer{},duplicate_renderer{};
unsigned repair_calls{};
void check(bool value,const char* message){++checks;if(!value){++failures;std::cerr<<message<<'\n';}}
SPErr acquire(const char* name,int32,const void** out) {
    ++acquisitions;*out=nullptr;
    if(!std::strcmp(name,kAEGPProjSuite))*out=&projects;
    if(!std::strcmp(name,kAEGPItemSuite))*out=&item_suite;
    if(!std::strcmp(name,kAEGPCompSuite))*out=&comp_suite;
    if(!std::strcmp(name,kAEGPLayerSuite))*out=&layer_suite;
    if(!std::strcmp(name,kAEGPEffectSuite))*out=&effect_suite;
    if(!std::strcmp(name,kAEGPStreamSuite))*out=&stream_suite;
    if(!std::strcmp(name,kAEGPUtilitySuite))*out=&utility_suite;
    return *out?0:kSPBadParameterError;
}
SPErr release(const char*,int32){++releases;return 0;}
std::size_t fx(AEGP_EffectRefH ref){return reinterpret_cast<std::size_t>(ref)-1;}
struct Ref{std::size_t effect;A_long index;};
}
int main() {
    using namespace starfield::adapter;
    SPBasicSuite sdk{};sdk.AcquireSuite=acquire;sdk.ReleaseSuite=release;
    basic=&sdk;plugin_id=701;main_thread=std::this_thread::get_id();stopped=false;graph_commit_command=91;
    initialize_ui_exclusion();
    graph_repair_command=92;
    layer_suite.AEGP_GetActiveLayer=[](AEGP_LayerH* out)->A_Err{*out=no_active_layer?nullptr:reinterpret_cast<AEGP_LayerH>(2);return 0;};
    utility_suite.AEGP_ReportInfo=[](AEGP_PluginID,const A_char*)->A_Err{return 0;};
    projects.AEGP_GetProjectByIndex=[](A_long,AEGP_ProjectH* out)->A_Err{*out=reinterpret_cast<AEGP_ProjectH>(1);return 0;};
    item_suite.AEGP_GetFirstProjItem=[](AEGP_ProjectH,AEGP_ItemH* out)->A_Err{*out=reinterpret_cast<AEGP_ItemH>(1);return 0;};
    item_suite.AEGP_GetNextProjItem=[](AEGP_ProjectH,AEGP_ItemH,AEGP_ItemH* out)->A_Err{*out=nullptr;return 0;};
    item_suite.AEGP_GetItemType=[](AEGP_ItemH,AEGP_ItemType* out)->A_Err{*out=AEGP_ItemType_COMP;return 0;};
    comp_suite.AEGP_GetCompFromItem=[](AEGP_ItemH,AEGP_CompH* out)->A_Err{*out=reinterpret_cast<AEGP_CompH>(1);return 0;};
    layer_suite.AEGP_GetCompNumLayers=[](AEGP_CompH,A_long* out)->A_Err{*out=2;return 0;};
    layer_suite.AEGP_GetCompLayerByIndex=[](AEGP_CompH,A_long i,AEGP_LayerH* out)->A_Err{*out=reinterpret_cast<AEGP_LayerH>(std::size_t(i+1));return 0;};
    layer_suite.AEGP_GetLayerCurrentTime=[](AEGP_LayerH,AEGP_LTimeMode,A_Time* out)->A_Err{*out={7,24};return 0;};
    effect_suite.AEGP_GetLayerNumEffects=[](AEGP_LayerH,A_long* out)->A_Err{*out=duplicate_renderer?2:1;return 0;};
    effect_suite.AEGP_GetLayerEffectByIndex=[](AEGP_PluginID,AEGP_LayerH layer,A_long,AEGP_EffectRefH* out)->A_Err{
        ++effect_gets;*out=reinterpret_cast<AEGP_EffectRefH>(layer);return 0;};
    effect_suite.AEGP_GetInstalledKeyFromLayerEffect=[](AEGP_EffectRefH,AEGP_InstalledEffectKey* out)->A_Err{*out=1;return 0;};
    effect_suite.AEGP_GetEffectMatchName=[](AEGP_InstalledEffectKey,A_char* out)->A_Err{std::strcpy(out,"org.starfieldfx.particle");return 0;};
    effect_suite.AEGP_DisposeEffect=[](AEGP_EffectRefH)->A_Err{++effect_disposes;return 0;};
    stream_suite.AEGP_GetNewEffectStreamByIndex=[](AEGP_PluginID,AEGP_EffectRefH effect,A_long index,AEGP_StreamRefH* out)->A_Err{
        ++stream_gets;*out=reinterpret_cast<AEGP_StreamRefH>(new Ref{fx(effect),index});return 0;};
    stream_suite.AEGP_GetStreamType=[](AEGP_StreamRefH,AEGP_StreamType* out)->A_Err{*out=wrong_type?AEGP_StreamType_LAYER_ID:AEGP_StreamType_OneD;return 0;};
    stream_suite.AEGP_GetNewStreamValue=[](AEGP_PluginID,AEGP_StreamRefH ref,AEGP_LTimeMode,const A_Time*,A_Boolean,AEGP_StreamValue2* out)->A_Err{
        const auto& r=*reinterpret_cast<Ref*>(ref);out->val.one_d=values[r.effect][r.index];return 0;};
    stream_suite.AEGP_DisposeStreamValue=[](AEGP_StreamValue2*)->A_Err{return 0;};
    stream_suite.AEGP_DisposeStream=[](AEGP_StreamRefH ref)->A_Err{++stream_disposes;delete reinterpret_cast<Ref*>(ref);return 0;};
    stream_suite.AEGP_SetStreamValue=[](AEGP_PluginID,AEGP_StreamRefH ref,AEGP_StreamValue2* value)->A_Err{
        ++writes;const auto& r=*reinterpret_cast<Ref*>(ref);values[r.effect][r.index]=value->val.one_d;return 0;};
    effect_suite.AEGP_EffectCallGeneric=[](AEGP_PluginID,AEGP_EffectRefH effect,const A_Time* time,PF_Cmd cmd,void* extra)->A_Err{
        if(*static_cast<std::uint32_t*>(extra)==0x53465232){
            ++repair_calls;auto& repair=*static_cast<GraphRepairRequest*>(extra);
            check(cmd==PF_Cmd_COMPLETELY_GENERAL && repair.bytes==sizeof(repair) && repair.version==1 &&
                repair.layer==reinterpret_cast<AEGP_LayerH>(2) && repair.plugin==701 && time->value==0 && SFLD_UiExclusionActiveV1(),
                "repair targets selected layer with versioned request and resident exclusion");
            A_Boolean handled{};const auto before=acquisitions;
            check(repair_node_graph(nullptr,nullptr,92,AEGP_HP_BeforeAE,FALSE,&handled)!=0 && acquisitions==before,
                "repair rejects nested menu before SDK access");
            if(!miss){repair.acknowledged=1;repair.status=reject?PF_Err_BAD_CALLBACK_PARAM:0;}return 0;
        }
        ++generic_calls;auto& request=*static_cast<GraphCommitRequest*>(extra);
        check(cmd==PF_Cmd_COMPLETELY_GENERAL && time->value==7 && time->scale==24 && request.magic==0x53464731 &&
            request.bytes==sizeof(request) && request.renderer==effect && request.layer==reinterpret_cast<AEGP_LayerH>(effect),
            "explicit message has exact borrowed owner and versioned contract");
        if(reenter){A_Boolean handled{};const auto before=acquisitions;
            check(commit_node_graph(nullptr,nullptr,91,AEGP_HP_BeforeAE,FALSE,&handled)!=0 && acquisitions==before,
                "same-thread nested command is rejected before suite access");}
        if(miss)return 0;
        request.acknowledged=1;request.status=reject?PF_Err_BAD_CALLBACK_PARAM:0;
        if(!reject)values[fx(effect)][kGraphEditReceiptId]=request.nonce;
        return request.status;
    };
    const auto run=[](){A_Boolean handled{};const auto error=commit_node_graph(nullptr,nullptr,91,AEGP_HP_BeforeAE,FALSE,&handled);
        check(handled,"graph command hook acknowledges command handling");return error;};
    values[0][kGraphSyncGuardId]=1;values[0][kGraphEditCommitId]=123;
    values[1][kGraphSyncGuardId]=2;values[1][kGraphEditCommitId]=456;
    reenter=true;check(run()==0 && generic_calls==1 && values[0][kGraphEditReceiptId]==123 && values[1][kGraphEditReceiptId]==0,
        "unique pending renderer is found without current selection; backup ignored");
    reenter=false;check(run()==0 && generic_calls==1,"duplicate command does not compile again");
    values[0][kGraphEditCommitId]=124;values[1][kGraphSyncGuardId]=1;
    check(run()!=0 && generic_calls==1 && writes==0,"two pending renderers fail before graph or receipt writes");
    values[1][kGraphSyncGuardId]=0;reject=true;
    check(run()!=0 && generic_calls==2 && values[0][kGraphEditReceiptId]==-124,"native failure receives a negative terminal receipt");
    const auto rejected_calls=generic_calls,rejected_writes=writes;
    check(run()==0 && generic_calls==rejected_calls && writes==rejected_writes,
        "repeated rejected command is terminal and never republishes");
    reject=false;
    values[0][kGraphEditCommitId]=790333;values[0][kGraphEditReceiptId]=-790333;
    values[1][kGraphSyncGuardId]=1;values[1][kGraphEditCommitId]=158257;
    const auto rejected_renderer=values[0];
    check(run()==0 && generic_calls==rejected_calls+1 && values[1][kGraphEditReceiptId]==158257 &&
        values[0]==rejected_renderer && writes==rejected_writes,
        "owner trace: failed guarded renderer does not block fresh bootstrap or get unlocked");
    // Repeat with the terminal renderer encountered after the pending renderer.
    std::swap(values[0],values[1]);values[0][kGraphEditCommitId]=158258;
    check(run()==0 && values[0][kGraphEditReceiptId]==158258 && values[1]==rejected_renderer,
        "terminal rejection is skipped regardless of project traversal order");
    values[0][kGraphEditCommitId]=158259;values[1][kGraphEditReceiptId]=-790332;
    const auto mismatched_calls=generic_calls,mismatched_writes=writes;
    check(run()!=0 && generic_calls==mismatched_calls && writes==mismatched_writes,
        "negative receipt for another nonce remains pending and ambiguous");
    values[1][kGraphEditCommitId]=1000001;values[1][kGraphEditReceiptId]=-1000001;
    check(run()!=0 && generic_calls==mismatched_calls && writes==mismatched_writes,
        "out of range nonce cannot masquerade as a terminal rejection");
    values[1][kGraphSyncGuardId]=0;
    reject=false;miss=true;values[0][kGraphEditCommitId]=125;
    check(run()!=0 && values[0][kGraphEditReceiptId]==-125,"unrecognized generic message cannot report success");
    miss=false;values[0][kGraphEditCommitId]=126;wrong_type=true;
    const auto before_calls=generic_calls;check(run()!=0 && generic_calls==before_calls,"invalid stream type fails before callback");wrong_type=false;
    const auto before_worker=acquisitions;std::thread worker([&]{check(run()!=0,"worker cannot submit UI command");});worker.join();
    check(acquisitions==before_worker,"worker makes no SDK calls");
    const auto token=SFLD_BeginUiExclusionV1();check(token!=0 && run()==0,"Model executor's existing exclusion permits synchronous publication");SFLD_EndUiExclusionV1(token);
    const auto repair=[](){A_Boolean handled{};const auto e=repair_node_graph(nullptr,nullptr,92,AEGP_HP_BeforeAE,FALSE,&handled);check(handled,"repair menu handled");return e;};
    check(repair()==0 && repair_calls==1,"selected repair dispatch succeeds");
    reject=true;check(repair()!=0,"native repair rejection propagates");reject=false;
    miss=true;check(repair()!=0,"unacknowledged repair cannot succeed");miss=false;
    const auto repair_before=repair_calls;no_active_layer=true;check(repair()!=0 && repair_calls==repair_before,"no selected layer makes no repair call");no_active_layer=false;
    duplicate_renderer=true;check(repair()!=0 && repair_calls==repair_before,"ambiguous selected renderer makes no repair call");duplicate_renderer=false;
    const auto busy=SFLD_BeginUiExclusionV1();const auto reads_before=acquisitions;
    check(repair()!=0 && acquisitions==reads_before,"repair refuses an existing UI transaction");SFLD_EndUiExclusionV1(busy);
    std::thread repair_worker([&]{check(repair()!=0,"repair rejects worker thread");});repair_worker.join();
    check(acquisitions==reads_before,"worker repair makes no SDK calls");
    check(effect_gets==effect_disposes && stream_gets==stream_disposes && acquisitions==releases,"all callback-local references and suites are released");
    std::cout<<checks<<" explicit graph command checks, "<<failures<<" failures\n";return failures?1:0;
}
