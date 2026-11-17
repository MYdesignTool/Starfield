#include "AEConfig.h"
#include "AE_Effect.h"
#include <windows.h>
#include <filesystem>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdint>
namespace {
unsigned checks{},failures{};
void check(bool ok,const char* text){++checks;if(!ok){++failures;std::printf("FAIL: %s\n",text);}}
struct Resource{HRSRC value{};unsigned count{};};
BOOL CALLBACK resource(HMODULE module,LPCWSTR type,LPWSTR name,LONG_PTR context){auto& result=*reinterpret_cast<Resource*>(context);++result.count;result.value=FindResourceW(module,name,type);return TRUE;}
std::uint32_t read(const unsigned char* bytes){std::uint32_t value{};std::memcpy(&value,bytes,4);return value;}
}
int wmain(int argc,wchar_t** argv){
    std::printf("May2023 selectors: RENDER=%d SMART_PRE_RENDER=%d SMART_RENDER=%d SMART_RENDER_GPU=%d\n",int(PF_Cmd_RENDER),int(PF_Cmd_SMART_PRE_RENDER),int(PF_Cmd_SMART_RENDER),int(PF_Cmd_SMART_RENDER_GPU));
    if(argc!=4)return 2;
    const bool threaded=std::wstring(argv[2])==L"on";const auto version=unsigned(std::stoul(argv[3]));
    const wchar_t* names[]={L"Particle",L"Emitter",L"ParticleNode",L"Force",L"Transform",L"Model",L"Material",L"Motion",L"Turbulence"};
    using Entry=PF_Err(*)(PF_Cmd,PF_InData*,PF_OutData*,PF_ParamDef*[],PF_LayerDef*,void*);
    for(auto name:names){
        const auto path=std::filesystem::absolute(std::filesystem::path(argv[1])/(std::wstring(L"Starfield")+name+L".aex"));
        const auto module=LoadLibraryW(path.c_str());check(module!=nullptr,"actual AEX loads in isolated selector fixture");if(!module)continue;
        const auto entry=reinterpret_cast<Entry>(GetProcAddress(module,"EffectMain"));check(entry!=nullptr,"actual entry point exported");
        PF_InData in{};PF_OutData out{};
        if(entry){check(entry(PF_Cmd_GLOBAL_SETUP,&in,&out,nullptr,nullptr,nullptr)==0,"actual GLOBAL_SETUP succeeds without host registration suites");
            check(bool(out.out_flags2&PF_OutFlag2_SUPPORTS_THREADED_RENDERING)==threaded,"runtime MFR capability matches requested configuration");
            check(unsigned(out.my_version)==version,"actual runtime packed version");}
        Resource found;check(EnumResourceNamesW(module,L"PiPL",resource,reinterpret_cast<LONG_PTR>(&found))!=0 && found.count==1,"one embedded compiled PiPL resource");
        if(found.value){
            const auto length=std::size_t(SizeofResource(module,found.value));const auto loaded=LoadResource(module,found.value);
            const auto* bytes=static_cast<const unsigned char*>(LockResource(loaded));check(bytes && length>=10,"compiled resource header");
            if(bytes && length>=10){
                const auto count=read(bytes+6);std::size_t offset=10;unsigned flags{},flags2{},versions{};
                check(count>0 && count<100,"bounded PiPL property count");
                for(unsigned i=0;i<count && i<100;++i){
                    check(offset+16<=length,"PiPL property header bounds");if(offset+16>length)break;
                    const auto* key=bytes+offset+4;const auto size=std::size_t(read(bytes+offset+12));offset+=16;
                    check(size<=length-offset,"PiPL property value bounds");if(size>length-offset)break;
                    if(std::memcmp(key,"OLGe",4)==0){++flags;check(size==4 && read(bytes+offset)==unsigned(out.out_flags),"embedded out_flags equals actual GLOBAL_SETUP");}
                    if(std::memcmp(key,"2LGe",4)==0){++flags2;check(size==4 && read(bytes+offset)==unsigned(out.out_flags2),"embedded out_flags2 equals actual GLOBAL_SETUP");}
                    if(std::memcmp(key,"REVe",4)==0){++versions;check(size==4 && read(bytes+offset)==version,"embedded packed version equals actual runtime");}
                    offset+=(size+3)&~std::size_t(3);
                }
                check(flags==1 && flags2==1 && versions==1,"unique embedded flags and version properties");
            }
        }
        check(FreeLibrary(module)!=0,"isolated module unload");
    }
    std::printf("%u actual AEX PiPL/runtime MFR checks, %u failures\n",checks,failures);return failures?1:0;
}
