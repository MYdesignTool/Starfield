#include "NativeTemporalUI.hpp"
#include "NativeTemporalCache.hpp"
#include "GraphParameter.hpp"
#include "SPBasic.h"
#include <algorithm>
#include <chrono>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace starfield::adapter {
namespace {
struct UIStamp {PF_ProgPtr publisher{};PF_State state{};};
std::mutex ui_mutex;
std::thread::id ui_thread;
std::vector<UIStamp> ui_stamps;
NativeUITiming ui_timing;
struct UITimer {
    bool refreshed{};
    std::chrono::steady_clock::time_point start{std::chrono::steady_clock::now()};
    ~UITimer() {
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        std::lock_guard lock(ui_mutex);++ui_timing.calls;
        ui_timing.refreshes+=refreshed;ui_timing.last_ms=ms;ui_timing.max_ms=std::max(ui_timing.max_ms,ms);
    }
};
thread_local bool refreshing{};
struct RefreshScope {
    RefreshScope() {refreshing=true;}
    ~RefreshScope() {refreshing=false;}
};
struct ParamUtils {
    PF_InData* data;const PF_ParamUtilsSuite3* suite{};
    explicit ParamUtils(PF_InData* d):data(d) {
        (void)d->pica_basicP->AcquireSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3,reinterpret_cast<const void**>(&suite));
    }
    ~ParamUtils() {if(suite)data->pica_basicP->ReleaseSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3);}
    bool state(PF_State& out) const {
        return suite && suite->PF_GetCurrentState && suite->PF_AreStatesIdentical &&
            suite->PF_GetCurrentState(data->effect_ref,PF_ParamIndex_CHECK_ALL_EXCEPT_LAYER_PARAMS,nullptr,nullptr,&out)==0;
    }
    bool same(const PF_State& a,const PF_State& b) const {
        A_Boolean equal=FALSE;
        return suite && suite->PF_AreStatesIdentical &&
            suite->PF_AreStatesIdentical(data->effect_ref,&a,&b,&equal)==0 && equal;
    }
};
}
void initialize_native_temporal_ui() noexcept {
    std::lock_guard lock(ui_mutex);ui_thread=std::this_thread::get_id();ui_stamps.clear();ui_timing={};
}
NativeUITiming last_native_ui_timing() noexcept {
    std::lock_guard lock(ui_mutex);return ui_timing;
}
PF_Err register_native_temporal_ui(PF_InData* data) noexcept {
    if(!data || !data->inter.register_ui)return PF_Err_BAD_CALLBACK_PARAM;
    PF_CustomUIInfo info{};info.events=PF_CustomEFlag_COMP;
    // No ECW custom area, overlays, input handlers or additional parameters.
    return PF_REGISTER_UI(data,&info);
}
void refresh_native_temporal_ui(PF_InData* data,PF_ParamDef* params[],
    const PF_EventExtra* event,AEGP_PluginID plugin_id) noexcept try {
    if(refreshing || !event || event->e_type!=PF_Event_DRAW || !data || !data->effect_ref || !data->pica_basicP ||
        !params || data->num_params<=kGraphParameterId || !params[kGraphParameterId] ||
        params[kGraphParameterId]->param_type!=PF_Param_ARBITRARY_DATA || !plugin_id)return;
    std::optional<PF_State> old;
    {std::lock_guard lock(ui_mutex);
        if(ui_thread!=std::this_thread::get_id())return; // Before any SDK operation.
        for(const auto& entry:ui_stamps)if(entry.publisher==data->effect_ref){old=entry.state;break;}}
    RefreshScope scope;
    UITimer timer;
    ParamUtils utils(data);PF_State before{};
    if(!utils.state(before) || (old && utils.same(*old,before)))return;
    const auto graph=read_graph_parameter(data,params[kGraphParameterId]->u.arb_d.value);
    if(!graph.has_value())return;
    timer.refreshed=true;
    capture_native_temporal_metadata(data,graph.value(),plugin_id);
    std::vector<core::NodeId> ids;for(const auto& node:graph.value().nodes)ids.push_back(node.id);
    if(validated_native_control_proofs(data,ids).empty())return; // Retry optional unavailable metadata later.
    PF_State after{};if(!utils.state(after) || !utils.same(before,after))return;
    std::lock_guard lock(ui_mutex);
    std::erase_if(ui_stamps,[&](const auto& entry){return entry.publisher==data->effect_ref;});
    if(ui_stamps.size()>=32)ui_stamps.erase(ui_stamps.begin());
    ui_stamps.push_back({data->effect_ref,after});
    // No project writes, undo records, render requests or handled-event flags.
} catch(...) {} // Optional UI optimization cannot reject an authored edit.
}
