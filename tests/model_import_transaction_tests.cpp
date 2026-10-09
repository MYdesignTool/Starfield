#define main model_controls_fixture_main
#include "model_controls_tests.cpp"
#undef main
#include "ModelImportUI.hpp"
#include "NodeEffects.hpp"
#include "SPBasic.h"

namespace {
constexpr auto guard=adapter::native_nodes::sync_guard_index(adapter::native_nodes::Kind::model);
struct Ref{A_long index;};
std::array<AEGP_StreamVal2,101> host_values{};
std::array<PF_ParamDef,101> params{};std::array<PF_ParamDef*,101> pointers{};
AEGP_PFInterfaceSuite1 pf{};AEGP_StreamSuite6 streams{};AEGP_EffectSuite4 effects{};AEGP_UtilitySuite6 utility{};SPBasicSuite basic{};
PF_InData* context{};unsigned suites{},refs{},writes{},undo_groups{},undo_ends{},syncs{};
int fail_index=-1,silent_index=-1;bool fail_sync{},fail_rollback{};
SPAPI SPErr acquire(const char* name,int32,const void** out){
    if(!std::strcmp(name,kAEGPPFInterfaceSuite))*out=&pf;
    else if(!std::strcmp(name,kAEGPStreamSuite))*out=&streams;
    else if(!std::strcmp(name,kAEGPEffectSuite))*out=&effects;
    else if(!std::strcmp(name,kAEGPUtilitySuite))*out=&utility;
    else return 516;++suites;return 0;}
SPAPI SPErr release(const char*,int32){CHECK(suites);--suites;return 0;}
SPAPI A_Err effect(AEGP_PluginID,PF_ProgPtr,AEGP_EffectRefH* out){*out=reinterpret_cast<AEGP_EffectRefH>(1);return 0;}
SPAPI A_Err dispose_effect(AEGP_EffectRefH){return 0;}
SPAPI A_Err stream(AEGP_PluginID,AEGP_EffectRefH,A_long index,AEGP_StreamRefH* out){*out=reinterpret_cast<AEGP_StreamRefH>(new Ref{index});++refs;return 0;}
SPAPI A_Err dispose_stream(AEGP_StreamRefH ref){delete reinterpret_cast<Ref*>(ref);CHECK(refs);--refs;return 0;}
SPAPI A_Err type(AEGP_StreamRefH ref,AEGP_StreamType* out){*out=reinterpret_cast<Ref*>(ref)->index==layout::mesh?AEGP_StreamType_ARB:AEGP_StreamType_OneD;return 0;}
PF_ArbitraryH copy(PF_ArbitraryH source){PF_ArbitraryH target{};PF_ArbParamsExtra e{};e.id=1503;e.which_function=PF_Arbitrary_COPY_FUNC;
    e.u.copy_func_params.src_arbH=source;e.u.copy_func_params.dst_arbPH=&target;
    CHECK(adapter::model_geometry_arbitrary_callback(context,&e,1503)==0);return target;}
SPAPI A_Err get(AEGP_PluginID,AEGP_StreamRefH ref,AEGP_LTimeMode,const A_Time*,A_Boolean,AEGP_StreamValue2* out){
    const auto index=reinterpret_cast<Ref*>(ref)->index;out->streamH=ref;out->val=host_values[index];
    if(index==layout::mesh)out->val.arbH=reinterpret_cast<AEGP_ArbBlockVal>(copy(reinterpret_cast<PF_ArbitraryH>(out->val.arbH)));return 0;}
SPAPI A_Err dispose_value(AEGP_StreamValue2* value){if(reinterpret_cast<Ref*>(value->streamH)->index==layout::mesh)
    dispose(reinterpret_cast<PF_ArbitraryH>(value->val.arbH));return 0;}
SPAPI A_Err set(AEGP_PluginID,AEGP_StreamRefH ref,AEGP_StreamValue2* value){++writes;const auto index=reinterpret_cast<Ref*>(ref)->index;
    if(index==silent_index){silent_index=-1;return 0;}
    if(index==layout::mesh){auto cloned=copy(reinterpret_cast<PF_ArbitraryH>(value->val.arbH));dispose(reinterpret_cast<PF_ArbitraryH>(host_values[index].arbH));
        host_values[index].arbH=reinterpret_cast<AEGP_ArbBlockVal>(cloned);}else host_values[index]=value->val;
    if(index==fail_index){fail_index=-1;return 512;}return 0;}
SPAPI A_Err start(const A_char* name){CHECK(!std::strcmp(name,"Starfield: Import OBJ"));++undo_groups;return 0;}
SPAPI A_Err end(){++undo_ends;return 0;}
void setup(PF_InData& data){clear();context=&data;suites=refs=writes=undo_groups=undo_ends=syncs=0;fail_index=silent_index=-1;fail_sync=false;
    fail_rollback=false;host_values={};params={};CHECK(adapter::register_model_author_controls(&data)==0);
    for(unsigned i=1;i<=18;++i)params[i]=registered.at(i-1);
    for(unsigned i=0;i<params.size();++i)pointers[i]=&params[i];
    for(int axis=0;axis<6;++axis){params[layout::author_bounds_first+axis].param_type=PF_Param_FLOAT_SLIDER;
        params[layout::author_bounds_first+axis].u.fs_d.value=axis<3?-.5:.5;host_values[layout::author_bounds_first+axis].one_d=axis<3?-.5:.5;}
    params[layout::mesh].u.arb_d.value=params[layout::mesh].u.arb_d.dephault;
    params[guard].param_type=PF_Param_FLOAT_SLIDER;
    host_values[layout::mesh].arbH=reinterpret_cast<AEGP_ArbBlockVal>(copy(params[layout::mesh].u.arb_d.value));
    host_values[layout::source].one_d=1;data.pica_basicP=&basic;data.effect_ref=reinterpret_cast<PF_ProgPtr>(1);data.time_scale=24;data.current_time=12;
}
core::OpaqueBytes mesh_bytes(){return handles.at(reinterpret_cast<PF_ArbitraryH>(host_values[layout::mesh].arbH));}
constexpr std::string_view triangle="v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
void transactions(PF_InData& data){
    setup(data);PF_OutData out{};const auto original=mesh_bytes();
    CHECK(adapter::import_model_obj_text(&data,&out,pointers.data(),"bad",never)!=0&&writes==0&&mesh_bytes()==original);
    CHECK(adapter::import_model_obj_text(&data,&out,pointers.data(),triangle,cancelled)==PF_Interrupt_CANCEL&&writes==0);
    CHECK(adapter::import_model_obj_text(&data,&out,pointers.data(),triangle,never)==0&&host_values[layout::source].one_d==2&&
        host_values[layout::revision].one_d==1&&host_values[guard].one_d==0&&syncs==1&&undo_groups==1&&undo_ends==1);
    auto imported=take(adapter::read_model_geometry_parameter(&data,reinterpret_cast<PF_ArbitraryH>(host_values[layout::mesh].arbH),never));
    CHECK(imported.positions.size()==3&&imported.triangles.size()==1);
    const double expected[]{0,0,0,1,1,0};for(int axis=0;axis<6;++axis)CHECK(host_values[layout::author_bounds_first+axis].one_d==expected[axis]);
    CHECK(params[layout::source].u.pd.value==2&&params[layout::revision].u.sd.value==1&&
        (out.out_flags&PF_OutFlag_FORCE_RERENDER)&&(params[layout::source].uu.change_flags&PF_ChangeFlag_CHANGED_VALUE));
    CHECK(refs==0&&suites==0&&locks==0);
    out={};CHECK(adapter::import_model_obj_text(&data,&out,pointers.data(),triangle,never)==0&&host_values[layout::revision].one_d==2&&undo_groups==undo_ends);
    for(const auto failed:{guard,layout::mesh,layout::revision,layout::source,
        layout::author_bounds_first,layout::author_bounds_first+1,layout::author_bounds_first+2,
        layout::author_bounds_first+3,layout::author_bounds_first+4,layout::author_bounds_first+5}){
        setup(data);out={};const auto before=mesh_bytes();fail_index=failed;
        CHECK(adapter::import_model_obj_text(&data,&out,pointers.data(),triangle,never)==512&&mesh_bytes()==before&&
            host_values[layout::source].one_d==1&&host_values[layout::revision].one_d==0&&host_values[guard].one_d==0);
        CHECK(params[layout::source].u.pd.value==1&&params[layout::revision].u.sd.value==0&&params[guard].u.fs_d.value==0&&
            refs==0&&suites==0&&locks==0&&undo_groups==undo_ends&&handles.size()==2);
        for(int axis=0;axis<6;++axis)CHECK(host_values[layout::author_bounds_first+axis].one_d==(axis<3?-.5:.5));
    }
    setup(data);out={};const auto before=mesh_bytes();fail_sync=true;
    CHECK(adapter::import_model_obj_text(&data,&out,pointers.data(),triangle,never)==512&&syncs==1&&mesh_bytes()==before&&
        host_values[layout::revision].one_d==0&&host_values[layout::source].one_d==1&&host_values[guard].one_d==0&&refs==0&&suites==0&&handles.size()==2);
    setup(data);out={};const auto silent_before=mesh_bytes();silent_index=layout::mesh;
    CHECK(adapter::import_model_obj_text(&data,&out,pointers.data(),triangle,never)==PF_Err_INTERNAL_STRUCT_DAMAGED&&
        mesh_bytes()==silent_before&&syncs==0&&host_values[guard].one_d==0&&handles.size()==2&&refs==0&&suites==0);
    setup(data);host_values[layout::revision].one_d=2147483647;out={};
    CHECK(adapter::import_model_obj_text(&data,&out,pointers.data(),triangle,never)==PF_Err_BAD_CALLBACK_PARAM&&writes==0&&undo_groups==0&&handles.size()==2);
    setup(data);host_values[guard].one_d=1;out={};CHECK(adapter::import_model_obj_text(&data,&out,pointers.data(),triangle,never)==PF_Err_BAD_CALLBACK_PARAM&&writes==0);
    setup(data);out={};fail_sync=fail_rollback=true;
    CHECK(adapter::import_model_obj_text(&data,&out,pointers.data(),triangle,never)==PF_Err_INTERNAL_STRUCT_DAMAGED&&
        std::strstr(out.return_msg,"rollback 512")&&refs==0&&suites==0&&locks==0&&undo_groups==undo_ends);
    clear();CHECK(refs==0&&suites==0&&locks==0&&handles.empty());
}
}
AEGP_PluginID node_graph_sync_plugin_id() noexcept{return 1;}
PF_Err sync_node_graph_parameter(PF_InData*,PF_OutData*,PF_ParamDef* values[],const PF_UserChangedParamExtra* changed,bool) noexcept {
    ++syncs;CHECK(changed&&changed->param_index==layout::source&&values[layout::source]->u.pd.value==2&&values[guard]->u.fs_d.value==0);
    CHECK(host_values[layout::source].one_d==2&&host_values[layout::revision].one_d==values[layout::revision]->u.sd.value);
    if(fail_rollback)fail_index=layout::mesh;return fail_sync?512:0;
}
int main(){std::setvbuf(stdout,nullptr,_IONBF,0);if(model_controls_fixture_main())return 1;
    PF_UtilCallbacks utils{};utils.host_new_handle=allocate;utils.host_lock_handle=lock;utils.host_unlock_handle=unlock;
    utils.host_dispose_handle=dispose;utils.host_get_handle_size=size_of;PF_InData data{};data.utils=&utils;data.inter.add_param=add;
    basic.AcquireSuite=acquire;basic.ReleaseSuite=release;pf.AEGP_GetNewEffectForEffect=effect;effects.AEGP_DisposeEffect=dispose_effect;
    streams.AEGP_GetNewEffectStreamByIndex=stream;streams.AEGP_DisposeStream=dispose_stream;streams.AEGP_GetStreamType=type;
    streams.AEGP_GetNewStreamValue=get;streams.AEGP_DisposeStreamValue=dispose_value;streams.AEGP_SetStreamValue=set;
    utility.AEGP_StartUndoGroup=start;utility.AEGP_EndUndoGroup=end;
    try{transactions(data);std::printf("Model import transaction: %u checks, %u failures (May2023 SDK/fake host)\n",checks,failures);return failures?1:0;}
    catch(const std::exception& error){std::printf("FAILED: %s\n",error.what());clear();return 1;}}
