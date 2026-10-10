# ADR 0038 — Particle Model geometry and resources

Status: staged implementation, 2026-10-09. Task M3-17.

## Live graph and shutter milestone evidence

Model schema1 metadata and Particle port3 now participate in full dependency
planning while emission/Force/Transform/Auxiliary use separate particle-flow
adjacency. Groups are ordered by Particle/Model UUID; adding metadata cannot
reorder legacy birth partitions. Temporal sampling captures active Model metadata
once at the current time, including remapping/subframes, with no negative-time
or zero-cap captures. Parked and physics-only parent groups need no mesh capture.
Linear shutter validates/remaps Model tables and interpolates matching affine
groups, retaining discrete geometry when resources/member counts change.

The standard RunModelGraphTests.ps1 -Run fixture passes444 checks, including
live cube/imported pixels via CPU/C ABI, Auxiliary groups, Force/Transform,
animated birth shape, metadata sampling, graph/snapshot round trips, endpoint
birth/death/implicit cube/resource changes and bounded Model shutter group pairs.
Existing Texture4322, Birth3348 and Model CPU2695 regressions pass. The expanded
fixture initially used an incorrect PortId name and a nonexistent RenderRequest
member; these were repaired before passing. Numeric integration is not native
resource capture or AE authoring; no Model selector/binary has been published.

The same final fixture passes444 checks with MSVC AddressSanitizer; log
artifacts/m3-17-model-graph-asan-current.log. Intermediate371/440 runs do not
describe the final candidate.

Frozen live-graph source67fc06376729c45991a40122dc8130506d5e4806 at
artifacts/prepared/m3-17-model-graph-67fc063/source passes all May2023 x64
Release /MT targets with both NoPublish switches; log
artifacts/m3-17-model-graph-native-build.log. ModelEvaluation.hpp is included in
adapter fingerprints. All18 installed files still match native60/CEP61; report
artifacts/m3-17-model-graph-installed-hashes.json. No Model author/selector has
been published; native persistence/resource capture remains the next milestone.

### Editable Model author parameters

Before native authoring, schema1 appends optional keys4 origin(vector3 model
units),5 rotation(vector3 degrees),6 scale(vector3 percent,default100),7/8/9
Flip X/Y/Z(uint32 bool),10 Center(uint32 bool),11 Normalize(uint32 bool),12
mesh bounds(opaque48: minimumXYZ/maximumXYZ little-endian doubles),13 Source
(uint32,0 cube/1 OBJ). Existing metadata keys1..3 remain valid. An authored pose
uses keys4..11 and excludes the old explicit matrix key3; no lossy matrix
decomposition invents saved control values. Bounds/source alone do not replace
an explicit matrix. Source0 with a nonzero resource is rejected; Source1 with
zero resource displays the default cube until a mesh is imported. Missing
Source preserves existing numeric resource behavior.

The independent pose contract follows Particle's X-negative/Y-positive/Z-positive
Euler convention, applying per-axis scale/flip, optional normalization by the
largest bounding-box extent, optional centering, then rotation and origin.
Bounds/default cube are numeric, never queried from a host during graph
evaluation. These equations are our explicit author contract, not proven
reference numeric parity. The matrix must remain bounded affine. Native resource
capture will validate stored bounds/mesh identity; no filepath reaches Core.
This unreleased schema1 extension changes no existing native disk ID, snapshot8
or ABI8 layout. Earlier Model-only numeric candidates reject new keys rather
than silently reinterpret them. A full paired build remains required.

### Native Model author layout reservation

The unpublished Model author reserves native kind5 and match name
`org.starfieldfx.node.model`. Its first18 physical streams are Source1,
Import OBJ2, Mesh3, Mesh Revision4, Offset X/Y/Z5..7, Angle X/Y/Z8..10,
Scale X/Y/Z11..13, Flip X/Y/Z14..16, Center17 and Normalize18. Disk IDs
1501..1518 correspond one-to-one to those streams; these are new IDs in
the new Model effect, never replacements for released controls. Source uses
native choices1 Cube/2 OBJ, graph choices0/1. The mesh is an invisible
arbitrary parameter with PF_ParamFlag_NONE (the SDK arbitrary selectors
provide discrete interpolation); Source/Revision are supervised constants,
Import is a supervised button, and the pose controls can animate. Revision
is an exact integer0..2147483647, with zero meaning no imported resource.

The shared node record will append existing metadata after18: layout19/20,
connections21..85, UUID86..93, sync guard94 and total95 including input0.
Kind5 is not admitted by the existing binding6 reader/writer until a separate
binding migration is implemented. No Model AEX, palette or Particle menu is
published from this control-registration milestone. Main mesh mirrors and
import transaction/resource binding IDs remain unallocated at this point.
The packed native version stays60 until the complete paired author release.

ModelControls registers actual May2023 parameter definitions, captures checked
native values into owned numeric mesh/pose data and encodes editable Model
graph parameters. It does not retain parameter pointers or handles in Core.
An OBJ import can prepare a validated new handle without changing live controls;
the later UI transaction owns committing or disposing that handle. File-dialog,
sync/undo transaction, immutable render capture and CEP integration remain
required before exposing this author. A failed registration disposes an
unaccepted default mesh; accepted defaults belong to AE.

The final editable graph fixture passes559 checks in standard and ASAN builds;
resource2076/mesh arbitrary4915 regressions also pass. The actual May2023 control
registration/capture fixture passes452 checks in standard and ASAN builds.
It covers every registration failure, accepted default
ownership, exact integer revision, native type/value rejection, editable graph
round trip/live evaluation, imported/parked resource isolation, geometry-owned
capture and import preparation failure. Early compilation omitted SDK constants
and used incorrect fixture API/member names; those were corrected before the
passing run. Numeric/SDK evidence does not establish real AE UI/undo support.

Sourcec65d5f79728ae2340b3cd9dbfa37b5cbb46e2f4f is frozen under
artifacts/prepared/m3-17-model-controls-c65d5f7/source and passes all seven
May2023 x64 Release /MT targets with both NoPublish switches. The main AEX
compiles ModelControls.cpp; adapter fingerprints include its source/header and
ModelLayout.hpp. Logs/hashes: artifacts/m3-17-model-controls-native-build.log,
m3-17-model-controls-build-hashes.json and m3-17-model-controls-installed-hashes.json.
All18 installed native60/CEP61 files are unchanged. No Model node, kind5 binding,
resource mirror or Particle selector is published from this helper milestone.

### Native Model module and binding7 migration

The next unpublished author candidate implements kind5 and the reserved Model
effect. Physical controls/metadata remain1..94 (num_params95); no existing kind
or physical stream is shifted. Synthetic binding fields19..24 contain six
numeric mesh bounds, independently of physical metadata at those same indices.
They are constants in the private binding record and never expression aliases.
Model bindings include exactly scalar fields1 Source,4 revision and5..24
pose/bounds; fields2 button and3 mesh are not numeric bindings. Pose5..18 alone
consumes14 animation aliases, reusing the existing bounded512-slot bank.

Private tag0x8002 writes version7 only when a Model node is present, retaining
version1/2/6 selection for graphs without Model. Readers accept1..7; kind5 is
rejected in versions1..6. Model fields must be scalar, all22 required fields
must exist, buttons/ARB must be absent, and constant bounds/Source/revision
cannot consume alias slots. Unknown kinds, missing fields, duplicate aliases
and invalid source/revision/pose/bounds remain typed errors. Graph envelope1,
Model schema1, existing node schemas, snapshot8 and ABI8 stay fixed.

UI compile validates an imported SFMG1 mesh through its Model arbitrary stream,
copies it while the borrowed AEGP stream value is live, and records numeric
bounds; render playback reads the constant record/pose aliases only, acquiring
no AEGP mesh stream and executing no file access. Source Cube or unimported OBJ
needs no parked asset. Resource identity is the Model UUID for an imported OBJ;
default cube identity/revision are zero. The next resource-mirror transaction
must carry the actual mesh to SmartFX before this candidate is published.

Model's metadata output maps to Particle input3. Particle-flow input1 and all
existing connections retain their meanings; invalid destinations are rejected
by typed graph validation. The new Model PiPL uses the existing node pass-through
selector implementations/flags and the reserved match name. Candidate build
uses explicit IncludeModelCandidate with both NoPublish switches; default
deployment lists are unchanged until full resource/CEP authoring and the new
paired version/rollback plan are complete. No new AE host qualification follows
from this module/binding milestone.

The final Model module/binding fixture passes2809 checks in both standard and
MSVC AddressSanitizer builds, plus the reused452 control fixture. It compiles
the actual Model NodeEffects branch and tests95 registered parameters/ARB
callbacks,14 alias checkout/checkin paths, versions1..7, invalid records and
UI graph compilation/mesh ownership/typed Model output. Its initial UI fixture
omitted Particle's Random Limit default1, failing at stream303; the corrected
fixture passes. Logs: artifacts/m3-17-model-native-binding-{tests,asan}.log.
Actual AE UI/undo/persistence and complete resource authoring remain open.

Frozen source7f5c7d8 at artifacts/prepared/m3-17-model-native-7f5c7d8/source
passes all eight May2023 x64 Release /MT outputs, including the actual Model
AEX/PiPL, using IncludeModelCandidate and both NoPublish switches. Initial
SDK-path setup and a missing ParticleTransform.cpp link dependency were fixed
before refreezing and success. The old binding8441/camera12 regression passes;
all18 installed native60/CEP61 files remain unchanged. Logs/reports:
artifacts/m3-17-model-native-build.log, m3-17-model-legacy-binding-regression.log,
m3-17-model-native-build-hashes.json and m3-17-model-native-installed-hashes.json.

### Renderer mesh mirrors and immutable capture plan

The next unpublished main manifest30 appends256 hidden arbitrary mesh mirrors
at streams755..1010/disk1900..2155 and exact integer count1011/disk2200;
main num_params becomes1012. Existing0..754/IDs/defaults remain fixed. Model
node controls and binding7, graph schemas/envelope1, snapshot8 and ABI8 stay
unchanged. The complete author release must increment the packed native build
and pair every adapter/Core/panel; these candidate controls are not deployed alone.

SFMR1 wraps SFMG1 in a64-byte little-endian header: magic SFMR, version1(u16),
header64(u16), total bytes(u32), CRC32 of bytes16..end(u32), resource UUID16,
revision(u32), then28 zero reserved bytes. Empty slots have zero UUID/revision
and no mesh. Imported entries require nonzero UUID/revision1..2147483647 and
one bounded SFMG1 payload; total at most8MiB+64. CRC covers identity/revision
and mesh. The mirror arbitrary selectors implement independent copies,
discrete interpolation, flatten/restore/compare and bounded text before registration.

UI installation sorts imported Model UUIDs and copies their validated meshes
into corresponding renderer mirrors as part of the same graph/alias transaction.
It retains exact previous values for rollback and clears only the former occupied
tail. The renderer captures only resources requested by evaluated Model particles
across the current frame/shutter samples. PF parameter checkout/checkin owns each
borrowed mirror; identity/revision/bounds are verified against compiled graph
metadata before producing owned C ABI numeric arrays. No sibling AEGP scan,
file read or host handle reaches Smart Render/Core. Empty, parked, negative-time
and zero-cap paths must not decode unused mirrors. Mesh numeric bytes and
identity participate in SmartFX GUID mixing. Actual AE undo/reopen remains a gate.

ModelMirrorParameter implements SFMR1 persistence and the reserved registration
helper, compiling in main/node candidate projects. Standard and ASAN fixtures
pass8021 checks (including the reused452 controls): empty/imported values,
every byte mutation/truncation, revision bounds, all arbitrary selectors,
deep copies/discrete interpolation/text, accepted-default ownership/registration
failures and cancellation after unlocking. Logs:
artifacts/m3-17-model-mirror-{tests,asan}.log. Main manifest29/755 parameters
remain registered; the new helper is not called until mirror transactions and
immutable capture are implemented. This foundation is not a deployed author.

Frozen source0d868eb35fa00ba50619460722cc9b9f504cd1bb under
artifacts/prepared/m3-17-model-mirror-0d868eb/source passes all eight May2023
x64 Release /MT outputs with IncludeModelCandidate and both NoPublish switches.
ModelMirrorParameter is compiled in main/node candidate modules. Logs/hashes:
artifacts/m3-17-model-mirror-build.log, m3-17-model-mirror-build-hashes.json and
m3-17-model-mirror-installed-hashes.json. All18 installed files still match
native60/CEP61. No registration call, publication or new host qualification.

### Renderer resource transaction and immutable capture implementation

The unpublished main now calls the manifest30 registration helper (1012
parameters) and routes disk1900..2155 arbitrary selectors to SFMR1. Native
graph commits install sorted UUID mirrors before graph publication. Old values
and stream references remain owned through readback verification and exact
rollback. Count is written last; unchanged UUID/revision/bounds skip author
mesh reads and writes. Failed partial setters and silent writes are detected.
Model transaction disposal precedes renderer-reference disposal. Borrowed AEGP
values are disposed before their stream references, including source mesh reads.

SmartFX captures only evaluated imported Model resources, deduplicated across
shutter samples. Every successful PF checkout is checked in on decode/type/error
paths. The captured geometry is copied into typed, owned ABI8 arrays; no AE
handle survives into Core. UUID/revision/counts and numeric mesh arrays mix into
the cache GUID. Corruption, identity/revision/bounds mismatch and cancellation
publish no resource set. Parked, zero-cap and negative-time frames skip checkout.

Standard and ASAN resource-bridge fixtures each pass922 checks, including the
reused452 controls, transaction failure/rollback, capture ownership/shutter
deduplication and actual ABI8 pixels after all fake host handles are released.
Native binding2809 and legacy binding8697/camera12 checks pass. Logs:
artifacts/m3-17-model-resource-bridge-{tests,asan}.log,
m3-17-model-resource-native-binding-tests.log and
m3-17-model-resource-legacy-regression.log. These are SDK fake-host/numeric
evidence, not AE2023 UI/undo/reopen qualification. Full frozen SDK build remains
required; OBJ import/complete author publication are still pending. Installed
native60/ABI7/CEP61 is unchanged.

The resource-bridge source eae10aecb11f0a50dae8d1a3605d6750dc6eccaa,
frozen at artifacts/prepared/m3-17-model-resource-bridge-eae10ae/source,
passes all eight May2023 x64 Release /MT targets with IncludeModelCandidate
and both NoPublish switches. Main1012 registration/SFMR1 dispatch and SmartFX
entry points are compiled. Log: artifacts/m3-17-model-resource-bridge-build.log;
build and installed hash reports: m3-17-model-resource-bridge-{build,installed}-hashes.json.
All18 installed native60/CEP61 files remain unchanged. OBJ/complete author
publication and real AE qualification remain pending.

### OBJ import author transaction

The unpublished Import OBJ button uses a Windows file chooser owned by AE's
main window, with OFN_NOCHANGEDIR. It reads only the selected file, bounds the
text to8MiB and rejects unreadable, empty, growing or invalid OBJ input before
changing any author stream. No filepath is saved; only SFMG1 numeric geometry
is persisted. Cancelled selection performs no author/graph write.

After successful preparation, one UI undo group retains exact Mesh/Revision/
Source/guard values. Under the Model guard94 it sets Mesh3, increments exact
Revision4 (rejecting exhaustion at2147483647), sets Source1 to native OBJ2,
verifies each readback, releases the guard and publishes through the existing
native Source edit. Renderer mirrors/graph publication retain their own verified
rollback. Import failure restores author values and verifies them, reporting
rollback failure distinctly. Old borrowed values outlive their stream references;
the temporary prepared handle is disposed after host copies are complete.
This changes no disk ID, native95 layout, binding7, ABI8 or graph schema.
File dialog and actual AE undo/save/reopen remain host qualification gates.

RunModelImportTransactionTests.ps1 -Run and -Run -Sanitize each pass1019
checks (including reused452 controls), using actual AEGP transaction functions
and a graph-publication test callback. Every author write partial failure,
silent setter, publication failure, rollback failure reporting, revision
exhaustion, guard rejection and value/stream/handle ownership is exercised.
Native module/binding2809 checks also pass. Logs:
artifacts/m3-17-model-import-transaction-{tests,asan}.log and
m3-17-model-import-native-binding-tests.log. The UI file chooser is not driven
by this fixture; full SDK compilation and actual AE qualification remain open.

### Earlier native mesh persistence milestone

### CEP-readable author bounds append

Before the complete author release, append six hidden constant float sliders
at Model streams95..100/disk1519..1524, after existing metadata/guard94.
Model num_params becomes101; authored1..18 and metadata19..94 keep their
indices and disk IDs. Bounds store minimumXYZ/maximumXYZ of the parked numeric
mesh, with cube defaults(-.5,-.5,-.5)/(.5,.5,.5). They are derived resource
metadata, not editable pose controls or animation aliases. Import commits and
rolls them back with Mesh/Revision/Source. UI compilation verifies imported
mesh bounds against these values; renderer binding7 continues to use the same
six synthetic constants19..24, not physical95..100. Main manifest30/1012,
Model schema1, graph keys and ABI8 remain unchanged. All Model candidates so
far are unpublished; the append does not alter installed native60/CEP61.

CEP can construct Model source/pose records from ordinary author controls and
this small bounds bank without reading custom-value handles or exporting a mesh
on every poll. Cube mode projects cube bounds/zero resource and revision while
retaining parked author mesh data. Portable preset assets still require a
separate explicit bounded numeric export/import transport before publication.

The isolated CEP author delta is tracked under tools/candidates and prepared by
tools/Prepare-ModelPanelCandidate.ps1 only beneath artifacts/prepared. It adds
Model output1 and a separate Particle Model input3; particle-flow endpoints keep
their existing ports. Normal snapshots read Source, pose, Revision and the six
numeric bounds only. They never read CUSTOM_VALUE or export mesh bytes during
polling. A parked asset descriptor remains outside the graph projection so
switching Cube back to OBJ restores its UUID/revision/bounds without losing the
native numeric mesh. Resource metadata is read-only to ordinary graph edits;
native writes require exact revision and bounds agreement before mutation.
Actual mesh export/import and imported-node duplicate/preset transactions are
still pending; no partial Model palette or Particle selector is published.

Focused fake-host evidence: standard/ASAN native module3040 and import1559
checks each (including the existing452 control checks). Physical95..100 must
not enter binding records; six stale-bounds variants are rejected. The isolated
CEP author fixture has74 checks, including source switching, tiny bounds
corruption, typed rejection before writes and the actual graph coordinator.
Texture80/Cloud49/Birth51 and the existing full gateway transaction fixture
also pass against this candidate. These results do not qualify an AE host.

### Bounded native preset mesh export seam

Before connecting CEP preset assets, reserve a private synchronous Model
PF_Cmd_COMPLETELY_GENERAL message: magic0x53464d58, version1, exact struct size,
operation1(export), expected UUID/source/revision and a caller-owned byte sink.
Only fixed-width fields, borrowed callback/context pointers and SFMG1 numeric
bytes cross the native module boundary; no allocator, STL object, AE handle or
PF callback crosses it. The module verifies constant UUID/source/revision,
guard0 and all six bounds, copies and validates its ARB through its own PF
context, disposes host values before encoding/calling the sink, and publishes
no payload on failure. Revision0 has no preset asset and returns zero bytes.
Render-only contexts are rejected. This seam is read-only and changes no graph,
project stream, alias, mirror, undo group, public IDs or persistent schema.

The eventual session-resident host caller must invoke this seam on UI idle,
resolve the pinned project/comp/layer/effect afresh, and copy the numeric payload
into its own bounded storage. It must not poll/export meshes in normal graph
snapshots or invoke script execution reentrantly from a script command hook.
The async CEP transport and exact import/rollback transaction remain separate
work before any Model author release. Generic context availability and actual
AE preset/undo/reopen behavior remain host gates; fake suites do not prove them.

The export seam is now routed by the actual Model module and included in the
node project and adapter fingerprint. Standard/ASAN native-module fixtures each
pass3483 checks, including the existing452 controls. They exercise request
rejection, source/revision/UUID/guard/bounds matching, zero and parked assets,
owned SFMG1 bytes, cancellation before and during copying, host/read/sink errors
and bad geometry without project changes or AE alerts. Sink storage must be
discarded whenever the message reports an error; it is never a committed preset.
UI idle/CEP wiring and import/rollback remain pending before publication.

Import source da76183c82f9584367561e2742f0ac76c411e397, frozen at
artifacts/prepared/m3-17-model-import-da76183/source, passes all eight May2023
x64 Release /MT outputs with IncludeModelCandidate and both NoPublish switches.
The Windows chooser and actual NodeEffects button route are compiled. Log:
artifacts/m3-17-model-import-build.log; build/installed reports:
m3-17-model-import-{build,installed}-hashes.json. All18 installed native60/CEP61
files remain unchanged; Particle selector/complete CEP/presets and real AE
qualification remain open. No Model candidate is deployed.

ModelGeometryParameter stores only a bounded, validated SFMG1 mesh in an AE
arbitrary handle. It owns no graph identity, filenames or external file lookup;
resource UUID/revision belong to the future Model author and immutable capture.
The future registered parameter is constant/discrete, holding its left mesh for
interpolation fractions below1. All arbitrary selectors are implemented before
registration: default cube, copy/dispose, flat size/flatten/unflatten, compare,
interpolate and bounded SFMODEL1 hexadecimal print/scan. Read/create/callback
errors publish no handle; copies own independent host storage. Handle bytes are
copied and unlocked before numeric decode/cancellation. Corrupted payloads are
rejected on reads and serialization, not passed to Core or saved as valid meshes.
Disk ID selection and native bindings remain a separate explicit migration before
Model registration; this helper takes the caller's expected disk ID. No new
parameter, selector flag or Model effect is registered in this persistence step.

RunModelGeometryParameterTests.ps1 -Run and -Run -Sanitize each pass4915
checks using the local May2023 SDK/fake handle callbacks. Cube/imported OBJ
mesh round trips, every truncated/corrupted byte, all arbitrary selectors,
independent copies, unlock-before-cancellation, host/C++ allocation failures,
failed lock disposal and bounded scan preflight pass. Logs
artifacts/m3-17-model-parameter-{tests,asan}-current.log. The first fixture used
ModelPosition.x instead of value.x; the subsequent boundary fixture incorrectly
claimed an allowed maximum span while supplying a one-character buffer. ASAN
found that fixture error; the count was corrected to one-over-limit and the
text prefix length to9 before recording passing evidence. No AE save/undo/reopen
behavior or registered author is inferred. Main AEX/fingerprint include the
helper; no existing native IDs, schemas or packed version are changed here.

Frozen persistence sourcea1751b9b761340e07f2b0003088e83d685c9bc88 at
artifacts/prepared/m3-17-model-parameter-a1751b9/source passes all May2023
x64 Release /MT targets with both NoPublish switches; main AEX compiles the
helper. Log artifacts/m3-17-model-parameter-native-build.log. All18 installed
native60/CEP61 hashes remain unchanged; report
artifacts/m3-17-model-parameter-installed-hashes.json. Native Model registration,
resource capture, Particle selector and isolated CEP authoring remain pending.

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

Graph planning distinguishes full dependencies (including Model inputs) from
particle-flow edges used by emission partitioning/Force/Transform/Auxiliary.
Model nodes must never become emitter branches.
Particle branch partitions use Particle UUID order independently of metadata
dependency readiness, retaining particle identities when a Model is connected.
Model groups are catalogued by all connected Particle UUIDs, including parked
ones, so the root-only Auxiliary
copy retains group indices. Resource presence is checked only for groups used by
evaluated Model particles; a parked group does not require a captured mesh.
Auxiliary parent-prefix simulation uses logical particle centers/size/color and
does not request mesh assets at historical births. Child geometry comes from its
own Model inputs. Temporal Model metadata is sampled once per active Model node
at the current evaluation time, with no host sampling on negative/zero-cap frames;
animated Particle shape at birth selects the already captured current-frame group.
Temporal evaluated_nodes uses full dependency order with UUID ties.

The Model render contract also owns MotionBlur.hpp's shared style transport.
Linear shutter interpolation validates and remaps Model group indices. Matching
member counts/resource IDs interpolate affine coefficients; implicit cube is a
single identity instance. Changed resources/member counts retain the selected
endpoint's discrete geometry (the first endpoint when that particle exists),
consistent with other discrete appearance values. Model matrices are never
copied per particle; group-pair results are shared. Subframe sampling evaluates
the current Model metadata independently at each shutter time.

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

### Private Model export transport candidate

The session-resident Host owns a second Window command, `Starfield Prepare Model
Asset Export`. Its command hook only queues work. UI idle, on the entry thread
and under the existing idle-to-idle running guard, executes fixed script entry
points; it does not invoke ExecuteScript from CEP's synchronous command stack.
That guard and the thread check do not establish exclusion against a different
outer ExecuteScript or modal callback on the same UI thread. Normal panel polling
never queues exports or executes these scripts.

A single volatile version1 session carries only plain numeric/hex data, expires
after60 seconds, and caps one SFMG1 mesh at8MiB. The request line contains a
32-character hexadecimal transfer ID, project root/comp/layer numeric identities,
Model UUID, native Source and revision. Host reacquires all references, checks
the project root, exactly one main renderer and exactly one matching Model, then
synchronously calls the existing private SFMX message. All effect/stream/AEGP
script-result handles are disposed inside the callback; only owned numeric mesh
bytes survive to a later idle callback.

Host sends at most eight32768-byte pages per idle pass, with a25ms soft deadline
checked between calls. Each script argument is validated hex or bounded numbers;
no arbitrary caller text is interpolated. ExtendScript keeps at most256 hex
pages, enforces exact ordering/length and verifies pinned target, graph revision,
Source, mesh revision, guard and six bounds before accepting or serving data.
Cancelled, expired, stale or failed transfers discard pages and native storage.
This protocol performs no project writes, selection changes, file access or
host-wide configuration. Preset mesh codecs and asset import/whole-graph rollback
are separate pending work; this transport does not advertise a usable Model
preset pipeline. Actual AE2023 UI-idle ExecuteScript/generic context and timing
remain qualification gates. The candidate stays isolated from installed60/61.

#### Modal/idle acceptance gate — static review, 2026-10-10

The owner raised these hypotheses; none is a reproduced AE defect. A read-only
check confirmed that deployed native60 Host still matches its frozen90b7a2b
bundle and receipt (SHA256
`8F17C43E7D2FA6658DBF2F732DF25CE069654A0249CAA4B62713B0DD84BFFFCF`).
That frozen StarfieldHost.cpp contains no Model asset step. The idle call to
step_model_asset_host was introduced later by3c10b5d and is not deployed.
Accordingly these are Model predeployment gates, with no immediate runtime fix
or installation change authorized by this review.

- PresetsUI.cpp:53..55 invokes app.executeCommand or alert inside
  AEGP_ExecuteScript. If a modal message loop admits Host idle before that outer
  call returns, queued or active Model work could invoke another ExecuteScript.
  Verify exclusion across the outer script/modal scope, including early returns
  and failures; the UI-thread comparison alone cannot close this gate.
- ModelImportUI.cpp:24 uses GetOpenFileNameW; EditorPresetPicker.cpp:133 uses
  DialogBoxIndirectParamW. Exercise queued and active Model transfers while each
  dialog is open, cancellation and dialog return. Model script work must not be
  admitted through a nested modal loop; deferred work must resume safely after
  the outer operation finishes.
- Record Model import separately: after the file chooser returns, revalidate
  the current target identity and author state before writing guard94, Mesh3,
  Revision4, Source1 and bounds. Current import code reads revision from live
  streams after the dialog, but that alone does not prove that callback params,
  target identity and the other captured state stayed consistent. The reported
  stale-revision/intermittent-import-failure scenario remains a hypothesis.
  Acceptance needs safe rejection or a consistent commit after any intervening
  state change, exact rollback and no lost update.

Fake-host and SDK compilation evidence do not exercise AE's modal message pump.
These gates remain open until the candidate has explicit scope protection and
target/state validation evidence, followed by actual AE2023 modal-path checks.

The private write seam uses SFMW/version1, not SFMX or the Core ABI. Its
borrowed byte span must remain alive for the synchronous Model generic call.
It checks UUID and expected native Source/revision/guard before changing Mesh,
six author bounds, revision and Source under guard94. Desired revisions are
restored exactly, including a parked mesh while Source=Cube. Revision0 with an
empty span explicitly resets the default unit cube/bounds; other revisions need
a valid bounded SFMG1 whose derived bounds exactly match the request. Every
setter is read back; any failure restores all touched fields in reverse order
and reports rollback failure separately. Old values and refs are callback-owned.
The caller owns the outer undo group and renderer graph/mirror transaction.
This seam creates no independent undo group, publishes no graph and writes no
pose or UUID. It must not be exposed as an independently usable preset operation
until Host import transport and complete Add/Replace/duplicate rollback are
connected. No persistent IDs, schemas or parameter types change.

### Portable preset files, staged version3

Preset version3 adds `modelAssets` entries containing the owning Model node UUID,
exact positive revision, six derived bounds and lowercase SFMG1 hex. Existing
versions1/2 remain readable and keep their texture layer-name resource map.
The graph remains limited to24KiB; each mesh stays8MiB and total numeric assets
64MiB. File text has a separate128MiB+256KiB character bound and128MiB+1MiB
UTF-8 byte bound. Ordinary gateway request bounds do not increase.

Explicit preset Save collects meshes from the immediately captured pinned
snapshot. Source=Cube also exports a parked imported mesh. Encode/decode checks
graph UUID/revision/bounds against every portable asset; default cubes need no
asset. Files travel through explicit32768-character pages with fixed transfer
IDs, exact sequence/length, expiry and cancellation, rather than one large CEP
evalScript argument or reply. Reload preserves only bounded plain session data;
file objects are callback-owned. No file transport runs during normal polling.

The user chooses the save/import path through the existing file dialogs. Saving
prepares a unique sibling temporary file, verifies its complete text, then uses
renames to preserve/replace an existing destination. Failed publication restores
the previous destination and reports any failed restoration with the retained
backup path. Success removes only the transaction's own temporary backup.
Preset file IO does not mutate the AE project. Until native whole-graph Model
asset restore is connected, imported meshes must be rejected before Add/Replace
project mutation rather than being accepted with missing geometry. Remapping
Model UUIDs and preserving exact resource metadata belong to that next step.

### Complete effect backup for Model graph transactions

Whole-graph Model mutations need a full effect backup, including animation,
expressions, arbitrary values and renderer resource mirrors. Recreating effects
from the current frame's ordinary manifest is insufficient for that rollback.
The UI transaction uses the SDK's AEGP_DuplicateEffect within its single outer
undo group. All initial metadata/names/positions/flags and backup UUIDs are
prepared before mutation. It backs up the unique renderer and at most63 nodes;
third-party effects are neither copied nor removed.

The existing hidden integer sync guard reserves value2 for transaction backups:
node guard indices/disks and renderer index40/disk42 stay unchanged, as do their
types and valid ranges. Value0 remains normal and value1 remains a write batch.
A backup node receives a fresh temporary UUID and guard2; its Active flag is
cleared. Native compilation and CEP inventories ignore guard2, and renderer
search ignores renderer guard2. The temporary UUID also prevents an alias from
matching a backup. No persistent public ID/schema or effect identity changes.
Normal polling does not create backups. Guard2 leftovers cause a fresh backup
request to fail with diagnostics rather than treating them as ordinary nodes.

On graph failure, the transaction deletes surviving active Starfield effects,
restores the backup UUIDs/names/flags/order and activates the saved renderer last.
This restores the renderer's complete saved graph/mirrors and full native node
animation without sampling it into static author values. Original layer identity
and all non-Starfield effects stay in place. All effect/stream references are
callback-owned and released before an ExecuteScript or structural edit.
Successful publication discards only this transaction's backups; deletion or
restoration failures are reported separately. SDK errors and fake-host evidence
do not qualify actual AE duplication, undo, or expression identity behavior.
The helper remains unpublished until the Model asset/whole-graph Host route is
connected and the full author candidate can be deployed as one pairing.

2026-10-10 checkpoint: the helper and guard2 inventory exclusions are implemented,
with focused fake-host and ASAN evidence recorded in testing.md. The helper is
not yet called by the whole-graph Host route. Complete SDK pairing, AE duplication
and undo/expression identity checks, and the modal/idle gate above remain open.
The owner requested a staged close and pause; installed native60/CEP61 is retained.

### Whole-graph Model transaction executor

The UI Host route will supply one callback-owned plan: transaction UUID, desired
node UUIDs, layer/time, numeric Model assets and prepare/commit callbacks. All
SFMG1 bytes, derived bounds, revisions, UUID uniqueness and the64MiB total are
validated before the undo group or any project write. The borrowed spans remain
alive and unchanged until the synchronous executor returns; no host objects or
allocator ownership cross the private generic message boundary.

One balanced SDK undo group encloses complete effect backup, structural/ordinary
author preparation, private SFMW writes and verified graph publication. Model
targets are reacquired by UUID after structural edits, guard2 backups are excluded,
and every asset write captures its expected current Source/revision/guard. No
effect, stream, value or handle is held across either script callback. The commit
callback must validate the native graph acknowledgement before returning success.
Failures, cancellation and callback exceptions restore the complete effect backup,
including earlier successful asset writes. Restore errors are reported separately.
After publication, backup cleanup or EndUndoGroup errors retain committed=true
and have separate diagnostics; a caller must not retry a committed mutation as if
it had failed before publication. The executor does not schedule work or select
layers. Only the session-resident Host's admitted UI route may invoke it.

This private executor adds no persistent IDs, schema or Core ABI. Its callbacks
still require the modal/script exclusion and target revalidation gates above.
It remains unpublished until the Host request/asset transport, CEP preparation
and verified commit callbacks are connected as the complete author pairing.

### Explicit Model graph transport

The candidate uses a separate queued Host command for graph mutation. CEP stages
one pinned graph request and numeric asset descriptors, then exact ordered32KiB
hex pages in a volatile ExtendScript session. Upload and Host download perform no
project writes. Single mesh8MiB, total64MiB,63 assets and64 desired IDs are checked
before mutation. The Host pulls bounded ASCII metadata/pages during idle and owns
all byte buffers for the synchronous executor. There is no file path, arbitrary
script text, host object or allocator in the protocol, and no transfer in polling.

The session's prepare/commit callbacks revalidate the pinned target and author
stamp. Prepare materializes identity/layout/link records while deferring Model
author controls until SFMW has restored the requested assets. Commit writes those
controls and publishes through the existing native graph receipt, then verifies
the resulting native author records. The native executor owns the only undo group
and full rollback. Cancellation/expiry before publication restores the backup;
published results are retained as committed even if cleanup, undo or notification
fails. A caller must surface those diagnostics without replaying the mutation.

The Host rechecks numeric project/comp/layer identity before both callbacks.
Cancellation during native asset validation/writes is numeric-only; it never
executes another script while an effect reference is owned. Modal/script exclusion
and actual AE timing remain the deployment gates above. No persistent ID, Core
ABI or installed pairing changes in this transport stage.
