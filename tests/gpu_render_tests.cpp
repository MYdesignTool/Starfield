// Standalone driver qualification. Private test contexts simulate AE's borrowed
// device data; the product GpuRender.cpp has no context/allocation creation API.
#include "GpuRender.hpp"
#include "NodeEffects.hpp"
#include "SmartRender.hpp"
#include "CoreLoader.hpp"
#include "EmitterHistory.hpp"
#include "Parameters.hpp"
#include "Camera.hpp"
#include "Diagnostics.hpp"
#include "AE_EffectCBSuites.h"
#include "AE_EffectGPUSuites.h"
#include "SPBasic.h"
#include "starfield/core/SpriteScene.hpp"
#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/SequenceCodec.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <chrono>

PF_Err register_node_graph_sync(PF_InData*) noexcept {return PF_Err_NONE;}
PF_Err sync_node_graph_parameter(PF_InData*,PF_OutData*,PF_ParamDef*[],const PF_UserChangedParamExtra*) noexcept {return PF_Err_NONE;}
using namespace starfield;
static std::shared_ptr<const core::Graph> smart_graph;
namespace starfield::adapter {
CoreGeneration::CoreGeneration(void* m,SfCoreApi api,std::uint64_t id,std::wstring p) noexcept:module_(m),api_(api),cache_identity_(id),path_(std::move(p)) {}
CoreGeneration::~CoreGeneration() noexcept = default;
CoreLoadResult acquire_core() noexcept {
    SfCoreApi api{};StarfieldCore_GetApi(3,sizeof(api),&api);
    return {std::make_shared<CoreGeneration>(nullptr,api,42,L"standalone test Core"),{},false};
}
PF_Err checkout_render_graph(PF_InData*,PF_OutData*,std::shared_ptr<const core::Graph>& g,A_long*,A_long,A_long) noexcept {g=smart_graph;return 0;}
PF_Err capture_emitter_origin_history(PF_InData*,PF_OutData*,core::Graph&,A_long,A_long,const core::Cancellation&) noexcept {return 0;}
PF_Err capture_camera(PF_InData*,SfCoreRenderRequest& r) noexcept {r.camera_enabled=0;return 0;}
void record_render_geometry(A_long,A_long,A_long,A_long,A_long,A_long,A_long,A_long) noexcept {}
}
namespace {
int checks{},failures{};
void check(bool ok,const char* text) {++checks;if(!ok){++failures;std::printf("FAIL: %s\n",text);}}
PF_GPUDeviceInfo info{};
HMODULE driver{};
void* context{};void* queue{};
void* world_buffer{};std::size_t world_bytes{};
std::map<void*,std::size_t> allocated;
std::map<PF_EffectWorld*,std::pair<void*,std::size_t>> world_views;
int pinned_count{},suite_count{},abort_count{},fail_allocation{};
bool interrupt{};
int (__stdcall *cuMemAlloc)(std::uint64_t*,std::size_t){};
int (__stdcall *cuMemFree)(std::uint64_t){};
int (__stdcall *cuAllocHost)(void**,std::size_t){};
int (__stdcall *cuFreeHost)(void*){};
int (__stdcall *cuCopyTo)(std::uint64_t,const void*,std::size_t){};
int (__stdcall *cuCopyFrom)(void*,std::uint64_t,std::size_t){};
void* (__stdcall *clCreateBuffer)(void*,std::uint64_t,std::size_t,void*,int*){};
int (__stdcall *clReleaseMem)(void*){};
int (__stdcall *clWrite)(void*,void*,unsigned,std::size_t,std::size_t,const void*,unsigned,const void*,void*){};
int (__stdcall *clRead)(void*,void*,unsigned,std::size_t,std::size_t,void*,unsigned,const void*,void*){};
template<class T> T proc(const char* name) {return reinterpret_cast<T>(GetProcAddress(driver,name));}
PF_Err device_info(PF_ProgPtr,A_u_long,PF_GPUDeviceInfo* result) {*result=info;return 0;}
PF_Err alloc_device(PF_ProgPtr,A_u_long,std::size_t bytes,void** result) {
    if(fail_allocation && --fail_allocation==0) return PF_Err_OUT_OF_MEMORY;
    if(info.device_framework==PF_GPU_Framework_CUDA) {
        std::uint64_t ptr{};if(cuMemAlloc(&ptr,bytes))return PF_Err_OUT_OF_MEMORY;*result=reinterpret_cast<void*>(ptr);
    } else {int error{};*result=clCreateBuffer(context,1,bytes,nullptr,&error);if(error)return PF_Err_OUT_OF_MEMORY;}
    allocated[*result]=bytes;return 0;
}
PF_Err free_device(PF_ProgPtr,A_u_long,void* value) {
    check(allocated.erase(value)==1,"free only suite-owned device allocation");
    return info.device_framework==PF_GPU_Framework_CUDA ? static_cast<PF_Err>(cuMemFree(reinterpret_cast<std::uint64_t>(value))) : static_cast<PF_Err>(clReleaseMem(value));
}
PF_Err alloc_host(PF_ProgPtr,A_u_long,std::size_t bytes,void** result) {
    if(info.device_framework==PF_GPU_Framework_CUDA) {if(cuAllocHost(result,bytes))return PF_Err_OUT_OF_MEMORY;}
    else *result=std::malloc(bytes);
    if(!*result)return PF_Err_OUT_OF_MEMORY;++pinned_count;return 0;
}
PF_Err free_host(PF_ProgPtr,A_u_long,void* value) {
    --pinned_count;if(info.device_framework==PF_GPU_Framework_CUDA)return static_cast<PF_Err>(cuFreeHost(value));
    std::free(value);return 0;
}
PF_Err get_data(PF_ProgPtr,PF_EffectWorld* world,void** result) {*result=world_views.contains(world)?world_views[world].first:world_buffer;return 0;}
PF_Err get_size(PF_ProgPtr,PF_EffectWorld* world,std::size_t* result) {*result=world_views.contains(world)?world_views[world].second:world_bytes;return 0;}
PF_Err get_index(PF_ProgPtr,PF_EffectWorld*,A_u_long* result) {*result=7;return 0;}
PF_Err get_format(const PF_EffectWorld*,PF_PixelFormat* result) {*result=PF_PixelFormat_GPU_BGRA128;return 0;}
PF_GPUDeviceSuite1 gpu{};PF_WorldSuite2 worlds{};
SPErr acquire(const char* name,int version,const void** result) {
    if(std::strcmp(name,kPFGPUDeviceSuite)==0 && version==1)*result=&gpu;
    else if(std::strcmp(name,kPFWorldSuite)==0 && version==2)*result=&worlds;
    else return PF_Err_BAD_CALLBACK_PARAM;
    ++suite_count;return 0;
}
SPErr release(const char*,int) {--suite_count;return 0;}
PF_Err abort_render(PF_ProgPtr) {++abort_count;return interrupt?PF_Interrupt_CANCEL:PF_Err_NONE;}
bool create_device(PF_GPU_Framework framework) {
    info={};info.device_framework=framework;info.compatibleB=TRUE;
    driver=LoadLibraryExW(framework==PF_GPU_Framework_CUDA?L"nvcuda.dll":L"OpenCL.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!driver)return false;
    if(framework==PF_GPU_Framework_CUDA) {
        const auto init=proc<int (__stdcall *)(unsigned)>("cuInit");
        const auto get=proc<int (__stdcall *)(int*,int)>("cuDeviceGet");
        const auto create=proc<int (__stdcall *)(void**,unsigned,int)>("cuCtxCreate_v2");
        if(!init || init(0)) return false;
        int id{};if(get(&id,0)||create(&context,0,id))return false;
        cuMemAlloc=proc<decltype(cuMemAlloc)>("cuMemAlloc_v2");cuMemFree=proc<decltype(cuMemFree)>("cuMemFree_v2");
        cuAllocHost=proc<decltype(cuAllocHost)>("cuMemAllocHost_v2");cuFreeHost=proc<decltype(cuFreeHost)>("cuMemFreeHost");
        cuCopyTo=proc<decltype(cuCopyTo)>("cuMemcpyHtoD_v2");cuCopyFrom=proc<decltype(cuCopyFrom)>("cuMemcpyDtoH_v2");
        const auto create_stream=proc<int (__stdcall *)(void**,unsigned)>("cuStreamCreate");
        if(create_stream(&queue,0))return false;info.devicePV=reinterpret_cast<void*>(std::intptr_t(id));
    } else {
        const auto platforms=proc<int (__stdcall *)(unsigned,void**,unsigned*)>("clGetPlatformIDs");
        const auto devices=proc<int (__stdcall *)(void*,std::uint64_t,unsigned,void**,unsigned*)>("clGetDeviceIDs");
        const auto create=proc<void* (__stdcall *)(const std::intptr_t*,unsigned,void* const*,void*,void*,int*)>("clCreateContext");
        const auto create_queue=proc<void* (__stdcall *)(void*,void*,std::uint64_t,int*)>("clCreateCommandQueue");
        void* platform{};if(!platforms || platforms(1,&platform,nullptr))return false;
        void* id{};if(devices(platform,4,1,&id,nullptr))return false;
        int error{};context=create(nullptr,1,&id,nullptr,nullptr,&error);if(error)return false;
        queue=create_queue(context,id,1,&error);if(error)return false;info.devicePV=id;
        clCreateBuffer=proc<decltype(clCreateBuffer)>("clCreateBuffer");clReleaseMem=proc<decltype(clReleaseMem)>("clReleaseMemObject");
        clWrite=proc<decltype(clWrite)>("clEnqueueWriteBuffer");clRead=proc<decltype(clRead)>("clEnqueueReadBuffer");
    }
    info.contextPV=context;info.command_queuePV=queue;return true;
}
void destroy_device() {
    if(info.device_framework==PF_GPU_Framework_CUDA) {
        proc<int (__stdcall *)(void*)>("cuStreamDestroy_v2")(queue);
        proc<int (__stdcall *)(void*)>("cuCtxDestroy_v2")(context);
    } else {
        proc<int (__stdcall *)(void*)>("clReleaseCommandQueue")(queue);
        proc<int (__stdcall *)(void*)>("clReleaseContext")(context);
    }
    FreeLibrary(driver);driver=nullptr;
}
void write(const std::vector<std::byte>& bytes) {
    check(info.device_framework==PF_GPU_Framework_CUDA ? cuCopyTo(reinterpret_cast<std::uint64_t>(world_buffer),bytes.data(),bytes.size())==0 :
        clWrite(queue,world_buffer,1,0,bytes.size(),bytes.data(),0,nullptr,nullptr)==0,"test output buffer initialized");
}
std::vector<std::byte> read() {
    std::vector<std::byte> bytes(world_bytes);
    check(info.device_framework==PF_GPU_Framework_CUDA ? cuCopyFrom(bytes.data(),reinterpret_cast<std::uint64_t>(world_buffer),bytes.size())==0 :
        clRead(queue,world_buffer,1,0,bytes.size(),bytes.data(),0,nullptr,nullptr)==0,"test output readback only in standalone comparison");return bytes;
}
core::Uuid128 uuid(unsigned n) {core::Uuid128 v{};v.bytes[15]=std::uint8_t(n);return v;}
void set(core::GraphNode& n,core::ParameterKey key,core::ParameterValue value) {
    for(auto& p:n.parameters)if(p.key==key){p.value=std::move(value);return;}n.parameters.push_back({key,std::move(value)});
}
core::RenderRequest request(unsigned shape,bool camera) {
    core::Settings settings;settings.birth_rate=31;settings.particle_size=13;settings.opacity=.4;
    settings.velocity={.3,.1,.02};settings.particle_count=10000;
    settings.appearance_enabled=true;settings.color_start={1.3,.2,.7};settings.color_end={.1,1.1,.3};
    auto graph=core::make_emitter_particle_output_graph(settings,core::NodeId{uuid(1)},core::NodeId{uuid(2)},core::NodeId{uuid(255)},core::EdgeId{uuid(3)},core::EdgeId{uuid(4)}).take_value();
    set(graph.nodes[1],core::graph_keys::kParticleShape,shape);
    set(graph.nodes[1],core::graph_keys::kSizeY,8.);
    set(graph.nodes[1],core::graph_keys::kParticleAngles,core::Vec3{10,5,31});
    set(graph.nodes[1],core::graph_keys::kUpAxis,std::uint32_t{2});
    set(graph.nodes[1],core::graph_keys::kParticleFeather,25.);
    core::RenderRequest r;r.settings=core::validate_settings(settings);
    r.graph=std::make_shared<const core::Graph>(std::move(graph));
    r.frame={64,48,64,48,{8,6,48,37},{1,1},{1,30},core::PixelFormat::rgba32f,core::ColorSpace::ae_working_space,core::AlphaMode::straight,1.,core::Quality::full};
    if(camera) {
        r.camera.enabled=true;r.camera.layer_to_view={1,0,0,0,0,1,0,0,0,0,1,0,-32,-24,100,1};
        r.camera.image_to_layer={1,0,0,0,1,0,0,0,1};r.camera.focal_x=r.camera.focal_y=100;
        r.camera.center_x=32;r.camera.center_y=24;
    }
    return r;
}
SfCoreGpuSceneResult view(const core::SpriteScene& s) {
    SfCoreGpuSceneResult v{};v.struct_size=sizeof(v);v.status=SF_CORE_OK;v.tile_size=16;
    v.region={s.region.left,s.region.top,s.region.right,s.region.bottom};v.tiles_x=s.tiles_x;v.tiles_y=s.tiles_y;
    v.sprite_count=static_cast<unsigned>(s.sprites.size());v.index_count=static_cast<unsigned>(s.indices.size());
    v.sprites=s.sprites.data();v.tile_offsets=s.offsets.data();v.tile_indices=s.indices.data();return v;
}
void test_scene_api() {
    SfCoreApi api{};check(!StarfieldCore_GetApi(2,sizeof(api),&api),"ABI 2 rejected after typed scene API addition");
    check(StarfieldCore_GetApi(3,sizeof(api),&api)==1 && api.prepare_gpu_scene && api.release_gpu_scene,"ABI 3 requires full CPU/scene API");
    auto r=request(2,true);const auto bytes=core::serialize_graph(*r.graph,core::particle_node_registry()).value();
    SfCoreRenderRequest input{};input.struct_size=sizeof(input);input.graph_bytes=bytes.data();input.graph_byte_count=bytes.size();
    input.frame={64,48,64,48,{8,6,48,37},1,1,1,30,2,0,0,1,1};
    SfCoreGpuSceneResult result{};result.struct_size=sizeof(result);
    check(api.prepare_gpu_scene(&input,&result)==SF_CORE_OK && result.opaque_handle && result.sprites && result.tile_offsets,"typed immutable scene returned across C ABI");
    check(result.sprite_count>0 && result.tile_size==16 && result.tile_offsets[result.tiles_x*result.tiles_y]==result.index_count,"scene ABI dimensions/index terminator");
    api.release_gpu_scene(&result);check(!result.opaque_handle && !result.sprites,"scene released by paired Core generation");
    api.release_gpu_scene(&result);check(!result.opaque_handle,"released result is idempotent");
    input.struct_size-=1;check(api.prepare_gpu_scene(&input,&result)==SF_CORE_INVALID_REQUEST,"scene ABI bad request size rejected");
    input.struct_size=sizeof(input);input.camera_enabled=2;
    check(api.prepare_gpu_scene(&input,&result)==SF_CORE_INVALID_REQUEST,"scene rejects invalid camera flag");input.camera_enabled=0;
    input.is_cancelled=[](void*)->std::int32_t{return 1;};
    check(api.prepare_gpu_scene(&input,&result)==SF_CORE_CANCELLED,"Core scene cancellation before allocation");input.is_cancelled=nullptr;
    input.frame.roi={8,6,8,6};
    check(api.prepare_gpu_scene(&input,&result)==SF_CORE_OK && result.sprite_count==0 && result.tiles_x==0 && result.tiles_y==0,"empty scene ROI returns no sprite arrays");api.release_gpu_scene(&result);
    auto dense=request(0,false);dense.frame={8192,8192,8192,8192,{0,0,8192,8192},{1,1},{1,30},core::PixelFormat::rgba32f,core::ColorSpace::ae_working_space,core::AlphaMode::straight,1,core::Quality::full};
    auto graph=*dense.graph;set(graph.nodes[0],core::graph_keys::kBirthRate,10.);set(graph.nodes[1],core::graph_keys::kSizeStart,100000.);
    set(graph.nodes[1],core::graph_keys::kSizeEnd,100.);dense.graph=std::make_shared<const core::Graph>(std::move(graph));
    const core::NeverCancelled never;const auto bounded=core::prepare_sprite_scene(dense,never);
    check(!bounded.has_value() && bounded.error().code==core::ErrorCode::unsupported_format,"tile work overflow requests pre-render CPU fallback without truncation");
}
void benchmark(PF_InData& in,PF_OutData& out,void* gpu_data) {
    core::Settings settings;settings.birth_rate=20000;settings.particle_size=16;settings.opacity=.4;
    settings.emitter_shape=core::EmitterShape::box;settings.emitter_size_pixels={400,400,0};settings.velocity={};settings.velocity_spread=0;
    auto graph=core::make_emitter_particle_output_graph(settings,core::NodeId{uuid(1)},core::NodeId{uuid(2)},core::NodeId{uuid(255)},core::EdgeId{uuid(3)},core::EdgeId{uuid(4)}).take_value();
    core::RenderRequest r;r.graph=std::make_shared<const core::Graph>(std::move(graph));
    r.frame={512,512,512,512,{0,0,512,512},{1,1},{1,30},core::PixelFormat::rgba32f,core::ColorSpace::ae_working_space,core::AlphaMode::straight,1,core::Quality::full};
    const core::NeverCancelled never;const core::CpuParticleRenderer cpu;
    using Clock=std::chrono::steady_clock;
    const auto a=Clock::now();auto scene=core::prepare_sprite_scene(r,never);const auto b=Clock::now();
    check(scene.has_value(),"benchmark scene fits GPU limits");if(!scene.has_value())return;
    const auto v=view(scene.value());
    auto* old_buffer=world_buffer;const auto old_bytes=world_bytes;
    PF_EffectWorld world{};world.width=world.height=512;world.rowbytes=512*16;
    world_bytes=std::size_t(world.rowbytes)*world.height;
    check(alloc_device(nullptr,7,world_bytes,&world_buffer)==0,"benchmark borrowed world allocation");
    check(adapter::render_gpu_scene(&in,&out,gpu_data,info.device_framework,7,v,&world,0,0,true)==0,"GPU benchmark warmup");
    double gpu_ms=0,cpu_ms=0;
    for(int iteration=0;iteration<3;++iteration) {
        auto t=Clock::now();const auto error=adapter::render_gpu_scene(&in,&out,gpu_data,info.device_framework,7,v,&world,0,0,true);
        gpu_ms+=std::chrono::duration<double,std::milli>(Clock::now()-t).count();check(error==0,"timed GPU dispatch");
        t=Clock::now();auto output=cpu.render(r,never);cpu_ms+=std::chrono::duration<double,std::milli>(Clock::now()-t).count();check(output.has_value(),"timed CPU render");
    }
    const double preparation_ms=std::chrono::duration<double,std::milli>(b-a).count();gpu_ms/=3;cpu_ms/=3;
    const auto* name=info.device_framework==PF_GPU_Framework_CUDA?"CUDA":"OpenCL";
    std::printf("Standalone 512x512, %u sprites: prepare %.3f ms; upload/allocate/kernel/sync %.3f ms; combined %.3f ms; CPU render %.3f ms\n",v.sprite_count,preparation_ms,gpu_ms,preparation_ms+gpu_ms,cpu_ms);
    auto* report=std::fopen("artifacts/gpu-tests/timings.csv","a");
    if(report){std::fprintf(report,"%s,%u,%.6f,%.6f,%.6f,%.6f\n",name,v.sprite_count,preparation_ms,gpu_ms,preparation_ms+gpu_ms,cpu_ms);std::fclose(report);}
    (void)free_device(nullptr,7,world_buffer);world_buffer=old_buffer;world_bytes=old_bytes;
}
PF_EffectWorld* smart_world{};PF_EffectWorld* node_input_world{}; int input_checkouts{},input_checkins{};
PF_Err checkout_metadata(PF_ProgPtr,PF_ParamIndex index,A_long id,const PF_RenderRequest* req,A_long,A_long,A_u_long,PF_CheckoutResult* result) {
    check(index==0 && (id==0 || (id==1 && req->rect.left==req->rect.right)),"SmartFX main geometry or node input request");
    result->result_rect=req->rect;result->max_result_rect={0,0,64,48};result->ref_width=64;result->ref_height=48;result->par={1,1};return 0;
}
PF_Err mix_guid(PF_ProgPtr,A_u_long size,const void*) {check(size==8,"immutable Core generation mixed into cache key");return 0;}
PF_Err checkout_input(PF_ProgPtr,A_long,PF_EffectWorld** value) {++input_checkouts;*value=node_input_world;return 0;}
PF_Err checkin_input(PF_ProgPtr,A_long) {++input_checkins;return 0;}
PF_Err checkout_output(PF_ProgPtr,PF_EffectWorld** value) {check(input_checkouts>input_checkins,"input checkout precedes output");*value=smart_world;return 0;}
void test_smartfx(PF_InData& in,PF_OutData& out,void* gpu_data,PF_EffectWorld& world) {
    auto r=request(1,false);
    smart_graph=r.graph;in.width=64;in.height=48;in.current_time=1;in.time_step=1;in.time_scale=1;in.pixel_aspect_ratio={1,1};
    PF_PreRenderInput pre_input{};pre_input.what_gpu=info.device_framework;pre_input.device_index=7;pre_input.gpu_data=gpu_data;
    pre_input.bitdepth=32;pre_input.output_request.rect={4,2,60,46};
    PF_PreRenderCallbacks callbacks{};callbacks.checkout_layer=checkout_metadata;callbacks.GuidMixInPtr=mix_guid;
    PF_PreRenderOutput pre_output{};PF_PreRenderExtra pre{&pre_input,&pre_output,&callbacks};
    check(adapter::pre_render(&in,&out,&pre)==0 && (pre_output.flags&PF_RenderOutputFlag_GPU_RENDER_POSSIBLE),"SmartFX pre-render prepares and qualifies actual GPU scene");
    const auto timing=adapter::last_pre_render_timings();
    check(timing.valid && timing.complete && timing.seconds==1 && timing.total_ms>=0 &&
        timing.controls_ms>=0 && timing.history_ms>=0 && timing.scene_ms>=0 &&
        timing.total_ms>=timing.controls_ms+timing.history_ms+timing.scene_ms,
        "actual pre-render timing separates controls, history and scene work");
    PF_SmartRenderInput render_input{};render_input.bitdepth=32;render_input.what_gpu=info.device_framework;
    render_input.device_index=7;render_input.gpu_data=gpu_data;render_input.pre_render_data=pre_output.pre_render_data;
    PF_SmartRenderCallbacks render_callbacks{checkout_input,checkin_input,checkout_output};PF_SmartRenderExtra render{&render_input,&render_callbacks};
    smart_world=&world;write(std::vector<std::byte>(world_bytes,std::byte{0x7e}));
    check(adapter::smart_render(&in,&out,&render)==0,"native SmartFX GPU transport dispatches into borrowed AE world");
    const auto execution=adapter::last_smart_render_timing();
    check(execution.valid && execution.complete && execution.paired && execution.seconds==1 && execution.prepared_seconds==1 && execution.total_ms>=0,
        "actual SmartFX execution records upload, kernel, sync and cleanup wall time");
    check(adapter::last_gpu_execution().rendered && adapter::last_gpu_execution().framework==info.device_framework,"GPU trace records executed framework");
    check(input_checkouts==input_checkins,"SmartFX input checked in after GPU render");
    const auto actual=read();const core::NeverCancelled never;const core::CpuParticleRenderer cpu;
    r.frame.region_of_interest={4,2,60,46};r.frame.frame_duration={1,1};
    const auto expected=cpu.render(r,never);bool equal=expected.has_value();
    if(equal)for(int y=0;y<world.height;++y)for(int x=0;x<world.width;++x) {
        float a[4]{},b[4]{};std::memcpy(a,actual.data()+std::size_t(y)*world.rowbytes+x*16,16);
        std::memcpy(b,expected.value().pixels.data()+std::size_t(y)*expected.value().row_bytes+x*16,16);
        equal &= std::abs(a[0]-b[2])<.0002 && std::abs(a[1]-b[1])<.0002 && std::abs(a[2]-b[0])<.0002 && std::abs(a[3]-b[3])<.0002;
    }
    check(equal,"pre-render scene and GPU transport agree with CPU projection/time/ROI");
    const auto checkouts_before=input_checkouts;
    in.current_time=2;
    check(adapter::smart_render(&in,&out,&render)==PF_Err_BAD_CALLBACK_PARAM && input_checkouts==checkouts_before && read()==actual,
        "another frame cannot reuse a time-dependent GPU scene or touch borrowed pixels");
    check(!adapter::last_smart_render_timing().paired,"diagnostics identify mismatched render/preparation times");
    in.current_time=2;in.time_step=2;in.time_scale=2;
    check(adapter::smart_render(&in,&out,&render)==0 && read()==actual,
        "equivalent rational time and duration preserve the matching scene");
    in.current_time=1;in.time_step=1;in.time_scale=1;
    pre_output.delete_pre_render_data_func(pre_output.pre_render_data);
    auto graph=*smart_graph;set(graph.nodes.back(),core::graph_keys::kAcceleration,std::uint32_t{1});smart_graph=std::make_shared<const core::Graph>(std::move(graph));
    pre_output={};check(adapter::pre_render(&in,&out,&pre)==0 && !(pre_output.flags&PF_RenderOutputFlag_GPU_RENDER_POSSIBLE),"CPU preference clears per-frame GPU eligibility");
    pre_output.delete_pre_render_data_func(pre_output.pre_render_data);
    smart_graph=r.graph;pre_input.what_gpu=PF_GPU_Framework_METAL;pre_output={};
    check(adapter::pre_render(&in,&out,&pre)==0 && !(pre_output.flags&PF_RenderOutputFlag_GPU_RENDER_POSSIBLE),"pre-render rejects unmatched framework without changing scene");
    pre_output.delete_pre_render_data_func(pre_output.pre_render_data);
    check(allocated.size()==1 && pinned_count==0 && suite_count==0,"SmartFX state and GPU allocations released");
}
void test_copy(PF_InData& in,void* device,PF_EffectWorld& output) {
    PF_EffectWorld input{};input.width=63;input.height=51;input.rowbytes=63*16+32;input.origin_x=-3;input.origin_y=-6;
    const auto input_bytes=std::size_t(input.rowbytes)*input.height;void* input_buffer{};
    check(alloc_device(nullptr,7,input_bytes,&input_buffer)==0,"node GPU input borrowed allocation");
    std::vector<std::byte> source(input_bytes,std::byte{0x42});
    for(int y=0;y<input.height;++y)for(int x=0;x<input.width;++x) {
        const float pixel[4]{float(x)/7-1,float(y)/19,float(x+y)/11,.37f};
        std::memcpy(source.data()+std::size_t(y)*input.rowbytes+x*16,pixel,16);
    }
    auto* old_buffer=world_buffer;const auto old_size=world_bytes;
    world_buffer=input_buffer;world_bytes=input_bytes;write(source);world_buffer=old_buffer;world_bytes=old_size;
    world_views[&input]={input_buffer,input_bytes};
    write(std::vector<std::byte>(world_bytes,std::byte{0x7e}));
    check(adapter::copy_gpu_pixels(&in,device,info.device_framework,7,&input,&output)==0,"node GPU pass-through dispatches without CPU pixel pointers");
    const auto bytes=read();bool pixels=true,padding=true;
    for(int y=0;y<output.height;++y)for(int x=0;x<output.width;++x) {
        const int ix=x+output.origin_x-input.origin_x,iy=y+output.origin_y-input.origin_y;
        std::byte zero[16]{};const auto* expected=ix>=0 && iy>=0 && ix<input.width && iy<input.height?
            source.data()+std::size_t(iy)*input.rowbytes+ix*16:zero;
        pixels &= std::memcmp(bytes.data()+std::size_t(y)*output.rowbytes+x*16,expected,16)==0;
    }
    for(int y=0;y<output.height;++y)for(int b=output.width*16;b<output.rowbytes;++b)padding&=bytes[std::size_t(y)*output.rowbytes+b]==std::byte{0x7e};
    check(pixels,"node pass-through exact HDR/alpha pixel bytes and translated ROI");check(padding,"node GPU copy leaves padded rows intact");
    PF_OutData node_out{};PF_GPUDeviceSetupInput node_setup_input{info.device_framework,7};PF_GPUDeviceSetupOutput node_setup_output{};
    PF_GPUDeviceSetupExtra node_setup{&node_setup_input,&node_setup_output};
    check(EffectMain(PF_Cmd_GPU_DEVICE_SETUP,&in,&node_out,nullptr,nullptr,&node_setup)==0 && node_setup_output.gpu_data,"actual node effect GPU setup selector accepts device");
    PF_PreRenderInput node_pre_input{};node_pre_input.what_gpu=info.device_framework;node_pre_input.device_index=7;
    node_pre_input.gpu_data=node_setup_output.gpu_data;node_pre_input.output_request.rect={4,2,60,46};node_pre_input.bitdepth=32;
    PF_PreRenderOutput node_pre_output{};PF_PreRenderCallbacks node_pre_callbacks{checkout_metadata,mix_guid};
    PF_PreRenderExtra node_pre{&node_pre_input,&node_pre_output,&node_pre_callbacks};
    check(EffectMain(PF_Cmd_SMART_PRE_RENDER,&in,&node_out,nullptr,nullptr,&node_pre)==0 &&
        (node_pre_output.flags&PF_RenderOutputFlag_GPU_RENDER_POSSIBLE),"actual node effect pre-render selector qualifies GPU pass-through");
    PF_SmartRenderInput node_render_input{};node_render_input.bitdepth=32;node_render_input.what_gpu=info.device_framework;
    node_render_input.device_index=7;node_render_input.gpu_data=node_setup_output.gpu_data;
    PF_SmartRenderCallbacks node_render_callbacks{checkout_input,checkin_input,checkout_output};
    PF_SmartRenderExtra node_render{&node_render_input,&node_render_callbacks};node_input_world=&input;smart_world=&output;
    check(EffectMain(PF_Cmd_SMART_RENDER_GPU,&in,&node_out,nullptr,nullptr,&node_render)==0,"actual node SMART_RENDER_GPU selector forwards borrowed buffers");
    check(read()==bytes && input_checkouts==input_checkins,"node GPU selector preserves pixels and checks input back in");node_input_world=nullptr;
    PF_GPUDeviceSetdownInput node_setdown_input{node_setup_output.gpu_data,info.device_framework,7};PF_GPUDeviceSetdownExtra node_setdown{&node_setdown_input};
    check(EffectMain(PF_Cmd_GPU_DEVICE_SETDOWN,&in,&node_out,nullptr,nullptr,&node_setdown)==0 && !node_setdown_input.gpu_data,"actual node GPU setdown selector releases module");
    interrupt=true;check(adapter::copy_gpu_pixels(&in,device,info.device_framework,7,&input,&output)==PF_Interrupt_CANCEL,"node GPU copy respects cancellation");interrupt=false;
    check(allocated.size()==2 && pinned_count==0 && suite_count==0,"node copy allocates no staging/device buffers");
    world_views.erase(&input);check(free_device(nullptr,7,input_buffer)==0,"node input released only by test owner");
}
void test_backend(PF_GPU_Framework framework) {
    if(!create_device(framework)) {check(false,"local test GPU device available");return;}
    std::printf("Actual %s driver backend\n",framework==PF_GPU_Framework_CUDA?"CUDA":"OpenCL");
    gpu.GetDeviceInfo=device_info;gpu.AllocateDeviceMemory=alloc_device;gpu.FreeDeviceMemory=free_device;
    gpu.AllocateHostMemory=alloc_host;gpu.FreeHostMemory=free_host;gpu.GetGPUWorldData=get_data;
    gpu.GetGPUWorldSize=get_size;gpu.GetGPUWorldDeviceIndex=get_index;worlds.PF_GetPixelFormat=get_format;
    SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
    PF_InData in{};in.pica_basicP=&basic;in.inter.abort=abort_render;PF_OutData out{};
    PF_GPUDeviceSetupInput setup_input{};setup_input.what_gpu=framework;setup_input.device_index=7;
    PF_GPUDeviceSetupOutput setup_output{};PF_GPUDeviceSetupExtra setup{&setup_input,&setup_output};
    check(adapter::gpu_device_setup(&in,&out,&setup)==0 && setup_output.gpu_data &&
        (out.out_flags2&PF_OutFlag2_SUPPORTS_GPU_RENDER_F32),"real device setup accepts compiled backend");
    if(!setup_output.gpu_data){destroy_device();return;}
    check(adapter::gpu_device_matches(setup_output.gpu_data,framework,7),"device/framework identity matched");
    const auto setup_timing=adapter::last_gpu_setup_timing();
    check(setup_timing.calls>0 && setup_timing.last_ms>=0 && setup_timing.max_ms>=setup_timing.last_ms,
        "device setup initialization has its own timing outside SmartFX");
    check(!adapter::gpu_device_matches(setup_output.gpu_data,framework,6),"other device rejected");
    PF_EffectWorld world{};world.width=56;world.height=44;world.rowbytes=56*16+64;world.origin_x=4;world.origin_y=2;
    world_bytes=std::size_t(world.rowbytes)*world.height;check(alloc_device(nullptr,7,world_bytes,&world_buffer)==0,"test GPU output allocated");
    const core::NeverCancelled never;const core::CpuParticleRenderer cpu;
    float max_error=0;
    for(unsigned shape=0;shape<3;++shape) for(bool camera:{false,true}) for(auto alpha:{core::AlphaMode::straight,core::AlphaMode::premultiplied}) {
        auto r=request(shape,camera);r.frame.alpha_mode=alpha;
        const auto scene=core::prepare_sprite_scene(r,never);const auto reference=cpu.render(r,never);
        check(scene.has_value() && reference.has_value(),"scene and CPU reference prepared");
        if(!scene.has_value()||!reference.has_value())continue;
        auto v=view(scene.value());std::vector<std::byte> sentinel(world_bytes,std::byte{0x7e});write(sentinel);
        const auto error=adapter::render_gpu_scene(&in,&out,setup_output.gpu_data,framework,7,v,&world,4,2,alpha==core::AlphaMode::straight);
        if(error)std::printf("GPU error %d %s\n",error,out.return_msg);
        check(error==0,"real GPU dispatch succeeds with null CPU world.data");
        const auto bytes=read();bool pixels_ok=true,padding_ok=true;
        const auto& expected=reference.value();
        for(int y=0;y<world.height;++y)for(int x=0;x<world.width;++x) {
            float pixel[4]{};std::memcpy(pixel,bytes.data()+std::size_t(y)*world.rowbytes+x*16,16);
            float target[4]{};
            const int gx=x+4,gy=y+2;
            if(gx>=expected.region.left && gx<expected.region.right && gy>=expected.region.top && gy<expected.region.bottom)
                std::memcpy(target,expected.pixels.data()+std::size_t(gy-expected.region.top)*expected.row_bytes+(gx-expected.region.left)*16,16);
            const float ordered[4]{target[2],target[1],target[0],target[3]};
            for(int c=0;c<4;++c) {const float e=std::abs(pixel[c]-ordered[c]);max_error=std::max(max_error,e);pixels_ok&=std::isfinite(pixel[c]) && e<.0002f;}
        }
        for(int y=0;y<world.height;++y)for(int b=world.width*16;b<world.rowbytes;++b)padding_ok&=bytes[std::size_t(y)*world.rowbytes+b]==std::byte{0x7e};
        check(pixels_ok,"BGRA/alpha/shapes/camera/ROI parity within 0.0002");check(padding_ok,"GPU row padding untouched");
        check(allocated.size()==1 && pinned_count==0 && suite_count==0,"only borrowed output remains after dispatch");
        if(shape==0 && !camera && alpha==core::AlphaMode::straight) {
            fail_allocation=2;
            check(adapter::render_gpu_scene(&in,&out,setup_output.gpu_data,framework,7,v,&world,4,2,true)==PF_Err_OUT_OF_MEMORY,"second allocation failure returned");
            check(allocated.size()==1 && pinned_count==0 && suite_count==0,"partial upload cleaned on allocation failure");
            fail_allocation=0;interrupt=true;
            check(adapter::render_gpu_scene(&in,&out,setup_output.gpu_data,framework,7,v,&world,4,2,true)==PF_Interrupt_CANCEL,"GPU cancellation returns host interrupt");
            check(out.return_msg[0]==0 && allocated.size()==1 && pinned_count==0 && suite_count==0,"cancel clears dialog and releases buffers");interrupt=false;
            world_bytes-=1;
            check(adapter::render_gpu_scene(&in,&out,setup_output.gpu_data,framework,7,v,&world,4,2,true)==PF_Err_BAD_CALLBACK_PARAM,"undersized GPU world rejected before write");++world_bytes;
        }
    }
    test_smartfx(in,out,setup_output.gpu_data,world);
    benchmark(in,out,setup_output.gpu_data);
    test_copy(in,setup_output.gpu_data,world);
    std::printf("Maximum component difference vs CPU: %.9g\n",max_error);
    check(free_device(nullptr,7,world_buffer)==0,"test output released");world_buffer=nullptr;
    PF_GPUDeviceSetdownInput setdown_input{setup_output.gpu_data,framework,7};PF_GPUDeviceSetdownExtra setdown{&setdown_input};
    check(adapter::gpu_device_setdown(&in,&setdown)==0 && !setdown_input.gpu_data,"GPU module/kernel setdown");
    check(allocated.empty() && pinned_count==0 && suite_count==0,"no standalone GPU resources leaked");
    setup_input.what_gpu=PF_GPU_Framework_METAL;out.out_flags2=PF_OutFlag2_SUPPORTS_GPU_RENDER_F32;
    check(adapter::gpu_device_setup(&in,&out,&setup)==0 && !setup_output.gpu_data && !(out.out_flags2&PF_OutFlag2_SUPPORTS_GPU_RENDER_F32),"unimplemented framework negotiation rejects cleanly");
    destroy_device();
}
}
int main() {
    static_assert(sizeof(SfGpuSprite)==80);
    auto* report=std::fopen("artifacts/gpu-tests/timings.csv","w");if(report){std::fputs("framework,sprites,prepare_ms,dispatch_ms,total_ms,cpu_ms\n",report);std::fclose(report);}
    test_scene_api();
    test_backend(PF_GPU_Framework_CUDA);test_backend(PF_GPU_Framework_OPENCL);
    std::printf("%d GPU driver checks, %d failures\n",checks,failures);return failures?1:0;
}
