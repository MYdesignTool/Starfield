# ADR 0021 — Reference Force and renderer globals

Status: development contract; exact reference behavior and AE qualification open.

## Scope and evidence

The observed local AE parameter inventory and Superluminal's public user guide
identify ordinary Force (Gravity, Wind, Spin and air resistance), sharing the
reference's air module with Turbulence. Physical Forces is a separate rigid-body
module. Independently implement the ordinary Force subset in StarfieldForce.aex.
No private identifiers, implementation code or assets are reused.

## Motion

Gravity is a signed downward acceleration in full-resolution pixels/s²; Wind
X/Y/Z use the same acceleration unit, with AE +Y downward and +Z away. Air
Density is a nonnegative drag rate. These units and the equations below are our
explicit implementation choices; observed names/defaults do not prove exact
reference numeric parity. Gravity random is a deterministic per-particle percent
attenuation, salted by the Force UUID. Multiple connected forces accumulate.
Wind and Spin Over Life is a bounded piecewise linear 0..100% curve. Integrate
each linear wind segment analytically with the branch's total linear drag, so
random frame order/subframes retain identical results. Spin adds a deterministic
XY orbital displacement: radius in pixels, frequency in cycles/s (zero uses one
cycle/s), exponentially attenuated by Spin resist after Spin Delay. Its curve
controls radius; instantaneous velocity includes the curve's slope. Circular
sprites have no visible orientation, so Air Density Rotation is not advertised.

## Renderer globals

The renderer main effect owns Max Particles (fresh default 1000000), Time
Remapping enable/time, and Preview enable/Particle chance (fresh 100%). Saved
Output parameters carry these values through the immutable graph snapshot.
Remapping changes the simulation clock, including Auxiliary; camera sampling
remains at the host composition time. Preview uses stable particle identity and
is a rendering filter, preserving full simulation/cap semantics and Auxiliary
parents. Main has meaningful nested sections; node effects retain flat controls.
No main-effect-only parameter copy replaces native node values.

## Development version plan

Build 14 uses Force schema 2, Output schema 3, native layout 6 and main manifest
21. Existing main stream indices 1..89 remain fixed; global controls are appended.
Reserve removed Force disk IDs 301/302. Allocate new scalar/control/curve IDs
explicitly in schema/node-parameters.json. No released identities change; this
unreleased project requires fresh effects and has no compatibility migration.
Core C ABI 2 remains unchanged: new values travel in existing graph bytes.
The deployment script retains a verified build-13 rollback and single Junction.
