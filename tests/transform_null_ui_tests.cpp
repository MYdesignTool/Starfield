#include "TransformNullUI.hpp"
#include "NodeEffects.hpp"
#include "NodeRecord.hpp"
#include "TextureLayerInventory.hpp"
#include "SPBasic.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <fstream>
#if defined(STARFIELD_TEXTURE_DISPATCH_TEST)
#include "GpuRender.hpp"
#include "ParticleGradientUI.hpp"
PF_Err register_node_graph_sync(PF_InData*) noexcept{return PF_Err_NONE;}
namespace starfield::adapter {
PF_Err gpu_device_setup(PF_InData*,PF_OutData*,PF_GPUDeviceSetupExtra*) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err gpu_device_setdown(PF_InData*,PF_GPUDeviceSetdownExtra*) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
bool gpu_device_matches(const void*,PF_GPU_Framework,A_u_long) noexcept{return false;}
PF_Err copy_gpu_pixels(PF_InData*,const void*,PF_GPU_Framework,A_u_long,PF_EffectWorld*,PF_EffectWorld*) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err particle_gradient_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err particle_rotation_curve_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err particle_gradient_param_ui(PF_InData*,PF_ParamDef*[]) noexcept{return PF_Err_NONE;}
void clear_particle_gradient_ui() noexcept{}
}
#endif

namespace {
int checks{},failures{},suites{},refs{},undo_start{},undo_end{},created{},deleted{},published{};
int fail{},layer_count=2,names{},item_flag_reads{};double guard{};AEGP_LayerIDVal selection=77;bool three_d{},cancel_menu=true,texture_inventory{};
int script_calls{},script_handles{},script_locks{};std::string script_result,script_capture;
constexpr std::intptr_t result_handle=999,error_handle=998;
AEGP_LayerIDVal expected_selection=101,menu_selection=101;
std::u16string name;constexpr int guard_index=starfield::adapter::native_nodes::sync_guard_index(starfield::adapter::native_nodes::Kind::transform);
std::vector<PF_ParamDef> registered_native;
AEGP_PFInterfaceSuite1 pf{};AEGP_LayerSuite9 layers{};AEGP_CompSuite11 comps{};
AEGP_StreamSuite6 streams{};AEGP_EffectSuite4 effects{};AEGP_UtilitySuite6 utility{};
AEGP_MemorySuite1 memory{};AEGP_ItemSuite9 items{};
void check(bool ok,const char* why){++checks;if(!ok){++failures;std::printf("FAILED: %s\n",why);}}
AEGP_LayerH layer(int id){return reinterpret_cast<AEGP_LayerH>(static_cast<std::intptr_t>(id));}
A_Err acquire(const char* n,int32,const void** out){
    if(!std::strcmp(n,kAEGPPFInterfaceSuite))*out=&pf;
    else if(!std::strcmp(n,kAEGPLayerSuite))*out=&layers;
    else if(!std::strcmp(n,kAEGPCompSuite))*out=&comps;
    else if(!std::strcmp(n,kAEGPStreamSuite))*out=&streams;
    else if(!std::strcmp(n,kAEGPEffectSuite))*out=&effects;
    else if(!std::strcmp(n,kAEGPUtilitySuite))*out=&utility;
    else if(!std::strcmp(n,kAEGPMemorySuite))*out=&memory;
    else if(!std::strcmp(n,kAEGPItemSuite))*out=&items;
    else return 1;
    ++suites;return 0;
}
A_Err release(const char*,int32){--suites;return 0;}
void reset(){fail=0;layer_count=2;selection=77;guard=0;three_d=false;cancel_menu=true;expected_selection=101;texture_inventory=false;item_flag_reads=0;
    name.clear();created=deleted=published=undo_start=undo_end=0;script_calls=script_handles=script_locks=0;utility.AEGP_ExecuteScript=nullptr;}
}
AEGP_PluginID node_graph_sync_plugin_id() noexcept{return 701;}
namespace starfield::adapter {
bool choose_transform_layer(PF_InData*,const std::vector<TransformLayerChoice>& choices,AEGP_LayerIDVal current,
                            AEGP_LayerIDVal& selected) noexcept {
    check(texture_inventory?(choices.size()==3 && choices[0].id==0 && choices[1].id==101 && choices[2].id==102 && current==0):
          (choices.size()==3 && choices[0].id==0 && choices[1].id==77 && choices[2].id==101 && current==77),
          "dropdown opens the composition inventory with stable IDs and current selection");
    selected=menu_selection;return !cancel_menu;
}
}
PF_Err sync_node_graph_parameter(PF_InData*,PF_OutData* out,PF_ParamDef* params[],const PF_UserChangedParamExtra* changed,bool) noexcept {
    ++published;check(changed->param_index==1 && selection==expected_selection && (!created || three_d) && guard==0 && params[guard_index]->u.fs_d.value==0,
        "graph publication sees the new 3D resource after guard restoration");
    if(fail==8){std::snprintf(out->return_msg,sizeof(out->return_msg),"detailed native binding failure");return 516;}
    return 0;
}
int main(){
    SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
    PF_InData data{};data.pica_basicP=&basic;data.effect_ref=reinterpret_cast<PF_ProgPtr>(1);data.time_scale=24;
    PF_OutData out{};std::array<PF_ParamDef,528> params{};std::array<PF_ParamDef*,528> pointers{};
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
    layers.AEGP_GetLayerID=[](AEGP_LayerH l,AEGP_LayerIDVal* v)->A_Err{if(fail==4)return 516;*v=l==layer(1)?77:99+static_cast<A_long>(reinterpret_cast<std::intptr_t>(l));return 0;};
    layers.AEGP_GetCompLayerByIndex=[](AEGP_CompH,A_long index,AEGP_LayerH* v)->A_Err{*v=layer(index+1);return 0;};
    layers.AEGP_GetLayerName=[](AEGP_PluginID,AEGP_LayerH l,AEGP_MemHandle* v,AEGP_MemHandle*)->A_Err{
        ++names;*v=reinterpret_cast<AEGP_MemHandle>(new std::u16string(texture_inventory?
            (l==layer(2)?u"Comp 2":u"Video footage"):(l==layer(1)?u"Null & one":u"Null two")));return 0;};
    layers.AEGP_GetLayerFlags=[](AEGP_LayerH l,AEGP_LayerFlags* v)->A_Err{
        *v=l==layer(5)?AEGP_LayerFlag_NULL_LAYER:0;return 0;};
    layers.AEGP_GetLayerSourceItem=[](AEGP_LayerH l,AEGP_ItemH* v)->A_Err{
        *v=l==layer(6)?nullptr:reinterpret_cast<AEGP_ItemH>(l);return 0;};
    items.AEGP_GetItemType=[](AEGP_ItemH item,AEGP_ItemType* v)->A_Err{
        if(fail==10)return 516;
        const auto id=reinterpret_cast<std::intptr_t>(item);
        *v=static_cast<AEGP_ItemType>(id==2?AEGP_ItemType_COMP:id==7?AEGP_ItemType_FOLDER:id==8?AEGP_ItemType_NONE:AEGP_ItemType_FOOTAGE);return 0;};
    items.AEGP_GetItemFlags=[](AEGP_ItemH item,AEGP_ItemFlags* v)->A_Err{
        ++item_flag_reads;check(item!=reinterpret_cast<AEGP_ItemH>(layer(2)),"precomp eligibility never depends on footage video-track flags");
        if(fail==11)return 516;
        *v=item==reinterpret_cast<AEGP_ItemH>(layer(4))?AEGP_ItemFlag_HAS_AUDIO:AEGP_ItemFlag_HAS_VIDEO;return 0;};
    memory.AEGP_LockMemHandle=[](AEGP_MemHandle h,void** v)->A_Err{
        if(reinterpret_cast<std::intptr_t>(h)==result_handle){if(fail==14)return 516;++script_locks;*v=script_result.data();return 0;}
        *v=const_cast<char16_t*>(reinterpret_cast<std::u16string*>(h)->c_str());return 0;};
    memory.AEGP_UnlockMemHandle=[](AEGP_MemHandle h)->A_Err{if(reinterpret_cast<std::intptr_t>(h)==result_handle)--script_locks;return 0;};
    memory.AEGP_FreeMemHandle=[](AEGP_MemHandle h)->A_Err{
        const auto id=reinterpret_cast<std::intptr_t>(h);if(id==result_handle || id==error_handle){--script_handles;return 0;}
        --names;delete reinterpret_cast<std::u16string*>(h);return 0;};
    memory.AEGP_GetMemHandleSize=[](AEGP_MemHandle,AEGP_MemSize* size)->A_Err{
        if(fail==15)return 516;*size=static_cast<AEGP_MemSize>(fail==18?starfield::adapter::kTextureInventoryMaxBytes+2:
            script_result.size()+(fail==19?0:1));return 0;};
    comps.AEGP_GetItemFromComp=[](AEGP_CompH c,AEGP_ItemH* v)->A_Err{
        check(c==reinterpret_cast<AEGP_CompH>(1),"script inventory uses the effect owner comp, independent of active UI");
        *v=reinterpret_cast<AEGP_ItemH>(44);return 0;};
    items.AEGP_GetItemID=[](AEGP_ItemH item,A_long* id)->A_Err{
        check(item==reinterpret_cast<AEGP_ItemH>(44),"script comp identity comes from the public Item suite");
        if(fail==13)return 516;*id=44;return 0;};
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
        const auto index=reinterpret_cast<std::intptr_t>(v);
        *t=(index==1 || index==521 || index==522)?AEGP_StreamType_LAYER_ID:AEGP_StreamType_OneD;return 0;};
    streams.AEGP_GetNewStreamValue=[](AEGP_PluginID,AEGP_StreamRefH v,AEGP_LTimeMode,const A_Time*,A_Boolean pre,AEGP_StreamValue2* out)->A_Err{
        check(pre,"captures the selector and guard without expression evaluation");if(fail==1)return 516;
        out->streamH=v;const auto index=reinterpret_cast<std::intptr_t>(v);
        if(index==1 || index==521 || index==522)out->val.layer_id=selection;else out->val.one_d=guard;return 0;};
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
    event.effect_win.current_frame.right=240;event.effect_win.current_frame.bottom=56;
    event.e_type=PF_Event_DO_CLICK;
    event.u.do_click.screen_point.h=159;event.u.do_click.screen_point.v=53;
    check(starfield::adapter::transform_null_event(&data,&out,pointers.data(),&event)==0 && !created && !event.evt_out_flags,
        "click outside the button cannot create a layer");
    event.u.do_click.screen_point.h=10;event.u.do_click.screen_point.v=40;
    check(starfield::adapter::transform_null_event(&data,&out,pointers.data(),&event)==0 && created==1 &&
        (event.evt_out_flags&PF_EO_HANDLED_EVENT),"button click is handled and creates exactly one layer");
    check(!suites && !refs,"event path balances host references");
    reset();out={};event.evt_out_flags=0;event.u.do_click.screen_point.v=10;
    check(starfield::adapter::transform_null_event(&data,&out,pointers.data(),&event)==0 && !created && !published && !undo_start,
          "canceling layer dropdown cannot mutate the graph or create a Null");
    cancel_menu=false;check(starfield::adapter::transform_null_event(&data,&out,pointers.data(),&event)==0 && !created && published==1 && selection==101,
          "layer dropdown selects an existing resource without creating another layer");
    check(!suites && !refs && !names && undo_start==undo_end,"layer inventory and selector release all resources");
    reset();expected_selection=0;
    check(starfield::adapter::set_transform_null_source(&data,&out,pointers.data(),0)==0 && selection==0 && !created && published==1,
          "None selection publishes identity without creating a layer");
    reset();check(starfield::adapter::set_transform_null_source(&data,&out,pointers.data(),999)!=0 && !undo_start && !created,
          "deleted resource rejects before changing the selector or opening undo");
    reset();fail=8;out={};
    check(starfield::adapter::set_transform_null_source(&data,&out,pointers.data(),101)!=0 && selection==77 && !deleted && guard==0,
          "existing-layer binding failure restores the previous resource without deleting any layer");
    check(!suites && !refs && !names,"all selector paths balance host ownership");
    reset();std::vector<starfield::adapter::TransformLayerChoice> choices;AEGP_LayerIDVal selected{};
    data.time_scale=0;
    check(starfield::adapter::read_transform_layers(&data,choices,selected,false)==0 && selected==77 &&
          choices.size()==2 && choices.back().id==77 && choices.back().name==u"Null & one",
          "selector paint reads the selected resource name even without DRAW frame timing");
    selection=0;
    check(starfield::adapter::read_transform_layers(&data,choices,selected,false)==0 && choices.size()==1 && choices[0].name==u"None",
          "None paint does not need the full composition name inventory");
    check(!suites && !refs && !names,"selector paint releases strings, streams and suites");
    reset();texture_inventory=true;layer_count=8;selection=0;
    for(const auto index:{521,522}) {
        item_flag_reads=0;
        check(starfield::adapter::read_transform_layers(&data,choices,selected,true,index)==0 &&
              selected==0 && choices.size()==3 && choices[0].id==0 &&
              choices[1].id==101 && choices[1].name==u"Comp 2" && choices[2].id==102,
              "both texture menus include empty precomps and video footage, excluding self/Null/audio/camera/folder");
        check(item_flag_reads==2 && !suites && !refs && !names,"only footage reads track flags and the texture inventory releases every reference");
    }
    selection=101;
    check(starfield::adapter::read_transform_layers(&data,choices,selected,false,521)==0 &&
          choices.size()==2 && choices.back().id==101 && choices.back().name==u"Comp 2" && item_flag_reads==2,
          "selected precomp paint resolves its stable ID without enumerating footage flags");
    for(const auto phase:{10,11}) {
        fail=phase;
        check(starfield::adapter::read_transform_layers(&data,choices,selected,true,521)==516 && !suites && !refs && !names,
              "item type/footage flag failures preserve the error and release all references");
    }
    reset();texture_inventory=true;selection=0;script_result="SF_TEXTURE_LAYERS_V1|44|77\n101|0043006f006d007000200032\n102|0056006900640065006f\n";
    utility.AEGP_ExecuteScript=[](AEGP_PluginID id,const A_char* script,A_Boolean platform,AEGP_MemHandle* result,AEGP_MemHandle* error)->A_Err{
        ++script_calls;check(id==701 && !platform,"read-only script uses the registered plugin and UTF8 result encoding");script_capture=script;
        check(script_capture.find("var cid=44,oid=77")!=std::string::npos && script_capture.find("activeItem")==std::string::npos &&
              script_capture.find("selectedLayers")==std::string::npos,"script is pinned to owner identities rather than UI selection");
        *error=reinterpret_cast<AEGP_MemHandle>(error_handle);++script_handles;
        if(fail!=16){*result=reinterpret_cast<AEGP_MemHandle>(result_handle);++script_handles;}
        return fail==12?516:0;
    };
    for(const auto index:{521,522}) {
        check(starfield::adapter::read_transform_layers(&data,choices,selected,true,index)==0 && choices.size()==3 &&
              choices[1].id==101 && choices[1].name==u"Comp 2" && choices[2].id==102 && item_flag_reads==0,
              "actual Texture click inventory uses scripting sources for both front and back menus");
        check(!suites && !refs && !script_handles && !script_locks,"script result/error handles and all suites balance");
    }
    event.effect_win.index=521;event.evt_out_flags=0;event.u.do_click.screen_point.v=10;
    check(starfield::adapter::texture_layer_event(&data,&out,pointers.data(),&event)==0 &&
          (event.evt_out_flags&PF_EO_HANDLED_EVENT) && script_calls==3 && !published && !undo_start,
          "Texture menu click reaches the owner-pinned inventory and cancellation performs no edit");
#if defined(STARFIELD_TEXTURE_DISPATCH_TEST)
    data.inter.add_param=[](PF_ProgPtr,PF_ParamIndex,PF_ParamDef* def)->PF_Err{registered_native.push_back(*def);return 0;};
    data.inter.register_ui=[](PF_ProgPtr,PF_CustomUIInfo* ui)->PF_Err{
        check(ui->events==PF_CustomEFlag_EFFECT,"actual Particle registers effect-window custom events");return 0;};
    PF_OutData registered_out{};
    check(EffectMain(PF_Cmd_PARAMS_SETUP,&data,&registered_out,nullptr,nullptr,nullptr)==0 &&
          registered_out.num_params==534 && registered_native.size()==533,
          "actual Particle registration preserves the complete physical parameter count");
    for(const auto index:{529,530,531}) {
        const auto& def=registered_native[index-1];
        check(def.param_type==PF_Param_FLOAT_SLIDER && def.uu.id==index-290 &&
              !(def.flags&PF_ParamFlag_CANNOT_TIME_VARY) && (def.flags&PF_ParamFlag_SUPERVISE),
              "Cloud public controls append unique IDs and support supervised animation");
        check(def.u.fs_d.valid_min==(index==531?0:1) && def.u.fs_d.valid_max==1000 &&
              def.u.fs_d.value==(index==529?10:index==530?150:66),"Cloud defaults and typed ranges match the numeric contract");
    }
    const auto& activation=registered_native[532];
    check(activation.uu.id==242 && activation.u.fs_d.value==0 &&
          (activation.flags&PF_ParamFlag_CANNOT_TIME_VARY) && (activation.ui_flags&PF_PUI_INVISIBLE),
          "constant hidden activation defaults to legacy without shifting prior controls");
    for(const auto index:{521,522}) {
        const auto& def=registered_native[index-1];
        check(def.param_type==PF_Param_LAYER && def.uu.id==(index==521?233:234) &&
              (def.ui_flags&PF_PUI_CONTROL) && def.ui_width==240 && def.ui_height==26,
              "registered Layer/Dark Side disk IDs and physical custom-control streams match dispatch");
        const auto before=script_calls;event.effect_win.index=index;event.evt_out_flags=0;
        check(EffectMain(PF_Cmd_EVENT,&data,&out,pointers.data(),nullptr,&event)==0 &&
              (event.evt_out_flags&PF_EO_HANDLED_EVENT) && script_calls==before+1 && !published,
              "actual Particle EffectMain dispatch opens the owner-pinned Texture menu without editing on cancel");
    }
#endif
    for(const auto phase:{12,13,14,15,16,18,19}) {
        fail=phase;
        check(starfield::adapter::read_transform_layers(&data,choices,selected,true,521)!=0 &&
              !suites && !refs && !script_handles && !script_locks,
              "script/identity/lock/size/null/unterminated failures preserve ownership and reject the menu");
    }
    fail=0;const auto calls=script_calls;
    check(starfield::adapter::read_transform_layers(&data,choices,selected,false,521)==0 && script_calls==calls,
          "None paint performs no scripting or full inventory scan");
#if defined(STARFIELD_TEXTURE_DISPATCH_TEST)
    std::ofstream script_file("artifacts/texture-selector-tests/generated-inventory.jsx");
    script_file<<script_capture;check(script_file.good(),"writes the actual generated script fixture for JS evaluation");
    script_file.close();
#endif
    using starfield::adapter::parse_texture_layer_inventory;
    const std::string prefix="SF_TEXTURE_LAYERS_V1|44|77\n";
    for(const auto row:{"77|0041\n","0|0041\n","2147483648|0041\n","101|0000\n","101|zzzz\n","101|d800\n",
        "101|dc00\n","101|004\n","101|0041","101|0041\n101|0042\n"})
        check(parse_texture_layer_inventory(prefix+row,44,77,choices)!=0,"malformed IDs/names/surrogates/duplicate rows reject");
    check(parse_texture_layer_inventory(prefix+"101|4e2dd83dde00\n",44,77,choices)==0 && choices[1].name==u"\u4e2d\U0001f600",
          "bounded UTF16 hex round-trips Chinese and a surrogate pair");
    std::printf("Transform Null UI: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
