#define NOMINMAX
#include <windows.h>
#include "starfield/core/PluginApi.h"
#include <atomic>
#include <barrier>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
namespace {
std::atomic<unsigned> checks{},failures{};
void check(bool ok,const char* text){++checks;if(!ok){++failures;std::printf("FAIL: %s\n",text);}}
struct Owned {
    const SfCoreApi& api;SfCoreRenderResult value{};
    explicit Owned(const SfCoreApi& owner):api(owner){value.struct_size=sizeof(value);}
    ~Owned(){api.release_render_result(&value);}
};
struct Snapshot {
    SfCoreRect region{};unsigned row{},format{};std::vector<char> pixels;
    std::uint64_t live{},nodes{},edges{};
};
int32_t SF_CORE_CALL cancelled(void*){return 1;}
Snapshot snapshot(const SfCoreRenderResult& result,const SfCoreInspectResult& inspection){
    Snapshot output;output.region=result.region;output.row=result.row_bytes;output.format=result.pixel_format;
    if(result.pixel_byte_count){check(result.pixels!=nullptr,"result storage present");if(result.pixels){const auto* bytes=static_cast<const char*>(result.pixels);output.pixels.assign(bytes,bytes+std::size_t(result.pixel_byte_count));}}
    output.live=inspection.live_particle_count;output.nodes=inspection.node_count;output.edges=inspection.edge_count;return output;
}
bool same(const Snapshot& a,const Snapshot& b){return std::memcmp(&a.region,&b.region,sizeof(a.region))==0 && a.row==b.row && a.format==b.format && a.pixels==b.pixels && a.live==b.live && a.nodes==b.nodes && a.edges==b.edges;}
SfCoreInspectResult inspect(const SfCoreApi& api,const SfCoreRenderRequest& request){
    const SfCoreInspectRequest input{sizeof(SfCoreInspectRequest),request.graph_bytes,request.graph_byte_count,request.frame.time_value,request.frame.time_scale};
    SfCoreInspectResult output{};output.struct_size=sizeof(output);
    check(api.inspect(&input,&output)==SF_CORE_OK && output.status==SF_CORE_OK,"actual DLL concurrent inspect");return output;
}
}
int main(){
    std::setvbuf(stdout,nullptr,_IONBF,0);
    const wchar_t* paths[]={L"artifacts/prepared/stb20-native129-panel124-v2/native-bundle/StarfieldCore.dll",L"artifacts/prepared/mfr01-native132-panel124-host-qualification-v1/native-bundle/StarfieldCore.dll"};
    HMODULE modules[2]{};SfCoreApi apis[2]{};
    for(unsigned generation=0;generation<2;++generation){
        modules[generation]=LoadLibraryW(paths[generation]);check(modules[generation]!=nullptr,"load actual frozen Core generation");if(!modules[generation])return 2;
        const auto entry=reinterpret_cast<SfCoreGetApiFn>(GetProcAddress(modules[generation],"StarfieldCore_GetApi"));
        check(entry && entry(SF_CORE_ABI_VERSION,sizeof(SfCoreApi),&apis[generation])==1,"negotiate actual ABI9 generation");if(!entry || !apis[generation].render)return 3;
    }
    std::vector<std::vector<char>> graphs;
    for(unsigned scene=0;scene<4;++scene){std::ifstream file("artifacts/ux-16-20261116/scene-"+std::to_string(scene)+".bin",std::ios::binary);graphs.emplace_back(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());check(!graphs.back().empty(),"pinned project-owned graph fixture");if(graphs.back().empty())return 4;}
    std::vector<SfCoreRenderRequest> requests;std::vector<Snapshot> reference;
    for(unsigned scene=0;scene<4;++scene)for(auto time:{0,24,72})for(unsigned format=0;format<3;++format){
        SfCoreRenderRequest request{};request.struct_size=sizeof(request);request.graph_bytes=graphs[scene].data();request.graph_byte_count=graphs[scene].size();
        request.frame=SfCoreFrame{128,64,128,64,{3,2,117,59},time,24,1,24,format,0,1,1,1.0};
        Snapshot old;
        for(unsigned generation=0;generation<2;++generation){Owned result(apis[generation]);
            check(apis[generation].render(&request,&result.value)==SF_CORE_OK && result.value.status==SF_CORE_OK,"actual DLL serial reference");
            auto saved=snapshot(result.value,inspect(apis[generation],request));
            if(generation==0)old=std::move(saved);else check(same(old,saved),"frozen129 versus132 actual DLL pixels/region/stride/format/inspect exact");
        }
        requests.push_back(request);reference.push_back(std::move(old));
    }
    std::barrier start(4);std::vector<std::thread> workers;
    for(unsigned worker=0;worker<4;++worker)workers.emplace_back([&,worker]{start.arrive_and_wait();
        for(unsigned frame=0;frame<100;++frame){
            const auto index=(frame*7+worker*11)%requests.size();const auto& api=apis[(frame+worker)%2];const auto& request=requests[index];
            Owned result(api);check(api.render(&request,&result.value)==SF_CORE_OK,"actual DLL parallel render");
            const auto info=inspect(api,request);std::this_thread::yield();
            // Hold this result across other calls/failures. Another request must
            // not overwrite it, and release must run in its owning generation.
            if(frame%17==0){auto stopped=requests[4];stopped.is_cancelled=cancelled;Owned interrupted(api);check(api.render(&stopped,&interrupted.value)==SF_CORE_CANCELLED,"actual DLL request-local cancellation");}
            check(same(reference[index],snapshot(result.value,info)),"actual DLL parallel result equals serial bytes and survives interleaved calls");
            api.release_render_result(&result.value);check(result.value.pixels==nullptr && result.value.opaque_handle==nullptr && result.value.pixel_byte_count==0,"owning generation release clears result");
            // Destructor also checks the supported release of an empty result.
        }
    });
    for(auto& worker:workers)worker.join();
    for(auto module:modules)check(FreeLibrary(module)!=0,"unload only after all generation-owned results release");
    std::printf("Actual Core DLL concurrency: %u checks, %u failures; 36 serial generation pairs, 400 parallel frames, 24 cancellations, RGBA8/16/32F. No AE qualification.\n",unsigned(checks),unsigned(failures));
    return failures?1:0;
}
