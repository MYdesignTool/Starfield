// Included by the standalone GPU driver fixture, never by the plug-in. The
// existing fixture's serial host maps/counters are replaced only for this test.
namespace concurrent_driver {
std::atomic<int> failures{},frames{},allocations{};
PF_GPUDeviceSuite1 suite{};PF_WorldSuite2 world_suite{};
thread_local std::map<PF_EffectWorld*,std::pair<void*,std::size_t>> views;
thread_local int aborts{},cancel_at{};
void require(bool ok,const char* text){if(!ok){++failures;std::printf("Concurrent driver FAIL: %s\n",text);}}
struct Context {
    bool cuda{info.device_framework==PF_GPU_Framework_CUDA},valid{};
    Context(){valid=!cuda || proc<int (__stdcall *)(void*)>("cuCtxPushCurrent_v2")(context)==0;require(valid,"worker context acquired");}
    ~Context(){if(cuda && valid){void* previous{};require(proc<int (__stdcall *)(void**)>("cuCtxPopCurrent_v2")(&previous)==0,"worker context released");}}
};
PF_Err allocate(PF_ProgPtr,A_u_long,std::size_t bytes,void** result) {
    if(info.device_framework==PF_GPU_Framework_CUDA){std::uint64_t value{};if(cuMemAlloc(&value,bytes))return PF_Err_OUT_OF_MEMORY;*result=reinterpret_cast<void*>(value);}
    else {int error{};*result=clCreateBuffer(context,1,bytes,nullptr,&error);if(error)return PF_Err_OUT_OF_MEMORY;}
    ++allocations;return 0;
}
PF_Err dispose(PF_ProgPtr,A_u_long,void* value) {
    --allocations;return info.device_framework==PF_GPU_Framework_CUDA?static_cast<PF_Err>(cuMemFree(reinterpret_cast<std::uint64_t>(value))):static_cast<PF_Err>(clReleaseMem(value));
}
PF_Err host_allocate(PF_ProgPtr,A_u_long,std::size_t bytes,void** result) {
    if(info.device_framework==PF_GPU_Framework_CUDA){if(cuAllocHost(result,bytes))return PF_Err_OUT_OF_MEMORY;}
    else *result=std::malloc(bytes);
    if(!*result)return PF_Err_OUT_OF_MEMORY;++allocations;return 0;
}
PF_Err host_dispose(PF_ProgPtr,A_u_long,void* value){--allocations;if(info.device_framework==PF_GPU_Framework_CUDA)return static_cast<PF_Err>(cuFreeHost(value));std::free(value);return 0;}
PF_Err data(PF_ProgPtr,PF_EffectWorld* world,void** result){*result=views.at(world).first;return 0;}
PF_Err size(PF_ProgPtr,PF_EffectWorld* world,std::size_t* result){*result=views.at(world).second;return 0;}
SPErr acquire_suite(const char* name,int version,const void** result){
    if(std::strcmp(name,kPFGPUDeviceSuite)==0 && version==1)*result=&suite;
    else if(std::strcmp(name,kPFWorldSuite)==0 && version==2)*result=&world_suite;
    else return PF_Err_BAD_CALLBACK_PARAM;return 0;
}
SPErr release_suite(const char*,int){return 0;}
PF_Err abort_frame(PF_ProgPtr){std::this_thread::yield();return ++aborts==cancel_at?PF_Interrupt_CANCEL:PF_Err_NONE;}
bool write_buffer(void* buffer,const std::vector<std::byte>& bytes){return info.device_framework==PF_GPU_Framework_CUDA?cuCopyTo(reinterpret_cast<std::uint64_t>(buffer),bytes.data(),bytes.size())==0:clWrite(queue,buffer,1,0,bytes.size(),bytes.data(),0,nullptr,nullptr)==0;}
bool read_buffer(void* buffer,std::vector<std::byte>& bytes){return info.device_framework==PF_GPU_Framework_CUDA?cuCopyFrom(bytes.data(),reinterpret_cast<std::uint64_t>(buffer),bytes.size())==0:clRead(queue,buffer,1,0,bytes.size(),bytes.data(),0,nullptr,nullptr)==0;}
void run(void* device) {
    suite=gpu;suite.AllocateDeviceMemory=allocate;suite.FreeDeviceMemory=dispose;
    suite.AllocateHostMemory=host_allocate;suite.FreeHostMemory=host_dispose;suite.GetGPUWorldData=data;suite.GetGPUWorldSize=size;
    world_suite=worlds;failures=0;frames=0;
    std::array<std::vector<std::byte>,96> serial_pixels;
    {
        SPBasicSuite basic{};basic.AcquireSuite=acquire_suite;basic.ReleaseSuite=release_suite;
        PF_InData in{};in.pica_basicP=&basic;in.inter.abort=abort_frame;PF_OutData out{};
        PF_EffectWorld world{};world.width=56;world.height=44;world.rowbytes=56*16+64;world.origin_x=4;world.origin_y=2;
        const auto bytes=std::size_t(world.rowbytes)*world.height;void* buffer{};
        require(allocate(nullptr,7,bytes,&buffer)==0,"serial baseline allocation");if(!buffer)return;
        views[&world]={buffer,bytes};
        for(unsigned worker=0;worker<4;++worker)for(unsigned i=0;i<24;++i){
            auto r=request((worker+i)%3,((worker+i)%2)!=0);r.frame.time={std::int64_t((i*7+worker*5)%48+1),24};
            const core::NeverCancelled never;const auto scene=core::prepare_sprite_scene(r,never);
            require(scene.has_value(),"serial baseline scene");if(!scene.has_value())continue;
            auto v=view(scene.value());auto& pixels=serial_pixels[worker*24+i];pixels.assign(bytes,std::byte{0x7e});
            require(write_buffer(buffer,pixels),"serial baseline initialize");aborts=0;cancel_at=0;
            require(adapter::render_gpu_scene(&in,&out,device,info.device_framework,7,v,&world,4,2,r.frame.alpha_mode==core::AlphaMode::straight)==0,"serial baseline render");
            require(read_buffer(buffer,pixels),"serial baseline readback");
        }
        views.clear();require(dispose(nullptr,7,buffer)==0,"serial baseline world released");
    }
    std::vector<std::thread> workers;
    for(unsigned worker=0;worker<4;++worker)workers.emplace_back([=]{
        Context context_scope;if(!context_scope.valid)return;
        SPBasicSuite basic{};basic.AcquireSuite=acquire_suite;basic.ReleaseSuite=release_suite;
        PF_InData in{};in.pica_basicP=&basic;in.inter.abort=abort_frame;PF_OutData out{};
        PF_EffectWorld world{};world.width=56;world.height=44;world.rowbytes=56*16+64;world.origin_x=4;world.origin_y=2;
        PF_EffectWorld copied=world;const auto bytes=std::size_t(world.rowbytes)*world.height;
        void* buffer{},*copy_buffer{};require(allocate(nullptr,7,bytes,&buffer)==0,"frame world allocation");
        require(allocate(nullptr,7,bytes,&copy_buffer)==0,"copy world allocation");
        if(!buffer || !copy_buffer){if(buffer)(void)dispose(nullptr,7,buffer);if(copy_buffer)(void)dispose(nullptr,7,copy_buffer);return;}
        views[&world]={buffer,bytes};views[&copied]={copy_buffer,bytes};
        for(unsigned i=0;i<24;++i) {
            auto r=request((worker+i)%3,((worker+i)%2)!=0);
            r.frame.time={std::int64_t((i*7+worker*5)%48+1),24};
            const core::NeverCancelled never;const core::CpuParticleRenderer cpu;
            const auto scene=core::prepare_sprite_scene(r,never);const auto reference=cpu.render(r,never);
            require(scene.has_value() && reference.has_value(),"parallel CPU scene and reference");if(!scene.has_value() || !reference.has_value())continue;
            auto v=view(scene.value());std::vector<std::byte> pixels(bytes,std::byte{0x7e});require(write_buffer(buffer,pixels),"initialize frame world");
            aborts=0;cancel_at=0;
            require(adapter::render_gpu_scene(&in,&out,device,info.device_framework,7,v,&world,4,2,r.frame.alpha_mode==core::AlphaMode::straight)==0,"parallel production render");
            require(read_buffer(buffer,pixels),"parallel readback");bool equal=true;float max_difference{};const auto& expected=reference.value();
            for(A_long y=0;y<world.height;++y)for(A_long x=0;x<world.width;++x){
                float actual[4]{},target[4]{};std::memcpy(actual,pixels.data()+std::size_t(y)*world.rowbytes+x*16,16);
                const auto gx=x+4,gy=y+2;
                if(gx>=expected.region.left && gx<expected.region.right && gy>=expected.region.top && gy<expected.region.bottom)
                    std::memcpy(target,expected.pixels.data()+std::size_t(gy-expected.region.top)*expected.row_bytes+(gx-expected.region.left)*16,16);
                const float ordered[4]{target[2],target[1],target[0],target[3]};
                for(unsigned c=0;c<4;++c){max_difference=std::max(max_difference,std::abs(actual[c]-ordered[c]));if(!std::isfinite(actual[c]) || std::abs(actual[c]-ordered[c])>=.0002f)equal=false;}
            }
            for(A_long y=0;y<world.height;++y)for(A_long b=world.width*16;b<world.rowbytes;++b)
                if(pixels[std::size_t(y)*world.rowbytes+b]!=std::byte{0x7e})equal=false;
            if(!equal)std::printf("CPU comparison worker=%u frame=%u max_difference=%.9g\n",worker,i,max_difference);
            require(equal,"parallel frame pixels match CPU with untouched padding");
            require(pixels==serial_pixels[worker*24+i],"parallel render matches same-driver serial bytes exactly");
            std::vector<std::byte> copied_pixels(bytes,std::byte{0x7e});require(write_buffer(copy_buffer,copied_pixels),"initialize copy world");
            aborts=0;require(adapter::copy_gpu_pixels(&in,device,info.device_framework,7,&world,&copied)==0,"parallel production node copy");
            require(read_buffer(copy_buffer,copied_pixels) && copied_pixels==pixels,"parallel node copy exact bytes");
            aborts=0;cancel_at=2;
            require(adapter::render_gpu_scene(&in,&out,device,info.device_framework,7,v,&world,4,2,true)==PF_Interrupt_CANCEL,"parallel cancellation drains pending uploads");
            ++frames;
        }
        views.clear();require(dispose(nullptr,7,copy_buffer)==0 && dispose(nullptr,7,buffer)==0,"worker worlds released");
    });
    for(auto& worker:workers)worker.join();
    check(failures==0 && frames==96 && allocations==0,"four-thread actual-driver render/copy/cancel and CPU comparison");
    std::printf("Actual %s concurrent: %d frames + copies + cancels, %d failures\n",info.device_framework==PF_GPU_Framework_CUDA?"CUDA":"OpenCL",int(frames),int(failures));
}
}
