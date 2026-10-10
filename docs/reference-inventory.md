# Reference behavior inventory

This is a planning index distilled from the existing Stardust binary-analysis report in the parent workspace. It contains observations and hypotheses, not copied implementation details. Static symbols and imports do not by themselves prove a visible feature or its exact semantics. Confirm user-facing behavior with the reference effect and record the AE version, settings, and result in `compatibility-matrix.md` before treating an item as a requirement.

| Candidate family | Evidence in the existing report | Confidence | Planning action |
|---|---|---:|---|
| Particle emitters and lifecycle | Particle subsystem types; current parameter inventory includes count, birth rate, lifetime, seed | Medium | Compare the existing deterministic emitter behavior against repeatable reference cases before adding graph-node features |
| Shape emitters | Core plan includes point/box/sphere/disc | Low until visually confirmed | Capture repeatable reference cases before implementing non-point shapes |
| Particle forces and surface modifiers | RTTI names include bend, duplicate, maps, path, sphere, stretch, transform, turbulence, and twist manipulators | Medium for existence, low for semantics | Group by visible behavior; implement each as a separate node/feature task |
| Mesh and depth rendering | Renderer and shader symbols include mesh, depth, normals, preview, and shadow-map programs | Medium | Defer until the 2D CPU renderer is stable; verify whether each is exposed in the UI |
| Volumetric rendering | OpenVDB-related types, volume sampler, and volume-rendering stage symbols | High for code presence, low for supported workflows | Keep as a late feature family with a separate performance and memory budget |
| Texture/layer sources | Texture-source and bitmap types; AE integration includes texture source | Medium | Identify source types and alpha/color behavior from UI and sample projects |
| Presets and UI utilities | Preset handlers and master-switch types for motion blur, preview, shy, solo, utilities, and helpers | Medium | Prioritize preset round-trip and essential controls; avoid recreating panel plumbing before the effect works alone |
| Separate panel communication | A panel-side talk handler and networking imports | Low; transport is inferred | Do not reproduce a socket or process boundary without a proven product requirement |
| Legacy stability risks | Report notes old statically linked rendering/parallel dependencies; a separate dump attributes one initialization crash to unchecked indexing | Medium for reported binary facts | Use as motivation for bounded inputs, minimal dependencies, and conservative threading; do not treat the old crash as evidence about the new implementation |

## Feature order

Particle Shape is restricted by the owner's complete 2026-10-10 menu screenshot:
Circle, Rectangle, Cloud, Texture, Face, Model, in that order. This explicit
user-visible inventory takes precedence over inferred type lists in static
reports. No additional Particle Shape mode is in scope. See
[the menu record](reference-texture-transfer-transform.md#particle-shape--owner-confirmed-complete-menu-2026-10-10).

1. Finish graph schemas, bounded persistence, and evaluation parity for the current deterministic emitter.
2. Add particle appearance and common behavior as graph nodes: size/opacity/color over life, gravity, drag, and common forces.
3. Add texture/layer sources, presets, and migration through repeatable AE cases.
4. Add mesh/material/light and volumetric families, each gated by a concrete user-visible scenario and performance target.
5. Qualify MFR and GPU only after serial graph rendering is correct and the host APIs are qualified.

The existing reverse-analysis report remains outside this Git root. This checked-in summary is the portable, reviewable feature index; do not add raw SDK payloads, plug-in binaries, or copied decompilation output to the source repository.

## External implementation reference

The owner suggested [H2O-2/particleGL](https://github.com/H2O-2/particleGL) on 2026-09-28. GitHub identifies the repository as MIT-licensed; it describes itself as a partially implemented OpenGL particle tool inspired by Trapcode Particular. Its README documents behavior differences in velocity distribution and feathering, and additional sprite color blending. Treat it as an independent behavior/performance reference, not as a drop-in engine or a specification for Stardust.

The implementation is a standalone SDL/OpenGL application with its own window/context, immediate-mode UI, and a wall-clock accumulator (`SDL_GetTicks`) that advances mutable emitter state. Its renderer uploads per-particle attributes/matrices and uses instanced draws. That buffer/draw pattern is a useful performance reference, but the live-time state model does not meet this plug-in's arbitrary-time, out-of-order deterministic AE render contract, and the private GL context is outside the current AE 2023 CPU-renderer policy. Do not link it into the `.aex` or copy code into the core as part of M2/M3; revisit its data-layout ideas only in a future GPU task using documented AE GPU selectors and a CPU fallback. If source code is ever adopted, pin the exact commit and retain the MIT notice plus notices for each bundled dependency.
