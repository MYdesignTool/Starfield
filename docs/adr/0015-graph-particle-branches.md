# ADR 0015: Particle branches and force-field merges

- Status: accepted for G-05 core evaluation; the portable core compiles and links on 2026-09-29. Regression and AE host qualification remain open.
- Date: 2026-09-29.
- Depends on: ADRs 0006–0008 and the M3-01/M3-02 kernels.

## Decision

### Particle node and branch allocation

- Add `org.starfieldfx.nodes.particle`, schema 1, on the existing particle-stream type. It has one required input from the active emitter and an unbounded output. Its six required values are the start/end RGB, size, and opacity curve values already used by the Appearance node. Values are scoped to the Particle node type and do not change the sequence codec or AE parameter IDs.
- A newly authored graph uses one active emitter and at least one active Particle node directly connected to that emitter. Multiple emitters feeding one output remain unsupported; each Particle branch is one distinct visual particle type from the same emitter.
- Sort active Particle nodes by `NodeId`. The emitter still owns the global birth rate, lifetime, seed, and live-particle cap. Global emission slot `k` belongs to branch `k mod N`, where `N` is the number of active Particle nodes. IDs and birth times remain the emitter's global slot values. Every contiguous group of `N` slots gives one particle to each branch; a partial group assigns its remainder by wrapping from the first slot's modulo index. Allocation is deterministic and branch populations differ by at most one for a contiguous live-slot interval. The total particle count and emission timing do not change.
- A Particle node applies its own linear age curves to its assigned particles. One downstream Appearance node may override those values for that Particle stream, preserving the existing modifier behavior. More than one active Appearance node affecting the same Particle stream is ambiguous and rejected.
- In explicit Particle mode, every active Force or Appearance path must descend from an active Particle node. Active bypass edges such as Emitter -> Force -> Output or Emitter -> Appearance -> Output are rejected as invalid graph connections; they are never silently omitted from evaluation. Fully isolated non-output nodes may be staged without required input edges and remain inert until connected; partially connected nodes must satisfy every required input.

### Force chains and parallel branches

- Force input ports accept fan-in. A stream may pass through forces in series, split into parallel force paths, or converge again before Appearance/Output. A Force contributes once to each Particle stream that can reach it and is included once when parallel paths converge; merging paths never duplicates particles.
- For a given Particle stream, collect each reachable Force node once in stable dependency order: serial nodes follow their graph dependencies, and independent parallel nodes use the stable UUID tie-break from the graph traversal. The current force kernel represents uniform gravity and linear drag fields. Their vectors/scalars are accumulated in that order and integrated once with the existing closed-form solver. Parallel paths therefore sum the field values; current gravity/drag values are additive, so path shape affects dependency and floating-point accumulation order, not the underlying physical field. Future non-additive force kernels need a separate composition contract.
- A Particle stream reaching Output through multiple paths is merged by identity, not concatenated repeatedly. The output particle list is ordered by the original global slot ID, independent of graph vector order. Evaluation sizes that list once from the bounded live-slot interval, writes each modulo branch directly into its global-ID position, and does not sort the completed list.

### Determinism, bounds, and compatibility

- Exactly one Output and one active Emitter are required. Disconnected nodes do not execute. Cycles, invalid edge stages, and a Force/Appearance path with no Particle source are rejected. When the active output ancestry has no Particle node, it uses the legacy compatibility path; disconnected Particle nodes remain inert.
- Branch traversal is iterative and cancellation-aware. Total node/edge visits across all active Particle branches are capped at 16,777,216; a graph exceeding this evaluation budget fails with `invalid_request`. The graph's existing node, edge, payload, settings, and particle-count bounds remain in force.
- Existing schema-1 graphs whose active output ancestry has no Particle node continue through the prior single-stream emitter/force/appearance path as a compatibility interpretation. The new `make_emitter_particle_output_graph` helper and future topology-edit transactions create an explicit Particle node; `make_emitter_output_graph` remains a named legacy compatibility constructor. This avoids rewriting AE-owned graph bytes or changing the sequence schema in G-05.
- Multiple active emitters, stochastic branch allocation, multiple Appearance overrides for one stream, and non-additive force merging are deferred. No CEP topology gesture or AE host support is implied by this core contract.

## Consequences

G-05 may extend the core node registry and evaluator without changing sequence schema 1, graph identity rules, AE parameter IDs, or the SmartFX render boundary. G-04 legacy-control capture continues to use its existing schema-1 graph and is evaluated through the compatibility path. P-02B must use the Particle schema and preserve these branch rules when its graph snapshot/submit path becomes qualified.

The evaluator resolves Particle-versus-Appearance precedence once per output particle: a downstream Appearance curve replaces the Particle curve as a whole. It does not evaluate the Particle curve first and overwrite its size, opacity, and color afterward.

Core build or static source review does not qualify AE persistence, undo, or panel editing. Those remain separate host gates.
