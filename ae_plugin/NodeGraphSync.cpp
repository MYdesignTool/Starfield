#include "NodeEffects.hpp"

#include "AE_GeneralPlug.h"
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
constexpr A_long kLastParameterIndex = 32;
constexpr A_long kNodeKind = 0;
#elif defined(STARFIELD_NODE_KIND_PARTICLE)
constexpr char kRegistrationName[] = "Starfield Particle Node Sync";
constexpr A_long kLastParameterIndex = 54;
constexpr A_long kNodeKind = 1;
#elif defined(STARFIELD_NODE_KIND_APPEARANCE)
constexpr char kRegistrationName[] = "Starfield Appearance Node Sync";
constexpr A_long kLastParameterIndex = 53;
constexpr A_long kNodeKind = 2;
#elif defined(STARFIELD_NODE_KIND_FORCE)
constexpr char kRegistrationName[] = "Starfield Force Node Sync";
constexpr A_long kLastParameterIndex = 13;
constexpr A_long kNodeKind = 3;
#else
#error Define exactly one STARFIELD_NODE_KIND_* for each node module.
#endif

constexpr A_long kIdentityFirstIndex = kLastParameterIndex - 8;
constexpr A_long kSyncGuardIndex = kLastParameterIndex;
constexpr A_long kNodeEditNonce = 0;
constexpr char kRendererMatchName[] = "org.starfieldfx.particle";
constexpr char kHexDigits[] = "0123456789abcdef";

constexpr A_long fourcc(char a, char b, char c, char d) noexcept {
    return (static_cast<A_long>(static_cast<unsigned char>(a)) << 24) |
           (static_cast<A_long>(static_cast<unsigned char>(b)) << 16) |
           (static_cast<A_long>(static_cast<unsigned char>(c)) << 8) |
           static_cast<A_long>(static_cast<unsigned char>(d));
}

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
    if (changed_id != fourcc('v', 'e', 'l', 'x') && changed_id != fourcc('v', 'e', 'l', 'y') &&
        changed_id != fourcc('v', 'e', 'l', 'z')) return false;
    starfield::core::Vec3 value{};
    if (!read_slider(params, fourcc('v', 'e', 'l', 'x'), value.x) ||
        !read_slider(params, fourcc('v', 'e', 'l', 'y'), value.y) ||
        !read_slider(params, fourcc('v', 'e', 'l', 'z'), value.z) || !encode_vector3(value, encoded.payload)) return false;
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
    constexpr A_long id = fourcc('s', 'e', 'e', 'd');
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
        return set_scalar(params, changed_id, fourcc('b', 'r', 't', 'h'), 2, encoded) ||
               set_seed(params, changed_id, encoded) ||
               set_emitter_popup(params, changed_id, fourcc('e', 's', 'h', 'a'), 5, encoded) ||
               set_vector_parameter(params, changed_id, fourcc('e', 'p', 'o', 's'), 6, encoded) ||
               set_velocity(params, changed_id, encoded) ||
               set_scalar(params, changed_id, fourcc('p', 's', 'i', 'z'), 8, encoded) ||
               set_scalar(params, changed_id, fourcc('o', 'p', 'a', 'c'), 9, encoded) ||
               set_scalar(params, changed_id, fourcc('d', 's', 'i', 'z'), 10, encoded) ||
               set_scalar(params, changed_id, fourcc('v', 's', 'p', 'd'), 11, encoded) ||
               set_scalar(params, changed_id, fourcc('e', 'm', 's', 'p'), 12, encoded) ||
               set_scalar(params, changed_id, fourcc('e', 'm', 'r', 'd'), 13, encoded) ||
               set_scalar(params, changed_id, fourcc('e', 'a', 'n', 'x'), 14, encoded) ||
               set_scalar(params, changed_id, fourcc('e', 'a', 'n', 'y'), 15, encoded) ||
               set_scalar(params, changed_id, fourcc('e', 'a', 'n', 'z'), 16, encoded) ||
               set_emitter_popup(params, changed_id, fourcc('d', 'i', 'r', 'm'), 17, encoded) ||
               set_scalar(params, changed_id, fourcc('d', 's', 'p', 'n'), 18, encoded) ||
               set_scalar(params, changed_id, fourcc('e', 's', 'z', 'x'), 19, encoded) ||
               set_scalar(params, changed_id, fourcc('e', 's', 'z', 'y'), 20, encoded) ||
               set_scalar(params, changed_id, fourcc('e', 's', 'z', 'z'), 21, encoded);
    } else if constexpr (kNodeKind == 1 || kNodeKind == 2) {
        return set_color(params, changed_id, fourcc('c', 'l', 'r', 's'), 1, encoded) ||
               set_color(params, changed_id, fourcc('c', 'l', 'r', 'e'), 2, encoded) ||
               set_scalar(params, changed_id, fourcc('s', 'i', 'z', 'e'), 3, encoded) ||
               set_scalar(params, changed_id, fourcc('s', 'z', 'e', 'n'), 4, encoded) ||
               set_scalar(params, changed_id, fourcc('o', 'p', 'a', 'c'), 5, encoded) ||
               set_scalar(params, changed_id, fourcc('o', 'p', 'e', 'n'), 6, encoded) ||
               (kNodeKind == 1 && set_scalar(params, changed_id, fourcc('l', 'i', 'f', 'e'), 11, encoded)) ||
               set_scalar(params, changed_id, fourcc('s', 'z', 'r', 'd'), 9, encoded) ||
               set_scalar(params, changed_id, fourcc('o', 'p', 'r', 'd'), 10, encoded);
    } else {
        return set_vector_parameter(params, changed_id, fourcc('g', 'r', 'a', 'v'), 1, encoded) ||
               set_scalar(params, changed_id, fourcc('d', 'r', 'a', 'g'), 2, encoded);
    }
}

bool read_node_id(PF_ParamDef* params[], std::array<std::uint16_t, 8>& chunks) noexcept {
    bool nonzero = false;
    for (A_long chunk = 0; chunk < 8; ++chunk) {
        const PF_ParamDef* parameter = params[kIdentityFirstIndex + chunk];
        if (!parameter || parameter->param_type != PF_Param_FLOAT_SLIDER ||
            parameter->uu.id != fourcc('u', 'i', 'd', static_cast<char>('0' + chunk)) ||
            !std::isfinite(parameter->u.fs_d.value) || parameter->u.fs_d.value < 0.0 ||
            parameter->u.fs_d.value > 65535.0 || std::floor(parameter->u.fs_d.value) != parameter->u.fs_d.value) {
            return false;
        }
        chunks[static_cast<std::size_t>(chunk)] = static_cast<std::uint16_t>(parameter->u.fs_d.value);
        nonzero = nonzero || chunks[static_cast<std::size_t>(chunk)] != 0;
    }
    return nonzero;
}

std::string make_node_edit_expression(const std::array<std::uint16_t, 8>& chunks,
                                      const EncodedValue& value) {
    std::string expression(starfield::adapter::node_sync::kRequestPrefix);
    expression += std::to_string(kNodeEditNonce);
    expression.push_back(':');
    for (const std::uint16_t chunk : chunks) {
        expression.push_back(kHexDigits[(chunk >> 12u) & 0x0fu]);
        expression.push_back(kHexDigits[(chunk >> 8u) & 0x0fu]);
        expression.push_back(kHexDigits[(chunk >> 4u) & 0x0fu]);
        expression.push_back(kHexDigits[chunk & 0x0fu]);
    }
    expression.push_back(':');
    char key_buffer[24]{};
    const auto key_result = std::to_chars(key_buffer, key_buffer + sizeof(key_buffer), value.key);
    if (key_result.ec != std::errc{}) return {};
    expression.append(key_buffer, key_result.ptr);
    expression.push_back(':');
    expression.push_back(static_cast<char>(value.kind));
    expression.push_back(':');
    expression += value.payload;
    expression += starfield::adapter::node_sync::kRequestSuffix;
    if (expression.size() > starfield::adapter::node_sync::kMaxRequestBytes) return {};
    return expression;
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

PF_Err write_request_expression(SuiteSet& suites, AEGP_PluginID plugin_id,
                                AEGP_EffectRefH renderer, const std::string& expression) {
    AEGP_StreamRefH request_stream = nullptr;
    A_Err error = suites.stream->AEGP_GetNewEffectStreamByIndex(
        plugin_id, renderer, starfield::adapter::node_sync::kGraphRequestStreamIndex, &request_stream);
    if (error || !request_stream) return static_cast<PF_Err>(error ? error : PF_Err_BAD_CALLBACK_PARAM);

    std::vector<A_UTF16Char> characters;
    characters.reserve(expression.size() + 1u);
    for (const unsigned char character : expression) {
        if (character > 0x7fu) {
            suites.stream->AEGP_DisposeStream(request_stream);
            return PF_Err_BAD_CALLBACK_PARAM;
        }
        characters.push_back(static_cast<A_UTF16Char>(character));
    }
    characters.push_back(0);
    error = suites.stream->AEGP_SetExpression(plugin_id, request_stream, characters.data());
    if (!error) error = suites.stream->AEGP_SetExpressionState(plugin_id, request_stream, FALSE);
    suites.stream->AEGP_DisposeStream(request_stream);
    return static_cast<PF_Err>(error);
}

PF_Err request_renderer_commit(PF_InData* in_data, SuiteSet& suites, AEGP_PluginID plugin_id,
                               AEGP_EffectRefH renderer) noexcept {
    PF_UserChangedParamExtra extra{};
    extra.param_index = starfield::adapter::node_sync::kGraphCommitStreamIndex;
    const A_Time time{in_data->current_time, in_data->time_scale};
    const A_Err error = suites.effect->AEGP_EffectCallGeneric(
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
        const std::string request = make_node_edit_expression(node_id, value);
        if (request.empty()) return PF_Err_BAD_CALLBACK_PARAM;

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
        error = write_request_expression(suites, plugin_id, renderer, request);
        if (!error) error = request_renderer_commit(in_data, suites, plugin_id, renderer);
        suites.effect->AEGP_DisposeEffect(renderer);
        return error;
    } catch (const std::bad_alloc&) {
        return PF_Err_OUT_OF_MEMORY;
    } catch (...) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
}
