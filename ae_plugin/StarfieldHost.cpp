#include "AEConfig.h"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include "NativeBootstrap.hpp"
#include "EffectReveal.hpp"
#include "ModelAssetHost.hpp"
#include "ModelTransactionHost.hpp"
#include "PresetFileHost.hpp"
#include "UiExclusionHost.hpp"
#include "GraphCommitRequest.hpp"
#include "GraphRepairRequest.hpp"
#include "WebPanelHost.hpp"
#include "Parameters.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cmath>
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
AEGP_Command model_asset_command{};
AEGP_Command model_transaction_command{};
AEGP_Command preset_file_command{};
AEGP_Command graph_commit_command{};
AEGP_Command graph_repair_command{};
bool graph_commit_running{};
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
        !std::strcmp(match,"org.starfieldfx.node.force") || !std::strcmp(match,"org.starfieldfx.node.transform") ||
        !std::strcmp(match,"org.starfieldfx.node.model") || !std::strcmp(match,"org.starfieldfx.node.material") ||
        !std::strcmp(match,"org.starfieldfx.node.motion") || !std::strcmp(match,"org.starfieldfx.node.turbulence");
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
    if(commands && model_asset_command)(void)commands->AEGP_EnableCommand(model_asset_command);
    if(commands && model_transaction_command)(void)commands->AEGP_EnableCommand(model_transaction_command);
    if(commands && preset_file_command)(void)commands->AEGP_EnableCommand(preset_file_command);
    if(commands && graph_commit_command)(void)commands->AEGP_EnableCommand(graph_commit_command);
    if(commands && graph_repair_command)(void)commands->AEGP_EnableCommand(graph_repair_command);
    return commands && reveal_command?commands->AEGP_EnableCommand(reveal_command):A_Err_NONE;
}
A_Err repair_node_graph(AEGP_GlobalRefcon,AEGP_CommandRefcon,AEGP_Command command,
    AEGP_HookPriority,A_Boolean,A_Boolean* handled) noexcept try {
    using namespace starfield::adapter;
    if(command!=graph_repair_command)return A_Err_NONE;
    if(handled)*handled=TRUE;
    if(stopped || graph_commit_running || std::this_thread::get_id()!=main_thread)return A_Err_GENERIC;
    HostUiExclusion exclusion;if(!exclusion)return A_Err_GENERIC;
    graph_commit_running=true;struct Running{~Running(){graph_commit_running=false;}} running_scope;
    Suite<AEGP_LayerSuite9> layers(kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    Suite<AEGP_EffectSuite4> effects(kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
    Suite<AEGP_UtilitySuite6> utility(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    if(!layers || !effects || !utility)return A_Err_GENERIC;
    AEGP_LayerH layer{};auto error=layers->AEGP_GetActiveLayer(&layer);
    if(error || !layer){utility->AEGP_ReportInfo(plugin_id,"Select the original Starfield layer before Repair Selected Graph.");return error?error:A_Err_GENERIC;}
    A_long count{};error=effects->AEGP_GetLayerNumEffects(layer,&count);if(error || count<1 || count>4224)return error?error:A_Err_GENERIC;
    AEGP_EffectRefH renderer{};struct Ref{const AEGP_EffectSuite4* s;AEGP_EffectRefH& r;~Ref(){if(r)s->AEGP_DisposeEffect(r);}} held{effects.value,renderer};
    for(A_long i=0;i<count;++i){AEGP_EffectRefH fx{};error=effects->AEGP_GetLayerEffectByIndex(plugin_id,layer,i,&fx);
        Ref owned{effects.value,fx};if(error || !fx)return error?error:A_Err_GENERIC;
        AEGP_InstalledEffectKey key{};char match[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
        error=effects->AEGP_GetInstalledKeyFromLayerEffect(fx,&key);if(!error)error=effects->AEGP_GetEffectMatchName(key,match);if(error)return error;
        if(!std::strcmp(match,"org.starfieldfx.particle")){if(renderer)return A_Err_GENERIC;renderer=fx;fx=nullptr;}
    }
    if(!renderer)return A_Err_GENERIC;
    GraphRepairRequest request;request.plugin=plugin_id;request.layer=layer;const A_Time zero{0,1};
    error=effects->AEGP_EffectCallGeneric(plugin_id,renderer,&zero,PF_Cmd_COMPLETELY_GENERAL,&request);
    if(request.message[0])utility->AEGP_ReportInfo(plugin_id,request.message);
    return error?error:request.acknowledged?static_cast<A_Err>(request.status):A_Err_GENERIC;
}catch(...){return A_Err_GENERIC;}
// One synchronous gateway transaction owns guard=1. Search the project rather
// than AE selection (native backups may change selection). Never select the first
// of ambiguous requests, and never execute ExtendScript from this command.
A_Err commit_node_graph(AEGP_GlobalRefcon,AEGP_CommandRefcon,AEGP_Command command,
    AEGP_HookPriority,A_Boolean,A_Boolean* handled) noexcept try {
    using namespace starfield::adapter;
    if(command!=graph_commit_command)return A_Err_NONE;
    if(handled)*handled=TRUE;
    if(stopped || graph_commit_running || SFLD_SourceUiExclusionActiveV1() || std::this_thread::get_id()!=main_thread)return A_Err_GENERIC;
    graph_commit_running=true;
    struct RunningScope{~RunningScope(){graph_commit_running=false;}} running_scope;
    // Model execution already holds the exclusion. Otherwise this scope keeps
    // idle work out while the synchronous generic callback publishes streams.
    HostUiExclusion exclusion;
    Suite<AEGP_ProjSuite6> projects(kAEGPProjSuite,kAEGPProjSuiteVersion6);
    Suite<AEGP_ItemSuite9> items(kAEGPItemSuite,kAEGPItemSuiteVersion9);
    Suite<AEGP_CompSuite11> comps(kAEGPCompSuite,kAEGPCompSuiteVersion11);
    Suite<AEGP_LayerSuite9> layers(kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    Suite<AEGP_EffectSuite4> effects(kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
    Suite<AEGP_StreamSuite6> streams(kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
    if(!projects || !items || !comps || !layers || !effects || !streams)return A_Err_GENERIC;
    GraphCommitRequest request;request.plugin=plugin_id;
    struct Owned {const AEGP_EffectSuite4* suite;AEGP_EffectRefH& effect;
        ~Owned(){if(effect)suite->AEGP_DisposeEffect(effect);}} owned{effects.value,request.renderer};
    const auto number=[&](AEGP_EffectRefH effect,A_long index,const A_Time& time,double& result)->A_Err {
        AEGP_StreamRefH ref{};AEGP_StreamType type{};AEGP_StreamValue2 value{};
        auto error=streams->AEGP_GetNewEffectStreamByIndex(plugin_id,effect,index,&ref);
        if(!error && ref)error=streams->AEGP_GetStreamType(ref,&type);
        if(!error && (!ref || type!=AEGP_StreamType_OneD))error=A_Err_GENERIC;
        if(!error){error=streams->AEGP_GetNewStreamValue(plugin_id,ref,AEGP_LTimeMode_LayerTime,&time,TRUE,&value);
            if(!error){result=value.val.one_d;streams->AEGP_DisposeStreamValue(&value);if(!std::isfinite(result))error=A_Err_GENERIC;}}
        if(ref)streams->AEGP_DisposeStream(ref);return error;
    };
    AEGP_ProjectH project{};AEGP_ItemH item{};A_Time request_time{};unsigned visited{};
    if(projects->AEGP_GetProjectByIndex(0,&project) || !project || items->AEGP_GetFirstProjItem(project,&item))return A_Err_GENERIC;
    while(item) {
        if(++visited>65536)return A_Err_GENERIC;
        AEGP_ItemType type{};
        if(items->AEGP_GetItemType(item,&type))return A_Err_GENERIC;
        if(type==AEGP_ItemType_COMP) {
            AEGP_CompH comp{};A_long count{};
            if(comps->AEGP_GetCompFromItem(item,&comp) || !comp || layers->AEGP_GetCompNumLayers(comp,&count) || count<0)return A_Err_GENERIC;
            for(A_long l=0;l<count;++l) {
                if(++visited>65536)return A_Err_GENERIC;
                AEGP_LayerH layer{};A_long effect_count{};A_Time time{};
                if(layers->AEGP_GetCompLayerByIndex(comp,l,&layer) || !layer || effects->AEGP_GetLayerNumEffects(layer,&effect_count) ||
                    effect_count<0 || layers->AEGP_GetLayerCurrentTime(layer,AEGP_LTimeMode_LayerTime,&time) || !time.scale)return A_Err_GENERIC;
                for(A_long e=0;e<effect_count;++e) {
                    if(++visited>65536)return A_Err_GENERIC;
                    AEGP_EffectRefH effect{};AEGP_InstalledEffectKey key{};A_char match[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
                    if(effects->AEGP_GetLayerEffectByIndex(plugin_id,layer,e,&effect) || !effect)return A_Err_GENERIC;
                    Owned local{effects.value,effect};
                    if(effects->AEGP_GetInstalledKeyFromLayerEffect(effect,&key) || effects->AEGP_GetEffectMatchName(key,match))return A_Err_GENERIC;
                    if(std::strcmp(match,"org.starfieldfx.particle"))continue;
                    double guard{},nonce{},receipt{};
                    if(number(effect,kGraphSyncGuardId,time,guard))return A_Err_GENERIC;
                    if(guard!=1)continue;
                    if(number(effect,kGraphEditCommitId,time,nonce) || number(effect,kGraphEditReceiptId,time,receipt))return A_Err_GENERIC;
                    if(nonce==receipt)continue; // Idle or already published.
                    if(nonce<1 || nonce>1000000 || std::floor(nonce)!=nonce)return A_Err_GENERIC;
                    // Rejection also ends this nonce. A failed rollback may
                    // retain guard=1; it must not poison another renderer's
                    // command or be implicitly retried/unlocked by this scan.
                    if(receipt==-nonce)continue;
                    if(request.renderer)return A_Err_GENERIC;
                    request.renderer=effect;effect=nullptr;request.layer=layer;request.nonce=static_cast<A_long>(nonce);request_time=time;
                }
            }
        }
        AEGP_ItemH next{};if(items->AEGP_GetNextProjItem(project,item,&next))return A_Err_GENERIC;item=next;
    }
    if(!request.renderer)return A_Err_NONE;
    auto error=effects->AEGP_EffectCallGeneric(plugin_id,request.renderer,&request_time,PF_Cmd_COMPLETELY_GENERAL,&request);
    if(!error && (!request.acknowledged || request.status))error=request.status?request.status:A_Err_GENERIC;
    if(error) {
        AEGP_StreamRefH ref{};
        if(!streams->AEGP_GetNewEffectStreamByIndex(plugin_id,request.renderer,kGraphEditReceiptId,&ref) && ref) {
            AEGP_StreamValue2 value{};value.streamH=ref;value.val.one_d=-request.nonce;
            (void)streams->AEGP_SetStreamValue(plugin_id,ref,&value);
        }
        if(ref)streams->AEGP_DisposeStream(ref);
    }
    return error;
} catch(...) {graph_commit_running=false;return A_Err_GENERIC;}
A_Err model_asset_queue(AEGP_GlobalRefcon,AEGP_CommandRefcon,AEGP_Command command,
    AEGP_HookPriority,A_Boolean,A_Boolean* handled) noexcept {
    if(command!=model_asset_command)return A_Err_NONE;
    if(handled)*handled=TRUE;
    starfield::adapter::queue_model_asset_export();return A_Err_NONE;
}
A_Err idle(AEGP_GlobalRefcon,AEGP_IdleRefcon,A_long* sleep) noexcept try {
    if(sleep)*sleep=std::min(*sleep,A_long{30});
    const auto now=std::chrono::steady_clock::now();
    if(stopped || running || std::this_thread::get_id()!=main_thread ||
       !starfield::adapter::ui_exclusion_available())return A_Err_NONE;
    running=true;struct RunningScope{~RunningScope(){running=false;}} scope;
    if(starfield::adapter::step_web_panel_host() && sleep)*sleep=std::min(*sleep,A_long{1});
    if(starfield::adapter::step_preset_file_host() && sleep)*sleep=std::min(*sleep,A_long{1});
    if(starfield::adapter::step_model_asset_host() && sleep)*sleep=std::min(*sleep,A_long{1});
    if(starfield::adapter::step_model_transaction_host() && sleep)*sleep=std::min(*sleep,A_long{1});
    starfield::adapter::HostUiExclusion idle_scope;if(!idle_scope)return A_Err_NONE;
    if(now<next_scan)return A_Err_NONE;
    next_scan=now+std::chrono::milliseconds(500);
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
A_Err model_transaction_queue(AEGP_GlobalRefcon,AEGP_CommandRefcon,AEGP_Command command,
    AEGP_HookPriority,A_Boolean,A_Boolean* handled) noexcept {
    if(command!=model_transaction_command)return A_Err_NONE;
    if(handled)*handled=TRUE;
    starfield::adapter::queue_model_graph_transaction();return A_Err_NONE;
}
A_Err death(AEGP_GlobalRefcon,AEGP_DeathRefcon) noexcept {
    stopped=true;starfield::adapter::stop_web_panel_host();
    starfield::adapter::stop_model_asset_host();starfield::adapter::stop_model_transaction_host();
    starfield::adapter::stop_preset_file_host();
    starfield::adapter::stop_ui_exclusion();return A_Err_NONE;
}
A_Err preset_file_queue(AEGP_GlobalRefcon,AEGP_CommandRefcon,AEGP_Command command,
    AEGP_HookPriority,A_Boolean,A_Boolean* handled) noexcept {
    if(command!=preset_file_command)return A_Err_NONE;
    if(handled)*handled=TRUE;
    starfield::adapter::queue_preset_file_dialog();return A_Err_NONE;
}
}
extern "C" __declspec(dllexport) A_Err StarfieldHostEntry(SPBasicSuite* suites,A_long,A_long,
    AEGP_PluginID id,AEGP_GlobalRefcon* refcon) noexcept {
    basic=suites;plugin_id=id;main_thread=std::this_thread::get_id();
    starfield::adapter::initialize_ui_exclusion();
    starfield::adapter::initialize_model_asset_host(suites,id);
    starfield::adapter::initialize_model_transaction_host(suites,id);
    starfield::adapter::initialize_preset_file_host(suites,id);
    if(refcon)*refcon=nullptr;
    Suite<AEGP_RegisterSuite5> registrations(kAEGPRegisterSuite,kAEGPRegisterSuiteVersion5);
    if(!registrations)return A_Err_GENERIC;
    auto error=registrations->AEGP_RegisterDeathHook(id,death,nullptr);
    if(!error)error=registrations->AEGP_RegisterIdleHook(id,idle,nullptr);
    // Once a hook is registered the module must stay resident, even if a later
    // registration fails. This optional helper never rejects AE startup.
    if(error)stopped=true;
    if(!error)starfield::adapter::initialize_web_panel_host(suites,id);
    // Session-resident AEGP owns UI commands. Navigation writes no trigger
    // parameters and does not dirty the graph or invalidate rendered frames.
    Suite<AEGP_CommandSuite1> commands(kAEGPCommandSuite,kAEGPCommandSuiteVersion1);
    if(!error && commands && !commands->AEGP_GetUniqueCommand(&reveal_command) &&
       !registrations->AEGP_RegisterCommandHook(id,AEGP_HP_BeforeAE,reveal_command,reveal_selected,nullptr) &&
       !commands->AEGP_InsertMenuCommand(reveal_command,reveal_name,AEGP_Menu_WINDOW,AEGP_MENU_INSERT_AT_BOTTOM)) {
        (void)commands->AEGP_EnableCommand(reveal_command);
    }
    if(!error && commands && !commands->AEGP_GetUniqueCommand(&model_asset_command) &&
       !registrations->AEGP_RegisterCommandHook(id,AEGP_HP_BeforeAE,model_asset_command,model_asset_queue,nullptr) &&
       !commands->AEGP_InsertMenuCommand(model_asset_command,starfield::adapter::model_asset_command_name,AEGP_Menu_WINDOW,AEGP_MENU_INSERT_AT_BOTTOM))
        (void)commands->AEGP_EnableCommand(model_asset_command);
    if(!error && commands && !commands->AEGP_GetUniqueCommand(&model_transaction_command) &&
       !registrations->AEGP_RegisterCommandHook(id,AEGP_HP_BeforeAE,model_transaction_command,model_transaction_queue,nullptr) &&
       !commands->AEGP_InsertMenuCommand(model_transaction_command,starfield::adapter::model_transaction_command_name,AEGP_Menu_WINDOW,AEGP_MENU_INSERT_AT_BOTTOM))
        (void)commands->AEGP_EnableCommand(model_transaction_command);
    if(!error && commands && !commands->AEGP_GetUniqueCommand(&preset_file_command) &&
       !registrations->AEGP_RegisterCommandHook(id,AEGP_HP_BeforeAE,preset_file_command,preset_file_queue,nullptr) &&
       !commands->AEGP_InsertMenuCommand(preset_file_command,starfield::adapter::preset_file_command_name,AEGP_Menu_WINDOW,AEGP_MENU_INSERT_AT_BOTTOM))
        (void)commands->AEGP_EnableCommand(preset_file_command);
    if(!error && commands && !commands->AEGP_GetUniqueCommand(&graph_commit_command) &&
       !registrations->AEGP_RegisterCommandHook(id,AEGP_HP_BeforeAE,graph_commit_command,commit_node_graph,nullptr) &&
       !commands->AEGP_InsertMenuCommand(graph_commit_command,starfield::adapter::graph_commit_command_name,AEGP_Menu_WINDOW,AEGP_MENU_INSERT_AT_BOTTOM))
        (void)commands->AEGP_EnableCommand(graph_commit_command);
    if(!error && commands && !commands->AEGP_GetUniqueCommand(&graph_repair_command) &&
       !registrations->AEGP_RegisterCommandHook(id,AEGP_HP_BeforeAE,graph_repair_command,repair_node_graph,nullptr) &&
       !commands->AEGP_InsertMenuCommand(graph_repair_command,starfield::adapter::graph_repair_command_name,AEGP_Menu_WINDOW,AEGP_MENU_INSERT_AT_BOTTOM))
        (void)commands->AEGP_EnableCommand(graph_repair_command);
    if(!error && commands && (reveal_command || model_asset_command || model_transaction_command || preset_file_command || graph_commit_command || graph_repair_command))
        (void)registrations->AEGP_RegisterUpdateMenuHook(id,reveal_menu,nullptr);
    return A_Err_NONE;
}
