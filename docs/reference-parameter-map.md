# Build34 native gradient pixel-format correction

Owner build33 gradient expansion triggers repeated Unsupported Pixel Format
warnings. Build34 queries Drawbot BGRA/ARGB support/preferences, uses matching
opaque 32-bit pixels and retains path drawing if image APIs are unavailable.
Context-local guards stop failure/reentrant retries. No supported parameter,
schema or layout changes; AE warning-free editor qualification remains pending.
See ADR0027 and build-matrix.md.

## Build33 native UI correction

Build32 owner testing found gradient seams, adding-stop compile failure and
internal metadata visible on fresh Emitter/Force. Build33 restores zero standard/
hidden control dimensions, draws one opaque gradient bitmap, submits one complete
bank without intermediate leaf supervision, and uses actual AE HIDDEN stream
flags for mode/shape visibility. No additional schema/layout change from build32.
Host confirmation is pending; these corrections do not establish reference parity.
See ADR0027 and build-matrix.md.

## Build32 development surface

The owner accepted build31 for main; main/origin main now contain 8857303.
New work is on codex/m3-07-particle-gradient. Particle uses schema5/base81;
create fresh development Particle effects/graphs. See ADR0027. Older checkpoints
below retain their historical schema and qualification statements.

The supplied 2026-10-04 screenshot owns the visible control organization:

| Location | Supported controls in order |
|---|---|
| Root | Shape, Life (Seconds), Life Random |
| Particle Properties | Size (Pixels), rectangle Size Y (Pixels), Size Random, Opacity, Opacity Random, Particle Color, Color, Color Gradient, Particle Feather, Up Axis |
| Over Life | Size Over Life, Opacity Over Life; existing curves remain editable in CEP |
| Rotation | Orient To, Angle X/Y/Z, Angle Random, Rotation Speed X/Y/Z, Rotation Speed Random, Limit to 2D |

Life defaults to 2 seconds, supports typed values up to 10000, and uses tenths
with a 0..10 normal slider range. Random/opacity controls retain percentage units;
angles retain native AE rotation controls. Existing supported rendering meanings
are unchanged; labels and groups alone do not establish numerical parity.

Native Color Gradient displays 2..8 RGB stops with linear interpolation. Click
the bar to add, drag an interior marker, double-click a marker for the AE picker,
and Alt-click or Delete/Backspace to remove an interior stop. Endpoints stay fixed.
Flip, Copy, Paste and White/Fire/Spectrum presets operate on the complete bank.
CEP has matching complete-gradient actions; the two editors have separate session
clipboards. AE Color remains visible as in the screenshot; Particle Color selects
solid, over-life, random or looping evaluation. Constant gradient stop banks use
native change flags and graph publication; other authored controls remain animated.

Texture ratio, Ignore Perspective, source/model shapes, gradient alpha and new
Transfer Mode behavior require their own render contracts and remain open work.
No placeholder switches are added for those unsupported families. Native drawing
commands were inspected locally; actual AE UI/undo/reopen/render remains an owner
qualification gate. Official behavior reference: https://superluminal.tv/user-guide.

## Accepted build31 checkpoint

Appearance is removed by owner request. Emitter/Auxiliary, Particle, Force and
Output are the current nodes; Particle owns color/gradient, size/opacity and
Over Life. Recreate graphs containing Appearance. Remaining native disk IDs,
Particle schema4/base75, Emitter schema6/base31, Force schema2/base27 and CoreABI3
are paired; the unused Color End disk ID207 definition is retained.
Main layout35/36 now has Particle labels; main manifest24 and public IDs stay.
Reopen CEP for native-idle-31. Connection/layout edits preserve authored controls
and keys. StarfieldHost.aex adds bounded read-only initialization on active-comp
UI idle, requiring real AE2023 no-Options qualification. Older sections below are
historical checkpoints, not the current node inventory. See ADR0026.

# Reference parameter map (observed behavior)

## Build 22 Particle Color / native rotation slice — 2026-10-03

The observed inventory defines Particle Color, Color and Color Gradient, rather
than two visible endpoints. The official guide documents Solid color, Color over
life, Random from gradient and Loop from grad; these are now implemented with
2–8 independently saved stops. Source/path color modes await source contracts.
Emitter Angle X/Y/Z use PF_Param_ANGLE controls at the owner's request. Emission,
birth appearance and lived Force timing are documented in temporal-parameter-audit.md.
This does not claim full Stardust feature or numerical kernel parity.


## Build 14 ordinary Force/main slice — 2026-10-02

Scalar Gravity/random, separate Wind X/Y/Z, Spin/frequency/resist/delay and Air
Density follow the observed ordinary air module's names/order/zero defaults.
Wind/Spin share an independent saved percentage curve. This is ordinary Force,
distinct from Physical Forces. Numeric trajectory formulas are independent and
explicit in ADR 0021; exact reference behavior still needs comparison. Main adds
functional Time Remapping and Preview/Particle chance; Max Particles defaults to
the owner's 1000000. See reference-main-force-comparison.md for remaining main
and Force capabilities. Historical tables below describe earlier checkpoints.

## Build 13 Emitter/Auxiliary slice — 2026-10-02

The local observed parameter inventory and owner's screenshot confirm Emitter and
Auxiliary share the reference Emitter module. Implement independent shared source
infrastructure in StarfieldEmitter.aex, with a separate Auxiliary creation action
and a Default/Auxiliary source-mode popup. This popup is our development mode
selector; the reference's Emitting options have not been characterized. Do not
claim their semantics match merely because the label is the same.

Implemented: Emit Chance (100%), Emit Life Start/End (0/100%), inheritance of
instantaneous velocity, size, opacity and color, native project storage, parent
particle input and independent child lifetime. The core uses deterministic chance
per parent identity and comp-zero emission ticks. Exact reference distributions,
Origin Time Sample, Speed Over Life, Inertia, Orient and Time Offset remain open.
Names/ordering alone do not establish behavior parity. Random Seed stays last.
No decompiled kernels or private identities are reused.

## Build 12 implemented control alignment — 2026-10-02

The local observed dump confirms Life (Seconds)=2, Size (Pixels)=10,
Opacity=100, Particles Per Second=100, Origin XY at layer center, Origin Z=0,
Speed=100, Speed Random=0, Size X/Y/Z=100, Angle X/Y/Z=0, Direction Span=60
and emitter Random Seed=1000. Build 12 adopts these implemented control names,
splits and defaults. Life max 10000/CEP step 0.1 comes from owner direction,
not an inferred reference limit. Native/CEP speed is pixels/second, randomness
and opacity display percent. Scalar speed's random percentage persists even
at zero speed. Reference distribution/interpolation kernels are not copied
or claimed equivalent. Per-axis velocity and birth appearance bootstrap are
hidden internal controls; Particle exposes size/opacity/life.

Shape-specific Disc controls, the current vector gravity/linear-drag kernel,
Life Random, non-square Size Y, richer gradients and additional reference
modules still have distinct incomplete contracts. The current Gravity and
Linear Drag labels are not replaced with reference names for different models.
This slice does not claim full reference parameter or behavior coverage.

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
| `Size X/Y/Z` | 100 / 100 / 100 | `Size X/Y/Z` (direct layer pixels) | The owner specifies direct dimensions, not percentages; current core uses these on Box and Sphere |
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
| `Size Random`, `Use Texture Ratio`, `Ignore Perspective` | 0, 1, 0 | `Size Random` (Particle node / AE index 86) | Deterministic seeded per-particle variation is implemented; exact distribution parity and AE host behavior remain to be qualified. Texture ratio and perspective options are missing. |
| `Opacity` (percent) | **100** | `Opacity` (0..1) | Value matches; unit differs — theirs is percent, ours normalized. Aligning to percent is queued |
| `Opacity Random` | 0 | `Opacity Random` (Particle node / AE index 87) | Independent deterministic seeded variation is implemented; exact reference distribution and AE host behavior remain to be qualified. |
| `Particle Color:` / `Color` / `Color Gradient` / `Color Use:` | 1, [1,1,1,1], —, 1 | `Color Start` / `Color End` | Ours are two endpoints; theirs is one color plus a gradient and a usage mode |
| `Particle Feather`, `Transfer Mode:`, `Up Axis:` | 0, 1, 3 | — | **Missing** |
| `Over Life` → `Size`, `Opacity` | groups | Piecewise-linear Size/Opacity curves in the CEP Particle inspector (P-02C) | Core evaluation and project parameter streams are implemented; visual rendering, undo, and save/reopen still need AE 2023 qualification |
| `Rotation Properties` (Orient To, Angle X/Y/Z, Speed X/Y/Z, Rotation Over Life, Anchor, Limit To 2D) | — | — | **Missing** |
| `Texture`, `Path`, `Shadow`, `Cloud Properties` | — | — | **Missing** |
| `Use Model(s):`, `Shift Seed`, `Birth Chance` | 1, 0, 100 | — | **Missing** |

## Findings that change our backlog

- **D-05 is answered for our AE 2023 target by the owner's 23.5.0 Build 52 readouts.** The reference's
  own `Origin XY` also reads `[1920,1080]` in a 3840×2160 composition, but that alone did not prove
  how AE scales our control at reduced preview. The paired Full/Quarter readouts show our control
  changes from `[1920,1080,1080]` to `[480,270,270]` as the preview factor changes from 1/1 to 1/4.
  The adapter now reverses that factor; the magnitude-based percentage/fixed-point guesses are gone.
- **Speed is a scalar plus a direction model** (`Direction`, `Angle X/Y/Z`, `Direction Span`,
  `Orient X/Y/Z`). Our three velocity sliders are the deviation to remove first, because it is
  the most visible behavioural difference in the emitter section.
- **Opacity is percent** in the reference UI. Ours is 0..1; aligning means range change plus a
  `/100` in the adapter, and it also affects `Opacity Over Life`.
- **Over Life uses curves**, not endpoint pairs. P-02C now authors bounded piecewise-linear
  curves in CEP and evaluates them in Particle/Appearance; the AE visual and project lifecycle
  gates remain open.
- **Emitter dimensions are direct pixels** (`Size X/Y/Z`). Revision 12 uses these
  direct values for Box and Sphere; `Disc Size` remains a separate layer-height diameter.

## How to regenerate

1. Apply one instance of every module to a single layer in a fresh composition.
2. `File > Scripts > Run Script File...` → `tools/dump_effect_parameters.jsx`.
3. Keep the output local (`artifacts/reference/`), never commit it.
