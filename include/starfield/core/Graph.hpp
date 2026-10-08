#pragma once

#include "starfield/core/Settings.hpp"

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace starfield::core {

// Graph identities are UUIDs with distinct C++ types. They are never derived
// from AE parameter IDs, array positions, or editor widget IDs.
struct Uuid128 {
    std::array<std::uint8_t, 16> bytes{};
    [[nodiscard]] constexpr bool is_zero() const noexcept {
        for (const auto byte : bytes) {
            if (byte != 0) return false;
        }
        return true;
    }
    auto operator<=>(const Uuid128&) const = default;
};

struct NodeId {
    Uuid128 value{};
    auto operator<=>(const NodeId&) const = default;
};

struct EdgeId {
    Uuid128 value{};
    auto operator<=>(const EdgeId&) const = default;
};

struct PortKey {
    std::uint64_t value{};
    auto operator<=>(const PortKey&) const = default;
};

struct ParameterKey {
    std::uint64_t value{};
    auto operator<=>(const ParameterKey&) const = default;
};

enum class PortDirection : std::uint8_t { input, output };

// Value kinds are deliberately independent of AE parameter types. Port type
// keys are extensible reverse-DNS strings so later feature families can add
// particle, texture, mesh, volume, and event streams without renumbering enums.
enum class ParameterKind : std::uint8_t {
    boolean,
    int32,
    uint32,
    float64,
    vector3_float64,
    utf8,
    opaque_bytes,
};

using OpaqueBytes = std::vector<std::byte>;
using ParameterValue = std::variant<bool, std::int32_t, std::uint32_t, double, Vec3, std::string, OpaqueBytes>;

struct NodeParameter {
    ParameterKey key{};
    ParameterValue value{};
};

struct GraphNode {
    NodeId id{};
    std::string type_key;
    std::uint16_t schema_version{1};
    std::vector<NodeParameter> parameters;
};

struct GraphEdge {
    EdgeId id{};
    NodeId source_node{};
    PortKey source_port{};
    NodeId destination_node{};
    PortKey destination_port{};
};

struct Graph {
    std::vector<GraphNode> nodes;
    std::vector<GraphEdge> edges;
    // Optional, length-delimited sequence records are opaque to graph
    // evaluation but must survive edits and native deserialize/serialize cycles.
    std::vector<OpaqueBytes> optional_records;
};

struct PortDescriptor {
    PortKey key{};
    PortDirection direction{PortDirection::input};
    std::string type_key;
    bool required{false};
    // Zero means unbounded. Inputs default to one connection; outputs default
    // to fan-out. Cardinality is part of the node schema, not the edge order.
    std::uint32_t max_connections{1};
};

struct ParameterDescriptor {
    ParameterKey key{};
    ParameterKind kind{ParameterKind::float64};
    bool required{false};
};

struct NodeTypeDescriptor {
    std::string type_key;
    std::uint16_t schema_version{1};
    std::vector<PortDescriptor> ports;
    std::vector<ParameterDescriptor> parameters;
    // Edges entering these inputs are state updates and do not create a
    // same-frame dependency. This is the explicit boundary for future feedback
    // nodes; ordinary cycles are rejected.
    std::vector<PortKey> cycle_breaking_inputs;
};

struct NodeRegistry {
    std::vector<NodeTypeDescriptor> types;
};

inline constexpr std::size_t kMaxGraphNodes = 4096;
inline constexpr std::size_t kMaxGraphEdges = 16384;
inline constexpr std::size_t kMaxNodeParameters = 512;
inline constexpr std::size_t kMaxNodeTypes = 256;
inline constexpr std::size_t kMaxNodePorts = 128;
inline constexpr std::size_t kMaxGraphTypeKeyBytes = 128;
inline constexpr std::uint64_t kMaxSavedGraphPayloadBytes = 64ull * 1024ull * 1024ull;
// A pre-render-only evaluated particle snapshot may carry the full 2M cap.
inline constexpr std::uint64_t kMaxGraphPayloadBytes = 512ull * 1024ull * 1024ull;

enum class GraphErrorCode : std::uint8_t {
    none,
    graph_too_large,
    invalid_registry,
    invalid_identifier,
    duplicate_node_id,
    duplicate_edge_id,
    unknown_node_type,
    unsupported_node_version,
    duplicate_parameter_key,
    unknown_parameter,
    missing_required_parameter,
    parameter_type_mismatch,
    invalid_parameter_value,
    unknown_source_node,
    unknown_destination_node,
    unknown_source_port,
    unknown_destination_port,
    port_direction_mismatch,
    port_type_mismatch,
    duplicate_input_connection,
    too_many_connections,
    missing_required_input,
    cycle_detected,
    allocation_failed,
    internal_failure,
};

struct GraphError {
    GraphErrorCode code{GraphErrorCode::none};
    NodeId node_id{};
    EdgeId edge_id{};
    PortKey port_key{};
    ParameterKey parameter_key{};
    const char* detail{"valid graph"};
};

struct GraphValidationResult {
    GraphError error{};
    [[nodiscard]] constexpr bool ok() const noexcept { return error.code == GraphErrorCode::none; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return ok(); }
};

[[nodiscard]] const char* describe(GraphErrorCode code) noexcept;

// The registry defines the currently supported node schemas. Validation is
// independent of vector order and bounded by the same graph limits as the
// sequence format. All failures are typed; invalid user graphs are never
// traversed by the renderer.
[[nodiscard]] GraphValidationResult validate_graph(const Graph& graph, const NodeRegistry& registry) noexcept;

// Stable built-in schemas used by the emitter -> Particle -> force
// -> Output graph. Output carries the renderer-wide particle cap. Keys in this namespace are
// graph ParameterKeys and are not AE parameter IDs.
namespace graph_keys {
inline constexpr const char* kParticleStream = "org.starfieldfx.types.particle-stream";
inline constexpr const char* kEmitterNode = "org.starfieldfx.nodes.emitter";
inline constexpr const char* kParticleNode = "org.starfieldfx.nodes.particle";
inline constexpr const char* kForceNode = "org.starfieldfx.nodes.force";
inline constexpr const char* kTransformNode = "org.starfieldfx.nodes.transform";
inline constexpr const char* kOutputNode = "org.starfieldfx.nodes.output";
inline constexpr PortKey kEmitterParticles{1};
inline constexpr PortKey kEmitterParents{2};
inline constexpr PortKey kParticleParticlesIn{1};
inline constexpr PortKey kParticleParticlesOut{2};
inline constexpr PortKey kForceParticlesIn{1};
inline constexpr PortKey kForceParticlesOut{2};
inline constexpr PortKey kTransformParticlesIn{1},kTransformParticlesOut{2};
inline constexpr PortKey kOutputParticles{1};
inline constexpr ParameterKey kParticleCount{1};
inline constexpr ParameterKey kBirthRate{2};
inline constexpr ParameterKey kSeed{3};
inline constexpr ParameterKey kEmitterShape{5};
inline constexpr ParameterKey kEmitterOrigin{6};
inline constexpr ParameterKey kVelocity{7};
inline constexpr ParameterKey kParticleSize{8};
inline constexpr ParameterKey kOpacity{9};
inline constexpr ParameterKey kEmitterSize{10};
inline constexpr ParameterKey kVelocitySpread{11};
// Emission direction model (reference-aligned, added with the M3-04 slice). Appended,
// never renumbered: stored graphs keep their existing keys.
inline constexpr ParameterKey kEmissionSpeed{12};
inline constexpr ParameterKey kEmissionSpeedRandom{13};
inline constexpr ParameterKey kEmissionAngleX{14};
inline constexpr ParameterKey kEmissionAngleY{15};
inline constexpr ParameterKey kEmissionAngleZ{16};
inline constexpr ParameterKey kDirectionMode{17};
inline constexpr ParameterKey kDirectionSpan{18};
inline constexpr ParameterKey kEmitterSizeX{19};
inline constexpr ParameterKey kEmitterSizeY{20};
inline constexpr ParameterKey kEmitterSizeZ{21};
// Native authoring preserves this percentage even at zero base Speed. When
// present it defines the amplitude; Settings/direct graph construction retain
// their independent world-unit jitter input (key 13).
inline constexpr ParameterKey kEmissionSpeedRandomPercent{22};
inline constexpr ParameterKey kEmittingMode{23};
inline constexpr ParameterKey kAuxiliarySource{31};
inline constexpr ParameterKey kEmitChance{24};
inline constexpr ParameterKey kEmitLifeStart{25};
inline constexpr ParameterKey kEmitLifeEnd{26};
inline constexpr ParameterKey kInheritVelocity{27};
inline constexpr ParameterKey kInheritSize{28};
inline constexpr ParameterKey kInheritOpacity{29};
inline constexpr ParameterKey kInheritColor{30};
// Parameter keys are scoped to their node type; Force and Particle nodes may
// therefore use compact local key ranges without aliasing emitter parameters.
inline constexpr ParameterKey kGravity{1};
inline constexpr ParameterKey kLinearDrag{2};
inline constexpr ParameterKey kGravityRandom{3};
inline constexpr ParameterKey kWind{4};
inline constexpr ParameterKey kSpin{5};
inline constexpr ParameterKey kSpinFrequency{6};
inline constexpr ParameterKey kSpinResist{7};
inline constexpr ParameterKey kSpinDelay{8};
inline constexpr ParameterKey kWindSpinCurve{9};
inline constexpr ParameterKey kTransformAnchor{1},kTransformPosition{2},kTransformRotation{3};
inline constexpr ParameterKey kTransformSystemScale{4},kTransformParticleScale{5},kTransformParticleOpacity{6};
inline constexpr ParameterKey kTransformInheritedMatrix{7};
inline constexpr ParameterKey kTransformInheritLayer{8}; // project-local layer ID; 0=None
// Output-scoped globals; optional values default to disabled / 100%.
inline constexpr ParameterKey kTimeRemapEnabled{2};
inline constexpr ParameterKey kTimeRemapSeconds{3};
inline constexpr ParameterKey kPreviewEnabled{4};
inline constexpr ParameterKey kPreviewChance{5};
inline constexpr ParameterKey kColorStart{1};
inline constexpr ParameterKey kColorEnd{2};
inline constexpr ParameterKey kSizeStart{3};
inline constexpr ParameterKey kSizeEnd{4};
inline constexpr ParameterKey kOpacityStart{5};
inline constexpr ParameterKey kOpacityEnd{6};
inline constexpr ParameterKey kSizeOverLifeCurve{7};
inline constexpr ParameterKey kOpacityOverLifeCurve{8};
inline constexpr ParameterKey kSizeRandom{9};
inline constexpr ParameterKey kOpacityRandom{10};
// Particle branch lifetime is distinct from the retained legacy emitter key.
inline constexpr ParameterKey kParticleLifetimeSeconds{11};
inline constexpr ParameterKey kParticleColorMode{12};
inline constexpr ParameterKey kColorGradient{13};
inline constexpr ParameterKey kLifeRandom{14}, kParticleShape{15}, kSizeY{16}, kOrientTo{17};
inline constexpr ParameterKey kParticleAngles{18}, kAngleRandom{19}, kRotationSpeed{20}, kRotationSpeedRandom{21};
inline constexpr ParameterKey kLimitTo2D{22}, kParticleFeather{23}, kUpAxis{24};
inline constexpr ParameterKey kRandomLimit{25}, kLimitAngle{26}, kRotationOverLife{27}, kAnchorX{28}, kAnchorY{29};
inline constexpr ParameterKey kParticleTransferMode{30};
inline constexpr ParameterKey kTextureFront{31}, kTextureBack{32}, kTextureTimeMode{33}, kTextureColorUse{34};
inline constexpr ParameterKey kTextureUseRatio{35}, kTextureIgnorePerspective{36};
inline constexpr ParameterKey kEmitterOrient{32}; // Emitter direction, distinct from shape angles.
inline constexpr ParameterKey kAcceleration{6}; // Output: 0 GPU / 1 CPU
inline constexpr ParameterKey kTimeSamplingHz{7};
// Optional Output metadata; the adapter renders the shutter exposure while
// Core ABI4 receives one immutable frame at a time. Popup values are zero based.
inline constexpr ParameterKey kMotionBlur{8}, kShutterAngle{9}, kShutterPhase{10};
inline constexpr ParameterKey kMotionBlurType{11}, kMotionBlurLevels{12}, kLinearAccuracy{13};
inline constexpr ParameterKey kOpacityBoost{14}, kMotionBlurDisregard{15};

} // namespace graph_keys

[[nodiscard]] NodeRegistry make_particle_node_registry();
[[nodiscard]] const NodeRegistry& particle_node_registry();

} // namespace starfield::core
