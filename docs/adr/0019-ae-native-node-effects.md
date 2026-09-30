# ADR 0019: AE-native effect instances own node records

- Status: accepted; source integration keeps Output on the main renderer and omits an Output AEX. AE 2023 synchronization acceptance is pending. The gateway rejects duplicate native node UUIDs, removes all same-UUID effects when deleting a graph node, and can reconcile direct Effect Parade deletion after initial node materialization.
- Date: 2026-09-30.
- Depends on ADRs 0006–0013 and 0015.

## Context

The owner reports that node addition and removal still do not work in the current
CEP graph editor. The current editor stores the entire topology in one graph
snapshot on the renderer effect and edits that snapshot through ADR 0013's
expression carrier. That carrier is only source-integrated; its AE 2023 callback,
undo, persistence, and topology-edit path have not passed host acceptance.

The project's observed Stardust parameter inventory records one main rendering
effect and nineteen separate control-effect modules, including Emitter and
Particle. Control-effect instances have a `uid` field, and the main effect owns
the mapping. This is evidence of the user-visible AE architecture, not a source
or binary implementation to reuse. See `docs/reference-parameter-map.md`.

AE scripting can add and remove effects from a layer's Effect Parade. Adding to
an indexed group invalidates existing script references, so the panel must keep
indices and reacquire property references after each structural edit. AE's C++
Effect Suite can enumerate layer effects and Stream Suite can read their
parameters. Adobe warns that an effect must not let untracked AEGP queries
control rendering, because AE may not invalidate cached output when those
external values change. AE's unique stream ID is session-unique, so it is not a
project-persistent node ID.

## Decision

1. **Editable nodes are internal AE effect instances.** Provide distinct thin AE
   modules for each editable node family, with `PF_OutFlag_I_AM_OBSOLETE` so they
   are not listed as standalone choices in the Effects menu. CEP creates and
   removes Emitter, Particle, Force, and later node instances by stable match
   name in the layer's Effect Parade. Each instance owns ordinary AE parameter
   streams, so repeated nodes keep independent saved values and native AE
   undo/copy behavior. AE 2023 must confirm that the hidden modules remain
   script-addable and that saved instances reopen normally.
2. **The existing Starfield Particle effect is the Output.** Do not add a
   separate Output effect instance. Keep one fixed, visible Output terminal in
   the CEP graph and bind it to the layer's main Starfield Particle render
   effect. This terminal is the graph-facing representation of that effect: it
   participates in connections and graph evaluation, but is never created,
   duplicated, or removed as an independent AE effect. Global render controls
   such as Max Particles remain owned by the main effect and are exposed from
   the Output inspector; they are not copied into emitter node effects. The
   fixed terminal stores its cap in the graph snapshot owned by the main effect.
   The graph compiler combines those global controls with editable node records
   when it builds the snapshot. The main effect owns the graph topology, edges,
   layout, and compiled graph snapshot. Editable node
   modules own their node values and persisted identities. Store the 128-bit
   identity as eight exact 16-bit chunks in script-visible scalar streams, not
   four 32-bit values that exceed the exact integer range of a 32-bit float.
   The graph compiler maps node effects to core UUIDs; it must not use display names, effect
   indices, or session-only stream IDs as persistent IDs. Duplicating a node
   allocates a fresh identity and follows the link policy below; AE host behavior
   still must be qualified before Ctrl+D is treated as accepted for node effects.
3. **Compile before rendering.** A host-side graph synchronizer enumerates node
   effect instances on the UI/edit path, reads their standard parameter streams,
   validates the resulting graph, and writes an immutable compiled snapshot to
   the renderer's AE-owned graph parameter. The renderer continues to consume
   only its own checked-out snapshot during SmartFX pre-render/render. It never
   queries sibling effects from a render callback.
4. **Track every edit.** Adding/removing/reordering node effects, editing a node
   parameter in Effect Controls, panel edits, undo/redo, duplication, copy/paste,
   and project reopen must leave the node records and renderer snapshot
   synchronized. The synchronization write must participate in AE's undo and
   cache invalidation model. A stale or invalid graph must fail visibly without
   rendering stale particles.
5. **Keep the runtime hot path.** The main renderer and thin node AEX modules share the
   reloadable `StarfieldCore.dll`. Core algorithm changes remain Core-only
   builds; only changes to an AE module's parameter/UI contract require replacing
   that module.
6. **No development-era migration burden.** Existing single-effect graph
   snapshots are not a released project format. This redesign may start with a
   fresh development schema; migration from the current in-development graph
   format is not required.

### Initial materialization and direct Effect Parade deletion

The main renderer appends hidden, non-animated parameter ID 89, `Node Effects
Ready`, to the project. Its default value is 0. During the first successful
panel synchronization, the gateway creates any native node effects missing from
the graph manifest and sets the marker to 1 in the same AE undo group. This
distinguishes a new project that still needs bootstrap from a later deliberate
deletion in Effect Controls; without project-owned state, every refresh would
recreate manually deleted nodes.

After the marker is 1, the gateway reports graph node IDs whose corresponding
native effect instances are missing. The CEP transaction client then submits a
revision-checked `deleteNodes` edit against the main effect's saved graph. The
graph edit also removes incident edges. This reverse reconciliation runs on the
panel refresh path and does not query sibling effects from rendering callbacks.

Direct Effect Parade deletion and the resulting graph cleanup are separate AE
operations, so this source design does not yet claim single-step undo for a
manual deletion. AE 2023 qualification must check the undo stack, refresh after
undo/redo, and save/reopen; if undo causes a missing node to be pruned again,
reconciliation needs an explicit conflict state instead of silently retrying.

P-02C's Size/Opacity curves and other non-scalar node values must also end up
owned by the corresponding Particle effect. The prototype may start with scalar
fields, but P-02B must not resume until the curve payload has a bounded,
project-persisted per-node representation and the renderer snapshot compiles it
without leaving a second editable source of truth.

### Node duplication link policy

- Emitter copies are always disconnected so duplication cannot add a second
  active emission source. If an Emitter and its Particle are copied together,
  the copied Particle remains connected to the original Emitter.
- Particle and Force copies retain compatible incoming and outgoing links to
  unselected nodes. Links between selected nodes are redirected to their copies.
  Edges touching an Appearance are not copied because multiple Appearance
  overrides on one stream remain outside the graph contract. This lets a copied
  Particle branch reach the existing Output and lets compatible Force copies
  preserve their stream position.
- Appearance copies are disconnected. Multiple Appearance overrides on one
  stream have no defined precedence, so users must explicitly reconnect a copy
  after deciding how that branch should be evaluated.
- Alt-drag and Ctrl+D use the same graph edit and link policy. CEP planner
  coverage does not qualify AE effect creation, undo, cache update, or reopen.

The CEP graph's Duplicate command creates new graph UUIDs and materializes new AE
effects. If an effect is copied directly in Effect Parade, the copied UUID makes
the graph-to-effect mapping ambiguous. The current gateway detects this and blocks
edits against that identity; deleting the graph node removes every effect carrying
that UUID. Automatic import/re-key of raw AE-level duplicates remains open.

Use one thin AEX code fragment per AE effect module. Adobe permits multiple
PiPLs in a single file for After Effects, but recommends one effect per code
fragment; separate modules also give each effect type a stable match name and
independent parameter schema.

## Synchronization spike

Before replacing P-02B's transaction plumbing, prove one Emitter module and one
Particle module as a narrow AE 2023 prototype alongside the existing Starfield
Particle renderer. The source candidate routes a node's
`PF_Cmd_USER_CHANGED_PARAM` through the existing hidden request-expression stream
and calls the main renderer's `PF_Cmd_USER_CHANGED_PARAM` with
`AEGP_EffectCallGeneric`. The renderer then updates its own arbitrary graph data
and snapshot. The panel raises a hidden per-node guard while it writes a batch
of scripted node controls, so intermediate values do not trigger individual
commits. The renderer recognizes these zero-nonce node requests before checking
the graph transaction slider, because that slider retains the last CEP nonce.
This is only a source-level hypothesis until AE 2023 confirms callback
delivery, cache refresh, and undo behavior.

Node-effect values currently serialize as constants. Their editable controls
are marked non-time-varying until graph animation has a dedicated contract;
keyframe and expression evaluation is not part of this synchronization spike.

The spike is successful only when all of these work in AE 2023:

- The panel can add two Particle instances and remove either one. Adding a node
  changes the Effect Controls stack immediately; adding a second of the same
  type keeps separate parameter values. The graph always displays one Output
  terminal bound to the existing main render effect; no Output effect is added
  to the Effect Controls stack.
- The AE Effects menu offers only the main Starfield renderer; CEP can still add
  each hidden node module by match name.
- Changing either node's native AE controls or CEP inspector parameters updates
  the rendered result without pressing Refresh or reopening the panel.
- One edit plus its compiled graph snapshot is one undo step. Undo/redo, Ctrl+D,
  same-name copy/paste, and save/close/reopen keep identities, links, values, and
  rendered output coherent.
- Duplicating an effect instance cannot leave two nodes with the same persistent
  ID or silently redirect existing graph edges.
- Render and pre-render read only the main effect's owned snapshot; no AEGP
  enumeration of peer effects occurs on the render path.
- The same graph bytes survive reopen and the panel reconstructs editable node
  instances plus the fixed Output terminal from the project without an
  incidental manual Refresh.
- After the first materialization, deleting a native node effect from Effect
  Controls removes the matching graph node and incident edges. Test deletion,
  undo/redo, panel refresh, and save/reopen as separate host gates before calling
  reverse synchronization accepted.

If the callback cannot update the main effect's graph snapshot atomically with an edit,
implement a native graph coordinator that explicitly tracks AE undo and cache
invalidation before resuming general graph operations. Do not fall back to
render-time sibling-effect queries.

## References

- `docs/reference-parameter-map.md` — observed module inventory and `uid` fields.
- [After Effects Scripting Guide: PropertyGroup](https://ae-scripting.docsforadobe.dev/property/propertygroup/) — adding effects to Effect Parade and reference invalidation.
- [After Effects Scripting Guide: PropertyBase](https://ae-scripting.docsforadobe.dev/property/propertybase/) — removing indexed-group children.
- [After Effects C++ SDK Guide: AEGP Effect Suite](https://ae-plugins.docsforadobe.dev/aegps/aegp-suites/) — effect enumeration, parameter streams, and session-unique stream IDs.
- [After Effects C++ SDK Guide: Effect use of AEGP suites](https://ae-plugins.docsforadobe.dev/aegps/cheating-effect-usage-of-aegp-suites/) — cache dependency warning.
- [After Effects C++ SDK Guide: PiPL Resources](https://ae-plugins.docsforadobe.dev/intro/pipl-resources/) — multiple PiPL behavior and one-effect-per-module recommendation.
- [After Effects C++ SDK Guide: PF_OutData](https://ae-plugins.docsforadobe.dev/effect-basics/PF_OutData/) — `PF_OutFlag_I_AM_OBSOLETE` keeps an effect usable in existing projects while omitting it from the Effects menu.
- ADR 0011 still governs every install, process, or other host-level change.
