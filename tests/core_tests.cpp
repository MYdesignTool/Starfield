// Host-independent core self-tests. They cover the M2 acceptance criteria that can
// be checked without After Effects: rational-time normalization and overflow,
// settings validation, deterministic simulation across frame order, and the CPU
// renderer's geometry, region of interest, alpha, and format contracts.
//
// Build with tests/RunCoreTests.ps1 (MSVC) or CMake's `starfield_core_tests`.

#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/Geometry.hpp"
#include "starfield/core/Graph.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/ParticleSimulation.hpp"
#include "starfield/core/PluginApi.h"
#include "starfield/core/Random.hpp"
#include "starfield/core/Render.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include "starfield/core/Settings.hpp"
#include "starfield/core/Time.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;

#define CHECK(condition)                                                                        \
    do {                                                                                        \
        ++g_checks;                                                                             \
        if (!(condition)) {                                                                     \
            ++g_failures;                                                                       \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);                    \
        }                                                                                       \
    } while (0)

using namespace starfield::core;

constexpr double kMicroseconds = 1000000.0;

RationalTime time_from_seconds(double seconds) {
    const auto value = *make_rational(static_cast<std::int64_t>(std::llround(seconds * kMicroseconds)),
                                     static_cast<std::int64_t>(kMicroseconds));
    return value;
}

struct Scene {
    Settings settings{};
    std::uint32_t layer_width{64};
    std::uint32_t layer_height{64};
    std::uint32_t frame_width{64};
    std::uint32_t frame_height{64};
    RectI roi{0, 0, 64, 64};
    PixelFormat format{PixelFormat::rgba8};
    double pixel_aspect{1.0};
    double time_seconds{1.0};
};

RenderRequest build_request(const Scene& scene) {
    RenderRequest request;
    request.settings = validate_settings(scene.settings);

    FrameSpec& frame = request.frame;
    frame.layer_width = scene.layer_width;
    frame.layer_height = scene.layer_height;
    frame.frame_width = scene.frame_width;
    frame.frame_height = scene.frame_height;
    frame.region_of_interest = scene.roi;
    frame.time = time_from_seconds(scene.time_seconds);
    frame.frame_duration = *make_rational(1, 24);
    frame.format = scene.format;
    frame.color_space = ColorSpace::ae_working_space;
    frame.alpha_mode = AlphaMode::premultiplied;
    frame.pixel_aspect_ratio = scene.pixel_aspect;
    frame.quality = Quality::full;
    return request;
}

struct Rgba8 {
    std::uint8_t r{0};
    std::uint8_t g{0};
    std::uint8_t b{0};
    std::uint8_t a{0};
};

Rgba8 pixel8(const RenderOutput& output, std::uint32_t x, std::uint32_t y) {
    const std::byte* pixel = output.pixels.data() + static_cast<std::size_t>(y) * output.row_bytes +
                             static_cast<std::size_t>(x) * 4;
    return Rgba8{std::to_integer<std::uint8_t>(pixel[0]), std::to_integer<std::uint8_t>(pixel[1]),
                 std::to_integer<std::uint8_t>(pixel[2]), std::to_integer<std::uint8_t>(pixel[3])};
}

std::uint16_t channel16(const RenderOutput& output, std::uint32_t x, std::uint32_t y,
                        std::uint32_t channel) {
    const std::byte* pixel = output.pixels.data() + static_cast<std::size_t>(y) * output.row_bytes +
                             static_cast<std::size_t>(x) * 8 + channel * 2;
    std::uint16_t value = 0;
    std::memcpy(&value, pixel, sizeof(value));
    return value;
}

float channel32(const RenderOutput& output, std::uint32_t x, std::uint32_t y, std::uint32_t channel) {
    const std::byte* pixel = output.pixels.data() + static_cast<std::size_t>(y) * output.row_bytes +
                             static_cast<std::size_t>(x) * 16 + channel * 4;
    float value = 0.0f;
    std::memcpy(&value, pixel, sizeof(value));
    return value;
}

class CancellingAfterFirstPoll final : public Cancellation {
public:
    explicit CancellingAfterFirstPoll(bool cancel) : cancel_(cancel) {}
    [[nodiscard]] bool is_cancelled() const noexcept override { return cancel_; }

private:
    bool cancel_{false};
};

class CancellingAtPoll final : public Cancellation {
public:
    explicit CancellingAtPoll(unsigned poll) : cancel_at_(poll) {}
    [[nodiscard]] bool is_cancelled() const noexcept override { return ++polls_ >= cancel_at_; }

private:
    unsigned cancel_at_{0};
    mutable unsigned polls_{0};
};

void test_rational_time() {
    CHECK(make_rational(2, 4)->value == 1 && make_rational(2, 4)->scale == 2);
    CHECK(make_rational(-2, 4)->value == -1 && make_rational(-2, 4)->scale == 2);
    CHECK(make_rational(0, 5)->value == 0 && make_rational(0, 5)->scale == 1);
    CHECK(!make_rational(1, 0).has_value());

    const RationalTime half = *make_rational(1, 2);
    const RationalTime third = *make_rational(1, 3);
    CHECK(add(half, third)->value == 5 && add(half, third)->scale == 6);
    CHECK(subtract(half, third)->value == 1 && subtract(half, third)->scale == 6);
    CHECK(multiply(*make_rational(2, 3), *make_rational(3, 4))->value == 1);
    CHECK(compare(half, *make_rational(2, 4)) == 0);
    CHECK(compare(third, half) == -1);
    CHECK(compare(half, third) == 1);

    const RationalTime maximum{std::numeric_limits<std::int64_t>::max(), 1};
    const RationalTime one{1, 1};
    CHECK(!add(maximum, one).has_value());
    CHECK(!multiply(maximum, maximum).has_value());

    CHECK(is_negative(*make_rational(-1, 3)));
    CHECK(is_zero(*make_rational(0, 7)));
    CHECK(std::abs(to_seconds(*make_rational(1, 2)) - 0.5) < 1e-12);
    CHECK(std::abs(to_seconds(*make_rational(3, 8)) - 0.375) < 1e-12);
}

Uuid128 test_uuid(std::uint8_t tail) {
    Uuid128 value;
    value.bytes[15] = tail;
    return value;
}

GraphNode make_test_emitter(std::uint8_t id) {
    using namespace graph_keys;
    GraphNode node;
    node.id = NodeId{test_uuid(id)};
    node.type_key = kEmitterNode;
    node.schema_version = 1;
    node.parameters = {
        {kParticleCount, std::uint32_t{100}},
        {kBirthRate, 30.0},
        {kSeed, std::uint32_t{1}},
        {kLifetimeSeconds, 2.0},
        {kEmitterShape, std::uint32_t{0}},
        {kEmitterOrigin, Vec3{}},
        {kVelocity, Vec3{0.0, 0.3, 0.0}},
        {kParticleSize, 8.0},
        {kOpacity, 1.0},
        {kEmitterSize, 0.05},
        {kVelocitySpread, 0.15},
    };
    return node;
}

Graph make_basic_graph() {
    using namespace graph_keys;
    Graph graph;
    graph.nodes.push_back(make_test_emitter(1));
    GraphNode output;
    output.id = NodeId{test_uuid(2)};
    output.type_key = kOutputNode;
    output.schema_version = 1;
    graph.nodes.push_back(std::move(output));
    graph.edges.push_back(GraphEdge{EdgeId{test_uuid(1)}, NodeId{test_uuid(1)}, kEmitterParticles,
                                    NodeId{test_uuid(2)}, kOutputParticles});
    return graph;
}

NodeTypeDescriptor make_test_pass_node(std::string type_key, bool cycle_breaking = false) {
    NodeTypeDescriptor type;
    type.type_key = std::move(type_key);
    type.schema_version = 1;
    type.ports = {
        PortDescriptor{PortKey{1}, PortDirection::input, graph_keys::kParticleStream, false, 0},
        PortDescriptor{PortKey{2}, PortDirection::output, graph_keys::kParticleStream, false, 0},
    };
    if (cycle_breaking) type.cycle_breaking_inputs.push_back(PortKey{1});
    return type;
}

GraphValidationResult validate_test_graph(const Graph& graph) {
    return validate_graph(graph, make_particle_node_registry());
}

void test_graph_contract() {
    using namespace graph_keys;
    Graph graph = make_basic_graph();
    CHECK(validate_test_graph(graph).ok());

    // Stable graph identities do not depend on editor/vector ordering.
    Graph reordered = graph;
    std::reverse(reordered.nodes.begin(), reordered.nodes.end());
    std::reverse(reordered.edges.begin(), reordered.edges.end());
    CHECK(validate_test_graph(reordered).ok());
    CHECK(reordered.edges[0].source_node == graph.edges[0].source_node);
    CHECK(reordered.edges[0].destination_node == graph.edges[0].destination_node);

    Graph bad = graph;
    bad.nodes[0].id = NodeId{};
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::invalid_identifier);

    bad = graph;
    bad.nodes[1].id = bad.nodes[0].id;
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::duplicate_node_id);

    bad = graph;
    bad.nodes[0].type_key = "org.starfieldfx.nodes.not-registered";
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::unknown_node_type);

    bad = graph;
    bad.nodes[0].schema_version = 2;
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::unsupported_node_version);

    bad = graph;
    bad.nodes[0].parameters[1].key = bad.nodes[0].parameters[0].key;
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::duplicate_parameter_key);

    bad = graph;
    bad.nodes[0].parameters[0].key = ParameterKey{999};
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::unknown_parameter);

    bad = graph;
    bad.nodes[0].parameters.pop_back();
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::missing_required_parameter);

    bad = graph;
    bad.nodes[0].parameters[0].value = 100.0;
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::parameter_type_mismatch);

    bad = graph;
    bad.nodes[0].parameters[1].value = std::numeric_limits<double>::infinity();
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::invalid_parameter_value);

    bad = graph;
    bad.edges.clear();
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::missing_required_input);

    bad = graph;
    bad.edges[0].id = EdgeId{};
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::invalid_identifier);

    bad = graph;
    bad.edges.push_back(bad.edges[0]);
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::duplicate_edge_id);

    bad = graph;
    bad.edges[0].source_node = NodeId{test_uuid(77)};
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::unknown_source_node);

    bad = graph;
    bad.edges[0].destination_node = NodeId{test_uuid(77)};
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::unknown_destination_node);

    bad = graph;
    bad.edges[0].source_port = PortKey{999};
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::unknown_source_port);

    bad = graph;
    bad.edges[0].destination_port = PortKey{999};
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::unknown_destination_port);

    // Look types up by key: the registry deliberately grows new node types, and
    // index-based mutations silently stopped testing anything when it did.
    const auto find_type = [](NodeRegistry& registry, const char* key) {
        return std::find_if(registry.types.begin(), registry.types.end(),
                            [key](const NodeTypeDescriptor& type) { return type.type_key == key; });
    };

    NodeRegistry wrong_direction_registry = make_particle_node_registry();
    const auto wrong_direction_emitter = find_type(wrong_direction_registry, kEmitterNode);
    CHECK(wrong_direction_emitter != wrong_direction_registry.types.end());
    if (wrong_direction_emitter != wrong_direction_registry.types.end()) {
        wrong_direction_emitter->ports[0].direction = PortDirection::input;
    }
    CHECK(validate_graph(graph, wrong_direction_registry).error.code == GraphErrorCode::port_direction_mismatch);

    NodeRegistry mismatched_registry = make_particle_node_registry();
    const auto mismatched_output = find_type(mismatched_registry, kOutputNode);
    CHECK(mismatched_output != mismatched_registry.types.end());
    if (mismatched_output != mismatched_registry.types.end()) {
        mismatched_output->ports[0].type_key = "org.starfieldfx.types.texture";
    }
    CHECK(validate_graph(graph, mismatched_registry).error.code == GraphErrorCode::port_type_mismatch);

    Graph two_emitters = graph;
    two_emitters.nodes.insert(two_emitters.nodes.begin(), make_test_emitter(3));
    two_emitters.edges.push_back(GraphEdge{EdgeId{test_uuid(2)}, NodeId{test_uuid(3)}, kEmitterParticles,
                                           NodeId{test_uuid(2)}, kOutputParticles});
    CHECK(validate_test_graph(two_emitters).error.code == GraphErrorCode::duplicate_input_connection);

    NodeRegistry pass_registry;
    pass_registry.types.push_back(make_test_pass_node("org.starfieldfx.nodes.pass"));
    Graph cycle;
    GraphNode first;
    first.id = NodeId{test_uuid(10)};
    first.type_key = "org.starfieldfx.nodes.pass";
    GraphNode second = first;
    second.id = NodeId{test_uuid(11)};
    cycle.nodes = {first, second};
    cycle.edges = {
        GraphEdge{EdgeId{test_uuid(10)}, first.id, PortKey{2}, second.id, PortKey{1}},
        GraphEdge{EdgeId{test_uuid(11)}, second.id, PortKey{2}, first.id, PortKey{1}},
    };
    CHECK(validate_graph(cycle, pass_registry).error.code == GraphErrorCode::cycle_detected);

    NodeRegistry feedback_registry;
    feedback_registry.types.push_back(make_test_pass_node("org.starfieldfx.nodes.pass"));
    feedback_registry.types.push_back(make_test_pass_node("org.starfieldfx.nodes.delay", true));
    Graph feedback;
    GraphNode pass = first;
    GraphNode delay = first;
    delay.id = NodeId{test_uuid(11)};
    delay.type_key = "org.starfieldfx.nodes.delay";
    feedback.nodes = {pass, delay};
    feedback.edges = {
        GraphEdge{EdgeId{test_uuid(10)}, pass.id, PortKey{2}, delay.id, PortKey{1}},
        GraphEdge{EdgeId{test_uuid(11)}, delay.id, PortKey{2}, pass.id, PortKey{1}},
    };
    CHECK(validate_graph(feedback, feedback_registry).ok());

    NodeRegistry duplicate_registry = make_particle_node_registry();
    duplicate_registry.types.push_back(duplicate_registry.types.front());
    CHECK(validate_graph(graph, duplicate_registry).error.code == GraphErrorCode::invalid_registry);

    NodeRegistry invalid_cycle_break = pass_registry;
    invalid_cycle_break.types[0].cycle_breaking_inputs.push_back(PortKey{99});
    CHECK(validate_graph(cycle, invalid_cycle_break).error.code == GraphErrorCode::invalid_registry);

    NodeRegistry text_registry;
    NodeTypeDescriptor text_type;
    text_type.type_key = "org.starfieldfx.nodes.text";
    text_type.parameters.push_back(ParameterDescriptor{ParameterKey{1}, ParameterKind::utf8, true});
    text_registry.types.push_back(std::move(text_type));
    Graph text_graph;
    GraphNode text_node;
    text_node.id = NodeId{test_uuid(12)};
    text_node.type_key = "org.starfieldfx.nodes.text";
    text_node.parameters.push_back(NodeParameter{ParameterKey{1}, std::string("bad\xc0\xaf", 5)});
    text_graph.nodes.push_back(std::move(text_node));
    CHECK(validate_graph(text_graph, text_registry).error.code == GraphErrorCode::invalid_parameter_value);

    bad = graph;
    bad.nodes[0].type_key = "Org.starfieldfx.nodes.emitter";
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::invalid_identifier);

    bad = graph;
    bad.nodes.resize(kMaxGraphNodes + 1);
    CHECK(validate_test_graph(bad).error.code == GraphErrorCode::graph_too_large);
}

void test_put_u16(OpaqueBytes& bytes, std::size_t offset, std::uint16_t value) {
    bytes[offset] = static_cast<std::byte>(value & 0xffu);
    bytes[offset + 1] = static_cast<std::byte>((value >> 8u) & 0xffu);
}

void test_put_u32(OpaqueBytes& bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes[offset + shift / 8] = static_cast<std::byte>((value >> shift) & 0xffu);
    }
}

std::uint16_t test_read_u16(const OpaqueBytes& bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[offset])) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes[offset + 1])) << 8u);
}

std::uint32_t test_read_u32(const OpaqueBytes& bytes, std::size_t offset) {
    std::uint32_t value = 0;
    for (unsigned shift = 0; shift < 32; shift += 8) {
        value |= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset + shift / 8])) << shift;
    }
    return value;
}

std::uint32_t test_crc32(std::span<const std::byte> bytes) {
    std::uint32_t crc = 0xffffffffu;
    for (const auto byte : bytes) {
        crc ^= std::to_integer<std::uint8_t>(byte);
        for (unsigned bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1u) ^ ((0u - (crc & 1u)) & 0xedb88320u);
        }
    }
    return crc ^ 0xffffffffu;
}

void test_refresh_crc(OpaqueBytes& bytes) {
    test_put_u32(bytes, 24, test_crc32(std::span<const std::byte>(bytes).subspan(kSequenceHeaderSize)));
}

void test_append_u16(OpaqueBytes& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::byte>(value & 0xffu));
    bytes.push_back(static_cast<std::byte>((value >> 8u) & 0xffu));
}

void test_append_u32(OpaqueBytes& bytes, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
    }
}

void append_unknown_record(OpaqueBytes& bytes, std::uint16_t kind, std::uint16_t version) {
    test_append_u16(bytes, kind);
    test_append_u16(bytes, version);
    test_append_u32(bytes, 8); // record prefix only
    test_put_u32(bytes, 12, static_cast<std::uint32_t>(bytes.size() - kSequenceHeaderSize));
    test_refresh_crc(bytes);
}

void test_sequence_codec() {
    NodeRegistry registry = make_particle_node_registry();
    Graph graph = make_basic_graph();
    const auto encoded = serialize_graph(graph, registry);
    CHECK(encoded.has_value());
    if (!encoded.has_value()) return;
    CHECK(encoded.value().size() > kSequenceHeaderSize);
    CHECK(test_read_u16(encoded.value(), 8) == kSequenceFormatVersion);
    CHECK(test_read_u16(encoded.value(), 10) == kSequenceHeaderSize);
    CHECK(test_read_u32(encoded.value(), 24) ==
          test_crc32(std::span<const std::byte>(encoded.value()).subspan(kSequenceHeaderSize)));
    CHECK(test_crc32(std::span<const std::byte>{}) == 0u);
    const std::array<std::byte, 9> crc_sample{
        std::byte{0x31}, std::byte{0x32}, std::byte{0x33}, std::byte{0x34}, std::byte{0x35},
        std::byte{0x36}, std::byte{0x37}, std::byte{0x38}, std::byte{0x39},
    };
    CHECK(test_crc32(crc_sample) == 0xcbf43926u);

    const auto decoded = deserialize_graph(encoded.value(), registry);
    CHECK(decoded.has_value());
    if (decoded.has_value()) {
        CHECK(decoded.value().nodes.size() == graph.nodes.size());
        CHECK(decoded.value().edges.size() == graph.edges.size());
        CHECK(decoded.value().nodes[0].id == graph.nodes[0].id);
        CHECK(decoded.value().nodes[0].parameters.size() == graph.nodes[0].parameters.size());
        CHECK(decoded.value().edges[0].source_node == graph.edges[0].source_node);
        CHECK(decoded.value().edges[0].destination_port == graph.edges[0].destination_port);
    }

    Graph reordered = graph;
    std::reverse(reordered.nodes.begin(), reordered.nodes.end());
    std::reverse(reordered.edges.begin(), reordered.edges.end());
    const auto canonical_reordered = serialize_graph(reordered, registry);
    CHECK(canonical_reordered.has_value());
    if (canonical_reordered.has_value()) CHECK(canonical_reordered.value() == encoded.value());

    NodeRegistry value_registry;
    NodeTypeDescriptor value_type;
    value_type.type_key = "org.starfieldfx.nodes.values";
    value_type.parameters = {
        {ParameterKey{1}, ParameterKind::boolean, true},
        {ParameterKey{2}, ParameterKind::int32, true},
        {ParameterKey{3}, ParameterKind::uint32, true},
        {ParameterKey{4}, ParameterKind::float64, true},
        {ParameterKey{5}, ParameterKind::vector3_float64, true},
        {ParameterKey{6}, ParameterKind::utf8, true},
        {ParameterKey{7}, ParameterKind::opaque_bytes, true},
    };
    value_registry.types.push_back(value_type);
    Graph value_graph;
    GraphNode value_node;
    value_node.id = NodeId{test_uuid(22)};
    value_node.type_key = value_type.type_key;
    value_node.parameters = {
        {ParameterKey{1}, true},
        {ParameterKey{2}, std::int32_t{-123456}},
        {ParameterKey{3}, std::uint32_t{345678}},
        {ParameterKey{4}, 3.125},
        {ParameterKey{5}, Vec3{-1.25, 2.5, 0.125}},
        {ParameterKey{6}, std::string("\xe7\xb2\x92\xe5\xad\x90\xe2\x9c\xa8")},
        {ParameterKey{7}, OpaqueBytes{std::byte{0x00}, std::byte{0x7f}, std::byte{0xff}}},
    };
    value_graph.nodes.push_back(value_node);
    const auto values_encoded = serialize_graph(value_graph, value_registry);
    CHECK(values_encoded.has_value());
    if (!values_encoded.has_value()) return;
    const auto values_decoded = deserialize_graph(values_encoded.value(), value_registry);
    CHECK(values_decoded.has_value());
    if (values_decoded.has_value()) {
        const auto& values = values_decoded.value().nodes[0].parameters;
        CHECK(std::get<bool>(values[0].value));
        CHECK(std::get<std::int32_t>(values[1].value) == -123456);
        CHECK(std::get<std::uint32_t>(values[2].value) == 345678u);
        CHECK(std::abs(std::get<double>(values[3].value) - 3.125) < 1e-12);
        CHECK(std::get<Vec3>(values[4].value).x == -1.25);
        CHECK(std::get<std::string>(values[5].value) == std::string("\xe7\xb2\x92\xe5\xad\x90\xe2\x9c\xa8"));
        CHECK(std::get<OpaqueBytes>(values[6].value) == std::get<OpaqueBytes>(value_node.parameters[6].value));
    }

    auto malformed = encoded.value();
    malformed.pop_back();
    CHECK(deserialize_graph(malformed, registry).error().code == SequenceErrorCode::length_mismatch);

    malformed = encoded.value();
    malformed[0] = std::byte{0};
    CHECK(deserialize_graph(malformed, registry).error().code == SequenceErrorCode::invalid_header);

    malformed = encoded.value();
    test_put_u16(malformed, 8, 2);
    CHECK(deserialize_graph(malformed, registry).error().code == SequenceErrorCode::unsupported_format_version);

    malformed = encoded.value();
    test_put_u16(malformed, 10, 31);
    CHECK(deserialize_graph(malformed, registry).error().code == SequenceErrorCode::invalid_header);

    malformed = encoded.value();
    test_put_u32(malformed, 12, static_cast<std::uint32_t>(malformed.size()));
    CHECK(deserialize_graph(malformed, registry).error().code == SequenceErrorCode::length_mismatch);

    malformed = encoded.value();
    test_put_u32(malformed, 28, 1);
    CHECK(deserialize_graph(malformed, registry).error().code == SequenceErrorCode::unsupported_flags);

    malformed = encoded.value();
    test_put_u32(malformed, 16, static_cast<std::uint32_t>(kMaxGraphNodes + 1));
    CHECK(deserialize_graph(malformed, registry).error().code == SequenceErrorCode::size_limit_exceeded);

    malformed = encoded.value();
    malformed.back() ^= std::byte{1};
    CHECK(deserialize_graph(malformed, registry).error().code == SequenceErrorCode::checksum_mismatch);

    malformed = encoded.value();
    test_put_u32(malformed, 36, 4); // first record size is smaller than its prefix
    test_refresh_crc(malformed);
    CHECK(deserialize_graph(malformed, registry).error().code == SequenceErrorCode::malformed_record);

    malformed = encoded.value();
    test_put_u16(malformed, 34, 2); // unsupported node-record schema
    test_refresh_crc(malformed);
    CHECK(deserialize_graph(malformed, registry).error().code == SequenceErrorCode::unsupported_record_version);

    malformed = encoded.value();
    append_unknown_record(malformed, 0x8001, 99); // optional unknown record is skipped
    const auto optional_decode = deserialize_graph(malformed, registry);
    CHECK(optional_decode.has_value());

    malformed = encoded.value();
    append_unknown_record(malformed, 3, 1); // required unknown record fails
    CHECK(deserialize_graph(malformed, registry).error().code == SequenceErrorCode::unsupported_record);

    malformed = encoded.value();
    const auto key_length = test_read_u16(malformed, 56);
    const std::size_t first_parameter_type_offset = 62u + key_length + 8u;
    test_put_u16(malformed, first_parameter_type_offset, 99);
    test_refresh_crc(malformed);
    CHECK(deserialize_graph(malformed, registry).error().code == SequenceErrorCode::unsupported_value_type);

    malformed = values_encoded.value();
    const auto values_key_length = test_read_u16(malformed, 56);
    const std::size_t first_value_offset = 62u + values_key_length + 8u + 2u + 4u;
    malformed[first_value_offset] = std::byte{2}; // bool values are exactly 0 or 1
    test_refresh_crc(malformed);
    CHECK(deserialize_graph(malformed, value_registry).error().code == SequenceErrorCode::invalid_value);

    Graph invalid_graph = graph;
    invalid_graph.nodes[0].type_key = "org.starfieldfx.nodes.not-registered";
    const auto invalid_encoded = serialize_graph(invalid_graph, registry);
    CHECK(!invalid_encoded.has_value());
    if (!invalid_encoded.has_value()) CHECK(invalid_encoded.error().code == SequenceErrorCode::invalid_graph);
}

void test_layer_point_conversion() {
    // Documented host delivery: absolute layer pixels with the origin at the layer
    // top-left, x growing right and y growing down.
    const LayerUnits units{1920.0, 1080.0, 1.0};
    const Vec3 centre = layer_point_to_world(960.0, 540.0, 540.0, units);
    CHECK(std::abs(centre.x) < 1e-12);
    CHECK(std::abs(centre.y) < 1e-12);
    CHECK(std::abs(centre.z) < 1e-12);

    const Vec3 top_left = layer_point_to_world(0.0, 0.0, 0.0, units);
    CHECK(std::abs(top_left.x + (1920.0 / 1080.0) * 0.5) < 1e-12);
    CHECK(std::abs(top_left.y - 0.5) < 1e-12);
    CHECK(std::abs(top_left.z + 0.5) < 1e-12);

    // Anamorphic hosts: one layer width equals (width * aspect / height) layer heights.
    const LayerUnits anamorphic{720.0, 480.0, 2.0};
    const Vec3 right_edge = layer_point_to_world(720.0, 240.0, 240.0, anamorphic);
    CHECK(std::abs(right_edge.x - 1.5) < 1e-12);
    CHECK(std::abs(right_edge.y) < 1e-12);

    // The host adapter restores preview scale, then the core preserves absolute pixel
    // coordinates exactly. Off-layer points remain valid positions; non-finite inputs
    // are rejected at this boundary.
    CHECK(host_point_component_to_layer_pixels(960.0) == 960.0);
    CHECK(host_point_component_to_layer_pixels(50.0) == 50.0);
    CHECK(host_point_component_to_layer_pixels(1.0e6) == 1.0e6);
    CHECK(host_point_component_to_layer_pixels(-540.0) == -540.0);
    CHECK(host_point_component_to_layer_pixels(std::numeric_limits<double>::infinity()) == 0.0);
}

void test_settings_validation() {
    Settings settings;
    settings.birth_rate = std::numeric_limits<double>::quiet_NaN();
    settings.particle_count = kMaxParticleCount + 10;
    settings.seed = kMaxSeed + 5;
    settings.opacity = 2.0;
    settings.particle_size = -1.0;
    settings.emitter_shape = static_cast<EmitterShape>(9);
    settings.emitter_origin = Vec3{1.0e6, -1.0e6, 0.0};
    settings.velocity = Vec3{1.0e9, 0.0, std::numeric_limits<double>::infinity()};

    const ValidatedSettings validated = validate_settings(settings);
    CHECK(std::isfinite(validated.value.birth_rate));
    CHECK(validated.value.birth_rate == 30.0); // documented default replaces non-finite input
    CHECK(validated.value.particle_count == kMaxParticleCount);
    CHECK(validated.value.seed == kMaxSeed);
    CHECK(validated.value.opacity == 1.0);
    CHECK(validated.value.particle_size == 0.0);
    CHECK(validated.value.emitter_shape == EmitterShape::point);
    CHECK(validated.value.emitter_origin.x == kMaxEmitterOffset);
    CHECK(validated.value.emitter_origin.y == -kMaxEmitterOffset);
    CHECK(validated.value.velocity.x == kMaxVelocity);
    CHECK(validated.value.velocity.z == 0.0); // non-finite replaced, then clamped
    CHECK(validated.notices.size() >= 8);
}

void test_simulation_emitter_origin() {
    const CancellingAfterFirstPoll never(false);

    Settings settings;
    settings.particle_count = 8; // population cap must not hide the travelled particle
    settings.birth_rate = 1.0;
    settings.particle_lifetime_seconds = 4.0;
    settings.emitter_origin = Vec3{0.25, -0.5, 0.125};
    settings.velocity = Vec3{0.0, 0.0, 0.0};
    settings.velocity_spread = 0.0; // isolate the closed-form origin math

    const auto stationary = simulate_particles(validate_settings(settings), 0.5, never);
    CHECK(stationary.has_value() && stationary.value().size() == 1);
    if (stationary.has_value() && stationary.value().size() == 1) {
        const ParticleInstance& particle = stationary.value().front();
        CHECK(std::abs(particle.age_seconds - 0.5) < 1e-12);
        CHECK(std::abs(particle.position.x - 0.25) < 1e-12);
        CHECK(std::abs(particle.position.y + 0.5) < 1e-12);
        CHECK(std::abs(particle.position.z - 0.125) < 1e-12);
    }

    // Straight-line motion is added on top of the origin: the newest particle sits at
    // the origin and the oldest has travelled for its whole life.
    Settings moving = settings;
    moving.velocity = Vec3{0.4, 0.2, 0.0};
    const auto travelled = simulate_particles(validate_settings(moving), 1.0, never);
    CHECK(travelled.has_value() && travelled.value().size() == 2);
    if (travelled.has_value() && travelled.value().size() == 2) {
        const ParticleInstance& newest = travelled.value().back();
        CHECK(std::abs(newest.age_seconds) < 1e-12);
        CHECK(std::abs(newest.position.x - 0.25) < 1e-9);
        CHECK(std::abs(newest.position.y + 0.5) < 1e-9);

        const ParticleInstance& oldest = travelled.value().front();
        CHECK(std::abs(oldest.age_seconds - 1.0) < 1e-9);
        CHECK(std::abs(oldest.position.x - 0.65) < 1e-9);
        CHECK(std::abs(oldest.position.y + 0.3) < 1e-9);
    }
}

void test_random_streams() {
    CHECK(mix64(0) == mix64(0));
    CHECK(mix64(1) != mix64(2));

    const std::uint64_t bits = stream_bits(1, 1, RandomPurpose::velocity_x);
    CHECK(bits == stream_bits(1, 1, RandomPurpose::velocity_x));
    CHECK(bits != stream_bits(2, 1, RandomPurpose::velocity_x)); // seed matters
    CHECK(bits != stream_bits(1, 2, RandomPurpose::velocity_x)); // particle id matters
    CHECK(bits != stream_bits(1, 1, RandomPurpose::velocity_y)); // purpose matters

    for (std::uint64_t id = 0; id < 128; ++id) {
        const double unit = unit_value(1, id, RandomPurpose::position_x);
        const double symmetric = symmetric_value(1, id, RandomPurpose::position_x);
        CHECK(unit >= 0.0 && unit < 1.0);
        CHECK(symmetric >= -1.0 && symmetric < 1.0);
    }
}

void test_emitter_shapes_and_spread() {
    const CancellingAfterFirstPoll never(false);

    Settings settings;
    settings.particle_count = 256;
    settings.birth_rate = 256.0;
    settings.particle_lifetime_seconds = 1.0;
    settings.emitter_size = 0.2; // half extent 0.1 layer heights
    settings.velocity = Vec3{0.0, 0.3, 0.0};
    settings.velocity_spread = 0.2;

    // Point without spread reproduces the closed-form behaviour M2 shipped.
    Settings flat = settings;
    flat.emitter_shape = EmitterShape::point;
    flat.velocity_spread = 0.0;
    const auto point = simulate_particles(validate_settings(flat), 1.0, never);
    CHECK(point.has_value() && !point.value().empty());
    if (point.has_value() && !point.value().empty()) {
        for (const ParticleInstance& particle : point.value()) {
            CHECK(std::abs(particle.position.x) < 1e-12);
            CHECK(std::abs(particle.position.y - 0.3 * particle.age_seconds) < 1e-12);
        }
    }

    // Every shape stays inside its advertised extent and actually spreads.
    const EmitterShape shapes[3] = {EmitterShape::box, EmitterShape::sphere, EmitterShape::disc};
    for (const EmitterShape shape : shapes) {
        Settings shaped = settings;
        shaped.emitter_shape = shape;
        shaped.velocity = Vec3{0.0, 0.0, 0.0}; // isolate the birth distribution
        shaped.velocity_spread = 0.0;
        const auto particles = simulate_particles(validate_settings(shaped), 1.0, never);
        CHECK(particles.has_value());
        if (!particles.has_value()) {
            continue;
        }
        CHECK(particles.value().size() > 16);
        double furthest = 0.0;
        for (const ParticleInstance& particle : particles.value()) {
            if (shape == EmitterShape::disc) {
                CHECK(std::abs(particle.position.z) < 1e-12);
            }
            CHECK(std::abs(particle.position.x) <= 0.1 + 1e-12);
            CHECK(std::abs(particle.position.y) <= 0.1 + 1e-12);
            CHECK(std::abs(particle.position.z) <= 0.1 + 1e-12);
            furthest = std::max(furthest, std::abs(particle.position.x));
        }
        CHECK(furthest > 0.02); // the seeded distribution is not collapsed onto the centre
    }

    // Velocity spread is bounded per axis, actually applied, and sticks to a particle.
    const auto jittered = simulate_particles(validate_settings(settings), 1.0, never);
    CHECK(jittered.has_value() && !jittered.value().empty());
    if (!jittered.has_value() || jittered.value().empty()) {
        return;
    }
    bool deviates = false;
    for (const ParticleInstance& particle : jittered.value()) {
        if (particle.age_seconds <= 0.0) {
            continue;
        }
        const double velocity_y = particle.position.y / particle.age_seconds;
        CHECK(std::abs(velocity_y - 0.3) <= 0.2 + 1e-9);
        if (std::abs(velocity_y - 0.3) > 0.05) {
            deviates = true;
        }
    }
    CHECK(deviates);

    const auto later = simulate_particles(validate_settings(settings), 1.2, never);
    CHECK(later.has_value());
    if (later.has_value()) {
        for (const ParticleInstance& particle : jittered.value()) {
            if (particle.age_seconds <= 0.0) {
                continue;
            }
            for (const ParticleInstance& moved : later.value()) {
                if (moved.id != particle.id || moved.age_seconds <= 0.0) {
                    continue;
                }
                const double before = particle.position.y / particle.age_seconds;
                const double after = moved.position.y / moved.age_seconds;
                CHECK(std::abs(before - after) < 1e-9);
                break;
            }
            break; // one stable particle is enough to pin the contract
        }
    }

    // Random Seed is observable now: same request reproduces exactly, another seed
    // moves the particles.
    Settings other_seed = settings;
    other_seed.seed = 7;
    const auto repeated = simulate_particles(validate_settings(settings), 1.0, never);
    const auto reseeded = simulate_particles(validate_settings(other_seed), 1.0, never);
    CHECK(repeated.has_value() && reseeded.has_value());
    if (repeated.has_value() && reseeded.has_value()) {
        CHECK(repeated.value().size() == jittered.value().size());
        bool identical = true;
        bool different = false;
        for (std::size_t i = 0; i < reseeded.value().size() && i < jittered.value().size(); ++i) {
            if (repeated.value()[i].position.y != jittered.value()[i].position.y) {
                identical = false;
            }
            if (reseeded.value()[i].position.y != jittered.value()[i].position.y) {
                different = true;
            }
        }
        CHECK(identical);
        CHECK(different);
    }
}

void test_simulation_boundaries() {
    Settings settings;
    settings.birth_rate = 10.0;
    settings.particle_lifetime_seconds = 1.0;
    settings.particle_count = 1000;
    settings.velocity = Vec3{0.5, 0.25, 0.0};
    settings.velocity_spread = 0.0; // the boundary cases assert exact positions

    const ValidatedSettings validated = validate_settings(settings);
    const CancellingAfterFirstPoll never(false);

    // Half-open lifetime: at t = 1.0 slot 0 has age 1.0 and is gone, slot 10 is born.
    const auto at_one = simulate_particles(validated, 1.0, never);
    CHECK(at_one.has_value());
    CHECK(at_one.value().size() == 10);
    CHECK(at_one.value().front().id == 1);
    CHECK(at_one.value().back().id == 10);
    CHECK(std::abs(at_one.value().back().age_seconds) < 1e-12);
    CHECK(std::abs(at_one.value().back().position.y) < 1e-12);
    CHECK(std::abs(at_one.value().front().age_seconds - 0.9) < 1e-9);
    CHECK(std::abs(at_one.value().front().position.y - 0.225) < 1e-9);
    CHECK(at_one.value().front().size_pixels == settings.particle_size);
    CHECK(at_one.value().front().lifetime_seconds == settings.particle_lifetime_seconds);

    // The clock is anchored at host time zero.
    const auto at_zero = simulate_particles(validated, 0.0, never);
    CHECK(at_zero.has_value() && at_zero.value().size() == 1 && at_zero.value().front().id == 0);

    const auto before_zero = simulate_particles(validated, -0.5, never);
    CHECK(before_zero.has_value() && before_zero.value().empty());

    Settings no_births = settings;
    no_births.birth_rate = 0.0;
    CHECK(simulate_particles(validate_settings(no_births), 5.0, never).value().empty());

    Settings instant = settings;
    instant.particle_lifetime_seconds = 0.0;
    CHECK(simulate_particles(validate_settings(instant), 5.0, never).value().empty());

    Settings capped = settings;
    capped.birth_rate = 1000.0;
    capped.particle_count = 5;
    const auto capped_result = simulate_particles(validate_settings(capped), 1.0, never);
    CHECK(capped_result.value().size() == 5);
    CHECK(capped_result.value().front().id == 996); // the newest slots survive
    CHECK(capped_result.value().back().id == 1000);

    // Frame order must not matter: repeated and inverted evaluation agree.
    const auto forward = simulate_particles(validated, 2.0, never);
    const auto earlier = simulate_particles(validated, 1.5, never);
    const auto repeated = simulate_particles(validated, 2.0, never);
    CHECK(forward.value().size() == repeated.value().size());
    for (std::size_t i = 0; i < forward.value().size(); ++i) {
        CHECK(forward.value()[i].id == repeated.value()[i].id);
        CHECK(forward.value()[i].age_seconds == repeated.value()[i].age_seconds);
        CHECK(forward.value()[i].position.y == repeated.value()[i].position.y);
    }
    CHECK(!earlier.value().empty());

    const CancellingAfterFirstPoll cancelled(true);
    const auto cancelled_result = simulate_particles(validated, 1.0, cancelled);
    CHECK(!cancelled_result.has_value());
    CHECK(cancelled_result.error().code == ErrorCode::cancelled);
}

void test_renderer_determinism_and_geometry() {
    const CpuParticleRenderer renderer;
    const CancellingAfterFirstPoll never(false);

    Scene scene;
    scene.settings.particle_count = 8;
    scene.settings.birth_rate = 8.0;
    scene.settings.particle_lifetime_seconds = 1.0;
    scene.settings.particle_size = 6.0;
    scene.settings.velocity = Vec3{0.0, 0.5, 0.0};
    scene.settings.velocity_spread = 0.0; // the trail rows below are asserted exactly
    scene.time_seconds = 0.95;

    const RenderRequest request = build_request(scene);
    const auto first = renderer.render(request, never);
    const auto second = renderer.render(request, never);
    CHECK(first.has_value() && second.has_value());
    CHECK(first.value().pixels == second.value().pixels);
    CHECK(first.value().row_bytes == 64u * 4u);

    // Eight particles, velocity 0.5 layer-heights/s, at t = 0.95 form a trail from
    // y = 0.075 * 32 px up to y = 0.95 * 32 px above the center. Rows above the
    // center must carry alpha, rows below the trail must stay empty, and the sprite
    // must never touch the frame corners.
    CHECK(pixel8(first.value(), 32, 24).a > 0);
    CHECK(pixel8(first.value(), 32, 40).a == 0);
    CHECK(pixel8(first.value(), 0, 0).a == 0);
    CHECK(pixel8(first.value(), 63, 0).a == 0);
    CHECK(pixel8(first.value(), 63, 63).a == 0);
}

void test_renderer_region_of_interest() {
    const CpuParticleRenderer renderer;
    const CancellingAfterFirstPoll never(false);

    Scene full;
    full.settings.particle_count = 6;
    full.settings.birth_rate = 6.0;
    full.settings.particle_lifetime_seconds = 1.0;
    full.settings.particle_size = 8.0;
    full.settings.velocity = Vec3{0.4, 0.2, 0.0};
    full.time_seconds = 0.9;

    Scene partial = full;
    partial.roi = RectI{32, 0, 64, 64};

    const auto full_output = renderer.render(build_request(full), never);
    const auto partial_output = renderer.render(build_request(partial), never);
    CHECK(full_output.has_value() && partial_output.has_value());
    CHECK(partial_output.value().width() == 32 && partial_output.value().height() == 64);

    bool identical = true;
    for (std::uint32_t y = 0; y < 64 && identical; ++y) {
        for (std::uint32_t x = 0; x < 32; ++x) {
            const Rgba8 expected = pixel8(full_output.value(), x + 32, y);
            const Rgba8 actual = pixel8(partial_output.value(), x, y);
            if (expected.r != actual.r || expected.g != actual.g || expected.b != actual.b ||
                expected.a != actual.a) {
                identical = false;
                break;
            }
        }
    }
    CHECK(identical);
}

void test_renderer_downsampled_frame_mapping() {
    // The frame grid is the render-resolution layer grid and may be smaller than the
    // full-resolution layer size (downsampled preview). World-to-pixel mapping must
    // follow the frame grid, otherwise particles drift or get clipped at low
    // preview resolutions (ADR 0005).
    const CpuParticleRenderer renderer;
    const CancellingAfterFirstPoll never(false);

    Scene scene;
    scene.layer_width = 256;
    scene.layer_height = 256;
    scene.frame_width = 128; // half-resolution preview
    scene.frame_height = 128;
    scene.roi = RectI{0, 0, 128, 128};
    scene.settings.particle_count = 1;
    scene.settings.birth_rate = 1.0;
    scene.settings.particle_lifetime_seconds = 4.0;
    scene.settings.particle_size = 8.0;   // radius 4 full-res pixels -> 2 at half res
    scene.settings.velocity = Vec3{0.0, 0.5, 0.0};
    scene.settings.velocity_spread = 0.0; // the mapped row is asserted exactly
    scene.time_seconds = 0.5;             // one particle, age 0.5 s, y = +0.25 layer heights

    const auto output = renderer.render(build_request(scene), never);
    CHECK(output.has_value());
    if (!output.has_value()) {
        return;
    }
    CHECK(output.value().width() == 128 && output.value().height() == 128);

    // (0.5 - 0.25) * 128 = row 32, so the sprite center sits on row 32 and nothing
    // reaches row 40. At full resolution the same scene would land on row 64.
    CHECK(pixel8(output.value(), 64, 32).a > 0);
    CHECK(pixel8(output.value(), 64, 40).a == 0);
    CHECK(pixel8(output.value(), 64, 0).a == 0);

    Scene full = scene;
    full.frame_width = 256;
    full.frame_height = 256;
    full.roi = RectI{0, 0, 256, 256};
    const auto full_output = renderer.render(build_request(full), never);
    CHECK(full_output.has_value());
    if (full_output.has_value()) {
        CHECK(pixel8(full_output.value(), 128, 64).a > 0);
        CHECK(pixel8(full_output.value(), 128, 32).a == 0);
    }
}

void test_renderer_transparent_particle_output() {
    const CpuParticleRenderer renderer;
    const CancellingAfterFirstPoll never(false);

    Scene scene;
    scene.settings.particle_count = 1;
    scene.settings.birth_rate = 1.0;
    scene.settings.particle_lifetime_seconds = 4.0;
    scene.settings.particle_size = 0.0;
    scene.settings.velocity = Vec3{0.0, 0.0, 0.0};
    scene.time_seconds = 1.0;

    const auto empty = renderer.render(build_request(scene), never);
    CHECK(empty.has_value());
    if (!empty.has_value()) return;
    const Rgba8 clear_pixel = pixel8(empty.value(), 5, 5);
    CHECK(clear_pixel.r == 0 && clear_pixel.g == 0 && clear_pixel.b == 0 && clear_pixel.a == 0);

    scene.settings.particle_size = 10.0;
    scene.settings.opacity = 0.5;
    const auto particles = renderer.render(build_request(scene), never);
    CHECK(particles.has_value());
    if (!particles.has_value()) return;
    const Rgba8 center = pixel8(particles.value(), 32, 32);
    CHECK(center.a >= 126 && center.a <= 129);
    CHECK(center.r <= center.a && center.g <= center.a && center.b <= center.a);
    const Rgba8 background = pixel8(particles.value(), 5, 5);
    CHECK(background.r == 0 && background.g == 0 && background.b == 0 && background.a == 0);
}

void test_renderer_formats_and_limits() {
    const CancellingAfterFirstPoll never(false);
    const CpuParticleRenderer renderer;

    Scene scene;
    scene.settings.particle_count = 1;
    scene.settings.birth_rate = 1.0;
    scene.settings.particle_lifetime_seconds = 4.0;
    scene.settings.particle_size = 8.0;
    scene.settings.velocity = Vec3{0.0, 0.0, 0.0}; // the sprite must sit at the layer centre
    scene.settings.velocity_spread = 0.0;
    scene.time_seconds = 1.0;

    Scene deep = scene;
    deep.format = PixelFormat::rgba16;
    const auto output16 = renderer.render(build_request(deep), never);
    CHECK(output16.has_value());
    CHECK(output16.value().row_bytes == 64u * 8u);
    CHECK(channel16(output16.value(), 32, 32, 3) == 32768);

    Scene floating = scene;
    floating.format = PixelFormat::rgba32f;
    const auto output32 = renderer.render(build_request(floating), never);
    CHECK(output32.has_value());
    CHECK(output32.value().row_bytes == 64u * 16u);
    CHECK(std::abs(channel32(output32.value(), 32, 32, 3) - 1.0f) < 1e-5f);
    CHECK(std::abs(channel32(output32.value(), 0, 0, 3)) < 1e-6f);

    // Invalid geometry is rejected before any allocation.
    Scene broken = scene;
    broken.roi = RectI{0, 0, 128, 64};
    const auto invalid = renderer.render(build_request(broken), never);
    CHECK(!invalid.has_value());
    CHECK(invalid.error().code == ErrorCode::invalid_request);

    Scene no_duration = scene;
    RenderRequest no_duration_request = build_request(no_duration);
    no_duration_request.frame.frame_duration = RationalTime{0, 24};
    CHECK(!renderer.render(no_duration_request, never).has_value());

    // An empty region of interest is legal and produces an empty output.
    Scene empty_roi = scene;
    empty_roi.roi = RectI{10, 10, 10, 40};
    const auto empty = renderer.render(build_request(empty_roi), never);
    CHECK(empty.has_value());
    CHECK(empty.value().pixels.empty());

    // The bounded-work budget is reported instead of being silently truncated.
    const CpuParticleRenderer budgeted(RenderLimits{256});
    Scene heavy = scene;
    heavy.settings.particle_count = 64;
    heavy.settings.birth_rate = 64.0;
    heavy.settings.particle_size = 64.0;
    const auto limited = budgeted.render(build_request(heavy), never);
    CHECK(!limited.has_value());
    CHECK(limited.error().code == ErrorCode::work_limit_exceeded);

    // Cancellation reaches the rasterizer as well.
    const CancellingAfterFirstPoll cancelled(true);
    const auto cancelled_output = renderer.render(build_request(scene), cancelled);
    CHECK(!cancelled_output.has_value());
    CHECK(cancelled_output.error().code == ErrorCode::cancelled);

    // Final pixel encoding observes cancellation even when no particles are alive.
    Scene encode_only = scene;
    encode_only.settings.particle_count = 0;
    encode_only.settings.birth_rate = 0.0;
    const CancellingAtPoll cancel_during_encode(3); // initial check, then two encoded rows
    const auto cancelled_encode = renderer.render(build_request(encode_only), cancel_during_encode);
    CHECK(!cancelled_encode.has_value());
    CHECK(cancelled_encode.error().code == ErrorCode::cancelled);
}

void test_graph_evaluation() {
    using namespace graph_keys;
    const NeverCancelled never;
    const CpuParticleRenderer renderer;
    const NodeId emitter{test_uuid(9)};
    const NodeId output{test_uuid(1)}; // dependency order must beat UUID order
    const EdgeId edge{test_uuid(7)};

    for (const auto shape : {EmitterShape::point, EmitterShape::box, EmitterShape::sphere, EmitterShape::disc}) {
        for (const auto format : {PixelFormat::rgba8, PixelFormat::rgba16, PixelFormat::rgba32f}) {
            Scene scene;
            scene.settings.emitter_shape = shape;
            scene.settings.opacity = 0.37;
            scene.settings.seed = 197;
            scene.settings.emitter_size = 0.3;
            scene.settings.velocity = Vec3{0.1, -0.05, 0.3};
            scene.settings.emitter_origin = Vec3{0.1, 0.05, 0.2};
            scene.format = format;
            scene.pixel_aspect = 1.2;
            auto made = make_emitter_output_graph(scene.settings, emitter, output, edge);
            CHECK(made.has_value());
            if (!made.has_value()) continue;
            auto graph = made.take_value();
            // Parked emitters validate but do not execute or change the stream.
            graph.nodes.push_back(make_test_emitter(22));
            std::reverse(graph.nodes.begin(), graph.nodes.end());
            for (auto& node : graph.nodes) std::reverse(node.parameters.begin(), node.parameters.end());
            auto snapshot = std::make_shared<const Graph>(graph);
            const auto serialized = serialize_graph(graph, particle_node_registry());
            CHECK(serialized.has_value());
            // Repeated, reverse, negative, birth-boundary and NTSC subframe time.
            for (const auto time : {RationalTime{3, 2}, RationalTime{1001, 30000}, RationalTime{-1, 24},
                                    RationalTime{0, 1}, RationalTime{1, 1}, RationalTime{3, 2}}) {
                auto flat = build_request(scene);
                flat.frame.time = time;
                auto request = flat;
                request.graph = snapshot;
                request.settings.value.opacity = 0.0;
                request.settings.value.particle_count = 0; // graph must take precedence
                const auto expected = renderer.render(flat, never);
                const auto actual = renderer.render(request, never);
                CHECK(expected.has_value() && actual.has_value());
                if (expected.has_value() && actual.has_value()) {
                    CHECK(expected.value().pixels == actual.value().pixels);
                    CHECK(expected.value().row_bytes == actual.value().row_bytes);
                }
                const auto evaluated = evaluate_particle_graph(graph, time, never);
                CHECK(evaluated.has_value());
                if (evaluated.has_value()) CHECK(evaluated.value().evaluated_nodes == std::vector<NodeId>({emitter, output}));
                // Half-resolution cropped output follows the same graph boundary.
                flat.frame.frame_width = request.frame.frame_width = 32;
                flat.frame.frame_height = request.frame.frame_height = 32;
                flat.frame.region_of_interest = request.frame.region_of_interest = RectI{5, 4, 29, 27};
                const auto small_expected = renderer.render(flat, never);
                const auto small_actual = renderer.render(request, never);
                CHECK(small_expected.has_value() && small_actual.has_value());
                if (small_expected.has_value() && small_actual.has_value()) CHECK(small_expected.value().pixels == small_actual.value().pixels);
            }
            const auto after = serialize_graph(graph, particle_node_registry());
            CHECK(after.has_value());
            if (serialized.has_value() && after.has_value()) CHECK(serialized.value() == after.value());
        }
    }

    auto made = make_emitter_output_graph(Settings{}, emitter, output, edge);
    CHECK(made.has_value());
    if (!made.has_value()) return;
    const Graph graph = made.take_value();
    const auto rejects = [&](const Graph& bad, ErrorCode code = ErrorCode::invalid_request) {
        const auto result = evaluate_particle_graph(bad, RationalTime{1, 1}, never);
        CHECK(!result.has_value());
        if (!result.has_value()) CHECK(result.error().code == code);
        auto request = build_request(Scene{});
        request.graph = std::make_shared<const Graph>(bad);
        const auto pixels = renderer.render(request, never);
        CHECK(!pixels.has_value()); // invalid graph must never fall back to flat settings
    };
    rejects(Graph{});
    auto bad = graph;
    bad.edges.clear();
    rejects(bad);
    bad = graph;
    bad.nodes[0].parameters[0].value = std::uint32_t{kMaxParticleCount + 1};
    rejects(bad);
    bad = graph;
    bad.nodes[0].parameters[4].value = std::uint32_t{256}; // must not wrap uint8 enum
    rejects(bad);
    bad = graph;
    bad.nodes[0].parameters[8].value = -0.5;
    rejects(bad);
    bad = graph;
    bad.nodes[0].parameters[1].value = std::numeric_limits<double>::infinity();
    rejects(bad);
    bad = graph;
    bad.nodes.push_back(GraphNode{NodeId{test_uuid(3)}, kOutputNode, 1, {}});
    bad.edges.push_back(GraphEdge{EdgeId{test_uuid(8)}, emitter, kEmitterParticles, bad.nodes.back().id, kOutputParticles});
    CHECK(validate_graph(bad, particle_node_registry()).ok());
    rejects(bad); // structurally legal, but ambiguous output selection
    bad = graph;
    bad.nodes.push_back(make_test_emitter(22));
    bad.nodes.back().parameters[8].value = -1.0;
    rejects(bad); // disconnected semantic errors are still reported

    const auto time_error = evaluate_particle_graph(graph, RationalTime{1, 0}, never);
    CHECK(!time_error.has_value());
    if (!time_error.has_value()) CHECK(time_error.error().code == ErrorCode::invalid_time);
    struct CancelAtPoll final : Cancellation {
        mutable unsigned polls{0};
        unsigned limit{0};
        bool is_cancelled() const noexcept override { return ++polls >= limit; }
    };
    for (unsigned limit : {1u, 3u, 8u, 10u}) {
        CancelAtPoll cancelled;
        cancelled.limit = limit;
        const auto result = evaluate_particle_graph(graph, RationalTime{1, 1}, cancelled);
        CHECK(!result.has_value());
        if (!result.has_value()) CHECK(result.error().code == ErrorCode::cancelled);
    }
    CHECK(!make_emitter_output_graph(Settings{}, emitter, emitter, edge).has_value());
    Settings invalid;
    invalid.opacity = 1.5;
    CHECK(!make_emitter_output_graph(invalid, emitter, output, edge).has_value());
}

// Regression coverage for the emitter -> force -> appearance -> output chain: the
// closed-form gravity/drag integration, age-driven size/opacity/color, stage-order
// enforcement, and graph/flat pixel parity. Added with the force/appearance kernels.
void test_force_and_appearance() {
    using namespace graph_keys;
    const NeverCancelled never;
    const CpuParticleRenderer renderer;
    const NodeId emitter{test_uuid(9)};
    const NodeId force{test_uuid(4)};
    const NodeId appearance{test_uuid(6)};
    const NodeId output{test_uuid(1)};
    const EdgeId emitter_to_force{test_uuid(10)};
    const EdgeId force_to_appearance{test_uuid(11)};
    const EdgeId appearance_to_output{test_uuid(12)};

    Settings settings;
    settings.particle_count = 256;
    settings.birth_rate = 24.0;
    settings.seed = 4242;
    settings.particle_lifetime_seconds = 2.0;
    settings.velocity = Vec3{0.2, 0.4, 0.0};
    settings.velocity_spread = 0.0;
    settings.emitter_size = 0.0;
    settings.gravity = Vec3{0.0, -0.5, 0.0};
    settings.linear_drag = 0.7;
    settings.color_start = Vec3{1.0, 0.8, 0.2};
    settings.color_end = Vec3{0.6, 0.1, 0.05};
    settings.particle_size = 10.0;
    settings.particle_size_end = 2.0;
    settings.opacity = 1.0;
    settings.opacity_end = 0.0;
    settings.appearance_enabled = true;

    auto made = make_emitter_force_appearance_output_graph(settings, emitter, force, appearance, output,
                                                           emitter_to_force, force_to_appearance, appearance_to_output);
    CHECK(made.has_value());
    if (!made.has_value()) return;
    const Graph graph = made.take_value();
    CHECK(validate_graph(graph, particle_node_registry()).ok());

    // A four-stage graph survives the bounded codec byte for byte.
    const auto serialized = serialize_graph(graph, particle_node_registry());
    CHECK(serialized.has_value());
    if (serialized.has_value()) {
        const auto decoded = deserialize_graph(serialized.value(), particle_node_registry());
        CHECK(decoded.has_value());
        if (decoded.has_value()) {
            const auto again = serialize_graph(decoded.value(), particle_node_registry());
            CHECK(again.has_value() && again.value() == serialized.value());
        }
    }

    const RationalTime time{3, 2}; // 1.5 s
    const auto evaluated = evaluate_particle_graph(graph, time, never);
    CHECK(evaluated.has_value());
    if (evaluated.has_value()) {
        // Stage order follows dependencies, not UUIDs.
        CHECK(evaluated.value().evaluated_nodes == std::vector<NodeId>({emitter, force, appearance, output}));
        const double k = settings.linear_drag;
        bool saw_birth = false;
        for (const auto& particle : evaluated.value().particles) {
            const double age = particle.age_seconds;
            const double lifetime = particle.lifetime_seconds;
            CHECK(age >= 0.0 && age < lifetime);
            if (age <= 0.0) { saw_birth = true; continue; }
            // Closed form for dv/dt = gravity - k*v from a point emitter at the origin.
            const double damping = std::exp(-k * age);
            const double velocity_factor = (1.0 - damping) / k;
            const double expected_y = settings.velocity.y * velocity_factor +
                                      settings.gravity.y * (age - velocity_factor) / k;
            const double expected_x = settings.velocity.x * velocity_factor;
            const double fraction = age / lifetime;
            CHECK(std::abs(particle.position.y - expected_y) < 1e-9);
            CHECK(std::abs(particle.position.x - expected_x) < 1e-9);
            CHECK(std::abs(particle.size_pixels - (10.0 + (2.0 - 10.0) * fraction)) < 1e-9);
            CHECK(std::abs(particle.opacity - (1.0 - fraction)) < 1e-9);
            CHECK(std::abs(particle.color.x - (1.0 + (0.6 - 1.0) * fraction)) < 1e-9);
        }
        CHECK(saw_birth); // the slot born exactly at the requested time is visible
    }

    // Graph and flat paths must agree pixel for pixel when both carry the same
    // curves, so wiring the chain in cannot silently change the default look.
    Scene scene;
    scene.settings = settings;
    scene.format = PixelFormat::rgba32f;
    auto flat = build_request(scene);
    flat.frame.time = time;
    auto request = flat;
    request.graph = std::make_shared<const Graph>(graph);
    request.settings.value.particle_count = 0; // the graph must take precedence
    const auto expected = renderer.render(flat, never);
    const auto actual = renderer.render(request, never);
    CHECK(expected.has_value() && actual.has_value());
    if (expected.has_value() && actual.has_value()) CHECK(expected.value().pixels == actual.value().pixels);

    // The appearance stage must reach the pixels: a red end color differs visibly.
    Graph red = graph;
    for (auto& node : red.nodes) {
        if (node.type_key != kAppearanceNode) continue;
        for (auto& parameter : node.parameters) {
            if (parameter.key == kColorEnd) parameter.value = Vec3{1.0, 0.0, 0.0};
        }
    }
    auto red_request = flat;
    red_request.graph = std::make_shared<const Graph>(red);
    const auto red_pixels = renderer.render(red_request, never);
    CHECK(red_pixels.has_value());
    if (red_pixels.has_value() && actual.has_value()) CHECK(red_pixels.value().pixels != actual.value().pixels);

    // Stage order is enforced, not assumed: appearance before force is rejected.
    Graph swapped = graph;
    swapped.edges = {
        GraphEdge{emitter_to_force, emitter, kEmitterParticles, appearance, kAppearanceParticlesIn},
        GraphEdge{force_to_appearance, appearance, kAppearanceParticlesOut, force, kForceParticlesIn},
        GraphEdge{appearance_to_output, force, kForceParticlesOut, output, kOutputParticles},
    };
    CHECK(validate_graph(swapped, particle_node_registry()).ok());
    const auto reversed = evaluate_particle_graph(swapped, time, never);
    CHECK(!reversed.has_value());
    if (!reversed.has_value()) CHECK(reversed.error().code == ErrorCode::invalid_request);

    // Two chained appearance stages are structurally legal but rejected at runtime.
    Graph chained = graph;
    const NodeId second_appearance{test_uuid(31)};
    const EdgeId second_edge{test_uuid(32)};
    chained.nodes.push_back(GraphNode{second_appearance, kAppearanceNode, 1, {
        {kColorStart, Vec3{1.0, 1.0, 1.0}}, {kColorEnd, Vec3{1.0, 1.0, 1.0}},
        {kSizeStart, 4.0}, {kSizeEnd, 4.0}, {kOpacityStart, 1.0}, {kOpacityEnd, 1.0}}});
    chained.edges = {
        GraphEdge{emitter_to_force, emitter, kEmitterParticles, force, kForceParticlesIn},
        GraphEdge{force_to_appearance, force, kForceParticlesOut, appearance, kAppearanceParticlesIn},
        GraphEdge{second_edge, appearance, kAppearanceParticlesOut, second_appearance, kAppearanceParticlesIn},
        GraphEdge{appearance_to_output, second_appearance, kAppearanceParticlesOut, output, kOutputParticles},
    };
    CHECK(validate_graph(chained, particle_node_registry()).ok());
    const auto two_appearances = evaluate_particle_graph(chained, time, never);
    CHECK(!two_appearances.has_value());
    if (!two_appearances.has_value()) CHECK(two_appearances.error().code == ErrorCode::invalid_request);

    // Node values are range checked before they can reach the simulation.
    Graph huge_gravity = graph;
    for (auto& node : huge_gravity.nodes) {
        if (node.type_key != kForceNode) continue;
        for (auto& parameter : node.parameters) {
            if (parameter.key == kGravity) parameter.value = Vec3{0.0, kMaxGravityMagnitude * 10.0, 0.0};
        }
    }
    const auto out_of_range = evaluate_particle_graph(huge_gravity, time, never);
    CHECK(!out_of_range.has_value());
    if (!out_of_range.has_value()) CHECK(out_of_range.error().code == ErrorCode::invalid_request);

    // A tiny drag exercises the series branch and must stay near the no-drag limit.
    Settings tiny = settings;
    tiny.linear_drag = 1e-5;
    const auto tiny_graph = make_emitter_force_appearance_output_graph(
        tiny, emitter, force, appearance, output, emitter_to_force, force_to_appearance, appearance_to_output);
    CHECK(tiny_graph.has_value());
    if (tiny_graph.has_value()) {
        const auto tiny_result = evaluate_particle_graph(tiny_graph.value(), time, never);
        CHECK(tiny_result.has_value());
        if (tiny_result.has_value()) {
            for (const auto& particle : tiny_result.value().particles) {
                const double age = particle.age_seconds;
                const double free_fall = tiny.velocity.y * age + tiny.gravity.y * age * age * 0.5;
                CHECK(std::isfinite(particle.position.y));
                CHECK(std::abs(particle.position.y - free_fall) < 1e-4);
            }
        }
    }

    // Identity collisions and out-of-range settings fail at construction time.
    CHECK(!make_emitter_force_appearance_output_graph(settings, emitter, emitter, appearance, output,
                                                      emitter_to_force, force_to_appearance, appearance_to_output).has_value());
    Settings invalid;
    invalid.opacity = 1.5;
    CHECK(!make_emitter_force_appearance_output_graph(invalid, emitter, force, appearance, output,
                                                      emitter_to_force, force_to_appearance, appearance_to_output).has_value());
}

void test_particle_branches_and_ordered_buffer() {
    using namespace graph_keys;
    const NeverCancelled never;
    const NodeId emitter{test_uuid(9)};
    const NodeId particle_a{test_uuid(5)};
    const NodeId particle_b{test_uuid(6)};
    const NodeId output{test_uuid(1)};
    const NodeId appearance{test_uuid(7)};
    const NodeId bypass_force{test_uuid(30)};
    const NodeId bypass_appearance{test_uuid(31)};

    Settings settings;
    settings.particle_count = 10;
    settings.birth_rate = 10.0;
    settings.particle_lifetime_seconds = 1.0;
    settings.velocity = Vec3{0.2, 0.4, 0.0};
    settings.velocity_spread = 0.0;
    settings.emitter_size = 0.0;
    settings.particle_size = 2.0;
    settings.opacity = 0.25;
    const auto validated = validate_settings(settings);

    const auto range = live_particle_slot_range(validated, 1.25);
    CHECK(range.has_value());
    if (!range.has_value()) return;
    CHECK(range.value().first_slot == 3 && range.value().count == 10);

    // Every branch writes its modulo-assigned slots into one pre-sized buffer;
    // the resulting list is already in global ID order and matches flat simulation.
    std::vector<ParticleInstance> partitioned(static_cast<std::size_t>(range.value().count));
    std::size_t assigned = 0;
    for (std::uint32_t branch = 0; branch < 2; ++branch) {
        const auto written = simulate_particles_partition_into(validated, 1.25, 2, branch, partitioned, never);
        CHECK(written.has_value());
        if (written.has_value()) assigned += written.value();
    }
    CHECK(assigned == partitioned.size());
    const auto flat = simulate_particles(validated, 1.25, never);
    CHECK(flat.has_value() && flat.value().size() == partitioned.size());
    if (flat.has_value() && flat.value().size() == partitioned.size()) {
        for (std::size_t i = 0; i < partitioned.size(); ++i) {
            CHECK(partitioned[i].id == range.value().first_slot + i);
            CHECK(partitioned[i].id == flat.value()[i].id);
            CHECK(partitioned[i].position.x == flat.value()[i].position.x);
            CHECK(partitioned[i].position.y == flat.value()[i].position.y);
            CHECK(partitioned[i].age_seconds == flat.value()[i].age_seconds);
        }
    }
    const auto wrong_size = simulate_particles_partition_into(validated, 1.25, 2, 0,
                                                               std::span<ParticleInstance>{}, never);
    CHECK(!wrong_size.has_value() && wrong_size.error().code == ErrorCode::invalid_request);

    auto made = make_emitter_particle_output_graph(
        settings, emitter, particle_a, output, EdgeId{test_uuid(10)}, EdgeId{test_uuid(11)});
    CHECK(made.has_value());
    if (!made.has_value()) return;
    Graph graph = made.take_value();

    // A second Particle branch has its own appearance. The first branch's
    // downstream Appearance node replaces all three Particle curves.
    graph.nodes.push_back(GraphNode{particle_b, kParticleNode, 1, {
        {kColorStart, Vec3{0.0, 1.0, 0.0}}, {kColorEnd, Vec3{0.0, 0.5, 0.0}},
        {kSizeStart, 9.0}, {kSizeEnd, 19.0}, {kOpacityStart, 0.1}, {kOpacityEnd, 0.9}}});
    graph.nodes.push_back(GraphNode{appearance, kAppearanceNode, 1, {
        {kColorStart, Vec3{1.0, 0.0, 0.0}}, {kColorEnd, Vec3{0.5, 0.0, 0.0}},
        {kSizeStart, 4.0}, {kSizeEnd, 8.0}, {kOpacityStart, 0.2}, {kOpacityEnd, 0.8}}});
    graph.edges[1] = GraphEdge{EdgeId{test_uuid(11)}, particle_a, kParticleParticlesOut,
                               appearance, kAppearanceParticlesIn};
    graph.edges.push_back(GraphEdge{EdgeId{test_uuid(12)}, appearance, kAppearanceParticlesOut,
                                    output, kOutputParticles});
    graph.edges.push_back(GraphEdge{EdgeId{test_uuid(13)}, emitter, kEmitterParticles,
                                    particle_b, kParticleParticlesIn});
    graph.edges.push_back(GraphEdge{EdgeId{test_uuid(14)}, particle_b, kParticleParticlesOut,
                                    output, kOutputParticles});
    CHECK(validate_graph(graph, particle_node_registry()).ok());

    const auto evaluated = evaluate_particle_graph(graph, RationalTime{5, 4}, never);
    CHECK(evaluated.has_value());
    if (evaluated.has_value()) {
        const auto& result = evaluated.value().particles;
        CHECK(result.size() == 10);
        for (std::size_t i = 0; i < result.size(); ++i) {
            CHECK(result[i].id == 3 + i);
        }
        // NodeId order places particle_a in branch 0; even slots reach its
        // downstream override, odd slots retain particle_b's own curve.
        const auto even = std::find_if(result.begin(), result.end(), [](const auto& p) { return p.id == 4; });
        const auto odd = std::find_if(result.begin(), result.end(), [](const auto& p) { return p.id == 5; });
        CHECK(even != result.end() && odd != result.end());
        if (even != result.end()) {
            const double fraction = even->age_seconds / even->lifetime_seconds;
            CHECK(std::abs(even->size_pixels - (4.0 + 4.0 * fraction)) < 1e-12);
            CHECK(std::abs(even->opacity - (0.2 + 0.6 * fraction)) < 1e-12);
            CHECK(std::abs(even->color.x - (1.0 - 0.5 * fraction)) < 1e-12);
            CHECK(even->color.y == 0.0 && even->color.z == 0.0);
        }
        if (odd != result.end()) {
            const double fraction = odd->age_seconds / odd->lifetime_seconds;
            CHECK(std::abs(odd->size_pixels - (9.0 + 10.0 * fraction)) < 1e-12);
            CHECK(std::abs(odd->opacity - (0.1 + 0.8 * fraction)) < 1e-12);
            CHECK(odd->color.y > 0.0 && odd->color.x == 0.0 && odd->color.z == 0.0);
        }
    }

    // An active force path that bypasses Particle is rejected explicitly rather
    // than being silently omitted from the rendered graph.
    Graph force_bypass = graph;
    force_bypass.nodes.push_back(GraphNode{bypass_force, kForceNode, 1, {
        {kGravity, Vec3{0.0, -1.0, 0.0}}, {kLinearDrag, 0.0}}});
    force_bypass.edges.push_back(GraphEdge{EdgeId{test_uuid(32)}, emitter, kEmitterParticles,
                                           bypass_force, kForceParticlesIn});
    force_bypass.edges.push_back(GraphEdge{EdgeId{test_uuid(33)}, bypass_force, kForceParticlesOut,
                                           output, kOutputParticles});
    CHECK(validate_graph(force_bypass, particle_node_registry()).ok());
    const auto rejected_force_bypass = evaluate_particle_graph(force_bypass, RationalTime{5, 4}, never);
    CHECK(!rejected_force_bypass.has_value());
    if (!rejected_force_bypass.has_value()) {
        CHECK(rejected_force_bypass.error().code == ErrorCode::invalid_request);
        CHECK(std::strcmp(rejected_force_bypass.error().detail, "invalid active Particle graph connection") == 0);
    }

    Graph appearance_bypass = graph;
    appearance_bypass.nodes.push_back(GraphNode{bypass_appearance, kAppearanceNode, 1, {
        {kColorStart, Vec3{1.0, 1.0, 1.0}}, {kColorEnd, Vec3{1.0, 1.0, 1.0}},
        {kSizeStart, 1.0}, {kSizeEnd, 1.0}, {kOpacityStart, 1.0}, {kOpacityEnd, 1.0}}});
    appearance_bypass.edges.push_back(GraphEdge{EdgeId{test_uuid(34)}, emitter, kEmitterParticles,
                                                bypass_appearance, kAppearanceParticlesIn});
    appearance_bypass.edges.push_back(GraphEdge{EdgeId{test_uuid(35)}, bypass_appearance,
                                                kAppearanceParticlesOut, output, kOutputParticles});
    CHECK(validate_graph(appearance_bypass, particle_node_registry()).ok());
    const auto rejected_appearance_bypass = evaluate_particle_graph(appearance_bypass, RationalTime{5, 4}, never);
    CHECK(!rejected_appearance_bypass.has_value());
    if (!rejected_appearance_bypass.has_value()) {
        CHECK(rejected_appearance_bypass.error().code == ErrorCode::invalid_request);
        CHECK(std::strcmp(rejected_appearance_bypass.error().detail, "invalid active Particle graph connection") == 0);
    }
}

// Emission direction model (M3-04): the reference emitter's Speed plus a direction axis,
// a cone span and a whole-sphere mode. Checks that the cone is bounded, that `uniform`
// really covers the sphere, that speed jitter varies the magnitude, and that everything
// stays a pure function of (settings, seed, slot).
void test_emission_direction() {
    const NeverCancelled never;
    const auto particle_velocity = [&never](const Settings& settings, double seconds, std::uint64_t id) {
        auto validated = validate_settings(settings);
        const auto particles = simulate_particles(validated, seconds, never);
        for (const auto& particle : particles.value()) {
            if (particle.id == id) return particle.position;
        }
        return Vec3{};
    };
    const auto speed_of = [&never](const Settings& settings, double seconds) {
        auto validated = validate_settings(settings);
        const auto particles = simulate_particles(validated, seconds, never);
        std::vector<double> speeds;
        for (const auto& particle : particles.value()) {
            if (particle.age_seconds <= 0.0) continue; // position is speed * age
            const Vec3 per_second{particle.position.x / particle.age_seconds,
                                  particle.position.y / particle.age_seconds,
                                  particle.position.z / particle.age_seconds};
            speeds.push_back(std::sqrt(per_second.x * per_second.x + per_second.y * per_second.y +
                                       per_second.z * per_second.z));
        }
        return speeds;
    };

    Settings base;
    base.velocity = Vec3{};
    base.velocity_spread = 0.0;
    base.gravity = Vec3{};
    base.linear_drag = 0.0;
    base.emitter_size = 0.0;
    base.particle_count = 512;
    base.birth_rate = 240.0;
    base.particle_lifetime_seconds = 4.0;

    // Span 0 is a straight line along the axis: 0/0/0 means straight up at the given speed.
    Settings straight = base;
    straight.emission_speed = 0.5;
    straight.direction_span_degrees = 0.0;
    for (const double speed : speed_of(straight, 1.0)) {
        CHECK(std::abs(speed - 0.5) < 1e-12);
    }
    const Vec3 first = particle_velocity(straight, 1.0, 24);
    const Vec3 again = particle_velocity(straight, 1.0, 24);
    CHECK(first.y > 0.0 && std::abs(first.x) < 1e-12 && std::abs(first.z) < 1e-12);
    CHECK(again.x == first.x && again.y == first.y && again.z == first.z);

    // A 60 degree span keeps every emitted direction inside 30 degrees of the axis.
    Settings cone = base;
    cone.emission_speed = 1.0;
    cone.direction_span_degrees = 60.0;
    const double cone_limit = std::cos(30.0 * 3.14159265358979323846 / 180.0);
    auto validated_cone = validate_settings(cone);
    const auto cone_particles = simulate_particles(validated_cone, 1.0, never);
    CHECK(cone_particles.has_value());
    for (const auto& particle : cone_particles.value()) {
        if (particle.age_seconds <= 0.0) continue;
        const double length = std::sqrt(particle.position.x * particle.position.x +
                                        particle.position.y * particle.position.y +
                                        particle.position.z * particle.position.z);
        CHECK(length > 0.0);
        const double cos_angle = particle.position.y / length; // axis is +Y
        CHECK(cos_angle >= cone_limit - 1e-9);
    }

    // Uniform mode covers the whole sphere: some particles must travel downward, and the
    // mean direction stays near zero because the sampling is even over the solid angle.
    Settings sphere = base;
    sphere.emission_speed = 1.0;
    sphere.direction_mode = DirectionMode::uniform;
    auto validated_sphere = validate_settings(sphere);
    const auto sphere_particles = simulate_particles(validated_sphere, 1.0, never);
    CHECK(sphere_particles.has_value());
    double sum_y = 0.0;
    int counted = 0;
    bool saw_downward = false;
    for (const auto& particle : sphere_particles.value()) {
        if (particle.age_seconds <= 0.0) continue;
        const double length = std::sqrt(particle.position.x * particle.position.x +
                                        particle.position.y * particle.position.y +
                                        particle.position.z * particle.position.z);
        if (length <= 0.0) continue;
        sum_y += particle.position.y / length;
        saw_downward = saw_downward || particle.position.y < 0.0;
        ++counted;
    }
    CHECK(counted > 100);
    CHECK(saw_downward);
    CHECK(std::abs(sum_y / static_cast<double>(counted)) < 0.2);

    // Speed jitter varies the magnitude but never makes it negative or zero on average.
    Settings jittered = base;
    jittered.emission_speed = 1.0;
    jittered.emission_speed_random = 0.25;
    double previous = -1.0;
    bool saw_different = false;
    for (const double speed : speed_of(jittered, 1.0)) {
        CHECK(speed >= 0.0);
        if (previous >= 0.0 && std::abs(speed - previous) > 1e-9) saw_different = true;
        previous = speed;
    }
    CHECK(saw_different);

    // Angles rotate the axis: a right-handed 90 degree rotation about Z maps up (+Y) to -X.
    Settings rotated = base;
    rotated.emission_speed = 1.0;
    rotated.direction_span_degrees = 0.0;
    rotated.emission_angles_degrees = Vec3{0.0, 0.0, 90.0};
    auto validated_rotated = validate_settings(rotated);
    const auto rotated_particles = simulate_particles(validated_rotated, 1.0, never);
    CHECK(rotated_particles.has_value());
    for (const auto& particle : rotated_particles.value()) {
        if (particle.age_seconds <= 0.0) continue;
        CHECK(std::abs(particle.position.x + particle.age_seconds) < 1e-9);
        CHECK(std::abs(particle.position.y) < 1e-9);
    }
}

void test_core_plugin_api() {
    SfCoreApi api{};
    CHECK(StarfieldCore_GetApi(SF_CORE_ABI_VERSION + 1, sizeof(api), &api) == 0);
    CHECK(StarfieldCore_GetApi(SF_CORE_ABI_VERSION, sizeof(api), &api) == 1);
    CHECK(api.abi_version == SF_CORE_ABI_VERSION && api.render && api.release_render_result && api.inspect);

    const Settings settings{};
    auto made = make_emitter_output_graph(settings, NodeId{test_uuid(41)}, NodeId{test_uuid(42)},
                                          EdgeId{test_uuid(43)});
    CHECK(made.has_value());
    if (!made.has_value()) return;
    const auto encoded = serialize_graph(made.value(), particle_node_registry());
    CHECK(encoded.has_value());
    if (!encoded.has_value()) return;

    SfCoreRenderRequest request{};
    request.struct_size = sizeof(request);
    request.frame = SfCoreFrame{64, 64, 64, 64, {0, 0, 64, 64},
                                1, 1, 1, 24, 0, 0, 1, 1, 1.0};
    request.graph_bytes = encoded.value().data();
    request.graph_byte_count = encoded.value().size();
    SfCoreRenderResult result{};
    result.struct_size = sizeof(result);
    CHECK(api.render(&request, &result) == SF_CORE_OK);
    CHECK(result.status == SF_CORE_OK && result.opaque_handle != nullptr);
    CHECK(result.pixels != nullptr && result.pixel_byte_count > 0);

    RenderRequest direct_request;
    direct_request.frame = FrameSpec{64, 64, 64, 64, {0, 0, 64, 64},
                                     {1, 1}, {1, 24}, PixelFormat::rgba8,
                                     ColorSpace::ae_working_space, AlphaMode::premultiplied, 1.0, Quality::full};
    direct_request.graph = std::make_shared<const Graph>(made.value());
    const NeverCancelled never;
    const auto direct = CpuParticleRenderer{}.render(direct_request, never);
    CHECK(direct.has_value());
    if (direct.has_value() && result.pixels) {
        CHECK(result.pixel_byte_count == direct.value().pixels.size());
        CHECK(std::memcmp(result.pixels, direct.value().pixels.data(),
                          direct.value().pixels.size()) == 0);
    }

    SfCoreInspectRequest inspect_request{sizeof(SfCoreInspectRequest), encoded.value().data(),
                                         encoded.value().size(), 1, 1};
    SfCoreInspectResult inspection{};
    inspection.struct_size = sizeof(inspection);
    CHECK(api.inspect(&inspect_request, &inspection) == SF_CORE_OK);
    CHECK(inspection.node_count == 2 && inspection.edge_count == 1 &&
          inspection.live_particle_count > 0);

    api.release_render_result(&result);
    CHECK(result.opaque_handle == nullptr && result.pixels == nullptr);
    request.graph_byte_count = 0;
    result.struct_size = sizeof(result);
    CHECK(api.render(&request, &result) == SF_CORE_INVALID_REQUEST);
    CHECK(result.detail[0] != '\0' && result.opaque_handle == nullptr);
    api.release_render_result(&result);
}

} // namespace

int main() {
    test_core_plugin_api();
    test_emission_direction();
    test_particle_branches_and_ordered_buffer();
    test_force_and_appearance();
    test_graph_evaluation();
    test_rational_time();
    test_graph_contract();
    test_sequence_codec();
    test_layer_point_conversion();
    test_settings_validation();
    test_random_streams();
    test_simulation_emitter_origin();
    test_emitter_shapes_and_spread();
    test_simulation_boundaries();
    test_renderer_determinism_and_geometry();
    test_renderer_region_of_interest();
    test_renderer_downsampled_frame_mapping();
    test_renderer_transparent_particle_output();
    test_renderer_formats_and_limits();

    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
