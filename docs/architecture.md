# Starfield plug-in architecture

Current target: After Effects 2023 on Windows x64, built with the supplied May 2023 SDK. Owner direction on 2026-09-27 defers newer-host adaptation and qualification. See [the detailed roadmap](roadmap.md) for the support matrix and milestone gates.

## Goal and boundary

This repository is the clean implementation of a node-based particle effect for After Effects. Existing reverse-engineering notes and binaries live in the parent directory and are reference material only. They are not linked into the build and must not be copied into the new implementation. Use observed user-facing behavior to define independent requirements; implement the algorithms and data structures anew.

The native effect is the product boundary. The AE SDK adapter translates selectors, parameters, pixel worlds, and host suites into a small host-independent C++ API. The core does not include AE headers, retain host pointers, access the filesystem implicitly, or call UI APIs.

## Layers

```text
After Effects
  ├─ StarfieldParticle.aex       main render effect, graph persistence,
  │                              SmartFX snapshots, host pixels and DLL loader
  ├─ StarfieldEmitter.aex        internal, menu-hidden Emitter node controls
  ├─ StarfieldParticleNode.aex   internal, menu-hidden Particle node controls
  ├─ StarfieldAppearance.aex    internal, menu-hidden Appearance node controls
  ├─ StarfieldForce.aex          internal, menu-hidden Force node controls
  └─ StarfieldCore.dll           graph evaluation, deterministic simulation,
                                 CPU renderer and later backends
```

The node AEX modules hold independent AE parameter streams; they do not render
particles. The existing `StarfieldParticle.aex` remains the only renderer and owns
the canonical graph snapshot. The graph's fixed visible Output terminal is the
panel representation of that main effect; no separate Output AEX is built.
Node AEX modules set `PF_OutFlag_I_AM_OBSOLETE` so they are not offered as
standalone effects in AE's Effects menu. The CEP bridge still adds them by stable
match name to persist independent node values; AE 2023 must confirm that this
scripted creation path works while the modules are hidden from the menu.

Keep UI and preset compatibility as separate adapter modules. The AE 2023 dockable CEP panel uses the versioned ExtendScript bridge in ADR 0009. A graph transaction creates/removes the corresponding node AEX instances, writes their values, and commits the canonical graph snapshot to the main effect in one undo group. The Output terminal and its global controls remain on the main effect. Direct node-control edits have a supervised graph-sync source path, but callback delivery, cache/render response, undo and save/reopen remain AE 2023 qualification gates. The panel never shares C++ object layouts with the effect, and render code never queries sibling effects or panel state.

ADR 0012 defines the runtime C ABI and versioned development DLLs. The AE
adapter still owns graph serialization and control-to-graph construction because
those operations participate in AE parameter persistence; evaluation and
rasterization run in the selected DLL generation. Each pre-render pins that
generation until its matching render and result release finish. Its content
identity is mixed into the SmartFX cache key.

## Render contract

1. SmartFX pre-render checks out input metadata for layer bounds/reference geometry and time-varying values, determines the output/ROI, and constructs an immutable `RenderRequest`. Particle rendering does not consume input pixels; the core composites into a premultiplied staging buffer and encodes the requested output alpha mode. The current AE adapter candidate requests straight-alpha output pending focused AE 2023 confirmation (ADR 0005).
2. The core validates all external values, evaluates the graph deterministically for the requested rational time, and returns an owned staging buffer or a typed error.
3. The adapter copies pixels to the host output while checkouts are valid, then releases every checkout on every exit path.

Particle state must be reproducible from graph parameters, seed, and absolute time. Do not advance a process-global simulation by “one frame”; AE can request frames out of order, repeatedly, or concurrently. Random streams should be derived from stable particle IDs and the effect seed. Frame time remains rational through the adapter and is converted once at the simulation boundary.

Each Particle node owns its lifetime and retires only the particle slots assigned
to its branch. Emitter schema 3 has no lifetime or Max Particles parameter; Particle
schema 2 requires its own lifetime. Output schema 2 stores one global Max Particles
cap in the main effect's graph snapshot and keeps stable particle-ID ordering across
branches. Particle Size is a base diameter
in full-resolution layer pixels (bounded at 100000 px). Size Over Life and Opacity
Over Life each use a fixed 0–100% curve that multiplies the corresponding base
value. Box and Sphere emitter dimensions are direct full-resolution layer pixels converted through frame
height and pixel aspect; the Disc uses its dedicated layer-height diameter control.

## State and concurrency

- Global setup owns immutable plug-in metadata and acquired suite references. Global teardown releases them.
- The graph is a versioned serialized arbitrary parameter owned by AE; never mutate it while rendering. Sequence data remains unused in the current graph design.
- Render requests and graph snapshots are immutable. No mutable global/static render state.
- Advertise threaded rendering only after every selector and every dependency is audited for concurrent calls. No host suite or checkout calls while holding a lock.
- Shared expensive derived data belongs in AE Compute Cache with a complete content key, not in mutable sequence data. Cache keys include schema version, graph revision, time, quality, dimensions, format, and every checked-out input dependency.
- CPU is the correctness reference. GPU is optional and must produce a defined fallback when the required device/backend is unavailable.

## Compatibility rules

- Parameter IDs and effect match name are persistent project-file identifiers. Never renumber/reuse a released parameter ID.
- Node IDs, graph edge IDs, AE parameter IDs, and UI widget IDs are separate identity domains. The graph schema owns node/edge identity and migrations; AE parameter IDs map UI controls into that graph but never become graph IDs.
- Store a schema version in flattened sequence data. Deserialization is bounded, validates lengths, and migrates known older versions; unknown versions fail safely with a readable message.
- Keep display labels, UI layout, and preset import/export independent from stable parameter IDs.
- Describe output time dependence accurately to AE so its frame cache cannot return stale particle frames.
- Convert host pixel formats at the boundary. Respect rowbytes, channel order, alpha mode, pixel aspect, downsample, ROI, and 8/16/32-bit worlds.
- Export only the SDK-required C entry points (`PluginDataEntryFunction2` and `EffectMain`). Hide other symbols to avoid collisions with AE and other plug-ins.

## Source layout

- `include/starfield/core/Settings.hpp`: stable internal IDs, manifest bounds, and validated settings types.
- `include/starfield/core/AgeCurve.hpp`: bounded normalized-age curves and their nested versioned graph payload.
- `include/starfield/core/Graph.hpp`: host-independent node, port, edge, parameter, registry, and graph-validation contracts.
- `include/starfield/core/SequenceCodec.hpp`: bounded schema-1 graph serialization and parsing with CRC-32.
- `include/starfield/core/Error.hpp`: typed error codes and the `Result` primitive shared by the core.
- `include/starfield/core/Time.hpp`: normalized signed rational time with checked arithmetic.
- `include/starfield/core/Render.hpp`: host-independent frame/request/output/backend contract and the cancellation interface.
- `include/starfield/core/ParticleSimulation.hpp`: deterministic point-emitter evaluation for one absolute time.
- `include/starfield/core/CpuRenderer.hpp`: the CPU reference backend and its bounded-work limits.
- `src/core/`: implementations of the above plus bounded, finite-value settings validation.
- `include/starfield/core/PluginApi.h` and `src/core/PluginApi.cpp`: fixed-width
  render/inspect C ABI with opaque result ownership and generation-local release.
- `src/core/GraphConstruction.cpp`: graph creation shared by the adapter and DLL;
  `GraphEvaluation.cpp` stays in the DLL for frequent algorithm changes.
- `ae_plugin/CoreLoader.*`: versioned DLL selection, ABI validation and leases.
- `tests/core_tests.cpp`: host-independent self-tests for the M2 contracts.
- `ae_plugin/`: native SDK adapter, PiPL resource, Windows MSBuild project, and reproducible PiPL build scripts (see `ae_plugin/README.md`).
- `docs/parameter-mapping.md`: schema row → AE control → core field table and the emission rules.
- `docs/compatibility-matrix.md`: behavior inventory and independently derived acceptance criteria.
- `docs/adr/`: decisions that affect saved projects or rendering semantics.

The CMake build compiles only the portable core. The AE module is built by the Windows MSBuild project against the local May 2023 SDK by default. M2 implements SmartFX transport and 8/16/32-bpc CPU rendering; the owner reports all bit depths and time consistency pass in AE 2023. On 2026-09-28, AE 2023.5.0 Build 52 visually confirmed the updated adapter renders particles over transparency. G-03/G-05 source includes explicit Particle branches, deterministic emitter-slot partitioning, and branch-local force accumulation (ADRs 0007/0015); all portable core translation units compile and link, while regression and host checks remain open. G-04 persists a graph snapshot in an AE arbitrary-data parameter (ADR 0008). MFR, GPU, and Compute Cache remain disabled.

## SDK guidance used

- [Entry point](https://docs.yuelili.com/en/ae-plugin/effect-basics/entry-point/) and [command selectors](https://docs.yuelili.com/en/ae-plugin/effect-basics/command-selectors/) define the AE boundary and lifecycle dispatch.
- [SmartFX](https://docs.yuelili.com/en/ae-plugin/smartfx/smartfx/) supplies pre-render/render separation and explicit input dependencies.
- [Multi-Frame Rendering](https://docs.yuelili.com/en/ae-plugin/effect-details/multi-frame-rendering-in-ae/) documents thread-safety requirements and Compute Cache guidance.
- [Parameters](https://docs.yuelili.com/en/ae-plugin/effect-basics/parameters/) and [PF_ParamDef](https://docs.yuelili.com/en/ae-plugin/effect-basics/pf_paramdef/) establish stable parameter IDs and selector-scoped values.
- [PiPL resources](https://docs.yuelili.com/en/ae-plugin/intro/pipl-resources/) and [symbol exporting](https://docs.yuelili.com/en/ae-plugin/intro/symbol-export/) guide registration and minimal exports.

## Delivery sequence

1. **Complete in code:** M0/M1 contracts and the AE 2023 Windows x64 shell.
2. **Complete in code; host gate open:** M2 SmartFX transport, ROI, pixel-format adapters, and deterministic CPU rendering. See M2-06 in `compatibility-matrix.md`.
3. **Implemented in core and controls:** M3-01 seeded emitters, M3-01B direct-pixel Box/Sphere dimensions, and M3-02 gravity, drag, and age curves. P-02C adds bounded Size/Opacity polylines. M3-06 makes Particle own lifetime and makes curve interpolation selection non-destructive; a render reproduced dark translucent particles over blue, so AE currently requests straight-alpha output pending focused host confirmation. AE curve, lifetime, and compositing checks remain open. G-05 provides Particle branches and parallel force merges, with focused core and adapter regression coverage; AE qualification remains open.
4. **Graph authoring architecture reopened:** G-01–G-05 provide the host-independent graph model, codec, persistence contract, and Particle/branch evaluation. P-02B's CEP canvas and transaction planner cover node add/connect/reconnect/disconnect/splice/delete/duplicate/move, typed edits, saved positions, selection, zoom/pan/minimap and context actions. P-02D adds/removes per-node Emitter, Particle, Appearance, and Force AEX instances, commits the graph snapshot to the main renderer, and includes a source prototype for direct node Effect Controls edits. The owner reproduced a graph commit failure because AE rejected expression writes to the non-time-varying request carrier; the source now leaves that hidden mailbox expression-capable and preflights `canSetExpression` before changing node effects. AE Controls startup also loads the graph snapshot so it can materialize native node AEX instances. The rebuilt candidate passes focused panel checks and the May 2023 SDK build, and is installed in the AE 2023 plug-in directory; AE has not loaded it yet. The fixed Output terminal stays visible in CEP, is backed by the main Starfield Particle effect, and has no standalone AEX. Reverse sync from manually deleting an Effect Controls module is not implemented. Never query sibling effects from render callbacks.
5. **Remaining Alpha work:** qualify P-02D's direct parameter-sync prototype and graph lifecycle in AE before resuming full P-02B/P-02C host integration; qualify G-05 behavior, define animation/history, and prioritize depth/projection, source types, and presets from observed behavior.
6. Keep MFR, Compute Cache, and GPU work behind separate concurrency/performance evidence and host-specific decisions.

## Current scope

H-01 separates the AE adapter from the runtime core and adds manual development
reload. The May 2023 SDK builds both binaries; 6,196 core checks, 395 adapter
checks and the loader harness pass. The `/MT` split pair is installed in AE
2023.5.0 Build 52. Full/Quarter hot reload, missing-DLL fallback, 8/16/32-bpc
visual rendering and save/close/reopen have host evidence. Three Full-resolution
8-bpc render-queue frames have exact decoded RGBA parity with the prior
monolithic binary; an in-flight AE render switch and broader pixel parity
remain open. The
prior monolith is backed up for rollback. Half/Third point mapping, split-build
copy/undo, shapes and basic gravity/size changes also have AE observations. See
`compatibility-matrix.md`.

M0/M1, M2 rendering, M3-01/M3-01B/M3-02 core behavior, G-01–G-05, and the CEP graph view/transaction source are present. P-02D wires CEP graph transactions to per-node AEX instances and the main effect's graph snapshot. The expression-capable mailbox fix and revision-14 hidden project parameter `Node Effects Ready` (ID 89) are installed in the main AEX (SHA-256 `9B3D75B2AA9E1EC1DED90F0993DCB7E66DD28FFC91DDB11968E656C1D9204017`); AE is closed and has not loaded this candidate. The prior main AEX is backed up under `artifacts/disabled/p02d-node-delete-sync-20260930/`. The marker distinguishes first-time node materialization from later manual Effect Parade deletion. AE Controls startup loads the graph snapshot and can materialize native node AEX instances. The reverse deletion path prunes a missing native node and incident graph edges through a revision-checked graph transaction; its AE undo/redo and reopen behavior remains unqualified. The four menu-hidden node AEXs were installed on 2026-09-30; menu visibility and CEP creation by match name remain unverified. P-02C routes graph-mode over-life curves through the same carrier and retains the AE Controls curve-bank path. The fixed visible CEP Output terminal is backed by the main Starfield Particle effect and is not a separate AEX. ADR 0014 defines the UUID-keyed project graph-layout record. Startup discovery polls until a target becomes available. The main effect currently registers parameter IDs 1–89 plus AE's implicit input (90 total indices). Owner testing confirms time consistency; AE visually confirmed 8/16/32-bpc output and transparent particles with the split build. Graph history/animation, AE qualification of node deletion synchronization, MFR, and GPU remain open.
