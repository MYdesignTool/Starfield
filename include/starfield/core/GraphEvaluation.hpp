#pragma once

#include "starfield/core/Graph.hpp"
#include "starfield/core/ParticleSimulation.hpp"
#include "starfield/core/EmitterHistory.hpp"
#include "starfield/core/EmissionTimeline.hpp"
#include <memory>

namespace starfield::core {

struct EvaluatedGraph {
    std::vector<ParticleInstance> particles;
    // Dependency order, with UUID ordering for independent nodes. Only ancestors
    // of the single output execute. The snapshot and its identities stay intact.
    std::vector<NodeId> evaluated_nodes;
};

// Pre-render supplies actual authored values at historical times. No host
// objects or frame-order state cross this boundary.
class TemporalGraphSampler {
public:
    virtual ~TemporalGraphSampler() = default;
    [[nodiscard]] virtual Result<GraphNode> node(NodeId, double seconds) = 0;
    [[nodiscard]] virtual Result<double> rate(NodeId, double seconds) = 0;
    // Only metadata can certify constancy/linear interpolation, not equal samples.
    [[nodiscard]] virtual Result<std::optional<EmissionRateProfile>> rate_profile(NodeId) {
        return Result<std::optional<EmissionRateProfile>>::success(std::nullopt);
    }
    // Optional caller-owned prefix lease. The caller certifies dependency identity
    // and holds exclusive access for its lifetime; missing cache changes no result.
    [[nodiscard]] virtual std::shared_ptr<EmissionTimeline> emission_timeline(NodeId,unsigned) {return {};}
    [[nodiscard]] virtual std::optional<double> lifetime_upper_bound(NodeId) {return {};}
    [[nodiscard]] virtual Result<double> lifetime(NodeId id, double seconds) {
        auto sampled=node(id,seconds);
        if(!sampled.has_value()) return Result<double>::failure(sampled.error());
        for(const auto& parameter:sampled.value().parameters) if(parameter.key==graph_keys::kParticleLifetimeSeconds) {
            const auto* value=std::get_if<double>(&parameter.value);
            if(value) return Result<double>::success(*value);
        }
        return Result<double>::failure(ErrorCode::invalid_request,"historical lifetime missing");
    }
};
[[nodiscard]] Result<EvaluatedGraph> evaluate_temporal_particle_graph(
    const Graph&, RationalTime, const Cancellation&, EmitterDimensionContext,
    TemporalGraphSampler&);
[[nodiscard]] Result<OpaqueBytes> encode_evaluated_particles(
    const EvaluatedGraph&, RationalTime);
[[nodiscard]] Result<EvaluatedGraph> decode_evaluated_particles(
    const OpaqueBytes&, RationalTime);

// Evaluate an immutable schema-1 snapshot at absolute rational time. Node values
// are sampled frame values; an optional origin sampler/history supplies emitter
// positions at each birth. Appearance endpoints define an age curve. Reject invalid
// topology, ambiguous outputs, and out-of-range values rather than falling back.
[[nodiscard]] Result<EvaluatedGraph> evaluate_particle_graph(
    const Graph& graph, RationalTime time, const Cancellation& cancellation,
    EmitterDimensionContext dimension_context = {}, EmitterOriginSampler* origin_sampler = nullptr);

// Construct a schema-1 graph with an explicit Particle node. The Particle node
// receives the settings' age-curve values; when appearance is disabled, constant
// emitter size/opacity and white color are used to preserve the current look.
[[nodiscard]] Result<Graph> make_emitter_particle_output_graph(
    const Settings& settings, NodeId emitter, NodeId particle, NodeId output,
    EdgeId emitter_to_particle, EdgeId particle_to_output);

// Construct the current Particle-first chain with a serial force stage:
// Emitter -> Particle -> Force -> Output.
[[nodiscard]] Result<Graph> make_emitter_particle_force_output_graph(
    const Settings& settings, NodeId emitter, NodeId particle, NodeId force, NodeId output,
    EdgeId emitter_to_particle, EdgeId particle_to_force, EdgeId force_to_output);

// Construct the current single-emitter Alpha chain with one force and one
// appearance stage. The settings' gravity/drag and age-curve fields are written
// to their corresponding nodes, not to the emitter node.
[[nodiscard]] Result<Graph> make_emitter_particle_force_appearance_output_graph(
    const Settings& settings, NodeId emitter, NodeId particle, NodeId force,
    NodeId appearance, NodeId output, EdgeId emitter_to_particle,
    EdgeId particle_to_force, EdgeId force_to_appearance, EdgeId appearance_to_output);

} // namespace starfield::core
