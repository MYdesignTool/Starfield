# ADR 0013: Script-visible graph snapshot carrier

- Status: implementation in progress; direct `CUSTOM_VALUE` scripting reads and writes are rejected by AE 2023; expression carrier is not host-qualified.
- Date: 2026-09-29.
- Depends on ADRs 0008, 0009, and 0011.

## Context

The owner ran `tools/probe_graph_property.jsx` and `tools/probe_graph_setvalue.jsx`
in AE 23.5x52. The `Node Graph Data` effect property reports
`PropertyValueType.CUSTOM_VALUE`; both reading `.value` and calling `.setValue` throw
the same host error that this value type has not been implemented. The write probe
used a temporary duplicate and removed it without saving the project. Direct
ExtendScript reads and writes of the arbitrary-data stream are therefore unavailable
on the qualified AE 2023 build.

CEP still needs a project-saved snapshot and an edit path into the effect's supervised
callback. It must not edit pixels, keep the only copy of a graph in browser storage, or
make the renderer query host state.

## Proposed carrier

Manifest revision 8 appends four hidden, ordinary `PF_Param_FLOAT_SLIDER` streams:

- `Graph Snapshot` (41) is the canonical script-readable mirror. Its expression is an
  inert comment envelope followed by `0` and is explicitly disabled.
- `Graph Edit Request` (42) is a write-only expression mailbox. The panel stages a
  complete replacement graph there.
- `Commit Graph Edit` (43) is the supervised numeric trigger. Its integer value is a
  bounded transaction nonce.
- `Graph Edit Receipt` (44) is the callback acknowledgement. The receipt equals the
  nonce on success and its negative on rejection.

The snapshot text is `/*SFLDSNAP1:<revision>:<byte-count>:<crc32>:<lowercase-hex>*/0`.
The request is `/*SFLDTXN1:<nonce>:<base-revision>:<byte-count>:<crc32>:<lowercase-hex>*/0`.
An initial `/*SFLDSYNC1:<nonce>*/0` request asks the callback to hydrate the snapshot
from the existing arbitrary-data parameter. The binary payload remains the canonical
schema-1 graph codec; the expression is only a script-visible mirror. The development
carrier currently caps graph bytes at 24 KiB so the UTF-16 expression and CEP bridge
stay bounded. The effect never evaluates the payload as code.

On `PF_Cmd_USER_CHANGED_PARAM` for the commit stream, the effect obtains its own
parameter streams through the AEGP effect/stream suites, reads the request with
`AEGP_GetExpression`, bounds and checks it, validates the graph with the existing
codec, and prepares a replacement arbitrary-data handle. It compares the request's
base revision with the current snapshot revision. Only after validation and allocation
does it update the expression mirror, canonical graph parameter, Control Source, and
receipt in the supervised callback. Render and pre-render continue to use the copied
graph parameter; they never query AEGP. The panel reads/writes ordinary expressions
through the scripting DOM. The arbitrary-data graph remains the render source of truth.

Capture and AE Controls synchronization also refresh the expression mirror before
replacing the graph handle. The CEP must perform request-expression write, nonce
trigger, receipt read, and any rollback in one `app.beginUndoGroup` transaction.

The ExtendScript gateway now has source-level `getGraphSnapshot`, `syncGraphSnapshot`,
and `submitGraph` entry points. They are not called by the panel yet. The panel still
needs a schema-1 JavaScript graph codec and graph-driven view/edit model; the carrier
remains disabled until those pieces and the AE host gates are complete.

## Why this is only proposed

The official SDK exposes `AEGP_GetExpression` and `AEGP_SetExpression` on ordinary
streams, and the scripting guide exposes `Property.expression` for standard property
types. The API listings do not prove that an effect can safely obtain its own
expression stream during parameter supervision, that setting the expression triggers
or joins the commit callback as intended, or that the arbitrary-graph update and
expression change undo together in AE 2023. The owner’s two probes establish that the
direct `CUSTOM_VALUE` route is unavailable; they do not qualify the expression carrier.

The graph snapshot expression is a second persisted copy. Every code path that changes
the graph must keep it synchronized, including panel edits, lazy first-open
initialization, control capture, undo/redo, effect copy/paste, and project reopen. The
24 KiB development cap must reject oversized graphs without truncation.

## Required spike and acceptance gates

Before accepting this ADR or shipping topology editing:

1. Build the development carrier and transaction plumbing in the current effect;
   source-level plumbing is now present but has not been installed or exercised in AE.
2. In a disposable AE 2023 project, set and read disabled OneD expressions from
   ExtendScript and verify the snapshot survives save/reopen, undo/redo, and effect
   duplication.
3. Verify that the supervised callback reads the request with `AEGP_GetExpression`,
   returns the matching receipt, and updates both graph and snapshot.
4. Verify the expression write, commit, graph update, Control Source switch, and
   receipt form one undo step. Verify malformed, stale, invalid, and oversized requests
   leave the old graph/snapshot intact and return a detectable negative receipt.
5. Verify the render path uses only the arbitrary-data graph parameter and produces
   the same pixels before and after a valid carrier edit.

The spike modifies an AE project and runs AE, so it requires the exact per-action host
authorization described by ADR 0011. Do not install or restart AE as part of the
spike. If any gate fails, reject this carrier and compare bounded standard-parameter
banks or a separately authorized native host bridge before editing the renderer.

## References

- [After Effects Scripting Guide: Property](https://ae-scripting.docsforadobe.dev/property/property/)
- [After Effects C++ SDK Guide: AEGP Stream Suite](https://ae-plugins.docsforadobe.dev/aegps/aegp-suites/)
- [ADR 0009: CEP panel bridge](0009-cep-panel-bridge.md)
- [ADR 0011: Host changes require per-action authorization](0011-host-authorization.md)
