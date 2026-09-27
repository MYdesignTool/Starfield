# Reference parameter map (observed behavior)

Source: `tools/dump_effect_parameters.jsx` run in AE 23.5 on one solid layer carrying every
module of the reference product. Parameter names, match names, nesting and the values of a
freshly applied instance are **observed behavior**, which ADR 0010 allows as a spec input.
The raw dump stays local in `artifacts/reference/stardust_effect_parameters.txt` (ignored by
Git, same rule as the SDK); this file records only what we derive from it.

## Module inventory

The reference is **one main effect plus nineteen control effects plus a panel**, all separate
`.aex` plugins. Several control modules share one match name, so an instance is identified by
its `uid` parameter and the main effect owns the mapping:

| Effect (as registered) | matchName |
|---|---|
| Stardust | `SC Stardust_effect_0_6` |
| Emitter | `SC_Strdst_cntrls_emitter_1` |
| Auxiliary | `SC_Strdst_cntrls_emitter_1` (same match name as Emitter) |
| Particle | `SC_Strdst_cntrls_particle_1` |
| Turbulence | `SC_Strdst_cntrls_air_1` |
| Field | `SC_Strdst_cntrls_space_1` |
| Motion | `SC_Strdst_cntrls_space_1` |
| Replica, Transform | `SC_Strdst_cntrls_transform_1` |
| Shading | `SC_Strdst_cntrls_lights_1` |
| Model | `SC_Strdst_cntrls_model_1` |
| Material | `SC_Strdst_cntrls_material_1` |
| Deform | `SC_Strdst_cntrls_modeldeform_1` |
| Group | `SC_Strdst_cntrls_uigroups_1` |
| Clone | `SC_Strdst_cntrls_splitter_1` |
| Physical | `SC_Strdst_cntrls_phys_params_1` |
| Source | `SC_Strdst_cntrls_override_1` |
| Forces | `SC_Strdst_cntrls_phys_forces_1` |
| Volumetric | `SC_Strdst_cntrls_posteffect_1` |
| Volume | `SC_Strdst_cntrls_vol_params_1` |

Every module starts with the same private preamble (`uid`, `undo data`, `undo ui`, two unnamed
slots, `Dummy` = 100) and ends with AE's built-in `Compositing Options`. Those are plumbing, not
parameters to copy.

The main effect's visible surface is a launcher: `Panel: Click To Open`, `Presets: Browse`,
`Switches` (Shy/Solo), `Time Remapping` (On/Off, Time), `Render Settings` (Quality Settings,
Render Output, `Preview` {On/Off, Particle chance}, `3D Preview`, `Helpers` {Draw Helpers, Color,
Opacity}, `Environment` {Environment Layer, Color, Orient X/Y/Z, Mix Lights Shading, Visible}).
Our equivalent is the `Render` topic plus the panel.

## Emitter module: parameters we can map today

| Reference parameter | Observed default | Ours | Status |
|---|---|---|---|
| `Type:` (popup) | 1 | `Type` (Point/Box/Sphere/Disc) | Name and position match; their choice list is longer (grid, path, text, layer, model emitters) |
| `Emitting:` (popup) | 1 | — | **Missing**: emission mode (continuous/burst) |
| `Starting With` | — | — | **Missing** |
| `Particles Per Second` | **100** | `Particles Per Second` | Aligned (revision 6 changed our default 30 → 100) |
| `Origin XY` | **[1920, 1080]** | `Origin` (one 3D point) | Unit question answered: see D-05 below. Model differs: theirs is a 2D point plus a separate `Origin Z` |
| `Origin Z` | 0 | `Origin` (same control) | Split pending |
| `Origin Time Sample:` (popup) | 1 | — | **Missing** |
| `Speed` | **100** | `Velocity X/Y/Z` (three axes) | **Model differs**: theirs is a scalar emission speed; `Speed X/Y/Z` in their Particle module are per-particle rotation speeds, so naming our axes "Speed" was wrong and is reverted to Velocity |
| `Speed Random` | 0 | `Speed Random` | Aligned in name; theirs randomizes the scalar speed, ours jitters each axis |
| `Speed Over Life` | group | — | **Missing** (split into Size/Opacity over life today) |
| `Inertia` | 0 | `Linear Drag` | Same idea, different name/units (theirs percent-like, ours inverse seconds); rename pending confirmation |
| `Size X/Y/Z` | 100 / 100 / 100 | `Emitter Size` | Model differs: theirs is a per-axis emitter box in percent, ours a single cube/diameter in layer heights |
| `Light Size`, `Angle X/Y/Z`, `Direction:`, `Orient X/Y/Z`, `Direction Span` | 0/0/0, 1, 0/0/0, 60 | — | **Missing**: the direction/cone model that replaces our three velocity sliders |
| `Auxiliary`, `Ring Particles`, `Grid/Path/Layer/Object Properties`, `Time Offset` | — | — | **Missing** (further emitter types) |
| `Random Seed` | **1000** | `Random Seed` | Ours sits in Render and defaults to 1 |

## Particle module: parameters we can map today

| Reference parameter | Observed default | Ours | Status |
|---|---|---|---|
| `Shape:` (popup) | 1 | — | **Missing** (sprite/texture shape) |
| `Life (Seconds)` | **2** | `Lifetime` | Aligned in meaning and default |
| `Life Random` | 0 | — | **Missing** |
| `Size (Pixels)` / `Size Y (Pixels)` | **10** / 10 | `Size` | Default aligned to 10 in this change; their Y size allows non-square sprites, ours are round |
| `Size Random`, `Use Texture Ratio`, `Ignore Perspective` | 0, 1, 0 | — | **Missing** (size random is a planned addition) |
| `Opacity` (percent) | **100** | `Opacity` (0..1) | Value matches; unit differs — theirs is percent, ours normalized. Aligning to percent is queued |
| `Opacity Random` | 0 | — | **Missing** |
| `Particle Color:` / `Color` / `Color Gradient` / `Color Use:` | 1, [1,1,1,1], —, 1 | `Color Start` / `Color End` | Ours are two endpoints; theirs is one color plus a gradient and a usage mode |
| `Particle Feather`, `Transfer Mode:`, `Up Axis:` | 0, 1, 3 | — | **Missing** |
| `Over Life` → `Size`, `Opacity` | groups | `Size Over Life`, `Opacity Over Life` | Ours are linear endpoints; theirs are curves |
| `Rotation Properties` (Orient To, Angle X/Y/Z, Speed X/Y/Z, Rotation Over Life, Anchor, Limit To 2D) | — | — | **Missing** |
| `Texture`, `Path`, `Shadow`, `Cloud Properties` | — | — | **Missing** |
| `Use Model(s):`, `Shift Seed`, `Birth Chance` | 1, 0, 100 | — | **Missing** |

## Findings that change our backlog

- **D-05 is answered for point controls.** The reference's own `Origin XY` reads `[1920, 1080]`
  in a 3840×2160 composition, i.e. **absolute layer pixels**, and its `Origin Z` reads 0. That is
  the delivery branch our adapter already prefers; the percentage and fixed-point rungs exist
  only because no host readout had confirmed it. Our own Options readout must still print the
  value our control receives, then the unused rungs can go.
- **Speed is a scalar plus a direction model** (`Direction`, `Angle X/Y/Z`, `Direction Span`,
  `Orient X/Y/Z`). Our three velocity sliders are the deviation to remove first, because it is
  the most visible behavioural difference in the emitter section.
- **Opacity is percent** in the reference UI. Ours is 0..1; aligning means range change plus a
  `/100` in the adapter, and it also affects `Opacity Over Life`.
- **Over Life uses curves**, not endpoint pairs. Our endpoint approximation should be replaced by
  a curve parameter when we add one (AE has no built-in gradient parameter, so this needs a
  custom UI or a keyframed slider pair).
- **Emitter size is per-axis and percent-based** (`Size X/Y/Z`), not a single cube edge.

## How to regenerate

1. Apply one instance of every module to a single layer in a fresh composition.
2. `File > Scripts > Run Script File...` → `tools/dump_effect_parameters.jsx`.
3. Keep the output local (`artifacts/reference/`), never commit it.
