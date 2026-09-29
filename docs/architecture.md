# Starfield plug-in architecture

Current target: After Effects 2023 on Windows x64, built with the supplied May 2023 SDK. Owner direction on 2026-09-27 defers newer-host adaptation and qualification. See [the detailed roadmap](roadmap.md) for the support matrix and milestone gates.

## Goal and boundary

This repository is the clean implementation of a node-based particle effect for After Effects. Existing reverse-engineering notes and binaries live in the parent directory and are reference material only. They are not linked into the build and must not be copied into the new implementation. Use observed user-facing behavior to define independent requirements; implement the algorithms and data structures anew.

The native effect is the product boundary. The AE SDK adapter translates selectors, parameters, pixel worlds, and host suites into a small host-independent C++ API. The core does not include AE headers, retain host pointers, access the filesystem implicitly, or call UI APIs.

## Layers

```text
After Effects
  └─ StarfieldParticle.aex   PiPL, selectors, AE parameters, graph persistence,
       │                     SmartFX snapshots, host pixels and DLL loader
       └─ StarfieldCore.dll graph evaluation, deterministic simulation,
                             CPU renderer and later backends
```

Keep UI and preset compatibility as separate adapter modules. The AE 2023 dockable CEP panel uses the versioned ExtendScript bridge in ADR 0009. It edits supervised, script-visible AE parameter streams; the effect turns those changes into the canonical arbitrary-data graph during `PF_Cmd_USER_CHANGED_PARAM`. The panel never shares C++ object layouts with the effect, and render code never queries panel or AEGP state.

ADR 0012 defines the runtime C ABI and versioned development DLLs. The AE
adapter still owns graph serialization and control-to-graph construction because
those operations participate in AE parameter persistence; evaluation and
rasterization run in the selected DLL generation. Each pre-render pins that
generation until its matching render and result release finish. Its content
identity is mixed into the SmartFX cache key.

## Render contract

1. SmartFX pre-render checks out input metadata for layer bounds/reference geometry and time-varying values, determines the output/ROI, and constructs an immutable `RenderRequest`. Particle rendering does not consume input pixels; it writes premultiplied particle color over transparent black (ADR 0005).
2. The core validates all external values, evaluates the graph deterministically for the requested rational time, and returns an owned staging buffer or a typed error.
3. The adapter copies pixels to the host output while checkouts are valid, then releases every checkout on every exit path.

Particle state must be reproducible from graph parameters, seed, and absolute time. Do not advance a process-global simulation by “one frame”; AE can request frames out of order, repeatedly, or concurrently. Random streams should be derived from stable particle IDs and the effect seed. Frame time remains rational through the adapter and is converted once at the simulation boundary.

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

The CMake build compiles only the portable core. The AE module is built by the Windows MSBuild project against the local May 2023 SDK by default. M2 implements SmartFX transport and 8/16/32-bpc CPU rendering; the owner reports all bit depths and time consistency pass in AE 2023. On 2026-09-28, AE 2023.5.0 Build 52 visually confirmed the updated adapter renders particles over transparency. G-03 evaluates the single-emitter `emitter → force → appearance → output` chain (ADR 0007), and G-04 persists a graph snapshot in an AE arbitrary-data parameter (ADR 0008). MFR, GPU, and Compute Cache remain disabled.

## SDK guidance used

- [Entry point](https://docs.yuelili.com/en/ae-plugin/effect-basics/entry-point/) and [command selectors](https://docs.yuelili.com/en/ae-plugin/effect-basics/command-selectors/) define the AE boundary and lifecycle dispatch.
- [SmartFX](https://docs.yuelili.com/en/ae-plugin/smartfx/smartfx/) supplies pre-render/render separation and explicit input dependencies.
- [Multi-Frame Rendering](https://docs.yuelili.com/en/ae-plugin/effect-details/multi-frame-rendering-in-ae/) documents thread-safety requirements and Compute Cache guidance.
- [Parameters](https://docs.yuelili.com/en/ae-plugin/effect-basics/parameters/) and [PF_ParamDef](https://docs.yuelili.com/en/ae-plugin/effect-basics/pf_paramdef/) establish stable parameter IDs and selector-scoped values.
- [PiPL resources](https://docs.yuelili.com/en/ae-plugin/intro/pipl-resources/) and [symbol exporting](https://docs.yuelili.com/en/ae-plugin/intro/symbol-export/) guide registration and minimal exports.

## Delivery sequence

1. **Complete in code:** M0/M1 contracts and the AE 2023 Windows x64 shell.
2. **Complete in code; host gate open:** M2 SmartFX transport, ROI, pixel-format adapters, and deterministic CPU rendering. See M2-06 in `compatibility-matrix.md`.
3. **Implemented in core and controls:** M3-01 seeded emitters and M3-02 gravity, drag, and linear age curves, evaluated through the single-emitter four-stage graph.
4. **Graph data path implemented; topology editing is in progress:** G-01–G-04 provide the graph model, codec, evaluation, and AE arbitrary-data persistence. CEP protocol v1 edits supervised parameter streams and cannot write graph topology. The current canvas source adds a pinned target, top-down layout, signed node coordinates, viewport-wide marquee/group movement, wheel zoom, middle-button pan, a bottom-left interactive minimap, project-saved node coordinates (ADR 0014), Alt-drag copy preview, Ctrl+D, port-to-port connection gestures, and a graph-specific context menu; topology commands remain guarded. Pan and zoom are transient. The owner’s AE 23.5x52 probe confirmed ExtendScript cannot read the graph property because `CUSTOM_VALUE` is not implemented. ADR 0013 proposes a script-visible expression snapshot plus supervised commit stream; it remains unaccepted pending callback, undo, and persistence qualification. P-02B must establish a bounded, project-saved, undoable graph transaction and snapshot carrier before topology gestures can change project state.
5. **Remaining Alpha work:** qualify the current build in AE 2023, then implement graph transactions and multi-branch evaluation, define animation/history, and prioritize depth/projection, source types, and presets from observed behavior.
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

M0/M1, M2 rendering, M3-01/M3-02 core behavior, and G-01–G-04 are implemented. P-02A's current source draws a fixed node canvas with draggable cards, signed project-saved node coordinates, viewport-wide marquee selection, a bottom-left minimap, automatic target polling, and a movable floating properties window; this UI revision has not yet been viewed in AE. ADR 0014 defines eight hidden effect streams and one undoable layout write. Earlier AE 2023.5.0 Build 52 testing confirmed the prior panel auto-populated after project and target-layer selection without a Refresh click. Startup discovery retries transient conditions until the target is available, with delay capped at 5 seconds. The current effect registers 24 active non-input render/control parameters plus eight hidden layout streams; topic markers and AE's implicit input account for the larger registration count. Prior checkpoint runs covered the host-independent core, fake-host adapter, and panel gateway/startup suites; no tests were rerun for this UI revision. Owner testing confirms time consistency; AE visually confirmed 8/16/32-bpc output and transparent particles with the split build. Save/reopen, effect copy, and undo/redo have host evidence on the split build; layout-stream persistence still needs its own host pass. Dynamic topology editing, multi-branch evaluation, graph history/animation, MFR, and GPU remain future work.
