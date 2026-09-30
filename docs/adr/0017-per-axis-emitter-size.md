# ADR 0017: direct per-axis emitter dimensions

- Status: accepted; supersedes the initial percentage interpretation during pre-release development.
- Date: 2026-09-30.
- Depends on: ADRs 0003, 0006, and 0007.

## Context

The first M3-01B implementation multiplied a shared `Emitter Size` by three
percentage controls. The owner clarified that axis sizes are direct dimensions,
not percentages, and that a 1000-unit ceiling is too low. The effect is still in
development, so there is no released project data that requires retaining the old
meaning.

## Decision

- Interpret `Size X`, `Size Y`, and `Size Z` as direct full-resolution layer pixels,
  with a 0–100000 range and 100 px defaults. Their existing AE parameter IDs 81–83
  and graph keys 19–21 stay fixed; the manifest revision advances to 12 to record
  the changed units. No percentage compatibility conversion is performed.
- Box uses the three values as its full edge lengths. Sphere samples a unit-volume
  sphere and scales each axis by half of the requested dimension, producing an
  ellipsoid. Point and the current planar Disc ignore these controls.
- The prior common `Emitter Size` parameter becomes `Disc Size`, measured in layer
  heights. It controls the Disc diameter. Box and Sphere do not combine it with
  Size X/Y/Z.
- The host-independent core receives an `EmitterDimensionContext` containing full
  layer height in pixels and pixel aspect ratio. It converts X as
  `size_x * pixel_aspect / layer_height` and Y/Z as `size / layer_height` into the
  canonical world coordinate system. Preview downsample is not applied here; the
  renderer's frame mapping performs that scale once.
- Graph keys 19–21 remain optional `float64` values on the schema-1 Emitter node.
  Missing values use the new 100 px default. This is a development default, not a
  promise to migrate pre-revision-12 graphs that stored percentages.

## Consequences

The same numeric control value now describes an actual full-resolution pixel
dimension. Existing pre-release projects can render with a different distribution
after revision 12, by owner direction. The parameter IDs and effect identity do not
change. The core remains independent of AE types and gets only the immutable frame
geometry it needs. Disc continues to use a separate common diameter until its
per-axis behavior is explicitly designed.

## Validation

Core checks cover finite-value fallback, the 100000 px clamp, independent Box
dimensions, ellipsoid scaling, Disc isolation, non-square pixel aspect, layer-height
conversion, and graph/control projection. The May 2023 SDK build verifies the AE
parameter declarations and ABI integration. AE 2023 visual and save/reopen
qualification remains a separate host gate.
