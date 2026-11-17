// Exercise the production adapter without a GPU/AE process. Only the driver and
// host callbacks below are fake; no product-only test hooks or copied adapter.
#ifndef STARFIELD_GPU_IMPLEMENTATION
#define STARFIELD_GPU_IMPLEMENTATION "../ae_plugin/GpuRender.cpp"
#endif
#include STARFIELD_GPU_IMPLEMENTATION
#include <condition_variable>
#include <functional>
#include <thread>
#include <vector>

using namespace starfield::adapter;
namespace {
std::atomic<int> checks{},failures{},kernels{},allocations{},suite_refs{};
void check(bool ok,const char* text) {
    ++checks;if(!ok){++failures;std::printf("FAIL: %s\n",text);}
}
struct Rendezvous {
    std::mutex mutex;std::condition_variable cv;int arrived{};
    bool enter() {
        std::unique_lock lock(mutex);++arrived;cv.notify_all();
        return cv.wait_for(lock,std::chrono::seconds(2),[&]{return arrived==2;});
    }
};
enum class Fault { none,kernel,allocation,upload,argument,enqueue,sync };
struct Frame {
    Fault fault{};int aborts{},cancel_at{},allocate_calls{},fail_allocate_at{1};
    Rendezvous* rendezvous{};std::function<void()> nested;
};
thread_local Frame* active{};
struct Active {
    Frame* previous{active};explicit Active(Frame& frame){active=&frame;}
    ~Active(){active=previous;}
};
bool fail(Fault fault){return active && active->fault==fault;}
struct Kernel {
    bool copy{};std::array<void*,5> pointers{};std::array<unsigned,11> values{};float gain{};
    std::thread::id owner{std::this_thread::get_id()};
};
std::mutex queue_mutex;
std::vector<std::function<void()>> commands;
int __stdcall finish(void*) {
    std::lock_guard lock(queue_mutex);
    for(auto& command:commands)command();commands.clear();
    return fail(Fault::sync)?-1:0;
}
void* __stdcall create_kernel(void*,const char* name,int* error) {
    *error=fail(Fault::kernel)?-6:0;if(*error)return nullptr;
    auto* kernel=new Kernel;kernel->copy=std::strcmp(name,"starfield_copy_pixels")==0;
    ++kernels;return kernel;
}
int __stdcall release_kernel(void* handle){delete static_cast<Kernel*>(handle);--kernels;return 0;}
int __stdcall argument(void* handle,unsigned index,std::size_t size,const void* value) {
    auto& k=*static_cast<Kernel*>(handle);
    check(k.owner==std::this_thread::get_id(),"OpenCL arguments belong to this request thread");
    if(fail(Fault::argument))return -1;
    const unsigned pointers=k.copy?2u:5u;
    if(index<pointers){check(size==sizeof(void*),"pointer argument size");std::memcpy(&k.pointers[index],value,size);}
    else if(!k.copy && index==16){check(size==sizeof(float),"gain argument size");std::memcpy(&k.gain,value,size);}
    else {check(size==sizeof(unsigned),"integer argument size");std::memcpy(&k.values[index-pointers],value,size);}
    std::this_thread::yield();return 0;
}
int __stdcall enqueue(void*,void* handle,unsigned,const std::size_t*,const std::size_t* global,const std::size_t*,unsigned,const void*,void*) {
    if(fail(Fault::enqueue))return -1;
    const Kernel k=*static_cast<Kernel*>(handle);const auto rows=unsigned(global[1]);
    std::lock_guard lock(queue_mutex);
    // Delay consumption until clFinish. Inputs must survive pending work even
    // when another request completes/cancels first on the shared host queue.
    commands.push_back([k,rows]{
        auto* output=static_cast<float*>(k.pointers[0]);const auto& v=k.values;
        const auto start=k.copy?v[8]:v[4];
        for(unsigned y=start;y<start+rows;++y)for(unsigned x=0;x<v[0];++x) {
            auto* pixel=output+std::size_t(y)*v[2]+x*4;
            if(k.copy) {
                const auto sx=std::int64_t(x)+std::int32_t(v[6]),sy=std::int64_t(y)+std::int32_t(v[7]);
                if(sx>=0 && sy>=0 && sx<v[3] && sy<v[4])
                    std::memcpy(pixel,static_cast<float*>(k.pointers[1])+std::size_t(sy)*v[5]+sx*4,16);
                else std::fill_n(pixel,4,0.f);
            } else {
                const auto value=static_cast<SfGpuSprite*>(k.pointers[1])->x*k.gain;
                std::fill_n(pixel,4,value);
            }
        }
    });return 0;
}
int __stdcall upload(void*,void* buffer,unsigned,std::size_t offset,std::size_t bytes,const void* source,unsigned,const void*,void*) {
    if(fail(Fault::upload))return -1;
    std::lock_guard lock(queue_mutex);
    commands.push_back([=]{std::memcpy(static_cast<std::byte*>(buffer)+offset,source,bytes);});return 0;
}
PF_Err alloc(PF_ProgPtr,A_u_long,std::size_t size,void** result) {
    if(active && ++active->allocate_calls==active->fail_allocate_at && fail(Fault::allocation))return PF_Err_OUT_OF_MEMORY;
    *result=std::malloc(size);if(!*result)return PF_Err_OUT_OF_MEMORY;++allocations;return 0;
}
PF_Err free_memory(PF_ProgPtr,A_u_long,void* buffer){std::free(buffer);--allocations;return 0;}
PF_Err world_data(PF_ProgPtr,PF_EffectWorld* world,void** result){*result=world->data;return 0;}
PF_Err world_size(PF_ProgPtr,PF_EffectWorld* world,std::size_t* result){*result=std::size_t(world->rowbytes)*world->height;return 0;}
PF_Err world_index(PF_ProgPtr,PF_EffectWorld*,A_u_long* result){*result=7;return 0;}
PF_Err world_format(const PF_EffectWorld*,PF_PixelFormat* result){*result=PF_PixelFormat_GPU_BGRA128;return 0;}
PF_GPUDeviceSuite1 gpu{};PF_WorldSuite2 worlds{};
SPErr acquire(const char* name,int version,const void** result) {
    if(std::strcmp(name,kPFGPUDeviceSuite)==0 && version==1)*result=&gpu;
    else if(std::strcmp(name,kPFWorldSuite)==0 && version==2)*result=&worlds;
    else return PF_Err_BAD_CALLBACK_PARAM;
    ++suite_refs;return 0;
}
SPErr release(const char*,int){--suite_refs;return 0;}
PF_Err abort_render(PF_ProgPtr) {
    auto& f=*active;++f.aborts;
    if(f.aborts==1 && f.rendezvous)check(f.rendezvous->enter(),"overlapping selectors reach host callback without a device lock");
    if(f.aborts==1 && f.nested)f.nested();
    return f.cancel_at==f.aborts?PF_Interrupt_CANCEL:PF_Err_NONE;
}
struct World {
    PF_EffectWorld value{};std::vector<float> pixels;
    explicit World(float sentinel,unsigned width=17,unsigned height=260):pixels(std::size_t(width+2)*height*4,sentinel) {
        value.width=A_long(width);value.height=A_long(height);value.rowbytes=A_long((width+2)*16);
        value.data=reinterpret_cast<PF_Pixel*>(pixels.data());
    }
    bool equals(float value_)const {
        for(A_long y=0;y<value.height;++y)for(A_long x=0;x<value.rowbytes/4;++x)
            if(pixels[std::size_t(y)*(value.rowbytes/4)+x]!=(x<value.width*4?value_: -1.f))return false;
        return true;
    }
};
PF_Err render(Device& device,Frame& frame,World& world,float value) {
    Active scope(frame);SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
    PF_InData in{};in.pica_basicP=&basic;in.inter.abort=abort_render;PF_OutData out{};
    SfGpuSprite sprite{};sprite.x=value;
    const auto tiles_x=(unsigned(world.value.width)+15)/16,tiles_y=(unsigned(world.value.height)+15)/16;
    std::vector<unsigned> offsets(tiles_x*tiles_y+1);
    SfCoreGpuSceneResult scene{};scene.struct_size=sizeof(scene);scene.status=SF_CORE_OK;
    scene.region={0,0,world.value.width,world.value.height};scene.tile_size=16;scene.tiles_x=tiles_x;scene.tiles_y=tiles_y;
    scene.sprite_count=1;scene.sprites=&sprite;scene.tile_offsets=offsets.data();
    return render_gpu_scene(&in,&out,&device,PF_GPU_Framework_OPENCL,7,scene,&world.value,0,0,true);
}
PF_Err copy(Device& device,Frame& frame,World& input,World& output) {
    Active scope(frame);SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
    PF_InData in{};in.pica_basicP=&basic;in.inter.abort=abort_render;
    return copy_gpu_pixels(&in,&device,PF_GPU_Framework_OPENCL,7,&input.value,&output.value);
}
void clean(){check(allocations==0 && kernels==0 && suite_refs==0 && commands.empty(),"all frame kernels, pinned/device allocations, pending commands and suites released");}
}
int main() {
    std::setvbuf(stdout,nullptr,_IONBF,0);
    gpu.AllocateDeviceMemory=alloc;gpu.FreeDeviceMemory=free_memory;gpu.AllocateHostMemory=alloc;gpu.FreeHostMemory=free_memory;
    gpu.GetGPUWorldData=world_data;gpu.GetGPUWorldSize=world_size;gpu.GetGPUWorldDeviceIndex=world_index;worlds.PF_GetPixelFormat=world_format;
    Device device;device.info.device_framework=PF_GPU_Framework_OPENCL;device.index=7;
    // Prototypes are capability tokens only; the fake runtime is not dynamically loaded.
    Kernel prototype;device.kernel=&prototype;device.copy_kernel=&prototype;device.program=&prototype;
    device.runtime.clCreateKernel=create_kernel;device.runtime.clReleaseKernel=release_kernel;
    device.runtime.clSetKernelArg=argument;device.runtime.clEnqueueNDRangeKernel=enqueue;
    device.runtime.clEnqueueWriteBuffer=upload;device.runtime.clFinish=finish;
    {
        Rendezvous overlap;Frame a,b;a.rendezvous=&overlap;b.rendezvous=&overlap;World x(-1),y(-1);
        std::thread first([&]{check(render(device,a,x,11)==0,"overlap first render");});
        std::thread second([&]{check(render(device,b,y,29)==0,"overlap second render");});
        first.join();second.join();check(x.equals(11) && y.equals(29),"overlapping frame arguments and outputs are independent");clean();
    }
    // The old adapter times out above; do not run the reentry counterexample on it.
    if(failures)return 1;
    {
        Frame outer,inner;World source(37),destination(-1),output(-1);
        outer.nested=[&]{check(copy(device,inner,source,destination)==0,"render host callback can reenter node GPU copy");};
        check(render(device,outer,output,19)==0 && output.equals(19) && destination.equals(37),"reentrant render/copy output");clean();
        Frame outer_copy,inner_render;World copy_source(41),copy_output(-1),inner_output(-1);
        outer_copy.nested=[&]{check(render(device,inner_render,inner_output,43)==0,"copy host callback can reenter main GPU render");};
        check(copy(device,outer_copy,copy_source,copy_output)==0 && copy_output.equals(41) && inner_output.equals(43),"reentrant copy/render output");clean();
    }
    for(auto fault:{Fault::kernel,Fault::allocation,Fault::upload,Fault::argument,Fault::enqueue,Fault::sync}) {
        Frame frame;frame.fault=fault;World output(-1);
        check(render(device,frame,output,3)!=0,"injected render failure propagates");clean();
        if(fault!=Fault::allocation && fault!=Fault::upload) {
            Frame node;node.fault=fault;World source(5),destination(-1);
            check(copy(device,node,source,destination)!=0,"injected copy failure propagates");clean();
        }
    }
    for(int allocation=2;allocation<=5;++allocation){Frame frame;frame.fault=Fault::allocation;frame.fail_allocate_at=allocation;World output(-1);check(render(device,frame,output,3)!=0,"partial allocation failure propagates");clean();}
    for(int at=1;at<=4;++at) {
        Frame frame;frame.cancel_at=at;World output(-1);
        check(render(device,frame,output,3)==PF_Interrupt_CANCEL,"render cancellation before/after submitted GPU work");clean();
        if(at<=3){Frame node;node.cancel_at=at;World source(7),destination(-1);check(copy(device,node,source,destination)==PF_Interrupt_CANCEL,"node copy cancellation across row chunks");clean();}
    }
    std::vector<std::thread> workers;
    for(unsigned worker=0;worker<4;++worker)workers.emplace_back([&,worker]{
        for(unsigned i=0;i<100;++i) {
            const float identity=float(worker*1000+i+1);Frame frame;World output(-1,1+(i%31),1+(i%260));
            check(render(device,frame,output,identity)==0 && output.equals(identity),"parallel render keeps frame identity and padding");
            Frame node;World input(identity,unsigned(output.value.width),unsigned(output.value.height)),copied(-1,unsigned(output.value.width),unsigned(output.value.height));
            check(copy(device,node,input,copied)==0 && copied.equals(identity),"parallel node copy keeps frame identity and padding");
        }
    });
    for(auto& worker:workers)worker.join();clean();
    std::printf("%d production GPU concurrency checks, %d failures; 400 render + 400 copy requests\n",int(checks),int(failures));
    return failures?1:0;
}
