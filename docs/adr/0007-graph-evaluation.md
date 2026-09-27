# ADR 0007: deterministic evaluation of graph snapshots

- Status: accepted for G-03's runtime; extended by M3-02 with the force and appearance stages.
- Date: 2026-09-27.
- Depends on ADR 0005 (render boundary) and ADR 0006 (graph validation).

## Decision

`RenderRequest::graph` owns an immutable, host-independent graph snapshot. If
present, it supplies the particle stream instead of `settings`. A malformed graph
returns an error; it never silently falls back to another source. G-04 supplies a
graph snapshot from Node Graph mode or constructs one from time-sampled AE controls
in legacy mode (ADR 0008).
An empty render ROI remains an allocation-free no-op, without evaluating particles.

The evaluator validates every node/edge against the immutable built-in registry.
Emitter numerical bounds are checked separately from structural/type validation;
out-of-range graph values are rejected, including values on disconnected nodes.
This differs intentionally from the legacy host adapter's value-clamping behavior.

Exactly one output is required. Only its ancestors execute. A stable Kahn traversal
orders dependencies before consumers and independent nodes by UUID, without
reordering or modifying the graph. Disconnected valid nodes remain editable but do
not allocate particle streams. The current schemas cover the single-emitter Alpha
chain emitter -> force -> appearance -> output (the legacy emitter -> output subset
still evaluates). One active stream is supported: a second active emitter or a second
active appearance stage is rejected at runtime, and the active stages must appear in
chain order, so a rewired appearance-before-force graph fails instead of rendering a
guess. Force values accumulate into the render settings; appearance values override
the emitter's size/opacity and supply the age-curve endpoints. Merge, branching, and
multiple-output semantics still need a contract before they can be enabled.

Time enters as a signed rational, is normalized, and converts to seconds only at
the simulation boundary. Schema-1 node parameters are constant values: this work
does not introduce keyframes, animated parameter sampling, or emission history.
Those require explicit time-sampling and serialization contracts under M3-03.
No playback history, host handle, mutable render global, or filesystem access is
introduced. Cancellation is checked before/after bounded validation, during
planning/execution, and by the existing particle simulator.

The CPU rasterizer consumes each particle's opacity, rather than the fallback
settings' opacity. This preserves flat-path pixels and allows later appearance
nodes to modify per-particle opacity without changing the rasterizer contract.

`make_emitter_output_graph` maps all eleven settings fields to stable graph keys.
`make_emitter_force_appearance_output_graph` does the same for the full chain, writing
the gravity/drag fields to the force node and the color/size/opacity curves to the
appearance node (not to the emitter). Callers supply the node/edge identities; neither
helper derives them from AE parameter IDs, and duplicate or zero identities are
rejected. The graph parameter keys are scoped per node type, so force and appearance
reuse small local key ranges without aliasing emitter parameters. These are
construction primitives, not an AE migration or persistence implementation.

## Evidence and remaining work

Core regression run on 2026-09-27: 6,184 assertions, zero failures. Added cases
compare graph/flat pixels across four emitter shapes, 8/16/32-bit formats, repeated
and reverse times, negative/subframe time, and reduced-resolution cropped output.
Cases also cover unchanged graph bytes after evaluation, parked nodes, invalid
topology/values, output ambiguity, explicit identity rejection, and cancellation.
M3-02 added the force/appearance cases: the closed-form trajectory is compared against
the analytic solution particle by particle, age curves are checked against their
interpolation, stage order and the single-appearance rule are enforced, a
four-stage graph is round-tripped through the codec, graph/flat pixels must still be
identical, and a tiny drag value exercises the series branch. Assertion totals include
repeated parameter combinations, not independent features.

AE arbitrary-data persistence and snapshot transport are implemented under G-04.
The CEP-to-ExtendScript bridge is specified in ADR 0009 and implemented in
`cep_panel/` (P-02). No AE host qualification is implied by the core tests, the
adapter suite, or this ADR: panel behaviour, persistence and undo remain host gates.
