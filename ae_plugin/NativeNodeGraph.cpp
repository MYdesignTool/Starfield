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

struct RawField {
    bool present{};
    node_sync::ValueKind type{};
    A_long slot{-1};
    std::array<double, 3> value{};
};
struct RawNode {
    core::NodeId id{};
    Kind kind{};
    std::array<RawField, 44> fields{};
};
constexpr std::uint16_t kBindingRecordTag = 0x8002;
constexpr A_long component_count(node_sync::ValueKind type) noexcept {
    return type == node_sync::ValueKind::scalar ? 1 : type == node_sync::ValueKind::point2 ? 2 : 3;
}
constexpr A_long animated_last_index(Kind kind) noexcept {
    return kind == Kind::emitter ? 30 : kind == Kind::particle ? 9 : kind == Kind::appearance ? 8 : 10;
}

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
    RawNode* recording{};
    const RawNode* playback{};
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

bool record_value(SuiteSet& suites, A_long index, node_sync::ValueKind type,
                  const std::array<double, 3>& value) noexcept {
    if (suites.recording) {
        if (index < 1 || index >= static_cast<A_long>(suites.recording->fields.size())) return false;
        auto& field = suites.recording->fields[index];
        field.present = true; field.type = type; field.value = value;
    }
    return true;
}
bool playback_value(SuiteSet& suites, A_long index, node_sync::ValueKind type,
                    std::array<double, 3>& value) noexcept {
    if (index < 1 || index >= static_cast<A_long>(suites.playback->fields.size())) return false;
    const auto& field = suites.playback->fields[index];
    if (!field.present || field.type != type) return false;
    value = field.value; return true;
}

bool read_one_d(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                A_long index, const A_Time& time, double& output) noexcept {
    if (suites.playback) {
        std::array<double, 3> sampled{};
        if (!playback_value(suites, index, node_sync::ValueKind::scalar, sampled)) return false;
        output = sampled[0]; return record_value(suites, index, node_sync::ValueKind::scalar, {output, 0.0, 0.0});
    }

    if (suites.edit && effect == suites.edited_effect && index == suites.edit->parameter_index) {
        if (suites.edit->value_kind != node_sync::ValueKind::scalar) return suites.fail(index);
        suites.edit_applied = true; output = suites.edit->value[0]; return record_value(suites, index, node_sync::ValueKind::scalar, {output, 0.0, 0.0});
    }
    AEGP_StreamRefH raw_stream = nullptr;
    A_Err error = suites.stream->AEGP_GetNewEffectStreamByIndex(plugin_id, effect, index, &raw_stream);
    if (error || !raw_stream) return suites.fail(index);
    StreamRef stream(suites.stream, raw_stream);
    AEGP_StreamValue2 value{};
    error = suites.stream->AEGP_GetNewStreamValue(plugin_id, stream.value, AEGP_LTimeMode_LayerTime,
                                                  &time, suites.recording ? FALSE : TRUE, &value);
    if (error) return suites.fail(index);
    const double result = value.val.one_d;
    suites.stream->AEGP_DisposeStreamValue(&value);
    if (!std::isfinite(result)) return suites.fail(index);
    output = result;
    return record_value(suites, index, node_sync::ValueKind::scalar, {output, 0.0, 0.0});
}

bool read_two_d(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                A_long index, const A_Time& time, core::Vec3& output) noexcept {
    if (suites.playback) {
        std::array<double, 3> sampled{};
        if (!playback_value(suites, index, node_sync::ValueKind::point2, sampled)) return false;
        output = {sampled[0], sampled[1], sampled[2]}; return record_value(suites, index, node_sync::ValueKind::point2, {output.x, output.y, output.z});
    }

    if (suites.edit && effect == suites.edited_effect && index == suites.edit->parameter_index) {
        if (suites.edit->value_kind != node_sync::ValueKind::point2) return suites.fail(index);
        suites.edit_applied = true; output = {suites.edit->value[0], suites.edit->value[1], 0.0}; return record_value(suites, index, node_sync::ValueKind::point2, {output.x, output.y, output.z});
    }
    AEGP_StreamRefH raw_stream = nullptr;
    A_Err error = suites.stream->AEGP_GetNewEffectStreamByIndex(plugin_id, effect, index, &raw_stream);
    if (error || !raw_stream) return suites.fail(index);
    StreamRef stream(suites.stream, raw_stream);
    AEGP_StreamValue2 value{};
    error = suites.stream->AEGP_GetNewStreamValue(plugin_id, stream.value, AEGP_LTimeMode_LayerTime,
                                                &time, suites.recording ? FALSE : TRUE, &value);
    if (error) return suites.fail(index);
    const core::Vec3 result{value.val.two_d.x, value.val.two_d.y, 0.0};
    suites.stream->AEGP_DisposeStreamValue(&value);
    if (!std::isfinite(result.x) || !std::isfinite(result.y)) return suites.fail(index);
    output = result;
    return record_value(suites, index, node_sync::ValueKind::point2, {output.x, output.y, output.z});
}

bool read_three_d(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                  A_long index, const A_Time& time, core::Vec3& output) noexcept {
    if (suites.playback) {
        std::array<double, 3> sampled{};
        if (!playback_value(suites, index, node_sync::ValueKind::point3, sampled)) return false;
        output = {sampled[0], sampled[1], sampled[2]}; return record_value(suites, index, node_sync::ValueKind::point3, {output.x, output.y, output.z});
    }

    if (suites.edit && effect == suites.edited_effect && index == suites.edit->parameter_index) {
        if (suites.edit->value_kind != node_sync::ValueKind::point3) return suites.fail(index);
        suites.edit_applied = true; output = {suites.edit->value[0], suites.edit->value[1], suites.edit->value[2]}; return record_value(suites, index, node_sync::ValueKind::point3, {output.x, output.y, output.z});
    }
    AEGP_StreamRefH raw_stream = nullptr;
    A_Err error = suites.stream->AEGP_GetNewEffectStreamByIndex(plugin_id, effect, index, &raw_stream);
    if (error || !raw_stream) return suites.fail(index);
    StreamRef stream(suites.stream, raw_stream);
    AEGP_StreamValue2 value{};
    error = suites.stream->AEGP_GetNewStreamValue(plugin_id, stream.value, AEGP_LTimeMode_LayerTime,
                                                  &time, suites.recording ? FALSE : TRUE, &value);
    if (error) return suites.fail(index);
    const core::Vec3 result{value.val.three_d.x, value.val.three_d.y, value.val.three_d.z};
    suites.stream->AEGP_DisposeStreamValue(&value);
    if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) return suites.fail(index);
    output = result;
    return record_value(suites, index, node_sync::ValueKind::point3, {output.x, output.y, output.z});
}

bool read_color(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
                A_long index, const A_Time& time, core::Vec3& output) noexcept {
    if (suites.playback) {
        std::array<double, 3> sampled{};
        if (!playback_value(suites, index, node_sync::ValueKind::color, sampled)) return false;
        output = {sampled[0], sampled[1], sampled[2]}; return record_value(suites, index, node_sync::ValueKind::color, {output.x, output.y, output.z});
    }

    if (suites.edit && effect == suites.edited_effect && index == suites.edit->parameter_index) {
        if (suites.edit->value_kind != node_sync::ValueKind::color) return suites.fail(index);
        suites.edit_applied = true; output = {suites.edit->value[0], suites.edit->value[1], suites.edit->value[2]}; return record_value(suites, index, node_sync::ValueKind::color, {output.x, output.y, output.z});
    }
    AEGP_StreamRefH raw_stream = nullptr;
    A_Err error = suites.stream->AEGP_GetNewEffectStreamByIndex(plugin_id, effect, index, &raw_stream);
    if (error || !raw_stream) return suites.fail(index);
    StreamRef stream(suites.stream, raw_stream);
    AEGP_StreamValue2 value{};
    error = suites.stream->AEGP_GetNewStreamValue(plugin_id, stream.value, AEGP_LTimeMode_LayerTime,
                                                  &time, suites.recording ? FALSE : TRUE, &value);
    if (error) return suites.fail(index);
    const core::Vec3 result{value.val.color.redF, value.val.color.greenF, value.val.color.blueF};
    suites.stream->AEGP_DisposeStreamValue(&value);
    if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) return suites.fail(index);
    output = result;
    return record_value(suites, index, node_sync::ValueKind::color, {output.x, output.y, output.z});
}

bool read_uint(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_EffectRefH effect,
               A_long index, const A_Time& time, std::uint32_t& output) noexcept {
    double value = 0.0;
    if (!read_one_d(suites, plugin_id, effect, index, time, value) || value < 0.0 ||
        value > static_cast<double>(std::numeric_limits<std::uint32_t>::max())) return suites.fail(index);
    if ((suites.playback && index <= animated_last_index(suites.playback->kind)) ||
        (suites.recording && index <= animated_last_index(suites.recording->kind))) value = std::round(value);
    if (std::floor(value) != value) return suites.fail(index);
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

core::OpaqueBytes make_binding_record(std::vector<RawNode>& nodes) {
    std::sort(nodes.begin(), nodes.end(), [](const auto& a, const auto& b) { return a.id < b.id; });
    core::OpaqueBytes bytes;
    append_u16(bytes, kBindingRecordTag); append_u16(bytes, 1); append_u32(bytes, 0);
    append_u32(bytes, static_cast<std::uint32_t>(nodes.size()));
    A_long slot = 0;
    for (auto& node : nodes) {
        for (auto b : node.id.value.bytes) bytes.push_back(static_cast<std::byte>(b));
        append_u16(bytes, static_cast<std::uint16_t>(node.kind));
        std::uint16_t count = 0;
        for (const auto& field : node.fields) if (field.present) ++count;
        append_u16(bytes, count);
        for (A_long index = 1; index < static_cast<A_long>(node.fields.size()); ++index) {
            auto& field = node.fields[index]; if (!field.present) continue;
            if (index <= animated_last_index(node.kind)) {
                field.slot = slot; slot += component_count(field.type);
                if (slot > kNativeBindingCapacity) return {};
            }
            append_u16(bytes, static_cast<std::uint16_t>(index));
            append_u16(bytes, static_cast<std::uint16_t>(field.type));
            append_u16(bytes, field.slot < 0 ? 0xffffu : static_cast<std::uint16_t>(field.slot));
            append_u16(bytes, 0);
            for (auto value : field.value) append_u64(bytes, std::bit_cast<std::uint64_t>(value));
        }
    }
    for (unsigned i = 0; i < 4; ++i) bytes[4 + i] = static_cast<std::byte>((bytes.size() >> (i * 8)) & 0xffu);
    return bytes;
}

bool read_binding_record(const core::Graph& graph, std::vector<RawNode>& nodes) {
    bool found = false;
    std::array<bool, kNativeBindingCapacity> used{};
    for (const auto& bytes : graph.optional_records) {
        if (bytes.size() < 2 || bytes[0] != std::byte{2} || bytes[1] != std::byte{0x80}) continue;
        if (found || bytes.size() < 12) return false;
        found = true; std::size_t at = 0;
        auto read = [&](unsigned count, std::uint64_t& out) {
            if (count > bytes.size() - at) return false;
            out = 0;
            for (unsigned i = 0; i < count; ++i) out |= std::uint64_t(std::to_integer<unsigned char>(bytes[at++])) << (8 * i);
            return true;
        };
        std::uint64_t tag{}, version{}, length{}, count{};
        if (!read(2, tag) || !read(2, version) || version != 1 || !read(4, length) ||
            length != bytes.size() || !read(4, count) || count >= core::kMaxGraphNodes) return false;
        for (std::uint64_t n = 0; n < count; ++n) {
            RawNode node;
            std::uint64_t value{};
            for (auto& byte : node.id.value.bytes) { if (!read(1, value)) return false; byte = static_cast<std::uint8_t>(value); }
            std::uint64_t kind{}, fields{};
            if (!read(2, kind) || kind > 3 || !read(2, fields) || fields > 43) return false;
            node.kind = static_cast<Kind>(kind);
            if (std::any_of(nodes.begin(), nodes.end(), [&](const auto& old) {return old.id == node.id;})) return false;
            for (std::uint64_t f = 0; f < fields; ++f) {
                std::uint64_t index{}, type{}, slot{}, reserved{};
                if (!read(2, index) || index < 1 || index >= node.fields.size() || node.fields[index].present ||
                    !read(2, type) || type > 3 || !read(2, slot) || !read(2, reserved) || reserved != 0) return false;
                auto& field = node.fields[index]; field.present = true;
                field.type = static_cast<node_sync::ValueKind>(type);
                field.slot = slot == 0xffffu ? -1 : static_cast<A_long>(slot);
                if (field.slot >= 0) {
                    if (index > static_cast<std::uint64_t>(animated_last_index(node.kind)) ||
                        field.slot + component_count(field.type) > kNativeBindingCapacity) return false;
                    for (A_long c = 0; c < component_count(field.type); ++c) {
                        if (used[field.slot + c]) return false;
                        used[field.slot + c] = true;
                    }
                } else if (index <= static_cast<std::uint64_t>(animated_last_index(node.kind))) return false;
                for (auto& scalar : field.value) {
                    if (!read(8, value)) return false;
                    scalar = std::bit_cast<double>(value);
                    if (!std::isfinite(scalar)) return false;
                }
            }
            nodes.push_back(std::move(node));
        }
        if (at != bytes.size()) return false;
    }
    return true; // No binding record on bootstrap graphs.
}

std::u16string binding_expression(const RawNode& node, A_long index, A_long component) {
    // PropertyGroup and Effect are host objects; use documented methods.
    std::string text = "var count = thisLayer(\"ADBE Effect Parade\").numProperties; var result = -1099511627776;\n";
    text += "for (var n = 1; n <= count; n++) { var fx = thisLayer.effect(n); var match = false; try { match = ";
    const auto first = native_nodes::uuid_first_index(node.kind);
    for (A_long chunk = 0; chunk < 8; ++chunk) {
        if (chunk) text += " && ";
        const unsigned word = (node.id.value.bytes[chunk * 2] << 8u) | node.id.value.bytes[chunk * 2 + 1];
        text += "fx.param(" + std::to_string(first + chunk) + ").value === " + std::to_string(word);
    }
    text += "; } catch (unrelatedEffect) {} if (match) { result = fx.param(" + std::to_string(index) + ").value";
    if (node.fields[index].type != node_sync::ValueKind::scalar) text += "[" + std::to_string(component) + "]";
    text += "; break; } }\nresult;";
    return std::u16string(text.begin(), text.end());
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
        std::vector<RawNode> raw_nodes;
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
            RawNode raw; raw.id = core::NodeId{uuid}; raw.kind = kind;
            suites.recording = &raw;
            core::GraphNode node{core::NodeId{uuid}, type_key, schema_version, {}};
            if (!read_node_parameters(suites, plugin_id, effect.value, kind, time, units, node))
                return PF_Err_BAD_CALLBACK_PARAM;
            suites.recording = nullptr; raw_nodes.push_back(std::move(raw));
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
        if (validation.ok()) {
            graph.optional_records.push_back(make_layout_record(layout_entries));
            auto bindings = make_binding_record(raw_nodes);
            if (bindings.empty() && !raw_nodes.empty()) return PF_Err_BAD_CALLBACK_PARAM;
            if (!bindings.empty()) graph.optional_records.push_back(std::move(bindings));
        }
        return validation.ok() ? PF_Err_NONE : PF_Err_BAD_CALLBACK_PARAM;
    } catch (const std::bad_alloc&) {
        return PF_Err_OUT_OF_MEMORY;
    } catch (...) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
}

struct NativeBindingTransaction::Impl {
    struct Change {
        AEGP_StreamRefH ref{};
        std::u16string previous;
        A_Boolean enabled{};
        double previous_value{};
        A_long index{-1};
        bool changed{};
    };
    PF_InData* data{};
    AEGP_PluginID id{};
    AEGP_EffectRefH renderer{};
    bool owned_renderer{};
    SuiteSet suites;
    const AEGP_MemorySuite1* memory{};
    std::vector<Change> changes;
    bool accepted{};
    Impl(PF_InData* d, AEGP_PluginID i, AEGP_EffectRefH r) : data(d), id(i), renderer(r), suites(d) {}
    ~Impl() {
        for (auto& change : changes) {
            if (!accepted && change.changed) {
                AEGP_StreamValue2 value{}; value.streamH = change.ref; value.val.one_d = change.previous_value;
                suites.stream->AEGP_SetStreamValue(id, change.ref, &value);
                suites.stream->AEGP_SetExpression(id, change.ref, reinterpret_cast<const A_UTF16Char*>(change.previous.c_str()));
                suites.stream->AEGP_SetExpressionState(id, change.ref, change.enabled);
            }
            if (change.ref) suites.stream->AEGP_DisposeStream(change.ref);
        }
        if (owned_renderer && renderer) suites.effect->AEGP_DisposeEffect(renderer);
        if (memory) suites.basic->ReleaseSuite(kAEGPMemorySuite, kAEGPMemorySuiteVersion1);
    }
};

NativeBindingTransaction::NativeBindingTransaction(PF_InData* data, AEGP_PluginID id, AEGP_EffectRefH renderer)
    : impl_(std::make_unique<Impl>(data, id, renderer)) {}
NativeBindingTransaction::~NativeBindingTransaction() = default;
void NativeBindingTransaction::accept() noexcept { impl_->accepted = true; }

PF_Err NativeBindingTransaction::install(const core::Graph& graph, A_long* failed_stream) noexcept {
    if (failed_stream) *failed_stream = -1;
    try {
        std::vector<RawNode> nodes;
        if (!read_binding_record(graph, nodes)) return PF_Err_BAD_CALLBACK_PARAM;
        if (nodes.empty()) return PF_Err_NONE;
        auto& tx = *impl_;
        auto error = tx.suites.acquire(); if (error) return error;
        A_Err ae = tx.suites.basic->AcquireSuite(kAEGPMemorySuite, kAEGPMemorySuiteVersion1,
            reinterpret_cast<const void**>(&tx.memory));
        if (ae) return static_cast<PF_Err>(ae);
        if (!tx.renderer) {
            ae = tx.suites.pf_interface->AEGP_GetNewEffectForEffect(tx.id, tx.data->effect_ref, &tx.renderer);
            tx.owned_renderer = true;
            if (ae || !tx.renderer) return static_cast<PF_Err>(ae ? ae : PF_Err_BAD_CALLBACK_PARAM);
        }
        tx.changes.reserve(kNativeBindingCapacity);
        for (const auto& node : nodes) for (A_long index = 1; index < static_cast<A_long>(node.fields.size()); ++index) {
            const auto& field = node.fields[index]; if (!field.present || field.slot < 0) continue;
            for (A_long component = 0; component < component_count(field.type); ++component) {
                tx.changes.emplace_back(); auto& change = tx.changes.back();
                change.index = kNativeBindingFirstIndex + field.slot + component;
                if (failed_stream) *failed_stream = change.index;
                ae = tx.suites.stream->AEGP_GetNewEffectStreamByIndex(tx.id, tx.renderer,
                    kNativeBindingFirstIndex + field.slot + component, &change.ref);
                if (ae || !change.ref) return static_cast<PF_Err>(ae ? ae : PF_Err_BAD_CALLBACK_PARAM);
                AEGP_StreamType type = AEGP_StreamType_NO_DATA;
                ae = tx.suites.stream->AEGP_GetStreamType(change.ref, &type);
                if (ae || type != AEGP_StreamType_OneD) return static_cast<PF_Err>(ae ? ae : PF_Err_BAD_CALLBACK_PARAM);
                const A_Time sample_time{tx.data->current_time, tx.data->time_scale};
                AEGP_StreamValue2 previous{};
                ae = tx.suites.stream->AEGP_GetNewStreamValue(tx.id, change.ref, AEGP_LTimeMode_LayerTime, &sample_time, TRUE, &previous);
                if (ae) return static_cast<PF_Err>(ae);
                change.previous_value = previous.val.one_d;
                tx.suites.stream->AEGP_DisposeStreamValue(&previous);
                ae = tx.suites.stream->AEGP_GetExpressionState(tx.id, change.ref, &change.enabled);
                if (ae) return static_cast<PF_Err>(ae);
                AEGP_MemHandle handle{};
                ae = tx.suites.stream->AEGP_GetExpression(tx.id, change.ref, &handle);
                if (ae) return static_cast<PF_Err>(ae);
                if (handle) {
                    void* text = nullptr; ae = tx.memory->AEGP_LockMemHandle(handle, &text);
                    struct LockedExpression {
                        const AEGP_MemorySuite1* suite;
                        AEGP_MemHandle handle;
                        bool locked;
                        ~LockedExpression() {
                            if (locked) suite->AEGP_UnlockMemHandle(handle);
                            suite->AEGP_FreeMemHandle(handle);
                        }
                    } owned{tx.memory, handle, ae == 0};
                    if (!ae && text) change.previous = reinterpret_cast<const char16_t*>(text);
                    if (ae) return static_cast<PF_Err>(ae);
                }
                const auto desired = binding_expression(node, index, component);
                if (change.previous == desired && change.enabled) continue;
                change.changed = true;
                AEGP_StreamValue2 sentinel{}; sentinel.streamH = change.ref; sentinel.val.one_d = kNativeBindingUnavailable;
                ae = tx.suites.stream->AEGP_SetStreamValue(tx.id, change.ref, &sentinel);
                if (ae) return static_cast<PF_Err>(ae);
                ae = tx.suites.stream->AEGP_SetExpression(tx.id, change.ref, reinterpret_cast<const A_UTF16Char*>(desired.c_str()));
                if (!ae) ae = tx.suites.stream->AEGP_SetExpressionState(tx.id, change.ref, TRUE);
                if (ae) return static_cast<PF_Err>(ae);
            }
        }
        // Suite success alone does not prove an expression evaluated in AE.
        const A_Time sample_time{tx.data->current_time, tx.data->time_scale};
        for (const auto& change : tx.changes) {
            if (failed_stream) *failed_stream = change.index;
            AEGP_StreamValue2 evaluated{};
            ae = tx.suites.stream->AEGP_GetNewStreamValue(tx.id, change.ref, AEGP_LTimeMode_LayerTime, &sample_time, FALSE, &evaluated);
            if (ae) return static_cast<PF_Err>(ae);
            const double value = evaluated.val.one_d;
            tx.suites.stream->AEGP_DisposeStreamValue(&evaluated);
            A_Boolean enabled = FALSE;
            ae = tx.suites.stream->AEGP_GetExpressionState(tx.id, change.ref, &enabled);
            if (ae || !enabled) return static_cast<PF_Err>(ae ? ae : PF_Err_BAD_CALLBACK_PARAM);
            if (!std::isfinite(value) || value == kNativeBindingUnavailable) return PF_Err_BAD_CALLBACK_PARAM;
        }
        return PF_Err_NONE;
    } catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

PF_Err sample_native_node_animation(PF_InData* data, core::Graph& graph, A_long width, A_long height, A_long* failed_stream) noexcept {
    if (failed_stream) *failed_stream = -1;
    try {
        std::vector<RawNode> nodes;
        if (!read_binding_record(graph, nodes)) return PF_Err_BAD_CALLBACK_PARAM;
        if (nodes.empty()) return PF_Err_NONE;
        if (!data || !data->inter.checkout_param || !data->inter.checkin_param ||
            data->num_params < kNativeBindingFirstIndex + kNativeBindingCapacity) return PF_Err_BAD_CALLBACK_PARAM;
        const core::LayerUnits units{double(std::max<A_long>(width > 0 ? width : data->width, 1)),
            double(std::max<A_long>(height > 0 ? height : data->height, 1)),
            data->pixel_aspect_ratio.den ? double(data->pixel_aspect_ratio.num) / data->pixel_aspect_ratio.den : 1.0};
        SuiteSet reader(nullptr); // No AEGP acquisition, even during destruction.
        for (auto& raw : nodes) {
            for (auto& field : raw.fields) if (field.present && field.slot >= 0) {
                for (A_long component = 0; component < component_count(field.type); ++component) {
                    PF_ParamDef sampled{};
                    if (failed_stream) *failed_stream = kNativeBindingFirstIndex + field.slot + component;
                    const auto error = PF_CHECKOUT_PARAM(data, kNativeBindingFirstIndex + field.slot + component,
                        data->current_time, data->time_step, data->time_scale, &sampled);
                    if (error) return error;
                    const bool valid = sampled.param_type == PF_Param_FLOAT_SLIDER &&
                        std::isfinite(sampled.u.fs_d.value) && sampled.u.fs_d.value != kNativeBindingUnavailable;
                    const double value = valid ? sampled.u.fs_d.value : 0;
                    const auto checked_in = PF_CHECKIN_PARAM(data, &sampled);
                    if (!valid) return PF_Err_BAD_CALLBACK_PARAM;
                    if (checked_in) return checked_in;
                    field.value[component] = value;
                }
            }
            auto node = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&](const auto& n) {return n.id == raw.id;});
            if (node == graph.nodes.end()) return PF_Err_BAD_CALLBACK_PARAM;
            const auto expected = raw.kind == Kind::emitter ? core::graph_keys::kEmitterNode :
                raw.kind == Kind::particle ? core::graph_keys::kParticleNode :
                raw.kind == Kind::appearance ? core::graph_keys::kAppearanceNode : core::graph_keys::kForceNode;
            if (node->type_key != expected) return PF_Err_BAD_CALLBACK_PARAM;
            node->parameters.clear(); reader.playback = &raw;
            if (!read_node_parameters(reader, 0, nullptr, raw.kind, {}, units, *node)) return PF_Err_BAD_CALLBACK_PARAM;
        }
        return PF_Err_NONE;
    } catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

} // namespace starfield::adapter
