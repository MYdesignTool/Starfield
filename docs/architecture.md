# Starfield plug-in architecture

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

The planned GPU integration follows AE's framework/device negotiation and native
SMART_RENDER_GPU output worlds. Windows AE 2023 targets CUDA/OpenCL, using AE's
contexts, queues and device-memory suite; the May 2023 SDK does not expose DirectX.
The private D3D12/readback proposal was withdrawn before implementation. Core
prepares typed immutable scene data; host GPU resources and dispatch remain in
ae_plugin. Shared GPU output removes our full-frame readback, while CPU scene
uploads and historical sampling still have measurable costs. See
[ADR 0026](adr/0026-reference-particle-and-native-gpu.md) for selectors, the planned
scene ABI revision, resource lifetime and acceptance gates. Build 22 remains CPU-only.

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

## P-02I / build 17

Build 16 owner acceptance failed at delivery: its generic request was not acknowledged.
Native node USER_CHANGED_PARAM now directly calls a shared compiler/publisher compiled
inside every node AEX. It reads sibling node records with the node module's AEGP ID,
substitutes its accepted callback value by UUID/index, and saves/verifies/restores
the main renderer graph and integer receipts through AEGP streams. No cross-effect
generic call, transport payload or main edit selector remains. NativeGraphCommit.*
owns shared publication/snapshot helpers; NativeNodeGraph accepts an explicit caller
ID. Main CEP commits retain the same helpers, and render uses immutable saved graphs.
Independent node controls, main 21/layout 6/node schemas/Core ABI 2 remain unchanged.
Actual AE native edits, undo/redo and reopen still require owner qualification.

## P-02I / build 16

Native node USER_CHANGED_PARAM sends an acknowledged COMPLETELY_GENERAL request
to the renderer, carrying its accepted value and UUID. The native compiler applies
that value only to the matching node/index; other controls remain AE-owned streams.
The renderer publishes graph/revision/checksum/source with AEGP_SetStreamValue,
checks exact saved bytes and restores old values on failure. No synthetic callback
array is treated as persistence. Generic callback context is not assumed populated:
private request v2 borrows the node's suite/handle callbacks and layer/renderer refs,
with numeric source geometry/time. Only eight main control streams are read for
compilation. Publication/readback distinguishes scalar receipts from the arbitrary
graph. Receipt integers are float-exact and union reads require matching stream
types. Borrowed host objects never enter Core or persistent state. Build 15 owner
qualification failed with error 516 across controls; build 16 AE acceptance is open.
Rendering still consumes immutable saved graphs;
main 21/native layout 6/Core ABI 2 and node schemas are unchanged. See ADR 0022.

## P-02H / build 14

Fresh Output cap is 1000000 (maximum 2000000); low-population evaluation reserves
actual survivors, and Auxiliary does not reserve the full cap at startup. Force
schema 2/native layout 6 exposes scalar Gravity/random, Wind X/Y/Z,
Spin/frequency/resist/delay and Air Density, with a saved percentage curve.
Piecewise linear wind forcing integrates analytically under total branch drag;
random attenuation is salted by Force and Emitter UUIDs. Spin and its velocity
are explicit deterministic displacement equations. Reference numeric parity is
not inferred from names. Native effects remain separate and node controls flat.

Main manifest 21 appends streams 90..97 for Time Remapping and Preview. Output
schema 3 saves these globals; pre-render checks out their live animated values
and makes an immutable graph snapshot. The remapped clock is applied once before
recursive Auxiliary sampling. Preview filters stable identities after simulation,
preserving parents and full live counts. Core ABI 2/Render.hpp stay unchanged.
Fresh development effects required; see ADR 0021 and the main/Force comparison.

## P-02G / build 13

Native node edit callbacks dispatch by runtime parameter index. PF_ParamDef's
setup-only uu.id union cannot identify edits: during USER_CHANGED_PARAM it holds
change flags. UUID values and the batch guard still validate the owning node.
The renderer continues to consume immutable graph snapshots; no render-time
sibling reads are introduced. CEP node clicks select the UUID-owned AE effect
(Output selects the renderer), with coalesced transient requests and no undo group.
Unchanged node manifests skip redundant stream writes.

Core ABI 2 carries a numeric camera snapshot. The adapter captures SDK matrices
and the Core projects camera-facing sprites, clips behind-camera particles and
sorts visible sprites by depth. Main PiPL/runtime flags include camera dependency.
Layer inverse mapping avoids applying AE's later transform twice. See ADRs 0003,
0005, 0012 and 0020 for coordinates, absent-camera behavior and open host gates.

Emitter schema 5/native layout 5 adds Auxiliary mode and optional parent input 2.
Auxiliary reuses StarfieldEmitter.aex and the common motion kernel. Each child
samples parent position/velocity/appearance at birth and survives independently
after parent death. Chance, parent-life window and inheritance are percentages.
Output retains one cap; recursion/work are bounded and cancellable. CEP offers
Auxiliary creation, preserves stable native records, and omits an inaccurate
simple live-count estimate for Auxiliary. No old-project migration is provided.
Fresh development effects are required. SDK/scoped tests do not qualify AE behavior.

## P-02F / build 12

Particle input accepts distinct direct Emitters. The registry has no
emitter-specific merge prohibition; the edit planner preserves existing wires.
Each (Emitter, Particle) pair has one birth sequence, with shared Particle
properties and forces planned once. Emitter UUID/birth identity, deterministic
modulo child assignment and Output's global cap remain intact. Native records
still store up to four outgoing edges per node; all current inputs accept fan-in.
The CEP population counter sums actual per-emitter branch survivors before
applying the global cap. Emitter copies preserve mapped outgoing connections.

Native layout 4/identity 3 and main manifest 20 align implemented controls with
observed names: Life (Seconds), Size (Pixels), Origin XY/Z, Speed, Speed Random,
Angle X/Y/Z and percentage Opacity. Emitter schema 4 adds key 22 for independent
random percent; C ABI and graph envelope stay fixed. Life caps at 10000,
defaults to 2 and uses explicit 0.1 CEP steps. Base size/opacity never rewrite
percentage curve knots. Duplicate summary/record lookup functions are separated.

The renderer skips fully transparent sprites and has no default coverage-count
cutoff; explicit RenderLimits remain supported. Scan-row cancellation, graph,
population, ROI and storage bounds remain. Large visible sprites still cost
their full rasterization work; output is neither truncated nor approximated.
No development migration; use fresh effects. Source/SDK gates are distinct
from owner AE acceptance. See ADRs 0005, 0015, 0016 and 0019.

Current target: After Effects 2023 on Windows x64, built with the supplied May 2023 SDK. Owner direction on 2026-09-27 defers newer-host adaptation and qualification. See [the detailed roadmap](roadmap.md) for the support matrix and milestone gates.

## Goal and boundary

P-02E/build 11 uses flat native parameter layout 3 and main manifest 19: controls
appear directly beneath their effect headers, with no redundant outer topics.
Surviving disk IDs stay separate from compacted stream indices; arbitrary
callback disk ID 31 remains fixed. Gateway native-node-sync-11 and the node
direct-edit compiler share the new main commit index. Delete/Backspace/context
menu actions exclude fixed Output and use the existing native record transaction.
Core/graph schemas are unchanged; no development migration is provided.

G-06/build 10 evaluates multiple Emitters through their own UUID-ordered Particle
children and merges actual live births under Output's single cap. Internal
particle identity pairs Emitter UUID with local birth slot. Selected particles
write directly into a bounded final buffer; no per-emitter population allocation
or final sort. Host cancellation returns an interrupt without a dialog message.
CEP wire hits pass through the empty node container and connection previews snap
to compatible ports within 22 screen pixels. Independent saved AE node effects,
render snapshots, graph schemas and C ABI remain unchanged. SDK compilation
passes; owner qualification of these fixes is pending.

Native node parameter identity revision 2 (build 9) uses explicit disk IDs in
1..9999, defined in `schema/node-parameters.json` and shared `NodeRecord.hpp`.
Registration and supervised edit lookup use the same identities; compile-time
checks cover uniqueness, range and stream match-name length. Build 11 compacts
stream indices while retaining these numeric identities and graph schemas.
Unreleased FourCC IDs are not migrated; create fresh development effects.

This repository is the clean implementation of a node-based particle effect for After Effects. Existing reverse-engineering notes and binaries live in the parent directory and are reference material only. They are not linked into the build and must not be copied into the new implementation. Use observed user-facing behavior to define independent requirements; implement the algorithms and data structures anew.

The native effect is the product boundary. The AE SDK adapter translates selectors, parameters, pixel worlds, and host suites into a small host-independent C++ API. The core does not include AE headers, retain host pointers, access the filesystem implicitly, or call UI APIs.

## Layers

```text
After Effects
  ├─ StarfieldParticle.aex       main render effect, graph persistence,
  │                              SmartFX snapshots, host pixels and DLL loader
  ├─ StarfieldEmitter.aex        internal, menu-hidden Emitter node controls
  ├─ StarfieldParticleNode.aex   internal, menu-hidden Particle node controls
  ├─ StarfieldAppearance.aex    internal, menu-hidden Appearance node controls
  ├─ StarfieldForce.aex          internal, menu-hidden Force node controls
  └─ StarfieldCore.dll           graph evaluation, deterministic simulation,
                                 CPU renderer and later backends
```

The node AEX modules hold independent AE parameter streams; they do not render
particles. The existing `StarfieldParticle.aex` remains the only renderer and owns
the compiled graph snapshot. The independent node effects own saved authoring
records. The graph's fixed visible Output terminal is the
panel representation of that main effect; no separate Output AEX is built.
Node AEX modules set `PF_OutFlag_I_AM_OBSOLETE` so they are not offered as
standalone effects in AE's Effects menu. The CEP bridge still adds them by stable
match name to persist independent node values; AE 2023 must confirm that this
scripted creation path works while the modules are hidden from the menu.

Build 6 corrects the internal node 32-bpc contract reported by the owner: node
modules implement SmartFX pre-render/render pass-through and advertise SmartFX
together with float awareness (`flags2=0x00001400`). They forward the input ROI,
copy through the host world suite, and check in successful input checkouts on
every failure path. Parameter schema remains revision 18. Four actual-node
fake-host runs pass 276 checks; AE Particle creation/32-bpc qualification is open.

The main renderer has only one structural parameter group, Output. Hidden
bootstrap values use ordinary invisible controls; no invisible group boundary
participates in the ECW hierarchy (development schema 17). Installation exposes
the complete `dist` bundle through one `Plug-ins/Starfield` junction, with the
hot Core runtime in a real `dist/StarfieldRuntime` subdirectory.

Keep UI and preset compatibility as separate adapter modules. The AE 2023 dockable CEP panel uses the versioned ExtendScript bridge in ADR 0009. A graph transaction creates/removes the corresponding node AEX instances, writes their values, and commits the canonical graph snapshot to the main effect in one undo group. The Output terminal and its global controls remain on the main effect. Direct node-control edits have a supervised graph-sync source path, but callback delivery, cache/render response, undo and save/reopen remain AE 2023 qualification gates. The panel never shares C++ object layouts with the effect, and render code never queries sibling effects or panel state.

ADR 0012 defines the runtime C ABI and versioned development DLLs. The AE
adapter still owns graph serialization and control-to-graph construction because
those operations participate in AE parameter persistence; evaluation and
rasterization run in the selected DLL generation. Each pre-render pins that
generation until its matching render and result release finish. Its content
identity is mixed into the SmartFX cache key.

## Render contract

1. SmartFX pre-render checks out input metadata for layer bounds/reference geometry and time-varying values, determines the output/ROI, and constructs an immutable `RenderRequest`. Particle rendering does not consume input pixels; the core composites into a premultiplied staging buffer and encodes the requested output alpha mode. The current AE adapter candidate requests straight-alpha output pending focused AE 2023 confirmation (ADR 0005).
2. The core validates all external values, evaluates the graph deterministically for the requested rational time, and returns an owned staging buffer or a typed error.
3. The adapter copies pixels to the host output while checkouts are valid, then releases every checkout on every exit path.

Particle state must be reproducible from graph parameters, seed, and absolute time. Do not advance a process-global simulation by “one frame”; AE can request frames out of order, repeatedly, or concurrently. Random streams should be derived from stable particle IDs and the effect seed. Frame time remains rational through the adapter and is converted once at the simulation boundary.

Each Particle node owns its lifetime and retires only the particle slots assigned
to its branch. Emitter schema 3 has no lifetime or Max Particles parameter; Particle
schema 2 requires its own lifetime. Output schema 2 stores one global Max Particles
cap in the main effect's graph snapshot and keeps stable particle-ID ordering across
branches. Particle Size is a base diameter
in full-resolution layer pixels (bounded at 100000 px). Size Over Life and Opacity
Over Life each use a fixed 0–100% curve that multiplies the corresponding base
value. Box and Sphere emitter dimensions are direct full-resolution layer pixels converted through frame
height and pixel aspect; the Disc uses its dedicated layer-height diameter control.

## State and concurrency

- Global setup owns immutable plug-in metadata and acquired suite references. Global teardown releases them.
- The graph is a versioned serialized arbitrary parameter owned by AE; never mutate it while rendering. Sequence data remains unused in the current graph design.
- Render requests and graph snapshots are immutable. No mutable global/static render state.
- Advertise threaded rendering only after every selector and every dependency is audited for concurrent calls. No host suite or checkout calls while holding a lock.
- Shared expensive derived data belongs in AE Compute Cache with a complete content key, not in mutable sequence data. Cache keys include schema version, graph revision, time, quality, dimensions, format, and every checked-out input dependency.
- CPU is the correctness reference. GPU is optional and must produce a defined fallback when the required device/backend is unavailable.

## Compatibility rules

- Parameter IDs and effect match name are persistent project-file identifiers. Never renumber/reuse a released parameter ID.
- Node IDs, graph edge IDs, AE parameter IDs, and UI widget IDs are separate identity domains. The graph schema owns node/edge identity and migrations; AE parameter IDs map UI controls into that graph but never become graph IDs.
- Store a schema version in flattened sequence data. Deserialization is bounded, validates lengths, and migrates known older versions; unknown versions fail safely with a readable message.
- Keep display labels, UI layout, and preset import/export independent from stable parameter IDs.
- Describe output time dependence accurately to AE so its frame cache cannot return stale particle frames.
- Convert host pixel formats at the boundary. Respect rowbytes, channel order, alpha mode, pixel aspect, downsample, ROI, and 8/16/32-bit worlds.
- Export only the SDK-required C entry points (`PluginDataEntryFunction2` and `EffectMain`). Hide other symbols to avoid collisions with AE and other plug-ins.

## Source layout

- `include/starfield/core/Settings.hpp`: stable internal IDs, manifest bounds, and validated settings types.
- `include/starfield/core/AgeCurve.hpp`: bounded normalized-age curves and their nested versioned graph payload.
- `include/starfield/core/Graph.hpp`: host-independent node, port, edge, parameter, registry, and graph-validation contracts.
- `include/starfield/core/SequenceCodec.hpp`: bounded schema-1 graph serialization and parsing with CRC-32.
- `include/starfield/core/Error.hpp`: typed error codes and the `Result` primitive shared by the core.
- `include/starfield/core/Time.hpp`: normalized signed rational time with checked arithmetic.
- `include/starfield/core/Render.hpp`: host-independent frame/request/output/backend contract and the cancellation interface.
- `include/starfield/core/ParticleSimulation.hpp`: deterministic point-emitter evaluation for one absolute time.
- `include/starfield/core/CpuRenderer.hpp`: the CPU reference backend and its bounded-work limits.
- `src/core/`: implementations of the above plus bounded, finite-value settings validation.
- `include/starfield/core/PluginApi.h` and `src/core/PluginApi.cpp`: fixed-width
  render/inspect C ABI with opaque result ownership and generation-local release.
- `src/core/GraphConstruction.cpp`: graph creation shared by the adapter and DLL;
  `GraphEvaluation.cpp` stays in the DLL for frequent algorithm changes.
- `ae_plugin/CoreLoader.*`: versioned DLL selection, ABI validation and leases.
- `tests/core_tests.cpp`: host-independent self-tests for the M2 contracts.
- `ae_plugin/`: native SDK adapter, PiPL resource, Windows MSBuild project, and reproducible PiPL build scripts (see `ae_plugin/README.md`).
- `docs/parameter-mapping.md`: schema row → AE control → core field table and the emission rules.
- `docs/compatibility-matrix.md`: behavior inventory and independently derived acceptance criteria.
- `docs/adr/`: decisions that affect saved projects or rendering semantics.

The CMake build compiles only the portable core. The AE module is built by the Windows MSBuild project against the local May 2023 SDK by default. M2 implements SmartFX transport and 8/16/32-bpc CPU rendering; the owner reports all bit depths and time consistency pass in AE 2023. On 2026-09-28, AE 2023.5.0 Build 52 visually confirmed the updated adapter renders particles over transparency. G-03/G-05 source includes explicit Particle branches, deterministic emitter-slot partitioning, and branch-local force accumulation (ADRs 0007/0015); all portable core translation units compile and link, while regression and host checks remain open. G-04 persists a graph snapshot in an AE arbitrary-data parameter (ADR 0008). MFR, GPU, and Compute Cache remain disabled.

## SDK guidance used

- [Entry point](https://docs.yuelili.com/en/ae-plugin/effect-basics/entry-point/) and [command selectors](https://docs.yuelili.com/en/ae-plugin/effect-basics/command-selectors/) define the AE boundary and lifecycle dispatch.
- [SmartFX](https://docs.yuelili.com/en/ae-plugin/smartfx/smartfx/) supplies pre-render/render separation and explicit input dependencies.
- [Multi-Frame Rendering](https://docs.yuelili.com/en/ae-plugin/effect-details/multi-frame-rendering-in-ae/) documents thread-safety requirements and Compute Cache guidance.
- [Parameters](https://docs.yuelili.com/en/ae-plugin/effect-basics/parameters/) and [PF_ParamDef](https://docs.yuelili.com/en/ae-plugin/effect-basics/pf_paramdef/) establish stable parameter IDs and selector-scoped values.
- [PiPL resources](https://docs.yuelili.com/en/ae-plugin/intro/pipl-resources/) and [symbol exporting](https://docs.yuelili.com/en/ae-plugin/intro/symbol-export/) guide registration and minimal exports.

## Delivery sequence

1. **Complete in code:** M0/M1 contracts and the AE 2023 Windows x64 shell.
2. **Complete in code; host gate open:** M2 SmartFX transport, ROI, pixel-format adapters, and deterministic CPU rendering. See M2-06 in `compatibility-matrix.md`.
3. **Implemented in core and controls:** M3-01 seeded emitters, M3-01B direct-pixel Box/Sphere dimensions, and M3-02 gravity, drag, and age curves. P-02C adds bounded Size/Opacity polylines. M3-06 makes Particle own lifetime and makes curve interpolation selection non-destructive; a render reproduced dark translucent particles over blue, so AE currently requests straight-alpha output pending focused host confirmation. AE curve, lifetime, and compositing checks remain open. G-05 provides Particle branches and parallel force merges, with focused core and adapter regression coverage; AE qualification remains open.
4. **Graph authoring architecture:** P-02D uses independent hidden Emitter, Particle, Appearance and Force effects as the authoring source. Each owns its ordinary values, UUID, connections and position. Output stays on the main renderer. CEP reads these records directly and writes them under guarded transactions; numeric commit 43 builds the render snapshot, with receipt 44, revision 41 and checksum halves 90/91. All expression snapshots/mailboxes are removed. Revision-18 build 5 is deployed, 756 adapter checks and targeted CEP checks pass. The owner confirms build 4 fixed selection crashes; actual node operations, render response, undo and reopen await owner testing. Rendering never queries sibling effects.
5. **Remaining Alpha work:** qualify P-02D's native node-effect flow and graph lifecycle in AE before claiming P-02B/P-02C host completion; qualify G-05 behavior, define animation/history, and prioritize depth/projection, source types, and presets from observed behavior.
6. Keep MFR, Compute Cache, and GPU work behind separate concurrency/performance evidence and host-specific decisions.

## Current scope

H-01 separates the AE adapter from the runtime core and adds manual development
reload. The May 2023 SDK builds both binaries; 6,196 core checks, 395 adapter
checks and the loader harness pass. The `/MT` split pair is installed in AE
2023.5.0 Build 52. Full/Quarter hot reload, missing-DLL fallback, 8/16/32-bpc
visual rendering and save/close/reopen have host evidence. Three Full-resolution
8-bpc render-queue frames have exact decoded RGBA parity with the prior
monolithic binary; an in-flight AE render switch and broader pixel parity
remain open. The
prior monolith is backed up for rollback. Half/Third point mapping, split-build
copy/undo, shapes and basic gravity/size changes also have AE observations. See
`compatibility-matrix.md`.

M0/M1, M2 rendering, M3 core behavior, G-01–G-06 and CEP graph authoring source are present. Independent node effects own authoring data; the main effect owns Output and a compiled render graph. The gateway reads ordinary records without expressions or CUSTOM_VALUE scripting. Ready marker at index 87 prevents deleted nodes from being recreated; refresh prunes dangling links and re-keys raw duplicates. Build 10 addresses normal cancellation, multiple emitters and wire snapping; build 11 adds canvas deletion and flat Effect Controls. Exact node-operation acceptance, render response, multi-emitter behavior, undo and save/reopen remain owner gates. Automatic bootstrap with CEP closed, animation/history, MFR and GPU remain open.
