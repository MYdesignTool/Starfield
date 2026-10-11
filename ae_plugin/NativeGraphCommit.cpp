#include "NativeGraphCommit.hpp"
#include "NativeNodeGraph.hpp"
#include "GraphParameter.hpp"
#include "Parameters.hpp"
#include "MotionBlur.hpp"
#include "NativeTemporalCache.hpp"
#include "SPBasic.h"
#include "starfield/core/SequenceCodec.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include <vector>

namespace starfield::adapter {
namespace {
constexpr PF_FpLong kMaxRevision = 16777215.0;
void set_numeric(PF_ParamDef* params[], A_long index, PF_FpLong value) noexcept {
    params[index]->u.fs_d.value = value;
    params[index]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
}
}

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

static PF_Err commit_graph_streams(node_sync::NativeEdit* edit, AEGP_PluginID plugin_id,
                               PF_OutData* output, PF_InData* panel_data = nullptr, A_long nonce = 0) noexcept {
    if (!edit) return PF_Err_BAD_CALLBACK_PARAM;
    edit->accepted = false; edit->revision = 0; edit->status = PF_Err_BAD_CALLBACK_PARAM;
    edit->stage = node_sync::Stage::context; edit->stream_index = -1;
    if (!edit->handles || !edit->basic || (!panel_data && !node_sync::valid_edit(*edit)) ||
        !edit->renderer || !edit->layer || edit->width <= 0 || edit->height <= 0 ||
        edit->time_scale <= 0 || edit->pixel_aspect.num <= 0 || edit->pixel_aspect.den <= 0)
        return edit->status;
    try {
        // Compiler input is local; saved renderer values come only from AEGP.
        PF_InData context{};
        context.utils = edit->handles; context.pica_basicP = edit->basic;
        context.width = edit->width; context.height = edit->height;
        context.current_time = edit->time; context.time_scale = edit->time_scale;
        context.pixel_aspect_ratio = edit->pixel_aspect;
        context.num_params = static_cast<A_long>(kTotalEffectParameterCount + 1);
        PF_InData* data = &context;
        if(panel_data)context.effect_ref=panel_data->effect_ref;
        // Generic calls can be nested below AE's scripting stack. Keep the
        // bounded parameter bank off that UI thread's stack.
        std::vector<PF_ParamDef> scratch(kTotalEffectParameterCount + 1);
        std::array<PF_ParamDef*, kTotalEffectParameterCount + 1> pointers{};
        for (std::size_t i = 0; i < scratch.size(); ++i) {
            scratch[i].param_type = PF_Param_FLOAT_SLIDER; pointers[i] = &scratch[i];
        }
        scratch[kControlSourceId].param_type = PF_Param_POPUP;
        scratch[kControlSourceId].u.pd.value = kNodeControlSource;

        struct Publish {
            PF_InData* data{};
            const AEGP_StreamSuite6* streams{};
            AEGP_EffectRefH renderer{};
            PF_ArbitraryH graph{};
            std::array<AEGP_StreamRefH, 6> refs{};
            std::array<AEGP_StreamValue2, 6> old{};
            std::array<bool, 6> read{};
            ~Publish() {
                for (std::size_t i = 0; i < refs.size(); ++i) {
                    if (read[i]) streams->AEGP_DisposeStreamValue(&old[i]);
                    if (refs[i]) streams->AEGP_DisposeStream(refs[i]);
                }
                // renderer is borrowed from the caller's local UI context.
                if (graph) data->utils->host_dispose_handle(graph);
                if (streams) data->pica_basicP->ReleaseSuite(kAEGPStreamSuite, kAEGPStreamSuiteVersion6);
            }
        } publish;
        publish.data = data;
        publish.renderer = edit->renderer;
        if (!plugin_id) return edit->status;
        auto* basic = data->pica_basicP;
        edit->stage = node_sync::Stage::suites;
        A_Err ae_error = basic->AcquireSuite(kAEGPStreamSuite, kAEGPStreamSuiteVersion6,
                                             reinterpret_cast<const void**>(&publish.streams));
        if (ae_error || !publish.renderer) return edit->status = static_cast<PF_Err>(ae_error ? ae_error : PF_Err_BAD_CALLBACK_PARAM);
        const A_Time time{data->current_time, data->time_scale};
        if(panel_data) {
            // Prove the live request before touching bindings or graph. Backup
            // effects use guard=2 and must never become a command target.
            for(const auto index:{kGraphSyncGuardId,kGraphEditCommitId,kNodeEffectsReadyId}) {
                AEGP_StreamRefH ref{};AEGP_StreamValue2 value{};
                ae_error=publish.streams->AEGP_GetNewEffectStreamByIndex(plugin_id,publish.renderer,index,&ref);
                AEGP_StreamType type{};
                if(!ae_error && ref)ae_error=publish.streams->AEGP_GetStreamType(ref,&type);
                if(!ae_error && (!ref || type!=AEGP_StreamType_OneD))ae_error=PF_Err_BAD_CALLBACK_PARAM;
                if(!ae_error) {
                    ae_error=publish.streams->AEGP_GetNewStreamValue(plugin_id,ref,AEGP_LTimeMode_LayerTime,&time,TRUE,&value);
                    if(!ae_error) {
                        const double expected=index==kGraphEditCommitId?nonce:1;
                        if(value.val.one_d!=expected)ae_error=PF_Err_BAD_CALLBACK_PARAM;
                        publish.streams->AEGP_DisposeStreamValue(&value);
                    }
                }
                if(ref)publish.streams->AEGP_DisposeStream(ref);
                if(ae_error){edit->stream_index=index;return edit->status=static_cast<PF_Err>(ae_error);}
            }
        }
        edit->stage = node_sync::Stage::controls;
        constexpr std::array<A_long, 18> controls{kMaxParticlesId, kLayoutOutputXId, kLayoutOutputYId,
            kGraphRevisionId, kTimeRemapEnabledId, kTimeRemapSecondsId, kPreviewEnabledId, kPreviewChanceId,kTimeSamplingHzId,kAccelerationId,
            kMotionParameterIds[0],kMotionParameterIds[1],kMotionParameterIds[2],kMotionParameterIds[3],kMotionParameterIds[4],kMotionParameterIds[5],kMotionParameterIds[6],kMotionParameterIds[7]};
        for (const A_long index : controls) {
            edit->stream_index = index;
            AEGP_StreamRefH ref = nullptr;
            ae_error = publish.streams->AEGP_GetNewEffectStreamByIndex(plugin_id, publish.renderer, index, &ref);
            if (ae_error || !ref) return edit->status = static_cast<PF_Err>(ae_error ? ae_error : PF_Err_BAD_CALLBACK_PARAM);
            AEGP_StreamType type = AEGP_StreamType_NO_DATA;
            ae_error = publish.streams->AEGP_GetStreamType(ref, &type);
            if (ae_error || type != AEGP_StreamType_OneD) {
                publish.streams->AEGP_DisposeStream(ref);
                return edit->status = static_cast<PF_Err>(ae_error ? ae_error : PF_Err_BAD_CALLBACK_PARAM);
            }
            AEGP_StreamValue2 value{};
            ae_error = publish.streams->AEGP_GetNewStreamValue(plugin_id, ref, AEGP_LTimeMode_LayerTime, &time, TRUE, &value);
            if (!ae_error) {
                if (!std::isfinite(value.val.one_d)) ae_error = PF_Err_BAD_CALLBACK_PARAM;
                else if (index == kTimeRemapEnabledId || index == kPreviewEnabledId) {
                    if (value.val.one_d != 0 && value.val.one_d != 1) ae_error = PF_Err_BAD_CALLBACK_PARAM;
                    else { scratch[index].param_type = PF_Param_CHECKBOX; scratch[index].u.bd.value = static_cast<A_long>(value.val.one_d); }
                } else if(index==kTimeSamplingHzId) {
                    if(value.val.one_d<1 || value.val.one_d>3 || std::floor(value.val.one_d)!=value.val.one_d) ae_error=PF_Err_BAD_CALLBACK_PARAM;
                    else {scratch[index].param_type=PF_Param_POPUP;scratch[index].u.pd.value=static_cast<A_long>(value.val.one_d);}
                } else if(index==kAccelerationId || index==kMotionParameterIds[0] || index==kMotionParameterIds[3] || index==kMotionParameterIds[7]) {
                    if(value.val.one_d<1 || value.val.one_d>(index==kMotionParameterIds[0]?3:2) || std::floor(value.val.one_d)!=value.val.one_d) ae_error=PF_Err_BAD_CALLBACK_PARAM;
                    else {scratch[index].param_type=PF_Param_POPUP;scratch[index].u.pd.value=static_cast<A_long>(value.val.one_d);}
                } else scratch[index].u.fs_d.value = value.val.one_d;
                publish.streams->AEGP_DisposeStreamValue(&value);
            }
            publish.streams->AEGP_DisposeStream(ref);
            if (ae_error) return edit->status = static_cast<PF_Err>(ae_error);
        }
        edit->stage = node_sync::Stage::capture; edit->stream_index = kGraphParameterId;
        // Direct node callbacks have no renderer params[] delivery. Retain the
        // saved graph before compiling so existing attachment offsets survive
        // both native control edits and reference changes. Publish owns the value
        // until completion and reuses it for exact rollback below.
        constexpr std::size_t saved_graph_slot=4;
        ae_error=publish.streams->AEGP_GetNewEffectStreamByIndex(plugin_id,publish.renderer,
            kGraphParameterId,&publish.refs[saved_graph_slot]);
        AEGP_StreamType saved_graph_type=AEGP_StreamType_NO_DATA;
        if(!ae_error && !publish.refs[saved_graph_slot])ae_error=PF_Err_BAD_CALLBACK_PARAM;
        if(!ae_error)ae_error=publish.streams->AEGP_GetStreamType(publish.refs[saved_graph_slot],&saved_graph_type);
        if(!ae_error && saved_graph_type!=AEGP_StreamType_ARB)ae_error=PF_Err_BAD_CALLBACK_PARAM;
        if(!ae_error)ae_error=publish.streams->AEGP_GetNewStreamValue(plugin_id,publish.refs[saved_graph_slot],
            AEGP_LTimeMode_LayerTime,&time,TRUE,&publish.old[saved_graph_slot]);
        if(ae_error)return edit->status=static_cast<PF_Err>(ae_error);
        publish.read[saved_graph_slot]=true;
        scratch[kGraphParameterId].param_type=PF_Param_ARBITRARY_DATA;
        scratch[kGraphParameterId].u.arb_d.value=reinterpret_cast<PF_ArbitraryH>(publish.old[saved_graph_slot].val.arbH);
        edit->stage = node_sync::Stage::compile; edit->stream_index = -1;
        core::Graph graph;
        bool found = false;
        PF_Err error = compile_native_node_graph(data, pointers.data(), graph, found, plugin_id, edit);
        if (error || (!found && !panel_data)) return edit->status = error ? error : PF_Err_BAD_CALLBACK_PARAM;
        edit->stage = node_sync::Stage::snapshot;
        A_long revision = 0;
        error = write_graph_snapshot(data, pointers.data(), graph, &revision);
        if (error) return edit->status = error;
        edit->stage = node_sync::Stage::allocation;
        error = create_graph_parameter(data, graph, &publish.graph);
        if (error) return edit->status = error;
        NativeBindingTransaction bindings(data, plugin_id, edit->renderer, edit->layer);
        edit->stage = node_sync::Stage::animation_bindings;
        error = bindings.install(graph, &edit->stream_index, &edit->binding_stage, &edit->binding_parameter);
        if (error) return edit->status = error;
        // These are integer receipts, not authored node values. Revision <= 2^24-1,
        // source == 2 and both CRC halves <= 65535 survive PF_FpShort exactly.
        // Do not add continuous/color/point values to this scalar-only contract.
        constexpr std::array<A_long, 6> indices{kControlSourceId, kGraphRevisionId,
            kGraphChecksumHighId, kGraphChecksumLowId, kGraphParameterId, kGraphEditReceiptId};
        constexpr std::size_t graph_slot = 4;
        const std::size_t publish_count=panel_data?indices.size():graph_slot+1;
        const std::array<PF_FpLong, graph_slot> scalars{kNodeControlSource,
            scratch[kGraphRevisionId].u.fs_d.value, scratch[kGraphChecksumHighId].u.fs_d.value,
            scratch[kGraphChecksumLowId].u.fs_d.value};
        edit->stage = node_sync::Stage::snapshot;
        for (std::size_t i = 0; i < scalars.size(); ++i) {
            edit->stream_index = indices[i];
            const auto value = scalars[i];
            if (!std::isfinite(value) || std::floor(value) != value || value < 0 || value > kMaxRevision)
                return edit->status = PF_Err_BAD_CALLBACK_PARAM;
        }
        edit->stage = node_sync::Stage::capture;
        for (std::size_t i = 0; i < publish_count; ++i) {
            edit->stream_index = indices[i];
            if(publish.read[i])continue;
            ae_error = publish.streams->AEGP_GetNewEffectStreamByIndex(plugin_id, publish.renderer,
                indices[i], &publish.refs[i]);
            if (ae_error || !publish.refs[i]) return edit->status = static_cast<PF_Err>(ae_error ? ae_error : PF_Err_BAD_CALLBACK_PARAM);
            AEGP_StreamType type = AEGP_StreamType_NO_DATA;
            ae_error = publish.streams->AEGP_GetStreamType(publish.refs[i], &type);
            if (ae_error || type != (i == graph_slot ? AEGP_StreamType_ARB : AEGP_StreamType_OneD))
                return edit->status = static_cast<PF_Err>(ae_error ? ae_error : PF_Err_BAD_CALLBACK_PARAM);
            ae_error = publish.streams->AEGP_GetNewStreamValue(plugin_id, publish.refs[i], AEGP_LTimeMode_LayerTime,
                &time, TRUE, &publish.old[i]);
            if (ae_error) return edit->status = static_cast<PF_Err>(ae_error);
            publish.read[i] = true;
        }
        std::size_t written = 0;
        edit->stage = node_sync::Stage::publish_scalars;
        for (; written < graph_slot; ++written) {
            edit->stream_index = indices[written];
            AEGP_StreamValue2 value{}; value.streamH = publish.refs[written];
            value.val.one_d = scalars[written];
            ae_error = publish.streams->AEGP_SetStreamValue(plugin_id, publish.refs[written], &value);
            if (ae_error) { ++written; break; } // Restore even a partially applied failed write.
        }
        // Publish graph last: its changed stream invalidates the main renderer.
        if (!ae_error) {
            edit->stage = node_sync::Stage::publish_graph;
            edit->stream_index = kGraphParameterId;
            AEGP_StreamValue2 value{}; value.streamH = publish.refs[graph_slot];
            value.val.arbH = reinterpret_cast<AEGP_ArbBlockVal>(publish.graph);
            ++written; // Include the graph in rollback even if this call partially fails.
            ae_error = publish.streams->AEGP_SetStreamValue(plugin_id, publish.refs[graph_slot], &value);
        }
        if(!ae_error && panel_data) {
            // Receipt is last. It certifies a complete publication, never just
            // receipt of the command. Include failed/partial writes in rollback.
            edit->stream_index=kGraphEditReceiptId;
            AEGP_StreamValue2 value{};value.streamH=publish.refs[5];value.val.one_d=nonce;
            ++written;ae_error=publish.streams->AEGP_SetStreamValue(plugin_id,publish.refs[5],&value);
        }
        // A successful suite call still needs an actual saved graph and receipt.
        if (!ae_error) {
            for (std::size_t i = 0; i < publish_count; ++i) {
                edit->stage = i == graph_slot ? node_sync::Stage::verify_graph : node_sync::Stage::verify_scalars;
                edit->stream_index = indices[i];
                AEGP_StreamValue2 saved{};
                ae_error = publish.streams->AEGP_GetNewStreamValue(plugin_id, publish.refs[i], AEGP_LTimeMode_LayerTime,
                    &time, TRUE, &saved);
                if (ae_error) break;
                if (i == graph_slot) {
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
                } else if (saved.val.one_d != (i==5?nonce:scalars[i])) {
                    ae_error = PF_Err_BAD_CALLBACK_PARAM;
                }
                publish.streams->AEGP_DisposeStreamValue(&saved);
                if (ae_error) break;
            }
        }
        if (ae_error) {
            bool rollback_failed = bindings.rollback_model_resources()!=PF_Err_NONE;
            while (written > 0) {
                --written;
                if (publish.streams->AEGP_SetStreamValue(plugin_id, publish.refs[written], &publish.old[written]) != 0) {
                    rollback_failed = true; edit->stream_index = indices[written];
                }
            }
            if (output) std::snprintf(output->return_msg, sizeof(output->return_msg),
                "Starfield native graph publication failed%s.", rollback_failed ? "; rollback failed" : " (restored)");
            if (rollback_failed) edit->stage = node_sync::Stage::rollback;
            return edit->status = rollback_failed ? PF_Err_INTERNAL_STRUCT_DAMAGED : static_cast<PF_Err>(ae_error);
        }
        bindings.accept();
        if(panel_data)capture_native_temporal_metadata(data,graph,plugin_id);
        edit->revision = revision; edit->status = PF_Err_NONE; edit->accepted = true;
        edit->stage = node_sync::Stage::complete; edit->stream_index = -1;
        return PF_Err_NONE;
    } catch (const std::bad_alloc&) { return edit->status = PF_Err_OUT_OF_MEMORY; }
    catch (...) { return edit->status = PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

PF_Err commit_native_graph_edit(node_sync::NativeEdit* edit,AEGP_PluginID plugin_id,PF_OutData* output) noexcept {
    return commit_graph_streams(edit,plugin_id,output);
}
PF_Err commit_panel_graph(PF_InData* data,AEGP_EffectRefH renderer,AEGP_LayerH layer,
                         A_long nonce,AEGP_PluginID plugin_id,PF_OutData* output) noexcept {
    if(!data || !data->utils || !data->pica_basicP || !renderer || !layer || !plugin_id ||
        nonce<1 || nonce>1000000)return PF_Err_BAD_CALLBACK_PARAM;
    node_sync::NativeEdit context;
    context.records_only=true;
    context.renderer=renderer;context.layer=layer;context.handles=data->utils;context.basic=data->pica_basicP;
    context.width=data->width;context.height=data->height;context.time=data->current_time;
    context.time_scale=data->time_scale;context.pixel_aspect=data->pixel_aspect_ratio;
    if(context.basic && (context.width<=0 || context.height<=0 || context.time_scale<=0 ||
        context.pixel_aspect.num<=0 || context.pixel_aspect.den<=0)) {
        const AEGP_LayerSuite9* layers{};const AEGP_ItemSuite9* items{};
        auto error=context.basic->AcquireSuite(kAEGPLayerSuite,kAEGPLayerSuiteVersion9,reinterpret_cast<const void**>(&layers));
        if(!error)error=context.basic->AcquireSuite(kAEGPItemSuite,kAEGPItemSuiteVersion9,reinterpret_cast<const void**>(&items));
        if(!error && layers && items) {
            AEGP_ItemH item{};A_Ratio aspect{};A_Time time{};
            error=layers->AEGP_GetLayerSourceItem(layer,&item);
            if(!error && !item)error=PF_Err_BAD_CALLBACK_PARAM;
            if(!error)error=items->AEGP_GetItemDimensions(item,&context.width,&context.height);
            if(!error)error=items->AEGP_GetItemPixelAspectRatio(item,&aspect);
            if(!error)error=layers->AEGP_GetLayerCurrentTime(layer,AEGP_LTimeMode_LayerTime,&time);
            if(!error){context.pixel_aspect={aspect.num,aspect.den};context.time=time.value;context.time_scale=time.scale;}
        }
        if(items)context.basic->ReleaseSuite(kAEGPItemSuite,kAEGPItemSuiteVersion9);
        if(layers)context.basic->ReleaseSuite(kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
        if(error)return static_cast<PF_Err>(error);
    }
    const auto error=commit_graph_streams(&context,plugin_id,output,data,nonce);
    if(error && output && context.stage==node_sync::Stage::compile && context.compile_stage)
        std::snprintf(output->return_msg,sizeof(output->return_msg),
            "Starfield graph command failed: node compile: %s, effect %ld, stream %ld (error %d).",
            context.compile_stage,static_cast<long>(context.compile_effect_index),static_cast<long>(context.stream_index),static_cast<int>(error));
    else if(error && output)std::snprintf(output->return_msg,sizeof(output->return_msg),
        "Starfield graph command failed: %s, stream %ld%s%s (error %d).",
        node_sync::stage_name(context.stage),static_cast<long>(context.stream_index),
        context.binding_stage?", ":"",context.binding_stage?context.binding_stage:"",static_cast<int>(error));
    return error;
}
}
