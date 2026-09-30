# ADR 0018: deterministic Particle size and opacity randomness

- Status: accepted for implementation
- Date: 2026-09-30
- Scope: AE 2023 controls, Particle/Appearance graph values, and core appearance evaluation

## Context

The reference Particle group exposes Size Random and Opacity Random percentages,
both defaulting to zero. Maxon's [public particle-group documentation](https://help.maxon.net/rg/en-us/Content/html/19-Form-particle.html) describes
100% as variation from zero to the current Size or Opacity setting. This is a
reference-behavior observation from a related product; exact Stardust distribution
details remain unverified. The existing core already reserves independent random
stream purposes for size and opacity.

## Decision

- Add `particle_size_random_percent` and `opacity_random_percent`, each bounded
  from 0 to 100 and defaulting to 0. For a base value `v`, percent `r`, seed `s`,
  and stable particle slot `id`, calculate
  `v * (1 - (r / 100) * U(s, id, purpose))`, where `U` is the existing uniform
  stream in `[0, 1)`. A value of 100 therefore ranges from nearly zero through
  the base value; zero is exact parity with the previous render.
- Draw each factor once conceptually from its pure `(seed, id, purpose)` stream.
  Since the stream is stateless, the same particle keeps its size and opacity
  factors across frames and age-curve evaluation. Size and opacity use separate
  purpose IDs so changing one control does not perturb the other or emitter motion.
- Apply variation after the Size/Opacity age curves. This scales the whole curve
  proportionally and does not alter birth/lifetime, position, force integration,
  color, particle IDs, or branch allocation.
- Add optional graph keys 9 and 10 to Particle and Appearance nodes. Missing keys
  mean 0. New explicit graphs store the values on Particle; downstream Appearance
  nodes can override them.
- Append AE revision-11 controls without moving released indices: topic 85, Size
  Random 86, Opacity Random 87, topic end 88. Both controls are supervised, so
  Node Graph mode updates the canonical graph in AE's user-change transaction.
- The CEP Particle inspector exposes both percentages. Old project graphs lacking
  the optional keys display and evaluate as zero.

## Compatibility

Existing AE projects and schema-1 graph bytes omit the new controls/keys and retain
their previous output because the defaults are zero. Do not change the graph or
sequence schema version, existing AE IDs, effect match name, or plug-in version.

## Validation

Portable checks cover 0% exact parity, 100% bounds, intermediate bounds, stable
values under repeated and different-frame evaluation, independent size/opacity
streams, seed changes, and curve-proportional variation. Graph/adapter checks cover
optional-key defaults, flat/graph parity, and revision-11 parameter registration.
AE 2023 persistence, undo, and visible variation remain host qualification gates;
compilation alone does not establish them.
