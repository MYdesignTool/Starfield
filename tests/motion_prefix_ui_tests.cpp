#include "MotionPrefixUI.hpp"
#include "MotionPrefix.hpp"
#include "NodeRecord.hpp"
#include "UiExclusionClient.hpp"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace {
using namespace starfield::adapter;
namespace layout=native_nodes::motion_layout;
unsigned checks{},held{},suites{},dialogs{},publications{},undos{},ends{};
int fault{},checkouts{};bool cancel{};
motion_prefix::Bank bank{};
constexpr auto first=native_nodes::uuid_first_index(native_nodes::Kind::motion);
#define CHECK(x) do{++checks;if(!(x)){std::printf("FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(false)
AEGP_UtilitySuite6 utility{};AEGP_PFInterfaceSuite1 pf{};AEGP_LayerSuite9 layers{};
AEGP_CompSuite11 comps{};AEGP_ItemSuite9 items{};AEGP_ProjSuite6 projects{};
PF_Err checkout(PF_ProgPtr,PF_ParamIndex index,A_long time,A_long step,A_u_long scale,PF_ParamDef* out){
    CHECK(time==12 && step==0 && scale==24);++checkouts;
    if(fault==1)return 512;
    ++held;*out={};out->param_type=PF_Param_FLOAT_SLIDER;
    if(index==layout::mode){out->param_type=PF_Param_POPUP;out->u.pd.value=fault==2?1:2;}
    else if(index>=first && index<first+8)out->u.fs_d.value=(fault==3 && dialogs)?8:1;
    else if(index>=143 && index<=186)out->u.fs_d.value=bank[index-143];
    return 0;
}
PF_Err checkin(PF_ProgPtr,PF_ParamDef*){CHECK(held==1);--held;return 0;}
SPAPI SPErr acquire(const char* name,int32,const void** out){
    if(!std::strcmp(name,kAEGPUtilitySuite))*out=&utility;
    else if(!std::strcmp(name,kAEGPPFInterfaceSuite))*out=&pf;
    else if(!std::strcmp(name,kAEGPLayerSuite))*out=&layers;
    else if(!std::strcmp(name,kAEGPCompSuite))*out=&comps;
    else if(!std::strcmp(name,kAEGPItemSuite))*out=&items;
    else if(!std::strcmp(name,kAEGPProjSuite))*out=&projects;
    else return 1;
    ++suites;return 0;
}
SPAPI SPErr release(const char*,int32){CHECK(suites>0);--suites;return 0;}
INT_PTR WINAPI chooser(HINSTANCE,LPCDLGTEMPLATEW,HWND,DLGPROC,LPARAM);
}
PF_Err sync_motion_prefix_graph_parameter(PF_InData* data,PF_OutData*,PF_ParamDef* params[],
    const starfield::adapter::motion_prefix::Bank& old,const starfield::adapter::EffectUiExclusion& owner) noexcept {
    using namespace starfield::adapter;
    CHECK(owner && ui_transaction_active() && !held && undos==1 && ends==0);
    CHECK(data->num_params==native_nodes::parameter_count(native_nodes::Kind::motion) && old==bank);
    motion_prefix::Bank wanted{};CHECK(motion_prefix::encode(L"Light_\u706f",wanted));
    for(unsigned i=0;i<44;++i)CHECK(params[143+i]->u.fs_d.value==wanted[i]);
    for(unsigned i=0;i<8;++i)CHECK(params[first+i]->u.fs_d.value==1);
    ++publications;return fault==4?512:0;
}
#define DialogBoxIndirectParamW chooser
#include "../ae_plugin/MotionPrefixUI.cpp"
#undef DialogBoxIndirectParamW
namespace {
INT_PTR WINAPI chooser(HINSTANCE,LPCDLGTEMPLATEW,HWND owner,DLGPROC,LPARAM state){
    CHECK(!held && !suites && ui_transaction_active() && owner==reinterpret_cast<HWND>(1));++dialogs;
    reinterpret_cast<starfield::adapter::Dialog*>(state)->text=L"Light_\u706f";
    if(fault==7)CHECK(motion_prefix::encode(L"external change",bank));
    return cancel?0:1;
}
void reset(){held=suites=dialogs=publications=undos=ends=0;fault=checkouts=0;cancel=false;bank={};}
}
int main(){
    auto module=LoadLibraryW(L"artifacts/model-ui-exclusion-tests/paired/StarfieldHost.aex");CHECK(module);
    const auto initialize=std::bit_cast<void(*)() noexcept>(GetProcAddress(module,"SFLD_TestInitialize"));CHECK(initialize);initialize();
    utility.AEGP_GetMainHWND=[](void* out)->A_Err{*static_cast<HWND*>(out)=reinterpret_cast<HWND>(1);return 0;};
    utility.AEGP_StartUndoGroup=[](const char*)->A_Err{++undos;return 0;};
    utility.AEGP_EndUndoGroup=[]()->A_Err{++ends;return 0;};
    pf.AEGP_GetEffectLayer=[](PF_ProgPtr,AEGP_LayerH* out)->A_Err{*out=reinterpret_cast<AEGP_LayerH>(1);return 0;};
    layers.AEGP_GetLayerID=[](AEGP_LayerH,A_long* out)->A_Err{*out=fault==5 && dialogs?5:4;return 0;};
    layers.AEGP_GetLayerParentComp=[](AEGP_LayerH,AEGP_CompH* out)->A_Err{*out=reinterpret_cast<AEGP_CompH>(1);return 0;};
    layers.AEGP_GetLayerCurrentTime=[](AEGP_LayerH,AEGP_LTimeMode,A_Time* out)->A_Err{*out={fault==6 && dialogs?13:12,24};return 0;};
    comps.AEGP_GetItemFromComp=[](AEGP_CompH,AEGP_ItemH* out)->A_Err{*out=reinterpret_cast<AEGP_ItemH>(1);return 0;};
    items.AEGP_GetItemID=[](AEGP_ItemH,A_long* out)->A_Err{*out=2;return 0;};
    projects.AEGP_GetProjectByIndex=[](A_long,AEGP_ProjectH* out)->A_Err{*out=reinterpret_cast<AEGP_ProjectH>(1);return 0;};
    projects.AEGP_GetProjectRootFolder=[](AEGP_ProjectH,AEGP_ItemH* out)->A_Err{*out=reinterpret_cast<AEGP_ItemH>(1);return 0;};
    SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
    PF_InData data{};data.pica_basicP=&basic;data.effect_ref=reinterpret_cast<PF_ProgPtr>(1);data.time_scale=24;data.current_time=12;
    data.inter.checkout_param=checkout;data.inter.checkin_param=checkin;
    for(auto count:{0,1,100,509}){reset();data.num_params=count;PF_OutData out{};
        CHECK(edit_motion_prefix(&data,&out)==0);
        CHECK(data.num_params==count && publications==1 && dialogs==1 && !held && !suites && undos==1 && ends==1 && !ui_transaction_active());}
    for(int failure=1;failure<=7;++failure){reset();fault=failure;PF_OutData out{};
        CHECK(edit_motion_prefix(&data,&out)!=0);
        CHECK(!held && !suites && !ui_transaction_active());
        CHECK(publications==(fault==4?1u:0u) && undos==ends);}
    reset();cancel=true;{PF_OutData out{};CHECK(edit_motion_prefix(&data,&out)==0 && !publications && !undos);}
    reset();bank[0]=1;bank[1]=0xff;{PF_OutData out{};CHECK(edit_motion_prefix(&data,&out)!=0 && !dialogs && !publications);}
    reset();data.num_params=0;
    {PF_OutData out{};PF_EventExtra event{};event.contextH=reinterpret_cast<PF_ContextH>(1);
        event.effect_win.index=143;event.effect_win.area=PF_EA_CONTROL;event.effect_win.current_frame={0,0,300,48};
        event.e_type=PF_Event_DO_CLICK;event.u.do_click.screen_point={5,5};
        CHECK(motion_prefix_event(&data,&out,reinterpret_cast<PF_ParamDef**>(1),&event)==0 && publications==1);
        CHECK(event.evt_out_flags&PF_EO_HANDLED_EVENT);
        reset();fault=2;CHECK(motion_prefix_event(&data,&out,nullptr,&event)==0 && !dialogs && !publications);}
    CHECK(FreeLibrary(module));std::printf("Motion prefix UI: %u checks, 0 failures; owned input, modal exclusion and stale target checks.\n",checks);
}
