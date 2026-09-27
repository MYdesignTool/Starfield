#include "starfield/core/GraphEvaluation.hpp"

#include <algorithm>
#include <functional>
#include <new>
#include <queue>

namespace starfield::core {
namespace {

using namespace graph_keys;

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
            case kParticleSize.value: settings.particle_size = std::get<double>(parameter.value); break;
            case kOpacity.value: settings.opacity = std::get<double>(parameter.value); break;
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

} // namespace

Result<Graph> make_emitter_output_graph(const Settings& settings, NodeId emitter, NodeId output, EdgeId connection) {
    using R = Result<Graph>;
    try {
        if (!validate_settings(settings).notices.empty()) {
            return R::failure(ErrorCode::invalid_request, "cannot create graph from out-of-range settings");
        }
        Graph graph;
        graph.nodes = {
            GraphNode{emitter, kEmitterNode, 1, {
                {kParticleCount, settings.particle_count}, {kBirthRate, settings.birth_rate},
                {kSeed, settings.seed}, {kLifetimeSeconds, settings.particle_lifetime_seconds},
                {kEmitterShape, static_cast<std::uint32_t>(settings.emitter_shape)},
                {kEmitterOrigin, settings.emitter_origin}, {kVelocity, settings.velocity},
                {kParticleSize, settings.particle_size}, {kOpacity, settings.opacity},
                {kEmitterSize, settings.emitter_size}, {kVelocitySpread, settings.velocity_spread}}},
            GraphNode{output, kOutputNode, 1, {}}
        };
        graph.edges = {GraphEdge{connection, emitter, kEmitterParticles, output, kOutputParticles}};
        const auto validation = validate_graph(graph, particle_node_registry());
        if (!validation) {
            return R::failure(validation.error.code == GraphErrorCode::allocation_failed
                ? ErrorCode::allocation_failed : ErrorCode::invalid_request, validation.error.detail);
        }
        return R::success(std::move(graph));
    } catch (const std::bad_alloc&) {
        return R::failure(ErrorCode::allocation_failed, "default graph allocation failed");
    } catch (...) {
        return R::failure(ErrorCode::internal_failure, "default graph construction failed");
    }
}

Result<EvaluatedGraph> evaluate_particle_graph(const Graph& graph, RationalTime time,
                                               const Cancellation& cancellation) {
    using R = Result<EvaluatedGraph>;
    if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "graph evaluation cancelled");
    const auto normalized = make_rational(time.value, time.scale);
    if (!normalized) return R::failure(ErrorCode::invalid_time, "invalid graph evaluation time");
    try {
        // Validation is bounded by the graph limits, even for disconnected nodes.
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
        // Check semantic ranges on disconnected emitters too. Invalid parked
        // nodes must not become latent failures when the editor connects them.
        std::vector<ValidatedSettings> settings(count);
        for (std::size_t i = 0; i < count; ++i) {
            if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "node validation cancelled");
            if (nodes[i]->type_key == kOutputNode) {
                if (output != count) return R::failure(ErrorCode::invalid_request, "graph requires exactly one output");
                output = i;
            } else if (nodes[i]->type_key == kEmitterNode) {
                auto value = read_emitter(*nodes[i]);
                if (!value.has_value()) return R::failure(value.error());
                settings[i] = value.take_value();
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

        // Stable Kahn traversal; sorting storage cannot change execution order.
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
        bool has_stream = false;
        while (!ready.empty()) {
            if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "graph execution cancelled");
            const auto index = ready.top();
            ready.pop();
            const auto& node = *nodes[index];
            if (node.type_key == kEmitterNode) {
                if (has_stream) return R::failure(ErrorCode::invalid_request, "multiple active particle streams are unsupported");
                auto particles = simulate_particles(settings[index], to_seconds(*normalized), cancellation);
                if (!particles.has_value()) return R::failure(particles.error());
                result.particles = particles.take_value();
                has_stream = true;
            } else if (!has_stream) {
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
        return R::success(std::move(result));
    } catch (const std::bad_alloc&) {
        return R::failure(ErrorCode::allocation_failed, "graph evaluation allocation failed");
    } catch (...) {
        return R::failure(ErrorCode::internal_failure, "graph evaluation failed");
    }
}

} // namespace starfield::core
