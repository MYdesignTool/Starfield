# Agent-ready implementation backlog

This backlog is the task source for parallel coding agents after the Git baseline is created. Assign one task ID to one agent/worktree. Tasks below have explicit file ownership to reduce conflicts; the integrating owner reviews and merges interfaces in dependency order.

## Current checkpoint

- M0 architecture contracts and M1 SDK shell are in the source tree.
- The user confirmed the empty shell loads in AE 2023. It has not yet been confirmed through effect rendering, save/reopen, undo, or duplicate operations.
- Windows x64 `.aex` builds succeed against the May 2023 and 26.5 SDKs. Local SDKs and artifacts are ignored by Git.
- Product direction: independent implementation of observed behavior. Static analysis is a feature-discovery aid, not proof of exact behavior or a source-code specification.

## Before agent work: human-owned Git checkpoint

| ID | Owner | Work | Done when |
|---|---|---|---|
| GIT-00 | User | Initialize the Git repository and make a clean baseline commit. | Source, schemas, docs, `AGENTS.md`, and build scripts are tracked; `AdobeSDK/` and `artifacts/` remain ignored. |

No worker should create a branch/worktree from an uncommitted moving baseline. After this checkpoint, create one branch/worktree per assigned task ID.

## M1 closeout gate

| ID | Owner | Work | Depends on | Done when |
|---|---|---|---|---|
| HOST-01 | User / host operator | Finish the M1 host smoke pass in AE 2023 and record exact AE build, OS, and result. | M1 shell already loads | Effect can be added/removed, renders the input unchanged, duplicates, undo/redo works, and a saved project reopens. Record each result in `compatibility-matrix.md`. |
| HOST-02 | User / host operator | Check the shell in the current AE 26.x release. | HOST-01 | Record exact host build and load/render result; do not infer support from the 26.5 SDK compile. |

These are host-operated checks; code agents can prepare a concise checklist but cannot mark the results without host evidence.

## Wave A — tasks suitable for parallel assignment

### M2-01 — Freeze the render request contract and rational-time helpers

- **Owner:** core agent.
- **Dependencies:** Git baseline; ADRs 0002 and 0003.
- **Owned files:** `include/starfield/core/Render.hpp`, new core time/helper files, and the corresponding ADR edits.
- **Scope:** specify normalized signed rational time, overflow behavior, half-open ROI coordinates, downsample/pixel-aspect spaces, pixel row layout, and cancellation/error semantics. Keep AE types out of the core. Reconcile the request/output ownership comments with the actual value types.
- **Do not do:** add SmartFX code, particle physics, or a new third-party library.
- **Acceptance:** all invalid/overflow cases have defined errors; frame and output structures have unambiguous units and ownership; the AE adapter can convert SDK values without reinterpretation.

### M2-02 — Bind the parameter manifest to AE controls

- **Owner:** AE UI/adapter agent.
- **Dependencies:** Git baseline; `schema/parameters.json` and `Settings.hpp` are the public contract.
- **Owned files:** new parameter adapter files under `ae_plugin/`, `ae_plugin/EffectMain.cpp` parameter dispatch, `include/starfield/core/Settings.hpp`, `src/core/Settings.cpp`, plus mapping documentation. Coordinate before touching the shared entry-point file.
- **Scope:** register all eight controls with stable IDs, labels, ranges, defaults, and correct PF types; map popup values and AE point controls into core settings. Keep ID 0 reserved for AE's implicit input. Explicitly reconcile the one-based popup default (`Point` = 1 in AE) with the zero-based `EmitterShape::point` core enum.
- **Do not do:** add graph controls, custom panels, or render algorithms.
- **Acceptance:** each schema row maps to one AE control and one core field; bounds/defaults match `Settings.cpp`; seed validation respects the manifest's `2147483647` maximum; parameter IDs and match name remain unchanged.

These two tasks can run at the same time: M2-01 owns render/time contracts; M2-02 owns parameter registration and settings mapping. M2-03 waits for M2-02's settings mapping, while M2-04 waits for the M2-01 buffer contract and M2-03's particle type.

## Wave B — deterministic CPU vertical slice

### M2-03 — Implement deterministic point-emitter simulation

- **Owner:** simulation agent.
- **Dependencies:** M2-01 and M2-02; `Settings` validation.
- **Owned files:** new `include/starfield/core/ParticleSimulation.hpp` and `src/core/ParticleSimulation.cpp`; avoid AE adapter and pixel-buffer files.
- **Scope:** generate point-emitter particles from seed and absolute rational time. Define stable particle IDs, birth ordering, lifetime boundary, velocity integration, and cancellation polling. Each render request must stand alone; do not advance process-global state.
- **Acceptance:** same request yields the same ordered particle list; out-of-order frame requests agree with chronological requests; invalid settings are bounded before allocation.

### M2-04 — Implement the CPU sprite rasterizer and source compositor

- **Owner:** CPU renderer agent.
- **Dependencies:** M2-01; consume M2-03's particle instance type after its interface is agreed.
- **Owned files:** new CPU renderer and pixel helper files under `src/core/` and `include/starfield/core/`; do not touch AE selectors.
- **Scope:** rasterize a minimal point/sprite into owned output storage; honor ROI, dimensions, row bytes, pixel format, premultiplied alpha, and source compositing. Use checked size arithmetic and return typed errors.
- **Acceptance:** bounds/ROI are respected, padding bytes are not assumed absent, alpha is correct, and allocation/unsupported-format failures return without leaking buffers.
- **Open behavior question:** confirm from the reference effect whether particles composite over the input or replace it. If evidence is unavailable, record the chosen default as an ADR decision instead of presenting it as verified compatibility.

### M2-05 — Add the SmartFX AE transport adapter

- **Owner:** AE rendering agent.
- **Dependencies:** M2-01 and M2-02; agree the renderer call contract with M2-03/M2-04 before integration.
- **Owned files:** AE adapter implementation under `ae_plugin/`; coordinate shared `EffectMain.cpp` edits with M2-02.
- **Scope:** replace legacy `PF_Cmd_RENDER` with the documented SmartFX pre-render/render path; use scoped input/time checkouts, calculate bounded ROI, translate worlds to owned core buffers, copy output while host pointers are valid, and release every checkout on every exit path.
- **Do not do:** set MFR, float-color, or GPU flags as placeholders; keep a CPU fallback.
- **Acceptance:** host resources have one cleanup path, errors/cancellation map predictably, and PiPL/runtime flags agree with implemented selectors.

### M2-06 — Integrate and qualify the first particle render

- **Owner:** integration lead.
- **Dependencies:** M2-02 through M2-05.
- **Owned files:** integration fixes across adapter/core plus compatibility evidence.
- **Scope:** produce the first visible point emitter over a defined frame, confirm source alpha/compositing, and capture repeatable outputs at the supported bit depths. Run in AE 2023 before claiming minimum-host support; then check current AE.
- **Acceptance:** AE effect controls affect deterministic rendered pixels; repeated/reverse-time requests match; project save/reopen preserves controls; all claimed host evidence is recorded.

## Wave C — MVP controls, graph, and behavior families

| ID | Work | Dependencies | Main ownership | Gate |
|---|---|---|---|---|
| M3-01 | Box/sphere/disc emitter distributions and documented coordinate mapping | M2-03/04 | Core simulation | Each shape has repeatable seeded reference cases and bounded sampling |
| M3-02 | Particle age curves, size/opacity over life, gravity and drag | M2 vertical slice | Core simulation/settings | Boundary behavior is defined for birth, death, negative time, and subframes |
| M3-03 | Native AE parameter grouping, animation sampling, and UI organization | M2-02 | AE adapter | Animated parameters are checked out at requested render time, not read from UI globals |
| M4-01 | Typed node/port/edge model and graph validator | M3 settings contract | Core graph | Stable NodeId/EdgeId/ParamKey domains; invalid endpoints/cycles fail cleanly |
| M4-02 | Bounded sequence serializer, parser, CRC, and schema migrations | M4-01; `schema/sequence-format.md` | Core serialization + AE sequence lifecycle | Truncated/oversized/future blobs fail safely; save/reopen preserves graph identity |
| M4-03 | Versioned preset import/export | M4-01/02 | Core codec and AE commands | Round-trip and malformed-input behavior are specified; no implicit filesystem writes |

## Wave D — feature parity by observable family

Use `docs/reference-inventory.md` to choose one independently reviewable family per task: common forces/manipulators; texture/layer sources; mesh/material/light rendering; volumetrics; master switches and presets. Before implementation, add an observable reference case to `compatibility-matrix.md`. Static RTTI names alone are not acceptance criteria. Defer the separate panel protocol until the native effect is complete and there is a confirmed workflow that needs a panel.

## Wave E — qualification and shipping

| ID | Work | Dependencies | Gate |
|---|---|---|---|
| M6-01 | Thread-safety audit and AE MFR qualification | Stable serial renderer, graph snapshots, cache ownership | Only then advertise threaded rendering; serial/MFR output agrees across repeated and out-of-order frames |
| M7-01 | Profile and choose any AE GPU backend | Correct CPU renderer and measured bottleneck | Documented API only, CPU fallback always available, bounded CPU/GPU output differences |
| M8-01 | Installer, migration/upgrade path, diagnostics, release matrix | Feature and MFR gates accepted | Clean install/uninstall, project migration, and supported-host evidence |

## Agent task handoff template

> Implement **[task ID]** only. Read `AGENTS.md`, the listed ADRs, and this task card. Keep edits within the owned files unless integration is explicitly assigned. Preserve parameter/match/version/schema contracts. Return a short summary, changed files, actual build/host evidence, and any unresolved decision. Do not claim an untested AE host is supported.
