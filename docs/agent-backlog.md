# Agent-ready implementation backlog

This backlog is the task source for staged implementation work. Assign one task ID per branch/worktree. Tasks below have explicit file ownership to reduce conflicts; the integrating owner reviews and merges interfaces in dependency order.

## Current checkpoint

- **BUILD-23 (integration lead):** owner scope is now AE 2023 only. Script/MSBuild defaults and primary docs use the May 2023 SDK and `artifacts/plugin/2023/`; newer-host tasks are deferred. Owns build defaults and policy documentation only.
- **G-03:** emitter/output core runtime and graph/flat parity implemented in `070c33e`; the force/appearance kernels now complete the Alpha chain (closed-form gravity/drag integration, linear age curves for size/opacity/color, stage-order enforcement) and graph/flat parity is pinned by the core tests. The current core suite passes 11,889 checks and the May 2023 SDK build succeeds.
- **G-04:** `GraphParameter.cpp` implements AE arbitrary-data ownership/codec callbacks; legacy controls preserve old-project animation, Node Graph mode consumes a pre-render snapshot, and Capture Current Controls uses AE's supervised parameter-change path. `graph_from_controls` builds Emitter -> Particle -> Force -> Output, and every bound control is supervised so a Node Graph edit rewrites the canonical graph in the same user-change transaction. Native adapter tests pass 660 checks after the preview-scale, output-world, and appearance-projection regressions; the May 2023 SDK build succeeds. AE Controls save/reopen, same-layer duplicate, same-name copy/paste replacement and undo/redo passed. The H-01 host pass reopened the saved project in a new process with Control Source still in Node Graph mode, saved values, particle preview and CEP panel target intact; integrated graph-transaction undo, byte-level graph comparison, and build-1 project migration remain unqualified in AE.
- **M3-02 core + authoring surface:** gravity (17-19), linear drag (20), Color Start/End (21/22), Size End (23) and Opacity End (24) are registered AE controls whose defaults reproduce the previous look, so the visible improvement is reachable without the panel; the same controls are the panel's edit surface.
- **M3-01B implementation:** pre-release revision 12 uses direct full-resolution layer-pixel Size X/Y/Z values (0–100000, default 100 px) in AE controls, the CEP Emitter inspector, and graph keys 19–21. Box uses all axes and Sphere forms an ellipsoid; Disc keeps its dedicated Disc Size. Dimensions convert through full-resolution layer height and pixel aspect. The prior percentage semantics are intentionally dropped during development. Core/adapter regressions and AE 2023 host qualification are tracked in `docs/compatibility-matrix.md`.
- **P-02 panel / P-02A canvas:** `cep_panel/` now projects the revisioned canonical graph into a node canvas and selected-node inspector over ADR 0009 protocol v1. The earlier AE 2023.5.0 Build 52 form populated automatically after project/layer selection, a `Size` write updated the frame, and host undo restored the picture. The owner confirmed the node canvas now displays normally after reloading the CEP panel and reports no issue with the revision-7 AEX load. Integrated topology edits, layout save/reopen, duplication, undo/redo, stale-state rejection, and Node Graph synchronization remain to be qualified in AE.
- **Examples:** Spark, Snow and Floating Light are documented as exact parameter recipes in `docs/examples.md` and shipped as panel presets, so they can be accepted with or without the panel.
- **Time-dependent output flag:** `NON_PARAM_VARY` is present in PiPL/runtime because frame time changes rendered particles with constant parameters. MFR/GPU flags remain disabled.

- M0 architecture contracts and M1 SDK shell are in the source tree.
- The user confirmed the empty M1 shell loads in AE 2023. The current build-2, 24-active-parameter graph build has AE 2023 host evidence for controls, lifecycle, all bit depths, time consistency, and transparent particle output.
- **G-03/G-04 core and adapter:** the initial 6,196-core/395-adapter baseline is historical. The current core suite passes 11,889 checks and the latest adapter fake-host suite passes 676 checks. The May 2023 SDK build succeeds.
- **M2-06 AE 2023.5.0 Build 52 host pass:** candidate `D22D43BAD15C5173867907369B2EF3293A3FD601C308158665A1F3FD0AB0816B` was rebuilt, installed and loaded on 2026-09-28. A forced fresh render showed particles on the transparency grid, with no solid input pixels. The owner reports all 8/16/32-bpc render correctly and time consistency passes (exact frame-comparison procedure not recorded). Save/reopen, Ctrl+D, same-name Ctrl+C/Ctrl+V replacement, undo and redo passed. The CEP panel populated after project/layer selection without manual Refresh. That candidate was superseded: the installed build is now the H-01 split pair (`StarfieldParticle.aex` `B7362B01AC0E935D8AD596A70D61690DA4D586EEEC3E939328BA1BDC420069D5`, with the authorized development junction selecting `StarfieldCore-095219764514FFCA.dll`). The row-stride build `C0830F649A149990942B40531E25E23C0841FA6BED9C9805B9D3E233AB1ABCAB` was never loaded in AE. Other gates include Node Graph persistence, render queue, cancellation, and diagnostic isolation across multiple render contexts. See `docs/compatibility-matrix.md` and `docs/parameter-mapping.md`.
- **Product direction recorded (owner, 2026-09-27):** node-based editing is the essence of the reference product, so the graph foundation is promoted ahead of the M3 feature families; those families will be implemented as node types. See `Wave G` below and the raised `docs/roadmap.md` policy.
- **Static review carried out by the owner (2026-09-27):** the adapter geometry was rewritten to derive render-space scale from observed host worlds. M3-01 added Box/Sphere/Disc and seeded per-particle variation. AE 2023.5.0 Build 52 confirms Full/Half/Third/Quarter centre normalization, Quarter Point playback, and visually distinct Box/Sphere/Disc distributions. Exact distribution parity remains open.
- **Host feedback after manifest revision 2 (2026-09-27):** the first revision-2 build rendered nothing because point controls were read as percentages. A separate later preview-offset fix then assumed point values stayed full-resolution. The owner's 2026-09-28 AE 23.5.0 Build 52 screenshots prove that values are absolute pixels scaled by preview resolution; D-05 now restores that scale and removes the magnitude-based unit ladder.
- **Feedback rule adopted:** every unit or coordinate assumption that can move geometry silently must be covered by either a core test or a printed readout. Silent geometry shifts produce no AE error, so they must be observable by construction.
- Current Windows x64 `.aex` build succeeds against the May 2023 SDK; dual-SDK builds are historical. Local SDKs and artifacts are ignored by Git.
- Product direction: independent implementation of observed behavior. Static analysis is a feature-discovery aid, not proof of exact behavior or a source-code specification.

## Before agent work: human-owned Git checkpoint

### H-01 — Split the AE adapter from a reloadable core DLL

- **Owner:** integration lead.
- **Dependencies:** M2-05, G-03/G-04, ADR 0012. AE 2023 only.
- **Owned files:** `include/starfield/core/PluginApi.h`, `src/core/PluginApi.cpp`, `src/core/GraphConstruction.cpp`, `src/core/GraphEvaluation.cpp`, `ae_plugin/CoreLoader.*`, `ae_plugin/StarfieldCore.vcxproj`, `ae_plugin/Starfield.vcxproj`, `ae_plugin/SmartRender.cpp`, `ae_plugin/WorldBridge.*`, `ae_plugin/Diagnostics.cpp`, `ae_plugin/EffectMain.cpp`, `ae_plugin/PluginFlags.h`, `ae_plugin/BuildWindows.ps1`, `tools/Deploy-HotCore.ps1`, the legacy installer guard, `CMakeLists.txt`, `tests/core_tests.cpp`, `tests/RunCoreTests.ps1`, `tests/CoreLoaderTests.vcxproj`, `tests/core_loader_tests.cpp`, and matching architecture/build/compatibility documentation. Preserve the existing uncommitted work in shared files.
- **Scope:** use a fixed-width, versioned C ABI; load unique core DLL generations at runtime; pin the generation across pre-render/render; expose an explicit Options reload action; mix the generation into the SmartFX cache key; publish a workspace-local runtime manifest atomically. Keep parameter IDs, match name, graph schema and packed plug-in version unchanged.
- **Acceptance:** the `.aex` and DLL build independently; core-only edits do not rebuild the `.aex`; render output matches the monolithic build; failed reload retains the prior generation; an in-flight render never executes an unloaded generation; AE 2023 confirms manual reload updates Full/Quarter previews and survives save/reopen. Exact installed paths, hashes and host results go in `compatibility-matrix.md`. Host changes require ADR 0011 authorization.
- **Status:** repository build and automated gate passed; AE 2023 smoke gate passed.
  The core C ABI equals direct renderer pixels in a normal frame, rejects
  malformed input and releases its result. A loader harness confirms switching,
  failed-reload fallback, concurrent API leases, and a real render completing on
  its pinned generation before the retired DLL unloads. The full May 2023 build
  succeeds; `-CoreOnly` leaves the `.aex` SHA-256 unchanged. AE 2023.5.0 Build
  52 confirmed Full/Quarter manual hot reload, missing-DLL fallback, visual
  8/16/32-bpc particle output, transparency, and save/close/reopen. Subsequent
  owner checks covered effect copy/undo, Half/Third geometry, lower-layer
  compositing, emitter shapes, gravity and size changes on the split build. The first
  `/MD` split build crashed in AE's old app-local C++ runtime; `/MT` in both
  modules fixed that observed crash. AE `aerender` exported three Full-resolution
  8-bpc frames from both the current split pair and the earlier AE-qualified
  monolith; decoded RGBA pixels matched exactly at frames 51–53, and an
  alternate-color Core changed frame 51 as a cache control. Broader pixel parity
  and an in-flight AE render switch remain open. The 2026-09-29 loader
  parser now rejects a second manifest line; the loader harness and AE 2023.5.0
  Build 52 confirmed that a malformed manifest retains the prior Core. Current
  hashes and rollback are in `docs/compatibility-matrix.md`.

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

### M2-04 — Implement the CPU sprite rasterizer and alpha-only particle output

- **Owner:** CPU renderer agent.
- **Dependencies:** M2-01; consume M2-03's particle instance type after its interface is agreed.
- **Owned files:** new CPU renderer and pixel helper files under `src/core/` and `include/starfield/core/`; do not touch AE selectors.
- **Scope:** rasterize particles into owned output storage over transparent black; honor ROI, dimensions, row bytes, pixel format and premultiplied alpha. Use checked size arithmetic and return typed errors.
- **Acceptance:** bounds/ROI are respected, padding bytes are not assumed absent, alpha is correct, and allocation/unsupported-format failures return without leaking buffers.
- **Status:** implemented in `CpuRenderer.hpp/.cpp` with alpha-only semantics recorded in ADR 0005. Sprites carry a one-pixel analytic coverage ramp; ROI, row bytes, transparent pixels, premultiplied source-over accumulation, requested straight/premultiplied output encoding, 8/16/32-bpc, bounded work, and cancellation during output encoding are covered by core tests. An empty ROI is a legal, allocation-free request. The public render request has no source pixel field.

### M2-05 — Add the SmartFX AE transport adapter

- **Owner:** AE rendering agent.
- **Dependencies:** M2-01 and M2-02; agree the renderer call contract with M2-03/M2-04 before integration.
- **Owned files:** AE adapter implementation under `ae_plugin/`; coordinate shared `EffectMain.cpp` edits with M2-02.
- **Scope:** replace legacy `PF_Cmd_RENDER` with the documented SmartFX pre-render/render path; use pre-render input metadata for geometry, calculate bounded ROI, copy particle output while host pointers are valid, and release every checkout on every exit path.
- **Do not do:** set MFR, float-color, or GPU flags as placeholders; keep a CPU fallback.
- **Acceptance:** host resources have one cleanup path, errors/cancellation map predictably, and PiPL/runtime flags agree with implemented selectors.
- **Status:** implemented in `ae_plugin/SmartRender.*`, `Parameters.*`, `EffectMain.cpp`, `PluginFlags.h`, and `StarfieldPiPL.r`. Pre-render requests empty input metadata for layer bounds/reference geometry and declares `result_rect = request ∩ layer` with request-independent `max_result_rect`. Smart Render performs the matching empty pixel checkout required before `checkout_output`, but never reads it; it generates particles over transparent black, clears the AE output world, and copies the core buffer. The clear is required because AE may seed output pixels from the source layer. `PF_OutFlag2_REVEALS_ZERO_ALPHA` is included in PiPL and runtime flags. Parameter checkouts use RAII; cancellation is polled during simulation, rasterization/encoding, and output-world writes; core errors map to stable AE errors. May 2023 SDK build and AE 2023 alpha-output visual check pass.
- **Deliberate M2 limits (documented, not hidden):** the adapter does not narrow `result_rect` to analytic particle bounds, so it always fills the requested region; content-bounds narrowing is deferred to a profiling task.
- **Geometry revision after static review:** the adapter no longer derives the render grid from `in_data->downsample_x/y` (the SDK documents that factor inconsistently: the header describes a divisor in the range 1–999+, while `Resizer.cpp` and `PathMaster.cpp` multiply by it as a scale). Render geometry now comes from observed checked-out worlds plus `max_result_rect`, `ref_width/ref_height`, and `par`, carried from pre-render to render in `pre_render_data`; a missing state block fails loudly. `tests/core_tests.cpp` pins the half-resolution mapping. Preview-resolution behavior still needs host confirmation.

### M2-06 — Integrate and qualify the first particle render

- **Owner:** integration lead.
- **Dependencies:** M2-02 through M2-05.
- **Owned files:** integration fixes across adapter/core plus compatibility evidence.
- **Scope:** produce the first visible point emitter over a defined frame, confirm transparent particle alpha, and capture repeatable outputs at the supported bit depths. Qualify AE 2023 only; newer-host adaptation is deferred by owner direction.
- **Acceptance:** AE effect controls affect deterministic rendered pixels; repeated/reverse-time requests match; project save/reopen preserves controls; all claimed host evidence is recorded.
- **Status:** AE 2023.5.0 Build 52 host checks confirm the visible offset is fixed, save/reopen and effect lifecycle work, all bit depths render normally, and the owner-reported time-consistency check passes. The rebuilt candidate renders particles over transparency; the adapter clears the output world before copying sparse particle pixels. The CEP parameter form auto-populates after project and effect-layer selection without clicking Refresh. Remaining host gates include Node Graph persistence, render-queue, cancellation, and diagnostic isolation across multiple render contexts.

## Diagnostics and shipped defaults (current triage)

| ID | Owner | Work | Done when |
|---|---|---|---|
| D-01 | AE adapter agent | Options-button readout of what the effect actually receives: frame time, live particle count, layer/frame grid, downsample, pixel aspect, every parameter as read, and the converted velocity | Implemented in `ae_plugin/Diagnostics.cpp` (read-only; not part of the render contract). A support tool, kept until M8 diagnostics supersede it |
| D-02 | Owner + lead | Decide the shipped default look. With `velocity = (0,0,0)` and a 2 s lifetime a fresh instance shows one overlapping cluster at the layer center, and at comp time 0 exactly one particle exists by construction, so the effect can look static even though it works | **Done (owner delegated):** the manifest default is Velocity `(0, 0.3, 0)` layer heights/s plus `velocity_spread = 0.15`, mirrored by `Settings`, `schema/parameters.json`, and `docs/parameter-mapping.md`. Defaults affect new instances only |
| D-03 | Lead | Split Velocity into scalars and keep point controls for positions only | **Done:** manifest revision 2 (three velocity sliders in layer heights/s plus a 3D-point Emitter Origin). Recorded in `docs/parameter-mapping.md`, `schema/parameters.json`, and `Settings.hpp` |
| D-04 | Build owner | Make a PiPL/runtime flag mismatch impossible to ship: declare the flags/version headers as `AdditionalInputs` for the PiPL custom build and verify the generated resource against them | **Done:** `Starfield.vcxproj` declares `AdditionalInputs` (`Outputs` stays, or MSBuild skips the step with MSB8018) and `BuildPiPL.ps1` fails the build when `StarfieldPiPL.rc` disagrees with `PluginFlags.h`/`PluginVersion.h`. This was the cause of the AE "global outflags mismatch" report |
| D-05 | AE adapter agent | Treat point controls as host-pixel positions, including preview scaling | **Measured, fixed, and host-qualified at Full/Quarter in AE 2023.5.0 Build 52:** Full delivered `1920,1080,1080`; Quarter delivered `480,270,270` with `ds 1/4`, `ref 3840x2160`, `grid 960x540`. Both normalize to the same center position; the adapter reverses the rational factors and the core no longer guesses percentages/fixed point from magnitude. Full/Quarter/anisotropic, Capture, and graph-sync regressions are added. Half and Third are now host-measured as well (`ds 1/2` grid `1920x1080`; `ds 1/3` grid `1280x720`; both normalize to the same centre), recorded in the 2026-09-28 acceptance block of `docs/compatibility-matrix.md`. |
| D-06 | AE adapter agent | Keep the emitter-shape readout visible: the closing `shape/esz/vspr/size/not` line is dropped from every Options report because the 255-character `return_msg` budget is spent by the lines printed before it, so the only readout of the emitter shape never appears | **Recorded 2026-09-28, deliberately deferred by the owner.** Fix = compact the writer (fold `shape` into the `cnt` line, or shorten the `layer`/`org` lines), which means a full AEX rebuild and reinstall, changing the recorded installed hash and needing ADR 0011 authorization. Geometry, force and count lines are unaffected. |

## Wave G — graph foundation (promoted ahead of Wave C by owner direction)

The owner confirmed that node-based editing is the product's essence. Wave A/B built the render boundary that the graph will feed; building M3's emitter/force families as flat parameters now would mean implementing each of them twice. Wave G therefore lands first, and M3 families become node types afterwards. M4-01/M4-02 are absorbed here.

| ID | Owner | Work | Dependencies | Done when |
|---|---|---|---|---|
| G-01 | Core graph agent | Typed node/port/edge/parameter model with stable identity domains and a validator (unknown node type, type mismatch, missing input, cycle, duplicate id) | ADR 0001 identity rules, ADR 0006 | **Implemented:** `Graph.hpp/.cpp` defines UUID node/edge IDs, typed port and parameter keys/values, bounded validation, emitter/output schemas and typed failures. Covered by reorder-stability, cardinality, cycle and feedback-boundary checks. G-02 adds core serialization; G-03 evaluates emitter/output graphs |
| G-02 | Core serialization agent | Bounded core serializer/parser with magic, schema version, counts, CRC, and safe unknown-version handling for `schema/sequence-format.md` | G-01 | **Core codec implemented:** `SequenceCodec.hpp/.cpp` validates before writing, emits canonical order, and bounds parsing before allocation. Regression cases cover all seven value kinds, round-trip, CRC, optional records, and malformed headers/records. No earlier graph schema exists to migrate. G-04 consumes these bytes in AE arbitrary parameter callbacks (ADR 0008) |
| G-03 | Core evaluation agent | Graph evaluation into the existing `RenderRequest`: dependency order, evaluation at rational time, cancellation, and emitter/output pixel parity | G-01, M2-01/03/04 | **Emitter/output runtime implemented:** `GraphEvaluation.hpp/.cpp`, ADR 0007. Stable dependency traversal, active-output reachability, semantic bounds, graph precedence and per-particle opacity, plus the M3-02 force/appearance stages. The current core suite passes 11,889 checks. Current schema stores constant parameters; animated sampling/history requires M3-03's contract |
| G-04 | AE adapter agent | Persist graph as AE arbitrary parameter data; retain animated legacy-control mode and provide explicit capture into node mode | G-02, G-03 | **Code complete; AE Controls host pass, integrated Node Graph gate open:** graph data, Control Source and Capture are registered; AE owns graph bytes and lifecycle callbacks; old projects select animated AE Controls via `USE_VALUE_FOR_OLD_PROJECTS`; capture writes the current-time graph and selects Node Graph in one supervised parameter transaction. SmartFX consumes a pre-render snapshot. AE 2023.5 Build 52 preview-scale point conversion is shared by render, capture, and graph synchronization; native adapter tests pass 660 checks. AE Controls save/reopen, copy and undo/redo passed on the build-2 candidate; the H-01 pass reopened the project in a new process with Control Source still in Node Graph mode and saved values intact. Integrated topology/curve transactions, byte-level graph comparison and build-1 project migration remain open in AE |
| G-05 | Core graph agent | Extend graph evaluation for Particle nodes, emitter fan-out, force chains and parallel force branches; preserve deterministic output and bounded work | G-01–G-04, ADRs 0006–0008, 0015, M3-01/M3-02 | **Portable core plus focused regression coverage pass; AE persistence/undo gates remain open:** `org.starfieldfx.nodes.particle` carries per-branch age curves; active Particle nodes are UUID-ordered and receive global emission slots by `slot % branch_count`, preserving total rate/cap and slot IDs. Force nodes support fan-in; each Particle branch accumulates reachable Force nodes once in stable dependency order. Active Force/Appearance bypass paths in explicit Particle mode are rejected, not silently dropped. Particle/Appearance precedence is resolved once per particle. Evaluation allocates one global-ID-ordered output buffer and writes each modulo branch directly into it without per-branch particle vectors or a final sort. The uniform gravity/drag kernel sums fields and integrates once. Core suite: 11,889 checks pass. Multiple active emitters and stochastic allocation remain deferred. An output ancestry with Emitter but no Particle is transparent; active Force/Appearance without a Particle source fails. Emitter/Particle schema-1 snapshots are unsupported during development. A traversal work cap is pinned in ADR 0015. P-02B projects dynamic graph topology into CEP and routes add, connect/reconnect, disconnect, splice, delete, duplicate, move, and typed parameter changes through revision-checked transactions. AE carrier, persistence, undo, and render behavior remain unqualified. |
| P-01 | Owner + lead | Choose the AE 2023 dockable-panel bridge and specify its versioned protocol and failure behavior | Wave G contracts | **Architecture accepted in ADR 0009:** CEP `CSInterface.evalScript` → ExtendScript → supervised script-visible AE parameter streams → effect updates canonical arb graph in `PF_Cmd_USER_CHANGED_PARAM`. No direct arb-stream writes, sockets, or render-time AEGP queries. AE 2023 host gate remains open for scripted hidden-stream access and undo behavior. |
| P-02 | Panel agent | Build the dockable effect-control panel over ADR 0009, with grouped parameter editing through supervised AE streams | P-01; force/appearance parameter bindings | **Bridge and automatic discovery implemented; partial AE qualification:** source is installed through the authorized Junction and the earlier form auto-populated after project/layer selection without Refresh. Parameter reads/writes, undo grouping, stale-state rejection, and Node Graph synchronization remain to be qualified. P-02A supplies the new fixed topology view. |
| P-02A | Panel agent | Add a visible dockable node canvas for Emitter → Particle → Force → Output; show compact node cards, top/bottom ports, connectors, and a selected-node inspector. Permit node dragging and refresh selected target state without a manual click | P-02, ADR 0009 protocol v1, ADR 0014 | **UI visibility confirmed; layout lifecycle qualification open:** `cep_panel/` implements the compact top-down node canvas, type colors, target pin, viewport-wide marquee/group movement, cursor-anchored wheel zoom, middle-button pan, signed coordinates, minimap, and movable inspector. The owner confirmed the canvas displays after reloading the CEP panel. The initial fixed four-stage view and eight AE layout streams remain for compatibility; P-02B now projects the canonical graph and persists UUID-keyed positions in the graph record. Dynamic graph edit and layout undo/save/reopen behavior still require the P-02B AE 2023 pass. |
| P-02B | Graph/panel | Define graph-backed CEP payload and bounded transactions for topology editing, including stable node/port/edge identity, graph validation, persistence, undo, and protocol migration | P-02A, G-01–G-05, ADRs 0006–0009, 0013, 0015 | **Source integration implemented; AE qualification remains open:** `graph_view.js` projects the revisioned canonical graph into UUID-based nodes/edges and inspector parameters. The panel routes add, connect/reconnect, disconnect, insert, delete, duplicate, move, and typed scalar/color/popup edits through the bounded 24 KiB revision-checked transaction. The expression snapshot initializes on first use and stays in the AE project; node positions update the graph's optional layout record. The custom menu no longer offers a duplicate Output node. The owner’s AE 23.5x52 probes confirmed direct ExtendScript `CUSTOM_VALUE` access is unavailable; do not repeat those probes. The ADR 0013 expression carrier and supervised callback are source-complete, but have not been exercised with the integrated panel in AE. Validate target pinning, valid/invalid edits, stale-write rejection, one-step undo, save/reopen, effect duplication, and render parity before treating topology editing as host-qualified. Preserve direct Emitter-to-Particle links, Particle fan-out semantics, ordered force evaluation, and force fan-in behavior from ADR 0015.
| P-02C | Panel/core/adapter | Add editable piecewise-linear Size and Opacity over-life curves to the Particle inspector, persist them in the AE project, and evaluate them in Particle/Appearance nodes | P-02A, G-03–G-05, M3-02, ADR 0016 | **Curve editor and graph persistence source implemented; percentage contract being integrated; AE host gates open:** both fixed 0–100% curves multiply the Particle base Size and Opacity. The CEP plot adds, drags, removes, and selects up to eight knots. Life percentage and value support numeric entry and horizontal scrubbing. In AE Controls mode the hidden project curve bank commits with the final-percentage endpoint and a supervised nonce; the gateway validates both banks before opening an undo group. In Node Graph mode the optional version-1 opaque curve payload and final percentage commit in one revision-checked graph transaction. Missing payloads mean a straight curve from 100% to the endpoint. Base Size/Opacity edits do not rewrite curve points. AE parameter binding, graph carrier undo, save/reopen, and rendered response need the owner’s AE 2023 pass. |
| P-03 | Panel agent | On-screen emitter gizmo using `PF_EffectCustomUISuite2` + overlay theme (Drawbot only; no graph editing in-effect) | P-02, ADR 0005 geometry | Dragging the gizmo changes the emitter position and matches the rendered trail |

P-02A display follow-up (2026-09-30): center node text; show Max Particles on the
Output inspector while preserving the existing AE parameter identity; replace the
Output placeholder caption with a live/max count sampled from the current comp time.
P-02B now projects the committed canonical graph into the canvas. The Output count
reads the unique emitter in the active output ancestry, including Node Graph mode;
parked emitters are ignored and multiple active emitters fail closed. Focused graph
view and gateway tests cover the count calculation and host-time response. AE carrier
and dynamic-layout host passes remain open.

P-02B integration update (2026-09-30): `graph_layout.js` stores a complete UUID-keyed
position map in the graph's optional layout record. The edit planner persists layout
through add, duplicate, delete, splice, and move; a wire splice reuses the dragged node.
The panel now loads the project graph and sends topology, layout, and parameter changes
through the revision-checked carrier. Native validation accepts incomplete/disconnected
topology so a wire disconnect can persist as one edit; when Output has no active emitter,
the renderer returns transparent output, and reconnecting restores deterministic particles.
Core regressions cover disconnecting each link in the default emitter/particle/force/output
chain, codec round-trip, transparent rendering, and reconnect parity. Source tests cover the
codec, projection, planner, transactions, and startup. AE callback, undo, save/reopen, stale
rejection, and render parity gates remain open; do not describe the integration as AE-qualified.

P-02B splice follow-up (2026-09-30): the canvas now passes the node's pointer-drop
position into the splice transaction and includes other moved selection positions in
that same edit. A focused CEP edit test checks the exact node drop coordinate, grouped
layout, and codec round-trip. The AE carrier and gesture still need host qualification.

## Wave C — MVP controls and behavior families (after Wave G)

| ID | Work | Dependencies | Main ownership | Gate |
|---|---|---|---|---|
| M3-01 | Box/sphere/disc emitter distributions and documented coordinate mapping, implemented as node parameters | Wave G, M2-03/04 | Core simulation | **Core implementation complete:** `core::Random` supplies per-particle streams keyed by (seed, id, purpose); emitter shape, extent, velocity, and spread flow through the emitter graph node. Each shape is bounded and reproducible, and `seed` changes pixels. AE playback and preview-resolution confirmation remain open |
| M3-01B | Add reusable per-axis emitter size controls for Box and Sphere | M3-01, G-03/G-04, ADRs 0003/0006/0007 | Core simulation, AE adapter, CEP authoring | **Pre-release semantics revised by owner:** Size X/Y/Z at AE indices 81–83 and graph keys 19–21 are direct full-resolution layer pixels, bounded 0–100000 with a 100 px default. Box uses all axes; Sphere forms an ellipsoid. The common size parameter is now Disc Size, and Point/Disc ignore axis dimensions. `EmitterDimensionContext` converts X through pixel aspect and every axis through layer height. Regression/build gates and AE visual/save/undo qualification are recorded in `docs/compatibility-matrix.md`. Owned files: `Settings.hpp/.cpp`, `Graph.hpp/.cpp`, `GraphConstruction.cpp`, `GraphEvaluation.cpp`, `ParticleSimulation.hpp/.cpp`, `CpuRenderer.cpp`, `ae_plugin/Parameters.hpp/.cpp`, `schema/parameters.json`, `cep_panel/js/graph_view.js`, `cep_panel/js/panel.js`, `cep_panel/jsx/starfield_gateway.jsx`, `tests/core_tests.cpp`, `tests/graph_parameter_tests.cpp`, and matching ADR/parameter/compatibility/build documentation. |
| M3-02 | Particle age curves, size/opacity over life, gravity and drag as node types | Wave G | Core simulation/graph | **Core implemented; host confirmation pending:** force and appearance node types plus the four-stage chain (`GraphEvaluation.cpp`), closed-form gravity/drag integration and linear age curves (`ParticleSimulation.cpp`), per-particle RGB in the rasterizer, and AE controls 17-24 for authoring. Boundary behavior (birth, death, negative time, subframes) is pinned by `tests/core_tests.cpp`; AE playback and the panel's edit path still need the AE 2023 host pass |
| M3-05 | Deterministic Particle Size Random and Opacity Random controls | M3-01, M3-02, G-03–G-05, ADR 0018 | Core simulation, graph, AE adapter, CEP inspector | Add 0–100% stable per-particle attenuation after each age curve; append AE revision-11 controls and optional Particle/Appearance keys without changing prior project output. Focused core, graph, adapter, and panel checks pass; May 2023 SDK build succeeds; AE 2023 visual/save/undo qualification remains open. Owned files: `include/starfield/core/Settings.hpp`, `src/core/Settings.cpp`, `src/core/ParticleSimulation.cpp`, `src/core/Graph.cpp`, `src/core/GraphConstruction.cpp`, `src/core/GraphEvaluation.cpp`, `include/starfield/core/Graph.hpp`, `ae_plugin/Parameters.hpp/.cpp`, `schema/parameters.json`, `cep_panel/js/graph_view.js`, `cep_panel/jsx/starfield_gateway.jsx`, `tests/core_tests.cpp`, `tests/graph_parameter_tests.cpp`, relevant adapter tests, and matching docs. |
| M3-06 | Correct Particle lifetime ownership, curve authoring, and alpha compositing | G-05, M3-02, P-02C, ADRs 0003/0005/0015/0016 | Core graph/evaluation, CEP inspector, AE render boundary | **Source implementation, focused regressions, and the AE 2023 candidate build are complete; host qualification remains open.** Particle lifetime is owned by each Particle node. Revision 13 makes both curves fixed 0–100% multipliers of independent base Size (pixels) and Opacity (0…1); the native adapter keeps curve point zero from the project bank. The CEP gateway uses matching 0–100 bounds for both hidden curve banks. A new regression edits Size while preserving an existing Opacity curve with ordinates above 1. Core tests pass 11,905 checks, focused graph-view/edit/transaction, gateway, and startup checks pass, and the May 2023 SDK build succeeds. Candidate hashes and open AE checks are in `docs/compatibility-matrix.md`; straight-alpha rendering, curve response, undo, and save/reopen still require AE 2023 qualification. Owned files: `include/starfield/core/Graph.hpp`, `include/starfield/core/Render.hpp`, `src/core/Graph.cpp`, `src/core/GraphConstruction.cpp`, `src/core/GraphEvaluation.cpp`, `src/core/Render.cpp`, `src/core/CpuRenderer.cpp`, `ae_plugin/SmartRender.cpp`, `ae_plugin/WorldBridge.cpp`, `ae_plugin/Parameters.cpp`, `cep_panel/js/graph_view.js`, `cep_panel/js/panel.js`, `cep_panel/jsx/starfield_gateway.jsx`, `cep_panel/css/panel.css`, `tests/core_tests.cpp`, `tests/panel_gateway_tests.js`, `tests/panel_startup_tests.js`, `docs/architecture.md`, `docs/roadmap.md`, `docs/adr/0003-pixel-and-alpha-contract.md`, `docs/adr/0005-render-request-contract.md`, `docs/adr/0015-graph-particle-branches.md`, `docs/adr/0016-particle-over-life-curves.md`, `docs/parameter-mapping.md`, `docs/agent-backlog.md`, `docs/build-matrix.md`, and `docs/compatibility-matrix.md`. |
| M3-03 | Define animation/history semantics for graph values and finish remaining control organization | M2-02, Wave G | AE adapter/core contract | Legacy AE controls are checked out at requested render time and revision 6 groups the current controls. Graph values remain constants until an animation/history contract and migration are accepted. |
| M4-03 | Versioned preset import/export over the graph | G-02 | Core codec and AE commands | Round-trip and malformed-input behavior are specified; no implicit filesystem writes |

## Wave D — feature parity by observable family

Use `docs/reference-inventory.md` to choose one independently reviewable family per task: common forces/manipulators; texture/layer sources; mesh/material/light rendering; volumetrics; master switches and presets. Before implementation, add an observable reference case to `compatibility-matrix.md`. Static RTTI names alone are not acceptance criteria. P-01/P-02 protocol v1 is implemented; dynamic graph editing will need a later panel protocol after an observable topology-editing case is defined.

## Wave E — qualification and shipping

| ID | Work | Dependencies | Gate |
|---|---|---|---|
| M6-01 | Thread-safety audit and AE MFR qualification | Stable serial renderer, graph snapshots, cache ownership | Only then advertise threaded rendering; serial/MFR output agrees across repeated and out-of-order frames |
| M7-01 | Profile and choose any AE GPU backend | Correct CPU renderer and measured bottleneck | Documented API only, CPU fallback always available, bounded CPU/GPU output differences |
| M8-01 | Installer, migration/upgrade path, diagnostics, release matrix | Feature and MFR gates accepted | Clean install/uninstall, project migration, and supported-host evidence |

## Agent task handoff template

> Implement **[task ID]** only. Read `AGENTS.md`, the listed ADRs, and this task card. Keep edits within the owned files unless integration is explicitly assigned. Preserve parameter/match/version/schema contracts. Return a short summary, changed files, actual build/host evidence, and any unresolved decision. Do not claim an untested AE host is supported.
