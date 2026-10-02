# ADR 0022: acknowledged native control commits

Status: development implementation, owner AE 2023 qualification pending.

## Build 16 correction

The owner reports every tried native control edit rejected with error 516 in build 15. That
build required all 98 generic params entries, including the input image, and
trusted generic count/geometry. Those requirements are not guaranteed by the
inter-effect request contract. Exact host failure phase was not recorded.

Private request v2 borrows the caller's renderer/layer references for this
synchronous call and carries numeric layer geometry/time. The generic receiver
also borrows the node callback's suite/handle callbacks; it does not require
generic params[], num_params, input image, effect_ref, utils or pica_basicP. It reads only
the eight main controls/revision needed for compilation through AEGP streams.
If numeric source geometry/time is missing, the node caller reads its source item
dimensions/PAR and current layer time using LayerSuite9/ItemSuite9. No host state
is changed, and borrowed refs/callbacks cannot outlive this synchronous call.
Save/readback/rollback stay explicit. Record phase and stream index on every
failure, including a command that was never delivered. No persistent schema or
Core ABI changes. Validate null/partial generic context in the scoped fixture;
the corrected path still needs owner AE 2023 qualification.

The review confirms scratch params are compiler input, not persistence. Separate
publication into publish scalars / publish graph and readback into verify scalars /
verify graph. Keep graph last and restore every attempted write on failure.
Publication contains only source 2, revision <= 16777215 and two 16-bit CRC halves:
all are integers exactly representable in PF_FpShort. Exact scalar comparison is
appropriate for this receipt contract, not for arbitrary future float controls.
May 2023 AE_Effect.h declares PF_FloatSliderDef.value as PF_FpLong (double); bounds
and default are PF_FpShort. No evidence establishes a host float roundtrip for
the current value. Simulated float storage is an extra robustness check.
Validate OneD / ARB stream types before reading union members. The scoped host
fixture quantizes writes to float and exercises the largest revision, wrong stream
type, changed integer receipt and graph/scalar failures with phase/index diagnostics.

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
rollback, nonce independence and balanced handle lifetimes. Build 16 must be
tested by the owner for native dragging, undo/redo, save/reopen and CEP closed.
Compilation and fake-host checks do not establish AE host acceptance.

SDK references: local May 2023 AE_GeneralPlug.h StreamSuite6/EffectSuite4 and
[AEGP suites](https://ae-plugins.docsforadobe.dev/aegps/aegp-suites/).
