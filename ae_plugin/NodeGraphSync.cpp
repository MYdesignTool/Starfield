#include "NodeEffects.hpp"

#include "AE_GeneralPlug.h"
#include "NodeRecord.hpp"
#include "NodeGraphSync.hpp"
#include "NativeGraphCommit.hpp"
#include "SPBasic.h"
#include "starfield/core/Graph.hpp"

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <new>

namespace {

#if defined(STARFIELD_NODE_KIND_EMITTER)
constexpr char kRegistrationName[] = "Starfield Emitter Node Sync";
constexpr A_long kLastParameterIndex = starfield::adapter::native_nodes::last_parameter_index(
    starfield::adapter::native_nodes::Kind::emitter);
constexpr A_long kNodeKind = 0;
#elif defined(STARFIELD_NODE_KIND_PARTICLE)
constexpr char kRegistrationName[] = "Starfield Particle Node Sync";
constexpr A_long kLastParameterIndex = starfield::adapter::native_nodes::last_parameter_index(
    starfield::adapter::native_nodes::Kind::particle);
constexpr A_long kNodeKind = 1;
#elif defined(STARFIELD_NODE_KIND_APPEARANCE)
constexpr char kRegistrationName[] = "Starfield Appearance Node Sync";
constexpr A_long kLastParameterIndex = starfield::adapter::native_nodes::last_parameter_index(
    starfield::adapter::native_nodes::Kind::appearance);
constexpr A_long kNodeKind = 2;
#elif defined(STARFIELD_NODE_KIND_FORCE)
constexpr char kRegistrationName[] = "Starfield Force Node Sync";
constexpr A_long kLastParameterIndex = starfield::adapter::native_nodes::last_parameter_index(
    starfield::adapter::native_nodes::Kind::force);
constexpr A_long kNodeKind = 3;
#else
#error Define exactly one STARFIELD_NODE_KIND_* for each node module.
#endif

constexpr A_long kIdentityFirstIndex = kLastParameterIndex - 8;
constexpr A_long kSyncGuardIndex = kLastParameterIndex;
constexpr char kRendererMatchName[] = "org.starfieldfx.particle";

using namespace starfield::adapter::native_nodes::disk_ids;
using starfield::adapter::native_nodes::uuid_id;

std::atomic<AEGP_PluginID> g_plugin_id{0};
std::mutex g_registration_mutex;

struct SuiteSet {
    explicit SuiteSet(PF_InData* data) : basic(data ? data->pica_basicP : nullptr) {}
    ~SuiteSet() {
        if (!basic) return;
        if (effect) basic->ReleaseSuite(kAEGPEffectSuite, kAEGPEffectSuiteVersion4);
        if (pf_interface) basic->ReleaseSuite(kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
    }
    PF_Err acquire() noexcept {
        if (!basic) return PF_Err_BAD_CALLBACK_PARAM;
        A_Err error = basic->AcquireSuite(kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1,
                                          reinterpret_cast<const void**>(&pf_interface));
        if (!error) error = basic->AcquireSuite(kAEGPEffectSuite, kAEGPEffectSuiteVersion4,
                                                 reinterpret_cast<const void**>(&effect));
        return static_cast<PF_Err>(error);
    }
    SPBasicSuite* basic{};
    const AEGP_PFInterfaceSuite1* pf_interface{};
    const AEGP_EffectSuite4* effect{};
};

bool read_node_id(PF_ParamDef* params[], std::array<std::uint16_t, 8>& chunks) noexcept {
    bool nonzero = false;
    for (A_long chunk = 0; chunk < 8; ++chunk) {
        const PF_ParamDef* parameter = params[kIdentityFirstIndex + chunk];
        if (!parameter || parameter->param_type != PF_Param_FLOAT_SLIDER ||
            !std::isfinite(parameter->u.fs_d.value) || parameter->u.fs_d.value < 0.0 ||
            parameter->u.fs_d.value > 65535.0 || std::floor(parameter->u.fs_d.value) != parameter->u.fs_d.value) {
            return false;
        }
        chunks[static_cast<std::size_t>(chunk)] = static_cast<std::uint16_t>(parameter->u.fs_d.value);
        nonzero = nonzero || chunks[static_cast<std::size_t>(chunk)] != 0;
    }
    return nonzero;
}

PF_Err locate_renderer(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_LayerH layer,
                       AEGP_EffectRefH& renderer) noexcept {
    renderer = nullptr;
    A_long effect_count = 0;
    A_Err error = suites.effect->AEGP_GetLayerNumEffects(layer, &effect_count);
    if (error) return static_cast<PF_Err>(error);
    for (A_long index = 0; index < effect_count; ++index) {
        AEGP_EffectRefH candidate = nullptr;
        error = suites.effect->AEGP_GetLayerEffectByIndex(plugin_id, layer, index, &candidate);
        if (error) return static_cast<PF_Err>(error);
        AEGP_InstalledEffectKey key = 0;
        char match_name[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
        error = suites.effect->AEGP_GetInstalledKeyFromLayerEffect(candidate, &key);
        if (!error) error = suites.effect->AEGP_GetEffectMatchName(key, match_name);
        const bool is_renderer = !error && std::strcmp(match_name, kRendererMatchName) == 0;
        if (is_renderer && renderer) {
            suites.effect->AEGP_DisposeEffect(candidate);
            suites.effect->AEGP_DisposeEffect(renderer);
            renderer = nullptr;
            return PF_Err_BAD_CALLBACK_PARAM;
        }
        if (is_renderer) renderer = candidate;
        else suites.effect->AEGP_DisposeEffect(candidate);
        if (error) {
            if (renderer) suites.effect->AEGP_DisposeEffect(renderer);
            renderer = nullptr;
            return static_cast<PF_Err>(error);
        }
    }
    return PF_Err_NONE;
}

bool capture_edit(PF_InData& data, const PF_ParamDef& param,
                  starfield::adapter::node_sync::NativeEdit& edit) noexcept {
    using starfield::adapter::node_sync::ValueKind;
    const auto scale = [](PF_RationalScale value) {
        return value.num > 0 && value.den > 0 ? static_cast<double>(value.den) / value.num : 1.0;
    };
    switch (param.param_type) {
        case PF_Param_FLOAT_SLIDER: edit.value[0] = param.u.fs_d.value; break;
        case PF_Param_POPUP: edit.value[0] = param.u.pd.value; break;
        case PF_Param_CHECKBOX: edit.value[0] = param.u.bd.value; break;
        case PF_Param_ANGLE: edit.value[0] = param.u.ad.value / 65536.0; break;
        case PF_Param_POINT:
            edit.value_kind = ValueKind::point2;
            edit.value[0] = param.u.td.x_value / 65536.0 * scale(data.downsample_x);
            edit.value[1] = param.u.td.y_value / 65536.0 * scale(data.downsample_y);
            break;
        case PF_Param_POINT_3D:
            edit.value_kind = ValueKind::point3;
            edit.value[0] = param.u.point3d_d.x_value * scale(data.downsample_x);
            edit.value[1] = param.u.point3d_d.y_value * scale(data.downsample_y);
            edit.value[2] = param.u.point3d_d.z_value;
            break;
        case PF_Param_COLOR:
            edit.value_kind = ValueKind::color;
            edit.value = {param.u.cd.value.red / 255.0, param.u.cd.value.green / 255.0,
                          param.u.cd.value.blue / 255.0, param.u.cd.value.alpha / 255.0};
            break;
        default: return false;
    }
    return starfield::adapter::node_sync::valid_edit(edit);
}

PF_Err complete_context(starfield::adapter::node_sync::NativeEdit& edit) noexcept {
    if (edit.width > 0 && edit.height > 0 && edit.pixel_aspect.num > 0 &&
        edit.pixel_aspect.den > 0 && edit.time_scale > 0) return PF_Err_NONE;
    const AEGP_LayerSuite9* layers = nullptr;
    const AEGP_ItemSuite9* items = nullptr;
    A_Err error = edit.basic->AcquireSuite(kAEGPLayerSuite, kAEGPLayerSuiteVersion9,
        reinterpret_cast<const void**>(&layers));
    const bool geometry_missing = edit.width <= 0 || edit.height <= 0 ||
        edit.pixel_aspect.num <= 0 || edit.pixel_aspect.den <= 0;
    if (!error && geometry_missing) error = edit.basic->AcquireSuite(kAEGPItemSuite, kAEGPItemSuiteVersion9,
        reinterpret_cast<const void**>(&items));
    if (!error && geometry_missing) {
        AEGP_ItemH item = nullptr; A_Ratio aspect{};
        error = layers->AEGP_GetLayerSourceItem(edit.layer, &item);
        if (!error && !item) error = PF_Err_BAD_CALLBACK_PARAM;
        if (!error) error = items->AEGP_GetItemDimensions(item, &edit.width, &edit.height);
        if (!error) error = items->AEGP_GetItemPixelAspectRatio(item, &aspect);
        if (!error) edit.pixel_aspect = {aspect.num, aspect.den};
    }
    if (!error && edit.time_scale <= 0) {
        A_Time time{};
        error = layers->AEGP_GetLayerCurrentTime(edit.layer, AEGP_LTimeMode_LayerTime, &time);
        if (!error) { edit.time = time.value; edit.time_scale = static_cast<A_long>(time.scale); }
    }
    if (items) edit.basic->ReleaseSuite(kAEGPItemSuite, kAEGPItemSuiteVersion9);
    if (layers) edit.basic->ReleaseSuite(kAEGPLayerSuite, kAEGPLayerSuiteVersion9);
    return static_cast<PF_Err>(error);
}

} // namespace

PF_Err register_node_graph_sync(PF_InData* in_data) noexcept {
    if (!in_data || !in_data->pica_basicP) return PF_Err_BAD_CALLBACK_PARAM;
    if (g_plugin_id.load(std::memory_order_acquire) != 0) return PF_Err_NONE;
    std::lock_guard lock(g_registration_mutex);
    if (g_plugin_id.load(std::memory_order_relaxed) != 0) return PF_Err_NONE;
    const AEGP_UtilitySuite6* utility = nullptr;
    A_Err error = in_data->pica_basicP->AcquireSuite(kAEGPUtilitySuite, kAEGPUtilitySuiteVersion6,
                                                      reinterpret_cast<const void**>(&utility));
    AEGP_PluginID plugin_id = 0;
    if (!error && utility) error = utility->AEGP_RegisterWithAEGP(nullptr, kRegistrationName, &plugin_id);
    if (utility) in_data->pica_basicP->ReleaseSuite(kAEGPUtilitySuite, kAEGPUtilitySuiteVersion6);
    if (!error && plugin_id != 0) {
        g_plugin_id.store(plugin_id, std::memory_order_release);
        return PF_Err_NONE;
    }
    return static_cast<PF_Err>(error ? error : PF_Err_BAD_CALLBACK_PARAM);
}

PF_Err sync_node_graph_parameter(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[],
                                const PF_UserChangedParamExtra* extra) noexcept {
    if (!in_data || !params || !extra || extra->param_index <= 0 ||
        extra->param_index > kLastParameterIndex) return PF_Err_NONE;
    const PF_ParamDef* guard = params[kSyncGuardIndex];
    if (guard && guard->param_type == PF_Param_FLOAT_SLIDER && guard->u.fs_d.value != 0.0) return PF_Err_NONE;
    const PF_ParamDef* changed = params[extra->param_index];
    if (!changed) return PF_Err_NONE;

    try {
        // uu.id is a setup-only union member. During USER_CHANGED_PARAM it
        // aliases change_flags, so testing it silently drops native edits.
        // Node records follow the authored parameters in the runtime layout.
        constexpr auto kind = static_cast<starfield::adapter::native_nodes::Kind>(kNodeKind);
        if (extra->param_index > starfield::adapter::native_nodes::base_parameter_count(kind)) return PF_Err_NONE;
        std::array<std::uint16_t, 8> node_id{};
        if (!read_node_id(params, node_id)) return PF_Err_BAD_CALLBACK_PARAM;
        starfield::adapter::node_sync::NativeEdit edit;
        edit.node_kind = kNodeKind;
        edit.parameter_index = extra->param_index;
        edit.uuid = node_id;
        if (!capture_edit(*in_data, *changed, edit)) return PF_Err_BAD_CALLBACK_PARAM;

        const AEGP_PluginID plugin_id = g_plugin_id.load(std::memory_order_acquire);
        if (plugin_id == 0 || !in_data->pica_basicP || !in_data->effect_ref) return PF_Err_BAD_CALLBACK_PARAM;
        SuiteSet suites(in_data);
        PF_Err error = suites.acquire();
        if (error != PF_Err_NONE) return error;

        AEGP_LayerH layer = nullptr;
        error = static_cast<PF_Err>(suites.pf_interface->AEGP_GetEffectLayer(in_data->effect_ref, &layer));
        if (error || !layer) return error ? error : PF_Err_BAD_CALLBACK_PARAM;

        AEGP_EffectRefH renderer = nullptr;
        error = locate_renderer(suites, plugin_id, layer, renderer);
        if (error != PF_Err_NONE) return error;
        if (!renderer) return PF_Err_NONE;
        edit.renderer = renderer; edit.layer = layer;
        edit.handles = in_data->utils; edit.basic = in_data->pica_basicP;
        edit.width = in_data->width; edit.height = in_data->height;
        edit.time = in_data->current_time; edit.time_scale = static_cast<A_long>(in_data->time_scale);
        edit.pixel_aspect = in_data->pixel_aspect_ratio;
        // The host may not have saved this callback's value into its stream yet.
        // Apply it locally by UUID and explicitly publish the renderer snapshot.
        error = complete_context(edit);
        if (error) edit.stage = starfield::adapter::node_sync::Stage::context;
        else {
            error = starfield::adapter::commit_native_graph_edit(&edit, plugin_id, out_data);
        }
        suites.effect->AEGP_DisposeEffect(renderer);
        if (!error) error = edit.accepted && edit.revision > 0 ? edit.status :
            (edit.status ? edit.status : PF_Err_BAD_CALLBACK_PARAM);
        if (out_data) {
            if (!error) out_data->out_flags |= PF_OutFlag_FORCE_RERENDER;
            else std::snprintf(out_data->return_msg, sizeof(out_data->return_msg),
                "Starfield native edit failed: %s (parameter %ld, stream %ld, error %d).",
                starfield::adapter::node_sync::stage_name(edit.stage), static_cast<long>(extra->param_index),
                static_cast<long>(edit.stream_index), static_cast<int>(error));
        }
        return error;
    } catch (const std::bad_alloc&) {
        return PF_Err_OUT_OF_MEMORY;
    } catch (...) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
}
