# ADR 0022: acknowledged native control commits

Status: development implementation, owner AE 2023 qualification pending.

## Evidence and decision

The owner reports native Effect Controls edits become visible only after another
CEP edit. The old fake host merely acknowledged EffectCallGeneric; it never
checked the rendered graph. Reading stored node streams inside the node's edit
callback may also read the value preceding that callback. This timing explanation
is a hypothesis until exercised in AE.

Use the SDK's COMPLETELY_GENERAL inter-effect command with a bounded, synchronous
request containing node kind/UUID, runtime parameter index and its accepted PF
value. Normalize point coordinates to full-resolution pixels. The renderer reads
other native records normally and substitutes that single value only on the
matching UUID/kind. No node values move into the main-effect public controls.

COMPLETELY_GENERAL does not authorize saving by changing params[]. Publish the
compiled arbitrary graph and numeric revision/checksum/source explicitly with
AEGP_SetStreamValue. Retain old stream values until publication is verified;
restore them on failure. Return an explicit status and revision to the caller.
An ignored generic request is a failure, never a successful edit. Native output
also requests rerender after a successful publication. No render-time sibling
reads, expression carrier, idle process, schema change or migration is added.

## Qualification

The scoped fake host delays committing its node stream and ignores changes to
the renderer callback array. Assert new Origin and scalar values enter the saved
graph without CEP, UUID isolation, ordinary curve/color values, rejection and
rollback, nonce independence and balanced handle lifetimes. Build 15 must be
tested by the owner for native dragging, undo/redo, save/reopen and CEP closed.
Compilation and fake-host checks do not establish AE host acceptance.

SDK references: local May 2023 AE_GeneralPlug.h StreamSuite6/EffectSuite4 and
[AEGP suites](https://ae-plugins.docsforadobe.dev/aegps/aegp-suites/).
