# ADR 0019: AE-native effect instances own node records

- Status: proposed; owner direction is to investigate per-node AE effects, synchronization spike pending.
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

## Proposed architecture

1. **A node is an AE effect instance.** Provide distinct thin AE effect modules
   for the renderer/output and each node family. The panel creates and removes
   node instances in the layer's Effect Parade. Each instance owns its ordinary
   AE parameter streams, so each Emitter, Particle, and Force has independent
   saved values and native AE undo/copy behavior.
2. **Keep the renderer singular.** One Starfield Output/renderer effect owns the
   graph topology, edges, and layout in its project-saved graph record. Each
   node module owns that node's editable parameter values and a persisted node
   identity. The graph compiler maps node effects to core UUIDs; it must not use
   display names, effect indices, or session-only stream IDs as persistent IDs.
   Duplicating a node must allocate a fresh identity and apply a defined link
   policy; the prototype must prove this before enabling Ctrl+D for node effects.
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
5. **Keep the runtime hot path.** The thin node/output AEX modules share the
   reloadable `StarfieldCore.dll`. Core algorithm changes remain Core-only
   builds; only changes to an AE module's parameter/UI contract require replacing
   that module.
6. **No development-era migration burden.** Existing single-effect graph
   snapshots are not a released project format. This redesign may start with a
   fresh development schema; migration from the current in-development graph
   format is not required.

P-02C's Size/Opacity curves and other non-scalar node values must also end up
owned by the corresponding Particle effect. The prototype may start with scalar
fields, but P-02B must not resume until the curve payload has a bounded,
project-persisted per-node representation and the renderer snapshot compiles it
without leaving a second editable source of truth.

Use one thin AEX code fragment per AE effect module. Adobe permits multiple
PiPLs in a single file for After Effects, but recommends one effect per code
fragment; separate modules also give each effect type a stable match name and
independent parameter schema.

## Synchronization spike

Before replacing P-02B's transaction plumbing, prove one Emitter module, one
Particle module, and the Output renderer as a narrow AE 2023 prototype. The
likely native route is for a node's `PF_Cmd_USER_CHANGED_PARAM` to request a
UI-thread graph compile through the AE effect communication path, then write the
compiled graph snapshot to the Output effect inside the same host edit/undo
operation. The exact callback and stream update mechanism is intentionally not
declared proven here.

The spike is successful only when all of these work in AE 2023:

- The panel can add two Particle instances and remove either one. Adding a node
  changes the Effect Controls stack immediately; adding a second of the same
  type keeps separate parameter values.
- Changing either node's native AE controls or CEP inspector parameters updates
  the rendered result without pressing Refresh or reopening the panel.
- One edit plus its compiled graph snapshot is one undo step. Undo/redo, Ctrl+D,
  same-name copy/paste, and save/close/reopen keep identities, links, values, and
  rendered output coherent.
- Duplicating an effect instance cannot leave two nodes with the same persistent
  ID or silently redirect existing graph edges.
- Render and pre-render read only the Output effect's owned snapshot; no AEGP
  enumeration of peer effects occurs on the render path.
- The same graph bytes survive reopen and the panel reconstructs node instances
  from the project without an incidental manual Refresh.

If the callback cannot update the Output snapshot atomically with an edit,
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
- ADR 0011 still governs every install, process, or other host-level change.
