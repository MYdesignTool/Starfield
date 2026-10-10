#include "ParticleShape.hpp"
#include "ParticleShapeUI.hpp"
#include "NodeEffects.hpp"
#include "NodeGraphSync.hpp"
#include "GpuRender.hpp"
#include "ParticleGradientUI.hpp"
#include "UiExclusionClient.hpp"
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>

namespace {
unsigned checks{},picker_calls{},sync_calls{};std::uint32_t picked=6;bool cancelled{};PF_Err sync_error{};
using Begin=std::uint64_t(*)() noexcept;using End=void(*)(std::uint64_t) noexcept;using Notify=void(*)() noexcept;
Begin begin{};End end{};
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"line "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
}
namespace starfield::adapter {
bool choose_particle_shape(PF_InData*,std::uint32_t current,std::uint32_t& selected) noexcept {
    ++picker_calls;CHECK(current==1);CHECK(begin()==0);selected=picked;return !cancelled;
}
}
PF_Err sync_node_graph_parameter(PF_InData*,PF_OutData*,PF_ParamDef* params[],const PF_UserChangedParamExtra* edit,bool) noexcept {
    ++sync_calls;CHECK(begin()==0);CHECK(edit->param_index==starfield::adapter::native_nodes::particle_layout::shape);
    CHECK(params[edit->param_index]->u.pd.value==6);
    CHECK(params[edit->param_index]->uu.change_flags&PF_ChangeFlag_CHANGED_VALUE);return sync_error;
}
PF_Err register_node_graph_sync(PF_InData*) noexcept{return PF_Err_NONE;}
AEGP_PluginID node_graph_sync_plugin_id() noexcept{return 1;}
namespace starfield::adapter {
PF_Err gpu_device_setup(PF_InData*,PF_OutData*,PF_GPUDeviceSetupExtra*) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err gpu_device_setdown(PF_InData*,PF_GPUDeviceSetdownExtra*) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
bool gpu_device_matches(const void*,PF_GPU_Framework,A_u_long) noexcept{return false;}
PF_Err copy_gpu_pixels(PF_InData*,const void*,PF_GPU_Framework,A_u_long,PF_EffectWorld*,PF_EffectWorld*) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err particle_gradient_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept{return PF_Err_NONE;}
PF_Err particle_rotation_curve_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept{return PF_Err_NONE;}
PF_Err particle_gradient_param_ui(PF_InData*,PF_ParamDef*[]) noexcept{return PF_Err_NONE;}
void clear_particle_gradient_ui() noexcept{}
}
int main(int argc,char** argv) {
    namespace shapes=starfield::adapter::particle_shapes;namespace layout=starfield::adapter::native_nodes::particle_layout;
    constexpr auto guard=starfield::adapter::native_nodes::sync_guard_index(starfield::adapter::native_nodes::Kind::particle);
    CHECK(argc==2);
    const std::array<std::u16string,6> names{u"Circle",u"Rectangle",u"Cloud",u"Texture",u"Face",u"Model"};
    for(unsigned i=0;i<names.size();++i){CHECK(names[i]==shapes::choices[i].label);CHECK(shapes::choices[i].native==i+1);CHECK(shapes::choices[i].enabled==(i!=4));}
    CHECK(std::string(shapes::popup_names)=="Circle|Rectangle|Cloud|Texture|Face|Model");
    for(std::uint32_t i=0;i<=4;++i){std::uint32_t native=0,core=99;CHECK(shapes::core_to_native(i,native));
        CHECK(native==(i==4?6:i+1));CHECK(shapes::native_to_core(native,core));CHECK(core==i);}
    for(double native:{-1.,0.,1.5,5.,7.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
        std::uint32_t core=99;CHECK(!shapes::native_to_core(native,core));CHECK(core==99);
    }
    std::uint32_t native=99;CHECK(!shapes::core_to_native(5,native));CHECK(native==99);
    starfield::adapter::node_sync::NativeEdit edit{};edit.node_kind=1;edit.parameter_index=layout::shape;edit.uuid[7]=1;
    for(double v:{1.,2.,3.,4.,6.}){edit.value[0]=v;CHECK(starfield::adapter::node_sync::valid_edit(edit));}
    for(double v:{0.,1.5,5.,7.}){edit.value[0]=v;CHECK(!starfield::adapter::node_sync::valid_edit(edit));}
    std::array<PF_ParamDef,guard+1> values{};std::array<PF_ParamDef*,guard+1> params{};
    for(unsigned i=0;i<values.size();++i)params[i]=&values[i];
    values[layout::shape].param_type=PF_Param_POPUP;values[layout::shape].u.pd.value=1;values[layout::shape].u.pd.num_choices=6;
    values[guard].param_type=PF_Param_FLOAT_SLIDER;
    PF_InData data{};PF_OutData out{};PF_EventExtra event{};
    static unsigned registered{};static PF_ParamDef shape{};
    data.inter.add_param=[](PF_ProgPtr,A_long,PF_ParamDef* parameter)->PF_Err {
        ++registered;if(parameter->uu.id==starfield::adapter::native_nodes::disk_ids::kParticleShapeId)shape=*parameter;return PF_Err_NONE;
    };
    CHECK(EffectMain(PF_Cmd_PARAMS_SETUP,&data,&out,params.data(),nullptr,nullptr)==0);
    CHECK(out.num_params==static_cast<A_long>(registered+1) && shape.param_type==PF_Param_POPUP);
    CHECK(shape.u.pd.num_choices==6&&shape.u.pd.value==1&&shape.u.pd.dephault==1);
    CHECK(std::string(shape.u.pd.u.namesptr)==shapes::popup_names);
    CHECK((shape.ui_flags&PF_PUI_CONTROL)&&shape.ui_height==26);
    const auto reset=[&]{event={};event.contextH=reinterpret_cast<PF_ContextH>(1);event.effect_win.index=layout::shape;
        event.effect_win.area=PF_EA_CONTROL;event.effect_win.current_frame.left=0;event.effect_win.current_frame.top=0;
        event.effect_win.current_frame.right=240;event.effect_win.current_frame.bottom=30;
        event.e_type=PF_Event_DO_CLICK;event.u.do_click.screen_point={10,10};values[layout::shape].u.pd.value=1;
        values[layout::shape].uu.change_flags=0;values[guard].u.fs_d.value=0;out={};picked=6;cancelled=false;sync_error=0;};
    const auto call=[&]{return EffectMain(PF_Cmd_EVENT,&data,&out,params.data(),nullptr,&event);};
    reset();CHECK(call()==0&&picker_calls==0&&sync_calls==0); // Missing resident Host fails closed.
    const auto module=LoadLibraryA(argv[1]);CHECK(module);
    begin=std::bit_cast<Begin>(GetProcAddress(module,"SFLD_BeginUiExclusionV1"));
    end=std::bit_cast<End>(GetProcAddress(module,"SFLD_EndUiExclusionV1"));
    const auto initialize=std::bit_cast<Notify>(GetProcAddress(module,"SFLD_TestInitialize"));CHECK(begin&&end&&initialize);initialize();
    reset();{starfield::adapter::EffectUiExclusion busy;CHECK(bool(busy));CHECK(call()==0&&picker_calls==0);}
    reset();values[guard].u.fs_d.value=1;CHECK(call()==0&&picker_calls==0);
    reset();event.u.do_click.screen_point={100,100};CHECK(call()==0&&picker_calls==0);
    reset();cancelled=true;CHECK(call()==0&&picker_calls==1&&sync_calls==0&&!event.u.do_click.send_drag);
    reset();picked=1;CHECK(call()==0&&!event.u.do_click.send_drag&&sync_calls==0);
    reset();picked=5;CHECK(call()==PF_Err_BAD_CALLBACK_PARAM&&sync_calls==0&&values[layout::shape].u.pd.value==1);
    reset();CHECK(call()==0&&event.u.do_click.send_drag&&event.u.do_click.continue_refcon[0]==6);
    CHECK(values[layout::shape].u.pd.value==1&&sync_calls==0); // No parameter write on DO_CLICK.
    event.e_type=PF_Event_DRAG;CHECK(call()==0&&sync_calls==1&&values[layout::shape].u.pd.value==6);
    CHECK(event.u.do_click.continue_refcon[0]==0&&(out.out_flags&PF_OutFlag_FORCE_RERENDER));
    CHECK(call()==0&&sync_calls==1); // Repeated DRAG cannot publish twice.
    reset();CHECK(call()==0);values[layout::shape].u.pd.value=2;event.e_type=PF_Event_DRAG;
    CHECK(call()==PF_Err_BAD_CALLBACK_PARAM&&sync_calls==1&&values[layout::shape].u.pd.value==2);
    reset();CHECK(call()==0);event.e_type=PF_Event_DRAG;sync_error=516;
    const auto previous=values[layout::shape];CHECK(call()==516&&sync_calls==2);
    CHECK(std::memcmp(&previous,&values[layout::shape],sizeof(previous))==0&&out.out_flags==0);
    const auto free_token=begin();CHECK(free_token!=0);end(free_token);
    CHECK(FreeLibrary(module));std::cout<<"Particle Shape UI: "<<checks<<" checks, 0 failures (SDK/fake picker/publication)\n";
}
