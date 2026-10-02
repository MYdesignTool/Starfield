#include "GraphCarrier.hpp"
#include "NativeNodeGraph.hpp"
#include "Parameters.hpp"
#include "NodeGraphSync.hpp"
#include "SPBasic.h"
#include "starfield/core/SequenceCodec.hpp"

#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <new>
#include <array>

namespace starfield::adapter {
namespace {
constexpr A_long kMaxNonce = 1000000;
constexpr PF_FpLong kMaxRevision = 16777215.0;
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

PF_Err write_graph_snapshot(PF_InData* data, PF_ParamDef* params[], const core::Graph& graph,
                            A_long* new_revision) noexcept {
    if (!data || !params) return PF_Err_BAD_CALLBACK_PARAM;
    for (const A_long index : {kGraphRevisionId, kGraphChecksumHighId, kGraphChecksumLowId}) {
        if (!params[index] || params[index]->param_type != PF_Param_FLOAT_SLIDER) return PF_Err_BAD_CALLBACK_PARAM;
    }
    try {
        const auto encoded = core::serialize_graph(graph, core::particle_node_registry());
        if (!encoded.has_value()) return encoded.error().code == core::SequenceErrorCode::allocation_failed
            ? PF_Err_OUT_OF_MEMORY : PF_Err_BAD_CALLBACK_PARAM;
        const auto old_revision = params[kGraphRevisionId]->u.fs_d.value;
        if (!std::isfinite(old_revision) || old_revision < 0.0 || old_revision >= kMaxRevision ||
            std::floor(old_revision) != old_revision || encoded.value().size() < 32) return PF_Err_BAD_CALLBACK_PARAM;
        std::uint32_t checksum = 0;
        for (unsigned i = 0; i < 4; ++i) {
            checksum |= std::to_integer<std::uint32_t>(encoded.value()[24 + i]) << (8u * i);
        }
        set_numeric(params, kGraphRevisionId, old_revision + 1.0);
        set_numeric(params, kGraphChecksumHighId, checksum >> 16u);
        set_numeric(params, kGraphChecksumLowId, checksum & 65535u);
        if (new_revision) *new_revision = static_cast<A_long>(old_revision + 1.0);
        return PF_Err_NONE;
    } catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

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
        PF_Err error = compile_native_node_graph(data, params, graph, found);
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
        error = write_graph_snapshot(data, params, graph);
        if (error) { data->utils->host_dispose_handle(replacement); return reject(output, params, nonce, "numeric graph receipt failed", error); }
        params[kGraphParameterId]->u.arb_d.value = replacement;
        params[kGraphParameterId]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        params[kControlSourceId]->u.pd.value = kNodeControlSource;
        params[kControlSourceId]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        set_numeric(params, kGraphEditReceiptId, nonce);
        if (output) output->out_flags |= PF_OutFlag_FORCE_RERENDER;
        return PF_Err_NONE;
    } catch (const std::bad_alloc&) { return reject(output, params, nonce, "allocation failed", PF_Err_OUT_OF_MEMORY); }
    catch (...) { return reject(output, params, nonce, "unexpected failure", PF_Err_INTERNAL_STRUCT_DAMAGED); }
}

PF_Err commit_native_graph_edit(PF_InData* data, PF_OutData* output, PF_ParamDef* params[],
                               node_sync::NativeEdit* edit) noexcept {
    // Unknown generic calls do not belong to our private transport.
    if (!edit || edit->magic != 0x53464E45u) return PF_Err_NONE;
    edit->accepted = false; edit->revision = 0; edit->status = PF_Err_BAD_CALLBACK_PARAM;
    if (!data || !params || !data->utils || !data->pica_basicP || !data->effect_ref ||
        !node_sync::valid_edit(*edit) || data->num_params <= kGraphChecksumLowId)
        return edit->status;
    try {
        // Never ask AE to honor modifications to a synthetic callback array.
        std::array<PF_ParamDef, kTotalEffectParameterCount + 1> scratch{};
        std::array<PF_ParamDef*, kTotalEffectParameterCount + 1> pointers{};
        if (data->num_params < static_cast<A_long>(scratch.size())) return edit->status;
        for (std::size_t i = 0; i < scratch.size(); ++i) {
            if (!params[i]) return edit->status;
            scratch[i] = *params[i]; pointers[i] = &scratch[i];
        }
        core::Graph graph;
        bool found = false;
        PF_Err error = compile_native_node_graph(data, pointers.data(), graph, found, edit);
        if (error || !found) return edit->status = error ? error : PF_Err_BAD_CALLBACK_PARAM;
        A_long revision = 0;
        error = write_graph_snapshot(data, pointers.data(), graph, &revision);
        if (error) return edit->status = error;
        scratch[kControlSourceId].u.pd.value = kNodeControlSource;

        struct Publish {
            PF_InData* data{};
            const AEGP_PFInterfaceSuite1* pf{};
            const AEGP_EffectSuite4* effects{};
            const AEGP_StreamSuite6* streams{};
            AEGP_EffectRefH renderer{};
            PF_ArbitraryH graph{};
            std::array<AEGP_StreamRefH, 5> refs{};
            std::array<AEGP_StreamValue2, 5> old{};
            std::array<bool, 5> read{};
            ~Publish() {
                for (std::size_t i = 0; i < refs.size(); ++i) {
                    if (read[i]) streams->AEGP_DisposeStreamValue(&old[i]);
                    if (refs[i]) streams->AEGP_DisposeStream(refs[i]);
                }
                if (renderer) effects->AEGP_DisposeEffect(renderer);
                if (graph) data->utils->host_dispose_handle(graph);
                if (streams) data->pica_basicP->ReleaseSuite(kAEGPStreamSuite, kAEGPStreamSuiteVersion6);
                if (effects) data->pica_basicP->ReleaseSuite(kAEGPEffectSuite, kAEGPEffectSuiteVersion4);
                if (pf) data->pica_basicP->ReleaseSuite(kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
            }
        } publish;
        publish.data = data;
        error = create_graph_parameter(data, graph, &publish.graph);
        if (error) return edit->status = error;
        const AEGP_PluginID plugin_id = graph_carrier_plugin_id();
        if (!plugin_id) return edit->status;
        auto* basic = data->pica_basicP;
        A_Err ae_error = basic->AcquireSuite(kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1,
                                             reinterpret_cast<const void**>(&publish.pf));
        if (!ae_error) ae_error = basic->AcquireSuite(kAEGPEffectSuite, kAEGPEffectSuiteVersion4,
                                                     reinterpret_cast<const void**>(&publish.effects));
        if (!ae_error) ae_error = basic->AcquireSuite(kAEGPStreamSuite, kAEGPStreamSuiteVersion6,
                                                     reinterpret_cast<const void**>(&publish.streams));
        if (!ae_error) ae_error = publish.pf->AEGP_GetNewEffectForEffect(plugin_id, data->effect_ref, &publish.renderer);
        if (ae_error || !publish.renderer) return edit->status = static_cast<PF_Err>(ae_error ? ae_error : PF_Err_BAD_CALLBACK_PARAM);
        // Publish graph last: its changed stream invalidates the main renderer.
        constexpr std::array<A_long, 5> indices{kControlSourceId, kGraphRevisionId,
            kGraphChecksumHighId, kGraphChecksumLowId, kGraphParameterId};
        const A_Time time{data->current_time, data->time_scale};
        for (std::size_t i = 0; i < indices.size(); ++i) {
            ae_error = publish.streams->AEGP_GetNewEffectStreamByIndex(plugin_id, publish.renderer,
                indices[i], &publish.refs[i]);
            if (ae_error || !publish.refs[i]) return edit->status = static_cast<PF_Err>(ae_error ? ae_error : PF_Err_BAD_CALLBACK_PARAM);
            ae_error = publish.streams->AEGP_GetNewStreamValue(plugin_id, publish.refs[i], AEGP_LTimeMode_LayerTime,
                &time, TRUE, &publish.old[i]);
            if (ae_error) return edit->status = static_cast<PF_Err>(ae_error);
            publish.read[i] = true;
        }
        std::size_t written = 0;
        for (; written < indices.size(); ++written) {
            AEGP_StreamValue2 value{}; value.streamH = publish.refs[written];
            if (written == indices.size() - 1) value.val.arbH = reinterpret_cast<AEGP_ArbBlockVal>(publish.graph);
            else value.val.one_d = written == 0 ? kNodeControlSource : scratch[indices[written]].u.fs_d.value;
            ae_error = publish.streams->AEGP_SetStreamValue(plugin_id, publish.refs[written], &value);
            if (ae_error) { ++written; break; } // Restore even a partially applied failed write.
        }
        // A successful suite call still needs an actual saved graph and receipt.
        if (!ae_error) {
            for (std::size_t i = 0; i < indices.size(); ++i) {
                AEGP_StreamValue2 saved{};
                ae_error = publish.streams->AEGP_GetNewStreamValue(plugin_id, publish.refs[i], AEGP_LTimeMode_LayerTime,
                    &time, TRUE, &saved);
                if (ae_error) break;
                if (i == indices.size() - 1) {
                    const auto handle = reinterpret_cast<PF_ArbitraryH>(saved.val.arbH);
                    const auto size = data->utils->host_get_handle_size(publish.graph);
                    if (!handle || data->utils->host_get_handle_size(handle) != size) ae_error = PF_Err_BAD_CALLBACK_PARAM;
                    else {
                        const void* expected = data->utils->host_lock_handle(publish.graph);
                        const void* actual = data->utils->host_lock_handle(handle);
                        if (!expected || !actual || std::memcmp(expected, actual, static_cast<std::size_t>(size)) != 0)
                            ae_error = PF_Err_BAD_CALLBACK_PARAM;
                        if (actual) data->utils->host_unlock_handle(handle);
                        if (expected) data->utils->host_unlock_handle(publish.graph);
                    }
                } else if (saved.val.one_d != (i == 0 ? kNodeControlSource : scratch[indices[i]].u.fs_d.value)) {
                    ae_error = PF_Err_BAD_CALLBACK_PARAM;
                }
                publish.streams->AEGP_DisposeStreamValue(&saved);
                if (ae_error) break;
            }
        }
        if (ae_error) {
            bool rollback_failed = false;
            while (written > 0) {
                --written;
                rollback_failed = publish.streams->AEGP_SetStreamValue(plugin_id, publish.refs[written],
                    &publish.old[written]) != 0 || rollback_failed;
            }
            if (output) std::snprintf(output->return_msg, sizeof(output->return_msg),
                "Starfield native graph publication failed%s.", rollback_failed ? "; rollback failed" : " (restored)");
            return edit->status = rollback_failed ? PF_Err_INTERNAL_STRUCT_DAMAGED : static_cast<PF_Err>(ae_error);
        }
        edit->revision = revision; edit->status = PF_Err_NONE; edit->accepted = true;
        return PF_Err_NONE;
    } catch (const std::bad_alloc&) { return edit->status = PF_Err_OUT_OF_MEMORY; }
    catch (...) { return edit->status = PF_Err_INTERNAL_STRUCT_DAMAGED; }
}
}
