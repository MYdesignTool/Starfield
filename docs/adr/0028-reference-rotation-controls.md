# ADR 0028: procedural particle rotation and emitter orientation

## Build38 default dial state

The owner supplies collapsed Emitter Angle/Orient rows as the required initial
appearance. The shared native angle registration sets PF_ParamFlag_START_COLLAPSED
for Emitter Angle/Orient, Particle Angle/Speed and Limit Angle. This is a creation
default only: subsequent UPDATE_PARAMS_UI callbacks do not force it and existing
user expansion remains respected. IDs, saved values, keyframes and schemas are
unchanged. Native compilation does not qualify actual AE dial appearance.

Status: implemented; integrated deployment and AE comparison pending, M3-08. Owner supplies the Rotation Properties
inventory and Random Limit choices None / All Axis / X / Y / Z on 2026-10-04.
M3-07 movable gradient work is committed separately; integration remains on the
development branch. The owner also requests a generated main preset launcher;
that belongs to subsequent card P-03, not this rotation contract.

M3-08 owns Graph/Settings/ParticleInstance, shared SpriteGeometry, transient
EmitterHistory transport, native node layouts/definitions/readers/synchronization,
CEP parameter/curve bindings, schema/version and corresponding focused checks.
Render.hpp, C ABI3, GPU frameworks/kernels and main binding layout stay unchanged.
The shared sprite projection gives CPU and GPU the same anchor/rotation geometry.

Particle retains its existing disk IDs, adds Random Limit (228), Limit Angle
(229), Anchor X/Y (230/231) and constant Rotation Over Life bank 960/970..977/
980..987. Native particle schema6/base102 reorganizes only the rotation section;
new graphs/effects are required, with no development legacy migration. Emitter
schema7/base34 adds native Orient X/Y/Z (137..139); Angle rotates emission shape,
Orient rotates the directional emission cone. All angle controls use native AE
turns/degrees. Direction defaults Uniform. Particle Limit To 2D defaults off,
and Rotation Speed Random has its reference label. Orient To labels Nothing for
the supported no-target mode; particle-motion and birth-position targeting remain
the supported procedural targets. Light/Null/Normal source targeting and Starting
With filtering require their own external source/dependency contract and are not
advertised as inactive menu choices.

Independent rotation semantics: Angle Random supplies stable per-axis birth
variation up to 180 degrees at 100%; Random Limit selects which axis variations
are bounded by abs(Limit Angle), without changing the authored base angles.
None ignores the limit. Rotation Over Life is a constant 2..8-point degree curve,
added around the local Z axis by normalized lifetime; its default is flat zero.
It combines with base angles and age-based spin. Exact reference random kernels
and undocumented curve scaling are not claimed; they remain comparison gates.
Anchor X/Y default50%, range0..100 and shift the primitive about its local pivot.
AE Z angles use a clockwise screen convention, including camera projection.
Birth controls keep birth sampling; all native public controls retain keyframes.

Particle snapshots advance from version2/184 bytes to version3/200 bytes for two
anchor doubles. Both Core and native adapter are deployed together. No persistent
sequence schema or public effect identity changes. Rollback restores the paired
native/Core/CEP bundle and requires re-creating these development test effects.
Official visible-behavior reference: https://superluminal.tv/user-guide (Particle
Rotation group, Anchor, Limit to 2D; Emitter Direction). Owner screenshots own the
names/order/choices/defaults; SDK/forensic material is reference data only.
