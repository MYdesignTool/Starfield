# Development plan: AE 2023 particle Alpha

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
save/reopen are separate AE acceptance gates. Partial/absent sequence parameters
use a balanced checkout of the main effect's own graph ARB. Source metadata can
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

## M3-06 GPU design correction — 2026-10-03

Use AE native GPU selectors and framework negotiation, with CUDA/OpenCL backends
for the current May 2023 SDK. Per-device setup, per-frame GPU eligibility, shared
GPU_BGRA128 output and device teardown must all be implemented before capability
advertisement. No private D3D12/OpenGL context or full-frame readback. Preserve a
CPU selection/fallback and measure scene preparation separately from GPU work.
DirectX/newer-host adaptation is deferred. See
[ADR 0026](adr/0026-reference-particle-and-native-gpu.md); implementation and actual
AE GPU qualification remain open, and build 22 stays CPU-only.

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



## M3-03 / build 21 — emitter Origin birth history

Build 20 owner playback renders but reveals current Origin moving all survivors.
Pre-render now samples owned Origin XY/Z at birth times and carries a transient
immutable history record to Core, shared across Auxiliary parent evaluations.
Ordinary velocity/force displacement stays relative to each birth position.
Wide-time dependency declarations match PiPL/runtime; history never persists in
project data or a process-global cache. IDs, Render.hpp and C ABI 2 stay stable.
921 scoped checks and the paired SDK build pass. Owner trajectory/interpolation,
cache invalidation after past-key edits and reopen need AE qualification.
Other animated clocks/birth controls and Force integration are separate work.

## P-02J / build 20 — SmartFX delivered parameter count

Owner build-19 opening error reports binding stream -1. Sampling no longer treats
PF_InData.num_params as the registered stream count; SmartFX passes no params[].
Actual checkout/checkin callbacks validate bindings and main render globals.
Count-zero full graph/CPU frame checks reproduce rejection before the fix and pass
afterward; 812 scoped checks pass. Diagnostics distinguish record/context/stream
failures and include delivered count. Host attribution remains an inference until
owner playback qualification. CEP animation-19 token and IDs/ABI stay unchanged.

## P-02J / build 19 — animation binding follow-up

Owner reports Origin XY keys produce black output and the panel rejects its
gateway. Paired animation-19 readiness fixes the confirmed token mismatch.
Bindings use documented effect/param methods, reject missing UUIDs instead of
zeroing values, and validate evaluated values/enabled state before publication.
Rollback restores underlying values as well as expressions. UI/render failures
identify the binding stream. The old callable-host-object assumption is a
hypothesis for the owner black output; AE playback acceptance remains open.
798 scoped adapter/camera/registration checks plus actual CPU frame regressions,
generated-expression tests and real JSX handshake tests pass. See ADR 0023.

## P-02J / build 18 — native keyframes

Owner confirms build 17 Effect Controls edits take effect. Public node controls
now permit keyframes/user expressions. Main manifest 22 appends 512 derived numeric
dependencies; UI commits bind native UUID/property/components, render samples its
own PF inputs at each requested time. Optional record 0x8002 holds typed mappings.
No render-thread AEGP reads, Core ABI or node layout change. ADR 0023 documents
capacity, fresh-development-effect qualification and current-frame kernel semantics.
AE stopwatches/interpolation, CEP-closed rendering and undo/reopen need owner tests.

## P-02I / build 17 — direct native publication

Owner build-16 error remains at delivery, before the commit handler acknowledges
the request. Remove the unreliable inter-effect generic call. Native node AEXs now
compile/publish the renderer snapshot directly on the UI edit path using their own
AEGP registration ID. Per-node authored values remain independent, and render still
consumes the saved arbitrary graph. Shared source/build inputs and scoped checks
must qualify the independent node links. Real AE dragging with CEP closed, undo/redo
and save/reopen remain the owner gate; compiler/fake-host success is not acceptance.

## P-02I / build 16 — native control synchronization

Build 15 owner acceptance failed: all tried native edits report error 516. Build 16
removes its assumptions about generic callback params and context, carrying bounded
borrowed caller context and reading main controls through AEGP. Separate scalar/graph
publication and verification phases identify failures, and integer receipt/type
contracts prevent inappropriate floating comparisons or union reads. Targeted fake
host checks exercise absent generic context, delayed node values and rollback.
Native dragging with CEP
closed, undo/redo and save/reopen remain owner AE 2023 qualification gates.

The owner's Appearance review is partially confirmed: it is an active optional
override, with duplicated Particle controls except Life. Its removal is a separate
cleanup scope; default topology omission alone does not prove unreachable code.

## P-02H / build 14 — 2026-10-02

Owner requests Force/main reference alignment, million-particle fresh cap and
removal of an editor SDK include warning. Force motion/curve and renderer Time
Remapping/Preview are integrated in independent native records. Missing main
renderers and controls are listed in reference-main-force-comparison.md, without
inactive UI placeholders. Native layout 6/main 21/Force 2/Output 3; Core ABI 2.
Minimum targeted checks and candidate build precede source push/deployment.
Owner AE qualification and exact reference trajectories remain open.

## Current P-02F deployment — 2026-10-02

Build 12 addresses shared Particle inputs, excessive numeric drag steps and
the sprite-coverage budget dialog during Over Life edits. It also separates
base values from percentage curves, fixes summary lookup shadowing, handles
multi-emitter counts and retains outgoing wires on Emitter copies. Implemented
control labels/splits/defaults use the local reference table. Life caps at
10000, defaults to 2, CEP step 0.1. Layout 4/main 20/Emitter schema 4 have no
development migration. Candidate uses May 2023 SDK, with no test suites run.
Source pushed as cadab2a, then build 12 installed with AE absent under standing
authorization. Six installed hashes/selected Core verified, build 11 backed up
and one Junction retained. Owner gates: fresh initialization, shared inputs, direct native and CEP edits,
undo/save/reopen, curve independence and large-sprite cancellation/rendering.
[Deployment status](native-node-checkpoint.md).

## Current acceptance checkpoint — 2026-10-02

P-02E/build 11 addresses unavailable canvas deletion and redundant collapsed
outer categories. Delete/Backspace and context-menu actions share editable
selection, preserving fixed Output. Outer topics are removed with compacted
main schema 19/native layout 3 indices; surviving disk IDs and Core unchanged.
Gateway native-node-sync-11 reads the new layout. No development migration.
SDK build and JS parsing pass; build 11 deployed with AE absent after source
push cb53d50. Six hashes verified; one Junction retained and build 10 backed up.
Owner verifies actual deletion,
native effect removal, flat controls, undo and reopening in AE 2023.
[Deployment status](native-node-checkpoint.md).

G-06/build 10 addresses owner feedback after build 9 was described as basically
working: quiet normal render cancellation, multiple independent emitter streams
under Output's single live-particle cap, clickable wire disconnect and compatible
port snapping within 22 screen pixels. May 2023 SDK compilation and CEP JavaScript
syntax checks are the candidate gates; no test suites added/rerun. Exact AE host
qualification of these fixes remains open. Build 10 is deployed under standing
AE-closed authorization with all six hashes verified after source push c368bbf.
The older creation checkpoints below
are historical. [Current checkpoint](native-node-checkpoint.md).

Build 9 is deployed: all node disk IDs are explicit numbers in the SDK's
1..9999 range. Build 8 still failed with the same duplicate-matchname error;
its topic-ID repair was insufficient. The ten-digit FourCC name-length model
predicts Particle/Appearance collisions, but host truncation remains inferred.
Registration/supervised lookup share numeric IDs; compile-time guards and SDK
build pass. Six hashes verified. No tests added/rerun. Fresh effects required;
actual Particle creation remains the owner gate. [Checkpoint](native-node-checkpoint.md).

Build 7 targeted Particle creation's actual spatial-interpolation
error by removing redundant interpolation flags from constant controls. 146
Particle/Appearance checks and SDK build pass; deployed with all six hashes
verified on 2026-10-02. Owner authorizes future AE-closed deployments (ADR 0011).
Owner feedback now reports the duplicate-matchname error; creation still fails.

Previous: the owner supplied AE's float-aware node / missing SmartFX verification
error. Build 6 implements node SmartFX passthrough with matching PiPL/runtime
flags; 276 actual-node checks and the SDK candidate build pass. CEP resize work
is deferred so it no longer hides operation errors. Build 6 is deployed after
explicit authorization, with six hashes verified. Particle/native-node acceptance
is still open for owner testing; Emitter duplication works.

Current owner evidence after CEP 5a: Emitter duplication works; Particle default
creation/addition still fails. CEP 5b preserves the failing operation's error and
captures the creation stage without an AEX replacement. Particle/native-node
acceptance is still open. The remaining failure detail must come from one owner
operation in AE; [checkpoint](native-node-checkpoint.md).

Build 3 renders but crashes on layer selection, as confirmed by the owner. Build 4
removes unused hidden structural groups and passes 721 adapter checks plus the
May 2023 SDK build. One-folder deployment/rollback checks pass; build 4 is deployed
after authorized AE closure. No additional AE runs are planned in this turn. The next host
gate is safe layer selection, then native node operations and render response.
Detailed evidence: [native-node-checkpoint.md](native-node-checkpoint.md).

Planning baseline: 2026-09-27.

Current owner scope supersedes the earlier multi-host plan: build and qualify AE
2023 only. The current Alpha must deliver emitter -> Particle -> force -> output,
a minimal dockable effect-control panel, four emitter shapes, gravity/drag, color and life curves,
project persistence/copy/undo behavior, and spark/snow/floating-light examples.
Full observed Stardust functionality and architecture/performance improvements
remain the long-term goal; this Alpha does not complete that goal.

## Current implementation and qualification status

- **H-01 reloadable core (repository gate passed; AE smoke gate passed):** the May 2023
  build now produces an AE adapter plus independently buildable Core DLL. A
  content-addressed development manifest selects the DLL; Options explicitly
  reloads it. The adapter pins the generation across SmartFX pre-render/render
  and mixes its content identity into the cache GUID. Core-only builds leave the
  `.aex` unchanged. AE 2023.5.0 Build 52 confirmed Full/Quarter hot reload
  without a restart, missing-DLL fallback, visual 8/16/32-bpc rendering,
  transparency, and save/close/reopen. The first `/MD` candidate crashed under
  AE's old app-local C++ runtime; both modules now use `/MT`. Three Full-resolution
  8-bpc render-queue frames match the prior monolith's decoded RGBA pixels.
  An in-flight AE render switch and broader pixel parity remain open.

- **M0 contract work is in place:** parameter manifest, sequence-format specification, build matrix, and ADRs for product identity, time, and pixels/alpha are checked in. The M1 shell uses the selected internal identity `org.starfieldfx.particle`.
- **M1 implementation is in place:** native entry-point source, official PiPL pipeline, lifecycle dispatch, and legacy pass-through render are present.
- **M1 SDK builds succeeded historically:** May 2023 and SDK 26.5 Windows x64 builds exported `EffectMain` and `PluginDataEntryFunction2`; the May 2023 SDK is the current build target, and newer-host qualification is deferred.
- **M1 load smoke check passed:** the user confirmed the shell loads in AE 2023 after correcting PiPL stage encoding. The precise AE build is not recorded. That pass-through shell was superseded by the M2 SmartFX render path; add/remove, save/reopen, duplicate, undo/redo and repeated loads were later qualified on the M2 and build-2 candidates (evidence in `docs/compatibility-matrix.md`).
- **M2 render-slice code is present and builds:** SmartFX transport, the parameter bridge, deterministic simulation, and the CPU sprite compositor are implemented; this deliberately minimal look is not feature parity. `docs/current-feature-audit.md` records the visible gaps. Current build verification uses the May 2023 SDK.
- **M2/G-04 host qualification is partial:** AE 2023.5.0 Build 52 loaded the corrected candidate. Full and Quarter readouts show normalized emitter origin `[1920,1080,1080]` and world `(0,0,0)`; a direct Quarter render reported grid `960x540`, and Quarter RAM preview visibly advanced. The Options diagnostic is process-global, so isolation across multiple effects/comps remains unqualified. The owner can open the project by dragging it into AE, so the missing-file warning is treated as a path/open-flow mismatch. Effect/control retention and the remaining render/lifecycle checks are recorded in `docs/compatibility-matrix.md`.
- **M3-01 core implementation is complete:** seeded Point/Box/Sphere/Disc birth distributions and per-particle velocity spread are implemented. AE 2023.5.0 Build 52 visually confirmed distinct Box/Sphere/Disc distributions and Quarter Point playback; exact distribution matching and reverse-time image determinism remain open.
- **M3-01B adds direct emitter dimensions:** pre-release revision 12 interprets Size X/Y/Z as full-resolution layer pixels (0–100000, default 100 px). Box uses all axes and Sphere forms an ellipsoid; Disc retains its dedicated Disc Size control, and Point ignores the dimensions. The former percentage behavior is intentionally dropped during development. Focused core and AE 2023 qualification status is in `docs/compatibility-matrix.md`.
- **M3-05 adds Particle size and opacity randomness:** revision 11 appends deterministic per-particle Size Random and Opacity Random controls. Their 0% defaults preserve existing output; factors remain stable for each particle across age-curve evaluation. Core/adapter/panel checks and the May 2023 SDK build are recorded in `docs/compatibility-matrix.md`; visual and project lifecycle checks remain open in AE 2023.
- **M3-06 owns Particle lifetime and curve authoring in source:** Particle schema 2 requires a branch lifetime and expires assigned slots independently. Emitter schema 3 has no lifetime or global-cap field; Output schema 2 owns Max Particles in the graph snapshot held by the main renderer. The CEP inspector shows base particle Size in pixels and fixed 0–100% Size/Opacity over-life curves that multiply their respective base values; curve clicks use SVG screen transforms, and Linear selects interpolation while preserving knots. The 2026-09-30 render of the paired candidate produced darker white particles over blue (background `[0,108,255,255]`, particle `[16,98,209,255]`). The AE candidate now requests straight-alpha output from the core while internal accumulation remains premultiplied. The repair remains unconfirmed until the revised Core is rendered over transparency and an opaque lower layer in AE 2023.
- **M3-02 core implementation is complete:** Emitter → Particle → Force → Appearance → Output evaluates with stage-order enforcement, closed-form gravity/drag integration, and age curves for size, opacity, and color. P-02C adds bounded editable piecewise-linear Size/Opacity percentage curves with a 100% endpoint fallback; the May 2023 SDK build passes, while AE visual/save/undo gates remain open. Appearance remains an optional downstream override under ADR 0015. The split build visibly responds to `Gravity Y = -2` and `Size Over Life = 1`; drag, opacity, and color curves still need host visual checks.
- **G-01–G-05 implementation:** model, validator, codec, Particle graph runtime, arbitrary-data callbacks, legacy-control selection/capture, supervised edit surface and render snapshot bridge are implemented. The current core suite passes 11,909 checks; the adapter fake-host suite passes 678 checks. Graph values are constant; capture samples controls at one time. AE Controls save/reopen, effect copy and undo/redo passed; integrated Node Graph persistence/undo remain host gates (ADR 0008).
- **P-02 / P-02A/P-02B/P-02D graph editor (revision-18 build 5 deployed; owner reported refresh failure; CEP 5a hotfix awaits retest):** separate hidden AE Emitter/Particle/Appearance/Force effects own values, UUID, links and positions. Main owns Output and its compiled render graph. First CEP synchronization creates Emitter → Particle → Output; ready marker 89 keeps deliberate deletion empty. Ordinary record reads plus revision/checksum/commit/receipt replace every expression dependency. Targeted gateway checks cover add/copy/edit/move/insert/connect/disconnect, native reorder/deletion, raw duplicate re-key and rollback; the actual numeric GraphCarrier passes 756 adapter checks and the May 2023 SDK build. Build 4 selection safety is owner-confirmed. Actual AE node operations, cache/render response, undo and reopen remain open, along with panel-closed bootstrap and multiple active emitters.
- **P-02C over-life curves (source integrated; AE gates open):** Particle and optional Appearance inspectors draw piecewise-linear Size and Opacity curves. Graph mode persists curve payloads and endpoint values in the graph transaction; AE Controls mode uses the project curve bank. Point insertion/drag/removal, Life/value entry and scrubbing are implemented in the panel source. Exercise both storage paths and rendered appearance with the matching AE 2023 build.
- **G-05/G-06 core source implemented; Output owns the global cap:** each Emitter divides local birth slots among its own UUID-ordered Particle children. Particle nodes own lifetime/age curves; multiple emitters merge newest live births under Output's single cap. Identity is `(emitter UUID, local slot)`. Selected births write directly into a pre-sized final buffer without per-emitter populations or a final sort. Force fan-in/path deduplication and one-pass appearance precedence remain unchanged. Emitter schema 3 and Output schema 2 are unchanged. Historical G-05 test counts do not qualify G-06; no suites rerun in this iteration. Stochastic allocation remains deferred. AE multi-emitter output, cancellation, editing, persistence and undo require owner confirmation. ADR 0015 contains the current contract.
- **Delivery examples (current version goal 5):** Spark, Snow and Floating Light are documented in `docs/examples.md` and shipped as panel presets; none has been rendered in the host yet.
- **Known renderer and host gaps:** output is still flat 2D discs; Z does not affect projection, depth, or occlusion. Texture/layer sources, motion blur, mesh/volume rendering remain open. The per-node AE effect and graph-sync path, including direct-control source synchronization, is implemented but not yet accepted in AE 2023; complete that host pass before full P-02B qualification. ROI narrowing is deferred to profiling.
- **Render geometry and point values are separate:** the render grid comes from observed checked-out worlds plus `max_result_rect`, `ref_width/ref_height`, and `par` (ADR 0005). The owner’s corrected-candidate AE 2023.5.0 Build 52 screenshots plus a direct Quarter session confirm point controls shrink with Quarter preview (`1920,1080,1080` → `480,270,270`), while normalized pixels remain `[1920,1080,1080]`, world offset stays zero, and the single-effect Quarter grid reports `960x540`. Options reads a process-global record of whichever instance rendered last; diagnostics across multiple effects/comps remain unqualified.
- **Default look updated (D-02, owner delegated):** velocity Y now defaults to 0.3 layer heights per second so a freshly applied instance shows a rising trail instead of one static dot. Defaults affect new instances only.
- **Current renderer control surface:** curve banks and the numeric curve commit stream occupy IDs 45–79; emitter dimensions use IDs 80–84; Size/Opacity Random use IDs 86–87; project marker ID 89 records native-node materialization. The failed expression request at ID 90 was removed from the current source. Zero curve count uses the linear 100%-to-endpoint curve. A Size edit has focused gateway coverage for retaining the project's Opacity curve. AE qualification of the integrated graph/curve edit paths remains open.
- **PiPL/runtime flag drift fixed (D-04):** the build now declares `PluginFlags.h`/`PluginVersion.h` as `AdditionalInputs` for the PiPL step and fails when the generated resource disagrees with them. The earlier AE "global outflags mismatch" came from exactly that drift.

## Support and toolchain policy

### Host versions

- Current host target: After Effects 2023, Windows x64. Record the exact tested 23.x build before claiming host support.
- Build with `AdobeSDK/May2023_AfterEffectsSDK`; both the PowerShell entry point and direct MSBuild default to it.
- Newer AE families, betas and other platforms are deferred. Historical 26.5 SDK builds are evidence for those older revisions only and are not current deliverables.
- Runtime code must only call APIs and suites available in AE 2023; obtain suites defensively and provide defined failures when unavailable.
- Do not copy SDK headers, samples, PiPL tools, or binaries into the repository. Keep the SDK as a local build input from Adobe Developer Console and record SDK version/toolchain metadata in build artifacts.

An MFR-capable host does not itself prove this plug-in is thread-safe; the threaded-rendering flag remains disabled.

### Build and dependency policy

- Core language: C++20, exceptions caught at the C ABI boundary, RAII internally. Do not let C++ exceptions cross an Adobe entry point.
- Windows is the first shipping platform because the current project/workspace is Windows. Release target starts at x64. Keep source portable; add macOS universal and Windows ARM64 packages as separately qualified targets.
- For native Windows releases, pin the MSVC toolset and Windows SDK in `docs/build-matrix.md`; CMake and Ninja are optional for the host-independent core and are not part of the `.aex` build.
- Use the AE SDK's own PiPL/resource tooling and templates. Keep PiPL declarations and runtime flags generated from one manifest or checked for exact equality.
- First-party core has no third-party runtime dependencies. Add a library only for a concrete feature, use a maintained release, pin its version/commit and license metadata, hide its symbols, and test interaction with AE's process-wide dependencies.
- No private OpenGL context and no private thread pool in the first renderer. CPU is the deterministic reference; later GPU work must use documented AE GPU selectors/device APIs and have an explicit CPU fallback.

### UI and panel policy (revised 2026-09-27 after owner direction)

The owner's product statement: **node-based editing is the essence of the reference product**. A flat parameter list with emitters and forces bolted on is not the product, and implementing the M3 feature families as flat parameters first would mean building them twice once the graph exists. That changes the sequencing, not the contracts.

- **The graph is the parameter layer.** Nodes own ports, edges, and per-node parameters; the AE effect keeps a small number of top-level controls (for example source mode/quality) and stores the graph in an AE arbitrary-data parameter, not sequence data. ADR 0005's `RenderRequest`, time, pixel, and error contracts stay as they are: the graph feeds the same host-independent boundary.
- **Graph foundation comes before feature families.** Model, validation, bounded serialization, and evaluation order are host-independent and testable without AE, so they land first (`Wave G` in `docs/agent-backlog.md`). After that, emitters/forces/modifiers are implemented as node types rather than as new flat settings.
- **The node editor must be a dockable panel; in-effect UI cannot host it.** Confirmed in the SDK: `PF_EffectCustomUISuite2` only hands out a Drawbot drawing reference (`PF_GetDrawingReference`) plus an overlay theme suite for stroking/filling paths and vertices. There is no widget toolkit, no text layout, no scrolling surface, so a graph editor there would mean hand-rolling text rendering and hit-testing. In-effect Drawbot UI stays reserved for *on-screen gizmos* (dragging the emitter, drawing velocity/force overlays in the comp window), which is exactly what the suite is designed for.
- **Panel bridge.** The AE 2023 panel uses CEP `CSInterface.evalScript` and the supported ExtendScript DOM. Supervised ordinary parameter streams are the edit surface; the effect updates its canonical arbitrary-data graph in `PF_Cmd_USER_CHANGED_PARAM` (ADR 0009). The panel is isolated from the renderer and can be replaced later without changing graph or render contracts. Qualify hidden stream access, scripted parameter supervision, and undo in AE 2023 before relying on the bridge.
- **Graph editing gestures.** The canvas uses upper input and lower output ports, with free node placement; blank-space drag selects nodes, selected nodes move together, the wheel zooms around the cursor, the middle button pans, Alt-drag previews a duplicate, Ctrl+D duplicates the selection, and right-click opens graph actions. Clicking a connection disconnects it; dropping a node over a connection inserts it by replacing one edge with two typed edges. These gestures submit revision-checked edits by writing node-owned AE effect streams and then changing the renderer's numeric commit trigger. The retired expression request at index 90 is not registered by the current source; index 42 guards batched Output writes. The gateway verifies the registered stream index and `propertyIndex` before writing. Current source still needs AE 2023 confirmation for gateway startup, callback acknowledgement, add/copy/delete, one-step undo, persistence, stale rejection, and render parity. Do not repeat the `CUSTOM_VALUE` probes.
- **No silent scope change.** Building the panel now contradicts the earlier "do not start a CEP panel" line; that line is superseded by this section and the tradeoff (a possible future UXP port) is accepted deliberately.

## M0 architecture decisions now locked

1. Product identity: `Starfield Particle`, category `Starfield FX`, match name `org.starfieldfx.particle`, and package ID `org.starfieldfx.aftereffects`; never reuse the old plug-in identity.
2. Parameter contract: the current renderer source registers indices 1–89, including AE topic markers, hidden bootstrap metadata, and the project-owned native-node readiness marker; index 0 is AE's implicit input. The rejected expression request at index 90 has been removed. Graph `NodeId`, `EdgeId`, and `ParamKey` remain separate identity domains.
3. Sequence storage: schema 1 defines a bounded binary representation with magic, lengths, counts, CRC, and migration rules in `schema/sequence-format.md`.
4. Time model: comp time and frame duration remain signed integer rationals; negative time, subframes, shutter samples, seed derivation, and particle ordering must stay deterministic.
5. Render semantics: canonical coordinates, pixel aspect/downsample, ROI, 8/16/32-bpc conversion, color space, and premultiplied-alpha handling are recorded in ADR 0003. The owner directs the effect to output particles over transparent black without copying its input layer (ADR 0005); AE 2023.5.0 Build 52 confirmed the rebuilt adapter's transparent output.
6. Failure model: map core errors to stable AE errors/messages. Every checkout, handle, suite acquisition, lock, and staging buffer has one clearly owned cleanup path.

## Milestones

| Milestone | Scope | Exit criteria |
|---|---|---|
| M0 — contracts | Lock IDs, support policy, parameter schema, time/render semantics, and supported platforms | Architecture decisions reviewed; data-format and parameter manifests checked into source control |
| M1 — loadable shell | One native effect, PiPL, one effect entry point plus SDK registration entry, About/global/sequence lifecycle, pass-through render, implicit input only | Loads in the recorded AE 2023 build; add/remove/save/reopen/copy/undo works; no MFR flag; graph storage and controls follow in M2/M4 |
| M2 — render vertical slice | SmartFX pre-render/render, bounded ROI, CPU-only point emitter/sprite, time/seed determinism, 8/16/32-bpc and correct rowbytes/alpha | Same request gives bit-identical output; out-of-order and repeated requests match; cancellation and allocation errors release all host resources |
| M3 — particle MVP | Point/box/sphere/disc emitters, birth/lifetime, velocity, gravity, drag, size/opacity curves, seed controls | Golden cases cover frame rate changes, non-integer frame rates, negative/subframe time, shutter sampling, and project reopen |
| M4 — graph and presets | Node/edge model, graph validation, schema migrations, preset import/export, native UI organization | Invalid graphs cannot hang/crash; stable identifiers survive node reordering and schema migration |
| M5 — feature families | Add modifiers/forces, auxiliary particles, layers/overrides, models/materials/lights, post effects, then volumetrics in priority order | Each family has a behavior spec, regression fixture, performance budget, and independent feature flag |
| M6 — MFR qualification | Audit shared state; use Compute Cache for shareable derived data; parallel render stress and host matrix | Explicit concurrency audit signed off; serial/MFR pixels match; no shared mutation, deadlocks, or cache aliasing; only then set threaded-rendering flag |
| M7 — acceleration and future panel port | Profile first; add documented AE GPU backend if worthwhile; consider a UXP view port only when it is in the supported-host scope | CPU fallback always works; GPU/CPU output differences bounded and documented; the AE 2023 CEP panel may be absent/restarted without affecting render correctness |
| M8 — packaging and release | Installer layout, versioned presets, diagnostics, crash-safe logging, release notes | Clean install/uninstall, upgrade-in-place, supported-host matrix and reproducible release build recorded |

## M1 selector/lifecycle work order

Implement and review the smallest lifecycle surface first:

1. `ABOUT` and `GLOBAL_SETUP`: set only validated metadata/flags; acquire required suites; no heavyweight GPU or network initialization.
2. `PARAMS_SETUP`: register parameters from a stable manifest. UI labels can change; IDs cannot.
3. `SEQUENCE_SETUP`, `SEQUENCE_RESETUP`, `SEQUENCE_FLATTEN`, `SEQUENCE_SETDOWN`: M1 owns no custom sequence state; keep these paths explicit and side-effect free. Add bounded format parsing with graph state in M4.
4. `RENDER`: pass through the source using AE's documented copy callback. Replace this transitional path with SmartFX in M2 after the SDK sample build and resource ownership pattern are confirmed.
5. `GLOBAL_SETDOWN`, error boundary: release all global resources, catch all C++ exceptions at the entry point, and map unknown exceptions to a controlled AE error.

M1 used legacy `PF_Cmd_RENDER` only as a low-risk pass-through load test. M2 has replaced it with SmartFX: the effect now declares `PF_OutFlag2_SUPPORTS_SMART_RENDER` and `PF_OutFlag2_FLOAT_COLOR_AWARE` and implements `PF_Cmd_SMART_PRE_RENDER`/`PF_Cmd_SMART_RENDER`. The legacy render entry point remains only as a documented non-smart-host fallback. Do not add `PF_OutFlag2_SUPPORTS_THREADED_RENDERING` until M6 passes.

## Compatibility and quality gates

- Each current release qualifies the recorded AE 2023 build. Record OS, exact AE build, SDK header/resource version, compiler, architecture, and result. Newer-host work is deferred by owner direction.
- Project lifecycle: fresh add, duplicate effect, undo/redo, copy/paste, save/reopen, render-only instance, missing/older/newer sequence schema.
- Render correctness: full frame and ROI, input-independent and input-dependent cases, 8/16/32-bpc, odd rowbytes, premultiplied/straight alpha, pixel aspect, downsample, color-space change, negative/subframe time, cancellation.
- Determinism: random-seed fixed and changing, repeated frames, reverse frame order, multiple comp rates, motion blur samples.
- Robustness: corrupted serialized data, extreme parameter values, allocation failure, missing optional suite/device, device reset, effect removal during preview, host shutdown.
- Concurrency: TSAN or equivalent core-level race checks where supported; AE MFR stress only after the serial renderer is stable. Compare output and cache hits across serial/MFR modes.
- Performance budgets are measured before choosing GPU work. Avoid claiming speedups from synthetic core benchmarks alone.

## Immediate next work

1. **Finish the M3-06 host pass on its paired candidate.** Check Particle lifetime in the Particle inspector and independent branch expiry, add/drag curve points at multiple panel sizes, confirm interpolation selection retains knots, and confirm base Size remains in pixels while both over-life curves remain 0–100% multipliers. Verify the revised straight-alpha output over both transparency grid and a blue lower layer at 8/16/32 bpc; if the blue composite remains dark, inspect the decoded candidate output before changing the alpha contract again.
2. **Continue the remaining AE 2023 host gates.** Full/Half/Third/Quarter centre normalization, Quarter Point playback, save/reopen, effect copy, undo/redo, all bit depths, transparency, lower-layer compositing, shape distinctions, gravity and size changes have earlier host evidence. The render queue produced three frames with exact sampled monolith parity. Still test fresh add/build-1 project load, off-centre positioning, color/opacity curves, control capture, graph-byte persistence, cancellation, reverse-time frame identity, and an in-flight Core switch.
3. **Continue CEP panel qualification** using the checklist in `cep_panel/README.md`. Automatic target discovery, a panel `Size` write and host undo updating the picture passed. Verify redo, focus refresh after other host edits, stale-state rejection and Node Graph synchronization.
4. **Record the three delivery examples** from `docs/examples.md`.
5. **Grow the graph past the fixed chain:** protocol v2 for create/delete/rewire, then the next node kernels (textures/layer sources, depth, spawn) each tied to an observed reference case.
6. **Deferred deliberately:** analytic ROI narrowing, Compute Cache, MFR, and GPU stay on their milestone cards.

## Sequencing note (owner direction, 2026-09-27)

The milestone table above was written before the owner restated that node-based editing is the product's core value. M3 and M4 swap priority: the graph model, serialization, and evaluation (**M4-01/02/03**, extended into **Wave G**) come before the M3 feature families, which are then implemented as node types. M5 feature families, M6 MFR, M7 GPU, and M8 packaging are unchanged. Nothing in the M0 contracts (IDs, time, pixels, version, match name) is invalidated by this re-ordering.

## Sources checked on 2026-09-27

- [Adobe AE developer portal](https://developer.adobe.com/after-effects/) — SDK download entry through Adobe Developer Console.
- [Adobe AE SDK What's New](https://ae-plugins.docsforadobe.dev/intro/whats-new/) — current 26.5 guide, 25.6 Windows ARM support, and SDK history.
- [Compatibility across versions](https://ae-plugins.docsforadobe.dev/intro/compatibility-across-multiple-versions/) — latest headers plus per-host testing guidance.
- [MFR](https://ae-plugins.docsforadobe.dev/effect-details/multi-frame-rendering-in-ae/) — shared-state constraints and Compute Cache.
- [PiPL resources](https://ae-plugins.docsforadobe.dev/intro/pipl-resources/) — match name permanence and PiPL/runtime flag consistency.
- [Adobe CEP-to-UXP transition announcement](https://blog.developer.adobe.com/en/publish/2026/09/investing-in-the-future-of-creative-cloud-extensibility-uxp-comes-to-our-flagship-applications) — AE UXP public beta target and CEP transition timeline.
