#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/AgeCurve.hpp"

#include <new>
#include <utility>

namespace starfield::core {
namespace {
using namespace graph_keys;

GraphNode make_emitter_node(const Settings& settings, NodeId id) {
    return GraphNode{id, kEmitterNode, 1, {
        {kParticleCount, settings.particle_count}, {kBirthRate, settings.birth_rate},
        {kSeed, settings.seed}, {kLifetimeSeconds, settings.particle_lifetime_seconds},
        {kEmitterShape, static_cast<std::uint32_t>(settings.emitter_shape)},
        {kEmitterOrigin, settings.emitter_origin}, {kVelocity, settings.velocity},
        {kParticleSize, settings.particle_size}, {kOpacity, settings.opacity},
        {kEmitterSize, settings.emitter_size}, {kVelocitySpread, settings.velocity_spread},
        {kEmissionSpeed, settings.emission_speed},
        {kEmissionSpeedRandom, settings.emission_speed_random},
        {kEmissionAngleX, settings.emission_angles_degrees.x},
        {kEmissionAngleY, settings.emission_angles_degrees.y},
        {kEmissionAngleZ, settings.emission_angles_degrees.z},
        {kDirectionMode, static_cast<std::uint32_t>(settings.direction_mode)},
        {kDirectionSpan, settings.direction_span_degrees},
        {kEmitterSizePercentX, settings.emitter_size_percent.x},
        {kEmitterSizePercentY, settings.emitter_size_percent.y},
        {kEmitterSizePercentZ, settings.emitter_size_percent.z}}};
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

Result<Graph> make_emitter_particle_output_graph(const Settings& settings, NodeId emitter, NodeId particle,
                                                NodeId output, EdgeId emitter_to_particle,
                                                EdgeId particle_to_output) {
    using R = Result<Graph>;
    try {
        if (!validate_settings(settings).notices.empty()) {
            return R::failure(ErrorCode::invalid_request, "cannot create graph from out-of-range settings");
        }
        const Vec3 default_color{1.0, 1.0, 1.0};
        const Vec3 color_start = settings.appearance_enabled ? settings.color_start : default_color;
        const Vec3 color_end = settings.appearance_enabled ? settings.color_end : default_color;
        const double size_end = settings.appearance_enabled ? settings.particle_size_end : settings.particle_size;
        const double opacity_end = settings.appearance_enabled ? settings.opacity_end : settings.opacity;

        GraphNode particle_node{particle, kParticleNode, 1, {
            {kColorStart, color_start}, {kColorEnd, color_end},
            {kSizeStart, settings.particle_size}, {kSizeEnd, size_end},
            {kOpacityStart, settings.opacity}, {kOpacityEnd, opacity_end}}};
        if (settings.size_over_life.count != 0) {
            particle_node.parameters.push_back({kSizeOverLifeCurve, encode_age_curve(settings.size_over_life)});
        }
        if (settings.opacity_over_life.count != 0) {
            particle_node.parameters.push_back({kOpacityOverLifeCurve, encode_age_curve(settings.opacity_over_life)});
        }
        if (settings.particle_size_random_percent != 0.0) {
            particle_node.parameters.push_back({kSizeRandom, settings.particle_size_random_percent});
        }
        if (settings.opacity_random_percent != 0.0) {
            particle_node.parameters.push_back({kOpacityRandom, settings.opacity_random_percent});
        }

        Graph graph;
        graph.nodes = {
            make_emitter_node(settings, emitter),
            std::move(particle_node),
            GraphNode{output, kOutputNode, 1, {}},
        };
        graph.edges = {
            GraphEdge{emitter_to_particle, emitter, kEmitterParticles, particle, kParticleParticlesIn},
            GraphEdge{particle_to_output, particle, kParticleParticlesOut, output, kOutputParticles},
        };
        return validate_constructed_graph(std::move(graph), "particle graph validation failed");
    } catch (const std::bad_alloc&) {
        return R::failure(ErrorCode::allocation_failed, "particle graph construction allocation failed");
    } catch (...) {
        return R::failure(ErrorCode::internal_failure, "particle graph construction failed");
    }
}

Result<Graph> make_emitter_particle_force_output_graph(
    const Settings& settings, NodeId emitter, NodeId particle, NodeId force, NodeId output,
    EdgeId emitter_to_particle, EdgeId particle_to_force, EdgeId force_to_output) {
    using R = Result<Graph>;
    try {
        auto base = make_emitter_particle_output_graph(settings, emitter, particle, output,
                                                       emitter_to_particle, particle_to_force);
        if (!base.has_value()) return base;
        Graph graph = base.take_value();
        graph.nodes.push_back(GraphNode{force, kForceNode, 1, {
            {kGravity, settings.gravity}, {kLinearDrag, settings.linear_drag}}});

        // The base constructor provides the stable Emitter -> Particle edge and a
        // Particle -> Output edge. Replace the latter with the requested serial
        // Particle -> Force -> Output chain while preserving the caller's IDs.
        if (graph.edges.size() != 2 || graph.edges[1].source_node != particle ||
            graph.edges[1].destination_node != output) {
            return R::failure(ErrorCode::internal_failure, "particle graph did not contain its expected output edge");
        }
        graph.edges[1] = GraphEdge{particle_to_force, particle, kParticleParticlesOut,
                                   force, kForceParticlesIn};
        graph.edges.push_back(GraphEdge{force_to_output, force, kForceParticlesOut,
                                        output, kOutputParticles});
        return validate_constructed_graph(std::move(graph), "particle-force graph validation failed");
    } catch (const std::bad_alloc&) {
        return R::failure(ErrorCode::allocation_failed, "particle-force graph allocation failed");
    } catch (...) {
        return R::failure(ErrorCode::internal_failure, "particle-force graph construction failed");
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
        GraphNode appearance_node{appearance, kAppearanceNode, 1, {
            {kColorStart, settings.color_start}, {kColorEnd, settings.color_end},
            {kSizeStart, settings.particle_size}, {kSizeEnd, settings.particle_size_end},
            {kOpacityStart, settings.opacity}, {kOpacityEnd, settings.opacity_end}}};
        if (settings.size_over_life.count != 0) {
            appearance_node.parameters.push_back({kSizeOverLifeCurve, encode_age_curve(settings.size_over_life)});
        }
        if (settings.opacity_over_life.count != 0) {
            appearance_node.parameters.push_back({kOpacityOverLifeCurve, encode_age_curve(settings.opacity_over_life)});
        }
        if (settings.particle_size_random_percent != 0.0) {
            appearance_node.parameters.push_back({kSizeRandom, settings.particle_size_random_percent});
        }
        if (settings.opacity_random_percent != 0.0) {
            appearance_node.parameters.push_back({kOpacityRandom, settings.opacity_random_percent});
        }
        graph.nodes = {
            make_emitter_node(settings, emitter),
            GraphNode{force, kForceNode, 1, {
                {kGravity, settings.gravity}, {kLinearDrag, settings.linear_drag}}},
            std::move(appearance_node),
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
} // namespace starfield::core
