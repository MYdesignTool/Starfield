#pragma once

#include "starfield/core/Graph.hpp"
#include "starfield/core/ParticleSimulation.hpp"
#include "starfield/core/EmitterHistory.hpp"

namespace starfield::core {

struct EvaluatedGraph {
    std::vector<ParticleInstance> particles;
    // Dependency order, with UUID ordering for independent nodes. Only ancestors
    // of the single output execute. The snapshot and its identities stay intact.
    std::vector<NodeId> evaluated_nodes;
};

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
