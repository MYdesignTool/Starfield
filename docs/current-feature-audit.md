# Current feature audit

Review baseline: `070c33e` plus G-04 host arbitrary parameter bridge, 2026-09-27.

## What the implementation currently does

- The AE adapter uses SmartFX and converts host parameters/worlds into a host-independent CPU render request.
- The core evaluates a deterministic birth/lifetime schedule at an absolute time, supports seeded Point/Box/Sphere/Disc birth positions and per-particle velocity spread, and renders white soft-edged 2D discs over the optional input.
- G-01/G-02/G-03 define the graph schema, bounded codec and validation, plus emitter/output CPU evaluation. G-04 adds an AE arbitrary-data graph parameter, render-time snapshots and explicit legacy-control capture. The dockable editor and further node kernels are not implemented.

## Why the current result feels far from Stardust

The M2 target was a render vertical slice, not a feature-parity release. M3-01 added emitter distributions, but the picture still has one appearance: white sprites with constant size and opacity. Particle age does not alter appearance; Z does not affect projection or occlusion; there are no forces, drag, gravity, textures, layer sources, meshes, materials, lights, volumes, or presets. The exposed controls are still a flat list. This explains the visual and workflow gap without treating a successful build as a parity result.

The current compositing default (particles over the optional source) is an explicit project choice in ADR 0005, not yet confirmed to match the reference. Treat differences from the old effect as open behavior questions until a repeatable AE reference case records them.

## Verification gaps

- The 16-parameter build has not been loaded in AE 2023. Existing host evidence covers an earlier eight-control revision only; the exact AE build is also unrecorded. Save/reopen, effect copy, old-project default selection and undo/redo remain unverified in the host.
- AE playback, Full/Half/Quarter preview, 8/16/32-bpc, cancellation, and project lifecycle remain unqualified for the current build.
- The point-control normalization shim is still provisional until the current AE 2023 Options readout records the delivered origin values.
- G-01 through G-04 have core/native code and fake-host coverage. Host persistence and undo claims remain unqualified until exercised in AE 2023. Capturing controls stores current-time constants; it does not transform historical animation into node tracks.

## Next product steps

1. Install the build-2 effect in AE 2023; exercise fresh add, build-1 project load, graph selection, explicit capture, save/reopen, duplication and undo/redo.
2. Implement particle size/opacity/color curves and common forces as graph nodes, each tied to observed reference cases and rendered fixtures.
3. Implement the dockable CEP panel using ADR 0009's ExtendScript/supervised-parameter bridge; edits must update the canonical graph parameter and redraw the effect. The v1 protocol displays a fixed single-emitter chain.
