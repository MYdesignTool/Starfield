#include "Parameters.hpp"
#include "MotionBlur.hpp"
#include "GraphCarrier.hpp"
#include "NativeNodeGraph.hpp"
#include "NativeTemporalUI.hpp"
#include "Camera.hpp"
#include "EmitterHistory.hpp"
#include "SPBasic.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <vector>
namespace starfield::adapter {
void capture_native_temporal_metadata(PF_InData*,const core::Graph&,AEGP_PluginID) noexcept {}
struct NativeBindingTransaction::Impl {};
NativeBindingTransaction::NativeBindingTransaction(PF_InData*,AEGP_PluginID,AEGP_EffectRefH):impl_(std::make_unique<Impl>()){}
NativeBindingTransaction::~NativeBindingTransaction()=default;
PF_Err NativeBindingTransaction::install(const core::Graph&,A_long*,const char**,A_long*) noexcept {return 0;}
void NativeBindingTransaction::accept() noexcept {}
PF_Err sample_native_node_animation(PF_InData*,core::Graph&,A_long,A_long,A_long*,const char**,const core::NodeId*) noexcept {return 0;}
PF_Err compile_native_node_graph(PF_InData*,PF_ParamDef*[],core::Graph&,bool& found,AEGP_PluginID,const node_sync::NativeEdit*) noexcept {
    found=false;return PF_Err_BAD_CALLBACK_PARAM;
}
PF_Err capture_camera(PF_InData*,SfCoreRenderRequest&,const PF_InData*) noexcept {return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err capture_motion_particles(PF_InData*,PF_OutData*,const core::Graph&,A_long,A_long,std::span<const core::RationalTime>,
    const core::Cancellation&,std::vector<CapturedParticleFrame>&) noexcept {return PF_Err_BAD_CALLBACK_PARAM;}
}
using namespace starfield;using namespace starfield::adapter;
namespace {
int checks{},failures{},acquires{},releases{};
#define CHECK(x) do {++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}} while(0)
std::map<PF_Handle,std::vector<std::byte>> memory;
std::vector<PF_ParamDef> registered;
std::array<PF_ParamDef,kTotalEffectParameterCount+1> values{};
std::set<PF_ParamDef*> checkouts;
std::vector<A_long> checkout_indices;
PF_Handle allocate(A_u_longlong size) {auto handle=new void*(nullptr);memory[handle].resize(size);*handle=memory[handle].data();return handle;}
void* lock(PF_Handle handle){return memory.at(handle).data();}
void unlock(PF_Handle){}
void dispose(PF_Handle handle){if(handle){CHECK(memory.erase(handle)==1);delete handle;}}
A_u_longlong size(PF_Handle handle){return memory.at(handle).size();}
PF_Err add(PF_ProgPtr,PF_ParamIndex,PF_ParamDef* def){registered.push_back(*def);return 0;}
PF_Err checkout(PF_ProgPtr,PF_ParamIndex index,A_long,A_long,A_u_long,PF_ParamDef* def){
    CHECK(index>0 && index<=kTotalEffectParameterCount);*def=values[index];checkouts.insert(def);checkout_indices.push_back(index);return 0;
}
PF_Err checkin(PF_ProgPtr,PF_ParamDef* def){CHECK(checkouts.erase(def)==1);return 0;}
PF_ParamUtilsSuite3 ui{};
PF_Err update(PF_ProgPtr,PF_ParamIndex index,const PF_ParamDef* def){values[index].ui_flags=def->ui_flags;return 0;}
SPErr acquire(const char* name,int version,const void** output){
    if(std::strcmp(name,kPFParamUtilsSuite) || version!=kPFParamUtilsSuiteVersion3)return 1;
    ++acquires;*output=&ui;return 0;
}
SPErr release(const char*,int){++releases;return 0;}
}
int main(){
    PF_UtilCallbacks utils{};utils.host_new_handle=allocate;utils.host_lock_handle=lock;utils.host_unlock_handle=unlock;
    utils.host_dispose_handle=dispose;utils.host_get_handle_size=size;
    PF_InData host{};host.utils=&utils;host.inter.add_param=add;host.inter.checkout_param=checkout;host.inter.checkin_param=checkin;
    host.width=host.height=64;host.pixel_aspect_ratio={1,1};host.current_time=24;host.time_step=1;host.time_scale=24;
    PF_OutData out{};CHECK(setup_parameters(&host,&out)==0);CHECK(out.num_params==755 && registered.size()==754);
    std::set<A_long> disks;
    for(std::size_t i=0;i<registered.size();++i){
        CHECK(disks.insert(registered[i].uu.id).second);values[i+1]=registered[i];
        auto& value=values[i+1];if(value.param_type==PF_Param_FLOAT_SLIDER)value.u.fs_d.value=value.u.fs_d.dephault;
        else if(value.param_type==PF_Param_POPUP)value.u.pd.value=value.u.pd.dephault;
        else if(value.param_type==PF_Param_CHECKBOX)value.u.bd.value=value.u.bd.dephault;
    }
    CHECK(values[1].uu.id==1631 && values[1].ui_height==130);
    CHECK(values[90].uu.id==920 && values[94].uu.id==924);
    CHECK(values[91].uu.id==921 && std::strcmp(values[91].name,"On / Off")==0);
    for(A_long i=610;i<=619;++i)CHECK(values[i].uu.id==1640+i-610);
    for(A_long i=620;i<=622;++i)CHECK(values[i].uu.id==1600+i-620);
    for(A_long i=623;i<=625;++i)CHECK(values[i].uu.id==1610+i-623);
    CHECK(values[610].param_type==PF_Param_GROUP_START && values[619].param_type==PF_Param_GROUP_END);
    for(A_long i=98;i<610;++i)CHECK(values[i].uu.id==1000+i-98);
    for(A_long i=626;i<754;++i)CHECK(values[i].uu.id==1700+i-626 && values[i].param_type==PF_Param_LAYER);
    CHECK(values[754].uu.id==1828 && values[754].param_type==PF_Param_FLOAT_SLIDER);
    std::array<PF_ParamDef*,kTotalEffectParameterCount+1> pointers{};for(std::size_t i=0;i<values.size();++i)pointers[i]=&values[i];
    ui.PF_UpdateParamUI=update;SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;host.pica_basicP=&basic;
    for(int mode=1;mode<=3;++mode)for(int type=1;type<=2;++type){
        values[kMotionParameterIds[0]].u.pd.value=mode;values[kMotionParameterIds[3]].u.pd.value=type;
        CHECK(update_motion_ui(&host,pointers.data())==0);
        for(std::size_t i=1;i<8;++i){const bool expected=mode==1 || ((i==1 || i==2)&&mode!=3) || (i==4&&type==1) || (i==5&&type!=1);
            CHECK(bool(values[kMotionParameterIds[i]].ui_flags&PF_PUI_DISABLED)==expected);}
    }
    CHECK(acquires==releases);host.pica_basicP=nullptr;
    auto graph=graph_from_controls(core::Settings{});CHECK(graph.has_value());
    PF_ArbitraryH current{};CHECK(create_graph_parameter(&host,graph.value(),&current)==0);
    values[kGraphParameterId].u.arb_d.value=current;values[kControlSourceId].u.pd.value=kNodeControlSource;
    values[kTimeSamplingHzId].u.pd.value=3;values[kAccelerationId].u.pd.value=2;
    const double authored[]{2,180,-20,1,16,88,20,1};
    for(std::size_t i=0;i<8;++i){if(motion_popup(i))values[kMotionParameterIds[i]].u.pd.value=static_cast<A_long>(authored[i])+1;
        else values[kMotionParameterIds[i]].u.fs_d.value=authored[i];}
    host.num_params=0;std::shared_ptr<const core::Graph> frozen;CHECK(checkout_render_graph(&host,&out,frozen)==0);
    CHECK(frozen && checkouts.empty());
    if(frozen)for(const auto& node:frozen->nodes)if(node.type_key==core::graph_keys::kOutputNode){
        for(const auto& parameter:node.parameters){
            if(parameter.key==core::graph_keys::kTimeSamplingHz)CHECK(std::get<std::uint32_t>(parameter.value)==120);
            if(parameter.key==core::graph_keys::kAcceleration)CHECK(std::get<std::uint32_t>(parameter.value)==1);
            for(std::size_t i=0;i<8;++i)if(parameter.key==kMotionParameterKeys[i])
                CHECK((motion_popup(i)?double(std::get<std::uint32_t>(parameter.value)):std::get<double>(parameter.value))==authored[i]);
        }
    }
    for(auto index:kMotionParameterIds)CHECK(std::find(checkout_indices.begin(),checkout_indices.end(),index)!=checkout_indices.end());
    dispose(current);dispose(registered[kGraphParameterId-1].u.arb_d.dephault);CHECK(memory.empty() && checkouts.empty());
    std::printf("Main parameter registration/UI/checkout: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
