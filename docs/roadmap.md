# Development plan: AE 2023 and newer

Planning baseline: 2026-09-27.

## Current M0/M1 status

- **M0 contract work is in place:** parameter manifest, sequence-format specification, build matrix, and ADRs for product identity, time, and pixels/alpha are checked in. The M1 shell uses the selected internal identity `org.starfieldfx.particle`.
- **M1 implementation is in place:** native entry-point source, official PiPL pipeline, lifecycle dispatch, and legacy pass-through render are present. MFR, SmartFX, and float-color flags are not declared.
- **M1 SDK builds succeed:** the same Windows x64 target builds with the supplied May 2023 SDK and current 26.5 SDK. Both builds export `EffectMain` and `PluginDataEntryFunction2`.
- **M1 load smoke check passed:** the user confirmed the shell loads in AE 2023 after correcting PiPL stage encoding. The precise AE build is not recorded. Render pass-through, save/reopen, duplicate, undo/redo, and current-AE load remain unqualified.

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

### UI and panel policy

- MVP controls live in the native effect parameter UI. Implement the particle graph as parameters and effect-owned graph data before building a dockable panel.
- Keep any future panel on a versioned message/data contract; it may not share in-process C++ object layouts with the renderer.
- Do not start a new CEP panel. Adobe announced a phased CEP-to-UXP transition and an After Effects UXP public beta target of November 2026. Revisit a UXP panel after the AE beta/API is actually available and its capabilities are verified. Until then, the effect remains fully operable without a panel.

## M0 architecture decisions now locked

1. Product identity: choose a new stable effect match name, display name, vendor ID, and version scheme. Never use the old plug-in's match name or vendor identity.
2. Parameter contract: reserve explicit stable AE parameter IDs; separately define UUID-like `NodeId`, `EdgeId`, and persistent `ParamKey` types inside the graph schema.
3. Sequence storage: define a bounded, versioned binary representation with magic, schema version, lengths, and integrity checks; migrations are pure functions from one schema version to the next. Treat malformed/truncated data as a recoverable project error.
4. Time model: represent comp time and frame duration as signed integer rationals. Define negative time, subframe sampling, shutter samples, seed derivation, and particle birth ordering before implementing simulation.
5. Render semantics: specify coordinate spaces, pixel aspect/downsample, ROI, 8/16/32-bpc conversion, color space, alpha convention, and behavior when no source layer is needed.
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

M1 uses legacy `PF_Cmd_RENDER` only as a low-risk pass-through load test. M2 replaces it with SmartFX. Do not add `PF_OutFlag2_SUPPORTS_THREADED_RENDERING` until M6 passes.

## Compatibility and quality gates

- Every release tests the minimum host (AE 23.0), each still-supported major family, and the latest stable host. For each, record OS, exact AE build, SDK header/resource version, compiler, architecture, and result.
- Project lifecycle: fresh add, duplicate effect, undo/redo, copy/paste, save/reopen, render-only instance, missing/older/newer sequence schema.
- Render correctness: full frame and ROI, input-independent and input-dependent cases, 8/16/32-bpc, odd rowbytes, premultiplied/straight alpha, pixel aspect, downsample, color-space change, negative/subframe time, cancellation.
- Determinism: random-seed fixed and changing, repeated frames, reverse frame order, multiple comp rates, motion blur samples.
- Robustness: corrupted serialized data, extreme parameter values, allocation failure, missing optional suite/device, device reset, effect removal during preview, host shutdown.
- Concurrency: TSAN or equivalent core-level race checks where supported; AE MFR stress only after the serial renderer is stable. Compare output and cache hits across serial/MFR modes.
- Performance budgets are measured before choosing GPU work. Avoid claiming speedups from synthetic core benchmarks alone.

## Immediate next work

The first agent wave is M2 contract hardening plus AE parameter registration; these work on separate files. Deterministic point simulation and the CPU compositor follow the render contract. SmartFX integration follows once those interfaces are stable. See the task cards in `agent-backlog.md`.

## Sources checked on 2026-09-27

- [Adobe AE developer portal](https://developer.adobe.com/after-effects/) — SDK download entry through Adobe Developer Console.
- [Adobe AE SDK What's New](https://ae-plugins.docsforadobe.dev/intro/whats-new/) — current 26.5 guide, 25.6 Windows ARM support, and SDK history.
- [Compatibility across versions](https://ae-plugins.docsforadobe.dev/intro/compatibility-across-multiple-versions/) — latest headers plus per-host testing guidance.
- [MFR](https://ae-plugins.docsforadobe.dev/effect-details/multi-frame-rendering-in-ae/) — shared-state constraints and Compute Cache.
- [PiPL resources](https://ae-plugins.docsforadobe.dev/intro/pipl-resources/) — match name permanence and PiPL/runtime flag consistency.
- [Adobe CEP-to-UXP transition announcement](https://blog.developer.adobe.com/en/publish/2026/09/investing-in-the-future-of-creative-cloud-extensibility-uxp-comes-to-our-flagship-applications) — AE UXP public beta target and CEP transition timeline.
