# ADR 0007: deterministic evaluation of graph snapshots

- Status: accepted for G-03's runtime, extended by M3-02 and G-05 (ADR 0015).
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
not allocate particle streams. If the Output ancestry has no emitter, evaluation
succeeds with an empty particle stream, so a temporarily disconnected or rewired
graph renders transparent black. This does not fall back to flat AE controls.
New graphs use one active emitter and one or more
Particle nodes directly connected to it. Their age curves define distinct output
looks, and the emitter's global slots are deterministically partitioned across those
Particle nodes. Force nodes may form serial chains and parallel DAG branches; each
reachable force contributes once per Particle stream, and parallel paths merge by
particle identity. Current gravity and linear drag values are accumulated in stable
dependency order and integrated once. At most one downstream Appearance override
may affect each Particle stream. Multiple active emitters and ambiguous appearance
merges remain errors. The complete branch and compatibility rules are in ADR 0015.

Previously stored schema-1 graphs with an active emitter and no active Particle
node in the output ancestry keep the prior single-stream emitter/force/appearance interpretation.
This compatibility path preserves the AE capture graph without rewriting arbitrary
data or changing sequence schema. New graph constructors and topology transactions
must create a Particle node. `make_emitter_output_graph` remains explicitly named
as a legacy compatibility constructor.

Time enters as a signed rational, is normalized, and converts to seconds only at
the simulation boundary. Schema-1 node parameters are constant values: this work
does not introduce keyframes, animated parameter sampling, or emission history.
Those require explicit time-sampling and serialization contracts under M3-03.
No playback history, host handle, mutable render global, or filesystem access is
introduced. Cancellation is checked before staging allocation, during graph
planning/execution and particle simulation, on each source-composite and output
encode row, and at the AE world-copy boundary. The adapter polls `PF_ABORT` while
copying both input and output worlds.

The CPU rasterizer consumes each particle's opacity, rather than the fallback
settings' opacity. This preserves flat-path pixels and allows later appearance
nodes to modify per-particle opacity without changing the rasterizer contract.

`make_emitter_output_graph` maps all eleven settings fields to stable graph keys
for legacy compatibility. `make_emitter_particle_output_graph` constructs the new
explicit Particle branch with the current appearance settings.
`make_emitter_force_appearance_output_graph` does the same for the full chain, writing
the gravity/drag fields to the force node and the color/size/opacity curves to the
appearance node (not to the emitter). Callers supply the node/edge identities; neither
helper derives them from AE parameter IDs, and duplicate or zero identities are
rejected. The graph parameter keys are scoped per node type, so force and appearance
reuse small local key ranges without aliasing emitter parameters. These are
construction primitives, not an AE migration or persistence implementation.

## Evidence and remaining work

Core regression run on 2026-09-28: 6,180 checks, zero failures (the suite has since
grown to 6,196 checks with the H-01 C-ABI and loader cases). Added cases
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

G-05 source implementation was added on 2026-09-29: explicit Particle branches,
global-slot partitioning, branch-local force accumulation, fan-in deduplication,
and a bounded traversal budget. All portable core translation units compiled with
MSVC x64 `/std:c++20 /W4 /permissive- /EHsc` and linked into a static core library;
the regression suite was not run, and the graph behavior has not been host-qualified.

AE arbitrary-data persistence and snapshot transport are implemented under G-04.
The CEP-to-ExtendScript bridge is specified in ADR 0009 and implemented in
`cep_panel/` (P-02). No AE host qualification is implied by the core tests, the
adapter suite, or this ADR: panel behaviour, persistence and undo remain host gates.
