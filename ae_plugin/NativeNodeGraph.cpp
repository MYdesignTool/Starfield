#include "AEConfig.h"
#include "NativeNodeGraph.hpp"

#include "AE_EffectCB.h"
#include "AE_GeneralPlug.h"
#include "NodeRecord.hpp"
#include "NodeGraphSync.hpp"
#include "Parameters.hpp"
#include "SPBasic.h"

#include "starfield/core/AgeCurve.hpp"
#include "starfield/core/Geometry.hpp"

#include <array>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace starfield::adapter {
namespace {

namespace core = starfield::core;
using native_nodes::Kind;

constexpr char kRendererMatchName[] = "org.starfieldfx.particle";
constexpr char kEmitterMatchName[] = "org.starfieldfx.node.emitter";
constexpr char kParticleMatchName[] = "org.starfieldfx.node.particle";
constexpr char kAppearanceMatchName[] = "org.starfieldfx.node.appearance";
constexpr char kForceMatchName[] = "org.starfieldfx.node.force";

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
    const node_sync::NativeEdit* edit{};
    AEGP_EffectRefH edited_effect{};
    bool edit_applied{};
    bool edit_matched{};
    bool fail(A_long index) const noexcept {
        if (edit) edit->stream_index = index;
        return false;
    }
};

struct EffectRef {
    const AEGP_EffectSuite4* suite{};
    AEGP_EffectRefH value{};
    ~EffectRef() { if (suite && value) suite->AEGP_DisposeEffect(value); }
    EffectRef(const EffectRef&) = delete;
    EffectRef& operator=(const EffectRef&) = delete;
    EffectRef(const AEGP_EffectSuite4* effect, AEGP_EffectRefH ref) : suite(effect), value(ref) {}
};

struct StreamRef {
    const AEGP_StreamSuite6* suite{};
    AEGP_StreamRefH value{};
    ~StreamRef() { if (suite && value) suite->AEGP_DisposeStream(value); }
    StreamRef(const AEGP_StreamSuite6* stream, AEGP_StreamRefH ref) : suite(stream), value(ref) {}
    StreamRef(const StreamRef&) = delete;
    StreamRef& operator=(const StreamRef&) = delete;
};

bool read_one_d(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                A_long index, const A_Time& time, double& output) noexcept {
    if (suites.edit && effect == suites.edited_effect && index == suites.edit->parameter_index) {
        if (suites.edit->value_kind != node_sync::ValueKind::scalar) return suites.fail(index);
        suites.edit_applied = true; output = suites.edit->value[0]; return true;
    }
    AEGP_StreamRefH raw_stream = nullptr;
    A_Err error = suites.stream->AEGP_GetNewEffectStreamByIndex(plugin_id, effect, index, &raw_stream);
    if (error || !raw_stream) return suites.fail(index);
    StreamRef stream(suites.stream, raw_stream);
    AEGP_StreamValue2 value{};
    error = suites.stream->AEGP_GetNewStreamValue(plugin_id, stream.value, AEGP_LTimeMode_LayerTime,
                                                  &time, TRUE, &value);
    if (error) return suites.fail(index);
    const double result = value.val.one_d;
    suites.stream->AEGP_DisposeStreamValue(&value);
    if (!std::isfinite(result)) return suites.fail(index);
    output = result;
    return true;
}

bool read_two_d(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                A_long index, const A_Time& time, core::Vec3& output) noexcept {
    if (suites.edit && effect == suites.edited_effect && index == suites.edit->parameter_index) {
        if (suites.edit->value_kind != node_sync::ValueKind::point2) return suites.fail(index);
        suites.edit_applied = true; output = {suites.edit->value[0], suites.edit->value[1], 0.0}; return true;
    }
    AEGP_StreamRefH raw_stream = nullptr;
    A_Err error = suites.stream->AEGP_GetNewEffectStreamByIndex(plugin_id, effect, index, &raw_stream);
    if (error || !raw_stream) return suites.fail(index);
    StreamRef stream(suites.stream, raw_stream);
    AEGP_StreamValue2 value{};
    error = suites.stream->AEGP_GetNewStreamValue(plugin_id, stream.value, AEGP_LTimeMode_LayerTime,
                                                &time, TRUE, &value);
    if (error) return suites.fail(index);
    const core::Vec3 result{value.val.two_d.x, value.val.two_d.y, 0.0};
    suites.stream->AEGP_DisposeStreamValue(&value);
    if (!std::isfinite(result.x) || !std::isfinite(result.y)) return suites.fail(index);
    output = result;
    return true;
}

bool read_three_d(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                  A_long index, const A_Time& time, core::Vec3& output) noexcept {
    if (suites.edit && effect == suites.edited_effect && index == suites.edit->parameter_index) {
        if (suites.edit->value_kind != node_sync::ValueKind::point3) return suites.fail(index);
        suites.edit_applied = true; output = {suites.edit->value[0], suites.edit->value[1], suites.edit->value[2]}; return true;
    }
    AEGP_StreamRefH raw_stream = nullptr;
    A_Err error = suites.stream->AEGP_GetNewEffectStreamByIndex(plugin_id, effect, index, &raw_stream);
    if (error || !raw_stream) return suites.fail(index);
    StreamRef stream(suites.stream, raw_stream);
    AEGP_StreamValue2 value{};
    error = suites.stream->AEGP_GetNewStreamValue(plugin_id, stream.value, AEGP_LTimeMode_LayerTime,
                                                  &time, TRUE, &value);
    if (error) return suites.fail(index);
    const core::Vec3 result{value.val.three_d.x, value.val.three_d.y, value.val.three_d.z};
    suites.stream->AEGP_DisposeStreamValue(&value);
    if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) return suites.fail(index);
    output = result;
    return true;
}

bool read_color(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                A_long index, const A_Time& time, core::Vec3& output) noexcept {
    if (suites.edit && effect == suites.edited_effect && index == suites.edit->parameter_index) {
        if (suites.edit->value_kind != node_sync::ValueKind::color) return suites.fail(index);
        suites.edit_applied = true; output = {suites.edit->value[0], suites.edit->value[1], suites.edit->value[2]}; return true;
    }
    AEGP_StreamRefH raw_stream = nullptr;
    A_Err error = suites.stream->AEGP_GetNewEffectStreamByIndex(plugin_id, effect, index, &raw_stream);
    if (error || !raw_stream) return suites.fail(index);
    StreamRef stream(suites.stream, raw_stream);
    AEGP_StreamValue2 value{};
    error = suites.stream->AEGP_GetNewStreamValue(plugin_id, stream.value, AEGP_LTimeMode_LayerTime,
                                                  &time, TRUE, &value);
    if (error) return suites.fail(index);
    const core::Vec3 result{value.val.color.redF, value.val.color.greenF, value.val.color.blueF};
    suites.stream->AEGP_DisposeStreamValue(&value);
    if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) return suites.fail(index);
    output = result;
    return true;
}

bool read_uint(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
               A_long index, const A_Time& time, std::uint32_t& output) noexcept {
    double value = 0.0;
    if (!read_one_d(suites, plugin_id, effect, index, time, value) || value < 0.0 ||
        value > static_cast<double>(std::numeric_limits<std::uint32_t>::max()) || std::floor(value) != value) return suites.fail(index);
    output = static_cast<std::uint32_t>(value);
    return true;
}

bool read_uuid(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
               A_long first_index, const A_Time& time, core::Uuid128& output) noexcept {
    core::Uuid128 id{};
    bool any = false;
    for (A_long chunk = 0; chunk < 8; ++chunk) {
        std::uint32_t value = 0;
        if (!read_uint(suites, plugin_id, effect, first_index + chunk, time, value) || value > 65535u) return false;
        id.bytes[static_cast<std::size_t>(chunk) * 2] = static_cast<std::uint8_t>(value >> 8u);
        id.bytes[static_cast<std::size_t>(chunk) * 2 + 1] = static_cast<std::uint8_t>(value & 0xffu);
        any = any || value != 0;
    }
    if (!any) return false;
    output = id;
    return true;
}

bool read_curve(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                A_long count_index, A_long first_point_index, const A_Time& time,
                core::OpaqueBytes& bytes, bool& has_curve) {
    std::uint32_t count = 0;
    if (!read_uint(suites, plugin_id, effect, count_index, time, count) || count > core::kMaxAgeCurvePoints ||
        (count != 0 && count < 2)) return false;
    has_curve = count != 0;
    bytes.clear();
    if (count == 0) return true;
    core::AgeCurve curve{};
    curve.count = static_cast<std::uint8_t>(count);
    for (std::uint32_t point = 0; point < count; ++point) {
        if (!read_one_d(suites, plugin_id, effect, first_point_index + static_cast<A_long>(point * 2), time,
                        curve.points[point].age) ||
            !read_one_d(suites, plugin_id, effect, first_point_index + static_cast<A_long>(point * 2 + 1), time,
                        curve.points[point].value)) return false;
    }
    if (!core::valid_age_curve(curve, 0.0, 100.0)) return false;
    bytes = core::encode_age_curve(curve);
    return !bytes.empty();
}

void add_value(core::GraphNode& node, core::ParameterKey key, core::ParameterValue value) {
    node.parameters.push_back(core::NodeParameter{key, std::move(value)});
}

struct Connection {
    core::NodeId source{};
    core::EdgeId edge{};
    core::NodeId destination{};
    Kind source_kind{Kind::emitter};
};

struct LayoutEntry {
    core::NodeId node{};
    double x{};
    double y{};
};

void append_u16(core::OpaqueBytes& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::byte>(value & 0xffu));
    bytes.push_back(static_cast<std::byte>((value >> 8u) & 0xffu));
}

void append_u32(core::OpaqueBytes& bytes, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8)
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
}

void append_u64(core::OpaqueBytes& bytes, std::uint64_t value) {
    for (unsigned shift = 0; shift < 64; shift += 8)
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
}

core::OpaqueBytes make_layout_record(std::vector<LayoutEntry>& entries) {
    std::sort(entries.begin(), entries.end(), [](const LayoutEntry& left, const LayoutEntry& right) {
        return left.node < right.node;
    });
    core::OpaqueBytes bytes;
    bytes.reserve(12u + entries.size() * 32u);
    append_u16(bytes, 0x8001u);
    append_u16(bytes, 1u);
    append_u32(bytes, static_cast<std::uint32_t>(12u + entries.size() * 32u));
    append_u32(bytes, static_cast<std::uint32_t>(entries.size()));
    for (const auto& entry : entries) {
        for (const auto byte : entry.node.value.bytes) bytes.push_back(static_cast<std::byte>(byte));
        append_u64(bytes, std::bit_cast<std::uint64_t>(entry.x));
        append_u64(bytes, std::bit_cast<std::uint64_t>(entry.y));
    }
    return bytes;
}

bool decode_node_kind(const char* match_name, Kind& kind, const char*& type_key, std::uint16_t& schema) noexcept {
    if (std::strcmp(match_name, kEmitterMatchName) == 0) {
        kind = Kind::emitter; type_key = core::graph_keys::kEmitterNode; schema = 5; return true;
    }
    if (std::strcmp(match_name, kParticleMatchName) == 0) {
        kind = Kind::particle; type_key = core::graph_keys::kParticleNode; schema = 2; return true;
    }
    if (std::strcmp(match_name, kAppearanceMatchName) == 0) {
        kind = Kind::appearance; type_key = core::graph_keys::kAppearanceNode; schema = 1; return true;
    }
    if (std::strcmp(match_name, kForceMatchName) == 0) {
        kind = Kind::force; type_key = core::graph_keys::kForceNode; schema = 2; return true;
    }
    return false;
}

bool read_node_parameters(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                          Kind kind, const A_Time& time, const core::LayerUnits& units, core::GraphNode& node) {
    using namespace core::graph_keys;
    double scalar = 0.0;
    std::uint32_t integer = 0;
    core::Vec3 vector{};
    if (kind == Kind::emitter) {
        if (!read_one_d(suites, plugin_id, effect, 3, time, scalar)) return false;
        add_value(node, kBirthRate, scalar);
        if (!read_uint(suites, plugin_id, effect, 24, time, integer)) return false;
        add_value(node, kSeed, integer);
        if (!read_uint(suites, plugin_id, effect, 1, time, integer) || integer < 1 || integer > 4) return false;
        add_value(node, kEmitterShape, integer - 1u);
        if (!read_two_d(suites, plugin_id, effect, 4, time, vector) ||
            !read_one_d(suites, plugin_id, effect, 5, time, vector.z)) return false;
        add_value(node, kEmitterOrigin, core::layer_point_to_world(vector.x, vector.y, vector.z + units.layer_height / 2.0, units));
        if (!read_one_d(suites, plugin_id, effect, 27, time, vector.x) ||
            !read_one_d(suites, plugin_id, effect, 28, time, vector.y) ||
            !read_one_d(suites, plugin_id, effect, 29, time, vector.z)) return false;
        add_value(node, kVelocity, vector);
        if (!read_one_d(suites, plugin_id, effect, 25, time, scalar)) return false;
        add_value(node, kParticleSize, scalar);
        if (!read_one_d(suites, plugin_id, effect, 26, time, scalar)) return false;
        add_value(node, kOpacity, scalar / 100.0);
        if (!read_one_d(suites, plugin_id, effect, 11, time, scalar)) return false;
        add_value(node, kEmitterSize, scalar);
        if (!read_one_d(suites, plugin_id, effect, 30, time, scalar)) return false;
        add_value(node, kVelocitySpread, scalar);
        if (!read_one_d(suites, plugin_id, effect, 6, time, scalar)) return false;
        const double emission_speed = scalar / units.layer_height;
        add_value(node, kEmissionSpeed, emission_speed);
        if (!read_one_d(suites, plugin_id, effect, 7, time, scalar)) return false;
        add_value(node, kEmissionSpeedRandomPercent, scalar);
        constexpr std::array<core::ParameterKey, 3> angle_keys{kEmissionAngleX, kEmissionAngleY, kEmissionAngleZ};
        for (A_long axis = 0; axis < 3; ++axis) {
            if (!read_one_d(suites, plugin_id, effect, 12 + axis, time, scalar)) return false;
            add_value(node, angle_keys[static_cast<std::size_t>(axis)], scalar);
        }
        if (!read_uint(suites, plugin_id, effect, 15, time, integer) || integer < 1 || integer > 2) return false;
        add_value(node, kDirectionMode, integer - 1u);
        if (!read_one_d(suites, plugin_id, effect, 16, time, scalar)) return false;
        add_value(node, kDirectionSpan, scalar);
        constexpr std::array<core::ParameterKey, 3> size_keys{kEmitterSizeX, kEmitterSizeY, kEmitterSizeZ};
        for (A_long axis = 0; axis < 3; ++axis) {
            if (!read_one_d(suites, plugin_id, effect, 8 + axis, time, scalar)) return false;
            add_value(node, size_keys[static_cast<std::size_t>(axis)], scalar);
        }
        if (!read_uint(suites, plugin_id, effect, 2, time, integer) || integer < 1 || integer > 2) return false;
        add_value(node, kEmittingMode, integer - 1u);
        constexpr std::array<core::ParameterKey, 7> auxiliary_keys{kEmitChance, kEmitLifeStart, kEmitLifeEnd,
            kInheritVelocity, kInheritSize, kInheritOpacity, kInheritColor};
        for (A_long field = 0; field < 7; ++field) {
            if (!read_one_d(suites, plugin_id, effect, 17 + field, time, scalar)) return false;
            add_value(node, auxiliary_keys[field], scalar);
        }
        return true;
    }
    if (kind == Kind::force) {
        if (!read_one_d(suites, plugin_id, effect, 1, time, scalar)) return false;
        add_value(node, kGravity, core::Vec3{0, -scalar / units.layer_height, 0});
        if (!read_one_d(suites, plugin_id, effect, 2, time, scalar)) return false;
        add_value(node, kGravityRandom, scalar);
        if (!read_one_d(suites, plugin_id, effect, 3, time, vector.x) ||
            !read_one_d(suites, plugin_id, effect, 4, time, vector.y) ||
            !read_one_d(suites, plugin_id, effect, 5, time, vector.z)) return false;
        add_value(node, kWind, core::Vec3{vector.x * units.pixel_aspect_ratio / units.layer_height, -vector.y / units.layer_height, vector.z / units.layer_height});
        constexpr std::array<core::ParameterKey, 4> spin_keys{kSpin, kSpinFrequency, kSpinResist, kSpinDelay};
        for (A_long field = 0; field < 4; ++field) {
            if (!read_one_d(suites, plugin_id, effect, 6 + field, time, scalar)) return false;
            add_value(node, spin_keys[field], field == 0 ? scalar / units.layer_height : scalar);
        }
        if (!read_one_d(suites, plugin_id, effect, 10, time, scalar)) return false;
        add_value(node, kLinearDrag, scalar);
        core::OpaqueBytes curve; bool present = false;
        if (!read_curve(suites, plugin_id, effect, 11, 12, time, curve, present)) return false;
        if (present) add_value(node, kWindSpinCurve, std::move(curve));
        return true;
    }

    const bool particle = kind == Kind::particle;
    const A_long color_start_index = particle ? 6 : 5;
    const A_long color_end_index = particle ? 7 : 6;
    const A_long size_index = particle ? 2 : 1;
    const A_long size_end_index = particle ? 8 : 7;
    const A_long opacity_index = particle ? 4 : 3;
    const A_long opacity_end_index = particle ? 9 : 8;
    const A_long size_random_index = particle ? 3 : 2;
    const A_long opacity_random_index = particle ? 5 : 4;
    if (!read_color(suites, plugin_id, effect, color_start_index, time, vector)) return false;
    add_value(node, kColorStart, vector);
    if (!read_color(suites, plugin_id, effect, color_end_index, time, vector)) return false;
    add_value(node, kColorEnd, vector);
    if (!read_one_d(suites, plugin_id, effect, size_index, time, scalar)) return false;
    add_value(node, kSizeStart, scalar);
    if (!read_one_d(suites, plugin_id, effect, size_end_index, time, scalar)) return false;
    add_value(node, kSizeEnd, scalar);
    if (!read_one_d(suites, plugin_id, effect, opacity_index, time, scalar)) return false;
    add_value(node, kOpacityStart, scalar / 100.0);
    if (!read_one_d(suites, plugin_id, effect, opacity_end_index, time, scalar)) return false;
    add_value(node, kOpacityEnd, scalar);
    if (!read_one_d(suites, plugin_id, effect, size_random_index, time, scalar)) return false;
    add_value(node, kSizeRandom, scalar);
    if (!read_one_d(suites, plugin_id, effect, opacity_random_index, time, scalar)) return false;
    add_value(node, kOpacityRandom, scalar);
    if (particle) {
        if (!read_one_d(suites, plugin_id, effect, 1, time, scalar)) return false;
        add_value(node, kParticleLifetimeSeconds, scalar);
    }

    const A_long size_curve_count_index = particle ? 10 : 9;
    const A_long size_curve_first_index = size_curve_count_index + 1;
    const A_long opacity_curve_count_index = particle ? 27 : 26;
    const A_long opacity_curve_first_index = opacity_curve_count_index + 1;
    core::OpaqueBytes curve{};
    bool has_curve = false;
    if (!read_curve(suites, plugin_id, effect, size_curve_count_index, size_curve_first_index,
                    time, curve, has_curve)) return false;
    if (has_curve) add_value(node, kSizeOverLifeCurve, std::move(curve));
    if (!read_curve(suites, plugin_id, effect, opacity_curve_count_index, opacity_curve_first_index,
                    time, curve, has_curve)) return false;
    if (has_curve) add_value(node, kOpacityOverLifeCurve, std::move(curve));
    return true;
}

bool read_connections(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                      Kind kind, const A_Time& time, core::NodeId source,
                      std::vector<Connection>& output) {
    std::uint32_t count = 0;
    if (!read_uint(suites, plugin_id, effect, native_nodes::connection_count_index(kind), time, count) ||
        count > static_cast<std::uint32_t>(native_nodes::kMaxOutgoingEdges)) return false;
    for (std::uint32_t slot = 0; slot < count; ++slot) {
        core::Uuid128 destination{};
        core::Uuid128 edge{};
        if (!read_uuid(suites, plugin_id, effect,
                       native_nodes::connection_uuid_index(kind, static_cast<A_long>(slot), 0), time, destination) ||
            !read_uuid(suites, plugin_id, effect,
                       native_nodes::connection_edge_uuid_index(kind, static_cast<A_long>(slot), 0), time, edge)) return false;
        output.push_back(Connection{source, core::EdgeId{edge}, core::NodeId{destination}, kind});
    }
    return true;
}

core::NodeId output_node_id() noexcept {
    core::Uuid128 id{};
    id.bytes.back() = 0xff;
    return core::NodeId{id};
}

core::PortKey source_port(Kind kind) noexcept {
    using namespace core::graph_keys;
    switch (kind) {
        case Kind::emitter: return kEmitterParticles;
        case Kind::particle: return kParticleParticlesOut;
        case Kind::appearance: return kAppearanceParticlesOut;
        case Kind::force: return kForceParticlesOut;
    }
    return {};
}

core::PortKey destination_port(Kind kind) noexcept {
    using namespace core::graph_keys;
    switch (kind) {
        case Kind::emitter: return kEmitterParents;
        case Kind::particle: return kParticleParticlesIn;
        case Kind::appearance: return kAppearanceParticlesIn;
        case Kind::force: return kForceParticlesIn;
    }
    return {};
}

} // namespace

PF_Err compile_native_node_graph(PF_InData* in_data, PF_ParamDef* params[],
                                 core::Graph& graph, bool& found_node_effects, AEGP_PluginID plugin_id,
                                 const node_sync::NativeEdit* edit) noexcept {
    graph = {};
    found_node_effects = false;
    if (!in_data || !params || !in_data->pica_basicP || (!in_data->effect_ref && !(edit && edit->layer))) return PF_Err_BAD_CALLBACK_PARAM;
    if (edit && (!node_sync::valid_edit(*edit) || edit->parameter_index >
        native_nodes::base_parameter_count(static_cast<Kind>(edit->node_kind)))) return PF_Err_BAD_CALLBACK_PARAM;
    if (plugin_id == 0) return PF_Err_BAD_CALLBACK_PARAM;

    try {
        SuiteSet suites(in_data);
        suites.edit = edit;
        PF_Err error = suites.acquire();
        if (error != PF_Err_NONE) return error;
        AEGP_LayerH layer = edit ? edit->layer : nullptr;
        A_Err ae_error = layer ? 0 : suites.pf_interface->AEGP_GetEffectLayer(in_data->effect_ref, &layer);
        if (ae_error || !layer) return static_cast<PF_Err>(ae_error ? ae_error : PF_Err_BAD_CALLBACK_PARAM);

        A_long effect_count = 0;
        ae_error = suites.effect->AEGP_GetLayerNumEffects(layer, &effect_count);
        if (ae_error) return static_cast<PF_Err>(ae_error);
        const A_Time time{in_data->current_time, in_data->time_scale};
        // AEGP stream values are full-resolution layer pixels, unlike PF point
        // checkouts. Do not apply preview downsample scaling to them.
        const core::LayerUnits units{static_cast<double>(std::max<A_long>(in_data->width, 1)),
                                    static_cast<double>(std::max<A_long>(in_data->height, 1)),
                                    in_data->pixel_aspect_ratio.den ?
                                        static_cast<double>(in_data->pixel_aspect_ratio.num) / in_data->pixel_aspect_ratio.den : 1.0};
        std::vector<Connection> connections;
        std::vector<LayoutEntry> layout_entries;
        graph.nodes.reserve(static_cast<std::size_t>(effect_count) + 1u);
        connections.reserve(static_cast<std::size_t>(effect_count) * 2u);
        layout_entries.reserve(static_cast<std::size_t>(effect_count) + 1u);

        for (A_long effect_index = 0; effect_index < effect_count; ++effect_index) {
            suites.edited_effect = nullptr; // EffectRef handles may be reused after disposal.
            AEGP_EffectRefH raw_effect = nullptr;
            ae_error = suites.effect->AEGP_GetLayerEffectByIndex(plugin_id, layer, effect_index, &raw_effect);
            if (ae_error) return static_cast<PF_Err>(ae_error);
            if (!raw_effect) continue;
            EffectRef effect(suites.effect, raw_effect);
            AEGP_InstalledEffectKey installed_key = AEGP_InstalledEffectKey_NONE;
            char match_name[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
            ae_error = suites.effect->AEGP_GetInstalledKeyFromLayerEffect(effect.value, &installed_key);
            if (!ae_error) ae_error = suites.effect->AEGP_GetEffectMatchName(installed_key, match_name);
            if (ae_error) return static_cast<PF_Err>(ae_error);
            if (std::strcmp(match_name, kRendererMatchName) == 0) continue;

            Kind kind{};
            const char* type_key = nullptr;
            std::uint16_t schema_version = 0;
            if (!decode_node_kind(match_name, kind, type_key, schema_version)) continue;
            found_node_effects = true;
            if (graph.nodes.size() >= core::kMaxGraphNodes - 1u) return PF_Err_BAD_CALLBACK_PARAM;

            core::Uuid128 uuid{};
            if (!read_uuid(suites, plugin_id, effect.value, native_nodes::uuid_first_index(kind), time, uuid))
                return PF_Err_BAD_CALLBACK_PARAM;
            if (edit && static_cast<std::uint32_t>(kind) == edit->node_kind) {
                bool matches = true;
                for (std::size_t chunk = 0; chunk < edit->uuid.size(); ++chunk) {
                    const auto word = static_cast<std::uint16_t>((uuid.bytes[chunk * 2] << 8u) | uuid.bytes[chunk * 2 + 1]);
                    matches = matches && word == edit->uuid[chunk];
                }
                if (matches) {
                    if (suites.edit_matched) return PF_Err_BAD_CALLBACK_PARAM;
                    suites.edit_matched = true;
                    suites.edited_effect = effect.value;
                }
            }
            core::GraphNode node{core::NodeId{uuid}, type_key, schema_version, {}};
            if (!read_node_parameters(suites, plugin_id, effect.value, kind, time, units, node))
                return PF_Err_BAD_CALLBACK_PARAM;
            double layout_x = 0.0;
            double layout_y = 0.0;
            if (!read_one_d(suites, plugin_id, effect.value, native_nodes::layout_x_index(kind), time, layout_x) ||
                !read_one_d(suites, plugin_id, effect.value, native_nodes::layout_y_index(kind), time, layout_y) ||
                std::abs(layout_x) > 1000000000.0 || std::abs(layout_y) > 1000000000.0)
                return PF_Err_BAD_CALLBACK_PARAM;
            layout_entries.push_back(LayoutEntry{core::NodeId{uuid}, layout_x, layout_y});
            if (!read_connections(suites, plugin_id, effect.value, kind, time, node.id, connections))
                return PF_Err_BAD_CALLBACK_PARAM;
            graph.nodes.push_back(std::move(node));
        }

        if (edit && !suites.edit_applied) return PF_Err_BAD_CALLBACK_PARAM;
        if (!params[kMaxParticlesId] || params[kMaxParticlesId]->param_type != PF_Param_FLOAT_SLIDER ||
            !std::isfinite(params[kMaxParticlesId]->u.fs_d.value) || params[kMaxParticlesId]->u.fs_d.value < 0.0 ||
            params[kMaxParticlesId]->u.fs_d.value > static_cast<PF_FpLong>(std::numeric_limits<std::uint32_t>::max()) ||
            std::floor(params[kMaxParticlesId]->u.fs_d.value) != params[kMaxParticlesId]->u.fs_d.value) {
            return PF_Err_BAD_CALLBACK_PARAM;
        }
        const core::NodeId output_id = output_node_id();
        if (!params[kLayoutOutputXId] || !params[kLayoutOutputYId] ||
            params[kLayoutOutputXId]->param_type != PF_Param_FLOAT_SLIDER ||
            params[kLayoutOutputYId]->param_type != PF_Param_FLOAT_SLIDER ||
            !std::isfinite(params[kLayoutOutputXId]->u.fs_d.value) ||
            !std::isfinite(params[kLayoutOutputYId]->u.fs_d.value)) return PF_Err_BAD_CALLBACK_PARAM;
        layout_entries.push_back(LayoutEntry{output_id,
            params[kLayoutOutputXId]->u.fs_d.value, params[kLayoutOutputYId]->u.fs_d.value});
        graph.nodes.push_back(core::GraphNode{output_id, core::graph_keys::kOutputNode, 3,
            {{core::graph_keys::kParticleCount,
              static_cast<std::uint32_t>(params[kMaxParticlesId]->u.fs_d.value)}}});
        auto& output = graph.nodes.back();
        for (const auto& binding : {std::pair{kTimeRemapEnabledId, core::graph_keys::kTimeRemapEnabled},
                                   std::pair{kPreviewEnabledId, core::graph_keys::kPreviewEnabled}}) {
            if (!params[binding.first] || params[binding.first]->param_type != PF_Param_CHECKBOX) return PF_Err_BAD_CALLBACK_PARAM;
            add_value(output, binding.second, std::uint32_t(params[binding.first]->u.bd.value != 0));
        }
        for (const auto& binding : {std::pair{kTimeRemapSecondsId, core::graph_keys::kTimeRemapSeconds},
                                   std::pair{kPreviewChanceId, core::graph_keys::kPreviewChance}}) {
            if (!params[binding.first] || params[binding.first]->param_type != PF_Param_FLOAT_SLIDER) return PF_Err_BAD_CALLBACK_PARAM;
            add_value(output, binding.second, double(params[binding.first]->u.fs_d.value));
        }

        for (const auto& connection : connections) {
            Kind destination_kind = Kind::emitter;
            bool found_destination = false;
            if (connection.destination == output_id) {
                graph.edges.push_back(core::GraphEdge{connection.edge, connection.source,
                    source_port(connection.source_kind), output_id, core::graph_keys::kOutputParticles});
                continue;
            }
            for (const auto& node : graph.nodes) {
                if (node.id == connection.destination) {
                    found_destination = true;
                    if (node.type_key == core::graph_keys::kParticleNode) destination_kind = Kind::particle;
                    else if (node.type_key == core::graph_keys::kAppearanceNode) destination_kind = Kind::appearance;
                    else if (node.type_key == core::graph_keys::kForceNode) destination_kind = Kind::force;
                    break;
                }
            }
            graph.edges.push_back(core::GraphEdge{connection.edge, connection.source,
                source_port(connection.source_kind), connection.destination,
                found_destination ? destination_port(destination_kind) : core::PortKey{1}});
        }

        const auto validation = core::validate_graph(graph, core::particle_node_registry());
        if (validation.ok()) graph.optional_records.push_back(make_layout_record(layout_entries));
        return validation.ok() ? PF_Err_NONE : PF_Err_BAD_CALLBACK_PARAM;
    } catch (const std::bad_alloc&) {
        return PF_Err_OUT_OF_MEMORY;
    } catch (...) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
}

} // namespace starfield::adapter
