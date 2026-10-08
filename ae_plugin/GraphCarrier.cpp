#include "GraphCarrier.hpp"
#include "NativeNodeGraph.hpp"
#include "NativeTemporalCache.hpp"
#include "Parameters.hpp"
#include "SPBasic.h"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <new>

namespace starfield::adapter {
namespace {
constexpr A_long kMaxNonce = 1000000;
std::atomic<AEGP_PluginID> g_plugin_id{0};
std::mutex g_registration_mutex;

void set_numeric(PF_ParamDef* params[], A_long index, PF_FpLong value) noexcept {
    params[index]->u.fs_d.value = value;
    params[index]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
}
PF_Err reject(PF_OutData* output, PF_ParamDef* params[], A_long nonce, const char* reason, PF_Err error) noexcept {
    if (params[kGraphEditReceiptId]) set_numeric(params, kGraphEditReceiptId, -nonce);
    if (output) std::snprintf(output->return_msg, sizeof(output->return_msg), "Starfield graph edit rejected: %s", reason);
    return error;
}
}

PF_Err register_graph_carrier(PF_InData* data) noexcept {
    if (!data || !data->pica_basicP || data->appl_id == 'PrMr') return PF_Err_UNRECOGNIZED_PARAM_TYPE;
    if (g_plugin_id.load(std::memory_order_acquire)) return PF_Err_NONE;
    std::lock_guard lock(g_registration_mutex);
    if (g_plugin_id.load(std::memory_order_relaxed)) return PF_Err_NONE;
    const AEGP_UtilitySuite6* utility = nullptr;
    A_Err error = data->pica_basicP->AcquireSuite(kAEGPUtilitySuite, kAEGPUtilitySuiteVersion6,
                                                reinterpret_cast<const void**>(&utility));
    AEGP_PluginID id = 0;
    if (!error && utility) error = utility->AEGP_RegisterWithAEGP(nullptr, "Starfield Particle", &id);
    if (utility) data->pica_basicP->ReleaseSuite(kAEGPUtilitySuite, kAEGPUtilitySuiteVersion6);
    if (!error && id) { g_plugin_id.store(id, std::memory_order_release); return PF_Err_NONE; }
    return static_cast<PF_Err>(error ? error : PF_Err_BAD_CALLBACK_PARAM);
}

AEGP_PluginID graph_carrier_plugin_id() noexcept { return g_plugin_id.load(std::memory_order_acquire); }

PF_Err commit_graph_request(PF_InData* data, PF_OutData* output, PF_ParamDef* params[],
                            PF_UserChangedParamExtra* extra) noexcept {
    if (!data || !params || !extra || extra->param_index != kGraphEditCommitId) return PF_Err_NONE;
    if (data->num_params > 0 && data->num_params <= kGraphChecksumLowId) return PF_Err_BAD_CALLBACK_PARAM;
    A_long nonce = 1;
    try {
        if (!params[kGraphEditCommitId] || params[kGraphEditCommitId]->param_type != PF_Param_FLOAT_SLIDER ||
            !std::isfinite(params[kGraphEditCommitId]->u.fs_d.value) || params[kGraphEditCommitId]->u.fs_d.value < 1.0 ||
            params[kGraphEditCommitId]->u.fs_d.value > kMaxNonce ||
            std::floor(params[kGraphEditCommitId]->u.fs_d.value) != params[kGraphEditCommitId]->u.fs_d.value) {
            return reject(output, params, nonce, "invalid transaction nonce", PF_Err_NONE);
        }
        nonce = static_cast<A_long>(params[kGraphEditCommitId]->u.fs_d.value);
        if (!params[kGraphEditReceiptId] || params[kGraphEditReceiptId]->param_type != PF_Param_FLOAT_SLIDER ||
            !params[kGraphParameterId] || params[kGraphParameterId]->param_type != PF_Param_ARBITRARY_DATA ||
            !params[kControlSourceId] || params[kControlSourceId]->param_type != PF_Param_POPUP ||
            !params[kNodeEffectsReadyId] || params[kNodeEffectsReadyId]->param_type != PF_Param_FLOAT_SLIDER) {
            return reject(output, params, nonce, "renderer graph parameters unavailable", PF_Err_BAD_CALLBACK_PARAM);
        }
        core::Graph graph;
        bool found = false;
        PF_Err error = compile_native_node_graph(data, params, graph, found, graph_carrier_plugin_id());
        if (error) return reject(output, params, nonce, "native node records do not form a valid graph", error);
        if (!found && params[kNodeEffectsReadyId]->u.fs_d.value < 1.0) {
            ParameterSnapshot controls;
            error = controls.checkout(data);
            if (error) return reject(output, params, nonce, "initial defaults unavailable", error);
            auto initial = graph_from_controls(controls.settings());
            controls.checkin(data);
            if (!initial.has_value()) return reject(output, params, nonce, "initial graph invalid", PF_Err_BAD_CALLBACK_PARAM);
            graph = initial.take_value();
        }
        PF_ArbitraryH replacement = nullptr;
        error = create_graph_parameter(data, graph, &replacement);
        if (error) return reject(output, params, nonce, "graph allocation failed", error);
        NativeBindingTransaction bindings(data, graph_carrier_plugin_id());
        A_long failed_binding = -1;
        A_long failed_parameter = -1;
        const char* failed_stage = "unknown";
        error = bindings.install(graph, &failed_binding, &failed_stage, &failed_parameter);
        if (error) {
            data->utils->host_dispose_handle(replacement);
            char reason[200]{};
            std::snprintf(reason, sizeof(reason), "animation binding stream %ld, parameter %ld, %s failed (error %ld)",
                static_cast<long>(failed_binding), static_cast<long>(failed_parameter), failed_stage, static_cast<long>(error));
            return reject(output, params, nonce, reason, error);
        }
        error = write_graph_snapshot(data, params, graph);
        if (error) { data->utils->host_dispose_handle(replacement); return reject(output, params, nonce, "numeric graph receipt failed", error); }
        params[kGraphParameterId]->u.arb_d.value = replacement;
        params[kGraphParameterId]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        params[kControlSourceId]->u.pd.value = kNodeControlSource;
        params[kControlSourceId]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        bindings.accept();
        capture_native_temporal_metadata(data,graph,graph_carrier_plugin_id());
        set_numeric(params, kGraphEditReceiptId, nonce);
        if (output) output->out_flags |= PF_OutFlag_FORCE_RERENDER;
        return PF_Err_NONE;
    } catch (const std::bad_alloc&) { return reject(output, params, nonce, "allocation failed", PF_Err_OUT_OF_MEMORY); }
    catch (...) { return reject(output, params, nonce, "unexpected failure", PF_Err_INTERNAL_STRUCT_DAMAGED); }
}
}
