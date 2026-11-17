#include "NodeEffects.hpp"
#include "NodeRecord.hpp"
#include <Windows.h>
#include <map>
#include <vector>
#include <string>
#include <cstdio>
namespace {
unsigned checks=0,failures=0,registrations=0;
std::vector<PF_ParamDef> defs;
std::map<PF_Handle,std::vector<char>> handles;
void check(bool ok,const char* message){++checks;if(!ok){++failures;std::printf("FAILED: %s\n",message);}}
PF_Handle allocate(A_u_longlong n){auto h=new void*{};handles[h].resize(static_cast<size_t>(n));return h;}
void* lock(PF_Handle h){return handles.at(h).data();}
void unlock(PF_Handle){}
void dispose(PF_Handle h){handles.erase(h);delete h;}
A_u_longlong size_of(PF_Handle h){return handles.at(h).size();}
PF_Err add(PF_ProgPtr,PF_ParamIndex,PF_ParamDef* def){defs.push_back(*def);return 0;}
PF_Err register_ui(PF_ProgPtr,PF_CustomUIInfo* info){++registrations;check(info && info->events==PF_CustomEFlag_EFFECT,"custom locator/editor UI is registered");return 0;}
}
int main(int argc,char** argv){
    if(argc!=2)return 2;
    PF_UtilCallbacks utils{};utils.host_new_handle=allocate;utils.host_lock_handle=lock;
    utils.host_unlock_handle=unlock;utils.host_dispose_handle=dispose;utils.host_get_handle_size=size_of;
    using Entry=PF_Err(*)(PF_Cmd,PF_InData*,PF_OutData*,PF_ParamDef*[],PF_LayerDef*,void*);
    using namespace starfield::adapter::native_nodes;
    const char* modules[]={"Emitter","ParticleNode","Force","Transform","Model","Material","Motion","Turbulence"};
    const Kind kinds[]={Kind::emitter,Kind::particle,Kind::force,Kind::transform,Kind::model,Kind::material,Kind::motion,Kind::turbulence};
    for(unsigned k=0;k<8;++k){
        defs.clear();registrations=0;
        const auto file=std::string(argv[1])+"/Starfield"+modules[k]+".aex";
        auto module=LoadLibraryA(file.c_str());check(module!=nullptr,"load actual candidate AEX");
        if(!module){std::printf("LoadLibrary error %lu: %s\n",GetLastError(),file.c_str());continue;}
        auto entry=reinterpret_cast<Entry>(GetProcAddress(module,"EffectMain"));check(entry!=nullptr,"candidate exports EffectMain");
        if(!entry){FreeLibrary(module);continue;}
        PF_InData data{};PF_OutData out{};data.utils=&utils;data.inter.add_param=add;data.inter.register_ui=register_ui;
        check(entry(PF_Cmd_PARAMS_SETUP,&data,&out,nullptr,nullptr,nullptr)==0,"actual candidate registers controls");
        check(out.num_params==parameter_count(kinds[k]) && defs.size()+1==static_cast<size_t>(out.num_params),"candidate stream count matches current contract");
        check(registrations==1,"each candidate registers custom editors/locator exactly once");
        std::vector<PF_ParamDef*> delivered(out.num_params);PF_ParamDef input{};delivered[0]=&input;
        for(size_t i=0;i<defs.size();++i)delivered[i+1]=&defs[i];data.num_params=out.num_params;
        unsigned selectors=0;
        for(size_t i=0;i<defs.size();++i){const auto& def=defs[i];
            if(def.param_type!=PF_Param_POPUP && def.param_type!=PF_Param_LAYER)continue;
            ++selectors;check(def.ui_flags==PF_PUI_NONE && def.ui_width==0 && def.ui_height==0,"ordinary selector uses the same unmodified native registration as Particle Color/Up Axis/texture layers");
            check((def.flags&PF_ParamFlag_SUPERVISE)!=0 || (def.flags&PF_ParamFlag_CANNOT_TIME_VARY)!=0,"selector preserves supervision/structural contract");
            for(auto area:{PF_EA_CONTROL,PF_EA_PARAM_TITLE})for(auto type:{PF_Event_DRAW,PF_Event_DO_CLICK,PF_Event_DRAG}){
                PF_EventExtra event{};event.e_type=type;event.contextH=reinterpret_cast<PF_ContextH>(1);
                event.effect_win.index=static_cast<A_long>(i+1);event.effect_win.area=area;
                check(entry(PF_Cmd_EVENT,&data,&out,delivered.data(),nullptr,&event)==0 && !event.evt_out_flags,"candidate delegates the complete native selector row, including its inline title/value, to AE");
            }
            std::printf("  %s index%zu disk%ld native selector\n",def.name,i+1,static_cast<long>(def.uu.id));
        }
        std::printf("%s: %u native selectors; %zu controls\n",modules[k],selectors,defs.size());
        while(!handles.empty())dispose(handles.begin()->first);
        check(FreeLibrary(module)!=0,"unload isolated candidate module");
    }
    std::printf("All-node native selector audit: %u checks, %u failures\n",checks,failures);return failures?1:0;
}