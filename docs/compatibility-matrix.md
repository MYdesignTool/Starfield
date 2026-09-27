# Behavior inventory

Use this file to turn observed behavior into requirements before implementing each feature. Do not infer undocumented internal algorithms from binary details. Static deductions stay hypotheses until a host pass or a reference-effect observation confirms them.

## Host qualification checkpoint

| Check | Host | Status | Evidence / next step |
|---|---|---|---|
| M1 plug-in discovery and load | AE 2023, exact build not recorded | User-confirmed pass | Effect loads; render pass-through, add/remove, save/reopen, duplicate, and undo/redo still unrecorded |
| M1 discovery and load | AE 2023, exact build not recorded | User-confirmed pass for the empty M1 shell | Current build must be qualified separately |
| Build-2 graph render/persistence | AE 2023, exact build not recorded | Not checked | Current AE 2023 target binary has not been loaded; test graph source, save/reopen, duplication and undo/redo |
| Apply-time crash | AE 2023 installed at `D:\Software\Adobe\Adobe After Effects 2023`; plug-in build `0x8002` (24 parameters, `artifacts/plugin/2023/x64/Release/StarfieldParticle.aex`, 21:19) | **Open: crashed once while applying the effect** | Dump `5f8321e1-3dac-4b7e-b3b7-4fa60b9b283b.dmp` (2026-09-27 21:35). Exception `0x40000015` (fatal app exit, not an access violation), raised on a thread whose stack carries `sentry_crashpad.dll` (Adobe crash handler) and NVIDIA OpenGL/D3D12 frames; scanning the captured stacks found no return address inside `StarfieldParticle.aex`, so the fatal exit did not happen under our own frame. The dump also shows the reference `Stardust_panel.aex` and Adobe plug-ins loaded, and our PDB path. Mitigations in the same commit: the Options readout now refuses to check out parameters without a render context, and `STARFIELD_FLAT_RENDER=1` bisects the graph render path against the flat path. Next steps are listed under "Crash triage" below. |
| M2 particle render | AE 2023; exact build not recorded | Partial, old revision only | The eight-control build loaded and rendered a center sprite. A later eleven-control revision rendered nothing after adding Emitter Origin. The conversion was rewritten; the current 24-parameter build has not been checked in AE. |
| M3-01 shapes and playback | AE 2023; exact build not recorded | Core implementation only | Install the current build and confirm at t ≥ 1 s with the graph-aware Options readout |
| M3-02 force/appearance | AE 2023; exact build not recorded | Core implementation only | Gravity, drag, color and the size/opacity age curves have core + control coverage (IDs 17-24). Confirm visible change: with defaults the picture must be unchanged, then set Gravity Y = -2 and Size End = 1 and re-render. |
| P-02 dockable panel | AE 2023; exact build not recorded | Not checked | Panel appears under Window > Extensions, `Lookup: name` resolves, an edit updates the comp, one undo/redo restores it, and a stale edit is rejected. See `cep_panel/README.md`. |
| Delivery examples | AE 2023; exact build not recorded | Not checked | Apply Spark, Snow and Floating Light from `docs/examples.md` and record a still frame at t ≥ 1 s for each |
| M2 point-control units | Any host | Fixed in code, corroborated by observation, still unverified for our own control | Documented delivery is absolute layer pixels; the adapter normalizes through a ladder (pixels / legacy percentage / fixed-point) and the Options readout prints host values, interpreted pixels, and world position so the real delivery can be recorded. **Corroboration:** the reference product's own `Origin XY` parameter reads `[1920, 1080]` in a 3840×2160 composition (`docs/reference-parameter-map.md`), i.e. point-style parameters carry layer pixels in this host. Our Options readout is still the only way to close D-05 for our control |
| M2 preview geometry | Any host | Fixed in code, unverified in a host | Static review found the old adapter derived the render grid from `in_data->downsample_x/y`, whose direction the SDK documents inconsistently. The adapter now derives geometry from observed checked-out worlds (`docs/adr/0005`); a core test pins the half-resolution mapping |
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

## Fixed: emitter offset at reduced preview resolution (2026-09-27)

Symptom reported from the host: at Half/Quarter preview the emitter sits far from where it sits at
Full resolution. Cause: point controls are delivered in **full-resolution** layer pixels, but the
parameter bridge divided them by `in_data->width/height`, which shrink with the preview resolution —
at quarter resolution the conversion scaled the emitter origin by four. The renderer's world-to-pixel
mapping was already correct: it maps world units through the preview-sized frame grid and the
full-resolution `ref_width/ref_height`.

Fix: pre-render now performs the input checkout **first** and passes `PF_CheckoutResult::ref_width/
ref_height` into the parameter conversion, so both sides use the same reference. At Full resolution
`ref_*` equals `in_data->width/height`, so nothing changes there.

**Known remaining gap:** the Capture action has no render context, so it still converts with
`in_data->width/height`; capturing while a reduced-resolution preview is active can bake an offset
origin into the stored graph. The fix belongs with the manifest-revision-7 work: store the raw
control values (or the observed reference size) instead of a resolution-dependent world position.

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
4. **Options readout** — click the effect's `Options` button and record the whole text. It reports the frame time, the live particle count, the layer/frame grid, the velocity as both read and converted, and the gravity/drag and color/size/opacity curve values. This is the fastest way to classify a rendering surprise; see `docs/parameter-mapping.md`.
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

The effect's `Options` button prints a read-only diagnostic summary: frame time, live particle count, layer size, the raw downsample factor and pixel aspect ratio, every parameter as read by the code, and the emitter origin both raw (percent) and converted (world). It changes no pixels and no settings. It exists so a host-side surprise can be classified without a debugger; the interpretation table is in `docs/parameter-mapping.md`.

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
| Nodes | Graph connections validate cycles, missing inputs, and invalid references without crashing | Graph model/codec/evaluator handle the emitter → force → appearance → output chain, including stage-order enforcement, single-emitter and single-appearance rules, and graph/flat pixel parity. The editor displays the fixed v1 chain; dynamic node creation, deletion, and rewiring are not implemented. |
| Rendering | Alpha, premultiplication, color depth, rowbytes, ROI, and downsample are explicit | Implemented for 8/16/32-bpc, ROI, rowbytes, and premultiplied alpha. Downsampling no longer depends on the ambiguous SDK factor: geometry comes from observed worlds. Preview-resolution rendering still needs host confirmation |
| Preview resolution | The same frame at Full/Half/Quarter puts particles in the same comp positions | Core test covers a half-resolution frame grid; host confirmation pending |
| Compositing | Particles composite over the input instead of replacing it | Chosen default recorded in ADR 0005; not yet confirmed against the reference effect |
| Color management | Working-space conversion through documented AE suites | Not started; M2 performs no conversion (ADR 0005) |
| Control shape | Positions use point controls, rates use scalar sliders | Done in code: Emitter Origin is a 3D point; X/Y/Z velocity are sliders in layer heights/s. Build-2 appends graph data, source mode and capture action; current AE build is unverified |
| Emitter origin | The point control places the emitter and the render agrees with it | Implemented as host pixels → world conversion, pinned by `tests/core_tests.cpp` (`layer_point_to_world`, unit ladder); host confirmation pending |
| Point-unit tolerance | A host delivering pixels, a legacy percentage, or fixed-point values all place the emitter sensibly | Implemented as a documented shim; remove the unused branches once a host pass records the real delivery (backlog D-05) |
| Persistence | Save/reopen, effect copy and undo preserve graph identity and values | Implemented with AE arbitrary-data parameter callbacks; build-2 host verification required. Capturing controls samples current time into a constant graph and does not convert animation tracks. |
| Concurrency | Repeated concurrent renders return identical pixels and never mutate shared state | Not advertised (no MFR flag); the core render is a pure function of one request, which M6 must audit before claiming support |
| Cancellation | A host abort stops a long render predictably | Implemented through `PF_ABORT` polling in the simulation and rasterizer; unverified in a host |
| Bounded work | Extreme settings fail with a typed error instead of hanging the host | Implemented (`work_limit_exceeded` + manifest caps); the specific budget is a provisional constant pending M6 profiling |
| Presets | Import/export validates version and rejects malformed or oversized data | Not started as file import/export (M4-03). The three delivery examples ship as documented recipes and panel presets; the graph codec already provides the bounded, versioned container a preset will use |
| Panel | UI state synchronizes through a versioned protocol and tolerates disconnect/restart | Protocol v1 implemented in `cep_panel/` with bounded requests, typed errors and stale-state rejection; reads/writes go through supervised parameter streams (ADR 0009). Host qualification is the open gate; file import/export presets remain M4-03 |
