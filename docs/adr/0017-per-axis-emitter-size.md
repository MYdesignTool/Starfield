# ADR 0017: per-axis emitter size

- Status: accepted for the M3-01B implementation; AE 2023 qualification is pending.
- Date: 2026-09-30.
- Depends on: ADRs 0003, 0006, and 0007.

## Context

The existing emitter has one `Emitter Size` extent. It produces a cube for Box and
a sphere for Sphere. The reference exposes `Size X/Y/Z` as percentages. Adding
independent axis controls must preserve renders from existing AE projects and
schema-1 emitter nodes.

## Decision

- Keep the existing `Emitter Size` parameter and graph key as the common extent, in
  layer heights. Append `Size X`, `Size Y`, and `Size Z` percentages, each with a
  default of 100 and a supported range of 0–1000. A shape's axis extent is the common
  extent multiplied by that axis percentage and divided by 100.
- The three controls are general emitter dimensions. Box uses all three values.
  Sphere samples its existing uniform-volume sphere, then scales each coordinate to
  form an ellipsoid with the requested diameters. Disc uses X and Y to form an ellipse
  in its existing XY plane; Z remains unused until a depth-oriented disc is defined.
  Point ignores the dimensions. This gives future emitter types the same validated
  vector without claiming unsupported shape behavior today.
- Append AE parameter indices 80–84 after the released revision-9 streams: a topic
  marker at 80, the three visible controls at 81–83, and its topic end at 84. Existing
  indices and disk identities do not move. The CEP Emitter inspector binds the three
  visible controls through the existing supervised parameter path.
- Append emitter graph parameter keys 19–21 as optional `float64` values on the
  existing schema-1 emitter node. Missing keys mean 100% on each axis, so old graph
  bytes retain their prior geometry. New graph constructors write all three values.
- Graph percentages are constant values under the current schema-1 animation
  contract. In AE Controls mode, ordinary parameter checkout samples each axis at the
  requested render time.

## Consequences

The new parameters refine the old shared extent instead of replacing it. Existing
projects, captured graphs, and presets retain their size. The visible controls match
the reference's percentage convention while the renderer continues to use the
project's documented layer-height coordinates. The renderer's shape algorithm is
changed only at the emitter's deterministic birth-offset boundary.

## Validation

Core validation rejects non-finite input by replacing it with 100%, and clamps finite
percentages into 0–1000. Focused cases must cover legacy graph defaults, exact 100%
parity, independent Box bounds, ellipsoid axis scaling, Disc's XY scaling, and
deterministic repeatability. The May 2023 SDK build alone does not qualify After
Effects persistence or rendered shape appearance.
