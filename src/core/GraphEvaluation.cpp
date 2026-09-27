#include "starfield/core/GraphEvaluation.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <new>
#include <optional>
#include <queue>

namespace starfield::core {
namespace {

using namespace graph_keys;

struct ForceValues {
    Vec3 gravity{};
    double linear_drag{0.0};
};

struct AppearanceValues {
    Vec3 color_start{1.0, 1.0, 1.0};
    Vec3 color_end{1.0, 1.0, 1.0};
    double size_start{8.0};
    double size_end{8.0};
    double opacity_start{1.0};
    double opacity_end{1.0};
};

const ParameterValue* find_value(const GraphNode& node, ParameterKey key) noexcept {
    const auto found = std::find_if(node.parameters.begin(), node.parameters.end(),
        [key](const NodeParameter& parameter) { return parameter.key == key; });
    return found == node.parameters.end() ? nullptr : &found->value;
}

Result<ValidatedSettings> read_emitter(const GraphNode& node) {
    // validate_graph has already checked required keys and variant alternatives.
    Settings settings;
    for (const NodeParameter& parameter : node.parameters) {
        switch (parameter.key.value) {
            case kParticleCount.value: settings.particle_count = std::get<std::uint32_t>(parameter.value); break;
            case kBirthRate.value: settings.birth_rate = std::get<double>(parameter.value); break;
            case kSeed.value: settings.seed = std::get<std::uint32_t>(parameter.value); break;
            case kLifetimeSeconds.value: settings.particle_lifetime_seconds = std::get<double>(parameter.value); break;
            case kEmitterShape.value: {
                const auto shape = std::get<std::uint32_t>(parameter.value);
                if (shape >= kEmitterShapeCount) {
                    return Result<ValidatedSettings>::failure(ErrorCode::invalid_request, "invalid graph emitter shape");
                }
                settings.emitter_shape = static_cast<EmitterShape>(shape);
                break;
            }
            case kEmitterOrigin.value: settings.emitter_origin = std::get<Vec3>(parameter.value); break;
            case kVelocity.value: settings.velocity = std::get<Vec3>(parameter.value); break;
            case kParticleSize.value:
                settings.particle_size = std::get<double>(parameter.value);
                settings.particle_size_end = settings.particle_size;
                break;
            case kOpacity.value:
                settings.opacity = std::get<double>(parameter.value);
                settings.opacity_end = settings.opacity;
                break;
            case kEmitterSize.value: settings.emitter_size = std::get<double>(parameter.value); break;
            case kVelocitySpread.value: settings.velocity_spread = std::get<double>(parameter.value); break;
        }
    }
    auto validated = validate_settings(settings);
    if (!validated.notices.empty()) {
        return Result<ValidatedSettings>::failure(ErrorCode::invalid_request, "graph emitter value is outside supported bounds");
    }
    return Result<ValidatedSettings>::success(std::move(validated));
}

Result<ForceValues> read_force(const GraphNode& node) {
    const auto* gravity = find_value(node, kGravity);
    const auto* drag = find_value(node, kLinearDrag);
    if (!gravity || !drag) return Result<ForceValues>::failure(ErrorCode::invalid_request, "force node is missing values");
    Settings settings;
    settings.gravity = std::get<Vec3>(*gravity);
    settings.linear_drag = std::get<double>(*drag);
    auto validated = validate_settings(settings);
    if (!validated.notices.empty()) {
        return Result<ForceValues>::failure(ErrorCode::invalid_request, "force node value is outside supported bounds");
    }
    return Result<ForceValues>::success(ForceValues{validated.value.gravity, validated.value.linear_drag});
}

Result<AppearanceValues> read_appearance(const GraphNode& node) {
    const auto* color_start = find_value(node, kColorStart);
    const auto* color_end = find_value(node, kColorEnd);
    const auto* size_start = find_value(node, kSizeStart);
    const auto* size_end = find_value(node, kSizeEnd);
    const auto* opacity_start = find_value(node, kOpacityStart);
    const auto* opacity_end = find_value(node, kOpacityEnd);
    if (!color_start || !color_end || !size_start || !size_end || !opacity_start || !opacity_end) {
        return Result<AppearanceValues>::failure(ErrorCode::invalid_request, "appearance node is missing values");
    }
    Settings settings;
    settings.color_start = std::get<Vec3>(*color_start);
    settings.color_end = std::get<Vec3>(*color_end);
    settings.particle_size = std::get<double>(*size_start);
    settings.particle_size_end = std::get<double>(*size_end);
    settings.opacity = std::get<double>(*opacity_start);
    settings.opacity_end = std::get<double>(*opacity_end);
    settings.appearance_enabled = true;
    auto validated = validate_settings(settings);
    if (!validated.notices.empty()) {
        return Result<AppearanceValues>::failure(ErrorCode::invalid_request, "appearance node value is outside supported bounds");
    }
    return Result<AppearanceValues>::success(AppearanceValues{
        validated.value.color_start, validated.value.color_end,
        validated.value.particle_size, validated.value.particle_size_end,
        validated.value.opacity, validated.value.opacity_end});
}

GraphNode make_emitter_node(const Settings& settings, NodeId id) {
    return GraphNode{id, kEmitterNode, 1, {
        {kParticleCount, settings.particle_count}, {kBirthRate, settings.birth_rate},
        {kSeed, settings.seed}, {kLifetimeSeconds, settings.particle_lifetime_seconds},
        {kEmitterShape, static_cast<std::uint32_t>(settings.emitter_shape)},
        {kEmitterOrigin, settings.emitter_origin}, {kVelocity, settings.velocity},
        {kParticleSize, settings.particle_size}, {kOpacity, settings.opacity},
        {kEmitterSize, settings.emitter_size}, {kVelocitySpread, settings.velocity_spread}}};
}

Result<Graph> validate_constructed_graph(Graph graph, const char* failure_detail) {
    using R = Result<Graph>;
    const auto validation = validate_graph(graph, particle_node_registry());
    if (!validation) {
        return R::failure(validation.error.code == GraphErrorCode::allocation_failed
            ? ErrorCode::allocation_failed : ErrorCode::invalid_request,
            validation.error.detail ? validation.error.detail : failure_detail);
    }
    return R::success(std::move(graph));
}

} // namespace

Result<Graph> make_emitter_output_graph(const Settings& settings, NodeId emitter, NodeId output, EdgeId connection) {
    using R = Result<Graph>;
    try {
        if (!validate_settings(settings).notices.empty()) {
            return R::failure(ErrorCode::invalid_request, "cannot create graph from out-of-range settings");
        }
        Graph graph;
        graph.nodes = {make_emitter_node(settings, emitter), GraphNode{output, kOutputNode, 1, {}}};
        graph.edges = {GraphEdge{connection, emitter, kEmitterParticles, output, kOutputParticles}};
        return validate_constructed_graph(std::move(graph), "default graph validation failed");
    } catch (const std::bad_alloc&) {
        return R::failure(ErrorCode::allocation_failed, "default graph allocation failed");
    } catch (...) {
        return R::failure(ErrorCode::internal_failure, "default graph construction failed");
    }
}

Result<Graph> make_emitter_force_appearance_output_graph(
    const Settings& settings, NodeId emitter, NodeId force, NodeId appearance, NodeId output,
    EdgeId emitter_to_force, EdgeId force_to_appearance, EdgeId appearance_to_output) {
    using R = Result<Graph>;
    try {
        if (!validate_settings(settings).notices.empty()) {
            return R::failure(ErrorCode::invalid_request, "cannot create graph from out-of-range settings");
        }
        Graph graph;
        graph.nodes = {
            make_emitter_node(settings, emitter),
            GraphNode{force, kForceNode, 1, {
                {kGravity, settings.gravity}, {kLinearDrag, settings.linear_drag}}},
            GraphNode{appearance, kAppearanceNode, 1, {
                {kColorStart, settings.color_start}, {kColorEnd, settings.color_end},
                {kSizeStart, settings.particle_size}, {kSizeEnd, settings.particle_size_end},
                {kOpacityStart, settings.opacity}, {kOpacityEnd, settings.opacity_end}}},
            GraphNode{output, kOutputNode, 1, {}},
        };
        graph.edges = {
            GraphEdge{emitter_to_force, emitter, kEmitterParticles, force, kForceParticlesIn},
            GraphEdge{force_to_appearance, force, kForceParticlesOut, appearance, kAppearanceParticlesIn},
            GraphEdge{appearance_to_output, appearance, kAppearanceParticlesOut, output, kOutputParticles},
        };
        return validate_constructed_graph(std::move(graph), "full chain validation failed");
    } catch (const std::bad_alloc&) {
        return R::failure(ErrorCode::allocation_failed, "full chain graph allocation failed");
    } catch (...) {
        return R::failure(ErrorCode::internal_failure, "full chain graph construction failed");
    }
}

Result<EvaluatedGraph> evaluate_particle_graph(const Graph& graph, RationalTime time,
                                               const Cancellation& cancellation) {
    using R = Result<EvaluatedGraph>;
    if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "graph evaluation cancelled");
    const auto normalized = make_rational(time.value, time.scale);
    if (!normalized) return R::failure(ErrorCode::invalid_time, "invalid graph evaluation time");
    try {
        const auto validation = validate_graph(graph, particle_node_registry());
        if (!validation) {
            const auto code = validation.error.code == GraphErrorCode::allocation_failed ? ErrorCode::allocation_failed
                : validation.error.code == GraphErrorCode::internal_failure ? ErrorCode::internal_failure
                : ErrorCode::invalid_request;
            return R::failure(code, validation.error.detail);
        }
        if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "graph validation cancelled");

        std::vector<const GraphNode*> nodes;
        nodes.reserve(graph.nodes.size());
        for (const auto& node : graph.nodes) nodes.push_back(&node);
        std::sort(nodes.begin(), nodes.end(), [](const auto* a, const auto* b) { return a->id < b->id; });
        const auto index_of = [&nodes](NodeId id) {
            return static_cast<std::size_t>(std::lower_bound(nodes.begin(), nodes.end(), id,
                [](const auto* node, NodeId key) { return node->id < key; }) - nodes.begin());
        };

        const std::size_t count = nodes.size();
        std::size_t output = count;
        std::vector<std::optional<ValidatedSettings>> emitters(count);
        std::vector<std::optional<ForceValues>> forces(count);
        std::vector<std::optional<AppearanceValues>> appearances(count);
        // Check semantic bounds on every node, including disconnected/parked nodes.
        for (std::size_t i = 0; i < count; ++i) {
            if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "node validation cancelled");
            const auto& node = *nodes[i];
            if (node.type_key == kOutputNode) {
                if (output != count) return R::failure(ErrorCode::invalid_request, "graph requires exactly one output");
                output = i;
            } else if (node.type_key == kEmitterNode) {
                auto value = read_emitter(node);
                if (!value.has_value()) return R::failure(value.error());
                emitters[i] = value.take_value();
            } else if (node.type_key == kForceNode) {
                auto value = read_force(node);
                if (!value.has_value()) return R::failure(value.error());
                forces[i] = value.take_value();
            } else if (node.type_key == kAppearanceNode) {
                auto value = read_appearance(node);
                if (!value.has_value()) return R::failure(value.error());
                appearances[i] = value.take_value();
            } else {
                return R::failure(ErrorCode::invalid_request, "node has no evaluation kernel");
            }
        }
        if (output == count) return R::failure(ErrorCode::invalid_request, "graph requires exactly one output");

        std::vector<std::vector<std::size_t>> incoming(count), outgoing(count);
        for (const auto& edge : graph.edges) {
            if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "graph planning cancelled");
            const auto source = index_of(edge.source_node);
            const auto destination = index_of(edge.destination_node);
            incoming[destination].push_back(source);
            outgoing[source].push_back(destination);
        }
        std::vector<bool> active(count, false);
        std::vector<std::size_t> pending{output};
        active[output] = true;
        while (!pending.empty()) {
            if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "graph reachability cancelled");
            const auto index = pending.back();
            pending.pop_back();
            for (const auto source : incoming[index]) {
                if (!active[source]) { active[source] = true; pending.push_back(source); }
            }
        }

        std::vector<std::size_t> degrees(count, 0);
        std::priority_queue<std::size_t, std::vector<std::size_t>, std::greater<>> ready;
        std::size_t active_count = 0;
        for (std::size_t i = 0; i < count; ++i) {
            if (!active[i]) continue;
            ++active_count;
            degrees[i] = incoming[i].size();
            if (degrees[i] == 0) ready.push(i);
        }

        EvaluatedGraph result;
        result.evaluated_nodes.reserve(active_count);
        Settings render_settings;
        bool has_emitter = false;
        int previous_stage = -1;
        std::uint32_t active_emitter_count = 0;
        std::uint32_t active_appearance_count = 0;
        while (!ready.empty()) {
            if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "graph execution cancelled");
            const auto index = ready.top();
            ready.pop();
            const auto& node = *nodes[index];
            int stage = -1;
            if (node.type_key == kEmitterNode) stage = 0;
            else if (node.type_key == kForceNode) stage = 1;
            else if (node.type_key == kAppearanceNode) stage = 2;
            else if (node.type_key == kOutputNode) stage = 3;
            if (stage < previous_stage) {
                return R::failure(ErrorCode::invalid_request, "active graph stages must be emitter, force, appearance, output");
            }
            previous_stage = stage;

            if (node.type_key == kEmitterNode) {
                if (++active_emitter_count != 1) {
                    return R::failure(ErrorCode::invalid_request, "multiple active emitters are unsupported");
                }
                render_settings = emitters[index]->value;
                has_emitter = true;
            } else if (node.type_key == kForceNode) {
                if (!has_emitter) return R::failure(ErrorCode::invalid_request, "force node has no upstream emitter");
                const auto& force = *forces[index];
                render_settings.gravity.x += force.gravity.x;
                render_settings.gravity.y += force.gravity.y;
                render_settings.gravity.z += force.gravity.z;
                render_settings.linear_drag += force.linear_drag;
            } else if (node.type_key == kAppearanceNode) {
                if (!has_emitter) return R::failure(ErrorCode::invalid_request, "appearance node has no upstream emitter");
                if (++active_appearance_count != 1) {
                    return R::failure(ErrorCode::invalid_request, "multiple active appearance nodes are unsupported");
                }
                const auto& appearance = *appearances[index];
                render_settings.color_start = appearance.color_start;
                render_settings.color_end = appearance.color_end;
                render_settings.particle_size = appearance.size_start;
                render_settings.particle_size_end = appearance.size_end;
                render_settings.opacity = appearance.opacity_start;
                render_settings.opacity_end = appearance.opacity_end;
                render_settings.appearance_enabled = true;
            } else if (!has_emitter) {
                return R::failure(ErrorCode::invalid_request, "output has no evaluated particle stream");
            }

            result.evaluated_nodes.push_back(node.id);
            for (const auto destination : outgoing[index]) {
                if (active[destination] && --degrees[destination] == 0) ready.push(destination);
            }
        }
        if (result.evaluated_nodes.size() != active_count) {
            return R::failure(ErrorCode::invalid_request, "graph cannot be evaluated in dependency order");
        }
        if (!has_emitter || active_emitter_count != 1) {
            return R::failure(ErrorCode::invalid_request, "active output must have exactly one emitter");
        }
        const auto bounded = validate_settings(render_settings);
        if (!bounded.notices.empty()) {
            return R::failure(ErrorCode::invalid_request, "combined force or appearance values exceed supported bounds");
        }
        auto particles = simulate_particles(bounded, to_seconds(*normalized), cancellation);
        if (!particles.has_value()) return R::failure(particles.error());
        result.particles = particles.take_value();
        return R::success(std::move(result));
    } catch (const std::bad_alloc&) {
        return R::failure(ErrorCode::allocation_failed, "graph evaluation allocation failed");
    } catch (...) {
        return R::failure(ErrorCode::internal_failure, "graph evaluation failed");
    }
}

} // namespace starfield::core
