# Agent-ready implementation backlog

This backlog is the task source for staged implementation work. Assign one task ID per branch/worktree. Tasks below have explicit file ownership to reduce conflicts; the integrating owner reviews and merges interfaces in dependency order.

## Current checkpoint

- **BUILD-23 (integration lead):** owner scope is now AE 2023 only. Script/MSBuild defaults and primary docs use the May 2023 SDK and `artifacts/plugin/2023/`; newer-host tasks are deferred. Owns build defaults and policy documentation only.
- **G-03:** emitter/output core runtime and graph/flat parity implemented in `070c33e`; the force/appearance kernels now complete the Alpha chain (closed-form gravity/drag integration, linear age curves for size/opacity/color, stage-order enforcement) and graph/flat parity is pinned by the core tests. 4,735 core assertions passed and the May 2023 SDK build succeeds.
- **G-04:** `GraphParameter.cpp` implements AE arbitrary-data ownership/codec callbacks; legacy controls preserve old-project animation, Node Graph mode consumes a pre-render snapshot, and Capture Current Controls uses AE's supervised parameter-change path. `graph_from_controls` now builds the full emitter -> force -> appearance -> output chain, and every bound control is supervised so a Node Graph edit rewrites the canonical graph in the same user-change transaction. Native adapter tests: 249 assertions passed; May 2023 SDK build passed. Save/reopen, duplicate, build-1 migration and undo/redo remain unqualified in AE.
- **M3-02 core + authoring surface:** gravity (17-19), linear drag (20), Color Start/End (21/22), Size End (23) and Opacity End (24) are registered AE controls whose defaults reproduce the previous look, so the visible improvement is reachable without the panel; the same controls are the panel's edit surface.
- **P-02 panel:** `cep_panel/` implements ADR 0009 protocol v1 (fixed chain view, editable parameters, one-undo-group writes, stale-state rejection, 32-change/64 KiB bounds, three example presets). Host qualification is the open gate.
- **Examples:** Spark, Snow and Floating Light are documented as exact parameter recipes in `docs/examples.md` and shipped as panel presets, so they can be accepted with or without the panel.
- **Time-dependent output flag:** `NON_PARAM_VARY` is present in PiPL/runtime because frame time changes rendered particles with constant parameters. MFR/GPU flags remain disabled.

- M0 architecture contracts and M1 SDK shell are in the source tree.
- The user confirmed the empty M1 shell loads in AE 2023. An earlier M2 build showed controls and a center sprite; the build-2 sixteen-parameter graph build needs host qualification.
- **G-03/G-04 core, adapter and AE 2023 build:** the May 2023 SDK target compiles, `tests/RunCoreTests.ps1` reports 4,735 core assertions, and the adapter mode reports 249 fake-host assertions.
- **M2-06 is partial:** host load/render evidence is from older parameter revisions. A later emitter-origin revision rendered nothing; the conversion path has since been rewritten. The current build has twenty-four registered parameters (13 original controls, graph/source/capture, then the force/appearance controls 17-24). Re-test the current build, then complete bit depth, preview resolution, lifecycle, source compositing, and cancellation checks. The Options readout (`ae_plugin/Diagnostics.cpp`) prints the gravity/drag and color/curve values as read; see `docs/compatibility-matrix.md`.
- **Product direction recorded (owner, 2026-09-27):** node-based editing is the essence of the reference product, so the graph foundation is promoted ahead of the M3 feature families; those families will be implemented as node types. See `Wave G` below and the raised `docs/roadmap.md` policy.
- **Static review carried out by the owner (2026-09-27):** the adapter geometry was rewritten to derive render-space scale from observed host worlds. M3-01 added Box/Sphere/Disc and seeded per-particle variation. The current build needs host playback and preview-resolution confirmation.
- **Host feedback after manifest revision 2 (2026-09-27):** the first revision-2 build rendered nothing. Cause: AE delivers point-control values as layer pixels while the bridge read them as percentages, so the default "centre" landed several layer heights off-canvas (D-05). Fixed in `Geometry.hpp/.cpp` with core tests; the point-unit ladder still needs a host readout to collapse to the confirmed branch.
- **Feedback rule adopted:** every unit or coordinate assumption that can move geometry silently must be covered by either a core test or a printed readout. Silent geometry shifts produce no AE error, so they must be observable by construction.
- Current Windows x64 `.aex` build succeeds against the May 2023 SDK; dual-SDK builds are historical. Local SDKs and artifacts are ignored by Git.
- Product direction: independent implementation of observed behavior. Static analysis is a feature-discovery aid, not proof of exact behavior or a source-code specification.

## Before agent work: human-owned Git checkpoint

| ID | Owner | Work | Done when |
|---|---|---|---|
| GIT-00 | User | Initialize the Git repository and make a clean baseline commit. | Source, schemas, docs, `AGENTS.md`, and build scripts are tracked; `AdobeSDK/` and `artifacts/` remain ignored. **Observed: done — clean tree on `main`.** |

No worker should create a branch/worktree from an uncommitted moving baseline. After this checkpoint, create one branch/worktree per assigned task ID.

## M1 closeout gate

| ID | Owner | Work | Depends on | Done when |
|---|---|---|---|---|
| HOST-01 | User / host operator | Finish the M1 host smoke pass in AE 2023 and record exact AE build, OS, and result. | M1 shell already loads | Effect can be added/removed, renders the input unchanged, duplicates, undo/redo works, and a saved project reopens. Record each result in `compatibility-matrix.md`. |
| HOST-02 | User / host operator | Newer-host qualification, deferred by owner direction. | Owner reopens scope | Do not infer support from historical SDK builds. |

These are host-operated checks; code agents can prepare a concise checklist but cannot mark the results without host evidence.

## Wave A — tasks suitable for parallel assignment

### M2-01 — Freeze the render request contract and rational-time helpers

- **Owner:** core agent.
- **Dependencies:** Git baseline; ADRs 0002 and 0003.
- **Owned files:** `include/starfield/core/Render.hpp`, new core time/helper files, and the corresponding ADR edits.
- **Scope:** specify normalized signed rational time, overflow behavior, half-open ROI coordinates, downsample/pixel-aspect spaces, pixel row layout, and cancellation/error semantics. Keep AE types out of the core. Reconcile the request/output ownership comments with the actual value types.
- **Do not do:** add SmartFX code, particle physics, or a new third-party library.
- **Acceptance:** all invalid/overflow cases have defined errors; frame and output structures have unambiguous units and ownership; the AE adapter can convert SDK values without reinterpretation.
- **Status:** implemented. `Error.hpp`, `Time.hpp/.cpp`, `Render.hpp/.cpp`, ADR 0005, and amendments to ADR 0002/0003. `FrameSpec` separates layer geometry, the render-resolution frame grid, and the ROI; `RenderOutput` owns a staging buffer with an explicit `row_bytes`; `validate_frame` covers the invalid cases; `tests/core_tests.cpp` covers normalization, overflow, and rejection paths.

### M2-02 — Bind the parameter manifest to AE controls

- **Owner:** AE UI/adapter agent.
- **Dependencies:** Git baseline; `schema/parameters.json` and `Settings.hpp` are the public contract.
- **Owned files:** new parameter adapter files under `ae_plugin/`, `ae_plugin/EffectMain.cpp` parameter dispatch, `include/starfield/core/Settings.hpp`, `src/core/Settings.cpp`, plus mapping documentation. Coordinate before touching the shared entry-point file.
- **Scope:** register all eight controls with stable IDs, labels, ranges, defaults, and correct PF types; map popup values and AE point controls into core settings. Keep ID 0 reserved for AE's implicit input. Explicitly reconcile the one-based popup default (`Point` = 1 in AE) with the zero-based `EmitterShape::point` core enum.
- **Do not do:** add graph controls, custom panels, or render algorithms.
- **Acceptance:** each schema row maps to one AE control and one core field; bounds/defaults match `Settings.cpp`; seed validation respects the manifest's `2147483647` maximum; parameter IDs and match name remain unchanged.
- **Status:** implemented through manifest revision 3. `ae_plugin/Parameters.hpp/.cpp` registers thirteen rows (index 0 is the implicit input, so `num_params = 14`), including three velocity sliders, Emitter Origin, Emitter Size, and Velocity Spread. The seed now drives per-particle shape and velocity streams. Full mapping and unit caveats are in `docs/parameter-mapping.md`; current AE control behavior still needs host confirmation.

These two tasks can run at the same time: M2-01 owns render/time contracts; M2-02 owns parameter registration and settings mapping. M2-03 waits for M2-02's settings mapping, while M2-04 waits for the M2-01 buffer contract and M2-03's particle type.

## Wave B — deterministic CPU vertical slice

### M2-03 — Implement deterministic point-emitter simulation

- **Owner:** simulation agent.
- **Dependencies:** M2-01 and M2-02; `Settings` validation.
- **Owned files:** new `include/starfield/core/ParticleSimulation.hpp` and `src/core/ParticleSimulation.cpp`; avoid AE adapter and pixel-buffer files.
- **Scope:** generate point-emitter particles from seed and absolute rational time. Define stable particle IDs, birth ordering, lifetime boundary, velocity integration, and cancellation polling. Each render request must stand alone; do not advance process-global state.
- **Acceptance:** same request yields the same ordered particle list; out-of-order frame requests agree with chronological requests; invalid settings are bounded before allocation.
- **Status:** implemented in `ParticleSimulation.hpp/.cpp`. Emission slots are `k / birth_rate`, the lifetime interval is half-open, the population cap keeps the newest slots, and the clock is anchored at host time 0. Determinism, frame-order independence, cap behavior, and cancellation are covered by `tests/core_tests.cpp`. The point emitter honours all three axes of velocity but renders depth only from M5 onward.

### M2-04 — Implement the CPU sprite rasterizer and source compositor

- **Owner:** CPU renderer agent.
- **Dependencies:** M2-01; consume M2-03's particle instance type after its interface is agreed.
- **Owned files:** new CPU renderer and pixel helper files under `src/core/` and `include/starfield/core/`; do not touch AE selectors.
- **Scope:** rasterize a minimal point/sprite into owned output storage; honor ROI, dimensions, row bytes, pixel format, premultiplied alpha, and source compositing. Use checked size arithmetic and return typed errors.
- **Acceptance:** bounds/ROI are respected, padding bytes are not assumed absent, alpha is correct, and allocation/unsupported-format failures return without leaking buffers.
- **Open behavior question:** confirm from the reference effect whether particles composite over the input or replace it. If evidence is unavailable, record the chosen default as an ADR decision instead of presenting it as verified compatibility.
- **Status:** implemented in `CpuRenderer.hpp/.cpp` with the compositing default recorded in ADR 0005 (premultiplied "over", source passthrough elsewhere, no color-space conversion in M2). Sprites carry a one-pixel analytic coverage ramp; ROI, row bytes, 8/16/32-bpc, source placement and straight-alpha input, and the bounded-work budget are covered by the core tests. An empty ROI is a legal, allocation-free request.

### M2-05 — Add the SmartFX AE transport adapter

- **Owner:** AE rendering agent.
- **Dependencies:** M2-01 and M2-02; agree the renderer call contract with M2-03/M2-04 before integration.
- **Owned files:** AE adapter implementation under `ae_plugin/`; coordinate shared `EffectMain.cpp` edits with M2-02.
- **Scope:** replace legacy `PF_Cmd_RENDER` with the documented SmartFX pre-render/render path; use scoped input/time checkouts, calculate bounded ROI, translate worlds to owned core buffers, copy output while host pointers are valid, and release every checkout on every exit path.
- **Do not do:** set MFR, float-color, or GPU flags as placeholders; keep a CPU fallback.
- **Acceptance:** host resources have one cleanup path, errors/cancellation map predictably, and PiPL/runtime flags agree with implemented selectors.
- **Status:** implemented in `ae_plugin/SmartRender.*`, `WorldBridge.*`, `Parameters.*`, `EffectMain.cpp`, `PluginFlags.h`, and `StarfieldPiPL.r`. Pre-render performs the single input checkout, declares `result_rect = request ∩ layer` and a request-independent `max_result_rect`, and keep `RETURNS_EXTRA_PIXELS` unset. Render re-fetches the same checkout id, converts worlds into owned buffers, runs the core, writes back while the pointer is valid, and releases the checkout on every path. Params check in through an RAII guard; `PF_ABORT` backs the cancellation contract; core errors map to stable AE errors.
- **Deliberate M2 limits (documented, not hidden):** the adapter does not narrow `result_rect` to analytic particle bounds, so it always fills the requested region; content-bounds narrowing is deferred to a profiling task.
- **Geometry revision after static review:** the adapter no longer derives the render grid from `in_data->downsample_x/y` (the SDK documents that factor inconsistently: the header describes a divisor in the range 1–999+, while `Resizer.cpp` and `PathMaster.cpp` multiply by it as a scale). Render geometry now comes from observed checked-out worlds plus `max_result_rect`, `ref_width/ref_height`, and `par`, carried from pre-render to render in `pre_render_data`; a missing state block fails loudly. `tests/core_tests.cpp` pins the half-resolution mapping. Preview-resolution behavior still needs host confirmation.

### M2-06 — Integrate and qualify the first particle render

- **Owner:** integration lead.
- **Dependencies:** M2-02 through M2-05.
- **Owned files:** integration fixes across adapter/core plus compatibility evidence.
- **Scope:** produce the first visible point emitter over a defined frame, confirm source alpha/compositing, and capture repeatable outputs at the supported bit depths. Run in AE 2023 before claiming minimum-host support; then check current AE.
- **Acceptance:** AE effect controls affect deterministic rendered pixels; repeated/reverse-time requests match; project save/reopen preserves controls; all claimed host evidence is recorded.
- **Status:** partial host pass in AE 2023 on an earlier build. The current build-2 revision has thirteen legacy controls plus graph/source/capture parameters, seeded shape emitters, and an emitter-origin conversion rewrite, but has not been reloaded in AE. The exact host build and graph-aware Options readout are missing; confirm current visual output before testing bit depths, save/reopen, and cancellation. Continue the checklist in `docs/compatibility-matrix.md`.

## Diagnostics and shipped defaults (current triage)

| ID | Owner | Work | Done when |
|---|---|---|---|
| D-01 | AE adapter agent | Options-button readout of what the effect actually receives: frame time, live particle count, layer/frame grid, downsample, pixel aspect, every parameter as read, and the converted velocity | Implemented in `ae_plugin/Diagnostics.cpp` (read-only; not part of the render contract). A support tool, kept until M8 diagnostics supersede it |
| D-02 | Owner + lead | Decide the shipped default look. With `velocity = (0,0,0)` and a 2 s lifetime a fresh instance shows one overlapping cluster at the layer center, and at comp time 0 exactly one particle exists by construction, so the effect can look static even though it works | **Done (owner delegated):** the manifest default is Velocity `(0, 0.3, 0)` layer heights/s plus `velocity_spread = 0.15`, mirrored by `Settings`, `schema/parameters.json`, and `docs/parameter-mapping.md`. Defaults affect new instances only |
| D-03 | Lead | Split Velocity into scalars and keep point controls for positions only | **Done:** manifest revision 2 (three velocity sliders in layer heights/s plus a 3D-point Emitter Origin). Recorded in `docs/parameter-mapping.md`, `schema/parameters.json`, and `Settings.hpp` |
| D-04 | Build owner | Make a PiPL/runtime flag mismatch impossible to ship: declare the flags/version headers as `AdditionalInputs` for the PiPL custom build and verify the generated resource against them | **Done:** `Starfield.vcxproj` declares `AdditionalInputs` (`Outputs` stays, or MSBuild skips the step with MSB8018) and `BuildPiPL.ps1` fails the build when `StarfieldPiPL.rc` disagrees with `PluginFlags.h`/`PluginVersion.h`. This was the cause of the AE "global outflags mismatch" report |
| D-05 | AE adapter agent | Treat point controls as host-pixel positions, not percentages | **Done (with a shim):** `core::layer_point_to_world()` and `core::host_point_component_to_layer_pixels()` convert the delivered value, folding the pixel aspect ratio into the horizontal axis. The ladder tolerates layer pixels (documented), a legacy percentage, and a fixed-point delivery. **Remove condition:** a host pass must record the real delivery through the Options readout, then the unused branches are deleted and `docs/parameter-mapping.md` reduces to the confirmed fact |

## Wave G — graph foundation (promoted ahead of Wave C by owner direction)

The owner confirmed that node-based editing is the product's essence. Wave A/B built the render boundary that the graph will feed; building M3's emitter/force families as flat parameters now would mean implementing each of them twice. Wave G therefore lands first, and M3 families become node types afterwards. M4-01/M4-02 are absorbed here.

| ID | Owner | Work | Dependencies | Done when |
|---|---|---|---|---|
| G-01 | Core graph agent | Typed node/port/edge/parameter model with stable identity domains and a validator (unknown node type, type mismatch, missing input, cycle, duplicate id) | ADR 0001 identity rules, ADR 0006 | **Implemented:** `Graph.hpp/.cpp` defines UUID node/edge IDs, typed port and parameter keys/values, bounded validation, emitter/output schemas and typed failures. Covered by reorder-stability, cardinality, cycle and feedback-boundary checks. G-02 adds core serialization; G-03 evaluates emitter/output graphs |
| G-02 | Core serialization agent | Bounded core serializer/parser with magic, schema version, counts, CRC, and safe unknown-version handling for `schema/sequence-format.md` | G-01 | **Core codec implemented:** `SequenceCodec.hpp/.cpp` validates before writing, emits canonical order, and bounds parsing before allocation. Regression cases cover all seven value kinds, round-trip, CRC, optional records, and malformed headers/records. No earlier graph schema exists to migrate. G-04 consumes these bytes in AE arbitrary parameter callbacks (ADR 0008) |
| G-03 | Core evaluation agent | Graph evaluation into the existing `RenderRequest`: dependency order, evaluation at rational time, cancellation, and emitter/output pixel parity | G-01, M2-01/03/04 | **Emitter/output runtime implemented:** `GraphEvaluation.hpp/.cpp`, ADR 0007. Stable dependency traversal, active-output reachability, semantic bounds, graph precedence and per-particle opacity. Core regression: 4,418 assertions, zero failures (2026-09-27). Current schema stores constant parameters; animated sampling/history requires M3-03's contract. AE does not yet supply snapshots; G-04 remains open |
| G-04 | AE adapter agent | Persist graph as AE arbitrary parameter data; retain animated legacy-control mode and provide explicit capture into node mode | G-02, G-03 | **Code complete; AE host gate open:** parameters 14–16 appended; AE owns graph bytes and lifecycle callbacks; old projects select animated AE Controls via `USE_VALUE_FOR_OLD_PROJECTS`; capture writes the current-time graph and selects Node Graph in one supervised parameter transaction. SmartFX consumes a pre-render snapshot. 192 adapter checks and May 2023 SDK build pass. Exercise save/reopen, copy, build-1 project load and undo/redo in AE 2023. No previous ID or sequence schema changed |
| P-01 | Owner + lead | Choose the AE 2023 dockable-panel bridge and specify its versioned protocol and failure behavior | Wave G contracts | **Architecture accepted in ADR 0009:** CEP `CSInterface.evalScript` → ExtendScript → supervised script-visible AE parameter streams → effect updates canonical arb graph in `PF_Cmd_USER_CHANGED_PARAM`. No direct arb-stream writes, sockets, or render-time AEGP queries. AE 2023 host gate remains open for scripted hidden-stream access and undo behavior. |
| P-02 | Panel agent | Build the dockable panel over ADR 0009. Display the single-emitter emitter → force → appearance → output chain, its ports/edges, and editable parameters; write changes through supervised AE streams. Topology display is fixed in protocol v1. | P-01; force/appearance parameter bindings | **Code complete; AE 2023 host gate open:** `cep_panel/` implements protocol v1 (chain view with connections, editable parameters, one-undo-group writes, baseRevision/stale-state rejection, 32-change and 64 KiB bounds, typed errors, three example presets). Acceptance still requires host evidence that the panel reads and edits the active effect, that the render updates, that one undo/redo group restores/reapplies edits, and that save/reopen and effect copy preserve values. |
| P-03 | Panel agent | On-screen emitter gizmo using `PF_EffectCustomUISuite2` + overlay theme (Drawbot only; no graph editing in-effect) | P-02, ADR 0005 geometry | Dragging the gizmo changes the emitter position and matches the rendered trail |

## Wave C — MVP controls and behavior families (after Wave G)

| ID | Work | Dependencies | Main ownership | Gate |
|---|---|---|---|---|
| M3-01 | Box/sphere/disc emitter distributions and documented coordinate mapping, implemented as node parameters | Wave G, M2-03/04 | Core simulation | **Core implementation complete:** `core::Random` supplies per-particle streams keyed by (seed, id, purpose); emitter shape, extent, velocity, and spread flow through the emitter graph node. Each shape is bounded and reproducible, and `seed` changes pixels. AE playback and preview-resolution confirmation remain open |
| M3-02 | Particle age curves, size/opacity over life, gravity and drag as node types | Wave G | Core simulation/graph | **Core implemented; host confirmation pending:** force and appearance node types plus the four-stage chain (`GraphEvaluation.cpp`), closed-form gravity/drag integration and linear age curves (`ParticleSimulation.cpp`), per-particle RGB in the rasterizer, and AE controls 17-24 for authoring. Boundary behavior (birth, death, negative time, subframes) is pinned by `tests/core_tests.cpp`; AE playback and the panel's edit path still need the AE 2023 host pass |
| M3-03 | Native AE parameter grouping, animation sampling, and UI organization for the remaining top-level controls | M2-02, Wave G | AE adapter | Animated parameters are checked out at requested render time, not read from UI globals |
| M4-03 | Versioned preset import/export over the graph | G-02 | Core codec and AE commands | Round-trip and malformed-input behavior are specified; no implicit filesystem writes |

## Wave D — feature parity by observable family

Use `docs/reference-inventory.md` to choose one independently reviewable family per task: common forces/manipulators; texture/layer sources; mesh/material/light rendering; volumetrics; master switches and presets. Before implementation, add an observable reference case to `compatibility-matrix.md`. Static RTTI names alone are not acceptance criteria. The owner has selected a dockable node editor over a versioned protocol; P-01/P-02 remain after graph persistence and evaluation are stable.

## Wave E — qualification and shipping

| ID | Work | Dependencies | Gate |
|---|---|---|---|
| M6-01 | Thread-safety audit and AE MFR qualification | Stable serial renderer, graph snapshots, cache ownership | Only then advertise threaded rendering; serial/MFR output agrees across repeated and out-of-order frames |
| M7-01 | Profile and choose any AE GPU backend | Correct CPU renderer and measured bottleneck | Documented API only, CPU fallback always available, bounded CPU/GPU output differences |
| M8-01 | Installer, migration/upgrade path, diagnostics, release matrix | Feature and MFR gates accepted | Clean install/uninstall, project migration, and supported-host evidence |

## Agent task handoff template

> Implement **[task ID]** only. Read `AGENTS.md`, the listed ADRs, and this task card. Keep edits within the owned files unless integration is explicitly assigned. Preserve parameter/match/version/schema contracts. Return a short summary, changed files, actual build/host evidence, and any unresolved decision. Do not claim an untested AE host is supported.
