# ADR 0032: independent Transform node

- Status: staged implementation; affine and render transport selected, host/node contract open.
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

## Sprite transport and migration

EvaluatedGraph owns a bounded table of at most4096 row-major3x3 sprite bases.
ParticleInstance carries one uint32 index:0 means the existing identity path,
1..table.size select shared immutable entries. Share a basis across a branch,
rather than adding nine doubles to every particle. Table coefficients must be
finite and within +/-1e12. A composed centre matrix is bounded to +/-1e18;
particle scale products are bounded to1e6 and opacity remains0..1.
Exceeding these derived bounds rejects the evaluation, without clamping values.

The transient0x8004 evaluated-particle record adds version4. Its40-byte header
extends the existing32-byte header with basis-count:uint32 and reserved:uint32
(zero), followed by evaluated-node UUIDs, then72 bytes per basis, then particles.
Particles retain their200-byte stride, with the formerly reserved final uint32
now the basis index. Decode version3 with index0 and no bases; encode version3
whenever the table is empty, preserving prior bytes. Saved graph envelope/schema,
released node kinds and parameter IDs are unchanged. The existing512MiB transient
payload and particle/node limits remain; sizes and indices validate before use.

Core ABI4 is a capability guard for this transport, although exported C request
and GPU sprite layouts do not change. New AEX and pinned/selected Core must be
published as one pair. Old Core ABI3 cannot accept new requests; no Core-only
publication across this boundary. Prior paired builds provide rollback, and old
saved projects still evaluate through the existing identity path.
BuildWindows's runtime-only publication checks all five installed AEX hashes
against their paired build artifacts before creating runtime files or replacing
the selector. A fingerprint from an unpublished full build alone is insufficient.
An incompatible candidate remains buildable with -NoRuntimePublish.

CPU rasterization and portable GPU scene preparation use the same projection.
Base sprite axes use the existing Euler/up-axis convention. With an explicit
basis, convert display-axis Y to canonical world Y, apply the basis, then project
back through layer pixel aspect and camera transforms. Transform acts on this
plane for every procedural shape, including a previously camera-facing sprite;
identity index0 preserves the old billboard behavior exactly. Local system Scale
is absent from this sprite basis. A full inherited affine basis retains shear
and reflection. This opt-in convention is independently specified; matching
Stardust camera/Null geometry still needs observable host evidence.

Linear motion blur interpolates basis coefficients for each unique pair of
endpoint indices, once per pair, with the same4096-entry limit. A birth/death
visible at one endpoint carries that endpoint's basis. Exact Subframe sampling
retains the sampled basis. Interpolation is an affine approximation (it may
collapse halfway through a large rotation); it does not claim rigid rotation
interpolation. Invalid indices, allocation failure, work limits and cancellation
return typed errors rather than silently dropping transforms or particles.

## Remaining node and host contracts

These stages do not add a graph kind, native effect or controls.
Ordered Force/Transform and branching/auxiliary semantics must be specified
before integration. The adapter must capture Null motion at each shutter sample
and convert to the particle world frame; no host object may cross into core.
Null reference-pose and anchor semantics require host evidence. Existing
wire/kind/disk IDs remain. Do not expose an inert node while these gates are open.

Focused math checks are separate from actual AE render, Null animation, presets,
undo, project persistence and motion-blur qualification. Keep M3-11 open until
those integration gates are met; do not expose or deploy an inert Transform node.

2026-10-06 foundation evidence: tests/RunCoreTests.ps1 -ParticleTransform compiles
two sources without SDK dependencies and passes 268 checks with MSVC C++20 /W4
and /O2 /DNDEBUG. Checks remain active in release builds and cover identity,
translation, pivots, Euler order, independent scales, reflection/shear, singular
matrices, composition, finite bounds and malformed inputs. Full node and actual
AE behavior are not covered. Log: artifacts/m3-11-transform-math.log.

2026-10-06 source transport evidence: math466, transport3544, current-node437
and publication11 focused checks pass. May2023 /MT native build succeeds with
-NoDistPublish -NoRuntimePublish. Exported C CPU/GPU-scene transport and software
packed-scene parity are covered; actual AE and GPU execution are not. Native46/
CEP49 remains installed. See build-matrix.md for logs and remaining gates.
