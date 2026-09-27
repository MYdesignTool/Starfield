# Development plan: AE 2023 particle Alpha

Planning baseline: 2026-09-27.

Current owner scope supersedes the earlier multi-host plan: build and qualify AE
2023 only. The current Alpha must deliver emitter -> force -> appearance -> output,
a minimal dockable editor, four emitter shapes, gravity/drag, color and life curves,
project persistence/copy/undo behavior, and spark/snow/floating-light examples.
Full observed Stardust functionality and architecture/performance improvements
remain the long-term goal; this Alpha does not complete that goal.

## Current M0/M1/M2/M3-01 status

- **M0 contract work is in place:** parameter manifest, sequence-format specification, build matrix, and ADRs for product identity, time, and pixels/alpha are checked in. The M1 shell uses the selected internal identity `org.starfieldfx.particle`.
- **M1 implementation is in place:** native entry-point source, official PiPL pipeline, lifecycle dispatch, and legacy pass-through render are present.
- **M1 SDK builds succeed:** the same Windows x64 target builds with the supplied May 2023 SDK and current 26.5 SDK. Both builds export `EffectMain` and `PluginDataEntryFunction2`.
- **M1 load smoke check passed:** the user confirmed the shell loads in AE 2023 after correcting PiPL stage encoding. The precise AE build is not recorded. Render pass-through, save/reopen, duplicate, undo/redo, and current-AE load remain unqualified.
- **M2 render-slice code is present and builds:** SmartFX transport, the parameter bridge, deterministic simulation, and the CPU white-disc compositor are implemented; this deliberately minimal look is not feature parity. `docs/current-feature-audit.md` records the visible gaps. Both supplied SDK builds include the core target.
- **M2/G-04 host qualification is partial:** AE 2023 loaded an earlier eight-control build and showed a center sprite. The current twenty-four-parameter plugin has not been loaded in AE. Record the exact host build, Options readout, bit depths, preview scale, save/reopen, effect copy, capture, gravity/color edits, undo/redo and cancellation results in `docs/compatibility-matrix.md`.
- **M3-01 core implementation is complete:** seeded Point/Box/Sphere/Disc birth distributions and per-particle velocity spread are implemented. Graph parity regression covers these shapes and supported pixel formats; AE playback remains unconfirmed.
- **M3-02 core implementation is complete (current version goal 1 and 3):** the emitter -> force -> appearance -> output chain evaluates with stage-order enforcement, closed-form gravity/drag integration and linear age curves for size, opacity and color. Gravity X/Y/Z (17-19), Linear Drag (20), Color Start/End (21/22), Size End (23) and Opacity End (24) are registered supervised controls whose defaults reproduce the previous look. AE confirmation pending.
- **G-01–G-04 implementation:** model, validator, codec, four-stage runtime, arbitrary-data callbacks, legacy-control selection/capture, supervised edit surface and render snapshot bridge are implemented. 4,735 core assertions and 249 adapter assertions pass. Graph values are constant; capture samples controls at one time. Save/reopen/copy/undo remain AE host gates (ADR 0008).
- **P-02 panel implementation (current version goal 2):** `cep_panel/` implements ADR 0009 protocol v1 over supervised parameter streams; the chain view is fixed. AE 2023 host qualification is the open gate.
- **Delivery examples (current version goal 5):** Spark, Snow and Floating Light are documented in `docs/examples.md` and shipped as panel presets; none has been rendered in the host yet.
- **Known renderer gaps:** output is still flat 2D discs; Z does not affect projection, depth, or occlusion. Texture/layer sources, motion blur, mesh/volume rendering, and dynamic graph editing (create/delete/rewire) remain open. ROI narrowing is deferred to profiling.
- **Static-review geometry finding resolved in code:** the old `host_render_layer_rect` helper assumed the SDK's downsample factor is a divisor, which the SDK documents inconsistently (the header says 1–999+, the `Resizer`/`PathMaster` samples treat it as a scale). That helper is gone; render geometry now comes from observed checked-out worlds plus `max_result_rect`, `ref_width/ref_height`, and `par`, carried through `pre_render_data` (ADR 0005), with a core test pinning the half-resolution mapping. Reduced-resolution renders still need host confirmation before being called supported.
- **Default look updated (D-02, owner delegated):** velocity Y now defaults to 0.3 layer heights per second so a freshly applied instance shows a rising trail instead of one static dot. Defaults affect new instances only.
- **Control surface reshaped by manifest revisions 2–5 (D-03/M3-01/M3-02):** Velocity is three scalar sliders, `Emitter Origin` is a 3D point, `Emitter Size`/`Velocity Spread` were appended, and revision 5 appended the force/appearance controls (17-24) after the reserved graph/source/capture IDs. The current layout has twenty-one user controls plus the implicit input. IDs remain pre-release and will freeze at the first shared release; UI grouping is still one flat list (M3-03).
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
- **No silent scope change.** Building the panel now contradicts the earlier "do not start a CEP panel" line; that line is superseded by this section and the tradeoff (a possible future UXP port) is accepted deliberately.

## M0 architecture decisions now locked

1. Product identity: `Starfield Particle`, category `Starfield FX`, match name `org.starfieldfx.particle`, and package ID `org.starfieldfx.aftereffects`; never reuse the old plug-in identity.
2. Parameter contract: the pre-release manifest currently assigns IDs 1–13; freeze them at the first shared release. Graph `NodeId`, `EdgeId`, and `ParamKey` remain separate identity domains.
3. Sequence storage: schema 1 defines a bounded binary representation with magic, lengths, counts, CRC, and migration rules in `schema/sequence-format.md`.
4. Time model: comp time and frame duration remain signed integer rationals; negative time, subframes, shutter samples, seed derivation, and particle ordering must stay deterministic.
5. Render semantics: canonical coordinates, pixel aspect/downsample, ROI, 8/16/32-bpc conversion, color space, and premultiplied-alpha handling are recorded in ADR 0003. Behavior for source-independent output remains to be confirmed against the reference.
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

1. **Requalify the current build in AE 2023.** Test fresh add, build-1 project load, playback at t ≥ 1 s, Full/Half/Quarter resolution, gravity/drag/color/curve edits, control capture, save/reopen, effect copy and undo/redo.
2. **Qualify the CEP panel** using the checklist in `cep_panel/README.md` (name lookup, undo group, stale-state rejection, Node Graph rewrite).
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
