# Behavior inventory

## Owner acceptance / main integration - 2026-10-04

The owner reports the build31 issues are basically resolved and explicitly
authorizes merging this installed version into main. This is an accepted
development checkpoint for AE 2023, superseding the previous merge hold.
No new exact timing numbers or exhaustive host matrix results were supplied;
unreported render-queue/MFR/new-host gates remain open. Development continues
on a separate branch for the Particle gradient editor and reference controls.
The installed build31 and its paired rollback remain unchanged by integration.

## M3-06 / build 31 - post-load bootstrap and Appearance removal

Build30 failed owner no-Options startup qualification: W57/57 P57 E0 but
Temporal0/57, PF193372 and history1671.3ms (CUDA16.3ms). Build31 (32799/0x801F)
adds a session-resident General AEGP for bounded, read-only main-thread idle
certification after load; sequence callbacks only maintain SFU1. Actual generic
PF context availability and no-Options first/reopen preview remain owner gates.
See ADR0026 for the protocol, lifetime rules, retry/state checks and limitations.

Owner explicitly requires complete Appearance removal without compatibility.
Only Emitter (including Auxiliary), Particle, Force and Output remain registered.
Particle owns style/Over Life; connection/layout edits only write node records.
The retired AEX is archived on installation; one-step rollback restores it and
removes the new StarfieldHost.aex. Main manifest24/IDs, remaining native schemas,
C ABI3 and SFU1 remain paired. Old graphs containing Appearance must be recreated.
Gateway native-idle-31; reopen CEP. Main integration waits for AE qualification.

Build31 candidate compilation and all scoped fake-host/core/panel suites pass;
exact counts and ignored evidence paths are in docs/build-matrix.md. The new
General AEGP bootstrap, Particle -> Force wire edit and no-Options first/reopen
performance remain UNQUALIFIED in AE 2023.5.0 Build52. Complete Appearance
removal is intentional; development test graphs containing it must be recreated.

Build31 deployed on 2026-10-04 under standing authorization after the immediate
read-only AE process check. All six installed/old retained hashes, 13 paired CEP
snapshots, retired Appearance absence, selected Core and the single Junction are
verified. Deployment/rollback details are in docs/build-matrix.md. Actual host
qualification remains pending; installing a candidate is not a support claim.

## M3-06 / build 30 - evaluate restored bindings before state certification

The owner rejects build29 startup performance: Options is still required. The
2026-10-03 screenshot reports B2 (two sequence capture attempts), Temporal 0/57,
PF58 / N2/2, followed by 57 valid proofs after Options. This establishes that
bootstrap ran but does not establish usable render proofs. It reports no last
GPU frame and preparation at time zero; it is not a preview latency measurement.

Source inspection identifies a missing step: Options' binding transaction reads
all active aliases after expressions before capture, whereas startup only read
PF states and source metadata. Lazy expression dependency discovery may change a
pre-evaluation state during first rendering. This is a hypothesis about AE, not
host confirmation. Build30 (32798 / 0x801E) evaluates all recorded active aliases
through callback-local AEGP UI streams BEFORE any proof state is captured. It
checks OneD type, expression enabled, finite value and unavailable sentinel.
Only successfully evaluated components are eligible for the existing source
metadata plus before/after all-time state certification and render revalidation.
No expression, source value/key, project stream or selection is written.

Scoped fake-host tests model a dependency generation changing on first expression
evaluation. A negative control demonstrates that pre-evaluation tokens become
invalid. Cold setup and save/reopen then retain proofs after first render reads;
read/type/nonfinite/disabled-expression failures publish no proof for that input
and release references. Denied PF checkout/checkin in sequence callbacks, absent
params, source keys/expressions and worker/render-only exclusions remain covered.
5,157 scoped checks pass (native sync 5,145; camera 12); evidence is
artifacts/build30-native-sync.log. Native build/deployment is recorded separately.

Diagnostics snapshot the previous automatic reader BEFORE Options refreshes it:
W is evaluated/active aliases, P published proofs, E optional warmup error; B
remains sequence attempts. Build30/proofs is the subsequent explicit Options
result. Process-global readings may belong to another effect callback.
Actual AE no-Options first preview and save/reopen are still mandatory gates.
The owner-authorized main integration waits for those results. Core ABI3, public
IDs, match names, graph codec, SFU1 sequence schema, CEP and PiPL flags are unchanged.
Owned scope: NativeNodeGraph, NativeTemporalCache, Diagnostics, PluginVersion,
scoped native sync fixture and these M3-06/ADR0026 records.

## M3-06 / build 29 - remove forbidden sequence parameter callbacks

Owner rejects build28: AE2023 reports effect cannot use checkout/checkin callbacks
in SEQUENCE_RESETUP (25:83), both on project open and during use. The absent-array
fallback added in build28 was invalid. Its fake host allowed a callback AE forbids;
that coverage did not qualify the host selector. Automatic startup remains unproven.

Build29 (32797 / 0x801D) never reads sequence params[] and never calls PF parameter
checkout/checkin in SETUP/RESETUP. On the recorded main UI thread only, it obtains
the current effect's graph stream through AEGP_PFInterfaceSuite1/StreamSuite6,
checks ARB type, copies the graph while the returned value is alive, then reads
source metadata through the existing reader. Value, stream and effect references
are released in reverse order within the callback. Missing graph/type/suites is
an optional miss; it neither rejects project opening nor publishes a guessed proof.
All-time owned-alias states still bracket source metadata and are checked in render.
Worker/render-only resetup skips AEGP entirely. No render-thread AEGP, retained
source handle, idle hook or cross-effect generic message is added.

Qualification: 3,974 scoped checks pass (native sync 3,962; camera 12). The full
May 2023 SDK Release /MT candidate builds all five AEXs and Core without compiler
warnings. Evidence: artifacts/build29-native-sync.log and build29-native-build.log.

Schema SFU1 remains exactly four flat bytes; graph codec, node controls, public
IDs, Core ABI3, CEP and PiPL flags are unchanged. Full paired AEX deployment is
required. Focused tests now deny PF checkout/checkin in sequence callbacks, pass
an undersized params array, omit callback functions, inject graph read/type failures,
and check stream/ARB cleanup, recovery, keys and worker exclusions. Actual AE
open/run without 25:83, no-Options first preview and save/reopen performance are
mandatory gates before the owner-authorized main integration.

## M3-06 / build 28 - initialize history proofs when opening projects

The owner confirms build27 cuts uncached-frame preparation from 6-8 seconds to
approximately 1-2 seconds, but only after clicking Options, including after saving
and reopening. This qualifies a visible improvement, not full frame-time parity.
The constant/key metadata registry is process-local; the DRAW refresh added in
build26 is not delivered for every unopened/unselected main effect.

Build28 (32796 / 0x801C) reads optional source metadata in main-thread
SEQUENCE_SETUP/RESETUP, before preview needs it. Worker/render-only callbacks
perform no AEGP source queries. Options and DRAW remain optional refresh routes;
source keys/expressions and all-time PF states are revalidated after every load.
No equality-of-values shortcut is used. Diagnostics B counts sequence refresh
attempts; Static/Temporal and PF/N indicate actual render-side use.

Qualification: 3,627 scoped checks pass (native sync 3,615; camera 12); all five
AEXs and Core build with May 2023 SDK Release /MT without compiler warnings.
Evidence: artifacts/build28-native-sync.log and build28-native-build.log.

Private main sequence-data schema SFU1 consists of exactly four byte-ordered ASCII
bytes, with no handles, references, PF states, graph values or cached particles.
Legacy null data is provisioned during setup/resetup and also during flatten/save;
unknown non-null schema/size is rejected without replacement. SETDOWN releases the
host allocation. Saved non-null data provides a RESETUP opportunity on reopening.
An old null-data project may lack that callback; first cold legacy open and first
save/reopen are separate AE acceptance gates. Build28's partial/absent-array checkout fallback was subsequently rejected by AE
(25:83); build29 replaces it with an AEGP graph stream read. Source metadata can
be unavailable while siblings restore; later DRAW can retry. Fake-host results
cannot establish AE callback order or availability.

Scope: EffectMain, NativeTemporalUI, Diagnostics, PluginVersion, scoped native
sync tests and these records; no Core/Render transport, source node controls,
public parameter IDs, match names, graph codec, CEP or PiPL flag changes. The
private sequence migration is recorded in ADR0026. Full paired AEX deployment
is necessary. The owner-authorized main merge awaits no-Options reopen validation.

## M3-06 / build 27 - native dependency lookup and constant-node reuse

Owner AE2023 feedback for build26 confirms that delay no longer grows with time,
but uncached frames still take a subjective 6-8 seconds even with CEP closed.
Options reports roughly 39 ms preparation and 11 ms CUDA rendering in one sample.
Those selector timings exclude AE work before/after entry. The earlier screenshot
with 56/57 constants used Life queries; the fully certified Static path used zero
birth-node queries. Therefore the proposed (node, birth-time) cache-miss analysis
cannot by itself explain the observed Static-frame wait.

Source inspection finds a distinct dependency defect: UUID lookup scanned every
effect and read numeric properties before establishing that they were UUID fields.
Emitter UUID indices 99..106 and Particle indices 143..150 overlap the renderer's
98..609 expression inputs. This could introduce self/cross-renderer expression
dependencies. Build27 (32795 / 0x801B) skips the expression's own effect by
propertyGroup(1).propertyIndex and gates other effects by the fixed hidden property
name before reading all eight UUID words. Effect display names/order remain free.
A generated-expression regression proves the old lookup reads renderer values and
the new lookup performs zero such reads. Actual AE latency impact remains a hypothesis.

Constant nodes are converted once per frame and reused across every birth time,
including mixed/Force graphs; dynamic nodes retain exact-time sampling. Equality
of sampled values never certifies a node. Static simple graphs reuse the already
sampled current-frame graph without a second batch of alias checkouts. Dependency
changes discard the optimization. No render-thread AEGP access is introduced.

Options upgrades the generated owned bindings in existing development projects;
click it once after deployment. Source keys/expressions and graph values are preserved.
Diagnostics include node requests/actual samples (N), separate main GPU setup and
UI refresh maxima, and the render's own prepared time (p). A time/duration-mismatched
SmartFX pair is rejected before borrowed pixel access, with equivalent rational times
accepted. Process-global last-call diagnostics do not measure full AE preview time.

Qualification: 3,383 scoped C++ checks pass (native sync 2,938; camera 12;
actual CUDA/OpenCL drivers 433), plus generated-expression/keyframe JS checks.
May 2023 SDK Release /MT builds all five AEXs and Core without compiler warnings.
Evidence: artifacts/build27-native-sync.log, build27-expressions.log,
build27-gpu.log and build27-native-build.log. These are not AE frame-time results.

Main/native IDs, parameter counts, schemas, PiPL flags, CEP native-gpu-24 and Core
C ABI3 stay unchanged. Full paired AEX deployment is required. AE 6-8 second latency,
key/expression edits, copy/reorder/undo/reopen remain owner gates; M3-06 stays open.
The requested main integration is pending this candidate's qualification.

## M3-06 / build 26 — UI/render proof matching and actual preparation costs

Owner reports build25 remains slow or becomes slower at later frames. Its
Options `History` count was measured in the UI and did not prove render-side
use. The strict equality of UI/render `effect_ref` was an invalid instance-key
assumption: the SDK defines it as an opaque callback reference. Whether this
was the owner's actual failure remains an AE qualification question.

Build26 (32794 / 0x801A) matches proofs by current graph node UUID, owned alias
index, and AE's all-time dependency-state comparison. A different callback
reference alone no longer rejects a proof or prefix. Copied layers with unequal
states remain isolated. Prefix versions are bounded and matched by host state;
UI analytic-profile publication also invalidates sampled prefixes created by
render copies. Comparison errors and unavailable proofs retain exact sampling.

Main CUSTOM_UI is implemented through a zero-sized COMP registration and EVENT
DRAW handler; PiPL/runtime flags agree at 0x02008466. On AE's recorded main thread,
DRAW refreshes numeric metadata after dependency changes. Equal all-time non-layer
states skip source reads. No overlay, ECW area, project write, undo record or
rerender request is created. Worker/non-DRAW/reentrant calls are ignored before
SDK access. This automatic path depends on the main effect receiving composition
DRAW events (selection and visible layer controls); Options remains an explicit
refresh when no DRAW callback arrives. No render/sequence-worker AEGP query is added.

Options now snapshots real last pre-render costs before UI refresh: total,
controls/history/scene milliseconds, Static/Temporal path, constant/total inputs,
actual history PF checkouts, PPS and Life queries. Smart Render wall time includes
GPU upload, dispatch, synchronization, resource cleanup and host world checkouts.
These are coherent process-global
last-call diagnostics, not an instance-specific or full preview-time measurement.
UI proofs are labelled separately. Main/native parameter layouts and IDs, CEP
native-gpu-24, and Core C ABI3 remain unchanged; full paired AEX build is required.

Focused checks cover distinct UI/render references with equal dependencies,
copy isolation, key/expression invalidation, undo, prefix publication, static
history bypass, UI callback/throttling/thread safety and real preparation timing.
3,253 scoped checks pass: native sync 2,671, camera 12, emission cache 157,
CUDA/OpenCL drivers 413. May 2023 SDK Release /MT builds all five AEXs and Core.
Actual AE t=2/10/100 performance, cross-project state equality, UI event delivery,
key/expression edits and reopen remain owner gates. M3-06 stays open.

## M3-06 / build 25 — bounded native history preparation

Owner confirms build24 runs on CUDA device0, but 10,000 PPS / Life 2, no keys
or extra nodes, becomes slower at later frames. The reverse birth loop continues
through expired births because the 1,000,000 Output cap exceeds the live population.
The previous global Life bound is 10,000 seconds; it is not the authored Life.

Build25 (32793 / 0x8019) certifies native source controls on supervised main UI
commits or Options: no source expression/keys, matching all-time owned-alias PF_State
before/after metadata capture. Render validates with PF_ParamUtilsSuite3; no render
AEGP read or retained source handles. Certified fields are hoisted into a frame-local plan.
Simple static Default Emitter/Particle/Output graphs with Life Random 0 use the
closed-form alive-slot path, skipping historical checkouts and snapshot encoding.
Other temporal graphs use certified Life bounds and optional emission prefix leases.
Linear/hold Life bounds use the complete key envelope; Life Random only shortens Life.
Bezier/expression Life retains the safe global bound. PPS constants/linear/hold keys
use analytic integration; nonlinear sources reuse fixed-lattice prefixes under equal
all-time dependency stamps and 30/60/120 Hz. Changes/errors/contention/eviction fall
back safely. Complete cached/uncached particle snapshots compare byte-identically.

3,480 focused C++ checks pass: native sync 2,101, camera 12, emission timelines 397,
emission cache 131, current nodes 379, CUDA/OpenCL drivers 409, main controls 51.
At cap1,000,000 / PPS10,000 / Life2, t=100 and t=10,000 both perform 20,000 Life
queries; core temporal evaluation measured 25.442/27.272 ms. The certified static
core path measured 2.421/2.033 ms. These exclude AE checkout, GPU dispatch and host
preview overhead and are not an AE frame-time claim.

Main/native layouts, public IDs, CEP native-gpu-24 token and C ABI3 stay unchanged.
A full paired build is required because the adapter and statically linked Core contract
both change. Existing build24 effects may remain. For an existing project, click main
Options once to capture metadata; it shows `History: N certified inputs` and requests
rerender. External Effect Controls/key/expression edits may require Options recapture
for full optimization; invalid proofs preserve correct historical rendering meanwhile.
Automatic recapture after every external edit remains an open workflow gate.

Owner AE gates: real end-to-end timings at t=2/10/100, key/expression invalidation,
reverse seek, undo/reopen and surrounding effects. No AE process was operated by the
agent. Do not close M3-06 from compilation or standalone timings.

## M3-06 / build 24 — AE native CUDA/OpenCL candidate

Build 24 (32792 / 0x8018) implements host-proposed CUDA/OpenCL device setup/setdown,
per-frame SmartFX eligibility and SMART_RENDER_GPU into AE GPU_BGRA128 output.
Main Acceleration is GPU by default, with a CPU selection at index 614/disk 1611.
The four control-node effects also implement GPU pass-through to avoid our own
CPU image copy between nodes. Main/node PiPL and runtime capability flags agree.
Unimplemented frameworks or initialization failures reject GPU support cleanly.
MFR, Compute Cache and DirectX support remain disabled; no private graphics context.

Core C ABI 3 adds an immutable, typed, ROI-relative sprite/tile scene and paired
release callback. CPU simulation/history/projection prepare the scene; GPU kernels
perform deterministic ordered rasterization. CPU/GPU share camera/shape geometry.
There is no full-frame CPU staging image or GPU image readback in the product GPU
path. Numeric scene uploads still cost time. GPU scene budgets reject before GPU
output checkout and negotiate CPU without truncating particles.

May 2023 SDK full Release /MT build succeeds. 2,432 scoped C++ checks and focused
JS round trips pass. 409 actual-driver checks exercise CUDA and OpenCL (including
an out-of-order OpenCL queue), main SmartFX and the real node-effect GPU selectors,
Circle/Rectangle/Cloud, camera, HDR/alpha, padded rows/ROI, cancellation, partial
allocation failure, ABI rejection and cleanup. Small-scene maximum float component
difference versus CPU is 0.00000175834; this is scoped evidence, not a global error bound.
A standalone 512x512, 20,001-sprite benchmark takes roughly 3.4–3.7 ms preparation
plus 1.2–1.6 ms allocation/upload/kernel/synchronization, versus 67–69 ms CPU rendering.
AE parameter/history capture, device startup and surrounding host effects are excluded.
Actual AE GPU dispatch, preview scales/ROI, render queue and end-to-end timings remain
owner qualification gates. Do not claim AE GPU support from these tests alone.

Fresh main effects are required (main manifest/schema 24, registered count 616
including implicit input). Existing IDs 1..612 and native node layouts are unchanged.
Reopen CEP for native-gpu-24. Full paired AEX/Core deployment is required for ABI 3;
no old-project migration. AE-certified PPS metadata and cross-frame prefix-cache
invalidation/reuse remain open; build 24 still samples native PPS.


## M3-06 / build 23 — Particle controls and temporal preparation

AE 2023 / May 2023 SDK candidate, packed version 32791 (0x8017). Emitting now
means Default / Once / Sequenced / Randomized; Auxiliary Source is separate.
Unordered Point/Box/Sphere/Disc sources use default ordering for Sequenced/Randomized;
Once births an initial PPS-sized batch. Direction defaults to Uniform.
Particle schema 4 adds Life Random, Circle/Rectangle/Cloud, Size Y, Feather,
Up Axis, Orient To, Angle X/Y/Z and spin Speed X/Y/Z with native AE Angle controls,
Angle/Speed Random and Limit to 2D. Existing four color modes/gradient are retained;
the initial gradient colors are visible in Effect Controls. Cloud is an independent,
fixed five-circle procedural cluster, not the complete reference Cloud settings.
Shapes and rotation preserve transparent output in 8/16/32-bpc CPU rendering.

Main manifest 23 appends Simulation Settings 610..612: Time Sampling at index 611 /
disk 1601 offers 30/60/120 Hz, default 30. Emitter schema 6/base 31 and Particle
schema 4/base 75 require fresh development effects. Output schema 4 carries Hz.
Transient 0x8004 version 2 preserves the added sprite fields in paired AEX/Core;
C ABI 2 and Render.hpp remain unchanged. Reopen CEP for ready marker
native-particle-controls-23. No old-project compatibility path is added.

Historical capture now decodes bindings once per frame and samples one node
without copying the entire graph; Force conversions share a bounded evaluation
cache. Core EmissionTimeline implements metadata-certified rate*t, exact linear/hold
integration and fixed-lattice prefix reuse/inversion. 367 focused checks prove those
Core paths. **AE rate metadata capture, reliable invalidation, and prefix reuse across
AE frames are still open; the current adapter still samples PPS.** No constant curve
is guessed from equal samples, and no render-thread AEGP read is introduced.

GPU ADR 0026 follows host CUDA/OpenCL negotiation, GPU_DEVICE_SETUP/SETDOWN,
per-frame GPU_RENDER_POSSIBLE and SMART_RENDER_GPU into AE GPU_BGRA128 worlds.
The May 2023 SDK has no DirectX contract. GPU handlers/kernels, main Acceleration
selection, actual AE GPU qualification and end-to-end timings remain open. Build 23
is CPU-only and retains disabled GPU/MFR/Compute Cache capability flags.

Scoped qualification: 2,017 C++ checks across current-node behavior/pixels,
native sync/camera, emission timelines, all four node registrations and main
renderer controls; focused JS checks cover production gateway round trips,
generated UUID expressions/keyframe preservation, gradient encoding, panel
startup/error retention and node interactions. All pass. The paired five AEXs
and Core build with the May 2023 SDK Release /MT. No AE session is operated;
owner host rendering/keyframes/undo/reopen remain open.

Build 23 is deployed with AE absent. Six installed files, selected/pinned Core,
prior backups/selector and the 13-file paired CEP rollback snapshot were verified.
See build-matrix.md for hashes, before/after records and the one-step paired undo.
Fresh development effects and a reopened CEP panel are required. Actual owner
AE rendering, new controls, keyframes, undo/reopen and GPU qualification remain open.
## M3-05 / build 22 — temporal controls, Particle color and native Angle

Owner confirms build 21 Origin birth behavior. Build 22 integrates historical
PPS, samples all existing birth controls at actual birth times, and integrates
Force over lived time for ordinary/Auxiliary systems. Particle Color uses four
modes and a saved 2–8-stop gradient. Emitter rotations use native AE Angle controls
with turns/degrees, a dial and keyframes. Fresh Emitter/Particle effects required.
See [parameter time audit](temporal-parameter-audit.md) and ADR 0025 for the full
control matrix, precision, bounds and remaining reference/AE gates.

1,420 scoped C++ checks and focused generated-expression/startup/gradient JS
checks pass; all five May 2023 SDK Release /MT AEXs and paired Core DLL build.
No AE session operated. Owner AE 2023.5.0 Build 52 qualification remains open.
Build 22 is 32790/0x8016; native identity 3/layout metadata 7; main IDs/manifest,
Render.hpp and C ABI 2 unchanged. The CEP ready marker remains animation-19.


Build 22 is deployed with AE absent. The single Junction and six installed,
selected/pinned Core and old backup hashes were verified. Source 5d4acdb is pushed;
before/after states and one-step rollback are recorded in build-matrix.md.
Fresh Emitter/Particle effects and a reopened CEP panel are required; owner AE
qualification remains open.

## Build 21 M3-03 — emitter Origin at birth

Owner confirms build 20 renders and Origin keys animate, but survivors follow the
current emitter position. Birth-position semantics are implemented separately from
current-frame parameter sampling (ADR 0024). Pre-render queries owned Origin XY/Z
dependencies at selected particles' birth times, memoized by emitter UUID/time.
Its transient optional record 0x8003/version 1 passes immutable positions to the
DLL. Primary particles retain their birth origin plus normal velocity/force motion;
Auxiliary children use historical parent positions and their own birth offsets.
No project graph writes, process-global state, AE handles in Core, public ID,
Render.hpp or C ABI 2 changes. Paired PiPL/runtime wide-time flags let AE track
historical dependencies; no cross-frame history cache is kept.

Native sync 792, camera 12, portable current-node 78 and renderer controls 39
checks pass (921 total). Nonlinear/subframe origins, frozen codec transport, old
particles staying at birth positions, Auxiliary parents, forward/reverse pixels,
unchanged project graph, failed checkout and quiet cancellation are covered.
All five May 2023 SDK Release /MT AEXs and the paired Core DLL build without
compiler warnings/errors. Logs: artifacts/build21-*. No AE session operated.
Owner AE interpolation, prior-key cache invalidation and reopen remain open gates.

History is bounded to 1,000,000 unique origins (48 bytes each); host sampling uses
up to one-million ticks/s, reduced to fit its 32-bit numerator. Other animated
birth controls/emission clocks and integrated Force history remain separate work.
The main AEX's birth planner shares GraphEvaluation/ParticleSimulation/Random
source with Core; these inputs are now part of the adapter fingerprint, requiring
paired builds when planning changes. CEP keeps its unchanged animation-19 token.

## Build 20 P-02J follow-up — 2026-10-02

Owner rejects build 19 on opening: animation binding stream -1 unavailable (516).
The sampler rejected num_params below 610 before checking out any binding. That
field counts delivered parameters, not registered streams; SmartFX delivers no
params[] array. The previous fixtures always supplied the registered count, hiding
this path. Reproduction with num_params=0 failed the build-19 native suite (30
failures); removing that gate restores the same samples and visible frame pixels.
The owner screenshot does not expose the actual count; attribution of that host
error to this gate remains an inference until owner AE playback qualification.

Sampling now relies on registered stream IDs and checkout/checkin callback errors.
The main render checkout also reads Time Remapping/Preview controls without a
delivered-array count gate. Failures identify record/callback/checkout/value/
checkin/conversion phase, stream index and delivered count. Invalid records,
missing callbacks and invalid expression values still reject; no zero fallback.
No parameter ID/layout/Core ABI, expression format or CEP changes. The paired
animation-19 gateway token intentionally stays unchanged.

Native sync 761, camera 12 and renderer controls 39 checks pass (812 total).
The full main graph checkout and CPU pixel test use num_params=0 at forward,
intermediate and reverse times. Counts 1 and 610 also succeed; actual checkout
errors, missing callbacks and malformed records retain precise rejection phases.
Time Remapping/Preview values and failure propagation are exercised with count 0.
May 2023 SDK x64 Release /MT builds all five AEXs with no compiler warnings/errors.
Generated-expression and actual JSX startup suites pass; CEP sources are unchanged.
Logs: artifacts/build20-*. AE interpolation, CEP-closed playback and reopen remain
owner qualification gates. No AE session operated by the agent.

## Build 19 P-02J follow-up — 2026-10-02

Owner rejected build 18: gateway_missing despite the JSX animation-18 token;
Origin XY keys did not animate and produced black output. Panel expected sync-14,
so the gateway/panel mismatch is confirmed. Both now use animation-19, and the
startup regression executes the actual JSX readiness function.

The old binding expression called intermediate host objects and swallowed all
errors into zero. This is a hypothesis for the AE black output, not a confirmed
host diagnosis. Generated bindings now use Layer.effect(index)/Effect.param(index),
catch only unrelated-effect identity probes, and return an unavailable sentinel
when no UUID matches. UI installation evaluates every installed binding and checks
its enabled state before publication. Failure restores previous values/expressions;
UI and render diagnostics include the failed binding stream. Render still samples
only its own PF inputs; no Core ABI, parameter ID or node layout changes.

Packed version 32787 / 0x8013. Native sync 747, camera 12 and renderer controls 39
checks pass (798 total). Pixel regression renders the sampled graphs with the
actual CPU backend, checking visible alpha, changed frames and reverse-time
repeatability. Generated-expression tests use non-callable host-object fixtures,
missing identities and source failures; actual JSX/panel handshake tests pass.
All five May 2023 SDK Release /MT AEXs build with no warnings/errors in the log.
Logs: artifacts/build19-*. No AE session operated. AE Origin XY interpolation,
CEP-closed playback and reopen remain owner qualification gates.

An initial mistyped runner switch selected the old broad Core suite (269 failures,
including outdated graph/codec expectations); this is not counted as passing
evidence. No broad-suite qualification is claimed by this scoped follow-up.

## Build 18 P-02J candidate — 2026-10-02

Owner confirms build 17 native edits work, then reports almost all stopwatches are
unavailable. Public native controls no longer carry CANNOT_TIME_VARY. Main manifest
22 appends 512 hidden expression-capable numeric dependencies at indices 98..609,
disk IDs 1000..1511. UUID/property/component bindings are installed transactionally
on the UI path. Optional record 0x8002 saves typed raw fields and slots; own PF
checkouts sample each requested frame, including points, RGB, opacity, Life and
Force. The native conversion code is shared. No render-time AEGP acquisition.
Failed graph publication restores changed expressions. CEP edits keyed values at
current comp time, protects user expressions, and never asks constant metadata
for expression state. No authored keyframes are stored on the main effect.

Packed version 32786 / 0x8012; node IDs/schemas/layout 6 and Core ABI 2 unchanged.
Native sync 404/camera 12, four node registration suites 306, renderer registration
39 checks pass (761 total). Actual generated expressions pass JavaScript execution
with reordered/same-name/duplicated peers; focused CEP keyframe preservation checks
pass. May 2023 SDK x64 Release /MT final candidate builds all five AEXs with
-NoRuntimePublish -NoDistPublish; no compiler warnings/errors found in its log.
Logs: artifacts/build18-*. No AE session operated. These checks do not qualify
AE expressions or keyframe playback. Recreate effects on a fresh test layer;
owner tests stopwatches, interpolation, CEP-closed rendering, undo and reopen.
Animation uses current-frame settings; birth/history integration is separate.
See ADR 0023.


Build 18 installed after source push and AE-absent check. All candidate/backup
hashes and selected Core verified; existing single Junction retained. Installation
is verified, AE animation acceptance remains open.

## Build 14 P-02H candidate evidence — 2026-10-02

Ordinary Force/reference control names and fresh zero defaults, the million-particle
cap, renderer remapping/percentage preview and valid SDK include paths are implemented.
Core 58/native Force 79/main registration/checkout 38 scoped checks pass, together
with current-node CEP checks and six-source parsing. May 2023 SDK candidate builds.
No AE session operated. Force native/CEP edits, curves, reference numeric behavior,
Time Remapping animation, Preview stability, undo/reopen and non-square PAR remain
owner host gates. Main feature differences are explicitly listed in
reference-main-force-comparison.md; this is not full Stardust parity.

Build 14 installed after source 6a32f94 push and a zero-process read-only check.
Six hashes, selected Core and prior build backups verified; single Junction
retained. No AE operation performed. These are installation checks only.

## Build 13 P-02G candidate evidence — 2026-10-02

Source 2751a56 pushed before paired deployment. Build 13 is installed with all
six hashes/selected Core checked, build-12 backup verified and one existing
Starfield Junction retained. No AE process started/stopped or GUI session operated.

Owner correction: CEP Origin moves particles; only native Effect Controls Origin
does not. Source diagnosis finds callback UUID/disk-ID comparisons against uu.id,
which aliases change_flags during USER_CHANGED_PARAM. Dispatch now uses runtime
indices and saved numeric UUID values. A fake-host callback test with overwritten
change flags confirms native XY/Z invoke the renderer commit path; actual AE
delivery/render cache/undo/reopen remains unqualified.

CEP effect selection resolves UUID rather than effect names/order and performs no
parameter writes or undo-group changes. Fake-host selection/reorder/stale-target
checks pass. New Emitter speed uses real geometry; Auxiliary records use schema 5,
optional parent input 2, percentages and the common source kernel. Child birth
position and velocity inheritance, survival after parent death, stable identity,
cap/zero chance/invalid intervals/cycles pass in the scoped Core tests.

Camera capture uses SDK row matrices and inverse 2D/3D layer mappings. Core tests
cover Z perspective, translation, behind-camera clipping, 8/16/32-bpc plane parity,
exact ROI crop and downsample. Fake suites cover transformed layers, default-view
fallback, singular explicit cameras and balanced releases. Main camera flags match
PiPL/runtime. Minimum scopes: Core 28, native sync 14, camera 12, actual Emitter 77,
Particle 73 checks; three focused CEP suites pass. No broad suite or AE session run.

Open owner gates: fresh default graph, native Origin XY/Z, selected effect highlight,
multiple parent Auxiliary links, inheritance, camera animation/zoom/perspective,
native/CEP edits undo/save/reopen and UI feel. Non-square comp/source PAR and extreme
3D plane transforms remain open. Advanced source controls and evaluated Auxiliary
live count are incomplete. Build 13 compilation/scoped checks do not imply AE 2023
host support beyond the owner's previously recorded observations.
[Deployment record](native-node-checkpoint.md).

## Build 12 P-02F owner feedback — 2026-10-02

Owner reports only one Emitter can feed a Particle, Life drags by huge values,
and Over Life edits show sprite coverage exceeds bounded work budget (512).
Source review finds four merge blockers (port count, editor replacement,
generic emitter-merge rejection and evaluator one-parent assumption); all are
removed. Input fan-in retains distinct emitter identity and global Output cap.
Normal native slider ranges are separate from typed bounds; CEP uses explicit
steps. Life is 2 by default, max 10000, CEP step 0.1. Supported names/splits
follow the observed table, with independent percent opacity/randomness.

Base Size/Opacity no longer overwrite custom curve point zero. Summary getter
shadowing is repaired. Live counts use every emitter's direct Particle children;
expired branch slots cannot consume the counter cap. Rendering skips opacity=0,
disables the default coverage limit and polls cancellation each scan row;
explicit finite budgets retain their error result. Rendering work is still
proportional to clipped visible coverage. No pixel/host acceptance claimed.

Build 12 SDK and four modified JavaScript syntax gates pass; no tests added/run.
Owner AE 2023.5 Build 52 checks remain open: fresh main creates Emitter+Particle,
multiple Emitters into one Particle, shared downstream Force/Output, copies,
Life/default/steps, XY/Z preservation, Speed=0 random retention, both curves,
native edits/undo/reopen and the original error case. No AE session operated.
Source pushed as cadab2a, then build 12 deployed after immediate AE-absent checks.
Six installed hashes and selected Core match; build 11 retained for rollback;
existing single Junction unchanged.
[Deployment record](native-node-checkpoint.md).

## Build 11 P-02E owner feedback — 2026-10-02

Owner reports canvas deletion unavailable; Effect Controls show redundant
collapsed Output/Emitter/Particle/Force categories. Source review identifies
missing Delete/Backspace handling and rejection of mixed selections containing
fixed Output. Build 11 filters editable selection, pauses polling for menus and
restores canvas focus for marquee/context interactions. Actual native record
removal still uses the established transaction path.

Actual outer group markers are removed from all five effects. Main indices
compact to 1..89 (schema 19); native layout 3 compacts value/record indices.
Surviving disk IDs and Core stay unchanged. Gateway token native-node-sync-11.
SDK build and JavaScript parsing pass; no tests added/rerun or AE session operated.
Source pushed as cb53d50, then installed with AE absent under standing
authorization. Six installed hashes verified; Core/generation unchanged,
single Junction retained, build 10 backed up. Owner checks: single/multiple node deletion,
native effect disappearance, fixed Output mixed selection, direct parameter
visibility, native edits, undo and save/reopen.
[Deployment status and rollback](native-node-checkpoint.md).

## Current build 10 G-06 deployment — 2026-10-02

Owner build-9 evidence: "基本正常但不完全正常", accompanied by graph-evaluation
and output-encoding cancellation dialogs and multiple-active-emitter rejection.
Exact Particle/default/add/delete/bit-depth results were not individually recorded.
New steering reports wire-click disconnect ineffective and requests port snapping.

Source/build 10 clears normal cancellation messages, evaluates multiple emitter
streams under Output's one cap, fixes wire hit testing, and snaps compatible ports
within 22 screen pixels. SDK compilation passes; CEP syntax validation recorded
in the checkpoint. No tests added/rerun; no AE process started/stopped. Source
pushed as c368bbf, then deployed under standing authorization after confirming
AE absent. Six installed hashes and selected runtime Core verified; one Junction
retained, build 9 backed up. All new host behavior remains pending owner testing.
[Before/after state and rollback](native-node-checkpoint.md).

## Build 9 Particle registration gate — 2026-10-02

Build 8 still fails with the same duplicate-matchname error. Every node FourCC
disk ID violated the SDK's 1..9999 contract; a match-name length model also fits
the observed Emitter/Particle difference. Actual host truncation remains inferred.
Build 9 replaces all node disk IDs and supervised lookup with shared numeric IDs,
keeps stream layout, and checks allocation range/uniqueness/name budget at compile
time. SDK build passes; deployed with six hashes verified. No tests added/rerun.
Fresh effects required; Particle creation awaits owner testing.
[Checkpoint](native-node-checkpoint.md).

## Build 8 Particle registration gate — 2026-10-02

Owner build-7 result: Particle still cannot be added, now reporting
`Duplicate matchname found during FillInStreamsFromCanonicalLayout` during
native creation. Node topics reuse start/end disk IDs in source. Build 8 repairs
these structural IDs for all four node modules, retaining value IDs and indices.
SDK compilation passes; six installed hashes verified. No tests added/rerun.
Actual creation remains pending: the identified source defect does not establish
an AE acceptance pass. [Checkpoint](native-node-checkpoint.md).

## Build 7 Particle registration gate — 2026-10-02

Build 6 still cannot create Particle: native addProperty throws a spatial-
interpolation error even though canAddProperty=true. Build 7 removes redundant
interpolation restrictions from constant node controls. That cause is a
hypothesis awaiting AE confirmation; 146 Particle/Appearance fake-host checks
and SDK build pass. Build 7 is deployed; all six hashes match. Actual Particle
addition is pending owner testing. [Checkpoint](native-node-checkpoint.md).

## Build 6 correction from actual AE error — 2026-10-01

Owner reports `FLOAT_COLOR_AWARE requires SUPPORTS_SMART_RENDER` in the test
project and a CEP ResizeObserver delivery warning. Internal node float flags
without implemented SmartFX are a confirmed contract defect. Build 6 adds the
actual node selectors plus matching flags. CEP resize work is deferred/coalesced
and resize-delivery warnings no longer replace native transaction errors.
276 actual-node fake-host checks, focused gateway/startup checks and the SDK
candidate build pass. Build 6 is deployed after specific owner authorization;
all six hashes match the candidate. The owner will test AE. Emitter
duplication is owner-confirmed; Particle default/addition and native 32-bpc
operation remain open until owner testing. [Checkpoint](native-node-checkpoint.md).

## CEP 5b owner evidence — 2026-10-01

Emitter duplication is owner-confirmed in AE 2023. Particle default creation
and explicit addition still fail; native node acceptance is incomplete. CEP 5b
keeps mutation/bootstrap errors visible and adds creation-stage diagnostics.
Failed bootstrap no longer repeats on background polls. Two focused gateway/
startup fixtures pass. No AEX was rebuilt/replaced and no AE session was started.
Actual Particle failure text is required next; see [native-node-checkpoint.md](native-node-checkpoint.md).

## CEP 5a hotfix after owner feedback — 2026-10-01

The owner reports build 5 creates native Emitter effects but repeatedly shows
`stale_graph`; copied effects do not appear in the canvas, and reopening CEP
loses the canvas. **Build 5 did not pass native node acceptance.**

The offending readonly `ensureNodeEffects` path compared a browser reconstruction
against the actual AE records and rejected reload itself. CEP 5a returns the
current Effect Parade records directly. Only mutation checks a host-produced
opaque authoring stamp, so decimal JSON/codec differences do not pretend that
the owner edited an effect. A targeted fixture reproduces rounded decimal JSON
and verifies a readonly reload performs no compile, copy/edit still works, and
a genuine intervening AE value change rejects before adding any effect.
The exact numerical mismatch in the owner's session was not captured; decimal
rounding is a reproduced hypothesis, not confirmed host evidence.

Numeric native payload CRCs remain diagnostic. A browser's rounded projection
cannot prove native byte equality; commit receipt, advancing native revision and
semantic node-record readback confirm a transaction. Additional supervised
callbacks may advance revision beyond exactly one. Failed graph reads keep the
last valid canvas and do not clear/re-show the error banner on every poll.

The gateway/loader token is `native-node-sync-5a`. This changes only workspace
CEP files through the existing extension junction: **no AEX replacement or AE
restart is needed**. Close/reopen the CEP panel. The actual effects are the source;
existing Emitter copies should become visible. If the Particle effect is absent,
add it from the node context menu and connect it. Partial first initialization now fills missing Emitter/Particle and initial links while ready=0; ready=1 deliberate deletion remains unchanged. Fresh-effect automatic bootstrap,
real render response, undo and reopen still require owner confirmation.


## Current native authoring gate — build 5, 2026-10-01

Owner evidence on **AE 2023.5.0 Build 52** confirms build 4 stopped the
layer-selection crash. Node editing remained blocked by
`AEGP_CanVaryOverTime must be true to get an expression`, before node creation.
Build 5 removes every expression read/write from synchronization. The saved
source is each independent node effect's ordinary records; main IDs 41/90/91
hold numeric revision/checksum receipts. Main 43/44 compile/acknowledge.

The May 2023 SDK x64 build passes. **756 adapter checks, zero failures** include
the actual numeric GraphCarrier implementation. Targeted gateway/transaction/startup
checks pass; expression getters/setters deliberately throw in the node fixture.
The fixture exercises initial Emitter/Particle creation, add/copy, independent
values and curves, signed movement, splice/connect/disconnect, native effect
reorder/deletion, raw Ctrl+D re-key, rollback and delete-all. No broad rendering
regressions or repeated AE attempts were run.

The owner authorized deployment and chose to perform host testing. Build 5 is
installed with all six file hashes verified through the existing single
`Plug-ins/Starfield -> dist` junction. No AE session was started by the agent.
Use fresh effects and reopen CEP to load gateway `native-node-sync-5`.
Node creation, actual render/cache response, undo/redo and save/reopen remain
unqualified until the owner's results arrive. Earlier checkpoints below are history.


## Current selection-crash gate — build 4, 2026-10-01

Owner evidence on AE 2023.5.0 Build 52: revision-16 build 3 renders, then crashes
when its layer is selected. The second dump repeats a null read in
`AfterFXLib.dll+0x1931d36`, before node modules load. Build 4 replaces hidden
structural topic markers with invisible scalar slots and leaves one balanced
Output group; the owner now confirms the selection crash is resolved.
721 adapter checks, the May 2023 SDK build, and a checkout-contained single-folder
deployment/rollback check pass. Build 4 is installed through one
`Plug-ins/Starfield -> dist` junction after authorized shutdown of PID 31772.
No new AE session was started. Next acceptance: fresh effect apply/render,
select/deselect its layer safely, then native node addition/copy/deletion.
The checkpoint below describes build 3 and earlier evidence.

## Current host checkpoint — 2026-10-01

Revision-16 build 3 is deployed for **AE 2023.5.0 Build 52**, with the full paired
five-AEX/Core set. The prior build-2 host check crashed after renderer apply and
viewer opening; no native-node add/copy/delete test completed. Build 3 has not yet
been started in AE. Build/cache metadata and group-end initialization were repaired,
but the crash cause and resolution are **unconfirmed**.

The May 2023 SDK build, eight focused CEP suites and 687 adapter checks pass.
They do not qualify AE callback delivery, render response, node creation/deletion,
undo or save/reopen. Exact failure evidence, limitations and the smallest next
acceptance steps are in [native-node-checkpoint.md](native-node-checkpoint.md).
Current installed hashes and one-step rollback are in [build-matrix.md](build-matrix.md).
Earlier installation and expression-mailbox entries below are historical.

Use this file to turn observed behavior into requirements before implementing each feature. Do not infer undocumented internal algorithms from binary details. Static deductions stay hypotheses until a host pass or a reference-effect observation confirms them.

## Host qualification checkpoint

| Check | Host | Status | Evidence / next step |
|---|---|---|---|
| M1 plug-in discovery and load | AE 2023, exact build not recorded | User-confirmed pass for the empty M1 shell | Historical load check only; current H-01 build has its own host evidence below |
| Corrected coordinate candidate (superseded) | AE 2023.5.0 Build 52 | **Full/Quarter origin placement confirmed on this candidate** | May 2023 SDK build on 2026-09-28. This earlier installable file `dist/StarfieldParticle.aex` was 189,440 bytes, packed version `0x8002`, SHA-256 `E2F304BFD3AC13A522CA71635E27F10BF8E0138BED9ACF5FDF4C30697D8B6FA0`; PDB SHA-256 `B0EB472A139D151424E87CF789E7A72B9E4220F94E1CC6FE1452EA016330B075`. AE loaded this candidate. Owner Full/Quarter screenshots and the direct Quarter session show normalized `px 1920,1080,1080` and world `(0,0,0)`. The direct Quarter session reports `grid 960x540`; diagnostic isolation across multiple render contexts remains untested. The current binary and transparency check are recorded in the M2 row below. |
| Build-2 graph render/persistence | AE 2023.5.0 Build 52 | **Save/reopen, Ctrl+D effect duplicate, same-name Ctrl+C/Ctrl+V replacement, undo and redo passed** | On 2026-09-28, saved and reopened `D:\Project\Code\test\testproject.aep` through AE's Open dialog. Comp 1, Medium Blue Solid 1, the Starfield Particle effect, visible particle output, and saved controls (Point, rate 100, origin 1920/1080/1080, velocity Y 0.30, lifetime 2 s, size 10) returned. At that time, the installed `0x8002` candidate matched `artifacts/plugin/2023/x64/Release/StarfieldParticle.aex` (SHA-256 `E2F304BFD3AC13A522CA71635E27F10BF8E0138BED9ACF5FDF4C30697D8B6FA0`). Selecting the effect and pressing Ctrl+D added `Starfield Particle 2` with matching visible parameters; Ctrl+Z removed it, Ctrl+Shift+Z restored it, and Ctrl+Z removed it again. To check same-name copy/paste, duplicated the solid layer, changed the duplicate layer's effect rate from 100 to 25, copied the original layer's Starfield effect with Ctrl+C, selected the duplicate layer's existing effect and pressed Ctrl+V; the target rate returned to 100 and only one effect instance remained on that layer. Undo restored the rate and removed the temporary layer; the original single-layer project was saved. The previously reported missing-file warning did not recur when opening the actual test path; the owner attributes that warning to a path/open-flow mismatch. These checks used AE Controls mode; Node Graph-specific persistence still needs a separate check. |
| Apply-time crash | AE 2023 installed at `D:\Software\Adobe\Adobe After Effects 2023`; plug-in build `0x8002` (24 parameters, `artifacts/plugin/2023/x64/Release/StarfieldParticle.aex`, 21:19) | **Open: crashed once while applying the effect** | Dump `5f8321e1-3dac-4b7e-b3b7-4fa60b9b283b.dmp` (2026-09-27 21:35). Exception `0x40000015` (fatal app exit, not an access violation), raised on a thread whose stack carries `sentry_crashpad.dll` (Adobe crash handler) and NVIDIA OpenGL/D3D12 frames; scanning the captured stacks found no return address inside `StarfieldParticle.aex`, so the fatal exit did not happen under our own frame. The dump also shows the reference `Stardust_panel.aex` and Adobe plug-ins loaded, and our PDB path. Mitigations in the same commit: the Options readout now refuses to check out parameters without a render context, and `STARFIELD_FLAT_RENDER=1` bisects the graph render path against the flat path. Next steps are listed under "Crash triage" below. |
| M2 particle render (superseded) | AE 2023.5.0 Build 52 | **Superseded by the H-01 split pair; its transparency and bit-depth checks carried over** | On 2026-09-28, built with the May 2023 SDK, installed, and loaded candidate `D22D43BAD15C5173867907369B2EF3293A3FD601C308158665A1F3FD0AB0816B` (packed version `0x8002`). Moving the playhead forced a fresh render; the Composition viewer showed particles over the transparency grid rather than the opaque purple solid. The adapter clears the AE output world before copying the sparse particle buffer, because AE may seed that world with source pixels. The row-stride guard build `C0830F649A149990942B40531E25E23C0841FA6BED9C9805B9D3E233AB1ABCAB` was never loaded in AE and is now superseded by the H-01 split pair recorded below. Owner reports all bit depths render normally and time consistency passes; exact frame-comparison procedure is not recorded. Lower-layer compositing remains a useful follow-up visual check. Save/reopen, Ctrl+D, copy/paste replacement, undo/redo and Full/Quarter geometry evidence is recorded above. |
| M3-01 shapes and playback | AE 2023.5.0 Build 52 | **Quarter playback and visual shape distinction passed** | The corrected candidate's Quarter RAM preview advanced visibly. On the later H-01 split build, Box/Sphere/Disc looked distinct from Point. Exact distributions and byte-for-byte out-of-order identity remain open. |
| M3-01B direct emitter dimensions | AE 2023.5.0 Build 52 target | **Core/adapter/CEP checks and build pass; updated AEX candidate deployed, host checks open** | Revision 12 treats Size X/Y/Z as direct full-resolution layer pixels (0–100000, default 100 px); Box/Sphere use all axes, Disc uses its separate Disc Size, and Point/Disc ignore axis values. The latest node AEX set containing this contract is installed, with previous files backed up under `artifacts/disabled/p02d-direct-node-sync-20260930/`; hashes are in `docs/build-matrix.md`. AE remains closed and has not loaded the updated candidate, so visual size behavior and lifecycle remain unverified. |
| M3-05 Particle size/opacity randomness | AE 2023.5.0 Build 52 target; candidate not installed | **Core, adapter, CEP checks and May 2023 SDK build passed; host gate open** | Candidate AEX SHA-256 `D367A3830A23F312E1D3150DBB392ADDF9E5073A182E40836AAA86458A188E6D`; Core DLL SHA-256 `D77BD11088A5D54B479FDA19FFED5183F8E8D85CC4490D0AB931F74B1678CC97`. Core: 11,082 checks; adapter: 676 checks; graph-view/edit, gateway and startup checks pass. Size Random and Opacity Random vary independently per stable particle ID after their life curves. Stardust's exact random distribution, AE visual response, undo, and save/reopen remain to be checked. No host file was replaced. |
| M3-06 Particle lifetime/curve and alpha candidate | AE 2023.5.0 Build 52 target; host gate open | **Source, focused checks, May 2023 SDK build, and AEX deployment passed; AE visual/project-lifecycle gate open** | Revision 13 defines both curves as 0–100% multipliers of base Size (pixels) and Opacity (0…1). Particle owns branch lifetime; the native adapter preserves curve point zero. Installed AEX SHA-256 `B646EF14937700FB31CBA8A957076899231A4D472C9EF6A1DAB460FBB4426560`; selected Core SHA-256 `DEF5B7804EBEB6653EE70A4F5B13EC95F14366184D35839DFB5A5B5EA4999F2B`. Core tests passed 11,905 checks; focused graph-view/edit/transaction, gateway, and startup tests passed. The gateway regression covers editing Size while preserving an existing Opacity curve with values above 1. The candidate requests straight-alpha output after the earlier render produced dark translucent particles over blue. The AEX was copied into the AE 2023 plug-in root on 2026-09-30 after a backup to `artifacts/disabled/`; AE remains closed and no render has been performed with this installed pair. Curve persistence, alpha appearance, undo, and save/reopen remain unqualified. |
| M3-02 force/appearance | AE 2023.5.0 Build 52 | **Gravity and size changes passed visually on H-01** | Gravity, drag, color and size/opacity age curves have core coverage. On the split build, `Gravity Y = -2` and `Size Over Life = 1` made the trail fall and shrink. Drag, color and opacity still need host visual checks. |
| P-02 dockable panel | AE 2023.5.0 Build 52 | **Discovery and one panel write passed; protocol checks remain open** | Through the installed Junction, the form populated after selecting the effect layer without Refresh and showed `Lookup: name`. A panel `Size` edit updated the AE frame; host undo restored the picture. The panel initially displayed a stale value after undo. A focus-triggered re-read was added and the owner reports it updates; redo, stale-state rejection, graph synchronization and panel-authored persistence remain open. |
| P-02A project-saved node layout | AE 2023.5.0 Build 52 | **Revision 7 installed; layout lifecycle qualification open** | Revision 7 appends eight hidden scalar streams and the CEP gateway reads/writes the complete layout in one undo group. The owner confirmed the node canvas displays normally and reports no issue after installing this AEX. Still qualify parameter name resolution, node move → undo/redo, effect duplication, save/close/reopen, and dragging nodes left/up beyond the previous bounds. |
| P-02B graph transactions | AE 2023.5.0 Build 52 | **Revision-15 AEX installed; AE 2023 host retest pending** | The copy operation reported that AE could not set `expression` or `expressionEnabled` on the hidden `Graph Edit Request` property. Source inspection found `PF_ParamFlag_CANNOT_TIME_VARY` on that stream. Revision 14 cleared the flag in source, but the owner still reported `canSetExpression=false`; revision 15 adds a fresh expression-capable request at ID 90, with a capability preflight before node-effect mutation. The installed revision-15 AEX SHA-256 is `EFAEE26451AC42B68D7FAFDD22A710514EA3A7E65BC5276CD7FE6B5493447DB7`; it adds active graph request ID 90 while retaining marker ID 89 for node materialization/deletion reconciliation. AE is closed and has not loaded it. Add/copy/delete/reconnect, undo, persistence, and render response remain untested. |
| P-02D per-node AE effects and fixed Output | AE 2023.5.0 Build 52 target; candidate installed, AE closed | **CEP/native-node source integration, focused suites, and May 2023 SDK build pass; host behavior open** | Installed `StarfieldParticle.aex` SHA-256 `CF6E08544CE81495932DDA3CD0B240C29A899E5EC435303D3170FA48555F616D`; selected Core SHA-256 `55B877A8F66D357649CD72938FB883A3C9F5CF584A07ED516E7494828D7C918E`. Node AEX hashes: Emitter `CC19F9DED39165FB904ECB726AC0624045D4F2649D85D84BC387DB821F4F5FC7`, Particle `31BA2D394E7C8628D77A9648C7D775A8B8EA09485CCF8BD520AB7240986057D7`, Appearance `70709CCB724CEF7FBF43CF5118BEEF042744278703137B3F67F469F1F703586A`, Force `CF6796CCF026ED1BBFD41CF98847C8A9E838139C81F112D6F7D364F7CC8DA7EE`. CEP transactions create/remove these node effects, write values, and commit graph data to the main renderer; the fixed Output terminal stays visible in CEP but is omitted from the native-effect manifest and has no AEX. Core: 11,909 checks; adapter: 678; seven focused CEP suites pass. The authorized deployment on 2026-09-30 backed up the prior main AEX and `current.txt` under `artifacts/disabled/p02d-node-sync-20260930/`; the existing runtime junction selects the new versioned Core. The prior installed AEX was `B646EF14937700FB31CBA8A957076899231A4D472C9EF6A1DAB460FBB4426560` with Core `DEF5B7804EBEB6653EE70A4F5B13EC95F14366184D35839DFB5A5B5EA4999F2B`. AE remains closed, so module loading, node add/remove, undo/redo, duplicate identity, save/reopen, and rendered response are unqualified. Direct node-control synchronization was added in a later source candidate; see the following row. |
| P-02D direct native parameter synchronization | AE 2023.5.0 Build 52 target; AEX candidate installed, AE closed | **Source prototype, focused gateway check, and May 2023 SDK build pass; host behavior open** | Direct node Effect Controls edits forward one constant typed value through the main renderer's supervised graph callback. CEP batch writes set a hidden per-node guard to prevent intermediate commits. The source controls are non-time-varying under the current constant graph-value contract. The installed main AEX SHA-256 is `3277A6F65567D58E67CD64F4C72AB7603CB38BC0720CC9CF3C5EAB4F0ACCAFEF`; the replaced main AEX is backed up under `artifacts/disabled/p02d-node-nonce-fix-20260930/`. The four node AEXs remain from the previous candidate, and the selected Core/selector is unchanged. The earlier five-file backup is under `artifacts/disabled/p02d-direct-node-sync-20260930/`. The build used `-NoRuntimePublish` and did not change the runtime selector. AE callback delivery, cache/render response, undo/redo, and save/reopen remain unqualified. |
| P-02D menu-hidden internal modules | AE 2023.5.0 Build 52 target; four node AEXs deployed 2026-09-30, AE closed | **Deployment hash-verified; AE menu and CEP creation checks pending** | Emitter `16C9FA717264D445BB7ECBFEE92920DC318D3774212FDA4364E8D1E72B0A9494`, Particle `2439EBE6550A3369D4EBB3894931CFEDFA7544F64DC9DAF5D55CC130E65E9169`, Appearance `0AF3A3C450E5B092DA462D11FD9DD71F5716F14D01FFD0C6F16ECCD9D5502CBF`, Force `E755C294E0B5654F662132E4AECE3490A701B982D054A8BB493E01825711EF5E`. The replaced files are backed up under `artifacts/disabled/p02d-hide-node-menu-20260930/`. The main renderer AEX and selected Core DLL were not changed. Verify only Starfield Particle is listed in the Effects menu and CEP can still add hidden nodes by match name; AE has not loaded this candidate yet. |
| P-02C Size/Opacity over-life curves | AE 2023.5.0 Build 52 | **Panel and native source checks pass; AE host gates open** | The Particle inspector draws independent Size and Opacity percentage polylines with knot insertion, drag, removal, selection, and numeric/scrub editing. AE Controls mode uses project curve-bank streams; Node Graph mode writes optional curve payloads with scalar endpoints through one graph transaction. Both curves use fixed 0–100% ordinates. The gateway regression confirms that a Size edit preserves a custom Opacity curve, including values above 1. Exercise curve edits in both modes, rendered size/opacity response, undo/redo and save/close/reopen with the matching candidate. |
| Delivery examples | AE 2023; exact build not recorded | Not checked | Apply Spark, Snow and Floating Light from `docs/examples.md` and record a still frame at t ≥ 1 s for each |
| M2 point-control units | AE 2023.5.0 Build 52 | **Measured:** absolute pixels scaled by preview resolution | Full/Quarter readouts show center `[1920,1080,1080]` → `[480,270,270]` as `ds` changes 1/1 → 1/4. The adapter restores the per-axis preview scale and the core preserves absolute pixels, including off-layer positions. A newer AE 2023 build needs its own host readout. |
| M2 preview geometry | AE 2023.5.0 Build 52 | **Single-effect Quarter grid observed; cross-context isolation remains open** | With the Composition viewer explicitly set to Quarter and the test comp/effect rendering, Options reported `ds 1/4,1/4`, `ref 3840x2160`, `grid 960x540`. This resolves the earlier mismatch caused by reading the right-side Preview panel while the Composition viewer was still Full. The diagnostic is process-global, so multiple effects or comps could overwrite the last-render record; that separate isolation behavior has not been checked. |
| M2 preview-resolution emitter origin | AE 2023.5.0 Build 52 | **Full/Half/Third/Quarter centre normalization passed** | Corrected-candidate Full/Quarter readouts and H-01 Half/Third readouts all normalize raw points to `[1920,1080,1080]`. Quarter playback placed the trail near the expected centre. Off-centre, anisotropic and exact image comparisons remain open. |
| Newer AE families | Deferred by owner direction | Deferred | No current adaptation or qualification work |

### P-02D hidden node-module candidate (built, not installed)

On 2026-09-30, the May 2023 SDK build passed with `PF_OutFlag_I_AM_OBSOLETE`
set on the four node modules in both PiPL and runtime flags. The local AE 2023
SDK says these effects stay out of the Effects menu while remaining available
for existing project instances. The candidate hashes are recorded in
`docs/build-matrix.md`. It has not been copied to the AE plug-in directory, and
AE has not tested whether CEP can add a new hidden module by match name. The
currently installed main AEX and `StarfieldCore-55B877A8F66D3576.dll` selector
were not changed by this build.

### Fixed suspect: the plug-in freed a host-owned handle

`capture_controls` and the Node Graph sync path replaced the graph parameter's value and then
disposed the handle they replaced. Parameter values belong to the host: AE frees the value it
replaced once the change is committed, so freeing it a second time corrupts the handle table and
makes AE abort later — on a thread that no longer holds any plug-in frame, which is exactly what the
dump shows (`0x40000015`, crash handler thread, no `StarfieldParticle.aex` return address). Both paths
now hand the new handle to the parameter and leave the old one to the host, and the sync path refuses
to touch a parameter array that is not fully registered and type-correct (apply/undo can deliver a
partially built array). The adapter suite pins the new ownership rule.

## Superseded diagnosis: emitter offset at reduced preview resolution (2026-09-27)

The first cause identified below was incorrect. AE 2023.5.0 Build 52 readouts supplied on
2026-09-28 show that point controls are preview-scaled; using the full-resolution reference alone
did not correct the point value. The code and test added after that measurement are the current fix.

The first attempted fix only passed the full-resolution reference into conversion. It assumed the
raw point control stayed at full resolution; that assumption was wrong. The owner supplied the
readouts that were missing in the earlier pass, and the 2026-09-28 row above records the corrected
cause and the replacement fix. The earlier Full-only capture restriction is also removed: capture
and Node Graph synchronization now use the same preview-aware point conversion as rendering.

## Crash triage (apply-time fatal exit, 2026-09-27)

Ordered experiments; record each result here before moving on. Do not "fix" anything before the
bisection says which layer is involved.

1. **Renderer and depth:** Project Settings > Video Rendering and Effects > **Mercury Software Only**,
   and an **8-bpc** composition. Apply the effect. A crash that disappears here points at AE's
   GPU/float compositing path, not at our renderer.
2. **Graph vs flat:** set the environment variable `STARFIELD_FLAT_RENDER=1` (close AE first;
   `setx STARFIELD_FLAT_RENDER 1`), restart AE, apply again. The Options readout prints
   `STARFIELD_FLAT_RENDER override: rendering flat controls` when it is active. This bypasses the
   stored graph and the arbitrary-data parameter during rendering.
3. **Isolate the plug-in:** move `StarfieldParticle.aex` out of the plug-ins folder and confirm AE
   applies other effects normally. Then put it back and apply it to a **new comp with one solid
   layer** (no other effects, 8-bpc, software-only).
4. **Record what AE says:** any error dialog text, and whether the crash happens on *apply*, on the
   *first preview frame*, or only when the ECW is opened.
5. **Capture:** with `STARFIELD_FLAT_RENDER=1` still set, reproduce and keep the new `.dmp` plus the
   exact AE build from Help > About. Our `.pdb` ships next to the `.aex` in `artifacts/`, so the next
   dump that contains a plug-in frame can be symbolized.

## M2-06 host smoke checklist

Load the current build once, then record host, build, and result per row:

1. **Load** — "Starfield Particle" appears under `Starfield FX` and applies without an error dialog.
2. **Controls** — the effect shows the implicit input, the thirteen original controls, Control Source, Capture Current Controls, then Gravity X/Y/Z, Linear Drag, Color Start, Color End, Size End and Opacity End. Node Graph Data remains hidden.
3. **First pixels** — defaults render the seeded emitter; at t ≥ 1 s, changing Velocity Y from `0.3` to `0.5` layer heights/s produces a visibly faster upward trail.
4. **Options readout** — click the effect's `Options` button and record the whole text. It reports the graph/control source and live count, the layer size with the downsample factor, the reference and grid the last rendered frame used, the raw emitter point with both world interpretations, time and velocity, then the counts. `grav`/`col` appear only when they differ from the defaults, and the tail can be dropped by the 255-character buffer. This is the fastest way to classify a rendering surprise, and step 1 of the D-05 measurement; see `docs/parameter-mapping.md`.
5. **Force and appearance** — with default values the picture must be identical to the previous build. Then set Gravity Y = `-2`, Linear Drag = `0.5`, Color End to red, and Size End = `1`: the trail must fall, slow down, warm toward red, and shrink along its age.
6. **Determinism** — scrubbing forward and backward over the same frames renders identical frames.
7. **Rate and lifetime** — Birth Rate and Particle Lifetime change the trail length; Particle Count caps how many sprites can be alive.
8. **Size and opacity** — both visibly change the sprite; size `0` renders nothing.
9. **Alpha output** — with a solid source layer, only particles have alpha; pixels between particles are transparent and layers beneath the source show through. The effect must not toggle the source layer's visibility.
10. **Bit depth** — owner reports 8/16/32-bpc rendering normally on the previous candidate; repeat alpha-output check after the `REVEALS_ZERO_ALPHA` build is installed.
11. **Lifecycle** — duplicate the effect, undo/redo, copy/paste, save, reopen, and render through the Render Queue.
12. **Cancellation** — start a RAM preview on a heavy setting and stop it; the effect must abort without an error dialog.
13. **Panel** — install `cep_panel/` per its README, confirm the chain renders, edit one value, undo/redo it, then re-open the panel and confirm the values are current.
14. **Examples** — apply the three recipes from `docs/examples.md` and capture one still frame each at t ≥ 1 s.

## Options readout

In the H-01 build, the effect's `Options` button first attempts to load the
versioned DLL named by `StarfieldRuntime/current.txt`. A successful switch
requests a fresh render; a failed switch keeps the prior generation. It then
prints a diagnostic summary in the order that matters when
something looks wrong: graph/control source with the live count, the layer size and downsample factor,
the reference and pixel grid of the **last rendered frame**, the raw emitter point with both world
interpretations (the readout's fallback and the frame's reference-based one), time and velocity, then
the remaining counts. `grav`/`col` are printed only when they differ from their defaults, because
`PF_OutData::return_msg` holds 255 characters and lines past the end are dropped — the earlier layout
put the origin lines there and silently lost them. It changes no pixels and no settings; the
interpretation table and the two-click D-05 measurement are in `docs/parameter-mapping.md`.
The superseded monolithic build had a read-only Options button.

## H-01 AE 2023 hot-core qualification (partial host pass, 2026-09-28)

The authorized development installation is in
`D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins`.
The initial AE 2023.5.0 Build 52 split-build smoke pass loaded
`StarfieldParticle.aex` SHA-256
`B7362B01AC0E935D8AD596A70D61690DA4D586EEEC3E939328BA1BDC420069D5`.
The installed AEX was first updated on 2026-09-29 to
`7BFE7092325C9AEE9E777DEDBFE31D5042249F0A4A78A11B35E23BAE0A1D3EB9`
for the Options readout. It was then updated to
`AEF074782242E9C76781DDF0FC43C197064F387C38D1AF0FE680490BB7EFC034`
for strict runtime-manifest parsing; both focused host results are recorded below.
The pinned fallback `StarfieldCore.dll` is
`A4F104B5858DE5938F87B93D4B59FF89A5E324CD238DFDB3AD67B31327CD2545`.
`StarfieldRuntime` is a junction to this checkout's ignored `artifacts/runtime/`;
`current.txt` currently selects `StarfieldCore-095219764514FFCA.dll`, SHA-256
`095219764514FFCA1A5C3CFD36D78E6C8368ECF32C6C17576A8C26F2FA584564`.
The 2026-09-28 live AE process (PID 36424 at that check) mapped that selected generation.
The prior monolithic AEX remains under `artifacts/disabled/h01-ae2023-crt-before-20260928`.

The first `/MD` split build crashed when opening the project. Dump
`f9992d57-3c46-4b68-9736-0d836874a46d.dmp` recorded a null read in AE's
app-local `MSVCP140.dll` while `CoreLoader.cpp:195` locked the loader mutex;
no core DLL had loaded. That dump was not copied into `artifacts/crash/`, which
holds only the earlier `5f8321e1-3dac-4b7e-b3b7-4fa60b9b283b.dmp`. AE bundles version 14.00.24210.0, older than the v145
toolset's STL. Both modules now use `/MT` in Release, keeping their CRT state
inside the module and their C ABI free of CRT-owned pointers. `dumpbin /dependents`
shows only `KERNEL32.dll` for both modules. The rebuilt pair opened and rendered
the same saved project without that crash.

Host observations on the `/MT` build:

- AE stayed in one process while Options switched the selected Core from normal
  (white) to a temporary red-only rasterizer and back at Full and Quarter.
  Preview pixels updated after each switch. The installed AEX hash stayed fixed;
  the old generation unloaded after the new one loaded. Quarter Options reported
  `ds 1/4,1/4`, `ref 3840x2160`, and `grid 960x540`.
- Selecting a nonexistent DLL in `current.txt` made Options report `cannot hash
  selected core DLL for the AE cache key`; the prior Core and visible frame were
  retained. The valid manifest was restored.
- `D:\Project\Code\test\testproject.aep` was saved, AE closed normally, and the
  project reopened through AE's Open dialog in a new process. The effect, Node
  Graph control mode, saved values, particle preview, and CEP panel target returned.
  The panel populated after layer selection without pressing Refresh.
- With the split build, the same project visibly rendered particles at 8, 16,
  and 32 bpc in Quarter. The 8-bpc transparency-grid view showed only particles
  over the grid, without the solid's opaque source color. Project depth was
  restored to 8 bpc and saved. This is a visual smoke check, not a pixel-diff
  comparison across bit depths or against the monolithic binary.

Extension acceptance pass on the same `/MT` split pair (owner-operated, 2026-09-28):

- **Effect duplicate, undo/redo and same-name copy/paste passed on the split build.**
  Ctrl+D added `Starfield Particle 2`, Ctrl+Z removed it, Ctrl+Shift+Z restored it and a
  second Ctrl+Z removed it again. Pasting the original effect over the duplicate layer's
  existing instance restored its `Particles Per Second` to 100 and left exactly one
  instance on that layer. This closes the earlier "copy/undo evidence predates the split
  build" gap.
- **Half and Third preview were measured for the first time.** Half reported
  `layer 3840x2160 ds 1/2,1/2 ref 3840x2160 grid 1920x1080` with
  `org host 960,540,540 px 1920,1080,1080`; Third reported `ds 1/3,1/3`,
  `grid 1280x720` and `org host 640,360,360 px 1920,1080,1080`. Both normalize to the
  same centre, so the preview-scale fix holds at every resolution measured so far.
- **Source compositing confirmed visually:** a solid layer placed below the particle
  layer stays visible between the particles, and the effect leaves the source layer's
  own visibility untouched.
- **Emitter shapes and the force/appearance stages are now host-confirmed.** Box, Sphere
  and Disc produce visibly different birth distributions from Point. `Gravity Y = -2`
  with `Size Over Life = 1` made the trail fall and shrink with age, and the readout
  printed `grav 0.00,-2.00,0.00 drag 0.000` for that frame.
- **Stacked instances:** two Starfield Particle effects on one layer render only the
  topmost one. This is the written contract, not a defect — the effect writes particles
  over transparent black, never copies its input, and pre-render requests an empty input
  rect, which lets AE skip the upstream instance entirely. Recorded because it is
  user-visible and may need a product decision.
- **Options readout truncation fixed in AE 2023.5.0 Build 52 (2026-09-29):** the
  rebuilt AEX SHA-256 `7BFE7092325C9AEE9E777DEDBFE31D5042249F0A4A78A11B35E23BAE0A1D3EB9`
  was installed over the previous `B7362B01…` AEX after copying that file to
  `artifacts/disabled/StarfieldParticle-before-options-20260929.aex`. The selected
  Core generation and pinned fallback DLL were unchanged. In the saved test project,
  the Third-resolution `Options` dialog showed `SF AE g4/3 live200`,
  `L3840x2160 ds1/3,1/3 ref3840x2160 grid1280x720`,
  `shape0 esz0.050 vspr0.15 sz10.00 not0`, `org952,487,360 px2856,1460,1080`,
  `world 0.433,-0.176,0.000`, and the complete time/count lines. White particles
  remained visible over the blue lower layer. This confirms the shape line and
  trailing count line fit for this case; 255 characters can still truncate optional
  force/color lines or longer numeric values. `Options` left an unsaved marker in AE's
  title; the session closed with **Don't Save**, and the test project on disk kept its
  2026-09-28 22:51:45 modification time. Whether `Options` itself dirties the project
  needs a separate isolated check. AEX-only rollback after AE exits:
  `Copy-Item -LiteralPath 'artifacts/disabled/StarfieldParticle-before-options-20260929.aex' -Destination 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins\StarfieldParticle.aex' -Force`.
- **Two-line runtime manifest rejected in AE 2023.5.0 Build 52 (2026-09-29):**
  the new installed AEX SHA-256 `AEF074782242E9C76781DDF0FC43C197064F387C38D1AF0FE680490BB7EFC034`
  was built with the May 2023 SDK and replaced the `7BFE7092…` AEX after backing
  it up to `artifacts/disabled/StarfieldParticle-before-manifest-20260929.aex`.
  With two valid DLL basenames on separate lines in `current.txt`, Options showed
  `Core reload: invalid core runtime manifest filename`; the prior generation
  and particles over the blue lower layer remained visible. The original
  one-line manifest was restored (SHA-256
  `2DB62F2244AF22AB33D2370CD515E188D2C7B384D4FD3AB1152C39EEB17B3F75`),
  and Options then showed `Core: current DLL` in the same AE process. AE exited
  normally with **Don't Save**; `testproject.aep` stayed at 106,687 bytes and
  its 2026-09-28 22:51:45 modification time. The installed AEX hash and manifest
  hash were rechecked after exit. The Core DLLs and development junction were
  unchanged. To roll back this AEX after AE exits:
  `Copy-Item -LiteralPath 'artifacts/disabled/StarfieldParticle-before-manifest-20260929.aex' -Destination 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins\StarfieldParticle.aex' -Force`.
  This checks malformed-manifest fallback; the separate render and parity pass
  below covers a different H-01 gate.
- **Render queue and sampled monolith parity passed in AE 2023.5.0 Build 52
  (2026-09-29):** `aerender.exe` rendered `Comp 1` from
  `D:\Project\Code\test\testproject.aep`, frames 51–53 at 3840×2160, Full,
  8 bpc, Best Settings, MFR off, to `Multi-Machine Sequence` PSDs with
  RGB + Alpha and premultiplied color. The split adapter was
  `AEF074782242E9C76781DDF0FC43C197064F387C38D1AF0FE680490BB7EFC034`
  with selected Core `095219764514FFCA…`; the prior AE-qualified monolith was
  `D22D43BAD15C5173867907369B2EF3293A3FD601C308158665A1F3FD0AB0816B`.
  `node tools/Compare-PsdFrames.cjs <split.psd> <monolith.psd>` decoded the
  flattened PackBits RGBA channels and found **zero differing pixels** on each
  of the three frames. The frames contain 7,900, 7,947 and 8,109 pixels whose
  RGB differs from the blue background, respectively. PSD file hashes differ
  because resource metadata differs; decoded channel hashes match. As a cache
  control, an alternate Core (`ED024907207169D9…`) with the same AEX and frame
  51 changed 7,818 pixels, showing that `aerender` did not simply reuse the
  previous normal-Core image. `aerender` reported disk cache **Read Only** and
  exited without saving the project. The current AEX and one-line manifest were
  restored and their SHA-256 hashes rechecked; the project size/time stayed
  106,687 bytes / 2026-09-28 22:51:45. Output frames and command scratch are
  under ignored `artifacts/reports/h01-parity/`. This proves parity for these
  rendered frames, not all depths, ROI shapes or parameter combinations. The
  in-flight AE render switch remains open. AEX-only rollback after AE exits:
  `Copy-Item -LiteralPath 'artifacts/disabled/StarfieldParticle-before-parity-20260929.aex' -Destination 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins\StarfieldParticle.aex' -Force`.
- **Panel write works; host undo initially left a stale panel value.** Editing `Size` from 10 to 40 in the panel
  updated the AE frame immediately, and Ctrl+Z restored both the value and the picture —
  but the panel kept showing 40 until it re-read host state. The protocol has no push
  channel; a focus-triggered re-read was added and the owner reports that it updates.
- **Panel/host value divergence, root-caused and fixed:** on the same layer the panel
  displayed `Origin 836,1732,1080`, `Velocity Y 0.1` and footer `Mode: Node Graph` while
  the Effect Controls window showed `1920,1080,1080` and `0.30`. The client only re-read
  on load, on Refresh and after its own writes, so it kept showing the snapshot it last
  adopted — including that stale mode line. `panel.js` now re-reads when the panel
  regains focus and skips a pending write; the owner reports the values update again.
- **Control animation works, and only in AE Controls mode.** The owner keyframed
  `Origin` and the emitter animated. That matches the code path exactly: with
  `Control Source = AE Controls`, pre-render rebuilds the graph from parameters checked
  out at the frame's time; with `Node Graph` it reads the stored bytes, which hold
  constants. Animation and Node Graph mode are therefore mutually exclusive today, and
  graph-side history needs M3-03's contract.
- The owner's verdict at the earlier panel checkpoint was that the tested panel was
  still a parameter form rather than a node canvas. The later P-02A source now draws ports,
  routed connectors, draggable node cards, and a floating properties window; the owner has
  since confirmed that the node canvas displays normally. It remains a fixed four-stage view
  until P-02B.

Repository gates still pass: 6,196 core checks, 395 adapter checks, and the
two-generation loader harness including a real pinned render and failed-reload
fallback. `-CoreOnly` leaves the AEX hash unchanged. Three Full-resolution
render-queue frames now have decoded RGBA parity with the prior monolith.
**Open host gates:** switch while an AE render is actually in flight; broaden
pixel parity across depths, ROI and parameters; exercise repeated switches,
reverse-time requests and cancellation. The rollback command (run after AE closes) is
`powershell -NoProfile -ExecutionPolicy Bypass -File tools/Deploy-HotCore.ps1 -Rollback -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins'`.
Rollback has been prepared and backed up; it has not been exercised, since the
tested split pair is still installed for development.

## Behavior status

| Area | Independent acceptance case | Status |
|---|---|---|
| Emitter | Emitter positions are deterministic from seed, rate, lifetime, and absolute time | Implemented for Point/Box/Sphere/Disc; the seed selects stable per-particle streams. Determinism and frame order are covered by `tests/core_tests.cpp` |
| Particle lifecycle | Birth rate, lifetime, age, and population cap behave consistently at arbitrary frame order | Implemented; half-open lifetime and newest-slot cap are documented in `docs/parameter-mapping.md` |
| Emitter shapes | Box/sphere/disc distributions produce deterministic positions | Core determinism is covered by tests; AE 2023.5.0 Build 52 visually confirmed Box, Sphere and Disc differ from Point on the split build. Exact distribution matching remains open |
| Per-particle variation | A steady emitter animates on playback instead of looking frozen | Implemented in core (M3-01): per-particle birth offsets plus per-axis velocity spread from `core::Random`, covered by core tests. Host playback confirmation pending |
| Random Seed | Changing the seed changes the rendered pixels | Implemented (M3-01): the seed now keys every per-particle stream. Host confirmation pending |
| Forces | Each force has isolated enable/disable and stable parameter semantics | Gravity and linear drag are implemented in one force stage. AE 2023.5.0 Build 52 visually confirmed `Gravity Y = -2` changes the trail on the split build; drag and per-force enable/disable remain unqualified or unimplemented respectively |
| Ages and appearance | Size, opacity, and color follow particle age | Core age curves are covered by tests; AE 2023.5.0 Build 52 visually confirmed a shrinking trail with `Size Over Life = 1`. Opacity and color curve visuals remain open |
| Nodes | Graph connections validate cycles, missing inputs, and invalid references without crashing | G-05 defines direct Emitter → Particle links, deterministic Particle fan-out, branch-local ordered force accumulation, parallel merge by particle identity, and a bounded traversal budget (ADR 0015). P-02B submits dynamic graph edits in source through the revision-checked expression carrier; focused panel tests cover edge-only redraw and saved-layout recovery after rejection. The AE carrier and render behavior for edits are not qualified; add/delete/rewire tests remain an AE gate. |
| Rendering | Alpha, premultiplication, color depth, rowbytes, ROI, and downsample are explicit | Core accumulates particles in premultiplied form and now encodes the requested output mode; the revised AE candidate requests straight alpha after the previous candidate rendered dark translucent particles over blue. Core has 8/16/32-bpc unit coverage, but the revised candidate still needs AE transparency-grid and blue-layer confirmation. Earlier opaque/transparent-output checks and render-queue pixel parity apply to their recorded candidates only |
| Reloadable core | A new core algorithm builds and renders in AE without restarting or replacing the AEX | AE 2023.5.0 Build 52 visually confirmed Full/Quarter white → red → white reload with unchanged AEX/process, missing-DLL fallback, and sampled monolith frame parity. An in-flight AE render switch remains open (H-01) |
| Preview resolution | The same frame at Full/Half/Quarter puts particles in the same comp positions | Full/Half/Third/Quarter centre normalization is host-confirmed in AE 2023.5.0 Build 52; Quarter playback advances visibly. Exact reverse-time image comparison remains open |
| Compositing | Effect output contains particles with transparent pixels; the input layer's solid color is not copied | AE 2023.5.0 Build 52 confirmed the transparency grid and a lower solid visible between particles on the split build. Two instances stacked on one layer display only the topmost effect under the current input-independent contract |
| Color management | Working-space conversion through documented AE suites | Not started; M2 performs no conversion (ADR 0005) |
| Control shape | Positions use point controls, rates use scalar sliders | Emitter Origin is a 3D point and X/Y/Z velocity are sliders; AE 2023.5.0 Build 52 exercised these controls on the split build. Capture and stored graph bytes need separate qualification |
| Emitter origin | The point control places the emitter and the render agrees with it | The split build's Full/Half/Third/Quarter readouts normalize the centre to `[1920,1080,1080]`; off-centre visual parity remains open |
| Point-control scaling | AE point values map to the same layer position at Full and Quarter | Host readouts confirm preview-scaled raw points normalize to the same comp coordinates at Full/Half/Third/Quarter in AE 2023.5.0 Build 52 |
| CEP node canvas | The dockable panel visibly presents nodes, ports, connectors, movable layout and editable node properties | Source projects nodes and edges from the project graph, persists UUID-keyed layout in the graph record, and routes topology and inspector edits through bounded transactions. The owner confirmed the canvas display and reproduced the blocked graph mailbox on the prior build. The revision-15 AEX is installed but not loaded; dynamic editing, deletion reconciliation, carrier undo, and persistence remain unobserved. |
| Persistence | Save/reopen, effect copy and undo preserve graph identity and values | On the split build, save/close/reopen retained visible Node Graph mode and values; Ctrl+D, same-name paste, undo/redo passed. A byte-level stored-graph comparison and build-1 migration remain open. Capture samples current-time constants and does not convert animation tracks |
| Concurrency | Repeated concurrent renders return identical pixels and never mutate shared state | Not advertised (no MFR flag); the core render is a pure function of one request, which M6 must audit before claiming support |
| Cancellation | A host abort stops a long render predictably | Implemented through `PF_ABORT` polling in the simulation and rasterizer; unverified in a host |
| Bounded work | Extreme settings fail with a typed error instead of hanging the host | Implemented (`work_limit_exceeded` + manifest caps); the specific budget is a provisional constant pending M6 profiling |
| Presets | Import/export validates version and rejects malformed or oversized data | Not started as file import/export (M4-03). The three delivery examples ship as documented recipes and panel presets; the graph codec already provides the bounded, versioned container a preset will use |
| Panel | UI state synchronizes through a versioned protocol and tolerates disconnect/restart | Protocol v1 implemented in `cep_panel/` with bounded requests, typed errors and stale-state rejection; reads/writes go through supervised parameter streams (ADR 0009). Host qualification is the open gate; file import/export presets remain M4-03 |

### P-02D reverse deletion reconciliation candidate (2026-09-30)

The May 2023 SDK x64 Release build includes schema revision 14 and the hidden
project marker `Node Effects Ready` (parameter ID 89). Focused gateway,
transaction, and startup suites pass; the AE adapter fake-host suite passes 682
checks. Main AEX candidate SHA-256:
`9B3D75B2AA9E1EC1DED90F0993DCB7E66DD28FFC91DDB11968E656C1D9204017`.

The candidate is installed at the AE 2023 plug-in path with the hash above. The
previous main AEX (SHA-256
`6301092C5D0E7A4C9B3FC646488A3F364BF76A97FF5115B75DCE83C1F1B22897`) is backed
up under `artifacts/disabled/p02d-node-delete-sync-20260930/`. AE remains closed
and has not loaded the revision-14 build. AE 2023 must verify first
materialization, manual Effect Controls deletion pruning the matching graph node
and incident edges, undo/redo without repeated pruning, and save/close/reopen.
The graph cleanup after a manual Effect Parade deletion is a separate undoable
transaction; its interaction with the native deletion's undo record is not yet
qualified. To roll back, restore the backed-up AEX to the plug-in path while AE
is closed.
