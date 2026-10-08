#include "GpuRender.hpp"
#include "starfield/core/Settings.hpp"
#include "AE_EffectCBSuites.h"
#include "AE_EffectGPUSuites.h"
#include "AE_Macros.h"
#include "GpuKernels.hpp" // build output, generated from our source by NVRTC
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>

namespace starfield::adapter {
namespace {
std::atomic<std::uint64_t> execution_trace{};
std::mutex setup_trace_mutex;
GpuSetupTiming setup_trace;
struct SetupTimer {
    std::chrono::steady_clock::time_point start{std::chrono::steady_clock::now()};
    ~SetupTimer() {
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        std::lock_guard lock(setup_trace_mutex);++setup_trace.calls;
        setup_trace.last_ms=ms;setup_trace.max_ms=std::max(setup_trace.max_ms,ms);
    }
};
// Only the documented dynamically loaded API subset is declared here. No vendor
// runtime, private device/context/queue creation, or allocation API is used.
using DevicePtr = std::uint64_t;
struct Runtime {
    HMODULE dll{};
    int (__stdcall *cuInit)(unsigned){};
    int (__stdcall *cuCtxPushCurrent)(void*){};
    int (__stdcall *cuCtxPopCurrent)(void**){};
    int (__stdcall *cuModuleLoadData)(void**,const void*){};
    int (__stdcall *cuModuleUnload)(void*){};
    int (__stdcall *cuModuleGetFunction)(void**,void*,const char*){};
    int (__stdcall *cuMemcpyHtoDAsync)(DevicePtr,const void*,std::size_t,void*){};
    int (__stdcall *cuStreamSynchronize)(void*){};
    int (__stdcall *cuLaunchKernel)(void*,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,void*,void**,void**){};
    void* (__stdcall *clCreateProgramWithSource)(void*,unsigned,const char**,const std::size_t*,int*){};
    int (__stdcall *clBuildProgram)(void*,unsigned,void* const*,const char*,void*,void*){};
    void* (__stdcall *clCreateKernel)(void*,const char*,int*){};
    int (__stdcall *clReleaseKernel)(void*){};
    int (__stdcall *clReleaseProgram)(void*){};
    int (__stdcall *clEnqueueWriteBuffer)(void*,void*,unsigned,std::size_t,std::size_t,const void*,unsigned,const void*,void*){};
    int (__stdcall *clSetKernelArg)(void*,unsigned,std::size_t,const void*){};
    int (__stdcall *clEnqueueNDRangeKernel)(void*,void*,unsigned,const std::size_t*,const std::size_t*,const std::size_t*,unsigned,const void*,void*){};
    int (__stdcall *clFinish)(void*){};
    ~Runtime() { if(dll) FreeLibrary(dll); }
    template<class T> bool symbol(T& slot,const char* name) {
        slot=reinterpret_cast<T>(GetProcAddress(dll,name)); return slot!=nullptr;
    }
    bool load(PF_GPU_Framework framework) {
        dll=LoadLibraryExW(framework==PF_GPU_Framework_CUDA?L"nvcuda.dll":L"OpenCL.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(!dll) return false;
        if(framework==PF_GPU_Framework_CUDA) return
            symbol(cuInit,"cuInit") && symbol(cuCtxPushCurrent,"cuCtxPushCurrent_v2") &&
            symbol(cuCtxPopCurrent,"cuCtxPopCurrent_v2") && symbol(cuModuleLoadData,"cuModuleLoadData") &&
            symbol(cuModuleUnload,"cuModuleUnload") && symbol(cuModuleGetFunction,"cuModuleGetFunction") &&
            symbol(cuMemcpyHtoDAsync,"cuMemcpyHtoDAsync_v2") && symbol(cuStreamSynchronize,"cuStreamSynchronize") &&
            symbol(cuLaunchKernel,"cuLaunchKernel") && cuInit(0)==0;
        return symbol(clCreateProgramWithSource,"clCreateProgramWithSource") && symbol(clBuildProgram,"clBuildProgram") &&
            symbol(clCreateKernel,"clCreateKernel") && symbol(clReleaseKernel,"clReleaseKernel") &&
            symbol(clReleaseProgram,"clReleaseProgram") && symbol(clEnqueueWriteBuffer,"clEnqueueWriteBuffer") &&
            symbol(clSetKernelArg,"clSetKernelArg") && symbol(clEnqueueNDRangeKernel,"clEnqueueNDRangeKernel") && symbol(clFinish,"clFinish");
    }
};
struct ContextScope {
    Runtime& runtime;
    bool cuda{}, valid{};
    ContextScope(Runtime& r,PF_GPU_Framework framework,void* context):runtime(r),cuda(framework==PF_GPU_Framework_CUDA) {
        valid=!cuda || (runtime.cuCtxPushCurrent && runtime.cuCtxPushCurrent(context)==0);
    }
    ~ContextScope() { if(cuda && valid) {void* old{}; (void)runtime.cuCtxPopCurrent(&old);} }
};
struct Device {
    static constexpr std::uint64_t signature=0x5346475055444556ull;
    std::uint64_t magic{signature};
    PF_GPUDeviceInfo info{};
    A_u_long index{};
    Runtime runtime;
    void* program{}; void* kernel{}; void* copy_kernel{};
    std::mutex mutex;
    ~Device() {
        if(!runtime.dll) return;
        ContextScope context(runtime,info.device_framework,info.contextPV);
        if(!context.valid) return;
        if(info.device_framework==PF_GPU_Framework_CUDA) {
            if(program) (void)runtime.cuModuleUnload(program);
        } else {
            if(copy_kernel) (void)runtime.clReleaseKernel(copy_kernel);
            if(kernel) (void)runtime.clReleaseKernel(kernel);
            if(program) (void)runtime.clReleaseProgram(program);
        }
    }
    bool initialize() {
        if(!runtime.load(info.device_framework)) return false;
        ContextScope context(runtime,info.device_framework,info.contextPV);
        if(!context.valid) return false;
        if(info.device_framework==PF_GPU_Framework_CUDA)
            return runtime.cuModuleLoadData(&program,kCudaPtx)==0 && runtime.cuModuleGetFunction(&kernel,program,"starfield_sprite_render")==0 &&
                runtime.cuModuleGetFunction(&copy_kernel,program,"starfield_copy_pixels")==0;
        const char* source=kOpenClSource; int error{};
        program=runtime.clCreateProgramWithSource(info.contextPV,1,&source,nullptr,&error);
        if(error || !program) return false;
        if(runtime.clBuildProgram(program,1,&info.devicePV,"-cl-std=CL1.2",nullptr,nullptr)!=0) return false;
        kernel=runtime.clCreateKernel(program,"starfield_sprite_render",&error);
        if(error || !kernel) return false;
        copy_kernel=runtime.clCreateKernel(program,"starfield_copy_pixels",&error);
        return error==0 && copy_kernel!=nullptr;
    }
    int sync() { return info.device_framework==PF_GPU_Framework_CUDA ? runtime.cuStreamSynchronize(info.command_queuePV) : runtime.clFinish(info.command_queuePV); }
    int upload(void* buffer,const void* source,std::size_t bytes) {
        return info.device_framework==PF_GPU_Framework_CUDA ?
            runtime.cuMemcpyHtoDAsync(reinterpret_cast<DevicePtr>(buffer),source,bytes,info.command_queuePV) :
            runtime.clEnqueueWriteBuffer(info.command_queuePV,buffer,0,0,bytes,source,0,nullptr,nullptr);
    }
    int dispatch(void* output,const std::array<void*,4>& buffers,std::array<unsigned,11>& values,float gain,unsigned rows) {
        if(info.device_framework==PF_GPU_Framework_CUDA) {
            std::array<DevicePtr,5> ptrs{reinterpret_cast<DevicePtr>(output),reinterpret_cast<DevicePtr>(buffers[0]),
                reinterpret_cast<DevicePtr>(buffers[1]),reinterpret_cast<DevicePtr>(buffers[2]),reinterpret_cast<DevicePtr>(buffers[3])};
            std::array<void*,17> args{};
            for(unsigned i=0;i<5;++i) args[i]=&ptrs[i];
            for(unsigned i=0;i<11;++i) args[i+5]=&values[i];
            args[16]=&gain;
            return runtime.cuLaunchKernel(kernel,(values[0]+15)/16,(rows+7)/8,1,16,8,1,0,info.command_queuePV,args.data(),nullptr);
        }
        std::array<void*,5> ptrs{output,buffers[0],buffers[1],buffers[2],buffers[3]};
        for(unsigned i=0;i<5;++i) {const auto err=runtime.clSetKernelArg(kernel,i,sizeof(void*),&ptrs[i]);if(err) return err;}
        for(unsigned i=0;i<11;++i) {const auto err=runtime.clSetKernelArg(kernel,i+5,sizeof(unsigned),&values[i]);if(err) return err;}
        if(const auto err=runtime.clSetKernelArg(kernel,16,sizeof(float),&gain);err)return err;
        const std::size_t global[2]{values[0],rows};
        // Let each driver choose legal local sizes, including narrow ROI worlds.
        return runtime.clEnqueueNDRangeKernel(info.command_queuePV,kernel,2,nullptr,global,nullptr,0,nullptr,nullptr);
    }
};
struct Suites {
    SPBasicSuite* basic{}; const PF_GPUDeviceSuite1* gpu{}; const PF_WorldSuite2* world{};
    ~Suites() {
        if(world) basic->ReleaseSuite(kPFWorldSuite,kPFWorldSuiteVersion2);
        if(gpu) basic->ReleaseSuite(kPFGPUDeviceSuite,kPFGPUDeviceSuiteVersion1);
    }
    PF_Err acquire(PF_InData* in,bool worlds=false) {
        if(!in || !in->pica_basicP || !in->pica_basicP->AcquireSuite || !in->pica_basicP->ReleaseSuite) return PF_Err_BAD_CALLBACK_PARAM;
        basic=in->pica_basicP;
        auto err=basic->AcquireSuite(kPFGPUDeviceSuite,kPFGPUDeviceSuiteVersion1,reinterpret_cast<const void**>(&gpu));
        if(!err && worlds) err=basic->AcquireSuite(kPFWorldSuite,kPFWorldSuiteVersion2,reinterpret_cast<const void**>(&world));
        return static_cast<PF_Err>(err);
    }
};
Device* device(const void* value) noexcept {
    auto* result=const_cast<Device*>(static_cast<const Device*>(value));
    return result && result->magic==Device::signature ? result : nullptr;
}
struct FrameMemory {
    const PF_GPUDeviceSuite1& suite; PF_ProgPtr effect; Device& device;
    std::array<void*,4> buffers{}; void* pinned{}; bool submitted{};
    ~FrameMemory() {
        // Drain even on an enqueue/launch error before releasing pinned inputs.
        if(submitted) (void)device.sync();
        for(auto* buffer:buffers) if(buffer) (void)suite.FreeDeviceMemory(effect,device.index,buffer);
        if(pinned) (void)suite.FreeHostMemory(effect,device.index,pinned);
    }
};
}
GpuExecutionInfo last_gpu_execution() noexcept {
    const auto trace=execution_trace.load(std::memory_order_relaxed);
    return {bool(trace>>63),static_cast<PF_GPU_Framework>(trace&255),static_cast<A_u_long>((trace>>8)&0xffffffff)};
}
GpuSetupTiming last_gpu_setup_timing() noexcept {
    std::lock_guard lock(setup_trace_mutex);return setup_trace;
}
void record_cpu_execution() noexcept { execution_trace.store(1ull<<63,std::memory_order_relaxed); }
bool gpu_device_matches(const void* data,PF_GPU_Framework framework,A_u_long index) noexcept {
    const auto* d=device(data);
    return d && d->kernel && d->info.device_framework==framework && d->index==index;
}
PF_Err gpu_device_setup(PF_InData* in,PF_OutData* out,PF_GPUDeviceSetupExtra* extra) noexcept try {
    SetupTimer timer;
    if(!in || !out || !extra || !extra->input || !extra->output) return PF_Err_BAD_CALLBACK_PARAM;
    extra->output->gpu_data=nullptr;
    out->out_flags2 &= ~PF_OutFlag2_SUPPORTS_GPU_RENDER_F32;
    const auto framework=extra->input->what_gpu;
    if(framework!=PF_GPU_Framework_CUDA && framework!=PF_GPU_Framework_OPENCL) return PF_Err_NONE;
    Suites suites; if(suites.acquire(in)!=PF_Err_NONE || !suites.gpu->GetDeviceInfo) return PF_Err_NONE;
    auto d=std::make_unique<Device>(); d->index=extra->input->device_index;
    if(suites.gpu->GetDeviceInfo(in->effect_ref,d->index,&d->info)!=PF_Err_NONE || !d->info.compatibleB ||
        d->info.device_framework!=framework || !d->info.contextPV ||
        (framework==PF_GPU_Framework_OPENCL && (!d->info.devicePV || !d->info.command_queuePV))) return PF_Err_NONE;
    if(!d->initialize()) return PF_Err_NONE;
    extra->output->gpu_data=d.release(); out->out_flags2 |= PF_OutFlag2_SUPPORTS_GPU_RENDER_F32;
    return PF_Err_NONE;
} catch(...) { return PF_Err_NONE; } // device rejection is a CPU negotiation, not an AE error dialog
PF_Err gpu_device_setdown(PF_InData* in,PF_GPUDeviceSetdownExtra* extra) noexcept {
    (void)in;
    if(!extra || !extra->input) return PF_Err_BAD_CALLBACK_PARAM;
    auto* d=device(extra->input->gpu_data);
    if(!d) return extra->input->gpu_data ? PF_Err_BAD_CALLBACK_PARAM : PF_Err_NONE;
    if(!gpu_device_matches(d,extra->input->what_gpu,extra->input->device_index)) return PF_Err_BAD_CALLBACK_PARAM;
    delete d; extra->input->gpu_data=nullptr;
    return PF_Err_NONE;
}
PF_Err render_gpu_scene(PF_InData* in,PF_OutData* out,const void* data,PF_GPU_Framework framework,A_u_long index,
    const SfCoreGpuSceneResult& scene,PF_EffectWorld* output,std::int32_t world_left,std::int32_t world_top,bool straight,unsigned samples,float gain) noexcept try {
    auto* d=device(data);
    if(!in || !out || !output || !gpu_device_matches(d,framework,index) || scene.status!=SF_CORE_OK ||
        scene.struct_size!=sizeof(scene) || scene.tile_size!=16 || samples<1 || samples>64 || !(gain>=1 && gain<=11)) return PF_Err_BAD_CALLBACK_PARAM;
    std::lock_guard lock(d->mutex);
    ContextScope context(d->runtime,framework,d->info.contextPV);
    if(!context.valid) return PF_Err_BAD_CALLBACK_PARAM;
    Suites suites; const auto suite_error=suites.acquire(in,true); if(suite_error) return suite_error;
    const auto& gpu=*suites.gpu;
    if(!gpu.GetGPUWorldData || !gpu.GetGPUWorldSize || !gpu.GetGPUWorldDeviceIndex ||
        !gpu.AllocateDeviceMemory || !gpu.FreeDeviceMemory || !gpu.AllocateHostMemory || !gpu.FreeHostMemory ||
        !suites.world->PF_GetPixelFormat) return PF_Err_BAD_CALLBACK_PARAM;
    PF_PixelFormat format{}; A_u_long actual_index{}; std::size_t world_size{}; void* output_buffer{};
    if(suites.world->PF_GetPixelFormat(output,&format)!=PF_Err_NONE || format!=PF_PixelFormat_GPU_BGRA128 ||
        gpu.GetGPUWorldDeviceIndex(in->effect_ref,output,&actual_index)!=PF_Err_NONE || actual_index!=index ||
        gpu.GetGPUWorldSize(in->effect_ref,output,&world_size)!=PF_Err_NONE ||
        gpu.GetGPUWorldData(in->effect_ref,output,&output_buffer)!=PF_Err_NONE || !output_buffer ||
        output->width<=0 || output->height<=0 || output->width>32768 || output->height>32768 ||
        output->rowbytes<std::int64_t(output->width)*16 || output->rowbytes%4!=0 ||
        std::uint64_t(output->rowbytes)*output->height>world_size) return PF_Err_BAD_CALLBACK_PARAM;
    const auto roi_width=std::int64_t(scene.region.right)-scene.region.left,roi_height=std::int64_t(scene.region.bottom)-scene.region.top;
    // Core scene ROI is contained in this world's pixel rectangle. The adapter
    // normalizes host origins/downsampling before dispatching; it passes offsets below.
    if(scene.sprite_count>8000000 || scene.index_count>32u*1024u*1024u || roi_width<0 || roi_height<0 || roi_width>output->width || roi_height>output->height ||
        scene.tiles_x!=(unsigned(roi_width)+15)/16 || scene.tiles_y!=(unsigned(roi_height)+15)/16 ||
        (scene.sprite_count && !scene.sprites) || (scene.index_count && !scene.tile_indices) ||
        (roi_width && roi_height && !scene.tile_offsets) || scene.cloud_circle_count>core::kMaxCloudMembers ||
        (scene.cloud_circle_count && !scene.cloud_circles)) return PF_Err_BAD_CALLBACK_PARAM;
    for(std::uint32_t i=0;i<scene.sprite_count;++i) {
        if((i&4095)==0) {const auto aborted=PF_ABORT(in);if(aborted)return aborted;}
        const auto& s=scene.sprites[i];
        if(s.reserved[0]>3 || (s.reserved[2] && s.shape!=2) || s.reserved[2]>core::kMaxCloudCircles ||
            s.reserved[1]>scene.cloud_circle_count || s.reserved[2]>scene.cloud_circle_count-s.reserved[1] ||
            (!s.reserved[2] && s.reserved[1]))return PF_Err_BAD_CALLBACK_PARAM;
    }
    for(std::uint32_t i=0;i<scene.cloud_circle_count;++i) {
        if((i&4095)==0) {const auto aborted=PF_ABORT(in);if(aborted)return aborted;}
        const auto& c=scene.cloud_circles[i];
        if(!std::isfinite(c.x) || !std::isfinite(c.y) || !std::isfinite(c.radius) || c.radius<=0 || c.radius>1 || c.reserved!=0)
            return PF_Err_BAD_CALLBACK_PARAM;
    }
    // Relative placement is supplied through normalized output origins by SmartRender.
    const auto roi_x=roi_width && roi_height?std::int64_t(scene.region.left)-world_left:0,roi_y=roi_width && roi_height?std::int64_t(scene.region.top)-world_top:0;
    if(roi_x<0 || roi_y<0 || roi_x+roi_width>output->width || roi_y+roi_height>output->height) return PF_Err_BAD_CALLBACK_PARAM;
    FrameMemory memory{gpu,in->effect_ref,*d};
    std::array<std::size_t,4> bytes{std::max<std::size_t>(16,std::size_t(scene.sprite_count)*sizeof(SfGpuSprite)),
        std::max<std::size_t>(16,(std::size_t(scene.tiles_x)*scene.tiles_y+1)*samples*sizeof(std::uint32_t)),
        std::max<std::size_t>(16,std::size_t(scene.index_count)*sizeof(std::uint32_t)),
        std::max<std::size_t>(16,std::size_t(scene.cloud_circle_count)*sizeof(SfGpuCloudCircle))};
    const auto total=bytes[0]+bytes[1]+bytes[2]+bytes[3];
    if(total>512u*1024u*1024u)return PF_Err_OUT_OF_MEMORY;
    const auto host_error=gpu.AllocateHostMemory(in->effect_ref,index,total,&memory.pinned);
    if(host_error || !memory.pinned) return host_error?host_error:PF_Err_OUT_OF_MEMORY;
    std::memset(memory.pinned,0,total);
    const void* sources[4]{scene.sprites,scene.tile_offsets,scene.tile_indices,scene.cloud_circles};
    const std::size_t sizes[4]{std::size_t(scene.sprite_count)*sizeof(SfGpuSprite),
        roi_width && roi_height?(std::size_t(scene.tiles_x)*scene.tiles_y+1)*samples*4:0,std::size_t(scene.index_count)*4,std::size_t(scene.cloud_circle_count)*sizeof(SfGpuCloudCircle)};
    auto* staging=static_cast<std::byte*>(memory.pinned);
    for(unsigned i=0;i<4;++i) {
        if(sizes[i]) std::memcpy(staging,sources[i],sizes[i]);
        const auto allocation=gpu.AllocateDeviceMemory(in->effect_ref,index,bytes[i],&memory.buffers[i]);
        if(allocation || !memory.buffers[i]) return allocation?allocation:PF_Err_OUT_OF_MEMORY;
        memory.submitted=true;
        if(d->upload(memory.buffers[i],staging,bytes[i])!=0) return PF_Err_INTERNAL_STRUCT_DAMAGED;
        staging+=bytes[i];
    }
    if(d->sync()!=0) return PF_Err_INTERNAL_STRUCT_DAMAGED; // explicit upload dependency, including out-of-order OpenCL queues
    std::array<unsigned,11> values{unsigned(output->width),unsigned(output->height),unsigned(output->rowbytes/4),
        scene.tiles_x,0,unsigned(roi_x),unsigned(roi_y),unsigned(roi_width),unsigned(roi_height),straight?1u:0u,samples};
    for(unsigned y=0;y<values[1];y+=128) {
        const auto interrupted=PF_ABORT(in); if(interrupted) {out->return_msg[0]='\0';return interrupted;}
        values[4]=y;
        if(d->dispatch(output_buffer,memory.buffers,values,gain,std::min(128u,values[1]-y))!=0 || d->sync()!=0) {
            std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield native GPU dispatch failed (%s, device %lu).",
                framework==PF_GPU_Framework_CUDA?"CUDA":"OpenCL",static_cast<unsigned long>(index));
            return PF_Err_INTERNAL_STRUCT_DAMAGED;
        }
    }
    execution_trace.store((1ull<<63)|(std::uint64_t(index)<<8)|std::uint64_t(framework),std::memory_order_relaxed);
    return PF_Err_NONE;
} catch(const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
catch(...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
PF_Err copy_gpu_pixels(PF_InData* in,const void* data,PF_GPU_Framework framework,A_u_long index,
    PF_EffectWorld* input,PF_EffectWorld* output) noexcept try {
    auto* d=device(data);
    if(!in || !input || !output || !gpu_device_matches(d,framework,index) || !d->copy_kernel) return PF_Err_BAD_CALLBACK_PARAM;
    if(output->width==0 || output->height==0) return PF_Err_NONE;
    std::lock_guard lock(d->mutex);ContextScope context(d->runtime,framework,d->info.contextPV);
    if(!context.valid) return PF_Err_BAD_CALLBACK_PARAM;
    Suites suites;const auto suite_error=suites.acquire(in,true);if(suite_error)return suite_error;
    const auto& gpu=*suites.gpu;
    if(!gpu.GetGPUWorldData || !gpu.GetGPUWorldSize || !gpu.GetGPUWorldDeviceIndex || !suites.world->PF_GetPixelFormat) return PF_Err_BAD_CALLBACK_PARAM;
    std::array<void*,2> buffers{};
    PF_EffectWorld* worlds[2]{output,input};
    for(unsigned i=0;i<2;++i) {
        auto* world=worlds[i];PF_PixelFormat format{};A_u_long actual{};std::size_t bytes{};
        if(suites.world->PF_GetPixelFormat(world,&format)!=0 || format!=PF_PixelFormat_GPU_BGRA128 ||
            gpu.GetGPUWorldDeviceIndex(in->effect_ref,world,&actual)!=0 || actual!=index ||
            gpu.GetGPUWorldSize(in->effect_ref,world,&bytes)!=0 || gpu.GetGPUWorldData(in->effect_ref,world,&buffers[i])!=0 || !buffers[i] ||
            world->width<=0 || world->height<=0 || world->width>32768 || world->height>32768 ||
            world->rowbytes<std::int64_t(world->width)*16 || world->rowbytes%4!=0 || std::uint64_t(world->rowbytes)*world->height>bytes)
            return PF_Err_BAD_CALLBACK_PARAM;
    }
    const auto dx=std::int64_t(output->origin_x)-input->origin_x,dy=std::int64_t(output->origin_y)-input->origin_y;
    if(dx < -32768 || dx > 32768 || dy < -32768 || dy > 32768 ||
        (buffers[0]==buffers[1] && (dx!=0 || dy!=0 || output->rowbytes!=input->rowbytes))) return PF_Err_BAD_CALLBACK_PARAM;
    std::array<unsigned,9> values{unsigned(output->width),unsigned(output->height),unsigned(output->rowbytes/4),
        unsigned(input->width),unsigned(input->height),unsigned(input->rowbytes/4),unsigned(std::int32_t(dx)),unsigned(std::int32_t(dy)),0};
    for(unsigned y=0;y<values[1];y+=128) {
        const auto abort=PF_ABORT(in);if(abort)return abort;
        values[8]=y;const auto rows=std::min(128u,values[1]-y);int error{};
        if(framework==PF_GPU_Framework_CUDA) {
            std::array<DevicePtr,2> pointers{reinterpret_cast<DevicePtr>(buffers[0]),reinterpret_cast<DevicePtr>(buffers[1])};
            std::array<void*,11> args{&pointers[0],&pointers[1]};for(unsigned i=0;i<9;++i)args[i+2]=&values[i];
            error=d->runtime.cuLaunchKernel(d->copy_kernel,(values[0]+15)/16,(rows+7)/8,1,16,8,1,0,d->info.command_queuePV,args.data(),nullptr);
        } else {
            for(unsigned i=0;i<2 && !error;++i)error=d->runtime.clSetKernelArg(d->copy_kernel,i,sizeof(void*),&buffers[i]);
            for(unsigned i=0;i<9 && !error;++i)error=d->runtime.clSetKernelArg(d->copy_kernel,i+2,sizeof(unsigned),&values[i]);
            const std::size_t global[2]{values[0],rows};
            if(!error)error=d->runtime.clEnqueueNDRangeKernel(d->info.command_queuePV,d->copy_kernel,2,nullptr,global,nullptr,0,nullptr,nullptr);
        }
        // Borrowed worlds remain alive through completion even after enqueue errors.
        const auto completion=d->sync();if(error || completion)return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
    return PF_Err_NONE;
} catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}

}
