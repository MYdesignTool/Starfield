#include "AEConfig.h"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include "NativeBootstrap.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>

// A separate General AEGP owns its hooks for AE's entire session. An effect DLL
// may be unloaded; it must never register a callback whose code could disappear.
namespace {
SPBasicSuite* basic{};
AEGP_PluginID plugin_id{};
std::thread::id main_thread;
std::chrono::steady_clock::time_point next_scan{};
A_long item_id{},layer_index{},effect_index{};
bool running{},stopped{};
template<class T> struct Suite {
    const char* name;A_long version;const T* value{};
    Suite(const char* n,A_long v):name(n),version(v) {
        if(basic)(void)basic->AcquireSuite(n,v,reinterpret_cast<const void**>(&value));
    }
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
    const T* operator->() const {return value;}
    explicit operator bool() const {return value!=nullptr;}
};
A_Err idle(AEGP_GlobalRefcon,AEGP_IdleRefcon,A_long* sleep) noexcept try {
    if(sleep)*sleep=std::min(*sleep,A_long{30});
    const auto now=std::chrono::steady_clock::now();
    if(stopped || running || std::this_thread::get_id()!=main_thread || now<next_scan)return A_Err_NONE;
    next_scan=now+std::chrono::milliseconds(500);
    running=true;struct RunningScope{~RunningScope(){running=false;}} scope;
    Suite<AEGP_ItemSuite9> items(kAEGPItemSuite,kAEGPItemSuiteVersion9);
    Suite<AEGP_CompSuite11> comps(kAEGPCompSuite,kAEGPCompSuiteVersion11);
    Suite<AEGP_LayerSuite9> layers(kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    Suite<AEGP_EffectSuite4> effects(kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
    if(!items || !comps || !layers || !effects)return A_Err_NONE;
    AEGP_ItemH active{};AEGP_ItemType type=AEGP_ItemType_NONE;A_long current_id{};
    if(items->AEGP_GetActiveItem(&active) || !active || items->AEGP_GetItemType(active,&type) ||
        type!=AEGP_ItemType_COMP || items->AEGP_GetItemID(active,&current_id))return A_Err_NONE;
    if(current_id!=item_id){item_id=current_id;layer_index=effect_index=0;}
    AEGP_CompH comp{};A_long count{};
    if(comps->AEGP_GetCompFromItem(active,&comp) || !comp ||
        layers->AEGP_GetCompNumLayers(comp,&count) || count<0)return A_Err_NONE;
    unsigned visited{},messages{};
    // Only numeric cursors survive callbacks. Every layer/effect is reacquired;
    // deletion, reordering, undo and project close cannot leave a dangling ref.
    while(layer_index<count && visited<64 && messages<4 &&
        std::chrono::steady_clock::now()-now<std::chrono::milliseconds(250)) {
        AEGP_LayerH layer{};A_long effect_count{};
        ++visited;
        if(layers->AEGP_GetCompLayerByIndex(comp,layer_index,&layer) || !layer ||
            effects->AEGP_GetLayerNumEffects(layer,&effect_count) || effect_count<0) {
            ++layer_index;effect_index=0;continue;
        }
        if(effect_index>=effect_count){++layer_index;effect_index=0;continue;}
        AEGP_EffectRefH effect{};
        const auto index=effect_index++;
        if(effects->AEGP_GetLayerEffectByIndex(plugin_id,layer,index,&effect) || !effect)continue;
        struct OwnedEffect {const AEGP_EffectSuite4* suite;AEGP_EffectRefH value;
            ~OwnedEffect(){suite->AEGP_DisposeEffect(value);}} owned{effects.value,effect};
        AEGP_InstalledEffectKey key{};A_char match[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
        if(effects->AEGP_GetInstalledKeyFromLayerEffect(effect,&key) ||
            effects->AEGP_GetEffectMatchName(key,match) || std::strcmp(match,"org.starfieldfx.particle")!=0)continue;
        A_Time time{0,1};
        (void)layers->AEGP_GetLayerCurrentTime(layer,AEGP_LTimeMode_LayerTime,&time);
        if(!time.scale)time={0,1};
        starfield::adapter::NativeBootstrapRequest request;
        ++messages;
        // The receiver owns its own PF context and suites. Failure is optional;
        // never borrow callbacks or force a write into the authored project.
        (void)effects->AEGP_EffectCallGeneric(plugin_id,effect,&time,PF_Cmd_COMPLETELY_GENERAL,&request);
    }
    if(layer_index>=count)layer_index=effect_index=0;
    return A_Err_NONE;
} catch(...) {running=false;return A_Err_NONE;}
A_Err death(AEGP_GlobalRefcon,AEGP_DeathRefcon) noexcept {stopped=true;return A_Err_NONE;}
}
extern "C" __declspec(dllexport) A_Err StarfieldHostEntry(SPBasicSuite* suites,A_long,A_long,
    AEGP_PluginID id,AEGP_GlobalRefcon* refcon) noexcept {
    basic=suites;plugin_id=id;main_thread=std::this_thread::get_id();
    if(refcon)*refcon=nullptr;
    Suite<AEGP_RegisterSuite5> registrations(kAEGPRegisterSuite,kAEGPRegisterSuiteVersion5);
    if(!registrations)return A_Err_GENERIC;
    auto error=registrations->AEGP_RegisterDeathHook(id,death,nullptr);
    if(!error)error=registrations->AEGP_RegisterIdleHook(id,idle,nullptr);
    // Once a hook is registered the module must stay resident, even if a later
    // registration fails. This optional helper never rejects AE startup.
    if(error)stopped=true;
    return A_Err_NONE;
}
