# ADR 0019: AE-native effect instances own node records

## Build 6 flags/selector correction — 2026-10-01

Owner AE evidence rejects float-aware internal effects without SmartFX. Build 6
implements `PF_Cmd_SMART_PRE_RENDER` and `PF_Cmd_SMART_RENDER` in the shared node
module, forwarding metadata/requested pixels and pairing pixel checkout/checkin.
World Transform suite copy provides host-depth passthrough. Node PiPL/runtime
flags2 both become `0x00001400`; packed version is `32774` (`0x8006`). This is an
implementation-contract fix with no public parameter or node-record migration.
Nodes remain independent saved effects, hidden from Effects menus. Main remains
the sole particle renderer and Output owner. MFR/GPU flags stay disabled.
276 actual-node fake-host checks and SDK compilation pass; build 6 is deployed
under specific owner authorization and hashes verified. Particle creation/
32-bpc host acceptance is still pending. CEP token is `native-node-sync-6`.

## CEP 5b qualification update — 2026-10-01

The owner confirms Emitter duplication; Particle is still absent by default and
cannot be added. Independent node effects remain the architecture. CEP 5b adds
persistent mutation/bootstrap diagnostics and stops repeated failing bootstrap
mutations on background reads. Actual Particle creation failure is not yet
known; no AEX workaround or schema change is justified by static inspection.
See [the checkpoint](../native-node-checkpoint.md) for the focused checks and the
single next host action. Gateway token is `native-node-sync-5b`.

## CEP 5a hotfix after owner feedback — 2026-10-01

The owner reports build 5 creates native Emitter effects but repeatedly shows
`stale_graph`; copied effects do not appear in the canvas, and reopening CEP
loses the canvas. **Build 5 did not pass native node acceptance.**

The offending readonly `ensureNodeEffects` path compared a browser reconstruction
against the actual AE records and rejected reload itself. CEP 5a returns the
current Effect Parade records directly. Only mutation checks a host-produced
opaque authoring stamp, so decimal JSON/codec differences do not pretend that
the owner edited an effect. A targeted fixture reproduces rounded decimal JSON
and verifies a readonly reload performs no compile, copy/edit still works, and
a genuine intervening AE value change rejects before adding any effect.
The exact numerical mismatch in the owner's session was not captured; decimal
rounding is a reproduced hypothesis, not confirmed host evidence.

Numeric native payload CRCs remain diagnostic. A browser's rounded projection
cannot prove native byte equality; commit receipt, advancing native revision and
semantic node-record readback confirm a transaction. Additional supervised
callbacks may advance revision beyond exactly one. Failed graph reads keep the
last valid canvas and do not clear/re-show the error banner on every poll.

The gateway/loader token is `native-node-sync-5a`. This changes only workspace
CEP files through the existing extension junction: **no AEX replacement or AE
restart is needed**. Close/reopen the CEP panel. The actual effects are the source;
existing Emitter copies should become visible. If the Particle effect is absent,
add it from the node context menu and connect it. Partial first initialization now fills missing Emitter/Particle and initial links while ready=0; ready=1 deliberate deletion remains unchanged. Fresh-effect automatic bootstrap,
real render response, undo and reopen still require owner confirmation.


- Status: accepted; Output stays on the main renderer and editable nodes are separate hidden AEX instances. Source integration stores node records on per-node effects and uses a numeric compile trigger. All expression transport, including the read-only snapshot, is removed. Numeric revision/checksum streams acknowledge the compiled graph. Revision-18 build 5 is deployed; owner testing is pending. The owner confirmed build 4 fixed the layer-selection crash.
- Date: 2026-09-30; implementation correction: 2026-10-01.
- Depends on ADRs 0006–0013 and 0015.

### Expression snapshot removal — revision 18, 2026-10-01

Build 4 stopped the owner's selection crash, but graph initialization still failed:
AE reported `AEGP_CanVaryOverTime must be true to get an expression`. The native
snapshot publisher called `AEGP_GetExpression` on ID 41 even though that control
is non-time-varying. This happened before the gateway could create node effects.

Revision 18 removes every native and script expression read/write from graph
synchronization. ID 41 becomes Graph Revision (0–16777215); IDs 90/91 hold the
compiled payload CRC32 as two exact 16-bit numeric halves. IDs 43/44 retain
numeric commit/receipt. The CEP reads each effect's ordinary values, UUID,
outgoing edges and signed position, reconstructs the portable graph locally,
and checks it against the compiled checksum. It never reads CUSTOM_VALUE.

Node effects are the authoring source. The main arbitrary graph is a compiled
render cache; it is not a source from which missing node effects are recreated.
The gateway compares the caller's base record manifest as well as numeric
revision before mutations, so an external effect edit/reorder/deletion cannot
silently be overwritten. One guarded transaction creates/removes independent
effects, writes records, reacquires indexed-group references, and compiles once.

The first panel synchronization creates Emitter → Particle → Output directly.
After marker 89 is set, an empty Effect Parade stays Output-only. The refresh
path prunes connections to deleted effects. Raw AE duplicates are re-keyed in
parade order, retaining the original node identity and assigning fresh edge
identities to copies. These synchronization actions require the CEP panel;
automatic creation/import with the panel closed remains a separate host gate.

The adapter suite now links the actual GraphCarrier, rather than replacing its
publisher with a success stub. It checks numeric revision/checksum publication,
commit/rejection and rerender flags with no expression suite available.
Gateway checks make all expression access throw and exercise bootstrap,
add/copy, independent values and curves, signed movement, insert/connect/disconnect,
Effect Parade reorder/direct deletion, raw Ctrl+D re-key, rollback and delete-all.
These are source checks; AE acceptance belongs to the owner's fresh-effect test.

## Context

### Selection crash correction — revision 17, 2026-10-01

The owner confirmed build 3 renders, but selecting the effect layer crashes AE.
Dump `cf077068-7651-46c6-a82c-4f8d4e451f8e` repeats the null read at
`AfterFXLib.dll+0x1931d36`; only the main AEX and Core are loaded, before node
module materialization. Source registration hid five GROUP_START parameters
while registering visible GROUP_END markers. This is a suspected ECW hierarchy
defect; the owner later confirmed build 4 no longer crashes on selection. Revision 17 replaces these ten unused markers
with ordinary hidden scalar slots; Output is the renderer's only structural group.
The compiled node architecture is unchanged. Code/PiPL both advance to `0x8004`.
Development effects must be recreated; no type migration is provided. Adapter
checks require one balanced visible group and no hidden structural group markers.

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

The main renderer's project-owned marker 89 distinguishes initial bootstrap from
an intentionally empty graph. Initial synchronization creates separate Emitter
and Particle effects and connects them to the existing Output. Later refreshes
read the actual Effect Parade. A deleted effect disappears from the graph and
its incoming links are pruned before recompilation; no snapshot recreates it.

Manual native deletion and the follow-up edge cleanup may occupy separate undo
steps. An undo restores the actual saved node records, which the panel rereads;
there is no session-local deletion tombstone or mirror-based pruning loop.
AE undo/redo, refresh and save/reopen remain host qualification gates.

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

The CEP Duplicate command creates new graph UUIDs and independent AE effects.
Raw AE-level duplicates are detected on refresh and re-keyed in Effect Parade
order; the original retains its identity and incoming links. The duplicate gets
new node/edge IDs and a small position offset. Script checks cover this policy;
native host undo/cache response still needs qualification.

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
creating any node effect. The expression request is removed; numeric checksum halves now occupy IDs 90/91. A
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
