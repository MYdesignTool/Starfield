#include "NodeEffects.hpp"

#include "AE_GeneralPlug.h"
#include "NodeRecord.hpp"
#include "NodeGraphSync.hpp"
#include "SPBasic.h"
#include "starfield/core/Graph.hpp"

#include <array>
#include <atomic>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>
#include <string>
#include <system_error>
#include <vector>

namespace {

using starfield::adapter::node_sync::ValueKind;

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
        if (stream) basic->ReleaseSuite(kAEGPStreamSuite, kAEGPStreamSuiteVersion6);
        if (effect) basic->ReleaseSuite(kAEGPEffectSuite, kAEGPEffectSuiteVersion4);
        if (pf_interface) basic->ReleaseSuite(kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
    }
    PF_Err acquire() noexcept {
        if (!basic) return PF_Err_BAD_CALLBACK_PARAM;
        A_Err error = basic->AcquireSuite(kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1,
                                          reinterpret_cast<const void**>(&pf_interface));
        if (!error) error = basic->AcquireSuite(kAEGPEffectSuite, kAEGPEffectSuiteVersion4,
                                                 reinterpret_cast<const void**>(&effect));
        if (!error) error = basic->AcquireSuite(kAEGPStreamSuite, kAEGPStreamSuiteVersion6,
                                                 reinterpret_cast<const void**>(&stream));
        return static_cast<PF_Err>(error);
    }
    SPBasicSuite* basic{};
    const AEGP_PFInterfaceSuite1* pf_interface{};
    const AEGP_EffectSuite4* effect{};
    const AEGP_StreamSuite6* stream{};
};

struct EncodedValue {
    std::uint64_t key{};
    ValueKind kind{ValueKind::float64};
    std::string payload;
};

PF_ParamDef* parameter_by_id(PF_ParamDef* params[], A_long id) noexcept {
    if (!params) return nullptr;
    for (A_long index = 1; index <= kLastParameterIndex; ++index) {
        if (params[index] && params[index]->uu.id == id) return params[index];
    }
    return nullptr;
}

bool encode_double(double value, std::string& output) {
    if (!std::isfinite(value)) return false;
    char buffer[64]{};
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value,
                                      std::chars_format::general,
                                      std::numeric_limits<double>::max_digits10);
    if (result.ec != std::errc{}) return false;
    output.assign(buffer, result.ptr);
    return true;
}

bool read_slider(PF_ParamDef* params[], A_long id, double& value) noexcept {
    const PF_ParamDef* parameter = parameter_by_id(params, id);
    if (!parameter || parameter->param_type != PF_Param_FLOAT_SLIDER ||
        !std::isfinite(parameter->u.fs_d.value)) return false;
    value = parameter->u.fs_d.value;
    return true;
}

bool read_vector3(PF_ParamDef* params[], A_long id, starfield::core::Vec3& value) noexcept {
    const PF_ParamDef* parameter = parameter_by_id(params, id);
    if (!parameter || parameter->param_type != PF_Param_POINT_3D) return false;
    value = {parameter->u.point3d_d.x_value, parameter->u.point3d_d.y_value,
             parameter->u.point3d_d.z_value};
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool encode_vector3(const starfield::core::Vec3& value, std::string& output) {
    std::string x, y, z;
    if (!encode_double(value.x, x) || !encode_double(value.y, y) || !encode_double(value.z, z)) return false;
    output = std::move(x);
    output.push_back(',');
    output += y;
    output.push_back(',');
    output += z;
    return true;
}

bool set_scalar(PF_ParamDef* params[], A_long changed_id, A_long parameter_id,
                std::uint64_t key, EncodedValue& encoded) {
    if (changed_id != parameter_id) return false;
    double value = 0.0;
    if (!read_slider(params, parameter_id, value) || !encode_double(value, encoded.payload)) return false;
    encoded.key = key;
    encoded.kind = ValueKind::float64;
    return true;
}

bool set_vector_parameter(PF_ParamDef* params[], A_long changed_id, A_long parameter_id,
                          std::uint64_t key, EncodedValue& encoded) {
    if (changed_id != parameter_id) return false;
    starfield::core::Vec3 value{};
    if (!read_vector3(params, parameter_id, value) || !encode_vector3(value, encoded.payload)) return false;
    encoded.key = key;
    encoded.kind = ValueKind::vector3;
    return true;
}

bool set_velocity(PF_ParamDef* params[], A_long changed_id, EncodedValue& encoded) {
    if (changed_id != kVelocityXId && changed_id != kVelocityYId &&
        changed_id != kVelocityZId) return false;
    starfield::core::Vec3 value{};
    if (!read_slider(params, kVelocityXId, value.x) ||
        !read_slider(params, kVelocityYId, value.y) ||
        !read_slider(params, kVelocityZId, value.z) || !encode_vector3(value, encoded.payload)) return false;
    encoded.key = 7;
    encoded.kind = ValueKind::vector3;
    return true;
}

bool set_emitter_popup(PF_ParamDef* params[], A_long changed_id, A_long parameter_id,
                       std::uint64_t key, EncodedValue& encoded) {
    if (changed_id != parameter_id) return false;
    const PF_ParamDef* parameter = parameter_by_id(params, parameter_id);
    if (!parameter || parameter->param_type != PF_Param_POPUP || parameter->u.pd.value < 1) return false;
    const auto value = static_cast<std::uint32_t>(parameter->u.pd.value - 1);
    char buffer[16]{};
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (result.ec != std::errc{}) return false;
    encoded.key = key;
    encoded.kind = ValueKind::uint32;
    encoded.payload.assign(buffer, result.ptr);
    return true;
}

bool set_seed(PF_ParamDef* params[], A_long changed_id, EncodedValue& encoded) {
    constexpr A_long id = kSeedId;
    if (changed_id != id) return false;
    double value = 0.0;
    if (!read_slider(params, id, value) || value < 0.0 || value > std::numeric_limits<std::uint32_t>::max() ||
        std::floor(value) != value) return false;
    char buffer[16]{};
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), static_cast<std::uint32_t>(value));
    if (result.ec != std::errc{}) return false;
    encoded.key = 3;
    encoded.kind = ValueKind::uint32;
    encoded.payload.assign(buffer, result.ptr);
    return true;
}

bool set_color(PF_ParamDef* params[], A_long changed_id, A_long parameter_id,
               std::uint64_t key, EncodedValue& encoded) {
    if (changed_id != parameter_id) return false;
    const PF_ParamDef* parameter = parameter_by_id(params, parameter_id);
    if (!parameter || parameter->param_type != PF_Param_COLOR) return false;
    const PF_Pixel color = parameter->u.cd.value;
    const starfield::core::Vec3 value{color.red / 255.0, color.green / 255.0, color.blue / 255.0};
    if (!encode_vector3(value, encoded.payload)) return false;
    encoded.key = key;
    encoded.kind = ValueKind::vector3;
    return true;
}

bool map_parameter_edit(PF_ParamDef* params[], A_long changed_id, EncodedValue& encoded) {
    if constexpr (kNodeKind == 0) {
        return set_scalar(params, changed_id, kBirthRateId, 2, encoded) ||
               set_seed(params, changed_id, encoded) ||
               set_emitter_popup(params, changed_id, kEmitterTypeId, 5, encoded) ||
               set_vector_parameter(params, changed_id, kOriginId, 6, encoded) ||
               set_velocity(params, changed_id, encoded) ||
               set_scalar(params, changed_id, kEmitterParticleSizeId, 8, encoded) ||
               set_scalar(params, changed_id, kOpacityId, 9, encoded) ||
               set_scalar(params, changed_id, kDiscSizeId, 10, encoded) ||
               set_scalar(params, changed_id, kSpeedRandomId, 11, encoded) ||
               set_scalar(params, changed_id, kEmissionSpeedId, 12, encoded) ||
               set_scalar(params, changed_id, kEmissionSpeedRandomId, 13, encoded) ||
               set_scalar(params, changed_id, kEmissionAngleXId, 14, encoded) ||
               set_scalar(params, changed_id, kEmissionAngleYId, 15, encoded) ||
               set_scalar(params, changed_id, kEmissionAngleZId, 16, encoded) ||
               set_emitter_popup(params, changed_id, kDirectionId, 17, encoded) ||
               set_scalar(params, changed_id, kDirectionSpanId, 18, encoded) ||
               set_scalar(params, changed_id, kEmitterSizeXId, 19, encoded) ||
               set_scalar(params, changed_id, kEmitterSizeYId, 20, encoded) ||
               set_scalar(params, changed_id, kEmitterSizeZId, 21, encoded);
    } else if constexpr (kNodeKind == 1 || kNodeKind == 2) {
        return set_color(params, changed_id, kColorStartId, 1, encoded) ||
               set_color(params, changed_id, kColorEndId, 2, encoded) ||
               set_scalar(params, changed_id, kSizeId, 3, encoded) ||
               set_scalar(params, changed_id, kSizeOverLifeId, 4, encoded) ||
               set_scalar(params, changed_id, kOpacityId, 5, encoded) ||
               set_scalar(params, changed_id, kOpacityOverLifeId, 6, encoded) ||
               (kNodeKind == 1 && set_scalar(params, changed_id, kLifetimeId, 11, encoded)) ||
               set_scalar(params, changed_id, kSizeRandomId, 9, encoded) ||
               set_scalar(params, changed_id, kOpacityRandomId, 10, encoded);
    } else {
        return set_vector_parameter(params, changed_id, kGravityId, 1, encoded) ||
               set_scalar(params, changed_id, kDragId, 2, encoded);
    }
}

bool read_node_id(PF_ParamDef* params[], std::array<std::uint16_t, 8>& chunks) noexcept {
    bool nonzero = false;
    for (A_long chunk = 0; chunk < 8; ++chunk) {
        const PF_ParamDef* parameter = params[kIdentityFirstIndex + chunk];
        if (!parameter || parameter->param_type != PF_Param_FLOAT_SLIDER ||
            parameter->uu.id != uuid_id(chunk) ||
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

PF_Err request_renderer_compile(PF_InData* in_data, SuiteSet& suites, AEGP_PluginID plugin_id,
                                AEGP_EffectRefH renderer) noexcept {
    AEGP_StreamRefH raw_stream = nullptr;
    A_Err error = suites.stream->AEGP_GetNewEffectStreamByIndex(
        plugin_id, renderer, starfield::adapter::node_sync::kGraphCommitStreamIndex, &raw_stream);
    if (error || !raw_stream) return static_cast<PF_Err>(error ? error : PF_Err_BAD_CALLBACK_PARAM);
    AEGP_StreamValue2 value{};
    const A_Time time{in_data->current_time, in_data->time_scale};
    error = suites.stream->AEGP_GetNewStreamValue(plugin_id, raw_stream, AEGP_LTimeMode_LayerTime,
                                                  &time, TRUE, &value);
    if (!error) {
        const double current = value.val.one_d;
        if (!std::isfinite(current) || current < 0.0 || current > 1000000.0 || std::floor(current) != current) {
            error = PF_Err_BAD_CALLBACK_PARAM;
        } else {
            const A_long nonce = current >= 1000000.0 ? 1 : static_cast<A_long>(current) + 1;
            value.val.one_d = static_cast<A_FpLong>(nonce);
            error = suites.stream->AEGP_SetStreamValue(plugin_id, raw_stream, &value);
        }
        suites.stream->AEGP_DisposeStreamValue(&value);
    }
    suites.stream->AEGP_DisposeStream(raw_stream);
    if (error) return static_cast<PF_Err>(error);

    PF_UserChangedParamExtra extra{};
    extra.param_index = starfield::adapter::node_sync::kGraphCommitStreamIndex;
    error = suites.effect->AEGP_EffectCallGeneric(
        plugin_id, renderer, &time, PF_Cmd_USER_CHANGED_PARAM, &extra);
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
    (void)out_data;
    if (!in_data || !params || !extra || extra->param_index <= 0 ||
        extra->param_index > kLastParameterIndex) return PF_Err_NONE;
    const PF_ParamDef* guard = params[kSyncGuardIndex];
    if (guard && guard->param_type == PF_Param_FLOAT_SLIDER && guard->u.fs_d.value != 0.0) return PF_Err_NONE;
    const PF_ParamDef* changed = params[extra->param_index];
    if (!changed) return PF_Err_NONE;

    try {
        EncodedValue value;
        if (!map_parameter_edit(params, changed->uu.id, value)) return PF_Err_NONE;
        std::array<std::uint16_t, 8> node_id{};
        if (!read_node_id(params, node_id)) return PF_Err_BAD_CALLBACK_PARAM;
        (void)node_id;
        (void)value;

        const AEGP_PluginID plugin_id = g_plugin_id.load(std::memory_order_acquire);
        if (plugin_id == 0 || !in_data->pica_basicP || !in_data->effect_ref) return PF_Err_NONE;
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
        // The node effect is the source of truth. Ask the renderer to recompile
        // from all sibling node streams; never serialize a partial edit through
        // an expression mailbox on the main effect.
        error = request_renderer_compile(in_data, suites, plugin_id, renderer);
        suites.effect->AEGP_DisposeEffect(renderer);
        return error;
    } catch (const std::bad_alloc&) {
        return PF_Err_OUT_OF_MEMORY;
    } catch (...) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
}
