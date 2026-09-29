# Development plan: AE 2023 particle Alpha

Planning baseline: 2026-09-27.

Current owner scope supersedes the earlier multi-host plan: build and qualify AE
2023 only. The current Alpha must deliver emitter -> force -> appearance -> output,
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
- **M3-02 core implementation is complete (current version goal 1 and 3):** the emitter -> force -> appearance -> output chain evaluates with stage-order enforcement, closed-form gravity/drag integration and linear age curves for size, opacity and color. The split build visibly responds to `Gravity Y = -2` and `Size Over Life = 1`; drag, opacity and color curves still need host visual checks.
- **G-01–G-04 implementation:** model, validator, codec, four-stage runtime, arbitrary-data callbacks, legacy-control selection/capture, supervised edit surface and render snapshot bridge are implemented. The current core suite passes 6,196 checks and the adapter suite passes 395 checks. Graph values are constant; capture samples controls at one time. AE Controls save/reopen, effect copy and undo/redo passed; Node Graph persistence and build-1 migration remain host gates (ADR 0008).
- **P-02 parameter bridge / P-02A node canvas (partial):** `cep_panel/` implements ADR 0009 protocol v1 over supervised render-value streams and ADR 0014 project-saved node-layout streams. Current source adds a pinned target, top-down cards, signed node coordinates, viewport-wide marquee/group movement, cursor-anchored wheel zoom, unrestricted middle-button pan in a clipped viewport with no scrollbars, a bottom-left interactive minimap, Alt-drag copy preview, Ctrl+D, port connection gestures, a graph context menu, and a movable floating properties window. The owner confirmed the node canvas displays normally after reloading the CEP panel and reports no issue after installing the revision-7 AEX. Node positions are stored in the effect's AE parameters and sent as one undo group; pan and zoom are transient. A dedicated node move → undo/redo, duplication, and save/reopen pass remains open. Earlier form-layout AE 2023 testing confirmed target discovery, a `Size` edit updating the frame, and host undo restoring the picture. The owner’s AE 23.5x52 probe confirmed ExtendScript cannot read the arbitrary graph property (`CUSTOM_VALUE` is not implemented). ADR 0013 proposes an expression-backed script-visible snapshot and supervised commit stream; it is awaiting a separate AE 2023 topology transaction spike. P-02B topology commands remain guarded until that project-saved graph snapshot and atomic transaction carrier are qualified.
- **Next graph extension contract (not implemented):** an emitter must link to at least one Particle node. When it links to multiple Particle nodes, divide its emission evenly for the first implementation; random allocation is deferred. Force nodes connected in series execute in connection order; parallel force branches combine their forces. G-05/P-02B must pin deterministic ordering/remainder behavior, graph persistence and undo, and the exact branch merge contract before the evaluator or panel advertises these topologies.
- **Delivery examples (current version goal 5):** Spark, Snow and Floating Light are documented in `docs/examples.md` and shipped as panel presets; none has been rendered in the host yet.
- **Known renderer gaps:** output is still flat 2D discs; Z does not affect projection, depth, or occlusion. Texture/layer sources, motion blur, mesh/volume rendering, and dynamic graph editing (create/delete/rewire) remain open. ROI narrowing is deferred to profiling.
- **Render geometry and point values are separate:** the render grid comes from observed checked-out worlds plus `max_result_rect`, `ref_width/ref_height`, and `par` (ADR 0005). The owner’s corrected-candidate AE 2023.5.0 Build 52 screenshots plus a direct Quarter session confirm point controls shrink with Quarter preview (`1920,1080,1080` → `480,270,270`), while normalized pixels remain `[1920,1080,1080]`, world offset stays zero, and the single-effect Quarter grid reports `960x540`. Options reads a process-global record of whichever instance rendered last; diagnostics across multiple effects/comps remain unqualified.
- **Default look updated (D-02, owner delegated):** velocity Y now defaults to 0.3 layer heights per second so a freshly applied instance shows a rising trail instead of one static dot. Defaults affect new instances only.
- **Control surface reshaped through manifest revision 6 (D-03/M3-01/M3-02):** Velocity is three scalar sliders, `Origin` is a 3D point, and the 21 render controls are grouped under Emitter / Particle / Physics / Render. The graph/source/capture values bring the active non-input parameter count to 24; topic markers and the implicit input are included in `num_params`. IDs remain pre-release and will freeze at the first shared release.
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
- **Graph editing gestures.** The canvas is top-down; blank-space drag selects nodes, selected nodes move together, the wheel zooms around the cursor, the middle button pans, Alt-drag previews a duplicate, Ctrl+D duplicates the selection, and right-click opens graph actions. A target pin prevents AE layer-selection changes from retargeting the panel. Clicking a connection disconnects it; dropping a node over a connection inserts it by replacing one edge with two typed edges. The canvas command handlers exist, but topology edits must commit through a graph-backed AE undo transaction. The owner’s AE 23.5x52 probe confirmed ExtendScript cannot read the arbitrary graph property. Protocol v1 cannot perform graph edits; a bounded project-saved snapshot and transaction carrier is the P-02B architecture gate (ADR 0009). Do not repeat the probe.
- **No silent scope change.** Building the panel now contradicts the earlier "do not start a CEP panel" line; that line is superseded by this section and the tradeoff (a possible future UXP port) is accepted deliberately.

## M0 architecture decisions now locked

1. Product identity: `Starfield Particle`, category `Starfield FX`, match name `org.starfieldfx.particle`, and package ID `org.starfieldfx.aftereffects`; never reuse the old plug-in identity.
2. Parameter contract: manifest revision 6 currently assigns registration indices 1–32, including AE topic markers; the 24 active non-input parameters are still pre-release and will freeze at the first shared release. Graph `NodeId`, `EdgeId`, and `ParamKey` remain separate identity domains.
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

1. **Continue AE 2023 host qualification.** Full/Half/Third/Quarter centre normalization, Quarter Point playback, save/reopen, effect copy, undo/redo, all bit depths, transparency, lower-layer compositing, shape distinctions, gravity and size changes have host evidence. The render queue produced three frames with exact sampled monolith parity. Still test fresh add/build-1 project load, off-centre positioning, drag/color/opacity curves, control capture, graph-byte persistence, cancellation, reverse-time frame identity, and an in-flight Core switch.
2. **Continue CEP panel qualification** using the checklist in `cep_panel/README.md`. Automatic target discovery, a panel `Size` write and host undo updating the picture passed. Verify redo, focus refresh after other host edits, stale-state rejection and Node Graph synchronization.
3. **Record the three delivery examples** from `docs/examples.md`.
4. **Grow the graph past the fixed chain:** protocol v2 for create/delete/rewire, then the next node kernels (textures/layer sources, depth, spawn) each tied to an observed reference case.
5. **Deferred deliberately:** analytic ROI narrowing, Compute Cache, MFR, and GPU stay on their milestone cards.

## Sequencing note (owner direction, 2026-09-27)

The milestone table above was written before the owner restated that node-based editing is the product's core value. M3 and M4 swap priority: the graph model, serialization, and evaluation (**M4-01/02/03**, extended into **Wave G**) come before the M3 feature families, which are then implemented as node types. M5 feature families, M6 MFR, M7 GPU, and M8 packaging are unchanged. Nothing in the M0 contracts (IDs, time, pixels, version, match name) is invalidated by this re-ordering.

## Sources checked on 2026-09-27

- [Adobe AE developer portal](https://developer.adobe.com/after-effects/) — SDK download entry through Adobe Developer Console.
- [Adobe AE SDK What's New](https://ae-plugins.docsforadobe.dev/intro/whats-new/) — current 26.5 guide, 25.6 Windows ARM support, and SDK history.
- [Compatibility across versions](https://ae-plugins.docsforadobe.dev/intro/compatibility-across-multiple-versions/) — latest headers plus per-host testing guidance.
- [MFR](https://ae-plugins.docsforadobe.dev/effect-details/multi-frame-rendering-in-ae/) — shared-state constraints and Compute Cache.
- [PiPL resources](https://ae-plugins.docsforadobe.dev/intro/pipl-resources/) — match name permanence and PiPL/runtime flag consistency.
- [Adobe CEP-to-UXP transition announcement](https://blog.developer.adobe.com/en/publish/2026/09/investing-in-the-future-of-creative-cloud-extensibility-uxp-comes-to-our-flagship-applications) — AE UXP public beta target and CEP transition timeline.
