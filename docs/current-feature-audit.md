# Current feature audit

Review baseline: `0cee6c5` plus G-01/G-02 core graph foundation, 2026-09-27.

## What the implementation currently does

- The AE adapter uses SmartFX and converts host parameters/worlds into a host-independent CPU render request.
- The core evaluates a deterministic birth/lifetime schedule at an absolute time, supports seeded Point/Box/Sphere/Disc birth positions and per-particle velocity spread, and renders white soft-edged 2D discs over the optional input.
- G-01/G-02 define stable graph identities, typed nodes/ports/parameters, built-in emitter/output schemas, bounded validation, and the version-1 sequence codec. Graphs are not yet saved in AE projects or used to render.

## Why the current result feels far from Stardust

The M2 target was a render vertical slice, not a feature-parity release. M3-01 added emitter distributions, but the picture still has one appearance: white sprites with constant size and opacity. Particle age does not alter appearance; Z does not affect projection or occlusion; there are no forces, drag, gravity, textures, layer sources, meshes, materials, lights, volumes, or presets. The exposed controls are still a flat list. This explains the visual and workflow gap without treating a successful build as a parity result.

The current compositing default (particles over the optional source) is an explicit project choice in ADR 0005, not yet confirmed to match the reference. Treat differences from the old effect as open behavior questions until a repeatable AE reference case records them.

## Verification gaps

- The 13-control build has not been reloaded and rendered in AE 2023. Existing host evidence covers an earlier eight-control revision only; the exact AE build is also unrecorded.
- AE playback, Full/Half/Quarter preview, 8/16/32-bpc, cancellation, and project lifecycle remain unqualified for the current build.
- The point-control normalization shim is still provisional until the current AE 2023 Options readout records the delivered origin values.
- G-01/G-02 are core-only. G-03 must evaluate graphs into `RenderRequest`; G-04 must persist the graph and migrate the current AE controls before the graph can replace the flat parameter surface.

## Next product steps

1. G-03: evaluate the built-in emitter → output graph and prove pixel parity with the existing settings path.
2. G-04: add AE sequence persistence and migrate the pre-graph controls without changing released parameter IDs.
3. Implement particle size/opacity/color curves and common forces as graph nodes, each tied to observed reference cases and rendered fixtures.
4. Requalify the current binary in AE 2023 while core work proceeds; record exact host build and Options output.
