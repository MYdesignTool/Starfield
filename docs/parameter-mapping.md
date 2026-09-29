# Parameter bridge: schema → AE control → core settings

Task: M2-02, revised by manifest revision 7. `schema/parameters.json` owns the IDs,
labels, ranges, and defaults; `ae_plugin/Parameters.cpp` owns the host controls and
the conversion; the core only ever sees `starfield::core::Settings` after
`validate_settings`. One conversion path (`settings_from_controls`) serves both the
render checkout and the supervised panel edit, so the two cannot drift apart.

## Manifest revision 7: project-saved node layout (current)

Revision 7 appends hidden, non-animated float sliders for the four fixed node positions.
These values are UI metadata owned by the effect instance: AE saves them with the project,
copies them with the effect, and records panel moves in one undo group. They are not
supervised and do not feed `Settings` or graph evaluation. CEP reads and writes them through
the public scripting DOM; AE 2023 read/write, save/reopen, duplicate, and undo qualification
is still open (ADR 0014).

| ID / index | key | AE stream | Default | Range |
|---|---|---|---:|---:|
| 33 | `layout_emitter_x` | Layout Emitter X | 180 | ±1,000,000,000 canvas units |
| 34 | `layout_emitter_y` | Layout Emitter Y | 22 | ±1,000,000,000 canvas units |
| 35 | `layout_force_x` | Layout Force X | 180 | ±1,000,000,000 canvas units |
| 36 | `layout_force_y` | Layout Force Y | 190 | ±1,000,000,000 canvas units |
| 37 | `layout_appearance_x` | Layout Appearance X | 180 | ±1,000,000,000 canvas units |
| 38 | `layout_appearance_y` | Layout Appearance Y | 358 | ±1,000,000,000 canvas units |
| 39 | `layout_output_x` | Layout Output X | 180 | ±1,000,000,000 canvas units |
| 40 | `layout_output_y` | Layout Output Y | 526 | ±1,000,000,000 canvas units |

## Manifest revision 6: topic grouping and renumbering (previous)

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

AE point controls deliver absolute destination-layer pixel positions, but AE 2023.5.0 Build 52
also scales those values with the preview resolution. The user's Full/Quarter Options readouts
show the same center as `1920,1080,1080` at `ds 1/1` and `480,270,270` at `ds 1/4`, while `ref`
remains `3840x2160` and `grid` changes from `3840x2160` to `960x540`. Therefore the adapter restores
full-resolution pixels with the reciprocal preview factor before converting to world space:

```text
px.x    = raw.x * downsample_x.den / downsample_x.num
px.y,z  = raw.y,z * downsample_y.den / downsample_y.num
world.x = (px.x / layer_width - 0.5) * (layer_width * par / layer_height)
world.y = 0.5 - px.y / layer_height
world.z = px.z / layer_height - 0.5
```

Invalid or unreported rational factors fall back to 1:1. X and Y are normalized separately;
Z follows the vertical factor. This conversion is shared by AE Controls rendering, Capture Current
Controls, Node Graph control synchronization, and the Options diagnostic. The core preserves valid
off-layer pixel positions as-is; it no longer guesses that large positions are percentages or
fixed-point values. This host observation is qualified only for AE 2023.5.0 Build 52 until another
AE 2023 build supplies equivalent readouts.

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
Core: current DLL
SF AE g4/3 live59
L3840x2160 ds1/1,1/1 ref3840x2160 grid3840x2160
shape0 esz0.050 vspr0.15 sz10.00 not0
org1920,1080,1080 px1920,1080,1080
world 0.000,0.000,0.000
t1.000 vel0.00,0.30,0.00
cnt1000 rate100.00 seed1 life2.000
```

- `AE` means flat controls drive the render; `NG` means the stored graph does. The two numbers
  after `g` are evaluated node and edge counts; `live` is the particle count.
- `L` / `ds`: layer size and horizontal/vertical preview factors reported by this call.
- `ref` / `grid`: what the **last rendered frame** actually used. `ref` is the reference the point
  conversion divided by; `grid` is the pixel grid the core mapped world space onto. The render phase
  records both (`record_render_geometry`), because this readout cannot call `checkout_layer` itself.
  The user's AE 2023.5 Build 52 screenshots confirmed `ref` stays full-size while `grid` follows
  preview resolution.
- `org` / `px`: the raw host value and the full-resolution layer pixels after reversing the
  preview factor. At Quarter, `480,270,270` must read back as `1920,1080,1080` in `px`.
- `world` is the resulting world coordinate. When the current control/graph value differs from
  the last frame's reference conversion, it expands to `world l… r…` to show both values.
- `shape` is the zero-based Point/Box/Sphere/Disc index; `esz`, `vspr`, `sz`, and `not` are emitter
  size, velocity spread, initial particle size, and validation notice count. This line now precedes
  the origin and optional controls, because AE caps the entire message at 255 characters.
- `grav`/`col` appear only when values differ from defaults. They can displace the trailing
  time/count line within AE's message limit; the Effect Controls window has those values.

### D-05 host measurement and regression

The owner supplied Full and Quarter readouts from AE **23.5.0 Build 52** with one effect instance and
`src AE`. For a repeatable host measurement, select Full/Quarter from the **Composition viewer's
bottom resolution menu**; the right-side Preview panel has a separate Resolution control, and its
Quarter setting alone left the Composition viewer at Full (`ds 1/1`) in the 2026-09-28 check. Full
reported `ds 1/1`, `ref/grid 3840x2160`, and origin `1920,1080,1080`. Quarter reported
`ds 1/4`, `ref 3840x2160`, `grid 960x540`, and origin `480,270,270`. The old candidate interpreted the
Quarter values as full-resolution pixels and reported world `(-0.667,0.375,-0.375)`, exactly the
observed offset. The source fix now applies the reciprocal `1/4` preview factor per axis before using
the full-resolution reference. The adapter regression pins Full, Quarter, and anisotropic factors;
Capture and Node Graph synchronization also exercise this shared conversion. AE 2023.5.0 Build 52
subsequently confirmed Full, Half, Third, and Quarter normalization; the 2026-09-29 Third-resolution
readout above showed `org952,487,360 px2856,1460,1080` for an off-center point.

## Motion or position looks wrong: triage

| Readout | Meaning | Next step |
|---|---|---|
| `t 0.000s … live 1` | Comp start. Only slot 0 exists by construction | Scrub to t ≥ 1 s, or raise Birth Rate |
| `org px` differs from the intended location | Point conversion or preview factor mismatch | Compare raw `host`, normalized `px`, and both `ds` axes against the measured AE delivery |
| `world l` differs from `world r` | The diagnostic's current settings conversion disagrees with the last rendered frame | Confirm one effect instance and `SF AE`; then report the complete readout |
| `ref 0x0` | The host handed over no full-resolution reference, so conversion falls back to the input dimensions | Record the complete readout; `ds` still normalizes the observed preview-scaled point values |
| `vel` non-zero but the picture is static | Mapping or compositing ignored the position | Reproduce in `tests/core_tests.cpp`, which pins the origin offset, the point conversion, and the half-resolution mapping |
| `layer 0x0` | Host geometry was not available | The render phase refuses to guess and reports `internal_failure` |

## Validation chain

1. The adapter converts host doubles without clamping, so non-finite host values reach the
   validator; counts and seeds are integral and are bounded in the adapter itself.
2. `core::validate_settings` replaces non-finite values with documented defaults and clamps
   every field to the manifest range, recording `ValidationNotice` entries.
3. The renderer enforces frame geometry, allocation, and bounded-work limits and returns typed
   errors instead of blocking the host.
