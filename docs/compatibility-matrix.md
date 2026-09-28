# Behavior inventory

Use this file to turn observed behavior into requirements before implementing each feature. Do not infer undocumented internal algorithms from binary details. Static deductions stay hypotheses until a host pass or a reference-effect observation confirms them.

## Host qualification checkpoint

| Check | Host | Status | Evidence / next step |
|---|---|---|---|
| M1 plug-in discovery and load | AE 2023, exact build not recorded | User-confirmed pass | Effect loads; render pass-through, add/remove, save/reopen, duplicate, and undo/redo still unrecorded |
| M1 discovery and load | AE 2023, exact build not recorded | User-confirmed pass for the empty M1 shell | Current build must be qualified separately |
| Corrected coordinate candidate | AE 2023.5.0 Build 52 (owner readout) | **Loaded; Full/Quarter origin placement confirmed** | May 2023 SDK build on 2026-09-28. Installable file `dist/StarfieldParticle.aex`, 189,440 bytes, packed version `0x8002`, SHA-256 `E2F304BFD3AC13A522CA71635E27F10BF8E0138BED9ACF5FDF4C30697D8B6FA0`; PDB SHA-256 `B0EB472A139D151424E87CF789E7A72B9E4220F94E1CC6FE1452EA016330B075`. `artifacts/plugin/2023/x64/Release/StarfieldParticle.aex` matches the installed copy. Owner Full/Quarter screenshots and the direct Quarter session show normalized `px 1920,1080,1080` and world `(0,0,0)`. The direct Quarter session reports `grid 960x540`; diagnostic isolation across multiple render contexts remains untested. |
| Build-2 graph render/persistence | AE 2023.5.0 Build 52 | **Save/reopen, Ctrl+D effect duplicate, same-name Ctrl+C/Ctrl+V replacement, undo and redo passed** | On 2026-09-28, saved and reopened `D:\Project\Code\test\testproject.aep` through AE's Open dialog. Comp 1, Medium Blue Solid 1, the Starfield Particle effect, visible particle output, and saved controls (Point, rate 100, origin 1920/1080/1080, velocity Y 0.30, lifetime 2 s, size 10) returned. The current installed `0x8002` plug-in matches `artifacts/plugin/2023/x64/Release/StarfieldParticle.aex` (SHA-256 `E2F304BFD3AC13A522CA71635E27F10BF8E0138BED9ACF5FDF4C30697D8B6FA0`). Selecting the effect and pressing Ctrl+D added `Starfield Particle 2` with matching visible parameters; Ctrl+Z removed it, Ctrl+Shift+Z restored it, and Ctrl+Z removed it again. To check same-name copy/paste, duplicated the solid layer, changed the duplicate layer's effect rate from 100 to 25, copied the original layer's Starfield effect with Ctrl+C, selected the duplicate layer's existing effect and pressed Ctrl+V; the target rate returned to 100 and only one effect instance remained on that layer. Undo restored the rate and removed the temporary layer; the original single-layer project was saved. The previously reported missing-file warning did not recur when opening the actual test path; the owner attributes that warning to a path/open-flow mismatch. These checks used AE Controls mode; Node Graph-specific persistence still needs a separate check. |
| Apply-time crash | AE 2023 installed at `D:\Software\Adobe\Adobe After Effects 2023`; plug-in build `0x8002` (24 parameters, `artifacts/plugin/2023/x64/Release/StarfieldParticle.aex`, 21:19) | **Open: crashed once while applying the effect** | Dump `5f8321e1-3dac-4b7e-b3b7-4fa60b9b283b.dmp` (2026-09-27 21:35). Exception `0x40000015` (fatal app exit, not an access violation), raised on a thread whose stack carries `sentry_crashpad.dll` (Adobe crash handler) and NVIDIA OpenGL/D3D12 frames; scanning the captured stacks found no return address inside `StarfieldParticle.aex`, so the fatal exit did not happen under our own frame. The dump also shows the reference `Stardust_panel.aex` and Adobe plug-ins loaded, and our PDB path. Mitigations in the same commit: the Options readout now refuses to check out parameters without a render context, and `STARFIELD_FLAT_RENDER=1` bisects the graph render path against the flat path. Next steps are listed under "Crash triage" below. |
| M2 particle render | AE 2023.5.0 Build 52, corrected build `0x8002` | **Loaded; visible render and source compositing pass at 8-bpc** | The current installed binary hashes to the recorded May 2023 SDK artifact. At t=2.75 s after reopening the saved test project, Full Options reported 200 live particles, `layer 3840x2160 ds 1/1 ref/grid 3840x2160`, host/normalized origin `[1920,1080,1080]`, and world `(0,0,0)`. With the Composition viewer explicitly set to Quarter, Options reported `layer 3840x2160 ds 1/4,1/4 ref 3840x2160 grid 960x540`, host origin `[480,270,270]`, normalized origin `[1920,1080,1080]`, and world `(0,0,0)`. The purple source solid remains visible beneath the particle trail. 16/32-bpc, cancellation, render-queue behavior, and multi-context diagnostic isolation remain open. |
| M3-01 shapes and playback | AE 2023.5.0 Build 52, corrected build `0x8002` loaded | **Quarter playback advances and visibly updates the Point emitter; Box/Sphere/Disc and reverse-time determinism unverified** | A RAM preview with the Composition viewer set to Quarter advanced from 2:45 to 4:55 in about 1.5 seconds; the particle cluster changed and rose. Playback was stopped, the viewer restored to Full, the playhead restored to 2:45, and the project saved. This confirms playback is not frozen at Quarter; it does not establish frame-for-frame reverse-time determinism or shape parity. |
| M3-02 force/appearance | AE 2023.5.0 Build 52, corrected candidate not loaded | Core implementation; host visual output unverified | Gravity, drag, color and the size/opacity age curves have core + control coverage (IDs 17-24). Confirm visible change: with defaults the picture must be unchanged, then set Gravity Y = -2 and Size End = 1 and re-render. |
| P-02 dockable panel | AE 2023.5.0 Build 52 | **Host connection not tested: extension is not installed** | The system CEP extension folder contains no Starfield entry. The user-authorized source path `newStardust/cep/_panel` does not exist; the repository panel is `newStardust/cep_panel`. Read-only queries found no `PlayerDebugMode` key/value under `CSXS.11` or `CSXS.12`; the panel is unsigned, and this project will not change that host-wide setting. Correct source-path authorization plus the owner's deliberate manual debug-mode decision are required before the panel can be loaded. The screenshot remains an effect-control form, not a node editor; see P-02A for the separate canvas task. |
| Delivery examples | AE 2023; exact build not recorded | Not checked | Apply Spark, Snow and Floating Light from `docs/examples.md` and record a still frame at t ≥ 1 s for each |
| M2 point-control units | AE 2023.5.0 Build 52 | **Measured:** absolute pixels scaled by preview resolution | Full/Quarter readouts show center `[1920,1080,1080]` → `[480,270,270]` as `ds` changes 1/1 → 1/4. The adapter restores the per-axis preview scale and the core preserves absolute pixels, including off-layer positions. A newer AE 2023 build needs its own host readout. |
| M2 preview geometry | AE 2023.5.0 Build 52 | **Single-effect Quarter grid observed; cross-context isolation remains open** | With the Composition viewer explicitly set to Quarter and the test comp/effect rendering, Options reported `ds 1/4,1/4`, `ref 3840x2160`, `grid 960x540`. This resolves the earlier mismatch caused by reading the right-side Preview panel while the Composition viewer was still Full. The diagnostic is process-global, so multiple effects or comps could overwrite the last-render record; that separate isolation behavior has not been checked. |
| M2 preview-resolution emitter origin | AE 2023.5.0 Build 52 | **Full and Quarter placement/playback checks passed** | Corrected-candidate Full/Quarter readouts show raw points `[1920,1080,1080]` at `ds 1/1` and `[480,270,270]` at `ds 1/4`; both normalize to `[1920,1080,1080]` and world `(0,0,0)`. The Quarter render grid was observed as `960x540`; a Quarter RAM preview showed the point trail moving from the expected center. The owner had already reported the visual offset fixed. Half/Third and anisotropic host views remain untested. |
| Newer AE families | Deferred by owner direction | Deferred | No current adaptation or qualification work |

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
9. **Source compositing** — the layer content stays visible under the particles rather than being replaced.
10. **Bit depth** — repeat rows 3–8 in 8-bpc, 16-bpc, and 32-bpc comps and record any difference.
11. **Lifecycle** — duplicate the effect, undo/redo, copy/paste, save, reopen, and render through the Render Queue.
12. **Cancellation** — start a RAM preview on a heavy setting and stop it; the effect must abort without an error dialog.
13. **Panel** — install `cep_panel/` per its README, confirm the chain renders, edit one value, undo/redo it, then re-open the panel and confirm the values are current.
14. **Examples** — apply the three recipes from `docs/examples.md` and capture one still frame each at t ≥ 1 s.

## Options readout

The effect's `Options` button prints a read-only diagnostic summary in the order that matters when
something looks wrong: graph/control source with the live count, the layer size and downsample factor,
the reference and pixel grid of the **last rendered frame**, the raw emitter point with both world
interpretations (the readout's fallback and the frame's reference-based one), time and velocity, then
the remaining counts. `grav`/`col` are printed only when they differ from their defaults, because
`PF_OutData::return_msg` holds 255 characters and lines past the end are dropped — the earlier layout
put the origin lines there and silently lost them. It changes no pixels and no settings; the
interpretation table and the two-click D-05 measurement are in `docs/parameter-mapping.md`.

## Behavior status

| Area | Independent acceptance case | Status |
|---|---|---|
| Emitter | Emitter positions are deterministic from seed, rate, lifetime, and absolute time | Implemented for Point/Box/Sphere/Disc; the seed selects stable per-particle streams. Determinism and frame order are covered by `tests/core_tests.cpp` |
| Particle lifecycle | Birth rate, lifetime, age, and population cap behave consistently at arbitrary frame order | Implemented; half-open lifetime and newest-slot cap are documented in `docs/parameter-mapping.md` |
| Emitter shapes | Box/sphere/disc distributions produce deterministic positions | Implemented in core (M3-01): seeded uniform sampling inside the requested extent, bounded and reproducible; see `docs/parameter-mapping.md`. AE confirmation pending |
| Per-particle variation | A steady emitter animates on playback instead of looking frozen | Implemented in core (M3-01): per-particle birth offsets plus per-axis velocity spread from `core::Random`, covered by core tests. Host playback confirmation pending |
| Random Seed | Changing the seed changes the rendered pixels | Implemented (M3-01): the seed now keys every per-particle stream. Host confirmation pending |
| Forces | Each force has isolated enable/disable and stable parameter semantics | Implemented in core (M3-02): gravity and linear drag are force-node values with closed-form integration, authored by controls 17-20. Per-force enable/disable is not implemented: the Alpha chain has one force stage, whose effect is zero at default values. AE confirmation pending |
| Ages and appearance | Size, opacity, and color follow particle age | Implemented in core (M3-02): linear age curves from the appearance node and the Color Start/End, Size End and Opacity End controls; the rasterizer uses per-particle RGB/opacity/size. Covered by `tests/core_tests.cpp`; AE confirmation pending |
| Nodes | Graph connections validate cycles, missing inputs, and invalid references without crashing | Graph model/codec/evaluator handle the emitter → force → appearance → output chain, including stage-order enforcement, single-emitter and single-appearance rules, and graph/flat pixel parity. The CEP screenshot shows no node canvas; P-02A adds a visual fixed topology, while dynamic create/delete/rewire remains P-02B. |
| Rendering | Alpha, premultiplication, color depth, rowbytes, ROI, and downsample are explicit | Implemented for 8/16/32-bpc, ROI, rowbytes, and premultiplied alpha. Full and Quarter geometry were observed in AE 2023.5.0 Build 52; 16/32-bpc and other preview scales remain unqualified |
| Preview resolution | The same frame at Full/Half/Quarter puts particles in the same comp positions | Full and Quarter center placement are host-confirmed for AE 2023.5.0 Build 52; Quarter playback advances and updates visibly. Half/Third and exact reverse-time image comparison remain open |
| Compositing | Particles composite over the input instead of replacing it | Chosen default recorded in ADR 0005; not yet confirmed against the reference effect |
| Color management | Working-space conversion through documented AE suites | Not started; M2 performs no conversion (ADR 0005) |
| Control shape | Positions use point controls, rates use scalar sliders | Done in code: Emitter Origin is a 3D point; X/Y/Z velocity are sliders in layer heights/s. Build-2 appends graph data, source mode and capture action; current AE build is unverified |
| Emitter origin | The point control places the emitter and the render agrees with it | Preview-scale source fix and adapter regression added from AE 2023.5.0 Build 52 readouts; retest the rebuilt binary in AE |
| Point-control scaling | AE point values map to the same layer position at Full and Quarter | Source fix and adapter regression added from AE 2023.5 Build 52 evidence; host retest of the rebuilt candidate pending |
| CEP node canvas | The dockable panel visibly presents nodes, ports, and connectors | Not implemented. Current screenshot is a grouped effect-control form; see P-02A |
| Persistence | Save/reopen, effect copy and undo preserve graph identity and values | Implemented with AE arbitrary-data parameter callbacks; build-2 host verification required. Capturing controls samples current time into a constant graph and does not convert animation tracks. |
| Concurrency | Repeated concurrent renders return identical pixels and never mutate shared state | Not advertised (no MFR flag); the core render is a pure function of one request, which M6 must audit before claiming support |
| Cancellation | A host abort stops a long render predictably | Implemented through `PF_ABORT` polling in the simulation and rasterizer; unverified in a host |
| Bounded work | Extreme settings fail with a typed error instead of hanging the host | Implemented (`work_limit_exceeded` + manifest caps); the specific budget is a provisional constant pending M6 profiling |
| Presets | Import/export validates version and rejects malformed or oversized data | Not started as file import/export (M4-03). The three delivery examples ship as documented recipes and panel presets; the graph codec already provides the bounded, versioned container a preset will use |
| Panel | UI state synchronizes through a versioned protocol and tolerates disconnect/restart | Protocol v1 implemented in `cep_panel/` with bounded requests, typed errors and stale-state rejection; reads/writes go through supervised parameter streams (ADR 0009). Host qualification is the open gate; file import/export presets remain M4-03 |
