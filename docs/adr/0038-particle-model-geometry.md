# ADR 0038 — Particle Model geometry and resources

Status: staged implementation, 2026-10-09. Task M3-17.

## Particle snapshot and CPU milestone evidence

Snapshot8 now retains Model groups and the explicit200-byte particle stride;
versions3..7 remain readable and keep their canonical bytes. CPU renders default
cube/imported polygon groups from evaluated snapshots, with full particle pose,
ROI/downsample/PAR, near clipping even when the center is behind the near plane,
four transfer modes and stable ordering with primitive particles. Multiple mesh
members combine coverage/depth before applying logical-particle opacity once.
Numeric meshes compile to call-lifetime leases once per resource/CPU call.
Shared frame input-triangle and sample-work limits fail without output pixels.
The GPU scene API explicitly rejects Model for CPU fallback.

`tests/RunModelParticleTests.ps1 -Run` and `-Run -Sanitize` each pass2695 checks;
logs artifacts/m3-17-model-particle-{tests,asan}-current.log. They cover actual
Core/C ABI pixels, Model tables and malformed lengths/indices/transforms,
every truncated snapshot, legacy versions, mixed Texture/Cloud tables,
allocation/cancellation, missing imported resources and shared frame caps.
The initial fixture lacked a SequenceResult overload; its first numeric run
tested a triangle boundary as an interior pixel and omitted the near-camera
identity homography. These fixture failures were repaired before passing.
MSVC link/debug flags were moved out of response files for correct ASAN symbols.
Resource2076 and scene6406 rechecks pass; existing Texture4322 also passes with
snapshot8/CPU candidate (artifacts/m3-17-texture-snapshot8-regression.log).
ABI exact-prefix ASAN30 passes on the current candidate as well.

The live graph's Model node/input/shape sampling and all native/CEP authoring
remain pending. No new menu/selector or installed binary is published. This is
render evidence from explicit evaluated snapshots, not a usable AE Model author.

Frozen snapshot/CPU sourcee12517d99a1545694a02c99182fde7e893b0f736 at
artifacts/prepared/m3-17-model-particles-e12517d/source passes the full May2023
x64 Release /MT build with -NoDistPublish -NoRuntimePublish; log
artifacts/m3-17-model-particles-native-build.log. The main AEX now compiles the
mesh/resource dependencies used by snapshot validation, and the adapter-input
fingerprint includes them. All18 installed native60/CEP61 files still match the
deployment receipt (artifacts/m3-17-model-particles-installed-hashes.json).
No Model publication or AE2023 qualification follows from this build.

## Resource and transport milestone evidence

ModelResources implements the SFMG1 numeric mesh codec and particle pose below.
`tests/RunModelResourceTests.ps1 -Run` passes2076 checks with MSVC /MT; log
artifacts/m3-17-model-resource-tests-current.log. Tests cover every cube payload
byte/truncation, header/checksum/numeric/index rejection, resource IDs/counts/
aggregate byte preflight, cancellation/allocation failure, and pose agreement
with Texture axes under Up Axis/Euler/Limit To2D/Transform reflection and shear.
Cube projection, PAR, anchors and local transforms also pass. The first fixture
mixed incompatible types in one auto declaration; it was repaired before passing.

RenderRequest now owns numeric Model sources; C ABI8 appends their call-lifetime
arrays while accepting the exact old ABI7 prefix. Both normal and AddressSanitizer
`tests/RunModelTransportTests.ps1 -Run` (add `-Sanitize`) pass30 checks, including
an allocation with only the ABI7 byte span and poisoned absent tail fields.
Counts, aggregate bytes and duplicate IDs reject before numeric array access;
invalid/copied geometry and cancellation reject without publishing pixels.
Logs: artifacts/m3-17-model-transport-{tests,asan}-current.log.
No graph/shape/snapshot/native resource authoring is connected yet. Accepting an
unused mesh source does not demonstrate Model rendering. The installed pairing
remains native60/ABI7/CEP61; this shared ABI change requires a full paired build.

Frozen resource source8220f25d1cc95b1aa5700fb962b8e137d8adf418 at
artifacts/prepared/m3-17-resources-8220f25/source passes the full May2023
x64 Release /MT build with both NoPublish switches; log
artifacts/m3-17-resources-native-build.log. All18 installed pairing hashes still
match native60/CEP61 (artifacts/m3-17-resources-installed-hashes.json).
This freeze predates the snapshot/CPU milestone above; it is not published.

## Triangle milestone evidence

tests/RunModelSceneTests.ps1 -Run passes6406 checks,0 failures with MSVC /MT;
log artifacts/m3-17-model-scene-tests-current.log. Cube shared edges/solid-depth
coverage, reversed winding, reflection/shear, coincident faces, downsample and
explicit PAR matrix pass. ROI tiles match full-frame coverage/depth. Analytic
perspective depth/UV, near-plane crossings, principal-branch horizon clipping,
homography sign/scale, work preflight, allocation failure and cancellation pass.
Four transfer modes include HDR signed Screen and Stencil on existing pixels.
Composition validates only touched pixels and rejects bad inputs before mutation;
mid-composition cancellation requires discarding the caller staging buffer.
The fixture first used the wrong near_clip member path and then a cancellation
threshold beyond the small cube's actual poll count; both fixture failures were
repaired before the passing result. Geometry4141 checks were rerun successfully.
CMake/Core vcxproj compile the numeric sources. This is a numeric staging API,
not graph/Particle/resource/native authoring, wire migration or AE qualification.

Frozen source5d178022aaf239e65426192b8d757103f2e01cd8 at
artifacts/prepared/m3-17-triangle-5d17802/source passes the complete May2023
x64 Release /MT build with -NoDistPublish -NoRuntimePublish. Log:
artifacts/m3-17-triangle-native-build.log. Both new numeric sources compile in
the Core target. All18 installed native60/CEP61 hashes still match its receipt
after the build; no candidate publication or host support claim follows.

## Geometry milestone evidence

tests/RunModelGeometryTests.ps1 -Run passes4141 checks,0 failures with MSVC /MT;
log artifacts/m3-17-model-geometry-tests.log. Cube topology, closed edges/outward
winding and UVs pass. OBJ cases cover separate indices/negative indices, weights
without XYZ division, UVW, comments/continuations, both windings of concave
polygons through256 corners, deterministic triangulation/area, per-corner
attributes, referenced bounds and translated/subnormal finite coordinates.
Malformed/non-planar/intersecting faces, unsupported statements, per-limit work
failures, cancellation and injected allocation failures reject with typed errors.
The cube initially used an MSVC-incompatible aggregate initializer; it was
repaired before recording this passing evidence. No renderer/authoring/host
qualification or full native build is inferred; installed native60/CEP61 stays
unchanged. Next: projected triangles, clipping/depth/compositing and resource
transport, followed by full authors.

## Reference and scope

The [Stardust guide](https://superluminal.tv/user-guide) describes a default cube
for Model particles and connected Model nodes for other geometry. Face particles
depend on OBJ face emitters. Path controls depend on path emitters. The owner's
parameter dump records Shape and Use Model(s) values but not their menu contents;
those two menus have been requested. Model geometry is implemented independently.

This card owns the complete Model geometry/resource/render/authoring route.
The first milestone establishes immutable geometry, a default cube and bounded
OBJ polygon input. It does not expose a Model menu, change the installed bundle,
or claim mesh rendering from a parser test. Subsequent milestones implement
projected triangles, clipping/depth/compositing, graph/native/CEP resources and
portable presets, then qualify them in AE2023. Face/OBJ emitter, path-emitter
semantics, material/light and shadow behavior remain explicit dependencies or
separate behavior cards; the full owner goal remains active.

## Independent geometry contract

Settings.hpp owns plain numeric mesh inputs, corner indices, optional
normals/texture coordinates and bounds; ModelGeometry.hpp owns their operations. No AE suite, host path, host pixel world
or mutable host lifetime enters this contract. Unit-cube coordinates are centred
at zero with side length1; twelve triangles have outward winding, six normals
and stable face UVs. Bounds cover referenced triangles; unreferenced positions
still undergo validation. Render size/orientation and model normalization are separate
operations, not implicit parser modifications.

OBJ input is an already-read string, not a filename. Independent parsing accepts
polygon positions, UVs, normals and the four face-corner index forms. Positive
indices are1-based; negative indices resolve against the corresponding list at
that face. References must resolve at the face declaration. Polygon winding and
corner attributes survive deterministic triangulation of simple planar concave
polygons. Invalid, self-intersecting, degenerate or non-planar polygons reject
explicitly. Names/material directives are recognized as geometry metadata;
material evaluation will be a later contract. Curves, surfaces and line/point
geometry return unsupported_format, never a silently successful partial mesh.

Parsing bounds: source8MiB, logical line64KiB, positions/normals/UVs65536 each,
triangles65536, face corners256, triangulation work16000000 operations and
absolute coordinate1000000000. Continuations are folded into a bounded logical
line. Limits return work_limit_exceeded, malformed input invalid_request,
allocation failure allocation_failed and cancellation cancelled. Cancellation
is polled per logical line and inside polygon work. Failed parses publish no
partial geometry. Error details are static core-owned strings; an optional line
location is numeric. The original [Wavefront Appendix B1](https://www.martinreddy.net/gfx/3d/OBJ.spec)
defines optional vertex weights and up to three texture coordinates. Polygon
XYZ stays unchanged by an optional weight; the weight and UVW are retained as
numeric metadata. This parser does not reinterpret rational curve weights as
Cartesian division. Curve/surface statements remain explicitly unsupported.

## Future render and migration boundary

### Resource/Particle transport plan

Mesh payloads belong to native Model resource storage, separate from CEP's
frequently synchronized graph. Model resources use nonzero128-bit stable IDs,
owned numeric geometry and no filename/host handle in Core. At most256 sources
and64MiB total numeric payloads are accepted per request; each mesh remains
bounded by the existing65536 vertex/triangle limits and an8MiB encoded cap.
Native storage will use independent SFMG version1: a32-byte little-endian header
(magic,version,header size,total bytes,payload CRC32,four counts), then position
records32bytes,UVW/normals24bytes,triangle corners36bytes. Bounds are recomputed,
never trusted from serialized metadata. Decode validates lengths/counts/checksum
before allocation, and numeric/indices/triangles before publishing a resource.

The next graph extension appends Model shape4, preserving shapes0..3. A new
org.starfieldfx.nodes.model schema1 produces org.starfieldfx.types.model-stream
on port1. Particle schema7 gains optional input port3; multiple Model connections
form one logical geometry group, ordered by Model node UUID. Model parameters
are key1 opaque16-byte resource ID (zero/missing=builtin cube), key2 optional
uint32 resource revision, key3 optional opaque128-byte affine local transform.
Meshes are not embedded in these graph parameters. Existing graphs/parameters,
envelope1 and node disk IDs remain unchanged; older readers reject the new
node/port/shape rather than interpreting them as another primitive.

Model groups are shared by EvaluatedGraph and referenced by a new in-memory
ParticleInstance model_style_index. Snapshot8 retains the explicit200-byte
particle stride and add a64-byte header/group table. The high16 shape-word bits
select the Model group only for shape4; zero means the implicit cube. Each group
contains bounded resource ID/local-affine entries. Snapshot3..7 readers remain.
No sizeof(C++ object) is used as a wire layout.

Snapshot8 groups are stored after the fixed node/basis/Texture/Cloud tables and
before particles. A group starts with its total byte length including the8-byte
header and member count, then144-byte entries (ID16 + affine matrix128). All
group lengths/counts are preflighted before allocating entries. Shape4's axis
word carries no random key; legacy shape words and snapshot3..7 stay unchanged.

CPU Model instances share a call-lifetime validated geometry lease. The owner
must keep that numeric mesh alive and unchanged; the factory validates once,
then every particle projects the same geometry without rechecking all vertices.
The old raw-mesh projection entry point still validates. Groups combine their
triangles before depth/coverage so one logical particle applies opacity once.
The staged frame caps are16million input triangles and512million sample visits;
per-group projection/surface caps remain ModelSceneLimits. Models and sprites
share stable logical-particle camera-center ordering; interpenetrating translucent
meshes are still an open rendering contract. GPU sprite preparation rejects
Model explicitly and routes the adapter to CPU, without claiming native Model GPU.

Core ABI8 appends model-source count/pointer to the ABI7 request prefix.
Each C source exposes bounded numeric position/UVW/normal/corner arrays with a
stable resource ID, never C++ containers or an AE world. The Core copies these
arrays into its own validated geometry for the call. GetApi(7) remains available
for exact ABI7-prefix requests with zero model sources; the adapter requests8
after the full migration. This requires a complete paired build. No authoring
menu/native disk ID is changed at this resource foundation step; append those
bindings and packed version explicitly before authoring publication.

Particle pose uses canonical upward-positive Y through source-local affine,
Up Axis, the existing rotation convention and shared Transform basis, then
converts to layer-pixel downward-positive Y once. A unit cube's side length is
Size(Pixels) in physical output pixels; X divides by output PAR. Particle
anchors offset that unit-size frame, while Model source-local transforms preserve
their own origin. Limit To2D suppresses X/Y Euler angles as on existing sprites.
Canonical rotation is X(-angleX),Y(angleY),Z(angleZ), in that order, matching
the existing display-axis rotation. Layer matrix coefficients may reach1e30
after composition; source-local/camera coefficients retain1e12 bounds. These
numeric rules are independently implemented and still require reference/AE
appearance evidence before claiming vendor parity.

### Numeric triangle milestone

ModelScene.hpp stages a single mesh using an explicit affine row-vector
model-to-layer-pixel matrix and the existing FrameSpec/Camera values. This
avoids guessing Particle Model menu/size semantics while reference menus are
pending. It adds no field to RenderRequest, ParticleInstance, C ABI or snapshots.
Layer pixels have downward-positive Y; the eventual Particle pose adapter must
convert canonical upward-positive Y exactly once. Source-local/camera coefficients
are bounded at1e12; a composed model-to-layer matrix may reach1e30. The camera
layer-to-view matrix is affine.

Triangles clip against view Z>=near_clip, the positive homogeneous image-to-layer
branch containing the principal point, and ROI boundaries before division.
The image homography is normalized at the principal point; a horizon through
that point rejects explicitly. Perspective-correct depth and UV payloads survive
clipping. Normals/material/light evaluation remain a subsequent contract.
Both windings render; reflection/shear must not discard a whole mesh.

The first raster surface is unlit coverage/depth: four fixed quarter-pixel
samples, a top-left edge rule, and nearest view depth per sample within one
logical mesh. A mesh is treated as a solid surface, so internal/back faces do
not multiply its opacity. This is not multi-layer transparent material support.
Separate logical particles still composite in caller order, matching the
existing particle transfer contract; intersecting transparent meshes need an
explicit later ordering contract. Color/opacity are applied once to coverage.
Normal/Add/Screen/Stencil use the existing premultiplied equations, including
HDR Screen behavior. The stage does not add lighting or expose a Model selector.

Bounds: at most131072 projected triangles,4194304 surface pixels and64000000
sample visits per mesh; caller limits may only lower these hard caps. A clipped
surface uses its ROI-intersected bounding rectangle. Cancellation is checked
per input triangle/raster row; failed work publishes no partial surface. The
composition helper validates source pixels and touched destination pixels
before mutation, without rescanning the full destination frame per mesh. It may return
cancelled with a partially changed caller-owned staging buffer, which the
caller must discard on failure, as with the existing CPU accumulation.
Its touched HDR destination channels must be finite with absolute value<=1e30;
alpha remains0..1. Untouched destination storage belongs to the caller.
Homogeneous division uses a1e-200 positive-W numerical floor.

Render.hpp belongs to M3-17 for later numeric mesh staging; it is unchanged in
the geometry milestone. Before authoring is exposed, append new graph shape and
resource records, preserve existing shape values0..3 and all native IDs, and
update this ADR with exact bindings/IDs/version limits. A full paired native/Core
build is required for shared evaluator inputs. New resource/wire fields will
require an explicitly versioned C ABI and snapshot migration, retaining ABI7/
snapshot7 readers for existing graphs. No partial Model selector is deployed.

Numeric evidence must cover cube topology/winding, OBJ index forms, corner
attributes, concave polygons, deterministic reversal, malformed data, budgets,
allocation/cancellation and immutable input. Render evidence must additionally
cover perspective/near-plane clipping, normals/reflection/shear, pixel depth,
transparent particle transfer modes, camera/downsample/PAR/ROI, shutter and
resource/undo/reopen behavior. Compilation alone does not qualify AE2023.
