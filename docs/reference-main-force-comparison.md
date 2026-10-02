# Stardust main effect and ordinary Force comparison

Updated 2026-10-02 for P-02H / build 14. Evidence: owner's images, the locally
observed AE 23.5 parameter inventory, and the [official guide](https://superluminal.tv/user-guide).
The private dump remains ignored under artifacts/reference. Behavior descriptions
and public control names are specification input; implementations are independent.

## Ordinary Force

The reference's ordinary Force shares its air module with Turbulence; the
separate Physical Forces module acts on dynamic physical particles. The owner's
current Force request concerns the ordinary node connected after Particle.

| Reference ordinary Force | Build 14 | Fresh default / CEP step |
|---|---|---|
| Gravity | Scalar downward acceleration; replaces our vector UI | 0 / 1 |
| Gravity random | Deterministic per-Force, per-particle attenuation | 0% / 1% |
| Wind X/Y/Z | Separate scalar controls, AE coordinate signs | 0 / 1 |
| Wind and Spin Over Life | Independent percentage curve in CEP, saved native bank | Constant 100%; fixed 0..100 axis |
| Spin | Radius of independent orbital displacement | 0 px / 1 px |
| Spin Frequency | Cycles/s; zero uses one cycle/s when Spin is active | 0 / 0.01 |
| Spin resist | Exponential attenuation | 0% / 1% |
| Spin Delay (Seconds) | Delay before spin begins | 0 s / 0.1 s |
| Air Density | Analytical linear resistance | 0 / 0.01 |
| Air Density Rotation | Requires sprite/model orientation | Open |
| Turbulence controls in the shared reference module | Separate noise/displacement contract | Open |

Reference names, order and observed zero defaults are aligned. Pixel acceleration,
drag coefficient, spin equations and randomness are explicitly our own numeric
contract (ADR 0021); exact reference trajectories have not been measured. Native
and CEP edits share the same saved effect records. Curves currently edit in CEP;
custom graph drawing inside AE Effect Controls remains a separate UI capability.

## Renderer main effect

| Reference main section | Current implementation | Remaining work |
|---|---|---|
| Panel / Presets | CEP editor and example setups; renderer is the sole menu effect | Main-effect open-panel button and preset library |
| Switches: Shy / Solo / Helpers | AE effect enable state and node selection | Node switch contract and helper drawing |
| Time Remapping | Enable and Time (Seconds), including animated renderer sampling | Real AE animation/undo qualification |
| Render Settings: Preview / Particle chance | Stable identity-based percentage rendering filter | Real AE visual qualification |
| Quality / Render Output / 3D Preview | Transparent CPU circle output at 8/16/32 bpc; camera projection | Quality alternatives and model render passes |
| Environment / AO / Shadows / Subsurface | No model/light shading engine | Model/material and lighting contracts |
| Motion Blur | Single-time deterministic evaluation | Shutter sampling, accumulation and gain |
| Fog / Clipping / DOF / Z Buffer | Behind-camera clipping and ROI | Distance fade, fog/depth outputs and lens sampling |
| Physics / Simulation / State | Stateless analytical ordinary motion | Collision engine and explicit state/cache contract |
| Volume Render / Cache | No volume backend | Volume engine/cache contract |
| Advanced / Cache hints / Maximum Life | Particle Life bound 10000 s; bounded work and allocations | Reference-specific cache and advanced policies |
| Starfield Output population cap | Fresh default **1000000**, maximum 2000000 | Owner-requested extension; local reference main dump has no matching cap control |

Do not register inert controls for missing renderers. This matrix distinguishes
current runtime controls from future capabilities; it is not a parity claim.
Output remains visible in the graph and owned by the main renderer, without an
extra Output effect. Other nodes retain independent native effects.

## Qualification

Use fresh development effects for Force schema 2 / Output schema 3 / main 21 /
native layout 6. Check native and CEP Force edits, curve independence, reversed
frame order, Auxiliary inheritance, Time Remapping animation, Preview stability,
undo and save/reopen in AE 2023. Core/fake-host checks and an SDK build do not
establish those host results.
