# Agent-ready implementation backlog

## M3-03 — emitter Origin birth history / build 21

Owner confirms build 20 renders but Origin keys translate every live particle.
Owns GraphEvaluation.hpp/.cpp, new core EmitterHistory.hpp/.cpp, native origin
binding accessors and new adapter EmitterHistory capture, SmartRender.cpp,
Starfield/Core MSBuild and CMake source lists, build fingerprint/targeted runner,
current/native scoped checks, PluginVersion, PluginFlags/EffectMain for paired
historical dependency declarations, and ADR 0024/architecture/roadmap/build/
compatibility/checkpoint records. Render.hpp and C ABI 2 remain unchanged.
Sample native Origin XY/Z at particle birth times through owned PF checkouts in
pre-render; encode a transient immutable history record for the DLL. Preserve
determinism, Auxiliary parent birth positions, force-relative motion, cancellation
and bounded memory. No project migration or process-global simulation state.
Animated rates/lifetime/forces and other birth controls require separate semantics.
AE visual interpolation/reopen remain owner qualification.

Build 21 source/SDK candidate complete: 921 focused checks pass, covering actual
historical PF checkouts, frozen graph/CPU pixels, Auxiliary birth origins and
failure/cancellation behavior. All five AEXs plus Core build; owner AE gate open.

## P-02I — native Effect Controls commit / build 17

Build 17 owner evidence: build 16 still fails at delivery/parameter 4/stream -1/516.
Replace the cross-effect generic transport with direct node-owned graph compilation
and AEGP publication, with explicit caller registration ID. Preserve independent
node records, immutable saved render graph and existing schema/ID/ABI contracts.
Extend this card's owned files to NativeGraphCommit.*, NodeEffect.vcxproj,
Starfield.vcxproj, BuildWindows.ps1, Parameters.cpp and the targeted fixture runner /
compiler stub, as required to compile the shared publisher inside each node AEX.
Native sync 348/camera 12 checks pass with a generic API that rejects every call
and absent main registration. Renderer controls 38 scoped checks and final May 2023
SDK x64 Release /MT build pass at packed 32785 / 0x8011, no compiler warnings/errors
found. All four independent node AEX links pass. Owner AE acceptance remains open.

Source b0e9e6b pushed before build-17 AE-absent installation. Existing single
Junction, unchanged Core, all six installed hashes and build-16 backup hashes /
selector verified. Before/after records and rollback are in native-node-checkpoint.md.
No AE session or other host setting changed; real native editing gate remains open.

Build 16 follow-up: owner reports all tried native edits fail with error 516. Build 15 host
acceptance failed. Remove assumptions that a generic callback supplies a complete
params array/input image/count and renderer geometry. Read only required main
streams via AEGP; use synchronous borrowed renderer/layer refs and numeric geometry
from the node callback. Add phase/index diagnostics and fixtures with null generic
params/effect_ref/utils/pica and num_params=0. Split scalar/graph publish and verify
phases, validate stream types and keep exact checks only for integer receipts.
Keep the same owned files and schemas. Native sync 330/camera 12 scoped checks pass;
float storage and maximum revision are covered. Renderer controls 38 scoped checks
and final May 2023 SDK x64 Release /MT build pass, packed 32784 / 0x8010, no compiler
warnings/errors found. Owner AE qualification remains open.

Source 76801bb pushed before build-16 AE-absent installation through the existing
single Junction. Installed candidate hashes, unchanged Core and build-15 backup
bundle/selector verified. Before/after records and one-step rollback recorded in
native-node-checkpoint.md. No AE process or other host setting changed.

Owner reports native controls only reach rendering after a subsequent CEP edit.
Owned files: NodeGraphSync.*, NativeNodeGraph.*, GraphCarrier.*, EffectMain.cpp,
PluginVersion.h, targeted native sync fixtures/runner and checkpoint/ADR docs.
Dependencies: P-02G native records and P-02H layout 6. Keep schemas, parameter
IDs, Core ABI and separate node effects unchanged.

Replace the synthetic USER_CHANGED_PARAM cross-effect call with an acknowledged
COMPLETELY_GENERAL request. Carry the callback's edited value by UUID so saved
sibling streams cannot substitute an old value. Explicitly persist the renderer
graph/revision/checksum through AEGP streams, verify readback and rollback failed
publishes. Test a host that discards generic params-array writes and retains old
node stream values until the native callback returns. AE acceptance remains open.

Native sync 252/camera 12 and renderer controls 38 scoped checks pass. May 2023
SDK x64 Release /MT candidate build passes at packed 32783; schemas/IDs/Core ABI
unchanged. Source publication and AE-absent installation follow the checks.

Source 96ecf6d pushed before build-15 installation. Existing single Junction
retained; six installed hashes, selected unchanged Core and build-14 backups
verified. Standing AE-absent authorization used; no AE session operated. Native
Effect Controls behavior, undo/redo and reopen await owner qualification.

## Current P-02H candidate — 2026-10-02

Build 14 adds ordinary Force controls/curve, renderer Time Remapping/Preview and
fresh Max Particles 1000000, with bounded initial Auxiliary storage. SDK/editor
includes no longer name the nonexistent Headers/Win directory. Minimal requested
checks pass: Core 58, native Force 79, renderer setup/checkout 38; current-node CEP
suite and six-source parsing pass. May 2023 SDK candidate builds at packed 32782,
main 21/native layout 6/Force 2/Output 3/Core ABI 2. No AE session operated.
Source push precedes installation; deployment evidence goes in the checkpoint.

Source 6a32f94 was pushed before deployment. Build 14 is now installed under the
standing AE-absent permission; six installed/selected Core hashes and build-13
backups match. One existing Junction retained; no AE session operated. Host
acceptance and the reference main-effect capability matrix remain open.

## Current P-02G checkpoint — 2026-10-02

Build 13 candidate fixes native runtime edit dispatch, adds UUID effect selection,
camera projection and unified Auxiliary sources. Minimum owner-authorized tests
and the May 2023 SDK build pass; six modified JavaScript sources parse. Core ABI 2,
Emitter schema 5/native layout 5 require one paired deployment and fresh effects.
Source 2751a56 was pushed, then build 13 deployed with AE absent. Six installed
hashes/selected Core and build-12 backups verified; one Junction retained.
Source/publish/deployment evidence is tracked in native-node-checkpoint.md;
all new host behavior remains owner qualification work. No AE session operated.

## Previous P-02F checkpoint — 2026-10-02

Build 12 source implements shared Particle inputs, stable independent emitter
births, corrected Life range/steps, reference control naming/splits/units,
independent base and percentage curves, multi-emitter counts and preserved
copy connections. Sprite rasterization skips zero alpha and uses cancellable
exact rendering without a default coverage budget. May 2023 SDK and four
JavaScript parse gates pass. No test suites added/run or AE session operated.
Main 20/native layout 4/Emitter schema 4; no development migration. Source
pushed as cadab2a, then build 12 deployed with AE absent under standing permission.
Six installed/selected Core hashes verified; build 11 backed up; one Junction
retained. Owner acceptance remains open.
[Checkpoint](native-node-checkpoint.md).

## Previous P-02E checkpoint — 2026-10-02

Owner reports canvas deletion unavailable and redundant collapsed outer topics.
Build 11 adds Delete/Backspace, filters fixed Output out of delete/duplicate
selection, and pauses polling while the context menu is open. Actual outer
topic markers are removed from all native effects; main/native indices and
gateway bindings are compacted together. Main schema 19, native layout 3,
gateway native-node-sync-11; surviving disk IDs unchanged, no development
migration. SDK build and CEP syntax checks pass; no tests added/rerun. Source
pushed as cb53d50, then deployed with AE absent under standing authorization.
Six installed hashes verified; Core/generation unchanged, one Junction retained,
build 10 backed up. Owner AE acceptance remains open.
[Checkpoint](native-node-checkpoint.md).

## Previous G-06 checkpoint — 2026-10-02

Owner describes build 9 as basically working but reports cancellation dialogs,
multiple-active-emitter rejection and ineffective wire-click disconnect. Build 10
implements quiet interrupts, bounded multi-emitter population merge and wire/port
interaction fixes. SDK build and both CEP syntax checks pass; no test suites
added/rerun. Source pushed as c368bbf, then build 10 deployed with AE absent under
standing authorization; six hashes and selected runtime Core verified. Precise
owner AE acceptance remains pending.

## Previous P-02D checkpoint — 2026-10-01

Independent node records and transactions are source-integrated. Owner evidence:
build 3 renders but crashes on layer selection. Build 4 removes unused hidden
structural groups and passes 721 adapter checks and the May 2023 SDK build.
One-folder deployment/rollback checks pass; build 4 is deployed after authorized AE closure.
The previous eight CEP suite results remain valid for unchanged panel code.
Selection safety and native-node host acceptance remain open.
See [native-node-checkpoint.md](native-node-checkpoint.md).

This backlog is the task source for staged implementation work. Assign one task ID per branch/worktree. Tasks below have explicit file ownership to reduce conflicts; the integrating owner reviews and merges interfaces in dependency order.

## Current checkpoint

### P-02H — Reference Force, renderer globals and population defaults (2026-10-02)

- **Owner:** integration lead; owner follow-up to P-02G.
- **Dependencies:** P-02G, ADRs 0002, 0005, 0012, 0015, 0019, 0021.
- **Owned files:** current core settings/simulation/graph/renderer, native Force records and parameter setup, main renderer controls, CEP gateway/view/edit/curve surface, parameter manifests, SDK include configurations, focused current-node/Force checks and corresponding architecture, mapping, build and deployment documentation.
- **Scope:** remove the nonexistent May 2023 SDK Headers/Win include; set fresh Output's cap to 1000000 without allocating that cap for small populations. Align ordinary Force with observed Gravity/random, Wind, Spin/frequency/resist/delay, Air Density and a Wind/Spin percentage curve. Add functional Time Remapping and Preview globals on the renderer effect. Retain separately saved node effects. Record every remaining reference main-effect capability explicitly rather than registering inactive controls.
- **Acceptance:** owner-authorized minimal current-node/native Force/panel checks and May 2023 SDK candidate build; sources pushed before AE-absent deployment through the existing single Junction. Fresh effects for the development layout; exact reference motion and real AE acceptance require owner verification.

### P-02G — Native edits, effect selection, camera and auxiliary emission (2026-10-02)

- **Owner:** integration lead; current task after P-02F.
- **Dependencies:** P-02F; ADRs 0003, 0005, 0009, 0012, 0015, 0019, 0020.
- **Owned files:** core graph, simulation, renderer and render contract (`Render.hpp`), C ABI `PluginApi.h/.cpp`; adapter native synchronization, camera capture, SmartRender, flags/version/build metadata; CEP gateway, graph model/edits/view, panel/styles; native manifests, scoped current-node/native-camera/Emitter/Particle/panel checks, project C++ editor configuration, scoped compiler include tracing and corresponding architecture, ADR, mapping, build, checkpoint and compatibility documentation.
- **Scope:** fix native Effect Controls edits using runtime indices instead of the setup-only `uu.id` union; select the UUID-owned AE effect on node click; improve current controls and avoid redundant stream writes. Add immutable camera projection across the render boundary. Emitter supports an Auxiliary mode with incoming particle sources, probability, normalized parent-life interval and inheritance. Preserve independent node effects and project storage. Development schema/ABI changes require the explicit ADR 0020 plan, without old-project migration.
- **Acceptance:** May 2023 SDK candidate builds and modified JavaScript parses; owner verifies native Origin XY/Z changes, effect selection, camera movement/zoom and auxiliary births, undo/reopen and current-node interaction. No tests or AE sessions run without request. Prefer one paired deployment after AE closes, retaining the single Junction and verified rollback.
- **Evidence:** owner goal explicitly permits necessary minimal tests. Core current-node scope passes 28 checks; native edit callback 14, camera capture 12, actual Emitter/Particle selector scopes 77/73; three focused CEP suites pass. Build 13 candidate uses packed 32781, native layout 5/Emitter schema 5/Core ABI 2 and main manifest 20. No AE session operated. Native Origin/highlight/camera/Auxiliary host acceptance, non-square PAR, advanced reference source controls and Auxiliary live counter remain open. See ADR 0020 and native-node-checkpoint.
- **Editor follow-up:** owner reports 14 C/C++(135) missing camera members. Current ABI 2 defines every reported member; actual NativeSync -TraceIncludes compile passes 26 checks and proves current-header inclusion. Add project-scoped C++20/MSVC/Core/SDK configuration, clean workspace entry and ignored editor cache paths. Stale editor indexing remains a hypothesis pending owner reload; installed runtime unchanged. See editor-setup.md.

### P-02F — Shared Particle inputs, parameter controls and curve rendering (2026-10-02)

- **Owner:** integration lead; current task after P-02E.
- **Dependencies:** G-06, P-02E, ADRs 0005, 0009, 0015, 0016, 0019.
- **Owned files:** core `Graph.hpp`, `Graph.cpp`, `GraphConstruction.cpp`, `GraphEvaluation.cpp`, `Settings.hpp`, `CpuRenderer.hpp/.cpp`; adapter `NodeEffects.cpp`, `NodeRecord.hpp`, `NodeGraphSync.cpp`, `NativeNodeGraph.cpp`, `Parameters.cpp`, `PluginVersion.h`; CEP `graph_edits.js`, `graph_view.js`, `panel.js`, `starfield_gateway.jsx`, README; main/native parameter manifests and corresponding ADR, architecture, roadmap, mapping, build, compatibility and deployment docs. `Render.hpp` and the C ABI remain unchanged.
- **Scope:** Particle accepts multiple direct Emitters; each emitter keeps its own UUID/birth allocation while sharing Particle lifetime/appearance. Audit visible scalar ranges and explicit CEP scrub steps; Life defaults to 2, caps at 10000 seconds and scrubs by 0.1. Align supported labels/units with the observed reference table and split native Origin XY/Z. Remove accidental base-value writes to percentage curve knots. Skip transparent sprites and use cancellable exact rasterization without the default arbitrary per-frame coverage cutoff; retain opt-in finite work limits.
- **Acceptance:** May 2023 SDK candidate builds; changed JavaScript parses. Owner tests shared Particle inputs, controls/curve independence, direct native edits, rendering, undo and reopen. No tests added/run without request. Native layout 4/main manifest 20 and packed build 12 have no development migration. AE-closed deployment follows standing authorization; no process start/stop.

### P-02E — Canvas deletion and flat Effect Controls (2026-10-02)

- **Owner:** integration lead; active task after G-06 integration.
- **Dependencies:** P-02D, G-06, ADRs 0009, 0011, 0019.
- **Owned files:** `cep_panel/js/panel.js`, `cep_panel/jsx/starfield_gateway.jsx`, `cep_panel/index.html`, `cep_panel/README.md`, `ae_plugin/NodeEffects.cpp`, `ae_plugin/NodeRecord.hpp`, `ae_plugin/NodeGraphSync.hpp`, `ae_plugin/NativeNodeGraph.cpp`, `ae_plugin/Parameters.hpp/.cpp`, `ae_plugin/GraphParameter.hpp/.cpp`, `ae_plugin/PluginVersion.h`, `schema/parameters.json`, `schema/node-parameters.json`, and matching ADR, architecture, roadmap, parameter mapping, checkpoint, build and compatibility documentation.
- **Scope:** owner reports canvas node deletion unavailable and redundant collapsed top-level Output/Emitter/Particle/Force parameter topics. Add Delete/Backspace and share editable-selection filtering with the context menu; fixed Output cannot block deletion of selected editable nodes. Pause automatic refresh while the context menu is open. Remove actual outer topic markers and compact native/main stream indices, keeping surviving disk IDs and independent node records. Main manifest revision 19; native layout revision 3; no development migration or placeholder topic streams. Update gateway bindings/token together.
- **Acceptance:** SDK candidate builds and modified JavaScript parses; owner verifies canvas deletion/native effect removal and directly visible controls in AE 2023. No tests added/run without a request. AEX deployment requires AE absent or separately authorized closure under ADR 0011.

### G-06 — Multiple emitters and normal render cancellation (2026-10-02)

- **Owner:** integration lead; active task.
- **Dependencies:** G-05, P-02D, M2-05; ADRs 0005, 0012, 0015.
- **Owned files:** `src/core/GraphEvaluation.cpp`, `include/starfield/core/ParticleSimulation.hpp`, `src/core/ParticleSimulation.cpp`, `ae_plugin/SmartRender.cpp`, `ae_plugin/PluginVersion.h`, `ae_plugin/BuildWindows.ps1`, `cep_panel/js/panel.js`, `cep_panel/js/graph_edits.js`, `cep_panel/css/panel.css`, `cep_panel/README.md`, and matching architecture, roadmap, backlog, ADR, checkpoint, build and compatibility documentation.
- **Scope:** handle owner build-9 feedback: two normal cancellation dialogs and the one-active-emitter restriction. Keep independent native node effects, saved IDs, graph schema, the C ABI and Render.hpp unchanged. Partition each emitter's births over its own active Particle children; merge live births under Output's single cap. Preserve deterministic identity `(emitter UUID, local birth slot)`, bounded memory/work, force merge and one-pass appearance precedence. Never fill AE's error message for normal cancellation.
- **Acceptance:** May 2023 SDK candidate builds; owner verifies multi-emitter output and editing cancellation in AE 2023. Do not add/run tests without an owner request. Compilation is not host qualification.
- **Status:** SDK build and both CEP syntax checks pass; source pushed as c368bbf before build-10 deployment. AE-absent standing deployment completed; six installed hashes and selected Core verified, one Junction retained, build 9 backed up. Owner verification pending. Owner says build 9 is basically working but reports the three remaining errors; exact node-operation and bit-depth acceptance remains unrecorded.
- **Owner steering:** fix wire-click disconnect and snap a dragged connection to compatible ports within a constant screen-space radius. The node container must allow wire hit testing; hold automatic refresh during a wire press and commit once on release. Existing native graph transactions remain the save/undo path.

- **BUILD-23 (integration lead):** owner scope is now AE 2023 only. Script/MSBuild defaults and primary docs use the May 2023 SDK and `artifacts/plugin/2023/`; newer-host tasks are deferred. Owns build defaults and policy documentation only.
- **G-03:** emitter/output core runtime and graph/flat parity implemented in `070c33e`; the force/appearance kernels now complete the Alpha chain (closed-form gravity/drag integration, linear age curves for size/opacity/color, stage-order enforcement) and graph/flat parity is pinned by the core tests. The current core suite passes 11,909 checks and the May 2023 SDK build succeeds.
- **G-04:** `GraphParameter.cpp` implements AE arbitrary-data ownership/codec callbacks; legacy controls preserve old-project animation, Node Graph mode consumes a pre-render snapshot, and Capture Current Controls uses AE's supervised parameter-change path. `graph_from_controls` builds Emitter -> Particle -> Force -> Output, and every bound control is supervised so a Node Graph edit rewrites the canonical graph in the same user-change transaction. Native adapter tests pass 660 checks after the preview-scale, output-world, and appearance-projection regressions; the May 2023 SDK build succeeds. AE Controls save/reopen, same-layer duplicate, same-name copy/paste replacement and undo/redo passed. The H-01 host pass reopened the saved project in a new process with Control Source still in Node Graph mode, saved values, particle preview and CEP panel target intact; integrated graph-transaction undo, byte-level graph comparison, and build-1 project migration remain unqualified in AE.
- **M3-02 core + authoring surface:** gravity (17-19), linear drag (20), Color Start/End (21/22), Size End (23) and Opacity End (24) are registered AE controls whose defaults reproduce the previous look, so the visible improvement is reachable without the panel; the same controls are the panel's edit surface.
- **M3-01B implementation:** pre-release revision 12 uses direct full-resolution layer-pixel Size X/Y/Z values (0–100000, default 100 px) in AE controls, the CEP Emitter inspector, and graph keys 19–21. Box uses all axes and Sphere forms an ellipsoid; Disc keeps its dedicated Disc Size. Dimensions convert through full-resolution layer height and pixel aspect. The prior percentage semantics are intentionally dropped during development. Core/adapter regressions and AE 2023 host qualification are tracked in `docs/compatibility-matrix.md`.
- **P-02 panel / P-02A canvas:** `cep_panel/` now projects the revisioned canonical graph into a node canvas and selected-node inspector over ADR 0009 protocol v1. The earlier AE 2023.5.0 Build 52 form populated automatically after project/layer selection, a `Size` write updated the frame, and host undo restored the picture. The owner confirmed the node canvas now displays normally after reloading the CEP panel and reports no issue with the revision-7 AEX load. Integrated topology edits, layout save/reopen, duplication, undo/redo, stale-state rejection, and Node Graph synchronization remain to be qualified in AE.
- **Examples:** Spark, Snow and Floating Light are documented as exact parameter recipes in `docs/examples.md` and shipped as panel presets, so they can be accepted with or without the panel.
- **Time-dependent output flag:** `NON_PARAM_VARY` is present in PiPL/runtime because frame time changes rendered particles with constant parameters. MFR/GPU flags remain disabled.

- M0 architecture contracts and M1 SDK shell are in the source tree.
- The user confirmed the empty M1 shell loads in AE 2023. The current build-2, 24-active-parameter graph build has AE 2023 host evidence for controls, lifecycle, all bit depths, time consistency, and transparent particle output.
- **G-03/G-04 core and adapter:** the initial 6,196-core/395-adapter baseline is historical. The current core suite passes 11,909 checks and the adapter fake-host suite passes 678 checks. The May 2023 SDK build succeeds.
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
| G-03 | Core evaluation agent | Graph evaluation into the existing `RenderRequest`: dependency order, evaluation at rational time, cancellation, and emitter/output pixel parity | G-01, M2-01/03/04 | **Emitter/output runtime implemented:** `GraphEvaluation.hpp/.cpp`, ADR 0007. Stable dependency traversal, active-output reachability, semantic bounds, graph precedence and per-particle opacity, plus the M3-02 force/appearance stages. The current core suite passes 11,909 checks. Current schema stores constant parameters; animated sampling/history requires M3-03's contract |
| G-04 | AE adapter agent | Persist graph as AE arbitrary parameter data; retain animated legacy-control mode and provide explicit capture into node mode | G-02, G-03 | **Code complete; AE Controls host pass, integrated Node Graph gate open:** graph data, Control Source and Capture are registered; AE owns graph bytes and lifecycle callbacks; old projects select animated AE Controls via `USE_VALUE_FOR_OLD_PROJECTS`; capture writes the current-time graph and selects Node Graph in one supervised parameter transaction. SmartFX consumes a pre-render snapshot. AE 2023.5 Build 52 preview-scale point conversion is shared by render, capture, and graph synchronization; native adapter tests pass 660 checks. AE Controls save/reopen, copy and undo/redo passed on the build-2 candidate; the H-01 pass reopened the project in a new process with Control Source still in Node Graph mode and saved values intact. Integrated topology/curve transactions, byte-level graph comparison and build-1 project migration remain open in AE |
| G-05 | Core graph agent | Extend graph evaluation for Particle nodes, emitter fan-out, force chains and parallel force branches; preserve deterministic output and bounded work | G-01–G-04, ADRs 0006–0008, 0015, M3-01/M3-02 | **Portable core plus focused regression coverage pass; AE persistence/undo gates remain open:** `org.starfieldfx.nodes.particle` carries per-branch age curves; active Particle nodes are UUID-ordered and receive global emission slots by `slot % branch_count`, preserving total rate/cap and slot IDs. Force nodes support fan-in; each Particle branch accumulates reachable Force nodes once in stable dependency order. Active Force/Appearance bypass paths in explicit Particle mode are rejected, not silently dropped. Particle/Appearance precedence is resolved once per particle. Evaluation allocates one global-ID-ordered output buffer and writes each modulo branch directly into it without per-branch particle vectors or a final sort. The uniform gravity/drag kernel sums fields and integrates once. Emitter schema 3 has no Max Particles value; Output schema 2 owns the cap. Core suite: 11,909 checks; adapter suite: 678 checks. Multiple active emitters and stochastic allocation remain deferred. An output ancestry with Emitter but no Particle is transparent; active Force/Appearance without a Particle source fails. Emitter/Particle schema-1 and pre-release emitter/output snapshots are unsupported during development. A traversal work cap is pinned in ADR 0015. P-02B projects dynamic graph topology into CEP and routes add, connect/reconnect, disconnect, splice, delete, duplicate, move, and typed parameter changes through revision-checked transactions. AE carrier, persistence, undo, and render behavior remain unqualified. |
| P-01 | Owner + lead | Choose the AE 2023 dockable-panel bridge and specify its versioned protocol and failure behavior | Wave G contracts | **Architecture accepted in ADR 0009:** CEP `CSInterface.evalScript` → ExtendScript → supervised script-visible AE parameter streams → effect updates canonical arb graph in `PF_Cmd_USER_CHANGED_PARAM`. No direct arb-stream writes, sockets, or render-time AEGP queries. AE 2023 host gate remains open for scripted hidden-stream access and undo behavior. |
| P-02 | Panel agent | Build the dockable effect-control panel over ADR 0009, with grouped parameter editing through supervised AE streams | P-01; force/appearance parameter bindings | **Bridge and automatic discovery implemented; partial AE qualification:** source is installed through the authorized Junction and the earlier form auto-populated after project/layer selection without Refresh. Parameter reads/writes, undo grouping, stale-state rejection, and Node Graph synchronization remain to be qualified. P-02A supplies the new fixed topology view. |
| P-02A | Panel agent | Add a visible dockable node canvas for Emitter → Particle → Force → Output; show compact node cards, top/bottom ports, connectors, and a selected-node inspector. Permit node dragging and refresh selected target state without a manual click | P-02, ADR 0009 protocol v1, ADR 0014 | **UI visibility confirmed; layout lifecycle qualification open:** `cep_panel/` implements the compact top-down node canvas, type colors, target pin, viewport-wide marquee/group movement, cursor-anchored wheel zoom, middle-button pan, signed coordinates, minimap, and movable inspector. The owner confirmed the canvas displays after reloading the CEP panel. The initial fixed four-stage view and eight AE layout streams remain for compatibility; P-02B now projects the canonical graph and persists UUID-keyed positions in the graph record. Dynamic graph edit and layout undo/save/reopen behavior still require the P-02B AE 2023 pass. |
| P-02B | Graph/panel | Deliver add, connect/reconnect, disconnect, splice, delete, duplicate, move and parameter editing | P-02A, G-01–G-05, ADRs 0013, 0015, 0019 | **Source/build pass; host acceptance open.** Revision 16 uses separate hidden node effects and numeric trigger 43; request 90 is removed. Guard 42 batches Output changes. Rollback restores native records and compiled render state; semantic acknowledgement permits AE quantization. Eight focused CEP suites and 687 adapter checks pass. The build blocker is resolved; build 3 is deployed. The preceding build-2 preview crashed before node tests. Safe apply/preview, actual node operations, rendered response, undo and reopen remain open. See the current checkpoint. |
| P-02D | AE adapter/panel | Make independent node effects the saved source and compile them into the renderer snapshot | P-02B, G-04/G-05, ADRs 0008, 0009, 0012, 0019 | **Build 9 deployed 2026-10-02; six hashes verified; fresh-effect owner test pending.** Build 8 still fails with the same duplicate-matchname error. All node FourCC disk IDs violated the SDK's 1..9999 contract. Registration and supervised lookup now share explicit numeric IDs. Compile-time guards cover allocation range/uniqueness and name budget. Native identity revision 2 has no development migration. Stream indices, main schema 18 and CEP token native-node-sync-6 unchanged. SDK build passes; no tests added/rerun. Host truncation is inferred. Owner confirms selection safety and Emitter duplication. Particle creation, native 32-bpc operation, render/cache response, undo and reopen remain open. |
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

P-02B owner acceptance update (2026-09-30): despite the canvas and transaction
implementation, node addition and removal still do not work in the owner's AE
test. Do not report dynamic topology editing as delivered. The owner confirmed
the desired direction: each node should be an independent AE effect instance,
with node parameters saved on that instance. P-02D and ADR 0019 now
define the first native-effect synchronization spike before resuming general
graph editing.

P-02B splice follow-up (2026-09-30): the canvas now passes the node's pointer-drop
position into the splice transaction and includes other moved selection positions in
that same edit. A focused CEP edit test checks the exact node drop coordinate, grouped
layout, and codec round-trip. The AE carrier and gesture still need host qualification.

P-02B duplication follow-up (2026-09-30): Alt-drag and Ctrl+D use the same graph
duplication policy from ADR 0019. Particle and Force copies retain compatible links;
Emitter copies stay disconnected, Appearance copies stay disconnected, and links
between selected nodes are redirected to their copies. Focused planner coverage checks
copied branches reaching the existing Output. AE effect creation, undo, cache update,
and save/reopen remain unqualified.

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


## P-02J — native node keyframes (2026-10-02)

Owner: primary adapter agent. Dependency: P-02I build 17, owner confirms edits work.
Owns NodeEffects, NativeNodeGraph, NativeGraphCommit, GraphCarrier, Parameters,
PluginVersion, schemas/parameters + node-parameters, gateway animated value writes,
scoped native/registration tests and runner, and matching architecture/roadmap/
ADR 0023/build/compatibility records. Core render API and simulation are unchanged.
Implement public stopwatches plus per-frame owned PF dependency inputs; constants
remain only for topology/identity/curve banks. Source/build checks precede deployment.
AE interpolation, undo/reopen and CEP-closed animation remain owner qualification.

Build 18 follow-up: owner reports loader token mismatch and incorrect animation.
Extend this card to own panel.js startup handshake and its scoped startup tests/
README. Correct paired-version validation using the actual JSX readiness response,
then investigate render-time sampling and owner-described animation behavior.
Also own NodeGraphSync.hpp's local failure-stage diagnostics for animation bindings.

Build 19 source candidate: confirmed panel/JSX mismatch fixed; typed effect/param
bindings, evaluated-value/state validation, sentinel rejection and rollback added.
798 scoped checks plus actual CPU pixels, generated-expression and JSX startup
regressions pass. Owner Origin XY black output is recorded; its binding-object
cause is a hypothesis pending AE 2023 qualification. SDK Release candidate builds.
Build 19 deployed with AE absent; all six installed hashes, prior bundle backup
hashes and selected/pinned Core parity verified. Owner fresh-layer animation gate
remains open; deployment and one-step rollback recorded in build-matrix.md.

Build 20 follow-up owns the same adapter/tests/docs files: owner opening error
stream -1; remove SmartFX's incorrect delivered-parameter-count guards from
animation sampling and render globals. Count-zero rejection reproduced before
repair, full graph/CPU pixel checks pass after. 812 scoped checks pass; new phase/
stream/count diagnostics preserve real checkout failures. Owner host acceptance
remains open; CEP and render contracts unchanged.
Build 20 installed with AE absent. Six candidate/installed and prior-backup hashes
plus pinned/selected Core verified; deployment and rollback recorded in build-matrix.


P-02J source/build/deployment complete: 761 scoped checks plus generated expression/
CEP keyed-write checks pass; build 18 installed through the existing Junction,
all six hashes/backups verified. Owner AE animation gate remains pending.
