# ADR 0006: graph identities, node schemas, and validation

- Status: accepted for the graph foundation (G-01), extended for G-05 by ADR 0015.
- Date: 2026-09-27.
- Depends on: ADR 0001 identity rules and `schema/sequence-format.md`.

## Decision

- Nodes and edges have independent 128-bit UUID identity types. Identity never comes from a vector index, AE parameter ID, port key, or panel widget.
- Port keys and node parameter keys are separate strong 64-bit types. They are scoped to a node type; they are not AE parameter IDs.
- Node and port type keys use lowercase reverse-DNS strings. Node schema versions are explicit. The registry owns port direction, port type, connection cardinality, required inputs, parameter value kinds, and cycle-breaking inputs.
- Parameter values are host-independent: bool, signed/unsigned 32-bit integer, finite float64, finite vec3, UTF-8 text without NUL, or opaque bytes.
- Validation is bounded by the sequence format's 4096-node, 16384-edge, and 64 MiB limits. It returns typed errors and rejects unsupported schemas, duplicate identities/keys, invalid endpoints, type/direction mismatch, excess connections, missing required parameters, non-finite values, and ordinary cycles. Required input ports describe the complete evaluation contract, but incomplete and disconnected topology is valid editor state, including a temporarily disconnected Output. This lets a user remove or reroute one edge in a single edit. Evaluation renders transparent output while the active Output ancestry has no emitter; other invalid active semantics still return an error (ADR 0007).
- A node schema may name input ports that break same-frame dependency. Edges entering those ports are treated as delayed state updates for cycle detection. No other cycle is accepted.
- Built-in node types are `org.starfieldfx.nodes.emitter`, `org.starfieldfx.nodes.particle`, `org.starfieldfx.nodes.force`, `org.starfieldfx.nodes.appearance`, and `org.starfieldfx.nodes.output`, connected by `org.starfieldfx.types.particle-stream`. Emitter parameter keys 1–21 describe emission settings (keys 19–21 are optional per-axis direct pixel dimensions defined by ADR 0017); Particle and Appearance use type-scoped age-curve keys. These are graph keys, not AE IDs. Particle branches and force fan-in are specified in ADR 0015.

## Consequences

Validation is independent of node and edge vector order and does not evaluate node behavior. G-02 implements the schema-1 core codec against these types; G-03 evaluates immutable graph snapshots (ADR 0007), with G-05 branch behavior defined by ADR 0015. G-04 stores snapshots in an AE arbitrary-data parameter and supports explicit capture of current legacy controls (ADR 0008). The editor remains P-01/P-02 work.

The error result identifies the affected node, edge, port, or parameter where one exists. Allocation failure is reported as a typed validation failure. Traversals are iterative and size-bounded so a project-controlled graph cannot create unbounded recursion or work.
