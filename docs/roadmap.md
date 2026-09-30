# Development plan: AE 2023 particle Alpha

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
- **P-02 panel / P-02A/P-02B/P-02D graph editor (source integration complete; AE acceptance open):** `cep_panel/` implements the canvas and graph transactions. P-02D creates/removes Emitter, Particle, Appearance, and Force AEX instances, writes node values, and commits the graph snapshot to the main Starfield Particle effect. Output remains a fixed visible terminal in the canvas and has no standalone AEX. Duplicate native node UUIDs now fail closed; deleting their graph node removes every matching effect instance. The May 2023 SDK build and focused CEP/core/adapter suites passed on the prior candidate. The owner has not yet exercised native node add/remove in AE; undo/redo, duplicate identity, save/reopen, direct node-effect parameter edits, and immediate render response remain AE 2023 gates. Raw AE-level duplicate effects are detected but not automatically imported/re-keyed.
- **P-02C over-life curves (source integrated; AE gates open):** Particle and optional Appearance inspectors draw piecewise-linear Size and Opacity curves. Graph mode persists curve payloads and endpoint values in the graph transaction; AE Controls mode uses the project curve bank. Point insertion/drag/removal, Life/value entry and scrubbing are implemented in the panel source. Exercise both storage paths and rendered appearance with the matching AE 2023 build.
- **G-05 core source implemented; Output owns the global cap:** Particle nodes own per-branch lifetime and age curves. Active Particle nodes are sorted by UUID; global emission slot `k` goes to branch `k % N`, preserving the emitter's rate, Output cap, birth times, and IDs. Force fan-in and parallel paths merge by particle identity, while reachable forces are accumulated once per branch in stable dependency order. An Output ancestry with no Particle renders transparent; active Force/Appearance bypass paths fail explicitly. Particle/Appearance precedence is resolved once per particle. Branches write directly into one pre-sized, global-ID-ordered buffer without per-branch particle vectors or a final sort. Emitter schema 3 has no global cap; fixed Output schema 2 stores Max Particles. Core tests pass 11,909 checks and adapter tests pass 678; focused graph view/edit/transaction, gateway, and startup suites pass. Multiple active emitters and stochastic allocation remain deferred. P-02B exposes dynamic graph editing in the CEP source through revision-checked transactions; AE carrier persistence/undo and rendered edits remain unqualified. ADR 0015 contains the core contract.
- **Delivery examples (current version goal 5):** Spark, Snow and Floating Light are documented in `docs/examples.md` and shipped as panel presets; none has been rendered in the host yet.
- **Known renderer and host gaps:** output is still flat 2D discs; Z does not affect projection, depth, or occlusion. Texture/layer sources, motion blur, mesh/volume rendering remain open. The per-node AE effect and graph-sync path is implemented in source but not yet accepted in AE 2023; complete that host pass before full P-02B qualification. ROI narrowing is deferred to profiling.
- **Render geometry and point values are separate:** the render grid comes from observed checked-out worlds plus `max_result_rect`, `ref_width/ref_height`, and `par` (ADR 0005). The owner’s corrected-candidate AE 2023.5.0 Build 52 screenshots plus a direct Quarter session confirm point controls shrink with Quarter preview (`1920,1080,1080` → `480,270,270`), while normalized pixels remain `[1920,1080,1080]`, world offset stays zero, and the single-effect Quarter grid reports `960x540`. Options reads a process-global record of whichever instance rendered last; diagnostics across multiple effects/comps remain unqualified.
- **Default look updated (D-02, owner delegated):** velocity Y now defaults to 0.3 layer heights per second so a freshly applied instance shows a rising trail instead of one static dot. Defaults affect new instances only.
- **Control surface through manifest revision 13:** revision 9 appends 34 hidden project streams for Size/Opacity curve banks and a supervised curve commit stream (IDs 45–79); revision 10 appends the Emitter Dimensions topic and Size X/Y/Z controls (IDs 80–84); revision 11 appends Size/Opacity Random controls (IDs 86–87); revision 12 gives Size X/Y/Z direct pixel units; revision 13 makes both life curves 0–100% multipliers. Zero curve count uses the linear 100%-to-endpoint curve. A Size edit now has a focused gateway regression proving it retains the project’s Opacity curve. AE qualification of the integrated graph/curve edit paths remains open.
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
- **Graph editing gestures.** The canvas is top-down; blank-space drag selects nodes, selected nodes move together, the wheel zooms around the cursor, the middle button pans, Alt-drag previews a duplicate, Ctrl+D duplicates the selection, and right-click opens graph actions. Clicking a connection disconnects it; dropping a node over a connection inserts it by replacing one edge with two typed edges. These gestures now submit revision-checked graph edits through the native snapshot/request/receipt carrier. In AE 23.5x52, direct scripting access to the arbitrary graph property is unavailable, so the expression carrier must be qualified for callback acknowledgement, one-step undo, persistence, stale rejection and render parity before this source integration is considered host-supported. Do not repeat the `CUSTOM_VALUE` probes.
- **No silent scope change.** Building the panel now contradicts the earlier "do not start a CEP panel" line; that line is superseded by this section and the tradeoff (a possible future UXP port) is accepted deliberately.

## M0 architecture decisions now locked

1. Product identity: `Starfield Particle`, category `Starfield FX`, match name `org.starfieldfx.particle`, and package ID `org.starfieldfx.aftereffects`; never reuse the old plug-in identity.
2. Parameter contract: manifest revision 11 assigns registration indices 1–88, including AE topic markers and hidden panel metadata; index 0 is AE's implicit input. Revisions 9–11 append curve-bank streams, emitter dimensions, and Particle variation. Graph `NodeId`, `EdgeId`, and `ParamKey` remain separate identity domains.
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
