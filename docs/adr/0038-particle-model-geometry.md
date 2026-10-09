# ADR 0038 — Particle Model geometry and resources

Status: staged implementation, 2026-10-09. Task M3-17.

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
