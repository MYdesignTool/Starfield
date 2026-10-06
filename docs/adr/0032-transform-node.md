# ADR 0032: independent Transform node

- Status: staged implementation; affine foundation selected, host/node contract open.
- Task: M3-11, AE2023 baseline.

## Reference and scope

The owner's text inventory and 2026-10-06 screenshot establish fourteen visible
controls: Inherit Motion (Null Layer), Anchor XY, Anchor Z, Position X/Y/Z,
Rotation X/Y/Z, Scale X/Y/Z, Particles Scale and Particles Opacity. Numeric defaults
are zero for translation/rotation, 100 for scales/opacity and composition centre
for Anchor XY. Source report identities and implementation are not reused.
Null layer selection is absent by default. Actual Stardust numerical parity is
not established by the screenshot or static inventory.

## Affine foundation

Settings.hpp owns a separate ParticleTransformSettings value type; existing
Settings layout is unchanged. The core uses canonical world coordinates and
row-major matrices acting on column vectors. The AE adapter will own pixel/sign
conversion and sampled Null transforms. Euler rotation is X then Y then Z, as
used by the existing core. With pivot A, translation P, rotation R and system
scale S, local centres map to P + A + R*S*(centre-A). Inherited affine matrix N
acts on that result. Velocity uses N.linear*R*S without translation.

System Scale changes spacing, not sprite size. Sprite axes use N.linear*R;
Particles Scale is a separate nonnegative multiplier and Particles Opacity a
0..1 multiplier. This is an independently specified initial convention requiring
AE qualification. A full inherited matrix preserves reflection and shear; avoid
lossy Euler decomposition. Chaining nodes must compose centre and sprite bases
separately. A centre matrix cannot stand in for both.

Compile once per sampled node into fixed arrays; no per-particle trigonometry,
allocation, host objects or mutable cache. Validate all values before use:
anchor/translation and inherited entries finite within +/-1e6, rotation within
the core angle bound, scales within +/-10000 percent, particle scale 0..10000,
opacity 0..100 and an exact affine final matrix row [0,0,0,1]. Negative and zero
system scales remain valid. Malformed input returns invalid_request.

## Remaining node and host contracts

The foundation does not add a graph kind, native effect, controls or render ABI.
Ordered Force/Transform and branching/auxiliary semantics must be specified
before integration. The adapter must capture Null motion at each shutter sample
and convert to the particle world frame; no host object may cross into core.
Null reference-pose and anchor semantics require host evidence. Sprite basis
transport through CPU/GPU needs a versioned contract; any ABI/snapshot extension
will be documented here before authoring it. Existing wire/kind/disk IDs remain.

Focused math checks are separate from actual AE render, Null animation, presets,
undo, project persistence and motion-blur qualification. Keep M3-11 open until
those integration gates are met; do not expose or deploy an inert Transform node.

2026-10-06 foundation evidence: tests/RunCoreTests.ps1 -ParticleTransform compiles
two sources without SDK dependencies and passes 268 checks with MSVC C++20 /W4
and /O2 /DNDEBUG. Checks remain active in release builds and cover identity,
translation, pivots, Euler order, independent scales, reflection/shear, singular
matrices, composition, finite bounds and malformed inputs. Full node and actual
AE behavior are not covered. Log: artifacts/m3-11-transform-math.log.
