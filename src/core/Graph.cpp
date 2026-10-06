#include "starfield/core/Graph.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <new>
#include <optional>
#include <string_view>
#include <utility>

namespace starfield::core {
namespace {

constexpr std::size_t kMaxNodeKeyBytes = kMaxGraphTypeKeyBytes;

GraphValidationResult failure(GraphErrorCode code, const char* detail, NodeId node = {}, EdgeId edge = {},
                               PortKey port = {}, ParameterKey parameter = {}) noexcept {
    return GraphValidationResult{GraphError{code, node, edge, port, parameter, detail}};
}

bool valid_dns_key(std::string_view value) noexcept {
    if (value.size() < 3 || value.size() > kMaxNodeKeyBytes || value.front() == '.' || value.back() == '.') {
        return false;
    }
    bool has_dot = false;
    bool component_start = true;
    char previous = '\0';
    for (const char character : value) {
        if (character == '.') {
            if (component_start || previous == '-') return false;
            has_dot = true;
            component_start = true;
            previous = character;
            continue;
        }
        if (!((character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') ||
              character == '-')) {
            return false;
        }
        if (component_start && character == '-') return false;
        component_start = false;
        previous = character;
    }
    return has_dot && previous != '-';
}

bool valid_utf8_without_nul(std::string_view value) noexcept {
    std::size_t index = 0;
    while (index < value.size()) {
        const auto first = static_cast<unsigned char>(value[index]);
        if (first == 0) return false;
        if (first <= 0x7f) {
            ++index;
            continue;
        }

        std::uint32_t codepoint = 0;
        std::size_t continuation_count = 0;
        if (first >= 0xc2 && first <= 0xdf) {
            codepoint = first & 0x1fu;
            continuation_count = 1;
        } else if (first >= 0xe0 && first <= 0xef) {
            codepoint = first & 0x0fu;
            continuation_count = 2;
        } else if (first >= 0xf0 && first <= 0xf4) {
            codepoint = first & 0x07u;
            continuation_count = 3;
        } else {
            return false;
        }
        if (continuation_count > value.size() - index - 1) return false;
        for (std::size_t offset = 1; offset <= continuation_count; ++offset) {
            const auto next = static_cast<unsigned char>(value[index + offset]);
            if ((next & 0xc0u) != 0x80u) return false;
            codepoint = (codepoint << 6u) | (next & 0x3fu);
        }
        if ((continuation_count == 1 && codepoint < 0x80u) ||
            (continuation_count == 2 && codepoint < 0x800u) ||
            (continuation_count == 3 && codepoint < 0x10000u) || codepoint > 0x10ffffu ||
            (codepoint >= 0xd800u && codepoint <= 0xdfffu)) {
            return false;
        }
        index += continuation_count + 1;
    }
    return true;
}

ParameterKind kind_of(const ParameterValue& value) noexcept {
    return static_cast<ParameterKind>(value.index());
}

bool finite_value(const ParameterValue& value) noexcept {
    if (const auto* scalar = std::get_if<double>(&value)) return std::isfinite(*scalar);
    if (const auto* vector = std::get_if<Vec3>(&value)) {
        return std::isfinite(vector->x) && std::isfinite(vector->y) && std::isfinite(vector->z);
    }
    if (const auto* text = std::get_if<std::string>(&value)) return valid_utf8_without_nul(*text);
    return true;
}

std::uint64_t value_bytes(const ParameterValue& value) noexcept {
    if (const auto* text = std::get_if<std::string>(&value)) return static_cast<std::uint64_t>(text->size());
    if (const auto* bytes = std::get_if<OpaqueBytes>(&value)) return static_cast<std::uint64_t>(bytes->size());
    switch (kind_of(value)) {
        case ParameterKind::boolean:
            return 1;
        case ParameterKind::int32:
        case ParameterKind::uint32:
            return 4;
        case ParameterKind::float64:
            return 8;
        case ParameterKind::vector3_float64:
            return 24;
        case ParameterKind::utf8:
        case ParameterKind::opaque_bytes:
            break;
    }
    return 0;
}

template <typename T>
bool add_bounded(std::uint64_t amount, T& total) noexcept {
    const auto current = static_cast<std::uint64_t>(total);
    if (amount > kMaxGraphPayloadBytes - current) return false;
    total = static_cast<T>(current + amount);
    return true;
}

struct NodeRef {
    const GraphNode* node{};
    const NodeTypeDescriptor* descriptor{};
};

struct ConnectionRef {
    NodeId node{};
    PortKey port{};
    std::uint32_t maximum{};
};

const NodeTypeDescriptor* find_type(const std::vector<const NodeTypeDescriptor*>& sorted_types,
                                    std::string_view key) noexcept {
    const auto found = std::lower_bound(sorted_types.begin(), sorted_types.end(), key,
                                        [](const NodeTypeDescriptor* type, std::string_view candidate) {
                                            return std::string_view(type->type_key) < candidate;
                                        });
    return found != sorted_types.end() && (*found)->type_key == key ? *found : nullptr;
}

std::optional<std::size_t> find_node_index(const std::vector<NodeRef>& sorted_nodes, NodeId id) noexcept {
    const auto found = std::lower_bound(sorted_nodes.begin(), sorted_nodes.end(), id,
                                        [](const NodeRef& node, NodeId candidate) {
                                            return node.node->id < candidate;
                                        });
    if (found == sorted_nodes.end() || found->node->id != id) return std::nullopt;
    return static_cast<std::size_t>(found - sorted_nodes.begin());
}

const PortDescriptor* find_port(const NodeTypeDescriptor& type, PortKey key) noexcept {
    for (const auto& port : type.ports) {
        if (port.key == key) return &port;
    }
    return nullptr;
}

const ParameterDescriptor* find_parameter(const NodeTypeDescriptor& type, ParameterKey key) noexcept {
    for (const auto& parameter : type.parameters) {
        if (parameter.key == key) return &parameter;
    }
    return nullptr;
}

bool is_cycle_breaking_input(const NodeTypeDescriptor& type, PortKey key) noexcept {
    return std::find(type.cycle_breaking_inputs.begin(), type.cycle_breaking_inputs.end(), key) !=
           type.cycle_breaking_inputs.end();
}

GraphValidationResult validate_registry(const NodeRegistry& registry,
                                        std::vector<const NodeTypeDescriptor*>& sorted_types) {
    if (registry.types.size() > kMaxNodeTypes) {
        return failure(GraphErrorCode::invalid_registry, "node registry exceeds its bounded type count");
    }

    sorted_types.reserve(registry.types.size());
    for (const auto& type : registry.types) sorted_types.push_back(&type);
    std::sort(sorted_types.begin(), sorted_types.end(), [](const auto* left, const auto* right) {
        return left->type_key < right->type_key;
    });

    for (std::size_t type_index = 0; type_index < sorted_types.size(); ++type_index) {
        const NodeTypeDescriptor& type = *sorted_types[type_index];
        if (!valid_dns_key(type.type_key) || type.schema_version == 0 || type.ports.size() > kMaxNodePorts ||
            type.parameters.size() > kMaxNodeParameters || type.cycle_breaking_inputs.size() > type.ports.size()) {
            return failure(GraphErrorCode::invalid_registry, "node type descriptor is malformed");
        }
        if (type_index > 0 && sorted_types[type_index - 1]->type_key == type.type_key) {
            return failure(GraphErrorCode::invalid_registry, "node registry contains a duplicate type key");
        }

        std::array<const PortDescriptor*, kMaxNodePorts> ports{};
        for (std::size_t i = 0; i < type.ports.size(); ++i) ports[i] = &type.ports[i];
        std::sort(ports.begin(), ports.begin() + static_cast<std::ptrdiff_t>(type.ports.size()),
                  [](const auto* left, const auto* right) { return left->key < right->key; });
        for (std::size_t i = 0; i < type.ports.size(); ++i) {
            const PortDescriptor& port = *ports[i];
            if (port.key.value == 0 || !valid_dns_key(port.type_key) ||
                (port.direction != PortDirection::input && port.direction != PortDirection::output) ||
                (port.direction == PortDirection::output && port.required)) {
                return failure(GraphErrorCode::invalid_registry, "port descriptor is malformed", {}, {}, port.key);
            }
            if (i > 0 && ports[i - 1]->key == port.key) {
                return failure(GraphErrorCode::invalid_registry, "node type contains a duplicate port key", {}, {},
                               port.key);
            }
        }

        for (const PortKey key : type.cycle_breaking_inputs) {
            const PortDescriptor* port = find_port(type, key);
            if (port == nullptr || port->direction != PortDirection::input) {
                return failure(GraphErrorCode::invalid_registry,
                               "cycle-breaking key must identify an input port", {}, {}, key);
            }
        }

        std::array<const ParameterDescriptor*, kMaxNodeParameters> parameters{};
        for (std::size_t i = 0; i < type.parameters.size(); ++i) parameters[i] = &type.parameters[i];
        std::sort(parameters.begin(), parameters.begin() + static_cast<std::ptrdiff_t>(type.parameters.size()),
                  [](const auto* left, const auto* right) { return left->key < right->key; });
        for (std::size_t i = 0; i < type.parameters.size(); ++i) {
            const ParameterDescriptor& parameter = *parameters[i];
            if (parameter.key.value == 0 || static_cast<unsigned>(parameter.kind) >
                                                static_cast<unsigned>(ParameterKind::opaque_bytes)) {
                return failure(GraphErrorCode::invalid_registry, "parameter descriptor is malformed", {}, {}, {},
                               parameter.key);
            }
            if (i > 0 && parameters[i - 1]->key == parameter.key) {
                return failure(GraphErrorCode::invalid_registry, "node type contains a duplicate parameter key", {},
                               {}, {}, parameter.key);
            }
        }
    }
    return {};
}

bool before_connection(const ConnectionRef& left, const ConnectionRef& right) noexcept {
    if (left.node != right.node) return left.node < right.node;
    return left.port < right.port;
}

} // namespace

const char* describe(GraphErrorCode code) noexcept {
    switch (code) {
        case GraphErrorCode::none: return "valid graph";
        case GraphErrorCode::graph_too_large: return "graph exceeds a documented size bound";
        case GraphErrorCode::invalid_registry: return "node registry is invalid";
        case GraphErrorCode::invalid_identifier: return "graph contains an invalid stable identifier";
        case GraphErrorCode::duplicate_node_id: return "node IDs must be unique";
        case GraphErrorCode::duplicate_edge_id: return "edge IDs must be unique";
        case GraphErrorCode::unknown_node_type: return "graph contains an unsupported node type";
        case GraphErrorCode::unsupported_node_version: return "node schema version is unsupported";
        case GraphErrorCode::duplicate_parameter_key: return "node parameter keys must be unique";
        case GraphErrorCode::unknown_parameter: return "node parameter key is not in its schema";
        case GraphErrorCode::missing_required_parameter: return "node is missing a required parameter";
        case GraphErrorCode::parameter_type_mismatch: return "node parameter value has the wrong type";
        case GraphErrorCode::invalid_parameter_value: return "node parameter value is invalid";
        case GraphErrorCode::unknown_source_node: return "edge source node does not exist";
        case GraphErrorCode::unknown_destination_node: return "edge destination node does not exist";
        case GraphErrorCode::unknown_source_port: return "edge source port does not exist";
        case GraphErrorCode::unknown_destination_port: return "edge destination port does not exist";
        case GraphErrorCode::port_direction_mismatch: return "edge connects ports in the wrong direction";
        case GraphErrorCode::port_type_mismatch: return "edge connects incompatible port types";
        case GraphErrorCode::duplicate_input_connection: return "input port cannot accept these simultaneous connections";
        case GraphErrorCode::too_many_connections: return "port connection limit was exceeded";
        case GraphErrorCode::missing_required_input: return "required input port is not connected";
        case GraphErrorCode::cycle_detected: return "graph contains a same-frame cycle";
        case GraphErrorCode::allocation_failed: return "graph validation allocation failed";
        case GraphErrorCode::internal_failure: return "graph validation failed unexpectedly";
    }
    return "unknown graph validation error";
}

GraphValidationResult validate_graph(const Graph& graph, const NodeRegistry& registry) noexcept {
    try {
        if (graph.nodes.size() > kMaxGraphNodes || graph.edges.size() > kMaxGraphEdges) {
            return failure(GraphErrorCode::graph_too_large, "graph exceeds the node or edge limit");
        }

        std::vector<const NodeTypeDescriptor*> sorted_types;
        const auto registry_result = validate_registry(registry, sorted_types);
        if (!registry_result) return registry_result;

        std::vector<NodeRef> nodes;
        nodes.reserve(graph.nodes.size());
        std::uint64_t estimated_bytes = 32;
        std::uint64_t total_parameters = 0;
        for (const auto& node : graph.nodes) {
            if (node.id.value.is_zero() || !valid_dns_key(node.type_key) || node.schema_version == 0) {
                return failure(GraphErrorCode::invalid_identifier, "node identity or type key is invalid", node.id);
            }
            if (node.parameters.size() > kMaxNodeParameters ||
                total_parameters > (1ull << 20) - node.parameters.size()) {
                return failure(GraphErrorCode::graph_too_large, "graph exceeds the parameter count limit", node.id);
            }
            total_parameters += node.parameters.size();
            if (!add_bounded<std::uint64_t>(32 + node.type_key.size(), estimated_bytes)) {
                return failure(GraphErrorCode::graph_too_large, "graph exceeds the serialized payload limit", node.id);
            }

            const NodeTypeDescriptor* type = find_type(sorted_types, node.type_key);
            if (type == nullptr) {
                return failure(GraphErrorCode::unknown_node_type, describe(GraphErrorCode::unknown_node_type), node.id);
            }
            if (node.schema_version != type->schema_version) {
                return failure(GraphErrorCode::unsupported_node_version,
                               describe(GraphErrorCode::unsupported_node_version), node.id);
            }
            nodes.push_back(NodeRef{&node, type});
        }

        std::sort(nodes.begin(), nodes.end(), [](const NodeRef& left, const NodeRef& right) {
            return left.node->id < right.node->id;
        });
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (i > 0 && nodes[i - 1].node->id == nodes[i].node->id) {
                return failure(GraphErrorCode::duplicate_node_id, describe(GraphErrorCode::duplicate_node_id),
                               nodes[i].node->id);
            }
        }

        // Validate parameter identity and schema for each node using a bounded
        // stack array, so malformed parameter counts cannot trigger allocation.
        for (const NodeRef& node_ref : nodes) {
            const GraphNode& node = *node_ref.node;
            std::array<const NodeParameter*, kMaxNodeParameters> values{};
            for (std::size_t i = 0; i < node.parameters.size(); ++i) values[i] = &node.parameters[i];
            std::sort(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(node.parameters.size()),
                      [](const auto* left, const auto* right) { return left->key < right->key; });

            for (std::size_t i = 0; i < node.parameters.size(); ++i) {
                const NodeParameter& parameter = *values[i];
                if (parameter.key.value == 0) {
                    return failure(GraphErrorCode::invalid_identifier, "parameter key is zero", node.id, {}, {},
                                   parameter.key);
                }
                if (i > 0 && values[i - 1]->key == parameter.key) {
                    return failure(GraphErrorCode::duplicate_parameter_key,
                                   describe(GraphErrorCode::duplicate_parameter_key), node.id, {}, {}, parameter.key);
                }
                const ParameterDescriptor* descriptor = find_parameter(*node_ref.descriptor, parameter.key);
                if (descriptor == nullptr) {
                    return failure(GraphErrorCode::unknown_parameter, describe(GraphErrorCode::unknown_parameter),
                                   node.id, {}, {}, parameter.key);
                }
                if (descriptor->kind != kind_of(parameter.value)) {
                    return failure(GraphErrorCode::parameter_type_mismatch,
                                   describe(GraphErrorCode::parameter_type_mismatch), node.id, {}, {}, parameter.key);
                }
                if (!finite_value(parameter.value)) {
                    return failure(GraphErrorCode::invalid_parameter_value,
                                   describe(GraphErrorCode::invalid_parameter_value), node.id, {}, {}, parameter.key);
                }
                const std::uint64_t bytes = value_bytes(parameter.value);
                if (!add_bounded<std::uint64_t>(14 + bytes, estimated_bytes)) {
                    return failure(GraphErrorCode::graph_too_large, "graph exceeds the serialized payload limit",
                                   node.id, {}, {}, parameter.key);
                }
            }

            for (const ParameterDescriptor& descriptor : node_ref.descriptor->parameters) {
                if (!descriptor.required) continue;
                const auto found = std::lower_bound(values.begin(),
                                                    values.begin() + static_cast<std::ptrdiff_t>(node.parameters.size()),
                                                    descriptor.key,
                                                    [](const NodeParameter* value, ParameterKey key) {
                                                        return value->key < key;
                                                    });
                if (found == values.begin() + static_cast<std::ptrdiff_t>(node.parameters.size()) ||
                    (*found)->key != descriptor.key) {
                    return failure(GraphErrorCode::missing_required_parameter,
                                   describe(GraphErrorCode::missing_required_parameter), node.id, {}, {},
                                   descriptor.key);
                }
            }
        }

        std::vector<const GraphEdge*> edges;
        edges.reserve(graph.edges.size());
        std::vector<ConnectionRef> connections;
        connections.reserve(graph.edges.size());
        std::vector<std::vector<std::size_t>> adjacency(nodes.size());
        for (const auto& edge : graph.edges) edges.push_back(&edge);
        std::sort(edges.begin(), edges.end(), [](const GraphEdge* left, const GraphEdge* right) {
            return left->id < right->id;
        });

        for (std::size_t edge_index = 0; edge_index < edges.size(); ++edge_index) {
            const GraphEdge& edge = *edges[edge_index];
            if (edge.id.value.is_zero()) {
                return failure(GraphErrorCode::invalid_identifier, "edge ID is zero", {}, edge.id);
            }
            if (edge_index > 0 && edges[edge_index - 1]->id == edge.id) {
                return failure(GraphErrorCode::duplicate_edge_id, describe(GraphErrorCode::duplicate_edge_id), {},
                               edge.id);
            }

            const auto source_index = find_node_index(nodes, edge.source_node);
            if (!source_index.has_value()) {
                return failure(GraphErrorCode::unknown_source_node, describe(GraphErrorCode::unknown_source_node), {},
                               edge.id, edge.source_port);
            }
            const auto destination_index = find_node_index(nodes, edge.destination_node);
            if (!destination_index.has_value()) {
                return failure(GraphErrorCode::unknown_destination_node,
                               describe(GraphErrorCode::unknown_destination_node), {}, edge.id, edge.destination_port);
            }

            const NodeRef& source_node = nodes[*source_index];
            const NodeRef& destination_node = nodes[*destination_index];
            const PortDescriptor* source_port = find_port(*source_node.descriptor, edge.source_port);
            if (source_port == nullptr) {
                return failure(GraphErrorCode::unknown_source_port, describe(GraphErrorCode::unknown_source_port), {},
                               edge.id, edge.source_port);
            }
            const PortDescriptor* destination_port = find_port(*destination_node.descriptor, edge.destination_port);
            if (destination_port == nullptr) {
                return failure(GraphErrorCode::unknown_destination_port,
                               describe(GraphErrorCode::unknown_destination_port), {}, edge.id, edge.destination_port);
            }
            if (source_port->direction != PortDirection::output ||
                destination_port->direction != PortDirection::input) {
                return failure(GraphErrorCode::port_direction_mismatch,
                               describe(GraphErrorCode::port_direction_mismatch), {}, edge.id,
                               source_port->direction != PortDirection::output ? edge.source_port : edge.destination_port);
            }
            if (source_port->type_key != destination_port->type_key) {
                return failure(GraphErrorCode::port_type_mismatch, describe(GraphErrorCode::port_type_mismatch), {},
                               edge.id, edge.destination_port);
            }

            connections.push_back(ConnectionRef{edge.destination_node, edge.destination_port,
                                                destination_port->max_connections});
            if (!is_cycle_breaking_input(*destination_node.descriptor, edge.destination_port)) {
                adjacency[*source_index].push_back(*destination_index);
            }
            if (!add_bounded<std::uint64_t>(72, estimated_bytes)) {
                return failure(GraphErrorCode::graph_too_large, "graph exceeds the serialized payload limit", {},
                               edge.id);
            }
        }

        std::sort(connections.begin(), connections.end(), before_connection);
        for (std::size_t first = 0; first < connections.size();) {
            std::size_t last = first + 1;
            while (last < connections.size() && connections[last].node == connections[first].node &&
                   connections[last].port == connections[first].port) {
                ++last;
            }
            const std::uint32_t maximum = connections[first].maximum;
            if (maximum != 0 && last - first > maximum) {
                const auto code = maximum == 1 ? GraphErrorCode::duplicate_input_connection
                                               : GraphErrorCode::too_many_connections;
                return failure(code, describe(code), connections[first].node, {}, connections[first].port);
            }
            first = last;
        }

        // Iterative DFS avoids recursion on graphs controlled by project data.
        std::vector<std::uint8_t> colors(nodes.size(), 0);
        std::vector<std::pair<std::size_t, std::size_t>> stack;
        stack.reserve(nodes.size());
        for (std::size_t root = 0; root < nodes.size(); ++root) {
            if (colors[root] != 0) continue;
            colors[root] = 1;
            stack.emplace_back(root, 0);
            while (!stack.empty()) {
                auto& current = stack.back();
                auto& outgoing = adjacency[current.first];
                if (current.second == outgoing.size()) {
                    colors[current.first] = 2;
                    stack.pop_back();
                    continue;
                }
                const std::size_t next = outgoing[current.second++];
                if (colors[next] == 1) {
                    return failure(GraphErrorCode::cycle_detected, describe(GraphErrorCode::cycle_detected),
                                   nodes[next].node->id);
                }
                if (colors[next] == 0) {
                    colors[next] = 1;
                    stack.emplace_back(next, 0);
                }
            }
        }

        return {};
    } catch (const std::bad_alloc&) {
        return failure(GraphErrorCode::allocation_failed, describe(GraphErrorCode::allocation_failed));
    } catch (...) {
        return failure(GraphErrorCode::internal_failure, describe(GraphErrorCode::internal_failure));
    }
}

NodeRegistry make_particle_node_registry() {
    using namespace graph_keys;
    NodeRegistry registry;

    NodeTypeDescriptor emitter;
    emitter.type_key = kEmitterNode;
    emitter.schema_version = 7;
    emitter.ports.push_back(PortDescriptor{kEmitterParents, PortDirection::input, kParticleStream, false, 0});
    emitter.ports.push_back(PortDescriptor{kEmitterParticles, PortDirection::output, kParticleStream, false, 0});
    emitter.parameters = {
        ParameterDescriptor{kBirthRate, ParameterKind::float64, true},
        ParameterDescriptor{kSeed, ParameterKind::uint32, true},
        ParameterDescriptor{kEmitterShape, ParameterKind::uint32, true},
        ParameterDescriptor{kEmitterOrigin, ParameterKind::vector3_float64, true},
        ParameterDescriptor{kVelocity, ParameterKind::vector3_float64, true},
        ParameterDescriptor{kParticleSize, ParameterKind::float64, true},
        ParameterDescriptor{kOpacity, ParameterKind::float64, true},
        ParameterDescriptor{kEmitterSize, ParameterKind::float64, true},
        ParameterDescriptor{kVelocitySpread, ParameterKind::float64, true},
        // Settings-based construction may omit authoring-only direction values.
        ParameterDescriptor{kEmissionSpeed, ParameterKind::float64, false},
        ParameterDescriptor{kEmissionSpeedRandom, ParameterKind::float64, false},
        ParameterDescriptor{kEmissionAngleX, ParameterKind::float64, false},
        ParameterDescriptor{kEmissionAngleY, ParameterKind::float64, false},
        ParameterDescriptor{kEmissionAngleZ, ParameterKind::float64, false},
        ParameterDescriptor{kEmitterOrient, ParameterKind::vector3_float64, false},
        ParameterDescriptor{kDirectionMode, ParameterKind::uint32, false},
        ParameterDescriptor{kDirectionSpan, ParameterKind::float64, false},
        // Dimensions may use Settings defaults in direct graph construction.
        ParameterDescriptor{kEmitterSizeX, ParameterKind::float64, false},
        ParameterDescriptor{kEmitterSizeY, ParameterKind::float64, false},
        ParameterDescriptor{kEmitterSizeZ, ParameterKind::float64, false},
        ParameterDescriptor{kEmissionSpeedRandomPercent, ParameterKind::float64, false},
        ParameterDescriptor{kEmittingMode, ParameterKind::uint32, false},
        ParameterDescriptor{kAuxiliarySource, ParameterKind::uint32, false},
        ParameterDescriptor{kEmitChance, ParameterKind::float64, false},
        ParameterDescriptor{kEmitLifeStart, ParameterKind::float64, false},
        ParameterDescriptor{kEmitLifeEnd, ParameterKind::float64, false},
        ParameterDescriptor{kInheritVelocity, ParameterKind::float64, false},
        ParameterDescriptor{kInheritSize, ParameterKind::float64, false},
        ParameterDescriptor{kInheritOpacity, ParameterKind::float64, false},
        ParameterDescriptor{kInheritColor, ParameterKind::float64, false},
    };

    NodeTypeDescriptor particle;
    particle.type_key = kParticleNode;
    particle.schema_version = 7;
    particle.ports = {
        PortDescriptor{kParticleParticlesIn, PortDirection::input, kParticleStream, true, 0},
        PortDescriptor{kParticleParticlesOut, PortDirection::output, kParticleStream, false, 0},
    };
    particle.parameters = {
        ParameterDescriptor{kColorStart, ParameterKind::vector3_float64, true},
        ParameterDescriptor{kColorEnd, ParameterKind::vector3_float64, false},
        ParameterDescriptor{kSizeStart, ParameterKind::float64, true},
        ParameterDescriptor{kSizeEnd, ParameterKind::float64, true},
        ParameterDescriptor{kOpacityStart, ParameterKind::float64, true},
        ParameterDescriptor{kOpacityEnd, ParameterKind::float64, true},
        ParameterDescriptor{kSizeOverLifeCurve, ParameterKind::opaque_bytes, false},
        ParameterDescriptor{kOpacityOverLifeCurve, ParameterKind::opaque_bytes, false},
        ParameterDescriptor{kSizeRandom, ParameterKind::float64, false},
        ParameterDescriptor{kOpacityRandom, ParameterKind::float64, false},
        ParameterDescriptor{kParticleLifetimeSeconds, ParameterKind::float64, true},
        ParameterDescriptor{kParticleColorMode, ParameterKind::uint32, false},
        ParameterDescriptor{kColorGradient, ParameterKind::opaque_bytes, false},
        ParameterDescriptor{kLifeRandom, ParameterKind::float64, false},
        ParameterDescriptor{kSizeY, ParameterKind::float64, false},
        ParameterDescriptor{kAngleRandom, ParameterKind::float64, false},
        ParameterDescriptor{kRotationSpeedRandom, ParameterKind::float64, false},
        ParameterDescriptor{kParticleFeather, ParameterKind::float64, false},
        ParameterDescriptor{kParticleShape, ParameterKind::uint32, false},
        ParameterDescriptor{kOrientTo, ParameterKind::uint32, false},
        ParameterDescriptor{kLimitTo2D, ParameterKind::uint32, false},
        ParameterDescriptor{kUpAxis, ParameterKind::uint32, false},
        ParameterDescriptor{kParticleAngles, ParameterKind::vector3_float64, false},
        ParameterDescriptor{kRotationSpeed, ParameterKind::vector3_float64, false},
        ParameterDescriptor{kRandomLimit, ParameterKind::uint32, false},
        ParameterDescriptor{kLimitAngle, ParameterKind::float64, false},
        ParameterDescriptor{kRotationOverLife, ParameterKind::opaque_bytes, false},
        ParameterDescriptor{kAnchorX, ParameterKind::float64, false},
        ParameterDescriptor{kAnchorY, ParameterKind::float64, false},

    };

    NodeTypeDescriptor output;
    output.type_key = kOutputNode;
    output.schema_version = 4;
    output.ports.push_back(PortDescriptor{kOutputParticles, PortDirection::input, kParticleStream, true, 0});
    output.parameters = {
        ParameterDescriptor{kParticleCount, ParameterKind::uint32, true},
        ParameterDescriptor{kTimeRemapEnabled, ParameterKind::uint32, false},
        ParameterDescriptor{kTimeRemapSeconds, ParameterKind::float64, false},
        ParameterDescriptor{kPreviewEnabled, ParameterKind::uint32, false},
        ParameterDescriptor{kPreviewChance, ParameterKind::float64, false},
        ParameterDescriptor{kAcceleration, ParameterKind::uint32, false},
        ParameterDescriptor{kTimeSamplingHz, ParameterKind::uint32, false},
        ParameterDescriptor{kMotionBlur, ParameterKind::uint32, false},
        ParameterDescriptor{kShutterAngle, ParameterKind::float64, false},
        ParameterDescriptor{kShutterPhase, ParameterKind::float64, false},
        ParameterDescriptor{kMotionBlurType, ParameterKind::uint32, false},
        ParameterDescriptor{kMotionBlurLevels, ParameterKind::float64, false},
        ParameterDescriptor{kLinearAccuracy, ParameterKind::float64, false},
        ParameterDescriptor{kOpacityBoost, ParameterKind::float64, false},
        ParameterDescriptor{kMotionBlurDisregard, ParameterKind::uint32, false},
    };

    NodeTypeDescriptor force;
    force.type_key = kForceNode;
    force.schema_version = 3;
    force.ports = {
        PortDescriptor{kForceParticlesIn, PortDirection::input, kParticleStream, true, 0},
        PortDescriptor{kForceParticlesOut, PortDirection::output, kParticleStream, false, 0},
    };
    force.parameters = {
        ParameterDescriptor{kGravity, ParameterKind::vector3_float64, true},
        ParameterDescriptor{kLinearDrag, ParameterKind::float64, true},
        ParameterDescriptor{kGravityRandom, ParameterKind::float64, false},
        ParameterDescriptor{kWind, ParameterKind::vector3_float64, false},
        ParameterDescriptor{kSpin, ParameterKind::float64, false},
        ParameterDescriptor{kSpinFrequency, ParameterKind::float64, false},
        ParameterDescriptor{kSpinResist, ParameterKind::float64, false},
        ParameterDescriptor{kSpinDelay, ParameterKind::float64, false},
        ParameterDescriptor{kWindSpinCurve, ParameterKind::opaque_bytes, false},
    };

    NodeTypeDescriptor transform;
    transform.type_key=kTransformNode;transform.schema_version=1;
    transform.ports={PortDescriptor{kTransformParticlesIn,PortDirection::input,kParticleStream,true,0},
        PortDescriptor{kTransformParticlesOut,PortDirection::output,kParticleStream,false,0}};
    transform.parameters={
        ParameterDescriptor{kTransformAnchor,ParameterKind::vector3_float64,true},
        ParameterDescriptor{kTransformPosition,ParameterKind::vector3_float64,true},
        ParameterDescriptor{kTransformRotation,ParameterKind::vector3_float64,true},
        ParameterDescriptor{kTransformSystemScale,ParameterKind::vector3_float64,true},
        ParameterDescriptor{kTransformParticleScale,ParameterKind::float64,true},
        ParameterDescriptor{kTransformParticleOpacity,ParameterKind::float64,true},
        ParameterDescriptor{kTransformInheritedMatrix,ParameterKind::opaque_bytes,false}};
    registry.types.push_back(std::move(emitter));
    registry.types.push_back(std::move(particle));
    registry.types.push_back(std::move(force));
    registry.types.push_back(std::move(output));
    registry.types.push_back(std::move(transform));
    return registry;
}

const NodeRegistry& particle_node_registry() {
    // The built-in schemas are immutable after initialization and contain no AE
    // state; sharing this registry does not add mutable render-global state.
    static const NodeRegistry registry = make_particle_node_registry();
    return registry;
}

} // namespace starfield::core
