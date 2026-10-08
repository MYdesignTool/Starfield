#include "TransformNullUI.hpp"
#include "NodeEffects.hpp"
#include "NodeRecord.hpp"
#include "SPBasic.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <string>

namespace {
int checks{},failures{},suites{},refs{},undo_start{},undo_end{},created{},deleted{},published{};
int fail{},layer_count=1;double guard{};AEGP_LayerIDVal selection=77;bool three_d{};
std::u16string name;constexpr int guard_index=starfield::adapter::native_nodes::sync_guard_index(starfield::adapter::native_nodes::Kind::transform);
AEGP_PFInterfaceSuite1 pf{};AEGP_LayerSuite9 layers{};AEGP_CompSuite11 comps{};
AEGP_StreamSuite6 streams{};AEGP_EffectSuite4 effects{};AEGP_UtilitySuite6 utility{};
void check(bool ok,const char* why){++checks;if(!ok){++failures;std::printf("FAILED: %s\n",why);}}
AEGP_LayerH layer(int id){return reinterpret_cast<AEGP_LayerH>(static_cast<std::intptr_t>(id));}
A_Err acquire(const char* n,int32,const void** out){
    if(!std::strcmp(n,kAEGPPFInterfaceSuite))*out=&pf;
    else if(!std::strcmp(n,kAEGPLayerSuite))*out=&layers;
    else if(!std::strcmp(n,kAEGPCompSuite))*out=&comps;
    else if(!std::strcmp(n,kAEGPStreamSuite))*out=&streams;
    else if(!std::strcmp(n,kAEGPEffectSuite))*out=&effects;
    else if(!std::strcmp(n,kAEGPUtilitySuite))*out=&utility;
    else return 1;
    ++suites;return 0;
}
A_Err release(const char*,int32){--suites;return 0;}
void reset(){fail=0;layer_count=1;selection=77;guard=0;three_d=false;name.clear();created=deleted=published=undo_start=undo_end=0;}
}
AEGP_PluginID node_graph_sync_plugin_id() noexcept{return 701;}
PF_Err sync_node_graph_parameter(PF_InData*,PF_OutData* out,PF_ParamDef* params[],const PF_UserChangedParamExtra* changed,bool) noexcept {
    ++published;check(changed->param_index==1 && selection==101 && three_d && guard==0 && params[guard_index]->u.fs_d.value==0,
        "graph publication sees the new 3D resource after guard restoration");
    if(fail==8){std::snprintf(out->return_msg,sizeof(out->return_msg),"detailed native binding failure");return 516;}
    return 0;
}
int main(){
    SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
    PF_InData data{};data.pica_basicP=&basic;data.effect_ref=reinterpret_cast<PF_ProgPtr>(1);data.time_scale=24;
    PF_OutData out{};std::array<PF_ParamDef,91> params{};std::array<PF_ParamDef*,91> pointers{};
    for(unsigned i=0;i<params.size();++i)pointers[i]=&params[i];
    params[1].param_type=PF_Param_LAYER;params[guard_index].param_type=PF_Param_FLOAT_SLIDER;
    pf.AEGP_GetEffectLayer=[](PF_ProgPtr,AEGP_LayerH* v)->A_Err{*v=layer(1);return 0;};
    pf.AEGP_GetNewEffectForEffect=[](AEGP_PluginID,PF_ProgPtr,AEGP_EffectRefH* v)->A_Err{++refs;*v=reinterpret_cast<AEGP_EffectRefH>(1);return 0;};
    effects.AEGP_DisposeEffect=[](AEGP_EffectRefH)->A_Err{--refs;return 0;};
    layers.AEGP_GetLayerParentComp=[](AEGP_LayerH owner,AEGP_CompH* v)->A_Err{
        check(owner==layer(1),"uses the effect owner composition");*v=reinterpret_cast<AEGP_CompH>(1);return 0;};
    layers.AEGP_GetCompNumLayers=[](AEGP_CompH,A_long* v)->A_Err{*v=layer_count;return 0;};
    layers.AEGP_SetLayerFlag=[](AEGP_LayerH v,AEGP_LayerFlags f,A_Boolean enabled)->A_Err{
        check(v==layer(2) && f==AEGP_LayerFlag_LAYER_IS_3D && enabled,"only the newly created Null gets its 3D switch");
        if(fail==3)return 516;three_d=true;return 0;};
    layers.AEGP_GetLayerID=[](AEGP_LayerH,AEGP_LayerIDVal* v)->A_Err{if(fail==4)return 516;*v=101;return 0;};
    layers.AEGP_SetLayerName=[](AEGP_LayerH,const A_UTF16Char* text)->A_Err{
        if(fail==5)return 516;name=reinterpret_cast<const char16_t*>(text);return 0;};
    layers.AEGP_DeleteLayer=[](AEGP_LayerH v)->A_Err{check(v==layer(2),"rollback deletes only its own new layer");++deleted;return 0;};
    comps.AEGP_CreateNullInComp=[](const A_UTF16Char*,AEGP_CompH c,const A_Time* duration,AEGP_LayerH* v)->A_Err{
        check(c==reinterpret_cast<AEGP_CompH>(1) && !duration,"Null uses the owner composition and its default duration");
        if(fail==2)return 516;++created;*v=layer(2);return 0;};
    streams.AEGP_GetNewEffectStreamByIndex=[](AEGP_PluginID,AEGP_EffectRefH,A_long index,AEGP_StreamRefH* v)->A_Err{
        ++refs;*v=reinterpret_cast<AEGP_StreamRefH>(static_cast<std::intptr_t>(index));return 0;};
    streams.AEGP_DisposeStream=[](AEGP_StreamRefH)->A_Err{--refs;return 0;};
    streams.AEGP_GetStreamType=[](AEGP_StreamRefH v,AEGP_StreamType* t)->A_Err{
        *t=v==reinterpret_cast<AEGP_StreamRefH>(1)?AEGP_StreamType_LAYER_ID:AEGP_StreamType_OneD;return 0;};
    streams.AEGP_GetNewStreamValue=[](AEGP_PluginID,AEGP_StreamRefH v,AEGP_LTimeMode,const A_Time*,A_Boolean pre,AEGP_StreamValue2* out)->A_Err{
        check(pre,"captures the selector and guard without expression evaluation");if(fail==1)return 516;
        out->streamH=v;if(v==reinterpret_cast<AEGP_StreamRefH>(1))out->val.layer_id=selection;else out->val.one_d=guard;return 0;};
    streams.AEGP_DisposeStreamValue=[](AEGP_StreamValue2*)->A_Err{return 0;};
    streams.AEGP_SetStreamValue=[](AEGP_PluginID,AEGP_StreamRefH v,AEGP_StreamValue2* value)->A_Err{
        if(v==reinterpret_cast<AEGP_StreamRefH>(1)){
            check(guard==1,"selector writes are guarded against recursive supervised commits");
            if(fail==7 && value->val.layer_id==101)return 516;selection=value->val.layer_id;
        }else {
            if((fail==6 && value->val.one_d==1) || (fail==9 && value->val.one_d==0)){fail=0;return 516;}
            guard=value->val.one_d;
        }
        return 0;};
    utility.AEGP_StartUndoGroup=[](const A_char*)->A_Err{++undo_start;return 0;};
    utility.AEGP_EndUndoGroup=[]()->A_Err{++undo_end;return 0;};
    reset();check(starfield::adapter::create_transform_null(&data,&out,pointers.data())==0,"one-click creation succeeds");
    check(created==1 && !deleted && published==1 && selection==101 && three_d && name==u"Starfield Transform Null 101",
        "creates one named 3D Null, referenced by stable ID, and publishes once");
    check(undo_start==1 && undo_end==1 && guard==0 && !refs && !suites,"one undo group and all host references balanced");
    for(int phase=1;phase<=9;++phase){
        reset();fail=phase;out={};
        check(starfield::adapter::create_transform_null(&data,&out,pointers.data())!=0,"injected failure rejects creation");
        check(selection==77 && guard==0 && created==deleted && !refs && !suites && undo_start==undo_end,
            "failure restores the old reference, deletes its new layer, and balances undo/suites");
        if(phase==8)check(std::strstr(out.return_msg,"detailed native binding failure")!=nullptr,"native binding diagnostics survive button rollback");
    }
    reset();layer_count=4096;out={};
    check(starfield::adapter::create_transform_null(&data,&out,pointers.data())!=0 && !created && !undo_start,
        "resource bound rejects before composition mutation");
    check(!suites && !refs,"preflight rejection releases references");
    reset();PF_EventExtra event{};event.contextH=reinterpret_cast<PF_ContextH>(1);event.effect_win.index=1;
    event.effect_win.area=PF_EA_CONTROL;
    event.effect_win.current_frame.left=0;event.effect_win.current_frame.top=0;
    event.effect_win.current_frame.right=160;event.effect_win.current_frame.bottom=28;
    event.e_type=PF_Event_DO_CLICK;
    event.u.do_click.screen_point.h=159;event.u.do_click.screen_point.v=26;
    check(starfield::adapter::transform_null_event(&data,&out,pointers.data(),&event)==0 && !created && !event.evt_out_flags,
        "click outside the button cannot create a layer");
    event.u.do_click.screen_point={10,10};
    check(starfield::adapter::transform_null_event(&data,&out,pointers.data(),&event)==0 && created==1 &&
        (event.evt_out_flags&PF_EO_HANDLED_EVENT),"button click is handled and creates exactly one layer");
    check(!suites && !refs,"event path balances host references");
    std::printf("Transform Null UI: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
