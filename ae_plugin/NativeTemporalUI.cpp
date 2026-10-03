#include "NativeTemporalUI.hpp"
#include "NativeTemporalCache.hpp"
#include "GraphParameter.hpp"
#include "SPBasic.h"
#include <algorithm>
#include <chrono>
#include <cstring>
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
    bool refreshed{},sequence{};
    std::chrono::steady_clock::time_point start{std::chrono::steady_clock::now()};
    ~UITimer() {
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        std::lock_guard lock(ui_mutex);++ui_timing.calls;
        ui_timing.refreshes+=refreshed;ui_timing.sequence_refreshes+=refreshed && sequence;
        ui_timing.last_ms=ms;ui_timing.max_ms=std::max(ui_timing.max_ms,ms);
    }
};
constexpr char kSequenceMarker[4]={'S','F','U','1'}; // Byte ordered, already flat POD.
bool on_ui_thread() {
    std::lock_guard lock(ui_mutex);return ui_thread==std::this_thread::get_id();
}
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

PF_Err flatten_native_temporal_sequence(PF_InData* data,PF_OutData* out) noexcept {
    if(!data || !out || !data->utils || !data->utils->host_new_handle ||
        !data->utils->host_lock_handle || !data->utils->host_unlock_handle ||
        !data->utils->host_dispose_handle || !data->utils->host_get_handle_size)return PF_Err_BAD_CALLBACK_PARAM;
    out->sequence_data=data->sequence_data;
    if(out->sequence_data) {
        if(data->utils->host_get_handle_size(out->sequence_data)!=sizeof(kSequenceMarker))return PF_Err_INTERNAL_STRUCT_DAMAGED;
        const auto* bytes=data->utils->host_lock_handle(out->sequence_data);
        if(!bytes)return PF_Err_OUT_OF_MEMORY;
        const bool valid=std::memcmp(bytes,kSequenceMarker,sizeof(kSequenceMarker))==0;
        data->utils->host_unlock_handle(out->sequence_data);
        return valid?PF_Err_NONE:PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
    // Provision legacy null data on save as well as setup/resetup. Some older
    // projects may not receive RESETUP until non-null data has been saved.
    out->sequence_data=data->utils->host_new_handle(sizeof(kSequenceMarker));
    if(!out->sequence_data)return PF_Err_OUT_OF_MEMORY;
    if(auto* bytes=data->utils->host_lock_handle(out->sequence_data)) {
        std::memcpy(bytes,kSequenceMarker,sizeof(kSequenceMarker));
        data->utils->host_unlock_handle(out->sequence_data);
        return PF_Err_NONE;
    }
    data->utils->host_dispose_handle(out->sequence_data);
    out->sequence_data=nullptr;return PF_Err_OUT_OF_MEMORY;
}
PF_Err setup_native_temporal_sequence(PF_InData* data,PF_OutData* out,PF_ParamDef* params[],
    AEGP_PluginID id,bool resetup) noexcept try {
    if(const auto error=flatten_native_temporal_sequence(data,out);error)return error;
    if(refreshing || (resetup && (data->in_flags & PF_InFlag_PROJECT_IS_RENDER_ONLY)) ||
        !on_ui_thread() || !data->effect_ref || !data->pica_basicP || !id)return PF_Err_NONE;
    RefreshScope scope;UITimer timer;timer.sequence=true;
    PF_ParamDef checked{};bool owned=false;PF_ArbitraryH handle{};
    // A partial/absent params array is not an excuse to index beyond it.
    if(params && data->num_params>kGraphParameterId && params[kGraphParameterId] &&
        params[kGraphParameterId]->param_type==PF_Param_ARBITRARY_DATA)
        handle=params[kGraphParameterId]->u.arb_d.value;
    else if(data->inter.checkout_param && data->inter.checkin_param) {
        if(PF_CHECKOUT_PARAM(data,kGraphParameterId,data->current_time,data->time_step,
            data->time_scale?data->time_scale:1,&checked))return PF_Err_NONE;
        owned=true;if(checked.param_type==PF_Param_ARBITRARY_DATA)handle=checked.u.arb_d.value;
    }
    struct Checkin {PF_InData* data;PF_ParamDef* value;bool owned;
        ~Checkin(){if(owned)(void)PF_CHECKIN_PARAM(data,value);}} checkin{data,&checked,owned};
    const auto graph=read_graph_parameter(data,handle);
    if(!graph.has_value())return PF_Err_NONE;
    timer.refreshed=true;
    capture_native_temporal_metadata(data,graph.value(),id);
    // Do not suppress a later DRAW retry: sibling effects may still be restoring.
    // No writes to project streams, no saved PF states and no generic messages.
    return PF_Err_NONE;
} catch(...) {return PF_Err_NONE;}
void setdown_native_temporal_sequence(PF_InData* data) noexcept {
    if(data && data->sequence_data && data->utils && data->utils->host_dispose_handle)
        data->utils->host_dispose_handle(data->sequence_data);
}
}
