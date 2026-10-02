# ADR 0015: Particle branches and force-field merges

- Status: accepted for G-05/G-06 core evaluation; build-10 SDK compilation passes. Multi-emitter AE qualification remains open.
- Date: 2026-09-29; multi-emitter amendment 2026-10-02.
- Depends on: ADRs 0006–0008 and the M3-01/M3-02 kernels.

## Decision

### Particle node and branch allocation

- Add `org.starfieldfx.nodes.particle`, schema 2, on the existing particle-stream type. It has one required input from the active emitter and an unbounded output. Its six required appearance values are start/end RGB, base Size in pixels, Size Over Life's final percentage, base Opacity in 0…1, and Opacity Over Life's final percentage. The two over-life curves use 0…100% ordinates that multiply their matching base values. Required key 11 stores that branch's Lifetime in seconds. Values are scoped to the Particle node type and do not change the sequence codec or AE parameter IDs.
- Advance `org.starfieldfx.nodes.emitter` to schema 3 and remove the Emitter Lifetime and Max Particles graph keys. Emitter keeps birth rate and seed; Particle lifetime belongs to each Particle branch.
- Advance `org.starfieldfx.nodes.output` to schema 2 and store the required global Max Particles cap there. Output is a fixed logical node in the graph and its data is persisted in the main Starfield Particle render effect; it has no separate AE effect instance.
- A newly authored graph initially uses one Emitter and Particle. Any number of active Emitters may feed the single Output through their own Particle branches. Each Particle has exactly one direct Emitter input; an Emitter without a connected active Particle produces no particles.
- For each Emitter, sort its active Particle children by `NodeId`. Birth slot `k` belongs to child `k mod N`, where `N` is that emitter's active child count. Each Emitter keeps its own birth rate and seed; each Particle keeps its own half-open lifetime. Particle identity is `(emitter UUID, emitter-local birth slot)`; the local slot continues to seed deterministic random streams. Standalone simulation has a zero emitter UUID. No C ABI or serialized graph change is needed for this internal identity field.
- Output owns one cap across all emitters. Build a strided live-slot sequence for every Particle branch using its actual lifetime, retaining at most the cap's newest assigned candidates. Merge sequences newest-first by computed birth time (`slot / birth_rate`), then Emitter UUID and local slot for ties; keep the globally newest capped population. Reverse selection for stable oldest-first compositing with ascending UUID/slot ties. Removing expired branch slots before selection prevents a short-lived branch from consuming the cap. This replaces the former longest-lifetime candidate interval during unreleased development; no project migration is needed.
- A Particle node applies its own linear age curves to its assigned particles. One downstream Appearance node may override those values for that Particle stream, preserving the existing modifier behavior. More than one active Appearance node affecting the same Particle stream is ambiguous and rejected.
- In explicit Particle mode, every active Force or Appearance path must descend from an active Particle node. Active bypass edges such as Emitter -> Force -> Output or Emitter -> Appearance -> Output are rejected as invalid graph connections; they are never silently omitted from evaluation. Fully isolated non-output nodes may be staged without required input edges and remain inert until connected; partially connected nodes must satisfy every required input.

### Force chains and parallel branches

- Force input ports accept fan-in. A stream may pass through forces in series, split into parallel force paths, or converge again before Appearance/Output. A Force contributes once to each Particle stream that can reach it and is included once when parallel paths converge; merging paths never duplicates particles.
- For a given Particle stream, collect each reachable Force node once in stable dependency order: serial nodes follow their graph dependencies, and independent parallel nodes use the stable UUID tie-break from the graph traversal. The current force kernel represents uniform gravity and linear drag fields. Their vectors/scalars are accumulated in that order and integrated once with the existing closed-form solver. Parallel paths therefore sum the field values; current gravity/drag values are additive, so path shape affects dependency and floating-point accumulation order, not the underlying physical field. Future non-additive force kernels need a separate composition contract.
- A Particle stream reaching Output through multiple paths is merged by identity, not concatenated repeatedly. Sequence cursors select each birth once. Evaluation sizes the final particle list once, groups selected slot targets by branch using counts/prefix offsets, and evaluates only selected births directly into their output positions. It allocates no per-emitter particle populations and does not sort the completed list. With one emitter the output remains in ascending local slot order.

### Determinism, bounds, and compatibility

- Exactly one Output is required. Incomplete required-input wiring is valid editor state; if the active Output ancestry has no Emitter, evaluation returns an empty stream and renders transparent black. An Emitter with no active Particle also produces an empty stream. Disconnected nodes do not execute. Cycles, invalid edge stages, and any active Force/Appearance path with no Particle source are rejected.
- Branch traversal is iterative and cancellation-aware. Total node/edge visits across all active Particle branches are capped at 16,777,216; a graph exceeding this evaluation budget fails with `invalid_request`. The graph's existing node, edge, payload, settings, and particle-count bounds remain in force.
- Population selection uses at most one cursor per active Particle branch and the global capped number of slot targets/particles. Additional population memory is O(cap + branches); selection is O(cap log(branches)), with no scan of expired slots or cap multiplied by emitter count. Planning, selection, simulation and appearance poll cancellation. The adapter returns normal host interrupts without filling `return_msg` (ADR 0005).
- Pre-release Emitter schema 1–2, Particle schema 1, and Output schema 1 snapshots are intentionally unsupported. This is a development-time graph contract change with no migration because the plug-in has not entered production use. The graph sequence envelope and AE parameter IDs remain unchanged.
- Stochastic branch allocation, multiple Appearance overrides for one stream, and non-additive force merging are deferred. No AE host support is implied by compilation of this core contract.

## Consequences

G-05 extends the core node registry and evaluator without changing the graph sequence envelope, graph identity rules, AE parameter IDs, or the SmartFX render boundary. Control capture constructs the current Emitter/Particle graph directly. P-02B preserves these branch rules in graph snapshot and edit transactions. The node editor displays Lifetime on Particle, alongside its Size, Opacity, and Color controls.

The evaluator resolves Particle-versus-Appearance precedence once per output particle: a downstream Appearance curve replaces the Particle curve as a whole. It does not evaluate the Particle curve first and overwrite its size, opacity, and color afterward.

Core build or static source review does not qualify AE persistence, undo, or panel editing. Those remain separate host gates.
