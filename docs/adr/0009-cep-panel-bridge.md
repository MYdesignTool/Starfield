# ADR 0009: CEP panel bridge through supervised AE parameters

- Status: protocol v1 accepted and implemented; graph topology editing is blocked on a qualified undoable graph transaction carrier.
- Date: 2026-09-27.
- Depends on ADRs 0006–0008 and the stable parameter identity rules in ADR 0001.

## Context

The dockable editor must read and edit the graph owned by an effect instance,
while AE remains responsible for project persistence, effect duplication, render
invalidation, and undo/redo. The graph is currently stored as a
`PF_Param_ARBITRARY_DATA` value. AE's AEGP stream API exposes arbitrary values for
reading, but its `AEGP_SetStreamValue` contract excludes opaque arbitrary blocks.
An AEGP generic effect call can deliver a command and payload, but the SDK sample
only demonstrates a read/response operation; it does not establish an undoable
write to an arbitrary parameter.

Adobe documents a supported route for this case: ordinary effect parameters can
be read and written through the scripting API, and an effect can supervise
ordinary parameters and update its arbitrary parameter in
`PF_Cmd_USER_CHANGED_PARAM`. The arbitrary graph remains the render-time source
of truth; the ordinary parameters are the panel's AE-managed edit surface.

## Decision

### Host and transport

- The first panel targets After Effects 2023 on Windows x64 and uses a dockable
  CEP extension.
- CEP calls a namespaced ExtendScript gateway through `CSInterface.evalScript`.
  The gateway uses only the public AE scripting DOM to resolve effects and
  standard parameter streams.
- Do not add a local socket, named pipe, helper process, shared-memory channel,
  or private C++ object ABI for panel communication.
- The panel never writes `PF_Param_ARBITRARY_DATA` directly. It sends parameter
  changes to supervised, script-visible AE streams. The effect validates those
  values, constructs and validates a new graph, serializes it, and assigns the
  arbitrary-data parameter with `PF_ChangeFlag_CHANGED_VALUE` during
  `PF_Cmd_USER_CHANGED_PARAM`.
- Panel-facing parameters must have real AE value streams. Do not use
  `PF_PUI_STD_CONTROL_ONLY` for those bindings: the SDK defines that flag as
  having no associated data stream. They may be hidden from the Effect Controls
  Window, but AE 2023 scripting access must be host-qualified before the panel
  relies on it.

### Protocol version 1

Requests and responses are JSON values. The envelope is:

```json
{
  "protocol": "org.starfieldfx.panel",
  "version": 1,
  "requestId": "caller-generated-id",
  "operation": "getState | getFrameStatus | setParameters | setNodeLayout",
  "target": { "token": "target-token-from-getState" },
  "baseRevision": "state-token",
  "changes": []
}
```

`getState` resolves the selected composition, layer, and Starfield effect,
returns an opaque target token, the fixed node/port/edge snapshot, editable
values, project-saved node positions, and a revision token. `setParameters` identifies bindings by stable
graph `NodeId` + `ParamKey`; AE parameter IDs remain the storage mapping and are
never treated as graph identities. Version 1 displays an Emitter → Particle →
Force → Output parameter view. It is a fixed projection, not a live snapshot of
the canonical graph. Its topology is read-only; parameter values and the
displayed nodes' saved positions are editable. The Particle card owns the size,
opacity, and color controls in this view.
`setNodeLayout` validates and writes all eight hidden layout coordinates in one AE
undo group (ADR 0014). Dynamic node creation, deletion,
reordering, and edge rewiring require a later protocol version.

Each parameter descriptor may include `min`, `max`, `displayDecimals`, and ordered
`choices` edit hints. The panel uses them to format values, populate AE-style popup
controls, and bound and scale horizontal numeric scrubbing. Direct edits are rounded
to `displayDecimals`; ordinary clicks still allow typing. A scrub gesture is
coalesced into one `setParameters` request on release, keeping the edit within one
host undo group. Shift-drag is ten times faster and Ctrl-drag is ten times finer.
The gateway's binding validation remains authoritative. Popup choices are ordered
and map to AE's one-based values.

`getFrameStatus` is a bounded read-only v1 operation. It samples the target comp's
current time and the emitter birth rate, lifetime, and global particle cap at that
time. The panel uses the same half-open lifetime interval and slot cap as the core
simulation to display the current live-particle total on Output. It polls this
small status at 200 ms while the panel is visible and automatic refresh is enabled;
the count can therefore lag a host time change by up to one poll interval. The
global `particle_count` AE parameter keeps its existing identity and render
semantics, while its CEP control is grouped under Output for the node-editor UI.
Because v1 does not expose canonical graph values, the counter reports unavailable
in Node Graph mode instead of estimating from inactive AE Controls.

Before writing, the gateway requires the target token and non-empty
`baseRevision` returned by the last `getState`. The target token binds the edit
to AE's project-root ID, composition ID, layer ID, and effect index; the revision also includes
that target identity, every current render value, and all saved layout coordinates. It validates every
node/parameter/value and the entire change set before mutation. A successful edit is wrapped in one AE
undo group. Unknown versions, stale state, ambiguous/missing targets, unknown
parameter bindings, non-finite/out-of-range values, and oversized payloads are
rejected without applying the requested changes. The gateway returns a stable
error code and does not include host pointers or serialized C++ objects.

Version 1 is bounded to 32 changed values and 64 KiB per request. Host scripting
calls stay short; status polling reads four scalar/time values and does not perform
simulation or rendering.

### Render and persistence boundary

The supervisor updates the canonical arbitrary graph value and the panel-facing
ordinary streams in the same AE parameter-change transaction. The SmartFX
pre-render selector continues to check out and snapshot the graph parameter;
render code never queries AEGP or ExtendScript state. AE owns serialization,
copy, and undo/redo for both the supervised stream values and the graph value.

If host qualification shows that hidden standard streams cannot be addressed by
AE scripting, or that scripted edits do not invoke the supervised callback and
undo transaction as required, stop before building more panel UI and revise the
transport contract. Do not fall back to mutating sequence data or process-local
state.

## Failure behavior

- Host unavailable or `evalScript` failure: panel reports disconnected and
  retains no assumed project state.
- No active composition, selected layer, matching effect, or a unique target:
  return a typed target error without mutation.
- More than one matching effect across the selected layers, including duplicate
  instances on one layer, is an ambiguous target; never silently select the first
  effect instance.
- Target/state token mismatch: return `stale_state`; the panel reloads before a
  new edit.
- Invalid request/version/value: reject the entire change set before opening an
  undo group.
- Host error during a multi-value update: attempt to restore the captured old
  values in the same undo group and return `host_write_failed`; report if
  rollback also fails.

## Qualification gate

In AE 2023, verify that the panel can read hidden stream values, write several
values in one operation, cause the supervised effect callback to update the
graph, update rendered pixels, undo and redo the edit, and preserve the result
through save/reopen and effect duplication. Code/build success alone does not
close this gate.

## Primary references

- [CEP HTML Extension Cookbook: invoking host scripts](https://github.com/Adobe-CEP/CEP-Resources/blob/master/CEP_12.x/Documentation/CEP%2012%20HTML%20Extension%20Cookbook.md)
- [After Effects Scripting Guide: Property.setValue](https://ae-scripting.docsforadobe.dev/property/property/)
- [After Effects Scripting Guide: effect properties](https://ae-scripting.docsforadobe.dev/property/propertybase/)
- [After Effects Scripting Guide: Item.id](https://ae-scripting.docsforadobe.dev/item/item/)
- [After Effects Scripting Guide: Layer.id](https://ae-scripting.docsforadobe.dev/layer/layer/)
- [After Effects Scripting Guide: PropertyBase.propertyIndex](https://ae-scripting.docsforadobe.dev/property/propertybase/)
- [After Effects C++ SDK Guide: parameter supervision](https://ae-plugins.docsforadobe.dev/effect-details/parameter-supervision/)
- [After Effects C++ SDK Guide: PF_ParamDef and arbitrary-data standard controls](https://ae-plugins.docsforadobe.dev/effect-basics/PF_ParamDef/)
- [After Effects C++ SDK Guide: arbitrary-data parameters](https://ae-plugins.docsforadobe.dev/effect-details/arbitrary-data-parameters/)
- [After Effects C++ SDK Guide: AEGP query/render dependency warning](https://ae-plugins.docsforadobe.dev/aegps/cheating-effect-usage-of-aegp-suites/)

## Amendment: graph editor interactions and protocol v2 gate (2026-09-29)

The owner requested a pinned target that survives AE selection changes and direct
node-canvas gestures: select a wire to disconnect it, and drop a node over a wire to
insert it into that connection. The panel source now resolves a pinned target by the
project/comp/layer/effect token from `getState`; missing or changed identities fail
closed. Node card coordinates are persisted by the hidden standard effect parameters
defined in ADR 0014. Marquee selection, canvas pan, and wheel zoom remain transient
view state. A topology gesture must never update only the canvas and pretend it changed
the project.

Protocol v1 cannot persist these edits: its state is a hard-coded four-node view, its
revision hashes only flat parameter values, and its only write operation updates those
ordinary streams. The graph itself is a `PF_Param_ARBITRARY_DATA` value. The scripting
API documents `PropertyValueType.CUSTOM_VALUE` generically, but does not establish that
AE 2023 scripts can read or set this plug-in's arbitrary-data encoding or that such a
write reaches the supervised callback and participates in undo. Do not assume that a
custom-value property accepts the plug-in's `PRINT`/`SCAN` text representation.

Protocol v2 must provide a bounded graph snapshot with stable NodeId, PortKey, and
EdgeId values, plus an atomic, revision-checked edit transaction. At minimum it must
support adding nodes, connecting ports, disconnecting a specific EdgeId, and
`InsertNodeOnEdge` (remove one edge and create the two typed replacement edges in one
transaction). The host validates the complete resulting graph before committing it to
the AE-owned graph parameter in one undo step. A successful response returns the new
graph snapshot and revision. Failed validation, stale revisions, or failed host writes
leave the stored graph unchanged. Node card coordinates are already covered by ADR
0014 and live in the AE project. Canvas pan and zoom are transient view state.

The owner ran the read-only
[`tools/probe_graph_property.jsx`](../../tools/probe_graph_property.jsx) in AE
23.5x52 (AE 2023.5.2), with one Starfield effect selected. The property at index 31
reported `propertyValueType: 649` (`CUSTOM_VALUE`). Reading `.value` threw “Can not
get or set a value from this property … This propertyValueType CUSTOM_VALUE has not
been implemented.” The object exposes a `setValue` function, but no setter call was
made, so script write, callback delivery, undo, and persistence are all unverified.
This closes the read-capability probe: ExtendScript cannot read the graph snapshot
through the arbitrary-data property on this tested host. Do not ask the owner to rerun
this probe.

The next step is a source-level carrier design review, recorded in a separate ADR,
before adding host parameters or changing the panel protocol. A viable design must
provide both directions: an atomic edit command into the supervised callback and a
bounded project-saved graph snapshot back to the panel. It must also preserve a single
AE undo step, duplicate/copy behavior, and stale-revision rejection. A one-way command
mailbox alone is insufficient because protocol v2 must redraw the committed graph
after edits and reopen. Do not assume that a `setValue` method existing means it can
serialize a plug-in custom value; do not add socket/helper-process channels, mutate
the graph from the panel, or claim topology edits are persistent. Any setter/carrier
host experiment must use a disposable duplicate project/effect and explicit owner
authorization under ADR 0011.

The current source-level carrier proposal is in
[ADR 0013](0013-script-visible-graph-snapshot.md). Its expression-backed snapshot is
not accepted until AE 2023 confirms the callback read and single-step undo gates.

## Amendment: vertical Particle flow and compact cards (2026-09-30)

The owner requests a top-to-bottom display flow: Emitter → Particle → Force →
Output. The Particle stage keeps the name `Particle`; it must not be presented as
`Appearance`. Node cards omit numbered stage/category badges, retain distinct
type colors, and use about one quarter of the previous card area (110 × 54 CSS
pixels). Inputs sit only on the top edge and outputs only on the bottom edge;
wire paths remain vertical at the ports. Reverse or overlapping connections are
rejected, and a connection preview is shown only when it runs downwards. The default
card positions follow the same top-to-bottom order.

Protocol v1 still constructs this view from flat supervised parameter streams.
It does not read the canonical graph bytes, and add/connect/disconnect/splice
gestures remain guarded. These display changes do not qualify topology editing;
protocol v2 must show and edit the actual graph snapshot through the transaction
carrier before the panel can claim graph edits are project-effective.

For compatibility with already saved panel layouts, indices 37–38 keep their
existing hidden-stream identities and values. The panel maps those coordinates
to the Particle card. When all four positions still equal the old untouched
defaults, the gateway presents the new vertical default layout; manually saved
positions remain intact and continue to be written to the same AE-owned streams.

Node titles and summaries are centered. Output no longer carries the fixed
"transparent output" note; its inspector owns the existing global Max Particles
control, and its summary reports `Live N · Max M`. `Live N` is derived from the
same time/rate/lifetime/cap slot-range rules used by the portable simulator. This
is only a presentation grouping: it does not reassign an AE parameter ID or change
the renderer's global cap contract.
## P-02K / panel48: bounded background inspection (2026-10-05)

The owner confirms frequent busy cursors stop when CEP closes. Inspection finds
independent200ms status calls and1200ms full refreshes, with repeated native reads
and an ensureNodeEffects gateway reload even for initialized graphs. These are
confirmed source operations; their contribution to wall-clock host latency is
not measured. Native46 render and temporal bootstrap remain unchanged.

Protocol v1 adds read-only getPanelPulse and getPanelState. Pulse reads target,
composition time, graph revision/ready/checksum, Effect Parade count, geometry and
main renderer globals; legacy AE Controls also retain their value revision. It
does not inspect sibling node fields, decode curves, simulate or write. The reply
includes the existing frame-status data. getPanelState combines legacy public
state and the current native snapshot in one synchronous host turn, pinning the
second read to the first resolved identity. Full schema/codec validation remains.

A single completion-scheduled poll replaces both old intervals. Changing replies
use500ms; three unchanged replies back off to2s. Hidden/disabled/interactive or
pending refresh/edit/selection states pause requests. Full inspection occurs on
marker change, focus/manual refresh, and a nominal15s audit when available to
catch same-frame expression dependencies or other changes absent from markers.
There is at most one background request; long callbacks do not enqueue a backlog.
Discarded interaction-time reads invalidate the marker so the next poll retries.
Byte-identical graphs with identical geometry reuse the existing projected view.
Live status may lag2s while idle/500ms while active plus host work; the historical
200ms statement below is superseded for the current panel. This is display
latency, not a change to animation sampling or render-time controls.

Native lookup caching contains Property locators only for the current request.
It tries direct exact names/disk-ID spellings first; one bounded fallback traversal
indexes unresolved fields. Caches clear at request start/reply and before/after
every own Effect Parade add/remove. Values are always read from AE. Identity
enumeration stores numeric slots, detects duplicates and reacquires/checks the
effect before mutation. Appends preserve old numeric slots; removal is descending.
No property references survive structural changes or request completion. Existing
guard, rollback, exact disk ID, schema, conflict and acknowledgement rules remain.
AE idle/seek/native-edit/undo and preset latency qualification remains an owner
gate. No test suites requested/run; source checks do not establish a speedup.
