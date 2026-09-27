#pragma once

#include "starfield/core/Graph.hpp"
#include "starfield/core/ParticleSimulation.hpp"

namespace starfield::core {

struct EvaluatedGraph {
    std::vector<ParticleInstance> particles;
    // Dependency order, with UUID ordering for independent nodes. Only ancestors
    // of the single output execute. The snapshot and its identities stay intact.
    std::vector<NodeId> evaluated_nodes;
};

// Evaluate an immutable schema-1 snapshot at absolute rational time. Node values
// are constant; appearance endpoints define a linear age curve. Reject invalid
// topology, ambiguous outputs, and out-of-range values rather than falling back.
[[nodiscard]] Result<EvaluatedGraph> evaluate_particle_graph(
    const Graph& graph, RationalTime time, const Cancellation& cancellation);

// Explicit identities allow the host/editor to own UUID creation. Values must
// already satisfy the Settings bounds; this helper does not silently clamp them.
[[nodiscard]] Result<Graph> make_emitter_output_graph(
    const Settings& settings, NodeId emitter, NodeId output, EdgeId connection);

// Construct the current single-emitter Alpha chain with one force and one
// appearance stage. The settings' gravity/drag and age-curve fields are written
// to their corresponding nodes, not to the emitter node.
[[nodiscard]] Result<Graph> make_emitter_force_appearance_output_graph(
    const Settings& settings, NodeId emitter, NodeId force, NodeId appearance, NodeId output,
    EdgeId emitter_to_force, EdgeId force_to_appearance, EdgeId appearance_to_output);

} // namespace starfield::core
