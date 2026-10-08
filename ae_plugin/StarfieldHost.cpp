#include "AEConfig.h"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include "NativeBootstrap.hpp"
#include "EffectReveal.hpp"
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
AEGP_Command reveal_command{};
constexpr char reveal_name[]="Starfield Reveal Selected Effect";
template<class T> struct Suite {
    const char* name;A_long version;const T* value{};
    Suite(const char* n,A_long v):name(n),version(v) {
        if(basic)(void)basic->AcquireSuite(n,v,reinterpret_cast<const void**>(&value));
    }
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
    const T* operator->() const {return value;}
    explicit operator bool() const {return value!=nullptr;}
};
bool starfield_effect(const char* match) noexcept {
    return !std::strcmp(match,"org.starfieldfx.particle") ||
        !std::strcmp(match,"org.starfieldfx.node.emitter") || !std::strcmp(match,"org.starfieldfx.node.particle") ||
        !std::strcmp(match,"org.starfieldfx.node.force") || !std::strcmp(match,"org.starfieldfx.node.transform");
}
A_Err reveal_selected(AEGP_GlobalRefcon,AEGP_CommandRefcon,AEGP_Command command,
    AEGP_HookPriority,A_Boolean,A_Boolean* handled) noexcept try {
    if(command!=reveal_command)return A_Err_NONE;
    if(handled)*handled=TRUE;
    if(stopped || std::this_thread::get_id()!=main_thread)return A_Err_NONE;
    Suite<AEGP_ItemSuite9> items(kAEGPItemSuite,kAEGPItemSuiteVersion9);
    Suite<AEGP_CompSuite11> comps(kAEGPCompSuite,kAEGPCompSuiteVersion11);
    Suite<AEGP_CollectionSuite2> collections(kAEGPCollectionSuite,kAEGPCollectionSuiteVersion2);
    Suite<AEGP_EffectSuite4> effects(kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
    Suite<AEGP_StreamSuite6> streams(kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
    Suite<AEGP_DynamicStreamSuite4> dynamic(kAEGPDynamicStreamSuite,kAEGPDynamicStreamSuiteVersion4);
    if(!items || !comps || !collections || !effects || !streams || !dynamic)return A_Err_NONE;
    AEGP_ItemH active{};AEGP_ItemType type{};AEGP_CompH comp{};AEGP_Collection2H selection{};
    if(items->AEGP_GetActiveItem(&active) || !active || items->AEGP_GetItemType(active,&type) ||
       type!=AEGP_ItemType_COMP || comps->AEGP_GetCompFromItem(active,&comp) || !comp ||
       comps->AEGP_GetNewCollectionFromCompSelection(plugin_id,comp,&selection) || !selection)return A_Err_NONE;
    struct CollectionScope {const AEGP_CollectionSuite2* suite;AEGP_Collection2H value;
        ~CollectionScope(){suite->AEGP_DisposeCollection(value);}} owned{collections.value,selection};
    A_u_long count{};
    if(collections->AEGP_GetCollectionNumItems(selection,&count) || count>1024)return A_Err_NONE;
    for(A_u_long i=0;i<count;++i) {
        AEGP_CollectionItemV2 item{};
        if(collections->AEGP_GetCollectionItemByIndex(selection,i,&item))continue;
        AEGP_EffectCollectionItem selected{};
        if(item.type==AEGP_CollectionItemType_EFFECT)selected=item.u.effect;
        else if(item.type==AEGP_CollectionItemType_STREAM && item.u.stream.type==AEGP_StreamCollectionItemType_EFFECT)
            selected=item.u.stream.u.effect_stream.effect;
        if(selected.layerH) {
            AEGP_EffectRefH effect{};AEGP_InstalledEffectKey key{};A_char match[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
            if(effects->AEGP_GetLayerEffectByIndex(plugin_id,selected.layerH,selected.index,&effect) || !effect)continue;
            struct EffectScope {const AEGP_EffectSuite4* suite;AEGP_EffectRefH value;
                ~EffectScope(){suite->AEGP_DisposeEffect(value);}} effect_scope{effects.value,effect};
            if(effects->AEGP_GetInstalledKeyFromLayerEffect(effect,&key) || effects->AEGP_GetEffectMatchName(key,match) ||
                !starfield_effect(match))continue;
            AEGP_StreamRefH control{};
            if(streams->AEGP_GetNewEffectStreamByIndex(plugin_id,effect,
                !std::strcmp(match,"org.starfieldfx.particle")?26:1,&control) || !control)continue;
            const auto result=starfield::adapter::reveal_stream(dynamic.value,control);
            streams->AEGP_DisposeStream(control);return result;
        }
        // Selection collections can represent a selected effect by its group ref.
        if(item.type==AEGP_CollectionItemType_STREAMREF && item.stream_refH) {
            A_char match[AEGP_MAX_STREAM_MATCH_NAME_SIZE]{};
            if(dynamic->AEGP_GetMatchName(item.stream_refH,match) || !starfield_effect(match))continue;
            A_long children{};
            if(dynamic->AEGP_GetNumStreamsInGroup(item.stream_refH,&children) || children<0 || children>1024)continue;
            for(A_long child=0;child<children;++child) {
                AEGP_StreamRefH control{};AEGP_DynStreamFlags flags{};
                if(dynamic->AEGP_GetNewStreamRefByIndex(plugin_id,item.stream_refH,child,&control) || !control)continue;
                const bool visible=!dynamic->AEGP_GetDynamicStreamFlags(control,&flags) &&
                    !(flags&(AEGP_DynStreamFlag_HIDDEN|AEGP_DynStreamFlag_ELIDED|AEGP_DynStreamFlag_SKIP_REVEAL_WHEN_UNHIDDEN));
                const auto result=visible?starfield::adapter::reveal_stream(dynamic.value,control):A_Err_NONE;
                streams->AEGP_DisposeStream(control);
                if(visible)return result;
            }
        }
    }
    return A_Err_NONE;
} catch(...) {return A_Err_NONE;}
A_Err reveal_menu(AEGP_GlobalRefcon,AEGP_UpdateMenuRefcon,AEGP_WindowType) noexcept {
    Suite<AEGP_CommandSuite1> commands(kAEGPCommandSuite,kAEGPCommandSuiteVersion1);
    return commands && reveal_command?commands->AEGP_EnableCommand(reveal_command):A_Err_NONE;
}
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
    // Session-resident AEGP owns UI commands. Navigation writes no trigger
    // parameters and does not dirty the graph or invalidate rendered frames.
    Suite<AEGP_CommandSuite1> commands(kAEGPCommandSuite,kAEGPCommandSuiteVersion1);
    if(!error && commands && !commands->AEGP_GetUniqueCommand(&reveal_command) &&
       !registrations->AEGP_RegisterCommandHook(id,AEGP_HP_BeforeAE,reveal_command,reveal_selected,nullptr) &&
       !commands->AEGP_InsertMenuCommand(reveal_command,reveal_name,AEGP_Menu_WINDOW,AEGP_MENU_INSERT_AT_BOTTOM)) {
        (void)commands->AEGP_EnableCommand(reveal_command);
        (void)registrations->AEGP_RegisterUpdateMenuHook(id,reveal_menu,nullptr);
    }
    return A_Err_NONE;
}
