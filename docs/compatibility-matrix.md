# Behavior inventory

Use this file to turn observed behavior into requirements before implementing each feature. Do not infer undocumented internal algorithms from binary details. Static deductions stay hypotheses until a host pass or a reference-effect observation confirms them.

## Host qualification checkpoint

| Check | Host | Status | Evidence / next step |
|---|---|---|---|
| M1 plug-in discovery and load | AE 2023, exact build not recorded | User-confirmed pass | Effect loads; render pass-through, add/remove, save/reopen, duplicate, and undo/redo still unrecorded |
| M1 discovery and load | Current AE 26.x | Not checked | Record exact host build before claiming support |
| M2 particle render | AE 2023; exact build not recorded | Partial, old revision only | The eight-control build loaded and rendered a center sprite. A later eleven-control revision rendered nothing after adding Emitter Origin. The conversion was rewritten; the current thirteen-control M3-01 build has not been checked in AE. |
| M3-01 shapes and playback | AE 2023; exact build not recorded | Not checked | Core tests cover seeded distributions and velocity spread; install the current build and confirm at t ≥ 1 s with the Options readout. |
| M2 point-control units | Any host | Fixed in code, unverified in a host | Documented delivery is absolute layer pixels; the adapter normalizes through a ladder (pixels / legacy percentage / fixed-point) and the Options readout prints host values, interpreted pixels, and world position so the real delivery can be recorded |
| M2 preview geometry | Any host | Fixed in code, unverified in a host | Static review found the old adapter derived the render grid from `in_data->downsample_x/y`, whose direction the SDK documents inconsistently. The adapter now derives geometry from observed checked-out worlds (`docs/adr/0005`); a core test pins the half-resolution mapping |
| M2 particle render | Current AE 26.x | Not checked | Same checklist; record the exact host build |

## M2-06 host smoke checklist

Load the current build once, then record host, build, and result per row:

1. **Load** — "Starfield Particle" appears under `Starfield FX` and applies without an error dialog.
2. **Controls** — the effect shows the input layer plus thirteen controls with the manifest ranges and defaults, including Emitter Origin, three velocity sliders, Emitter Size, and Velocity Spread (manifest revision 3).
3. **First pixels** — defaults render the seeded emitter; at t ≥ 1 s, changing Velocity Y from `0.3` to `0.5` layer heights/s produces a visibly faster upward trail.
4. **Options readout** — click the effect's `Options` button and record the whole text. It reports the frame time, the live particle count, the layer/frame grid, and the velocity as both read and converted. This is the fastest way to classify a rendering surprise; see `docs/parameter-mapping.md`.
5. **Determinism** — scrubbing forward and backward over the same frames renders identical frames.
6. **Rate and lifetime** — Birth Rate and Particle Lifetime change the trail length; Particle Count caps how many sprites can be alive.
7. **Size and opacity** — both visibly change the sprite; size `0` renders nothing.
8. **Source compositing** — the layer content stays visible under the particles rather than being replaced.
9. **Bit depth** — repeat rows 3–7 in 8-bpc, 16-bpc, and 32-bpc comps and record any difference.
10. **Lifecycle** — duplicate the effect, undo/redo, copy/paste, save, reopen, and render through the Render Queue.
11. **Cancellation** — start a RAM preview on a heavy setting and stop it; the effect must abort without an error dialog.

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
| Forces | Each force has isolated enable/disable and stable parameter semantics | Not started |
| Nodes | Graph connections validate cycles, missing inputs, and invalid references without crashing | Not started |
| Rendering | Alpha, premultiplication, color depth, rowbytes, ROI, and downsample are explicit | Implemented for 8/16/32-bpc, ROI, rowbytes, and premultiplied alpha. Downsampling no longer depends on the ambiguous SDK factor: geometry comes from observed worlds. Preview-resolution rendering still needs host confirmation |
| Preview resolution | The same frame at Full/Half/Quarter puts particles in the same comp positions | Core test covers a half-resolution frame grid; host confirmation pending |
| Compositing | Particles composite over the input instead of replacing it | Chosen default recorded in ADR 0005; not yet confirmed against the reference effect |
| Color management | Working-space conversion through documented AE suites | Not started; M2 performs no conversion (ADR 0005) |
| Control shape | Positions use point controls, rates use scalar sliders | Done in code: Emitter Origin is a 3D point; X/Y/Z velocity are sliders in layer heights/s. Manifest revision 3 has thirteen controls; current AE build is unverified |
| Emitter origin | The point control places the emitter and the render agrees with it | Implemented as host pixels → world conversion, pinned by `tests/core_tests.cpp` (`layer_point_to_world`, unit ladder); host confirmation pending |
| Point-unit tolerance | A host delivering pixels, a legacy percentage, or fixed-point values all place the emitter sensibly | Implemented as a documented shim; remove the unused branches once a host pass records the real delivery (backlog D-05) |
| Persistence | Save/reopen and schema migration preserve settings and graph identity | Parameter persistence is AE-owned; sequence data and migrations are M4 |
| Concurrency | Repeated concurrent renders return identical pixels and never mutate shared state | Not advertised (no MFR flag); the core render is a pure function of one request, which M6 must audit before claiming support |
| Cancellation | A host abort stops a long render predictably | Implemented through `PF_ABORT` polling in the simulation and rasterizer; unverified in a host |
| Bounded work | Extreme settings fail with a typed error instead of hanging the host | Implemented (`work_limit_exceeded` + manifest caps); the specific budget is a provisional constant pending M6 profiling |
| Presets | Import/export validates version and rejects malformed or oversized data | Not started |
| Panel | UI state synchronizes through a versioned protocol and tolerates disconnect/restart | Deferred until a validated workflow needs one |
