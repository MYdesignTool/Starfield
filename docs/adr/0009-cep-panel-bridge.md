# ADR 0009: CEP panel bridge through supervised AE parameters

- Status: accepted architecture for the AE 2023 panel MVP; implementation and host qualification pending.
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
  "operation": "getState | setParameters",
  "target": {},
  "baseRevision": "state-token",
  "changes": []
}
```

`getState` resolves the selected composition, layer, and Starfield effect,
returns an opaque target token, the fixed node/port/edge snapshot, editable
values, and a revision token. `setParameters` identifies bindings by stable
graph `NodeId` + `ParamKey`; AE parameter IDs remain the storage mapping and are
never treated as graph identities. Version 1 displays the required
emitter → force → appearance → output chain with one emitter. Its topology is
read-only; parameter values are editable. Dynamic node creation, deletion,
reordering, and edge rewiring require a later protocol version.

Before writing, the gateway verifies the target token and compares the current
editable state with `baseRevision`. It validates every node/parameter/value and
the entire change set before mutation. A successful edit is wrapped in one AE
undo group. Unknown versions, stale state, ambiguous/missing targets, unknown
parameter bindings, non-finite/out-of-range values, and oversized payloads are
rejected without applying the requested changes. The gateway returns a stable
error code and does not include host pointers or serialized C++ objects.

Version 1 is bounded to 32 changed values and 64 KiB per request. Host scripting
calls stay short and run only in response to panel actions; they do not perform
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
- [After Effects C++ SDK Guide: parameter supervision](https://ae-plugins.docsforadobe.dev/effect-details/parameter-supervision/)
- [After Effects C++ SDK Guide: PF_ParamDef and arbitrary-data standard controls](https://ae-plugins.docsforadobe.dev/effect-basics/PF_ParamDef/)
- [After Effects C++ SDK Guide: arbitrary-data parameters](https://ae-plugins.docsforadobe.dev/effect-details/arbitrary-data-parameters/)
- [After Effects C++ SDK Guide: AEGP query/render dependency warning](https://ae-plugins.docsforadobe.dev/aegps/cheating-effect-usage-of-aegp-suites/)
