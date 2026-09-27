# Development plan: AE 2023 and newer

Planning baseline: 2026-09-27.

## Current M0/M1/M2/M3-01 status

- **M0 contract work is in place:** parameter manifest, sequence-format specification, build matrix, and ADRs for product identity, time, and pixels/alpha are checked in. The M1 shell uses the selected internal identity `org.starfieldfx.particle`.
- **M1 implementation is in place:** native entry-point source, official PiPL pipeline, lifecycle dispatch, and legacy pass-through render are present.
- **M1 SDK builds succeed:** the same Windows x64 target builds with the supplied May 2023 SDK and current 26.5 SDK. Both builds export `EffectMain` and `PluginDataEntryFunction2`.
- **M1 load smoke check passed:** the user confirmed the shell loads in AE 2023 after correcting PiPL stage encoding. The precise AE build is not recorded. Render pass-through, save/reopen, duplicate, undo/redo, and current-AE load remain unqualified.
- **M2 render-slice code is present and builds:** SmartFX transport, the parameter bridge, deterministic simulation, and the CPU white-disc compositor are implemented; this deliberately minimal look is not feature parity. `docs/current-feature-audit.md` records the visible gaps. Both supplied SDK builds include the core target.
- **M2 host qualification is partial (M2-06):** AE 2023 loaded an earlier eight-control build and showed a center sprite. A later eleven-control revision rendered nothing because of the emitter-origin conversion; that path was changed. The current thirteen-control M3-01 build has not yet been reloaded in AE. Exact host build, Options readout, bit depths, downsample, lifecycle, and cancellation remain unqualified. See `docs/compatibility-matrix.md`.
- **M3-01 core implementation is complete:** seeded Point/Box/Sphere/Disc birth distributions and per-particle velocity spread are implemented and covered by the 3,828-check core suite. The current parameters remain flat AE controls pending graph persistence/evaluation; M3-01 host playback has not been confirmed.
- **G-01/G-02 graph foundation is implemented in the core:** UUID node/edge IDs, typed port/parameter identities, registry-owned schemas, bounded validation, and the schema-1 binary codec are covered by the 3,828-check core suite. Graphs are not yet evaluated or connected to AE sequence data.
- **Known renderer gaps:** output is still white 2D sprites with constant size and opacity. Z does not affect projection, depth, or occlusion. Age curves, forces, color/texture sources, motion blur, mesh/volume rendering, and graph editing remain open. ROI narrowing is deferred to profiling.
- **Static-review geometry finding resolved in code:** the old `host_render_layer_rect` helper assumed the SDK's downsample factor is a divisor, which the SDK documents inconsistently (the header says 1–999+, the `Resizer`/`PathMaster` samples treat it as a scale). That helper is gone; render geometry now comes from observed checked-out worlds plus `max_result_rect`, `ref_width/ref_height`, and `par`, carried through `pre_render_data` (ADR 0005), with a core test pinning the half-resolution mapping. Reduced-resolution renders still need host confirmation before being called supported.
- **Default look updated (D-02, owner delegated):** velocity Y now defaults to 0.3 layer heights per second so a freshly applied instance shows a rising trail instead of one static dot. Defaults affect new instances only.
- **Control surface reshaped by manifest revisions 2–3 (D-03/M3-01):** Velocity is three scalar sliders, `Emitter Origin` is a 3D point, and `Emitter Size`/`Velocity Spread` were appended. The current layout has thirteen user controls plus the implicit input. IDs remain pre-release and will freeze at the first shared release.
- **PiPL/runtime flag drift fixed (D-04):** the build now declares `PluginFlags.h`/`PluginVersion.h` as `AdditionalInputs` for the PiPL step and fails when the generated resource disagrees with them. The earlier AE "global outflags mismatch" came from exactly that drift.

## Support and toolchain policy

### Host versions

- Minimum host: After Effects 23.0 (2023).
- Rolling maximum: the newest stable AE release available at each release cut.
- Release qualification matrix: AE 23.0 as the minimum compatibility gate, the latest patch of each major family still claimed, and the newest stable host. At minimum this means 23.x, 24.x, 25.x, and 26.x while those families remain in scope.
- Beta hosts are smoke-tested when useful but are not release-qualified until stable. Record exact host build numbers in release evidence.
- Compile against the newest Adobe AE SDK available at the release cut, currently the 26.5 SDK guide. Runtime code must only call APIs and suites available in the running host; obtain optional suites defensively and provide CPU/no-suite fallbacks.
- Do not copy SDK headers, samples, PiPL tools, or binaries into the repository. Keep the SDK as a local build input from Adobe Developer Console and record SDK version/toolchain metadata in build artifacts.

Adobe's SDK guide recommends using the latest headers and checking compatibility in each host version. Its 26.5 additions include APIs/features explicitly marked as Premiere Pro beta only; these are not prerequisites for the AE effect. The supported AE range includes MFR-capable hosts, but compatibility with MFR does not itself prove this plug-in is thread-safe.

### Build and dependency policy

- Core language: C++20, exceptions caught at the C ABI boundary, RAII internally. Do not let C++ exceptions cross an Adobe entry point.
- Windows is the first shipping platform because the current project/workspace is Windows. Release target starts at x64. Keep source portable; add macOS universal and Windows ARM64 packages as separately qualified targets.
- For native Windows releases, pin the MSVC toolset and Windows SDK in `docs/build-matrix.md`; CMake and Ninja are optional for the host-independent core and are not part of the `.aex` build.
- Use the AE SDK's own PiPL/resource tooling and templates. Keep PiPL declarations and runtime flags generated from one manifest or checked for exact equality.
- First-party core has no third-party runtime dependencies. Add a library only for a concrete feature, use a maintained release, pin its version/commit and license metadata, hide its symbols, and test interaction with AE's process-wide dependencies.
- No private OpenGL context and no private thread pool in the first renderer. CPU is the deterministic reference; later GPU work must use documented AE GPU selectors/device APIs and have an explicit CPU fallback.

### UI and panel policy (revised 2026-09-27 after owner direction)

The owner's product statement: **node-based editing is the essence of the reference product**. A flat parameter list with emitters and forces bolted on is not the product, and implementing the M3 feature families as flat parameters first would mean building them twice once the graph exists. That changes the sequencing, not the contracts.

- **The graph is the parameter layer.** Nodes own ports, edges, and per-node parameters; the AE effect keeps a small number of top-level controls (for example enable/quality/preset) and stores the graph in sequence data. ADR 0005's `RenderRequest`, time, pixel, and error contracts stay as they are: the graph feeds the same host-independent boundary.
- **Graph foundation comes before feature families.** Model, validation, bounded serialization, and evaluation order are host-independent and testable without AE, so they land first (`Wave G` in `docs/agent-backlog.md`). After that, emitters/forces/modifiers are implemented as node types rather than as new flat settings.
- **The node editor must be a dockable panel; in-effect UI cannot host it.** Confirmed in the SDK: `PF_EffectCustomUISuite2` only hands out a Drawbot drawing reference (`PF_GetDrawingReference`) plus an overlay theme suite for stroking/filling paths and vertices. There is no widget toolkit, no text layout, no scrolling surface, so a graph editor there would mean hand-rolling text rendering and hit-testing. In-effect Drawbot UI stays reserved for *on-screen gizmos* (dragging the emitter, drawing velocity/force overlays in the comp window), which is exactly what the suite is designed for.
- **Panel risk and mitigation.** Adobe is phasing CEP out in favour of UXP with an AE UXP public beta target of November 2026. The panel therefore stays a thin client over a **versioned message/data protocol** that never shares in-process C++ layouts with the renderer, exactly as the architecture already requires. If CEP must be replaced by UXP later, the protocol and the graph stay; only the view layer is rewritten.
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
| M1 — loadable shell | One native effect, PiPL, one effect entry point plus SDK registration entry, About/global/sequence lifecycle, pass-through render, implicit input only | Loads in AE 23.0 and current stable AE; add/remove/save/reopen/copy/undo works; no MFR flag; graph storage and controls follow in M2/M4 |
| M2 — render vertical slice | SmartFX pre-render/render, bounded ROI, CPU-only point emitter/sprite, time/seed determinism, 8/16/32-bpc and correct rowbytes/alpha | Same request gives bit-identical output; out-of-order and repeated requests match; cancellation and allocation errors release all host resources |
| M3 — particle MVP | Point/box/sphere/disc emitters, birth/lifetime, velocity, gravity, drag, size/opacity curves, seed controls | Golden cases cover frame rate changes, non-integer frame rates, negative/subframe time, shutter sampling, and project reopen |
| M4 — graph and presets | Node/edge model, graph validation, schema migrations, preset import/export, native UI organization | Invalid graphs cannot hang/crash; stable identifiers survive node reordering and schema migration |
| M5 — feature families | Add modifiers/forces, auxiliary particles, layers/overrides, models/materials/lights, post effects, then volumetrics in priority order | Each family has a behavior spec, regression fixture, performance budget, and independent feature flag |
| M6 — MFR qualification | Audit shared state; use Compute Cache for shareable derived data; parallel render stress and host matrix | Explicit concurrency audit signed off; serial/MFR pixels match; no shared mutation, deadlocks, or cache aliasing; only then set threaded-rendering flag |
| M7 — acceleration and panel | Profile first; add documented AE GPU backend if worthwhile; evaluate UXP panel after public API stabilizes | CPU fallback always works; GPU/CPU output differences bounded and documented; panel may be absent/restarted without affecting render correctness |
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

- Every release tests the minimum host (AE 23.0), each still-supported major family, and the latest stable host. For each, record OS, exact AE build, SDK header/resource version, compiler, architecture, and result.
- Project lifecycle: fresh add, duplicate effect, undo/redo, copy/paste, save/reopen, render-only instance, missing/older/newer sequence schema.
- Render correctness: full frame and ROI, input-independent and input-dependent cases, 8/16/32-bpc, odd rowbytes, premultiplied/straight alpha, pixel aspect, downsample, color-space change, negative/subframe time, cancellation.
- Determinism: random-seed fixed and changing, repeated frames, reverse frame order, multiple comp rates, motion blur samples.
- Robustness: corrupted serialized data, extreme parameter values, allocation failure, missing optional suite/device, device reset, effect removal during preview, host shutdown.
- Concurrency: TSAN or equivalent core-level race checks where supported; AE MFR stress only after the serial renderer is stable. Compare output and cache hits across serial/MFR modes.
- Performance budgets are measured before choosing GPU work. Avoid claiming speedups from synthetic core benchmarks alone.

## Immediate next work

1. **Requalify the current M3-01 build in AE 2023.** The existing host evidence is from older control revisions; use the Options readout at t ≥ 1 s, then test playback and Full/Half/Quarter resolution.
2. **G-03: graph evaluation** into `RenderRequest`, with bit-identical output to the existing settings path for the built-in emitter → output graph.
3. **G-04: AE persistence and migration** so new instances create a graph and current parameter values can migrate without changing AE IDs.
4. **Move remaining behaviors onto nodes** after graph evaluation: age curves, appearance, forces, textures, depth, and rendering families; implement the editor as a thin client over a versioned protocol.
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
