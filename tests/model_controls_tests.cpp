#include "ModelControls.hpp"
#include "ModelGeometryParameter.hpp"
#include "NodeRecord.hpp"
#include "AE_EffectCB.h"
#include "Param_Utils.h"
#include "starfield/core/SequenceCodec.hpp"
#include "starfield/core/ModelGeometry.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>

using namespace starfield;
namespace {
namespace layout=adapter::native_nodes::model_layout;
using namespace core::graph_keys;
unsigned checks{},failures{},adds{},fail_add{},locks{};bool fail_allocate{};
#define CHECK(x) do{++checks;if(!(x)){++failures;std::printf("FAIL line %d: %s\n",__LINE__,#x);}}while(0)
std::map<PF_Handle,std::vector<std::byte>> handles;
std::vector<PF_ParamDef> registered;
core::NeverCancelled never;
struct Cancelled:core::Cancellation {bool is_cancelled()const noexcept override{return true;}} cancelled;
template<class T>T take(core::Result<T> result){if(!result.has_value())throw std::runtime_error(result.error().detail);return result.take_value();}
PF_Handle allocate(A_u_longlong size){if(fail_allocate){fail_allocate=false;return nullptr;}
    try{auto bytes=std::vector<std::byte>(static_cast<std::size_t>(size));auto* h=new void*(nullptr);handles.emplace(h,std::move(bytes));return h;}catch(const std::bad_alloc&){return nullptr;}}
void* lock(PF_Handle h){if(!handles.contains(h))return nullptr;++locks;return handles.at(h).data();}
void unlock(PF_Handle h){CHECK(handles.contains(h)&&locks);if(locks)--locks;}
void dispose(PF_Handle h){CHECK(handles.contains(h)&&!locks);if(handles.contains(h)){handles.erase(h);delete h;}}
A_u_longlong size_of(PF_Handle h){return handles.contains(h)?handles.at(h).size():0;}
PF_Err add(PF_ProgPtr,PF_ParamIndex index,PF_ParamDef* def){CHECK(index==-1&&def);++adds;if(adds==fail_add)return 516;registered.push_back(*def);return PF_Err_NONE;}
void clear(){while(!handles.empty())dispose(handles.begin()->first);registered.clear();adds=fail_add=0;}
core::NodeId nid(unsigned value){core::NodeId id;id.value.bytes[15]=static_cast<std::uint8_t>(value);return id;}
const core::ParameterValue& value(const core::GraphNode& node,core::ParameterKey key){for(const auto& p:node.parameters)if(p.key==key)return p.value;throw std::runtime_error("missing graph key");}
struct Controls {
    std::array<PF_ParamDef,layout::last+1> params{};
    std::array<PF_ParamDef*,layout::last+1> pointers{};
    Controls(){for(unsigned i=0;i<params.size();++i)pointers[i]=&params[i];
        for(unsigned i=1;i<params.size();++i)params[i]=registered.at(i-1);
        params[layout::mesh].u.arb_d.value=params[layout::mesh].u.arb_d.dephault;}
};
void registration(PF_InData& data){
    CHECK(adapter::register_model_author_controls(nullptr)==PF_Err_BAD_CALLBACK_PARAM);
    auto incomplete=data;incomplete.inter.add_param=nullptr;CHECK(adapter::register_model_author_controls(&incomplete)==PF_Err_BAD_CALLBACK_PARAM);
    incomplete=data;incomplete.utils=nullptr;CHECK(adapter::register_model_author_controls(&incomplete)==PF_Err_BAD_CALLBACK_PARAM);
    CHECK(adapter::register_model_author_controls(&data)==PF_Err_NONE&&registered.size()==18&&handles.size()==1);
    constexpr PF_ParamType types[]={PF_Param_POPUP,PF_Param_BUTTON,PF_Param_ARBITRARY_DATA,PF_Param_SLIDER,
        PF_Param_FLOAT_SLIDER,PF_Param_FLOAT_SLIDER,PF_Param_FLOAT_SLIDER,PF_Param_ANGLE,PF_Param_ANGLE,PF_Param_ANGLE,
        PF_Param_FLOAT_SLIDER,PF_Param_FLOAT_SLIDER,PF_Param_FLOAT_SLIDER,PF_Param_CHECKBOX,PF_Param_CHECKBOX,
        PF_Param_CHECKBOX,PF_Param_CHECKBOX,PF_Param_CHECKBOX};
    constexpr const char* names[]={"Source","Import OBJ","Model Mesh","Mesh Revision","Offset X","Offset Y","Offset Z",
        "Angle X","Angle Y","Angle Z","Scale X","Scale Y","Scale Z","Flip X","Flip Y","Flip Z","Center","Normalize"};
    for(int index=1;index<=layout::last;++index){const auto& d=registered[index-1];
        CHECK(d.uu.id==layout::disk_id(index)&&d.param_type==types[index-1]&&!std::strcmp(d.name,names[index-1]));
        if(index==layout::mesh)CHECK(d.flags==PF_ParamFlag_NONE&&d.u.arb_d.id==1503&&d.u.arb_d.dephault&&
            (d.ui_flags&(PF_PUI_NO_ECW_UI|PF_PUI_INVISIBLE))==(PF_PUI_NO_ECW_UI|PF_PUI_INVISIBLE));
        else CHECK((d.flags&PF_ParamFlag_SUPERVISE)&&bool(d.flags&PF_ParamFlag_CANNOT_TIME_VARY)==!layout::animated(index));
        if(index>=layout::scale&&index<=layout::scale+2)CHECK(d.u.fs_d.value==100&&d.u.fs_d.dephault==100&&
            d.u.fs_d.valid_min==-100000&&d.u.fs_d.valid_max==100000&&d.u.fs_d.slider_min==0&&d.u.fs_d.slider_max==200&&
            d.u.fs_d.display_flags==PF_ValueDisplayFlag_PERCENT);
        if(index>=layout::origin&&index<=layout::origin+2)CHECK(d.u.fs_d.value==0&&d.u.fs_d.valid_min==-1e9&&d.u.fs_d.valid_max==1e9&&
            d.u.fs_d.slider_min==-1&&d.u.fs_d.slider_max==1);
    }
    CHECK(registered[0].u.pd.value==1&&registered[0].u.pd.num_choices==2&&!std::strcmp(registered[0].u.pd.u.namesptr,"Cube|OBJ"));
    CHECK(!std::strcmp(registered[1].u.button_d.u.namesptr,"Import OBJ...")&&registered[3].u.sd.valid_max==2147483647);
    CHECK(adapter::native_nodes::disk_ids_are_unique_and_bounded());
    CHECK(adapter::native_nodes::parameter_count(adapter::native_nodes::Kind::particle)==537);
    clear();
    for(unsigned index=1;index<=18;++index){fail_add=index;
        CHECK(adapter::register_model_author_controls(&data)==516&&adds==index&&registered.size()==index-1);
        CHECK(handles.size()==(index>3?1u:0u));clear();}
    fail_allocate=true;CHECK(adapter::register_model_author_controls(&data)==PF_Err_OUT_OF_MEMORY&&registered.size()==2&&handles.empty());clear();
}
void graph_live(const core::GraphNode& model_node){
    core::Settings settings;settings.birth_rate=1;settings.emission_speed=0;settings.velocity={};settings.velocity_spread=0;
    core::EdgeId e1,e2,e3;e1.value.bytes[15]=1;e2.value.bytes[15]=2;e3.value.bytes[15]=3;
    auto graph=take(core::make_emitter_particle_output_graph(settings,nid(1),nid(2),nid(3),e1,e2));
    for(auto& n:graph.nodes)if(n.id==nid(2))for(auto& p:n.parameters)if(p.key==kParticleShape)p.value=std::uint32_t{4};
    graph.nodes.push_back(model_node);graph.edges.push_back({e3,model_node.id,kModelGeometryOut,nid(2),kParticleModelsIn});
    const auto encoded=core::serialize_graph(graph,core::particle_node_registry());CHECK(encoded.has_value());
    auto decoded=core::deserialize_graph(encoded.value(),core::particle_node_registry());CHECK(decoded.has_value());
    auto evaluated=take(core::evaluate_particle_graph(graph,{1,1},never));
    CHECK(!evaluated.particles.empty()&&evaluated.model_styles.size()==1&&evaluated.model_styles[0].instances.size()==1);
    const auto& matrix=evaluated.model_styles[0].instances[0].model_to_particle;
    CHECK(std::abs(matrix[12]-2)<1e-12&&std::abs(matrix[13]-3)<1e-12&&std::abs(matrix[14]-4)<1e-12);
}
void capture(PF_InData& data){
    CHECK(adapter::register_model_author_controls(&data)==PF_Err_NONE);Controls controls;
    auto state=take(adapter::capture_model_author_controls(&data,controls.pointers,never));
    CHECK(state.source==0&&state.revision==0&&state.geometry.positions.size()==8&&state.geometry.triangles.size()==12);
    CHECK(state.pose.scale_percent.x==100&&state.pose.scale_percent.y==100&&state.pose.scale_percent.z==100);
    auto node=take(adapter::model_author_graph_node(nid(4),state,never));CHECK(node.id==nid(4)&&node.type_key==kModelNode&&node.schema_version==1&&node.parameters.size()==12);
    CHECK(std::get<core::OpaqueBytes>(value(node,kModelResource))==core::OpaqueBytes(16));
    CHECK(std::get<std::uint32_t>(value(node,kModelSource))==0&&std::get<std::uint32_t>(value(node,kModelRevision))==0);
    for(int a=0;a<3;++a){controls.params[layout::origin+a].u.fs_d.value=2+a;
        controls.params[layout::scale+a].u.fs_d.value=50+50*a;controls.params[layout::rotation+a].u.ad.value=(30+30*a)*65536;}
    for(int b=0;b<5;++b)controls.params[layout::flip_x+b].u.bd.value=1;
    state=take(adapter::capture_model_author_controls(&data,controls.pointers,never));
    CHECK(state.pose.origin.x==2&&state.pose.origin.y==3&&state.pose.origin.z==4&&state.pose.rotation_degrees.x==30&&
        state.pose.rotation_degrees.y==60&&state.pose.rotation_degrees.z==90&&state.pose.scale_percent.x==50&&
        state.pose.scale_percent.y==100&&state.pose.scale_percent.z==150&&state.pose.flip_x&&state.pose.flip_y&&state.pose.flip_z&&state.pose.center&&state.pose.normalize);
    node=take(adapter::model_author_graph_node(nid(4),state,never));graph_live(node);
    for(int a=0;a<3;++a)controls.params[layout::rotation+a].u.ad.value=-65536;
    state=take(adapter::capture_model_author_controls(&data,controls.pointers,never));CHECK(state.pose.rotation_degrees.x==-1&&state.pose.rotation_degrees.y==-1&&state.pose.rotation_degrees.z==-1);
    CHECK(!adapter::capture_model_author_controls(nullptr,controls.pointers,never).has_value());
    CHECK(!adapter::capture_model_author_controls(&data,std::span(controls.pointers).first(18),never).has_value());
    auto result=adapter::capture_model_author_controls(&data,controls.pointers,cancelled);CHECK(!result.has_value()&&result.error().code==core::ErrorCode::cancelled);
    for(int index=1;index<=layout::last;++index){if(index==layout::import_obj)continue;
        auto* saved=controls.pointers[index];controls.pointers[index]=nullptr;CHECK(!adapter::capture_model_author_controls(&data,controls.pointers,never).has_value());controls.pointers[index]=saved;
        const auto type=controls.params[index].param_type;controls.params[index].param_type=PF_Param_NO_DATA;CHECK(!adapter::capture_model_author_controls(&data,controls.pointers,never).has_value());controls.params[index].param_type=type;}
    for(int index=layout::origin;index<layout::origin+3;++index){const auto saved=controls.params[index].u.fs_d.value;
        for(double bad:{1e9+1,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
            controls.params[index].u.fs_d.value=bad;CHECK(!adapter::capture_model_author_controls(&data,controls.pointers,never).has_value());}controls.params[index].u.fs_d.value=saved;}
    for(int index=layout::scale;index<layout::scale+3;++index){const auto saved=controls.params[index].u.fs_d.value;
        controls.params[index].u.fs_d.value=100001;CHECK(!adapter::capture_model_author_controls(&data,controls.pointers,never).has_value());controls.params[index].u.fs_d.value=saved;}
    for(int index=layout::flip_x;index<=layout::last;++index){controls.params[index].u.bd.value=2;
        CHECK(!adapter::capture_model_author_controls(&data,controls.pointers,never).has_value());controls.params[index].u.bd.value=1;}
    for(int source:{0,3}){controls.params[layout::source].u.pd.value=static_cast<A_short>(source);CHECK(!adapter::capture_model_author_controls(&data,controls.pointers,never).has_value());}
    controls.params[layout::source].u.pd.value=1;controls.params[layout::revision].u.sd.value=-1;CHECK(!adapter::capture_model_author_controls(&data,controls.pointers,never).has_value());controls.params[layout::revision].u.sd.value=0;
    CHECK(!adapter::model_author_graph_node({},state,never).has_value());
    state.source=2;CHECK(!adapter::model_author_graph_node(nid(4),state,never).has_value());state.source=0;
    state.revision=2147483648u;CHECK(!adapter::model_author_graph_node(nid(4),state,never).has_value());state.revision=0;
    CHECK(!adapter::model_author_graph_node(nid(4),state,cancelled).has_value());
    clear();
}
void imported(PF_InData& data){
    CHECK(adapter::register_model_author_controls(&data)==PF_Err_NONE);Controls controls;
    const auto original=controls.params[layout::mesh].u.arb_d.value;const auto original_bytes=handles.at(original);
    constexpr std::string_view obj="v 10 20 30\nv 14 20 30\nv 14 28 30\nv 10 28 30\nf 1 2 3 4\n";
    PF_ArbitraryH prepared=nullptr;CHECK(adapter::prepare_model_obj_parameter(&data,obj,&prepared,never)==PF_Err_NONE&&prepared&&prepared!=original);
    CHECK(controls.params[layout::source].u.pd.value==1&&controls.params[layout::revision].u.sd.value==0&&
        controls.params[layout::mesh].u.arb_d.value==original&&handles.at(original)==original_bytes);
    controls.params[layout::source].u.pd.value=2;controls.params[layout::mesh].u.arb_d.value=prepared;
    // OBJ placeholder ignores a parked mesh until the transaction publishes a revision.
    auto state=take(adapter::capture_model_author_controls(&data,controls.pointers,never));CHECK(state.geometry.positions.size()==8);
    auto node=take(adapter::model_author_graph_node(nid(4),state,never));CHECK(std::get<core::OpaqueBytes>(value(node,kModelResource))==core::OpaqueBytes(16));
    controls.params[layout::revision].u.sd.value=2147483647;
    state=take(adapter::capture_model_author_controls(&data,controls.pointers,never));CHECK(state.source==1&&state.revision==2147483647&&state.geometry.positions.size()==4&&state.geometry.triangles.size()==2);
    node=take(adapter::model_author_graph_node(nid(4),state,never));auto resource=std::get<core::OpaqueBytes>(value(node,kModelResource));CHECK(resource.size()==16&&resource[15]==std::byte{4});
    CHECK(std::get<std::uint32_t>(value(node,kModelRevision))==2147483647&&std::get<core::OpaqueBytes>(value(node,kModelBounds)).size()==48);
    const auto imported_bounds=std::get<core::OpaqueBytes>(value(node,kModelBounds));
    state.geometry.bounds={{0,0,0},{0,0,0}};
    CHECK(std::get<core::OpaqueBytes>(value(take(adapter::model_author_graph_node(nid(4),state,never)),kModelBounds))==imported_bounds);
    state.source=0;auto cube_node=take(adapter::model_author_graph_node(nid(4),state,never));
    CHECK(std::get<core::OpaqueBytes>(value(cube_node,kModelResource))==core::OpaqueBytes(16)&&
        std::get<core::OpaqueBytes>(value(cube_node,kModelBounds))!=imported_bounds);
    state.geometry={};CHECK(adapter::model_author_graph_node(nid(4),state,never).has_value());state.source=1;
    CHECK(!adapter::model_author_graph_node(nid(4),state,never).has_value());
    state=take(adapter::capture_model_author_controls(&data,controls.pointers,never));
    // Capture owns the geometry even if the host bytes change or disappear.
    handles.at(prepared).back()^=std::byte{1};CHECK(state.geometry.positions[0].value.x==10);
    CHECK(!adapter::capture_model_author_controls(&data,controls.pointers,never).has_value());
    controls.params[layout::source].u.pd.value=1;CHECK(adapter::capture_model_author_controls(&data,controls.pointers,never).has_value());
    dispose(prepared);controls.params[layout::mesh].u.arb_d.value=nullptr;controls.params[layout::source].u.pd.value=2;
    CHECK(!adapter::capture_model_author_controls(&data,controls.pointers,never).has_value());
    PF_ArbitraryH failed=reinterpret_cast<PF_ArbitraryH>(1);CHECK(adapter::prepare_model_obj_parameter(&data,"bad obj",&failed,never)==PF_Err_BAD_CALLBACK_PARAM&&!failed);
    CHECK(adapter::prepare_model_obj_parameter(&data,obj,nullptr,never)==PF_Err_BAD_CALLBACK_PARAM);
    failed=reinterpret_cast<PF_ArbitraryH>(1);CHECK(adapter::prepare_model_obj_parameter(&data,obj,&failed,cancelled)==PF_Interrupt_CANCEL&&!failed);
    fail_allocate=true;CHECK(adapter::prepare_model_obj_parameter(&data,obj,&failed,never)==PF_Err_OUT_OF_MEMORY&&!failed&&handles.size()==1);
    CHECK(handles.at(original)==original_bytes&&locks==0);clear();
}
}
int main(){PF_UtilCallbacks utils{};utils.host_new_handle=allocate;utils.host_lock_handle=lock;utils.host_unlock_handle=unlock;utils.host_dispose_handle=dispose;utils.host_get_handle_size=size_of;
    PF_InData data{};data.utils=&utils;data.inter.add_param=add;
    try{registration(data);capture(data);imported(data);CHECK(handles.empty()&&locks==0);
        std::printf("Model controls: %u checks, %u failures (May2023 SDK/fake host)\n",checks,failures);return failures?1:0;}
    catch(const std::exception& error){std::printf("FAILED: %s\n",error.what());clear();return 1;}}
