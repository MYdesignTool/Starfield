# Parameter bridge: schema → AE control → core settings

Task: M2-02, revised by manifest revision 6. `schema/parameters.json` owns the IDs,
labels, ranges, and defaults; `ae_plugin/Parameters.cpp` owns the host controls and
the conversion; the core only ever sees `starfield::core::Settings` after
`validate_settings`. One conversion path (`settings_from_controls`) serves both the
render checkout and the supervised panel edit, so the two cannot drift apart.

## Manifest revision 6: topic grouping and renumbering (current)

The ECW now follows the reference product's structure. Topic markers (`PF_Param_GROUP_START` /
`GROUP_END`) are parameters, so every index moved. Global out-flags carry
`PF_OutFlag2_PARAM_GROUP_START_COLLAPSED_FLAG`; without it AE collapses every group regardless of
the per-topic `PF_ParamFlag_START_COLLAPSED`.

| Topic | Index | Control (label) | Core field |
|---|---|---|---|
| Emitter | 2 | Type (popup Point/Box/Sphere/Disc) | `emitter_shape` |
| Emitter | 3 | Particles Per Second (default 100) | `birth_rate` |
| Emitter | 4 | Origin (3D point, 50/50/50 = layer centre) | `emitter_origin` |
| Emitter | 5 | Emitter Size | `emitter_size` |
| Emitter | 6/7/8 | Speed X / Speed Y / Speed Z | `velocity.x/y/z` |
| Emitter | 9 | Speed Random | `velocity_spread` |
| Particle | 12 | Lifetime | `particle_lifetime_seconds` |
| Particle | 13 | Size | `particle_size` |
| Particle | 14 | Size Over Life | `particle_size_end` |
| Particle | 15 | Opacity | `opacity` |
| Particle | 16 | Opacity Over Life | `opacity_end` |
| Particle | 17/18 | Color Start / Color End | `color_start` / `color_end` |
| Physics (collapsed) | 21/22/23 | Gravity X / Y / Z | `gravity.x/y/z` |
| Physics (collapsed) | 24 | Linear Drag | `linear_drag` |
| Render (collapsed) | 27 | Max Particles | `particle_count` |
| Render (collapsed) | 28 | Random Seed | `seed` |
| Render (collapsed) | 29 | Control Source (default **AE Controls**) | render source selector |
| Render (collapsed) | 30 | Capture Current Controls (button) | graph capture |
| Render (collapsed) | 31 | Node Graph Data (hidden arbitrary data) | canonical graph bytes |

The tables below describe the revision 2–5 layout and are kept for history: their index columns are
superseded by the table above. Labels in the reference vocabulary (Type, Particles Per Second, Origin,
Speed, Speed Random, Size Over Life, Opacity Over Life, Max Particles) replace the earlier working
names; semantics are unchanged except where a note says otherwise.

## Manifest revisions 2–3 (pre-release, 2026-09-27)

Revision 2 reshaped the controls after host feedback:

- The single 3D-point **Velocity** row is gone. Velocity is three scalar sliders, because a
  point control is a *position* control in AE (it draws a coordinate/pick widget, which is
  wrong for a direction and rate).
- **Emitter Origin** was added as a 3D point control, which is what AE point controls are
  for: it is a position, and AE provides on-screen picking for it.
- Emitter and velocity units are layer heights and layer heights per second.
- Revision 3 added `Emitter Size` and `Velocity Spread`, allowing seeded shape sampling and per-particle velocity variation.

No release has been published, so IDs are still being shaped here; they freeze at the first
shared release (ADR 0001). The old 3D-point Velocity stored a different value type, so a
project saved with revision 1 must be re-authored rather than migrated.

## Mapping table

| ID | key | AE control | core field | units and notes |
|---|---|---|---|---|
| 1 | `particle_count` | Float Slider, INTEGER, 0…2000000, default 1000 | `Settings::particle_count` | Maximum simultaneously live particles. Rounded, then bounded by `kMaxParticleCount`. |
| 2 | `birth_rate` | Float Slider, HUNDREDTHS, 0…1000000, default 30 | `Settings::birth_rate` | Births per second on the effect clock. |
| 3 | `seed` | Float Slider, INTEGER, 0…2147483647, default 1 | `Settings::seed` | Seeds each particle's independent shape and velocity streams. |
| 4 | `particle_lifetime` | Float Slider, THOUSANDTHS, 0…1000000, default 2 | `Settings::particle_lifetime_seconds` | Seconds, half-open: a particle is gone once `age == lifetime`. |
| 5 | `emitter_shape` | Popup, 4 choices, default Point (AE value 1) | `Settings::emitter_shape` | AE popup values are one-based; index = value − 1 → 0-based `EmitterShape`. Point, Box, Sphere, and Disc have distinct seeded birth distributions. |
| 6 | `emitter_origin` | 3D Point, default (50, 50, 50) | `Settings::emitter_origin` | Position control with comp-view picking. Host value conversion is documented under "Emitter Origin semantics" below. |
| 7 | `velocity_x` | Float Slider, HUNDREDTHS, ±1000 (slider ±20), default 0 | `Settings::velocity.x` | Layer heights per second, positive right. |
| 8 | `velocity_y` | Float Slider, HUNDREDTHS, ±1000 (slider ±20), default 0.3 | `Settings::velocity.y` | Layer heights per second, positive up. The non-zero default keeps a fresh instance visibly alive. |
| 9 | `velocity_z` | Float Slider, HUNDREDTHS, ±1000 (slider ±20), default 0 | `Settings::velocity.z` | Layer heights per second, positive toward the viewer. Reserved for depth; the 2D compositor ignores it. |
| 10 | `particle_size` | Float Slider, HUNDREDTHS, 0…100000, default 8 | `Settings::particle_size` | Sprite diameter in full-resolution layer pixels. 0 renders nothing. |
| 11 | `opacity` | Float Slider, THOUSANDTHS, 0…1, default 1 | `Settings::opacity` | Peak alpha at the sprite centre. |
| 12 | `emitter_size` | Float Slider, THOUSANDTHS, 0…10 (slider 0…1), default 0.05 | `Settings::emitter_size` | Cube edge for Box, diameter for Sphere/Disc, in layer heights. Ignored by Point. |
| 13 | `velocity_spread` | Float Slider, HUNDREDTHS, 0…100 (slider 0…1), default 0.15 | `Settings::velocity_spread` | Per-axis uniform jitter added to each particle's velocity, in layer heights per second. This is what makes a steady emitter animate (see below) and what gives `seed` a visible effect. |

| 17 | `gravity_x` | Float Slider, HUNDREDTHS, ±1000 (slider ±20), default 0 | `Settings::gravity.x` | Force node. Layer heights per second squared, positive right. |
| 18 | `gravity_y` | Float Slider, HUNDREDTHS, ±1000 (slider ±20), default 0 | `Settings::gravity.y` | Force node. Negative pulls down. |
| 19 | `gravity_z` | Float Slider, HUNDREDTHS, ±1000 (slider ±20), default 0 | `Settings::gravity.z` | Force node. Reserved for depth; the 2D compositor ignores Z. |
| 20 | `linear_drag` | Float Slider, THOUSANDTHS, 0…100 (slider 0…10), default 0 | `Settings::linear_drag` | Force node. Inverse seconds, solved in closed form with gravity. |
| 21 | `color_start` | Color, default white | `Settings::color_start` | Appearance node birth color. AE delivers 8-bit channels; the adapter maps `channel / 255` to working-space 0..1 (no color-space conversion, ADR 0005). Alpha comes from Opacity. |
| 22 | `color_end` | Color, default white | `Settings::color_end` | Appearance node color as age approaches lifetime. Equal to Color Start by default, so defaults change nothing. |
| 23 | `particle_size_end` | Float Slider, HUNDREDTHS, 0…100000, default 8 | `Settings::particle_size_end` | Appearance node size reached at the end of life; `particle_size` is the birth value. Equal by default. |
| 24 | `opacity_end` | Float Slider, THOUSANDTHS, 0…1, default 1 | `Settings::opacity_end` | Appearance node opacity reached at the end of life; `opacity` is the birth value. Equal by default. |

IDs are append-only; the UI order currently follows ID order, so the emitter controls (12, 13) and the
force/appearance controls (17-24) sit after the graph/system parameters until M3-03 adds AE parameter
groups. Parameter index 0 is AE's implicit input layer, so the effect registers 25 AE parameters: one
input, twenty-one manifest controls, graph data (14), source mode (15) and capture action (16). Node
Graph Data is hidden from the Effect Controls panel.

All bound controls are registered with `PF_ParamFlag_SUPERVISE`. In `AE Controls` mode a change is
ignored by the graph path and the render simply samples the new value. In `Node Graph` mode the effect
rebuilds the canonical graph from the delivered values in the same user-change transaction
(`user_changed_param` → `sync_graph_from_controls`), which is also the surface the CEP panel writes to
(ADR 0009). The delivered `params[]` array is the authoritative source during that callback: a
`PF_CHECKOUT_PARAM` can still return the pre-edit value, so the sync path never uses it for the edited
control.

## Emitter distributions and per-particle variation (M3-01)

Each particle's birth offset is drawn from `emitter_shape` within `emitter_size`
(`core::Random.hpp`, stream purposes `position_x/y/z`):

| Shape | Distribution |
|---|---|
| Point | exactly at `emitter_origin` |
| Box | uniform inside a cube of `emitter_size` edge length, centred on the origin |
| Sphere | uniform inside a sphere of `emitter_size` diameter (cube-root radius, isotropic direction) |
| Disc | uniform over a disc of `emitter_size` diameter in the emitter plane (square-root radius) |

Each particle also gets an independent velocity jitter of ±`velocity_spread` per axis (purposes
`velocity_x/y/z`), applied once at birth and kept for the particle's whole life. M3-01 remains a
2D sprite renderer: Z affects the evaluated particle state but does not yet affect depth or
occlusion.

**Why this exists — the frozen steady state.** With identical particles, a continuously emitting
trail looks motionless on playback even though every frame is computed correctly: births keep
replacing the particles that leave, so the covered span and spacing stay constant. The internal
particles do move, but the picture does not. Per-particle variation breaks that symmetry, and the
same mechanism finally makes `seed` observable: a different seed re-rolls the offsets, velocities,
and therefore the pixels.

## Emitter Origin semantics

AE point controls deliver **absolute layer pixels** in destination-layer space: the origin is the
layer's top-left, x grows right and y grows down (AE C++ SDK guide, "Parameters"; the same source
notes AE corrects the value for origin shifts introduced by upstream effects). Only the *default*
follows the older percentage convention — the `PF_Point3DDef` header comment says to "use 50 for
halfway" — so the registered default (50, 50, 50) means "layer centre".

Revision 1 of this bridge treated the delivered value as a percentage, which pushed the emitter
several layer heights off-canvas at the default "centre" and made the effect render nothing at
all. The conversion now normalizes to layer pixels first and then to the canonical world space
(ADR 0003):

```text
px      = host_point_component_to_layer_pixels(raw, layer_extent)
world.x = (px.x / layer_width - 0.5) * (layer_width * par / layer_height)
world.y = 0.5 - px.y / layer_height
world.z = px.z / layer_height - 0.5
```

The normalization ladder exists because the SDK describes point values inconsistently; it prefers
the documented reading and only falls back when a value cannot be a pixel position:

| Raw magnitude | Interpretation | Why |
|---|---|---|
| ≤ 4 × layer extent | layer pixels (documented delivery) | Plausible pixel positions, including the converted default |
| > 4 × layer extent | legacy percentage: `raw / 100 × layer_extent` | On a small layer a "50" cannot be a pixel position |
| > 1e5 | fixed-point scaled: `raw / 65536` first | Percent × 65536, i.e. what a `PF_Fixed` delivery looks like |

The Options readout prints the raw host value, the interpreted pixels, and the resulting world
position, so the host's real behaviour can be recorded rather than guessed. **The ladder is a
compatibility shim:** once a host pass confirms which delivery AE actually uses, delete the unused
branches and reduce this section to the confirmed fact.

`emitter_origin` is clamped to ±`kMaxEmitterOffset` (100 layer heights) by `validate_settings`, and
velocities are clamped to ±`kMaxVelocity` (1000 layer heights per second); both raise a
`ValidationNotice`.

## Popup reconciliation

`PF_ADD_POPUP` values run 1…num_choices while `EmitterShape` is zero-based, so the adapter maps
`EmitterShape = value - 1` through `core::emitter_shape_from_index()`, which also collapses
out-of-range values onto `point`. The manifest's `default: 1` means "Point, the first entry".

## Deterministic emission rules (M2)

- The particle clock is anchored at host time 0: negative comp time renders nothing, and at
  exactly t = 0 only slot 0 exists. A "no visible motion" report at comp start is expected
  behaviour, not a defect.
- Slot `k` is born at `k / birth_rate` seconds; a slot whose age equals the lifetime is gone.
- At most `particle_count` slots are alive at once. When the cap is exceeded the **newest**
  slots survive, so ids stay ascending and the ordering is reproducible.
- Position is closed form: with `k = linear_drag`, `g = gravity`, `v0 = velocity` and age `a`,
  `displacement = v0 * (1 - exp(-k a)) / k + g * (a - (1 - exp(-k a)) / k) / k`, and `k = 0`
  degenerates to `v0 * a + g * a² / 2`. Any absolute time can be evaluated without stepping, so
  out-of-order and repeated requests agree (ADR 0002). The small-`k a` branch uses a series so the
  `a - (1 - exp(-k a)) / k` term does not lose precision by cancellation.
- Size, opacity, and color interpolate linearly over `age / lifetime` when the appearance stage is
  active. `age == lifetime` is never visible, so the end values are approached, not reached, by a
  live particle.

## Options readout (diagnostic)

The effect sets `PF_OutFlag_I_DO_DIALOG`, so AE shows an `Options` button. Clicking it runs a
read-only readout (`ae_plugin/Diagnostics.cpp`) that prints exactly what the code receives:

```text
SF 0.1.0 src AE g4n3e live 59
layer 3840x2160 ds 1/1 ref 3840x2160 grid 3840x2160
org host 1920,1080,1080 px 1920,1080,1080
org wld l 0.000,0.000,0.000 r 0.000,0.000,0.000
t 1.000s vel 0.00,0.30,0.00
cnt 1000 rate 100.00 seed 1 life 2.000
```

- `src`: `AE` when the flat controls drive the render, `NG` when the stored graph does. `gn`/`ge` are
  the evaluated node and edge counts; `live` is the particle count that evaluation produced.
- `layer` / `ds`: what the host reports for this call, with the preview downsample factor.
- `ref` / `grid`: what the **last rendered frame** actually used. `ref` is the reference the point
  conversion divided by; `grid` is the pixel grid the core mapped world space onto. The render phase
  records both (`record_render_geometry`), because this readout cannot call `checkout_layer` itself.
  This pair is the evidence D-05 was missing: with it, one click at Full and one at Quarter settle
  whether a point control is delivered in full-resolution or preview-sized pixels.
- `org host` / `px`: the raw host value and the layer pixels it was interpreted as.
- `org wld l … r …`: the world position this readout computed from the sizes above (`l`) next to the
  one the last frame computed from its reference (`r`). At Full resolution they agree; at a reduced
  preview resolution the frame used `r`.
- `grav`/`col` lines appear only when those values differ from their defaults, and `shape`/`size`/
  `not` can fall off the end: `PF_OutData::return_msg` holds 255 characters and the writer drops what
  does not fit. Those controls are visible in the Effect Controls window; the geometry above is not.

### Resolving the point-unit question (D-05)

For a reliable measurement, use a fresh comp with one Starfield instance and set `Control Source` to
`AE Controls` (the default for a new effect). The diagnostic keeps one process-wide last-render
geometry record, so another Starfield instance can overwrite `ref/grid`; in `Node Graph` mode the
rendered origin comes from stored graph values, while `org host/px` describes the AE point control.
If the readout says `src NG`, switch back to `AE Controls` before taking these measurements.

1. Put the playhead at t ≥ 1 s, render a frame with the Composition panel at **Full**, press `Options`,
   and keep the text.
2. Switch to **Quarter**, let one frame render, press `Options` again.
3. Compare the two readouts:

| What the second readout shows | What it means | What to do |
|---|---|---|
| `ref` still equals the layer size, `grid` shrinks, `org host` unchanged | The host delivers point controls in **full-resolution pixels**; the frame divides by `ref` and `org wld r` is the position that was rendered. The `l` value is the fallback path and may differ. | Record it, retire the fallback path, and reduce the ladder below to this delivery |
| `ref 0x0`, or `org host` shrinks with the preview | No usable reference was handed over, or the host delivered preview-sized values; the conversion fell back to `layer` and the origin moves with the preview resolution | Record it, then decide the divisor rule from the numbers instead of the SDK note |

The old readout could not answer this: its origin lines came after longer lines and were cut off by the
255-character buffer, so every earlier "the fix works" statement about the emitter origin was a code
reading, never a measurement.

- `ds` is reported for context only; render geometry comes from the observed worlds (ADR 0005) and the
  reference above.

## Motion or position looks wrong: triage

| Readout | Meaning | Next step |
|---|---|---|
| `t 0.000s … live 1` | Comp start. Only slot 0 exists by construction | Scrub to t ≥ 1 s, or raise Birth Rate |
| `org px` far outside the layer | Point-unit mismatch | Compare `host` and `px` against the ladder above and record the host's real delivery in `docs/compatibility-matrix.md` |
| `org wld l` differs from `org wld r` at reduced resolution | The readout's fallback path disagrees with the frame's reference-based conversion; the frame used `r` | Expected until the fallback path is retired (D-05); always read `ref`/`grid` before believing `org wld l` |
| `ref 0x0` | The host handed over no full-resolution reference, so the conversion fell back to `layer` | Record the two-click readout from the section above; the origin then depends on the preview resolution |
| `vel` non-zero but the picture is static | Mapping or compositing ignored the position | Reproduce in `tests/core_tests.cpp`, which pins the origin offset, the point conversion, and the half-resolution mapping |
| `layer 0x0` | Host geometry was not available | The render phase refuses to guess and reports `internal_failure` |

## Validation chain

1. The adapter converts host doubles without clamping, so non-finite host values reach the
   validator; counts and seeds are integral and are bounded in the adapter itself.
2. `core::validate_settings` replaces non-finite values with documented defaults and clamps
   every field to the manifest range, recording `ValidationNotice` entries.
3. The renderer enforces frame geometry, allocation, and bounded-work limits and returns typed
   errors instead of blocking the host.
