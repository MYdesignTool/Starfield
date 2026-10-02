#include "NodeEffects.hpp"
#include "NodeRecord.hpp"
#include "NodeGraphSync.hpp"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include <array>
#include <cstdio>
#include <cstring>
int run_camera_capture_tests();
namespace {
int checks{},failures{},calls{},sets{},disposes{};
void check(bool v,const char* s) {++checks;if(!v){++failures;std::printf("FAILED: %s\n",s);}}
AEGP_PFInterfaceSuite1 pf{}; AEGP_EffectSuite4 effect{}; AEGP_StreamSuite6 stream{}; AEGP_UtilitySuite6 utility{};
A_Err acquire(const char* name,int32,const void** out) {
    if (!std::strcmp(name,kAEGPPFInterfaceSuite)) *out=&pf;
    else if (!std::strcmp(name,kAEGPEffectSuite)) *out=&effect;
    else if (!std::strcmp(name,kAEGPStreamSuite)) *out=&stream;
    else if (!std::strcmp(name,kAEGPUtilitySuite)) *out=&utility;
    else return 1;
    return 0;
}
A_Err release(const char*,int32) {return 0;}
}
int main() {
    SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
    utility.AEGP_RegisterWithAEGP=[](AEGP_GlobalRefcon,const A_char*,AEGP_PluginID* id)->A_Err{*id=1;return 0;};
    pf.AEGP_GetEffectLayer=[](PF_ProgPtr,AEGP_LayerH* layer)->A_Err{*layer=reinterpret_cast<AEGP_LayerH>(1);return 0;};
    effect.AEGP_GetLayerNumEffects=[](AEGP_LayerH,A_long* count)->A_Err{*count=1;return 0;};
    effect.AEGP_GetLayerEffectByIndex=[](AEGP_PluginID,AEGP_LayerH,A_long,AEGP_EffectRefH* value)->A_Err{*value=reinterpret_cast<AEGP_EffectRefH>(2);return 0;};
    effect.AEGP_GetInstalledKeyFromLayerEffect=[](AEGP_EffectRefH,AEGP_InstalledEffectKey* key)->A_Err{*key=1;return 0;};
    effect.AEGP_GetEffectMatchName=[](AEGP_InstalledEffectKey,A_char* name)->A_Err{std::strcpy(name,"org.starfieldfx.particle");return 0;};
    effect.AEGP_DisposeEffect=[](AEGP_EffectRefH)->A_Err{++disposes;return 0;};
    effect.AEGP_EffectCallGeneric=[](AEGP_PluginID,AEGP_EffectRefH,const A_Time*,PF_Cmd cmd,void* extra)->A_Err{
        ++calls;check(cmd==PF_Cmd_USER_CHANGED_PARAM,"native edit invokes renderer supervised callback");
        check(static_cast<PF_UserChangedParamExtra*>(extra)->param_index==41,"native compile index agrees with renderer");return 0;};
    stream.AEGP_GetNewEffectStreamByIndex=[](AEGP_PluginID,AEGP_EffectRefH,A_long index,AEGP_StreamRefH* ref)->A_Err{check(index==41,"commit stream resolved by runtime index");*ref=reinterpret_cast<AEGP_StreamRefH>(3);return 0;};
    stream.AEGP_GetNewStreamValue=[](AEGP_PluginID,AEGP_StreamRefH,AEGP_LTimeMode,const A_Time*,A_Boolean,AEGP_StreamValue2* value)->A_Err{value->val.one_d=0;return 0;};
    stream.AEGP_SetStreamValue=[](AEGP_PluginID,AEGP_StreamRefH,AEGP_StreamValue2* value)->A_Err{++sets;check(value->val.one_d==1,"commit nonce advances");return 0;};
    stream.AEGP_DisposeStreamValue=[](AEGP_StreamValue2*)->A_Err{return 0;};
    stream.AEGP_DisposeStream=[](AEGP_StreamRefH)->A_Err{return 0;};
    PF_InData data{};data.pica_basicP=&basic;data.effect_ref=reinterpret_cast<PF_ProgPtr>(4);data.time_scale=24;
    check(register_node_graph_sync(&data)==0,"native synchronization registers");
    using namespace starfield::adapter::native_nodes;
    constexpr auto kind=Kind::emitter;constexpr int count=parameter_count(kind);
    std::array<PF_ParamDef,count> values{};std::array<PF_ParamDef*,count> params{};
    for(int i=0;i<count;i++) {params[i]=&values[i];values[i].param_type=PF_Param_FLOAT_SLIDER;values[i].uu.change_flags=PF_ChangeFlag_CHANGED_VALUE;}
    values[uuid_first_index(kind)].u.fs_d.value=1;
    values[4].param_type=PF_Param_POINT;values[4].u.td.x_value=200<<16;values[4].u.td.y_value=300<<16;
    PF_UserChangedParamExtra changed{};changed.param_index=4;PF_OutData output{};
    check(sync_node_graph_parameter(&data,&output,params.data(),&changed)==0 && calls==1,"Origin XY with change flags triggers compile");
    changed.param_index=5;check(sync_node_graph_parameter(&data,&output,params.data(),&changed)==0 && calls==2,"Origin Z triggers compile");
    values[sync_guard_index(kind)].u.fs_d.value=1;
    check(sync_node_graph_parameter(&data,&output,params.data(),&changed)==0 && calls==2,"CEP batch guard prevents recursive compile");
    values[sync_guard_index(kind)].u.fs_d.value=0;changed.param_index=layout_x_index(kind);
    check(sync_node_graph_parameter(&data,&output,params.data(),&changed)==0 && calls==2,"record edits await explicit commit");
    check(sets==2 && disposes==2,"one balanced renderer handle per native edit");
    std::printf("Native sync: %d checks, %d failures\n",checks,failures);
    failures += run_camera_capture_tests();
    return failures?1:0;
}
