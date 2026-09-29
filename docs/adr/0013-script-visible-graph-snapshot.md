# ADR 0013: Script-visible graph snapshot carrier

- Status: proposed; direct `CUSTOM_VALUE` scripting writes are rejected by AE 2023; expression carrier still requires qualification.
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

Use one hidden, ordinary `PF_Param_FLOAT_SLIDER` property as a script-visible graph
snapshot envelope. Store an inert, syntactically valid expression comment in its
`expression` string, for example a versioned `SFLD` header plus base64url graph bytes,
followed by the numeric literal `0`. Keep expression evaluation disabled so this
property continues to deliver its ordinary numeric value. Add one hidden supervised
numeric commit parameter; the panel writes the expression and toggles the commit
parameter inside one ExtendScript undo group.

On `PF_Cmd_USER_CHANGED_PARAM` for the commit parameter, the effect obtains its own
parameter stream through the effect/stream suites, reads the raw expression text with
`AEGP_GetExpression`, bounds and decodes the envelope, validates it with the existing
graph codec, and writes the resulting graph to the existing AE-owned arbitrary-data
parameter. Render and pre-render continue to use the copied graph snapshot; they do
not query AEGP. The panel reads the expression property through the normal scripting
DOM and decodes the same snapshot for display. The arbitrary-data graph remains the
render cache and its existing project/undo contract stays in place.

The envelope must contain a format version, payload length, checksum, stable graph
revision, and the bounded serialized graph. The commit request includes its base
revision. The CEP gateway validates the complete envelope before writing it. The
native callback independently validates all lengths, checksum, schema, IDs, port
types, cycles, and graph invariants before replacing graph bytes. Failed writes or
stale revisions must leave the previous graph usable. A native rejection must also be
detectable by the gateway; if the callback cannot return a reliable acknowledgement,
the carrier is not acceptable.

## Why this is only proposed

The official SDK exposes `AEGP_GetExpression` and `AEGP_SetExpression` on ordinary
streams, and the scripting guide exposes `Property.expression` for standard property
types. The API listings do not prove that an effect can safely obtain its own
expression stream during parameter supervision, that setting the expression triggers
or joins the commit callback as intended, or that the arbitrary-graph update and
expression change undo together in AE 2023. The owner’s two probes establish that the
direct `CUSTOM_VALUE` route is unavailable; they do not qualify the expression carrier.

The graph snapshot expression is a second persisted copy. Every code path that changes
the graph must keep it synchronized, including panel edits, first-open initialization,
control capture, undo/redo, effect copy/paste, and project reopen. The first version
must set a strict encoded-size cap and reject oversized graphs without truncation.

## Required spike and acceptance gates

Before accepting this ADR or shipping topology editing:

1. Build the development carrier and transaction plumbing in the current effect,
   without changing the installed host. Keep the graph callbacks bounded and leave
   the panel topology gestures guarded until the host gates below pass.
2. In a disposable AE 2023 project, set and read a disabled OneD expression from
   ExtendScript; verify the exact source survives save/reopen, undo/redo, and effect
   duplication.
3. Verify that the supervised commit callback can read that same expression with
   `AEGP_GetExpression`, decode a bounded sample, and update the graph parameter.
4. Verify the expression write, commit parameter, and arbitrary-data graph update are
   one undo step. Verify a failed checksum and stale revision retain the old graph and
   return a detectable error to the panel.
5. Verify the render path uses only the graph snapshot and produces the same pixels
   before and after the carrier edit.

The spike modifies an AE project and runs AE, so it requires the exact per-action host
authorization described by ADR 0011. Do not install or restart AE as part of the
spike. If any gate fails, reject this carrier and compare bounded standard-parameter
banks or a separately authorized native host bridge before editing the renderer.

## References

- [After Effects Scripting Guide: Property](https://ae-scripting.docsforadobe.dev/property/property/)
- [After Effects C++ SDK Guide: AEGP Stream Suite](https://ae-plugins.docsforadobe.dev/aegps/aegp-suites/)
- [ADR 0009: CEP panel bridge](0009-cep-panel-bridge.md)
- [ADR 0011: Host changes require per-action authorization](0011-host-authorization.md)
