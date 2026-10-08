# ADR 0037 — Particle seed shift and birth probability

Status: staged implementation, 2026-10-08. Task M3-16.

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

Future native append: streams534 Shift Seed,535 Birth Chance,536 hidden constant
activation; disk243/244/245, binding6. Binding1..5 limits retain their old layouts.
Fresh authoring writes explicit defaults; old effects activation0 preserve their
old graph. An explicit edit activates the controls transactionally. CEP's current
virtual cloudControl(40) activation helper must be separated before real graph
key40 is used. Public native seed bounds require reference evidence; no bound is
claimed as Stardust parity from the pure signed32-bit numeric representation.

No Render.hpp/C ABI/snapshot wire change is planned. Shared evaluator/header
changes require a future complete native/Core build; do not hot publish these
changes over native57 or the frozen native58 candidate. Installed58 remains the
separate maintenance source7861a2b, not this work in progress.

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
