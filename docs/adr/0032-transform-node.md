# ADR 0032: independent Transform node

- Status: core graph, affine/render transport, native/Null and CEP authoring candidates implemented; AE qualification open.
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
BuildWindows's runtime-only publication checks all six installed AEX hashes
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

## Ordered graph evaluation contract

Add the independent org.starfieldfx.nodes.transform kind, schema1, with particle
input1/output2. Required keys1..6 are canonical Anchor:Vec3, Position:Vec3,
Rotation degrees:Vec3, system Scale percent:Vec3, Particles Scale:double and
Particles Opacity:double. Optional key7 carries the sampled inherited affine
matrix:132 bytes, version byte1 and three zero reserved bytes, followed by16
little-endian doubles. Missing matrix means identity. The native Null reference
will append its own authoring/resource key; no released kind/key is renumbered.

A Particle branch may traverse Force and Transform stages in graph order before
Output or an Auxiliary input. Different Particle streams retain independent
frames. Reconverging paths of the same stream must carry the same Transform
chain: parallel forces in one frame merge once, but bypassing or diverging
Transform chains is an ambiguous merge and returns invalid_request. Auxiliary
inputs terminate a parent prefix; unrelated child frames do not merge into it.
Plan only descendants that reach that branch's terminal without crossing another
Emitter. Existing graphs without Transform retain their current fast path.

Compile the complete centre map once per branch. Map its initial birth position
and velocity before integration. Each Force's gravity, wind and spin-plane axes
map through only the Transform suffix downstream of that Force. Scalar drag
commutes with affine linear maps, so the ordinary evaluator retains closed-form
integration, including singular/negative system scales, without an inverse or
frame stepping. Validate authored Force bounds before mapping; derived vectors
are finite products of already bounded transforms and are not slider-clamped.
Bound cached derived transforms to65536 per evaluation, in addition to existing
node/edge/work/cancellation limits. Sprite axes, size and opacity use the complete
branch map, independently of centre/system scale.

Temporal evaluation samples each Transform's pose at the requested render time;
Force history remains midpoint sampled on the existing simulation lattice and
maps through that current suffix. This is a geometric stage, not additional
physics integration. Velocity is the spatial linear map of simulation velocity;
it does not add a derivative of animated translation/rotation. Exact shutter
samples still capture the moving geometry. An Auxiliary parent's prefix evaluates
at child birth, retaining that transformed birth position after its parent dies;
the child's own Transform acts on inherited position/velocity and its own stream.
Orient To directions are mapped back into the sprite basis before applying the
existing Particle angle convention, using a precompiled bounded pseudoinverse
for reflection/shear and rank-deficient inherited matrices. Numerical reference
parity and animated Null velocity semantics remain observable AE gates.

## Native authoring and Null sampling contract

Append native kind4, independent match name org.starfieldfx.node.transform,
schema1 and fourteen controls in the reference order. Native stream indices1..14
use new explicit disk IDs1401..1414. Existing kinds/IDs/stream indices remain.
Metadata uses the existing bounded four-edge and UUID record after control14.
Append optional core key8:uint32 for a project-local AEGP layer ID;0 means None.
The native PF_LAYER selection is constant structure, while controls2..14 animate.
The portable resource ID does not encode an AE object or layer index. The CEP
adapter resolves it against the target composition when reading/writing the
layer selector; unresolved resources must fail rather than bind another layer.

The Null's current anchor maps to its world position in the effect layer's frame.
Input canonical coordinates are offsets from that anchor, using the effect
layer's pixel grid. Full Null-to-world and world-to-effect-layer affine matrices
retain parent scale/shear/reflection. None yields identity. A new centred Null
with unit scale/rotation and an untransformed full-comp effect layer yields
identity. This is an independently specified absolute pose convention, with no
implicit reference-frame capture. Actual Stardust numerical parity is open.
Local native Position converts pixels to canonical coordinates (+Y down to up);
native X/Z angles change sign, Y retains sign, before core Euler composition.
Anchor XY/Z uses the same centred canonical frame, with native Z0 -> core Z0.

Do not call AEGP suites during render/pre-render to follow another layer. UI-only
compilation records twelve raw pixel-space affine coefficients in synthetic
binding fields15..26 of the new native kind. They are not native parameter
indices or disk IDs. Extend the kind-specific field bound of record0x8002/v1;
the existing layout and existing kind records remain byte-compatible. Main-effect
owned numeric aliases evaluate generated UUID-selected expressions using the
documented layer-space transform methods at each requested time. Transform
playback converts those twelve coefficients into core key7 without host objects.
Source selection uses the native PF_LAYER reference, preserving layer reorder.
Bounds, failed layer transforms and singular effect-layer frames reject the edit
or render; a singular source Null itself remains valid.

Synthetic fields never use a native numeric parameter as a constancy proof.
With a selected source, keep them dynamic even if its numeric node controls have
no keys. With None, certify identity only through the actual alias PF states and
the constant resource selection. Invalidated states retain exact sampling. Core
pose sampling remains once per branch/current time and at Auxiliary parent birth.
The fixed512-alias bank bound remains; overflow rejects atomically. ABI4 native
publication adds StarfieldTransform.aex to the paired module guard and bundle.

## Remaining native and host contracts

The core graph kind and ordered Force/Transform/Auxiliary stages, native effect
and numeric Null capture are implemented in the source candidate. CEP graph,
inspector, resource selector and numeric preset roundtrip are implemented in the
panel50 source candidate. Native47/Core ABI4 and panel50 form the development pair.
The adapter samples the numeric pose at shutter and Auxiliary birth times;
no host object crosses into core.
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

2026-10-06 graph evidence: TransformGraph586, ParticleTransform483,
TransformTransport3544 and CurrentNodes437 focused release-active checks pass.
Coverage includes Force suffix order between two transforms, parallel forces,
independent Particle frames, static/temporal evaluation, Auxiliary parent-at-birth
and inherited velocity/style, origin history, pseudoinverse orientation, graph
codec/frozen replay, derived bounds/cancellation and live rectangular CPU pixels.
Native/CEP controls and actual AE/Null sampling remain unqualified. The installed
native46/CEP49 bundle is retained while the source candidate uses Core ABI4.

2026-10-07 native source evidence: Transform controls102, Particle controls91,
matrix bridge48, actual generated matrix expressions63, native sync6884 plus
camera12, TransformGraph589, publication guard12 and isolated deployment18
checks pass. The existing generated expression/keyframe suite also passes after
updating retired fixture indices and its gateway injection signature. Source
build47 compiles with May2023 /MT, NoDistPublish and NoRuntimePublish. The new
module and ABI4 remain uninstalled while native46/CEP49 hashes are retained.
The native suite covers None constancy, selected Null remaining dynamic,
numeric render playback without AEGP calls, IDs rather than layer indices,
coordinate/sign conversion and pre-publication rejection with balanced refs.
Layer selection's supervised stream timing, expression dimensions/engine,
camera geometry, shutter and persistence require actual AE2023 evidence.

## CEP authoring and preset resource policy — 2026-10-07

The palette and context menu create Transform1 with the fourteen reference
controls. Inspector pixels/AE angle signs convert to the existing canonical
keys; the inherited matrix key7 is hidden and never submitted as authoring.
Only the native adapter owns its current/birth/shutter-time sampled value.
Acknowledgement compares ordinary controls, layout, edges and resource ID,
not a stale derived matrix. Omitted resource key8 is equivalent to None.

The gateway resolves source disk1401 from the current composition's layer ID to
its current index immediately before writing. Readback converts the PF_LAYER
index back to that ID. Missing IDs, layers without an Anchor Point, malformed
records and unrepresentable native point/angle/pixel values reject before the
undo group or mutation. The composition resource inventory is bounded to4096
layers, cached for only one synchronous request, and discarded on reply. Idle
pulses read only numLayers; a full snapshot refreshes choices. Rename labels may
require Refresh. Numeric controls retain existing keyframe/expression guards.

Numeric Transform presets with None roundtrip through Save/Import/Add/Replace;
sampled matrix key7 is removed. A selected project-local layer ID cannot be
ported safely to another project: export/import rejects a nonzero key8 with an
explicit instruction to choose None. Add of an ordinary preset preserves
existing project Transform resources. A portable resource-remapping UI is a
separate future contract; never infer a match from a name or recycled layer ID.

The isolated panel candidate passes20 JS suites, including89 focused Transform
checks covering control order, unit/sign conversion, reorder-safe IDs, resource
preflight, native disk lookup, animation-preserving writes, native snapshot and
transaction receipts, and numeric preset Add/Replace. This does not qualify
actual AE2023 Null dimensions, parenting, render, undo or persistence.

Development publication: native47/Core ABI4 + CEP50 deployed through the existing
Starfield and CEP Junctions after a read-only check found AE closed. Seven native
and eleven changed CEP file hashes plus the selected Core were verified; the
installed panel repeats all20 passing suites. Before/after receipts are
artifacts/m3-11-panel50-deploy-before.json and m3-11-panel50-deploy-after.json.
The native46/CEP49 backup is artifacts/disabled/m3-11-native47-panel50-transform-20261007.
One-step undo, with AE closed: tools/Restore-TestBuild.ps1 -PluginDir
'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName
'm3-11-native47-panel50-transform-20261007' -Restore. Its read-only verification
passed. No AE process or unrelated host setting was changed; M3-11 host gates
remain open.

## Empty layer selector correction — 2026-10-08

The owner reports that adding Transform in native47/CEP50 fails with animation
binding stream294, error516. The previous message did not distinguish a suite
error from the adapter's own rejected evaluated value; the exact host operation
and cause remain unconfirmed. Inspection found that every default synthetic
expression reads the empty PF_LAYER property before taking the None branch.
The existing JS fixture returned numeric0 for None and did not model a throwing
empty layer property. The full generated expression now has that regression
fixture, which reproduces the unsafe read in the previous expression.

Native48 uses the constant, UI-validated resource field to generate identity
expressions for None, without accessing the layer selector. Choosing a source
or returning to None regenerates the expressions through the supervised edit.
Selected sources retain exact temporal sampling and explicit failures for
unavailable layers; failures are not converted to identity. Binding rejection
messages include the source parameter and precise read/write/verification step.

This changes no parameter ID, effect identity, graph/sequence schema, saved
record bytes or Core ABI4. Existing aliases are repaired through the normal UI
binding transaction, with expression/value/state rollback on failure. Build48
code/PiPL packing agrees at32816. Actual AE2023 creation, Null motion, undo and
reopen remain owner qualification gates.

Candidate evidence: May2023 /MT full native build with -NoDistPublish and
-NoRuntimePublish passes. NativeSync7015 plus Camera12, affine/None60 and actual
generated expression167 checks pass; the existing74-expression/keyframe JS
suite also passes. Run NativeSync and TransformBinding C++ scopes before the
Transform expression JS suite to generate both complete aliases and body data.
Logs: artifacts/m3-11-native48-build.log, m3-11-native48-sync.log,
m3-11-native48-binding.log, m3-11-native48-expressions.log and
m3-11-native48-legacy-expressions.log. These are source/fake-host results,
not confirmation that the owner's stream294 error is resolved in AE.
