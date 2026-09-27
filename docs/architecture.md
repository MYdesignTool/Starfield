# Starfield plug-in architecture

Target host baseline: After Effects 23.0 (2023). Use the newest Adobe AE SDK at each release cut, while runtime-gating optional suites and keeping the minimum-host selector path compatible. See [the detailed roadmap](roadmap.md) for the support matrix and milestone gates.

## Goal and boundary

This repository is the clean implementation of a node-based particle effect for After Effects. Existing reverse-engineering notes and binaries live in the parent directory and are reference material only. They are not linked into the build and must not be copied into the new implementation. Use observed user-facing behavior to define independent requirements; implement the algorithms and data structures anew.

The native effect is the product boundary. The AE SDK adapter translates selectors, parameters, pixel worlds, and host suites into a small host-independent C++ API. The core does not include AE headers, retain host pointers, access the filesystem implicitly, or call UI APIs.

## Layers

```text
After Effects
  └─ ae_plugin/       PiPL, selector dispatch, suites, parameter checkout,
                      SmartFX pre-render/render, sequence serialization
       └─ core/       typed graph/settings, deterministic simulation,
                      coordinate/color conversion, renderer interfaces
            ├─ cpu/   reference renderer; first production backend
            ├─ gpu/   optional backend behind the same render contract
            └─ cache/ stable keys and AE Compute Cache integration
```

Keep UI and preset compatibility as separate adapter modules. A future panel can use a documented, versioned message protocol; it must not share C++ object layouts with the effect or depend on undocumented process-local behavior.

## Render contract

1. SmartFX pre-render checks out only the requested inputs and time-varying values, determines the output/ROI, and constructs an immutable `RenderRequest`.
2. The core validates all external values, evaluates the graph deterministically for the requested rational time, and returns an owned staging buffer or a typed error.
3. The adapter copies pixels to the host output while checkouts are valid, then releases every checkout on every exit path.

Particle state must be reproducible from graph parameters, seed, and absolute time. Do not advance a process-global simulation by “one frame”; AE can request frames out of order, repeatedly, or concurrently. Random streams should be derived from stable particle IDs and the effect seed. Frame time remains rational through the adapter and is converted once at the simulation boundary.

## State and concurrency

- Global setup owns immutable plug-in metadata and acquired suite references. Global teardown releases them.
- Sequence data is versioned serialized user state only. Implement setup, resetup, flatten, and setdown; never mutate it while rendering.
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

## Initial source layout

- `include/starfield/core/Settings.hpp`: stable internal IDs and validated settings types.
- `include/starfield/core/Render.hpp`: host-independent time/frame/render request and backend contract.
- `src/core/Settings.cpp`: bounded, finite-value validation.
- `ae_plugin/`: native SDK adapter, PiPL resource, Windows MSBuild project, and reproducible PiPL build scripts.
- `docs/compatibility-matrix.md`: behavior inventory and independently derived acceptance criteria.
- `docs/adr/`: decisions that affect saved projects or rendering semantics.

The CMake build compiles only the portable core. The AE module is built by the Windows MSBuild project against local SDK 26.5 by default; the supplied May 2023 SDK is retained as an API compatibility build. The M1 shell does not claim SmartFX, floating-point color, or MFR support. Do not set those flags in PiPL or the adapter before implementing and qualifying the corresponding selector paths.

## SDK guidance used

- [Entry point](https://docs.yuelili.com/en/ae-plugin/effect-basics/entry-point/) and [command selectors](https://docs.yuelili.com/en/ae-plugin/effect-basics/command-selectors/) define the AE boundary and lifecycle dispatch.
- [SmartFX](https://docs.yuelili.com/en/ae-plugin/smartfx/smartfx/) supplies pre-render/render separation and explicit input dependencies.
- [Multi-Frame Rendering](https://docs.yuelili.com/en/ae-plugin/effect-details/multi-frame-rendering-in-ae/) documents thread-safety requirements and Compute Cache guidance.
- [Parameters](https://docs.yuelili.com/en/ae-plugin/effect-basics/parameters/) and [PF_ParamDef](https://docs.yuelili.com/en/ae-plugin/effect-basics/pf_paramdef/) establish stable parameter IDs and selector-scoped values.
- [PiPL resources](https://docs.yuelili.com/en/ae-plugin/intro/pipl-resources/) and [symbol exporting](https://docs.yuelili.com/en/ae-plugin/intro/symbol-export/) guide registration and minimal exports.

## Delivery sequence

1. **Complete:** select AE 2023 as the minimum, pin the local SDK/toolchain pair, and build/load the M1 shell.
2. Register parameters from the stable manifest and finalize the time/render request contract.
3. Add SmartFX checkout, ROI, pixel-format adapters, and a deterministic CPU point-emitter renderer.
4. Add remaining emitters and particle controls as independent behavior tasks.
5. Add graph types, bounded sequence migration, and presets.
6. Add Compute Cache, MFR, and an optional GPU backend only after the serial CPU contract is stable. Defer a panel until a validated workflow needs one.

## Current scope

M0 contracts and the M1 loadable shell are present. The user confirmed that the shell loads in AE 2023; it currently exposes no particle controls and only passes the source through. AE SDK 26.5 is the primary build input, with the supplied May 2023 SDK as a baseline compile. The implementation roadmap begins with the M2 render contract, parameter bridge, and deterministic CPU vertical slice.
