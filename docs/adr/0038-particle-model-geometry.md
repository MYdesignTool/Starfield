# ADR 0038 — Particle Model geometry and resources

Status: staged implementation, 2026-10-09. Task M3-17.

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

### Numeric triangle milestone

ModelScene.hpp stages a single mesh using an explicit affine row-vector
model-to-layer-pixel matrix and the existing FrameSpec/Camera values. This
avoids guessing Particle Model menu/size semantics while reference menus are
pending. It adds no field to RenderRequest, ParticleInstance, C ABI or snapshots.
Layer pixels have downward-positive Y; the eventual Particle pose adapter must
convert canonical upward-positive Y exactly once. Matrix coefficients and
camera scalars are bounded at1e12. The camera layer-to-view matrix is affine.

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
