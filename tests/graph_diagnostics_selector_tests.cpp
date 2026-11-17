#include "GraphDiagnosticsRequest.hpp"
#include "GraphParameter.hpp"
#include "Parameters.hpp"
#include "UiExclusionClient.hpp"
#include "SPBasic.h"
#include "starfield/core/SequenceCodec.hpp"
#include <Windows.h>
#include <filesystem>
#include <iostream>
#include <map>
using namespace starfield::adapter;
using namespace starfield::core;
unsigned checks{},failures{},refs{},values{},suites{},reads{};double guard=0;std::uint32_t crc{};
std::map<PF_Handle,OpaqueBytes> memory;PF_Handle saved{};AEGP_StreamSuite6 streams{};
void check(bool v,const char* name){++checks;if(!v){++failures;std::cerr<<"FAILED: "<<name<<'\n';}}
PF_Handle allocate(A_u_longlong n){auto h=new void*{};memory[h].resize(static_cast<std::size_t>(n));return h;}
void* lock(PF_Handle h){return memory.at(h).data();}void unlock(PF_Handle){}
void dispose(PF_Handle h){memory.erase(h);delete h;}A_u_longlong size(PF_Handle h){return memory.at(h).size();}
SPErr acquire(const char* name,int32,const void** out){if(std::strcmp(name,kAEGPStreamSuite))return kSPBadParameterError;*out=&streams;++suites;return 0;}
SPErr release(const char*,int32){--suites;return 0;}
Uuid128 uuid(unsigned char value){Uuid128 id;id.bytes[15]=value;return id;}
void save(const Graph& graph){auto encoded=serialize_graph(graph,particle_node_registry());check(encoded.has_value(),"fixture serializes");if(saved)dispose(saved);saved=allocate(encoded.value().size());memory[saved]=encoded.value();crc=0;for(unsigned i=0;i<4;++i)crc|=std::to_integer<std::uint32_t>(encoded.value()[24+i])<<(8*i);}
int wmain(int argc,wchar_t** argv){
    if(argc!=3)return 2;
    auto host=LoadLibraryW(argv[2]);check(host!=nullptr,"load isolated resident exclusion fixture");if(!host)return 2;
    auto initialize=reinterpret_cast<void(*)()noexcept>(GetProcAddress(host,"SFLD_TestInitialize"));check(initialize!=nullptr,"fixture initialize export");if(!initialize)return 2;initialize();
    auto module=LoadLibraryW((std::filesystem::path(argv[1])/L"StarfieldParticle.aex").c_str());check(module!=nullptr,"load actual main candidate AEX");if(!module)return 2;
    using Entry=PF_Err(*)(PF_Cmd,PF_InData*,PF_OutData*,PF_ParamDef*[],PF_LayerDef*,void*);
    auto entry=reinterpret_cast<Entry>(GetProcAddress(module,"EffectMain"));check(entry!=nullptr,"actual entry exists");if(!entry)return 2;
    PF_UtilCallbacks utils{};utils.host_new_handle=allocate;utils.host_lock_handle=lock;utils.host_unlock_handle=unlock;utils.host_dispose_handle=dispose;utils.host_get_handle_size=size;
    SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
    streams.AEGP_GetNewEffectStreamByIndex=[](AEGP_PluginID,AEGP_EffectRefH,A_long index,AEGP_StreamRefH* out)->A_Err{++refs;*out=reinterpret_cast<AEGP_StreamRefH>(static_cast<std::uintptr_t>(index+1));return 0;};
    streams.AEGP_GetStreamType=[](AEGP_StreamRefH ref,AEGP_StreamType* out)->A_Err{const auto index=reinterpret_cast<std::uintptr_t>(ref)-1;*out=index==kGraphParameterId?AEGP_StreamType_ARB:AEGP_StreamType_OneD;return 0;};
    streams.AEGP_DisposeStream=[](AEGP_StreamRefH)->A_Err{--refs;return 0;};
    streams.AEGP_GetNewStreamValue=[](AEGP_PluginID,AEGP_StreamRefH ref,AEGP_LTimeMode,const A_Time* time,A_Boolean pre,AEGP_StreamValue2* out)->A_Err{
        ++values;++reads;check(pre && time && time->value==0,"saved read is pre-expression at zero");const auto index=reinterpret_cast<std::uintptr_t>(ref)-1;
        if(index==kGraphParameterId)out->val.arbH=reinterpret_cast<AEGP_ArbBlockVal>(saved);
        else out->val.one_d=index==kGraphSyncGuardId?guard:index==kGraphRevisionId?8:index==kGraphChecksumHighId?crc>>16:crc&65535;return 0;};
    streams.AEGP_DisposeStreamValue=[](AEGP_StreamValue2*)->A_Err{--values;return 0;};
    // No write callback is installed: any mutation crashes/fails this probe.
    auto graph=make_emitter_particle_force_output_graph(Settings{},NodeId{uuid(1)},NodeId{uuid(2)},NodeId{uuid(3)},NodeId{uuid(255)},EdgeId{uuid(11)},EdgeId{uuid(12)},EdgeId{uuid(13)}).take_value();
    graph_projection::Issue issue;issue.edge=EdgeId{uuid(14)};issue.source=NodeId{uuid(3)};issue.destination=NodeId{uuid(255)};issue.code=103;
    std::memcpy(issue.detail.data(),"cannot merge different Motion chains",35);graph.optional_records.push_back(graph_projection::record({issue}));save(graph);
    PF_InData data{};PF_OutData out{};data.utils=&utils;data.pica_basicP=&basic;data.effect_ref=reinterpret_cast<PF_ProgPtr>(0x1001);data.num_params=0;
    auto request=std::make_unique<GraphDiagnosticsRequest>();
    const auto reset=[&](){*request={};request->plugin=701;request->renderer=reinterpret_cast<AEGP_EffectRefH>(0x1001);request->revision=8;request->checksum=crc;};
    PF_ParamDef* poisoned[]={reinterpret_cast<PF_ParamDef*>(1)};
    {EffectUiExclusion scope;check(bool(scope),"actual resident UI exclusion");reset();
        check(entry(PF_Cmd_COMPLETELY_GENERAL,&data,&out,poisoned,nullptr,request.get())==0 && request->acknowledged==1 && !request->status && request->count==1,"actual selector reads persisted issue without delivered params");
        check(request->issues[0].edge==issue.edge && request->issues[0].code==103 && !std::strcmp(request->issues[0].detail.data(),issue.detail.data()),"bounded issue identity/code/detail");
        reset();request->revision=9;check(entry(PF_Cmd_COMPLETELY_GENERAL,&data,&out,poisoned,nullptr,request.get())==0 && request->status,"stale revision silently unavailable");
        reset();request->checksum^=1;check(entry(PF_Cmd_COMPLETELY_GENERAL,&data,&out,poisoned,nullptr,request.get())==0 && request->status,"stale CRC silently unavailable");
        reset();guard=1;check(entry(PF_Cmd_COMPLETELY_GENERAL,&data,&out,poisoned,nullptr,request.get())==0 && request->status,"active transaction read silently unavailable");guard=0;
        reset();request->bytes--;check(entry(PF_Cmd_COMPLETELY_GENERAL,&data,&out,poisoned,nullptr,request.get())==0 && !request->acknowledged,"bad private shape silent/no ACK");
        reset();data.utils=nullptr;check(entry(PF_Cmd_COMPLETELY_GENERAL,&data,&out,poisoned,nullptr,request.get())==0 && request->status,"missing PF handle context silent/unavailable");data.utils=&utils;
        graph.optional_records.clear();save(graph);reset();check(entry(PF_Cmd_COMPLETELY_GENERAL,&data,&out,poisoned,nullptr,request.get())==0 && !request->status && request->count==0,"old graph has supported empty diagnostics");
        auto malformed=graph_projection::record({issue});malformed[8]=std::byte{255};graph.optional_records.push_back(malformed);save(graph);reset();check(entry(PF_Cmd_COMPLETELY_GENERAL,&data,&out,poisoned,nullptr,request.get())==0 && request->status,"malformed optional payload unavailable, not empty success");
    }
    reset();check(entry(PF_Cmd_COMPLETELY_GENERAL,&data,&out,poisoned,nullptr,request.get())==0 && request->status,"read requires UI exclusion");
    check(refs==0 && values==0 && suites==0,"all saved handles/streams/suites released");check(out.return_msg[0]==0,"background diagnostics never sets an AE dialog message");
    dispose(saved);check(memory.empty(),"all fake handles released");FreeLibrary(module);FreeLibrary(host);
    std::cout<<checks<<" actual main-AEX diagnostics checks, "<<failures<<" failures; fake May2023 suites, no AE host qualification\n";return failures?1:0;
}
