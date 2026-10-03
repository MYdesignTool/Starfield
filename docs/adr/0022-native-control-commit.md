# ADR 0022: acknowledged native control commits

Status: development implementation, owner AE 2023 qualification pending.

## Build 17 decision: direct native publication

Owner build-16 evidence: `delivery (parameter 4, stream -1, error 516)`.
The request never advanced to the receiver's context phase. This does not distinguish
an API rejection from an ignored selector, but confirms that the acknowledged
inter-effect path did not work in the owner's AE session. Build-16 host acceptance
failed; improving downstream generic context was not sufficient.

Remove EffectCallGeneric and the main COMPLETELY_GENERAL edit handler. Each native
node AEX compiles NativeGraphCommit.cpp / NativeNodeGraph.cpp / GraphParameter.cpp
and the minimal host-independent graph codec sources. Its USER_CHANGED_PARAM
callback directly compiles sibling node records with its own registered AEGP ID,
substituting the accepted callback value by UUID/index, then writes/verifies/restores
the main renderer streams. The main callback is not invoked to accept this edit.
AE may still call its arbitrary-data handlers as part of normal stream ownership.

NativeEdit is now a local UI context with no transport magic/version. Borrowed refs
and callbacks live only until the direct function returns. Main CEP transactions
share the compiler and snapshot helper, using their own registered AEGP ID. Saved
parameters, identities, node schemas, main 21/layout 6 and Core ABI 2 are unchanged.
The build inputs include the added adapter units so CoreOnly cannot skip their
deployment. Node effects retain independent authored parameters.

Scoped fixture: no main AEGP registration; the generic API always rejects with 516
and is never called. Actual node edit callbacks persist new Origin/scalars/types,
and direct compiler checks cover Particle/Force/color/curves and all existing
rollback/type/float tests. Native sync 348/camera 12 and renderer controls 38 scoped
checks pass. May 2023 SDK x64 Release /MT build and all four independent node AEX
links pass at build 17; real AE Effect Controls edits remain a qualification gate.

The following build-16 notes describe the superseded transport and its diagnosis.

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

## Build31 read-only bootstrap exception

The removed build15/16 generic edit transport stays removed. ADR0026 introduces
a separate private v1 POD bootstrap requested by a proper General AEGP on UI idle.
It requires its own usable PF context, acknowledges optional missing context and
never transports authored edits, borrowed callbacks or source handles. It makes
no project/selection/undo writes and sets no generic FORCE_RERENDER flag. Worker
and render-only contexts stop before AEGP access. Actual AE2023 delivery/context
and startup performance need owner qualification; this is not an edit receiver.
