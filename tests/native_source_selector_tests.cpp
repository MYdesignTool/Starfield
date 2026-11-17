#include "NativeSourceState.hpp"
#include "SPBasic.h"
#include <Windows.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <filesystem>
using namespace starfield::adapter;
namespace {
unsigned checks{},failures{},state_calls{},compare_calls{},opened{},disposed{};
const char* current_match{};
void check(bool ok,const char* message){++checks;if(!ok){++failures;std::printf("FAILED: %s\n",message);}}
AEGP_PFInterfaceSuite1 pf{};AEGP_EffectSuite4 effects{};AEGP_StreamSuite6 streams{};PF_ParamUtilsSuite3 params{};
SPErr acquire(const char* name,int32,const void** out){
    if(!std::strcmp(name,kAEGPPFInterfaceSuite))*out=&pf;
    else if(!std::strcmp(name,kAEGPEffectSuite))*out=&effects;
    else if(!std::strcmp(name,kAEGPStreamSuite))*out=&streams;
    else if(!std::strcmp(name,kPFParamUtilsSuite))*out=&params;
    else return kSPBadParameterError;return 0;
}
SPErr release(const char*,int32){return 0;}
}
int wmain(int argc,wchar_t** argv){
    if(argc!=3){std::printf("Expected native bundle and isolated Host paths.\n");return 2;}
    const auto host=LoadLibraryW(argv[2]);check(host!=nullptr,"load isolated resident Host fixture");if(!host)return 2;
    const auto initialize=reinterpret_cast<void(*)()noexcept>(GetProcAddress(host,"SFLD_TestInitialize"));check(initialize!=nullptr,"initialize actual exclusion implementation");initialize();
    pf.AEGP_GetNewEffectForEffect=[](AEGP_PluginID,PF_ProgPtr context,AEGP_EffectRefH* out)->A_Err{*out=reinterpret_cast<AEGP_EffectRefH>(context);return 0;};
    effects.AEGP_GetInstalledKeyFromLayerEffect=[](AEGP_EffectRefH,AEGP_InstalledEffectKey* key)->A_Err{*key=1;return 0;};
    effects.AEGP_GetEffectMatchName=[](AEGP_InstalledEffectKey,A_char* name)->A_Err{std::strcpy(name,current_match);return 0;};
    effects.AEGP_DisposeEffect=[](AEGP_EffectRefH)->A_Err{return 0;};
    streams.AEGP_GetNewEffectStreamByIndex=[](AEGP_PluginID,AEGP_EffectRefH,A_long,AEGP_StreamRefH* out)->A_Err{++opened;*out=reinterpret_cast<AEGP_StreamRefH>(1);return 0;};
    streams.AEGP_GetUniqueStreamID=[](AEGP_StreamRefH,A_long* out)->A_Err{*out=1234;return 0;};
    streams.AEGP_DisposeStream=[](AEGP_StreamRefH)->A_Err{++disposed;return 0;};
    params.PF_GetCurrentState=[](PF_ProgPtr,PF_ParamIndex index,const A_Time* start,const A_Time* duration,PF_State* state)->PF_Err{
        ++state_calls;check(index==PF_ParamIndex_CHECK_ALL_EXCEPT_LAYER_PARAMS && !start && !duration,"state query includes all numeric times");*state={};return 0;};
    params.PF_AreStatesIdentical=[](PF_ProgPtr,const PF_State*,const PF_State*,A_Boolean* same)->PF_Err{++compare_calls;*same=TRUE;return 0;};
    SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
    using Entry=PF_Err(*)(PF_Cmd,PF_InData*,PF_OutData*,PF_ParamDef*[],PF_LayerDef*,void*);
    struct Node{const wchar_t* file;const char* match;unsigned kind;bool eligible;};
    const Node nodes[]={{L"Emitter","emitter",0,true},{L"ParticleNode","particle",1,true},{L"Force","force",3,true},{L"Turbulence","turbulence",8,true},
        {L"Transform","transform",4,false},{L"Model","model",5,false},{L"Material","material",6,false},{L"Motion","motion",7,false}};
    for(const auto& node:nodes){
        const auto path=(std::filesystem::path(argv[1])/(std::wstring(L"Starfield")+node.file+L".aex")).wstring();
        const auto module=LoadLibraryW(path.c_str());check(module!=nullptr,"load frozen node candidate AEX");if(!module)continue;
        const auto entry=reinterpret_cast<Entry>(GetProcAddress(module,"EffectMain"));check(entry!=nullptr,"actual EffectMain exists");if(!entry){FreeLibrary(module);continue;}
        const auto match=std::string("org.starfieldfx.node.")+node.match;current_match=match.c_str();
        PF_InData data{};PF_OutData out{};data.pica_basicP=&basic;data.effect_ref=reinterpret_cast<PF_ProgPtr>(0x1001);data.num_params=0;
        NativeSourceStateRequest request;request.plugin=701;request.identity_stream=1234;request.kind=node.kind;request.compare=1;
        const auto before_states=state_calls;
        {EffectUiExclusion owner(true);check(bool(owner),"actual source exclusion acquired");
        const auto error=entry(PF_Cmd_COMPLETELY_GENERAL,&data,&out,nullptr,nullptr,&request);
        std::printf("SOURCE %s: nativeKind=%u err=%ld ack=%u supported=%u same=%u stateCalls=%u\n",node.match,node.kind,long(error),request.acknowledged,request.supported,request.same,state_calls-before_states);
        check(error==0 && request.acknowledged==1,"private selector is silent and acknowledges");
        check(request.supported==static_cast<unsigned>(node.eligible),"actual node selector capability matches native_nodes::Kind whitelist");
        check(request.same==static_cast<unsigned>(node.eligible),"actual selector returns successful SDK comparison for supported source");
        request={};request.plugin=701;request.identity_stream=1234;request.kind=node.kind;
        check(entry(PF_Cmd_COMPLETELY_GENERAL,nullptr,&out,nullptr,nullptr,&request)==0 && request.acknowledged && !request.supported,
            "actual selector treats absent PF context as silent unsupported");
        request={};request.plugin=701;request.identity_stream=1235;request.kind=node.kind;
        check(entry(PF_Cmd_COMPLETELY_GENERAL,&data,&out,nullptr,nullptr,&request)==0 && !request.supported,
            "actual selector rejects wrong source stream identity");}
        request={};request.plugin=701;request.identity_stream=1234;request.kind=node.kind;
        check(entry(PF_Cmd_COMPLETELY_GENERAL,&data,&out,nullptr,nullptr,&request)==0 && !request.supported,
            "actual selector does not probe outside source scope");
        check(FreeLibrary(module)!=0,"unload source candidate");
    }
    check(opened==disposed,"source selector disposes every UUID stream");
    check(FreeLibrary(host)!=0,"unload isolated Host");
    std::printf("Actual source selector probe: %u checks, %u failures; GetState=%u Compare=%u\n",checks,failures,state_calls,compare_calls);
    return failures?1:0;
}
