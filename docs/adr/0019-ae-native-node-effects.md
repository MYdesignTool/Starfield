# ADR 0019: AE-native effect instances own node records

- Status: accepted; Output stays on the main renderer and editable nodes are separate hidden AEX instances. Source integration stores node records on per-node effects and uses a numeric compile trigger. The failed expression mailbox at parameter index 90 has been removed from the active parameter schema. The current candidate builds with the May 2023 SDK but has not yet been qualified in AE 2023.
- Date: 2026-09-30; implementation correction: 2026-10-01.
- Depends on ADRs 0006–0013 and 0015.

## Context

The owner reports that node addition and removal still do not work in the current
CEP graph editor. Thin node AEX modules exist, but the current panel treats them as
replicas of a graph snapshot owned by the renderer. It checks whether parameter 90
on the main effect accepts an expression before adding any node; AE 2023 reports
that it does not. The panel therefore cannot add a node. ADR 0013's expression
mailbox is rejected for topology authoring.

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
   fixed terminal's cap stays in the main effect's output-wide controls. Editable
   node modules own their typed values, UUID, outgoing connections, and canvas
   position. The main effect stores the compiled render snapshot. Store the 128-bit
   identity as eight exact 16-bit chunks in script-visible scalar streams, not
   four 32-bit values that exceed the exact integer range of a 32-bit float.
   The graph compiler maps node effects to core UUIDs; it must not use display names, effect
   indices, or session-only stream IDs as persistent IDs. Duplicating a node
   allocates a fresh identity and follows the link policy below; AE host behavior
   still must be qualified before Ctrl+D is treated as accepted for node effects.
3. **Compile before rendering.** A host-side graph synchronizer enumerates node
   effect instances on the UI/edit path, reads their standard parameter streams,
   validates the resulting graph, and writes an immutable compiled snapshot to
   the renderer's AE-owned graph parameter. CEP writes ordinary AE node streams,
   then changes a supervised numeric compile trigger on the renderer. The same
   trigger handles edits made in Effect Controls. It carries no graph bytes and
   requires no expression support. The renderer continues to consume
   only its own checked-out snapshot during SmartFX pre-render/render. It never
   queries sibling effects from a render callback.
4. **Track every edit.** Adding/removing/reordering node effects, editing a node
   parameter in Effect Controls, panel edits, undo/redo, duplication, copy/paste,
   and project reopen must leave the node records and renderer snapshot
   synchronized. The node edits and compile trigger must participate in AE's undo and
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

### Node record streams and synchronization

Each node module stores its UUID in eight exact 16-bit chunks, two hidden
non-animated layout scalars, an outgoing-connection count, and four fixed outgoing
connection slots. Each slot stores the destination UUID in eight 16-bit chunks.
Four outgoing slots per node allow 16,380 edges across 4,095 editable nodes and
the fixed Output terminal. The source and destination ports are inferred from the v1 node types;
the graph compiler validates every resulting edge. A future multi-port node schema
must append explicit port fields and update the node schema version.

On a node parameter edit, the node module asks the renderer to run its supervised
numeric compile trigger. CEP graph edits write node values, connections, and layout
in one undo group, reacquire Effect Parade references after structural changes,
then trigger one compile. The renderer enumerates sibling node effects and reads
their ordinary streams only in that edit callback. It serializes a new graph into
its arbitrary-data parameter. Render and pre-render never query sibling effects.
AE 2023 must still qualify callback delivery, parameter reads, cache invalidation,
and undo behavior.

The old gateway stopped before modifying the Effect Parade because index 90 was
non-expressionable. The current source no longer registers that request stream.
It writes node values, links, and positions
to the separate node streams, then raises `Commit Graph Edit` as a numeric
trigger. It checks the renderer's new snapshot against the planned graph. This
source path builds with the May 2023 SDK, but AE 2023 callback delivery, snapshot
round-trip, undo, render invalidation, and save/reopen still require host testing.

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
Before pruning, the client rereads the graph and requires the revision to match
the revision returned by the native-effect inspection; the graph commit then
performs its own revision check. A stale inspection cannot delete from a newer
graph state.

Direct Effect Parade deletion and the resulting graph cleanup are separate AE
operations, so this source design does not yet claim single-step undo for a
manual deletion. The CEP transaction client now keeps a session-local guard for
each reconciled deletion. If undo restores the graph node while its AE effect is
still missing, the panel surfaces `native_node_undo_conflict` and pauses
automatic pruning for that node; restoring the AE effect clears the guard. This
prevents refresh from repeatedly consuming an undo. The guard is transient, so
the graph/effect pair must be coherent before closing or reloading the panel.
AE 2023 qualification must still check the undo stack, refresh after undo/redo,
and save/reopen.

Particle and Appearance modules store bounded Size/Opacity curve banks on their
own effect instances. The compiler copies those values into the renderer snapshot.
The main effect's flat legacy controls are hidden while node effects are the
authoring surface. Curves remain host-unqualified along with the other node data.

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

Revision 16 uses main sync guard 42 for Output cap/position batches, numeric commit
43 and receipt 44, plus ready marker 89. Removing all node effects after bootstrap
compiles an Output-only transparent graph. CEP rollback restores both authoring
records and the compiled renderer snapshot. Snapshot acknowledgement compares graph
semantics with AE float/color quantization tolerance instead of exact byte equality.
Origin values map between graph world units and full-resolution node point streams.
Callback timing and actual rendered origin parity remain host gates.

The 2026-10-01 build-2 attempt crashed during fresh apply/viewer opening, before node
operations. Build 3 is installed with synchronized PiPL/runtime build metadata and
cleared group-end definitions; it has not been exercised in AE. The crash fix is
unconfirmed. See `docs/native-node-checkpoint.md` for evidence and remaining work.

The expression-mailbox experiment is rejected: AE 2023.5 Build 52 reports
parameter index 90 as non-expressionable, and CEP's preflight stopped before
creating any node effect. The current source removes that parameter entirely. A
node control edit invokes the main renderer with `AEGP_EffectCallGeneric` and
the numeric `Commit Graph Edit` trigger at index 43. CEP batches node value and
metadata writes under a per-node sync guard, then raises one compile trigger.
The renderer enumerates sibling streams only in that edit callback; rendering
uses the renderer-owned arbitrary-data snapshot. Callback delivery, cache
invalidation, undo grouping, and reopen behavior are still AE 2023 gates.

The CEP loader probes the gateway first, then explicitly evaluates the JSX file
from the active extension folder. A failed load now returns the file-load result
or error in the panel banner. This is diagnostic behavior only; the updated loader
and native node flow still need a fresh AE 2023 session to establish host behavior.

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
