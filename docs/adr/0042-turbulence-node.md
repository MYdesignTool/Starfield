# ADR 0042 — Deterministic Turbulence field and node contract

Status: M3-19 implementation in progress. This first increment adds only the
host-independent bounded noise kernel; no Turbulence graph node, native effect,
CEP control, render behavior, or AE qualification is claimed yet.

## Evidence boundary

The [official Stardust guide](https://superluminal.tv/user-guide) describes a
Turbulence node as smooth noise that can affect particle position, color and
size. It names Normal, Spherical/Spherical 2, Axis/One side axis, axis choice,
all-particle or feathered-sphere influence, randomized amount, a zero-to-max
life graph, and noise scale/levels/amplitude/frequency/speed/evolution/offset.
The local parameter inventory records the public labels and fresh values in
`artifacts/reference/stardust_effect_parameters.txt`; it records default popup
values, not the full menu strings. These sources define visible scope only; they
do not establish the private implementation or exact numerical behavior.

## Independent numeric foundation

`TurbulenceNoise` provides deterministic, allocation-free 3D gradient noise with
quintic interpolation and a normalized multi-octave sum. Each output channel has
an independent hash domain. Valid inputs always yield finite values in [-1, 1];
invalid coordinates, frequency, or octave counts fail without changing output.
The field is a pure function of coordinate, seed, octave count and frequency, so
random frame order and render-thread scheduling cannot change a sample.

The initial kernel bounds coordinates to ±1e9, base frequency to (0, 1024], and
octaves to 1..8. Its values are implementation primitives, not Stardust units.
The public controls' conversion into coordinate/frequency/time is deliberately
left to the graph/AE adapter contract and must be recorded before authoring.

## Open author/evaluation decisions

- Confirm the full visible enum labels/order for Turbulence Type, Choose Axis,
  Turbulence Affect and Affected Properties from the reference UI before fixing
  public popup values or native stream bindings.
- Define Position/Scale/Color/Opacity/Rotation/Noise output application and units.
  The guide names the effects but does not give numeric equations or ranges.
- Define Normal, Spherical, Spherical 2, Axis and One side axis mappings from
  visible examples; do not infer that a guessed popup ordinal proves the mapping.
- Define feathered-sphere distance, spatial scale/frequency conversion, temporal
  evolution and Fractal Speed, per-particle randomization, and Over Life curve
  interpolation while preserving arbitrary-time determinism.
- Integrate the new type through graph validation/evaluation, branch-local
  upstream-to-downstream ordering, history, native stream bindings, CEP/presets,
  schema, build and paired deployment. Do not reuse the Force node identity or
  present an inert node as implemented.
- Keep evaluation work bounded by existing graph/render budgets and test repeat,
  reverse-time, animation, multiple branches, Auxiliary inheritance and invalid
  resource/value paths before AE qualification.

## Initial evidence

`RunTurbulenceNoiseTests.ps1 -Run` and `-Run -Sanitize` each pass 21,411 checks.
They cover deterministic seeds/channels, the full octave range, amplitude bounds,
continuity across lattice boundaries, nearby samples, input rejection, atomic
output preservation, and reversed query order. These tests validate only the
numeric kernel; they do not establish node behavior or AE 2023 compatibility.
