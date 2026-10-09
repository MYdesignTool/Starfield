# ADR 0037 — Particle seed shift and birth probability

Status: implemented and deployed; real AE2023 qualification open, 2026-10-09. Task M3-16.

## Paired deployment, 2026-10-09

Frozen90b7a2bfe982eab1c11d3592a928ca84e12ab1ec completes the May2023 x64
Release /MT build with NoDistPublish/NoRuntimePublish. At09:14 +08:00 a fresh
no-AE check permits tools/Deploy-TestBuild through the existing Junctions.
Native60/packed32828/ABI7 and CEP61 are installed; eighteen installed and eighteen
saved files plus both selectors are independently checked. The saved native59/
CEP60 pair is artifacts/disabled/m3-16-native60-panel61-birth-controls-20261009.
Receipt/rollback paths and the one-step Restore command are in current-state.md.
No process, registry, cache, environment or Junction change was made. CEP61
retains the separate Texture bounds repair; owner adding/back-source feedback
and all new birth-control host gates remain open.

## Native and CEP authoring milestone, 2026-10-09

Source candidate native60/packed32828/CEP61 appends the controls described below.
Actual USER_CHANGED handling captures the integer slider and enables the hidden
flag only after a successful graph commit. Unrelated edits and old presets retain
activation0. Binding6 encodes the appended aliases; readers1..5 retain their exact
historical field bounds. Metadata can certify chance independently of other
animated Particle controls, but a current zero sample or expression cannot prove
constant-zero probability.

CEP uses separate fixed-disk helpers for Cloud activation242 and Birth
activation245; key40 now maps solely to signed seed243. Fresh nodes write keys40/41;
an explicit edit on an older node adds the other default in the same transaction.
Inspector projections show defaults without modifying an older graph. Presets,
signed wire types and failure rollback retain the distinction between absent
controls and authored defaults. The panel candidate stays under artifacts/prepared
until a complete paired build and no-AE deployment.

Focused evidence: native bindings8441 plus camera12, callbacks344 and actual
registration269, all0 failures; Birth CEP51, Cloud49, Texture80, complete gateway
and startup pass. The proof fixture was corrected to report expression-bearing
controls as time-varying before recording the passing result. Logs use
artifacts/m3-16-*; full build and real AE2023 gates remain open.

## Graph integration milestone, 2026-10-09

The preserved graph/static/history/Auxiliary implementation is now exercised by
tests/RunParticleBirthGraphTests.ps1 -Run:3348 checks pass,0 failures. New
Force/Transform tests had incorrect key names; those fixture names were repaired
before recording this result. Motion interpolation is provided by the existing
MotionBlur header; no nonexistent source file is added to the runner.

Coverage includes source movement, random color/lifetime/size/opacity/rotation,
texture/Cloud random keys, signed seed extremes, nested chance subsets, full
authored sibling streams versus legacy partitioning, birth-time samples,
Auxiliary parents, low-chance cap filling, constant-zero proof, cancellation/
typed work limit, Force/Transform, shutter identities and snapshot/graph codecs.
It is Core evidence; native/CEP authors and real AE2023 remain open. Log:
artifacts/m3-16-birth-graph-current-tests.log. Current native59/CEP60 is unchanged.

The next authoring milestone uses a PF_Param_SLIDER for Shift Seed, with exact
signed32 valid bounds and a practical drag range -100..100. Unlike floating
slider bounds, this avoids rounding INT32_MAX upward. Birth Chance is a float
slider0..100/default100. The signed range is an independent supported numeric
contract, not a claim about reference slider bounds. Append only disk243/244/245
and streams534/535/536; activation0 preserves old branches. Binding6 reads old
v1..5 with their own historical limits, including v5 ending at533. Shared native
inputs require a full future build, not a Core-only replacement.

## Reference and ownership

Owner inventory artifacts/reference/stardust_effect_parameters.txt lists Shift
Seed0 and Birth Chance100. The public [Stardust guide](https://superluminal.tv/user-guide)
describes overriding the emitter seed for a connected Particle branch and
selecting a fraction of its resulting births. This requires source motion and
all random properties to use the shifted seed. Exact random distribution and
reference bounds remain hypotheses; no vendor implementation is copied.

Settings.hpp owns ParticleBirthControls. A pure ParticleBirth helper implements
validated finite chance0..100, signed32-bit shift with unsigned32-bit modular
addition, and dedicated RandomPurpose21. Released purposes1..20 remain fixed.
Default shift0/chance100 preserves seed and accepts every source birth; chance0
accepts none. Invalid chance is rejected by graph/native validation before
evaluation; the predicate also returns false for invalid input.

Probability compares one deterministic unit value against chance/100. Its
identity includes the emitter UUID and original source ordinal, excluding the
Particle branch UUID and current frame. Changing chance gives nested source
subsets; rejected births never renumber survivors or change the emission clock.
Different shifts change membership and existing emission/style random streams.
The pure policy is the first milestone; it alone does not implement a visible
control. No native/CEP UI or graph defaults are added before evaluator wiring.

## Planned evaluator and migration contract

Particle optional keys40/int32 and41/float64 retain node schema7 and envelope1.
Missing keys retain legacy branch partitioning. Explicit birth controls activate
one full source stream per Particle branch: siblings with equal shift share the
same source kinematics, then apply their own lifetime/style/chance. This full
source interpretation follows the guide's emitter duplication description and
needs a multi-branch owner reference check; old saved graphs are preserved.

Evaluate Shift Seed and Birth Chance at birth in both static and historical
paths, including Auxiliary. Validate authored Settings before applying an
internal modular seed (which can exceed the public emitter seed slider bound).
Filter chance before global population selection; scan enough earlier live
source births to fill the cap, under the existing bounded work/cancellation
contract. Do not truncate the candidate window before probability filtering.
Zero chance can skip candidate work after numeric/style validation.

Compute random appearance and motion from original source identities. After
those calculations, assign an output identity salted with Particle branch UUID
for authored branches so shutter matching cannot confuse sibling particles.
Keep emitter_id and old graph identities unchanged. Auxiliary parent identities
remain distinct across branches. Topology reordering or chance/shift changes
must not rename a surviving source ordinal in an unchanged branch.

Native build60 append: streams534 Shift Seed,535 Birth Chance,536 hidden constant
activation; disk243/244/245, binding6. Binding1..5 limits retain their old layouts.
Fresh authoring writes explicit defaults; old effects activation0 preserve their
old graph. An explicit edit activates the controls transactionally. CEP's current
virtual cloudControl(40) activation helper must be separated before real graph
key40 is used. Public native seed bounds require reference evidence; no bound is
claimed as Stardust parity from the pure signed32-bit numeric representation.

No Render.hpp/C ABI/snapshot wire change is made. Shared evaluator/header
changes require a complete native/Core build, performed for native60; this
implementation was not hot published over native57/58/59. Older maintenance
sources and paired backups remain independent preserved evidence.

## Evidence required

Pure policy: zero defaults, signed extremes/wrap, finite bounds, deterministic
reverse order, probability endpoints/nesting/distribution and independence from
released random purposes. Full evaluation: source kinematics/appearance, legacy
graphs, branching, birth-time animation, caps/work/cancel, Auxiliary, Force and
Transform, shutter matching and unchanged snapshot/CPU/GPU transport. Native,
CEP, presets, undo/reopen and actual AE2023 gates follow evaluator integration.

## Pure policy milestone evidence

tests/RunParticleBirthPolicyTests.ps1 -Run builds the isolated two-source /MT
fixture and passes40985 checks,0 failures (artifacts/m3-16-birth-policy-tests.log).
Coverage is limited to the numeric policy described above. Graph evaluation,
authoring, branch/cap/history behavior and host qualification are not claimed
from these checks. This milestone does not overwrite the frozen native58 build
or CEP57 Junction; a future native/Core pairing is still required for exposure.
