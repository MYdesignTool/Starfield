// Actual SmartFX pre_render + graph codec; host/GPU callbacks are memory-only.
#include "SmartRender.hpp"
#include "GraphOutputScope.hpp"
#include "CoreLoader.hpp"
#include "Camera.hpp"
#include "GpuRender.hpp"
#include "EmitterHistory.hpp"
#include "TextureResources.hpp"
#include "ModelRenderResources.hpp"
#include "MotionLightSources.hpp"
#include "MotionLightTiming.hpp"
#include "MotionPointCapture.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include <cstdio>
#include <memory>
#include <utility>
#include <cstdlib>
#include <new>
using namespace starfield;
namespace {bool fail_next_allocation{},arm_state_failure{};}
_Use_decl_annotations_ void* operator new(std::size_t n){if(fail_next_allocation){fail_next_allocation=false;throw std::bad_alloc{};}if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc{};}
_Use_decl_annotations_ void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
unsigned checks{},failures{},light_requests{},motion_nodes{},history_nodes{};
bool lights_needed{};
std::shared_ptr<const core::Graph> author;
void check(bool value,const char* label){++checks;if(!value){++failures;std::printf("FAIL: %s\n",label);}}
core::NodeId id(unsigned n){core::NodeId result{};result.value.bytes[15]=static_cast<std::uint8_t>(n);return result;}
void set(core::GraphNode& node,core::ParameterKey key,core::ParameterValue value){
    for(auto& p:node.parameters)if(p.key==key){p.value=std::move(value);return;}node.parameters.push_back({key,std::move(value)});
}
core::Graph base(){core::Settings settings;auto result=core::make_emitter_particle_output_graph(settings,id(1),id(2),id(3),{id(4).value},{id(5).value}).take_value();
    set(result.nodes.back(),core::graph_keys::kAcceleration,std::uint32_t{1});return result;}
}
namespace starfield::adapter {
AEGP_PluginID graph_carrier_plugin_id() noexcept{return 1;}
CoreGeneration::CoreGeneration(void* m,SfCoreApi api,std::uint64_t n,std::wstring p) noexcept:module_(m),api_(api),cache_identity_(n),path_(std::move(p)){}
CoreGeneration::~CoreGeneration() noexcept=default;
CoreLoadResult acquire_core() noexcept{auto lease=std::make_shared<CoreGeneration>(nullptr,SfCoreApi{},42,L"memory test");if(arm_state_failure){arm_state_failure=false;fail_next_allocation=true;}return {std::move(lease),{},false};}
PF_Err checkout_render_graph(PF_InData*,PF_OutData*,std::shared_ptr<const core::Graph>& graph,A_long*,A_long,A_long) noexcept{graph=author;return 0;}
PF_Err prepare_motion_exposure(PF_InData*,PF_OutData*,const core::Graph& graph,core::LayerUnits,const core::Cancellation&,MotionExposure& out) noexcept{motion_nodes=static_cast<unsigned>(graph.nodes.size());out={};return 0;}
PF_Err capture_emitter_origin_history(PF_InData*,PF_OutData*,core::Graph& graph,A_long,A_long,const core::Cancellation&) noexcept{history_nodes=static_cast<unsigned>(graph.nodes.size());return 0;}
PF_Err prepare_texture_resources(PF_InData*,PF_OutData*,PF_PreRenderExtra*,const core::Graph&,const MotionExposure&,A_long,double,const core::Cancellation&,TexturePreparation&,const core::Graph* layout) noexcept{check(layout==author.get(),"texture keeps complete original slot layout");return 0;}
PF_Err prepare_model_render_resources(PF_InData*,PF_OutData*,PF_PreRenderExtra*,const core::Graph&,const MotionExposure&,A_long,double,const core::Cancellation&,ModelRenderResources&,const core::Graph* layout) noexcept{check(layout==author.get(),"Model keeps complete original mirror layout");return 0;}
PF_Err capture_lights(PF_InData*,AEGP_PluginID,bool needed,LightStaging&) noexcept{lights_needed=needed;return 0;}
void apply_lights(SfCoreRenderRequest&,const LightStaging&) noexcept{}
PF_Err capture_camera(PF_InData*,SfCoreRenderRequest&,const PF_InData*) noexcept{return 0;}
bool gpu_device_matches(const void*,PF_GPU_Framework,A_u_long) noexcept{return false;}
void record_cpu_execution() noexcept{}
void record_render_geometry(A_long,A_long,A_long,A_long,A_long,A_long,A_long,A_long) noexcept{}
PF_Err render_gpu_scene(PF_InData*,PF_OutData*,const void*,PF_GPU_Framework,A_u_long,const SfCoreGpuSceneResult&,PF_EffectWorld*,std::int32_t,std::int32_t,bool,unsigned,float) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err render_motion_cpu(PF_InData*,PF_OutData*,const SfCoreApi&,SfCoreRenderRequest,const MotionExposure&,const WorldLayout&,PF_EffectWorld*,HostBitDepth,const core::Cancellation&) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err stage_texture_resources(PF_InData*,PF_OutData*,PF_SmartRenderExtra*,const TexturePreparation&,HostBitDepth,const core::Cancellation&,TextureStaging&) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err prepare_motion_gpu(PF_InData*,PF_OutData*,const SfCoreApi&,SfCoreRenderRequest,const MotionExposure&,SfCoreGpuSceneResult&,MotionGpuStorage&,const core::Cancellation&) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err capture_motion_light_sources(PF_InData*,AEGP_PluginID,const core::Cancellation&,std::vector<MotionLightSource>&,MotionLightSourceInventoryStats*) noexcept{++light_requests;return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err motion_light_prefix_to_utf16(std::string_view,std::u16string&) noexcept{return 0;}
PF_Err select_motion_light_source_ids(std::span<const MotionLightSource>,std::u16string_view,std::vector<std::uint32_t>&) noexcept{return 0;}
PF_Err select_motion_point_light_source_ids(std::span<const MotionLightSource>,std::u16string_view,std::vector<std::uint32_t>&) noexcept{return 0;}
PF_Err capture_motion_layer_points(PF_InData*,core::LayerUnits,std::span<const MotionLayerPointRequest>,const core::Cancellation&,std::vector<MotionLayerPoint>&) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err capture_motion_light_path_tracks(PF_InData*,AEGP_PluginID,std::span<const std::uint32_t>,core::LayerUnits,double,const core::Cancellation&,MotionLightPathTrackSet&) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err encode_motion_light_path_track_set(const MotionLightPathTrackSet&,const core::Cancellation&,std::vector<std::byte>&) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
}
int main(){
    PF_InData in{};in.width=1920;in.height=1080;in.current_time=30;in.time_step=1;in.time_scale=30;in.pixel_aspect_ratio={1,1};
    in.inter.abort=[](PF_ProgPtr)->PF_Err{return 0;};
    PF_OutData out{};PF_PreRenderInput input{};input.output_request.rect={0,0,1920,1080};PF_PreRenderOutput output{};
    PF_PreRenderCallbacks callbacks{};callbacks.checkout_layer=[](PF_ProgPtr,PF_ParamIndex,A_long,const PF_RenderRequest*,A_long,A_long,A_u_long,PF_CheckoutResult* result)->PF_Err{
        *result={};result->ref_width=1920;result->ref_height=1080;result->max_result_rect={0,0,1920,1080};result->par={1,1};return 0;};
    callbacks.GuidMixInPtr=[](PF_ProgPtr,A_u_long,const void*)->PF_Err{return 0;};
    PF_PreRenderExtra extra{&input,&output,&callbacks};
    const auto run=[&](core::Graph scene){author=std::make_shared<const core::Graph>(std::move(scene));output={};light_requests=motion_nodes=history_nodes=0;lights_needed=false;
        const auto error=adapter::pre_render(&in,&out,&extra);if(output.pre_render_data && output.delete_pre_render_data_func)output.delete_pre_render_data_func(output.pre_render_data);return error;};
    check(run(base())==0 && motion_nodes==3 && history_nodes==3,"plain Output prepares without host resources");
    auto parked=base();parked.nodes.push_back({id(6),core::graph_keys::kMotionNode,1,{{core::graph_keys::kMotionMode,std::uint32_t{0}}}});
    const auto original=core::serialize_graph(parked,core::particle_node_registry());
    check(run(parked)==0 && light_requests==0 && motion_nodes==3 && history_nodes==3,"parked Light Path requests no lights or shutter/history work");
    const auto preserved=core::serialize_graph(*author,core::particle_node_registry());
    check(original.has_value() && preserved.has_value() && original.value()==preserved.value(),"preparation leaves author graph bytes unchanged");
    set(parked.nodes.back(),core::graph_keys::kMotionMode,std::uint32_t{2});
    check(run(parked)==0 && light_requests==0,"parked Look At requests no lights");
    parked.edges.erase(parked.edges.begin()+1);parked.edges.push_back({{id(7).value},id(2),core::graph_keys::kParticleParticlesOut,id(6),core::graph_keys::kMotionParticlesIn});
    parked.edges.push_back({{id(8).value},id(6),core::graph_keys::kMotionParticlesOut,id(3),core::graph_keys::kOutputParticles});
    check(run(parked)==PF_Err_BAD_CALLBACK_PARAM && light_requests==1,"active missing light resource remains a strict failure");
    auto many=base();
    for(unsigned n=10;n<138;++n)many.nodes.push_back({id(n),core::graph_keys::kMotionNode,1,{{core::graph_keys::kMotionMode,std::uint32_t{0}}}});
    check(run(many)==0 && light_requests==0 && motion_nodes==3 && history_nodes==3,"128 parked processors add no light, shutter or history consumers");
    auto work=adapter::graph_output_working_copy(many);
    const auto plain_bytes=core::serialize_graph(base(),core::particle_node_registry()),work_bytes=core::serialize_graph(work,core::particle_node_registry());
    check(plain_bytes.has_value() && work_bytes.has_value() && plain_bytes.value()==work_bytes.value(),"Output working graph preserves plain serialized numerical input");
    many.nodes.back().parameters[0].value=1.0;
    check(run(many)==PF_Err_BAD_CALLBACK_PARAM && light_requests==0,"malformed parked parameter types remain rejected before scope filtering");
    auto closure=base();closure.nodes.push_back({id(9),core::graph_keys::kMaterialNode,1,{}});
    closure.edges.push_back({{id(10).value},id(9),core::PortKey{1},id(2),core::graph_keys::kParticleModelsIn});
    // Closure follows resource edges and reciprocal parent links; it does not
    // classify by particle port or recurse through feedback indefinitely.
    closure.edges.push_back({{id(11).value},id(2),core::PortKey{1},id(9),core::PortKey{1}});
    const auto active=adapter::graph_output_ancestors(closure);
    check(active.contains(id(9)) && active.size()==4,"all upstream resource/parent edges are retained with bounded cyclic traversal");
    auto material=base();material.nodes.push_back({id(9),core::graph_keys::kMaterialNode,1,{
        {core::graph_keys::kMaterialColorSource,std::uint32_t{0}},{core::graph_keys::kMaterialColor,core::Vec3{1,1,1}},
        {core::graph_keys::kMaterialRoughness,50.0},{core::graph_keys::kMaterialMetallic,0.0},
        {core::graph_keys::kMaterialOpacity,100.0},{core::graph_keys::kMaterialLights,std::uint32_t{1}}}});
    check(run(material)==0 && !lights_needed && motion_nodes==3,"parked material does not request material-light inventory");
    material.edges.push_back({{id(10).value},id(9),core::graph_keys::kMaterialOut,id(2),core::graph_keys::kParticleModelsIn});
    check(run(material)==0 && lights_needed && motion_nodes==4,"active material resource edge retains light preparation");
    arm_state_failure=true;
    check(run(base())==PF_Err_OUT_OF_MEMORY && !fail_next_allocation && !output.pre_render_data,"frame-state allocation failure returns OOM without partial pre-render ownership");
    std::printf("Render scope: %u checks, %u failures; no AE/device operations\n",checks,failures);return failures?1:0;
}
