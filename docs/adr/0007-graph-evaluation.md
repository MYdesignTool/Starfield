# ADR 0007: deterministic evaluation of graph snapshots

- Status: accepted for G-03's emitter/output runtime.
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
reordering or modifying the graph. Disconnected valid emitters remain editable but
do not allocate particle streams. Current schemas permit emitter -> output only;
the current evaluator supports one active stream. Future force/appearance kernels
must explicitly implement their behavior, and merge/multiple-output semantics need
a contract before they can be enabled.

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
Callers supply the node/edge identities; the helper does not derive them from AE
parameter IDs. It validates inputs and rejects duplicate or zero identities.
It is a construction primitive, not an AE migration or persistence implementation.

## Evidence and remaining work

Core regression run on 2026-09-27: 4,418 assertions, zero failures. Added cases
compare graph/flat pixels across four emitter shapes, 8/16/32-bit formats, repeated
and reverse times, negative/subframe time, and reduced-resolution cropped output.
Cases also cover unchanged graph bytes after evaluation, parked nodes, invalid
topology/values, output ambiguity, explicit identity rejection, and cancellation.
Assertion totals include repeated parameter combinations, not independent features.

AE arbitrary-data persistence and snapshot transport are implemented under G-04;
the editor protocol and panel remain P-01/P-02. No AE host qualification is implied
by the core tests or this ADR.
