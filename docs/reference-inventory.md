# Reference behavior inventory

This is a planning index distilled from the existing Stardust binary-analysis report in the parent workspace. It contains observations and hypotheses, not copied implementation details. Static symbols and imports do not by themselves prove a visible feature or its exact semantics. Confirm user-facing behavior with the reference effect and record the AE version, settings, and result in `compatibility-matrix.md` before treating an item as a requirement.

| Candidate family | Evidence in the existing report | Confidence | Planning action |
|---|---|---:|---|
| Particle emitters and lifecycle | Particle subsystem types; current parameter inventory includes count, birth rate, lifetime, seed | Medium | Build the deterministic point-emitter slice first; observe exact birth ordering and lifetime behavior |
| Shape emitters | Core plan includes point/box/sphere/disc | Low until visually confirmed | Capture repeatable reference cases before implementing non-point shapes |
| Particle forces and surface modifiers | RTTI names include bend, duplicate, maps, path, sphere, stretch, transform, turbulence, and twist manipulators | Medium for existence, low for semantics | Group by visible behavior; implement each as a separate node/feature task |
| Mesh and depth rendering | Renderer and shader symbols include mesh, depth, normals, preview, and shadow-map programs | Medium | Defer until the 2D CPU renderer is stable; verify whether each is exposed in the UI |
| Volumetric rendering | OpenVDB-related types, volume sampler, and volume-rendering stage symbols | High for code presence, low for supported workflows | Keep as a late feature family with a separate performance and memory budget |
| Texture/layer sources | Texture-source and bitmap types; AE integration includes texture source | Medium | Identify source types and alpha/color behavior from UI and sample projects |
| Presets and UI utilities | Preset handlers and master-switch types for motion blur, preview, shy, solo, utilities, and helpers | Medium | Prioritize preset round-trip and essential controls; avoid recreating panel plumbing before the effect works alone |
| Separate panel communication | A panel-side talk handler and networking imports | Low; transport is inferred | Do not reproduce a socket or process boundary without a proven product requirement |
| Legacy stability risks | Report notes old statically linked rendering/parallel dependencies; a separate dump attributes one initialization crash to unchecked indexing | Medium for reported binary facts | Use as motivation for bounded inputs, minimal dependencies, and conservative threading; do not treat the old crash as evidence about the new implementation |

## Feature order

1. A stable point emitter and CPU sprite compositor.
2. Common controls and deterministic behavior: count, rate, lifetime, seed, velocity, size, opacity.
3. Shape emitters and common forces/modifiers, one isolated feature per task.
4. Node graph, presets, textures/layers, and project migration.
5. Mesh/material/light and volumetric families, each gated by a concrete user-visible scenario and performance target.
6. MFR and GPU only after the serial CPU renderer is correct and the host APIs are qualified.

The existing reverse-analysis report remains outside this Git root. This checked-in summary is the portable, reviewable feature index; do not add raw SDK payloads, plug-in binaries, or copied decompilation output to the source repository.
