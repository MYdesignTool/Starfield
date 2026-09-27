# ADR 0006: graph identities, node schemas, and validation

- Status: accepted for the graph foundation (G-01).
- Date: 2026-09-27.
- Depends on: ADR 0001 identity rules and `schema/sequence-format.md`.

## Decision

- Nodes and edges have independent 128-bit UUID identity types. Identity never comes from a vector index, AE parameter ID, port key, or panel widget.
- Port keys and node parameter keys are separate strong 64-bit types. They are scoped to a node type; they are not AE parameter IDs.
- Node and port type keys use lowercase reverse-DNS strings. Node schema versions are explicit. The registry owns port direction, port type, connection cardinality, required inputs, parameter value kinds, and cycle-breaking inputs.
- Parameter values are host-independent: bool, signed/unsigned 32-bit integer, finite float64, finite vec3, UTF-8 text without NUL, or opaque bytes.
- Validation is bounded by the sequence format's 4096-node, 16384-edge, and 64 MiB limits. It returns typed errors, rejects unsupported schemas, duplicate identities/keys, invalid endpoints, type/direction mismatch, missing required inputs/parameters, excess connections, non-finite values, and ordinary cycles.
- A node schema may name input ports that break same-frame dependency. Edges entering those ports are treated as delayed state updates for cycle detection. No other cycle is accepted.
- The initial built-in node types are `org.starfieldfx.nodes.emitter` and `org.starfieldfx.nodes.output`, connected by `org.starfieldfx.types.particle-stream`. Emitter parameter keys 1–11 describe the current particle settings; these are graph keys, not AE IDs.

## Consequences

Validation is independent of node and edge vector order and does not evaluate node behavior. The renderer must only consume validated immutable graph snapshots after G-03/G-04 integrate this model. This decision does not implement sequence serialization, migrations, AE persistence, the editor, or graph evaluation; those remain G-02 through G-04 and P-01/P-02.

The error result identifies the affected node, edge, port, or parameter where one exists. Allocation failure is reported as a typed validation failure. Traversals are iterative and size-bounded so a project-controlled graph cannot create unbounded recursion or work.
