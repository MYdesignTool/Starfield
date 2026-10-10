# ADR 0039 — Motion geometry and first three modes

Status: numeric geometry, Circle and Look At graph candidates implemented;
public Motion contract remains proposed,
M3-18. Installed native61/CEP63 is unchanged.

## Scope and evidence

The owner's first Motion stage is exactly Light Path, Circle and Look At.
Particle Shape remains the six confirmed entries; this task adds no Shape mode.
M3-17's Model author test release has reached its deployment milestone, with
actual AE qualification and Use Model(s) reference still open. M3-18 can develop
its independent numeric layer while those owner checks remain pending.

The [official guide](https://superluminal.tv/user-guide), checked2026-10-10,
describes circular motion and following/orienting to a light path. Its Spline
emitter section describes light points as a B-spline. This does not establish
Motion's exact path construction, Circle speed units/radius or Look At target
selection. The owner's screenshots establish control labels and defaults only.
Circle calibration and its Origin Type menu have been requested; do not invent
additional modes, menu choices or public unit claims.

An [official Staff reply](https://superluminal.tv/question/orbit-sphere), checked
2026-10-10, describes orbiting particles around a sphere with zero emitter speed
and Circle. This supports preserving the emitted distribution while adding
rotation; that is an inference, not a measurement of the exact radius rule,
axis, angle clock or Speed units. The requested owner calibration remains open.

## Numeric geometry layer

`MotionGeometry` accepts only owned numbers; AE objects, light inventory,
animation histories and name filtering stay outside it. It supplies reusable
math for the later three-mode evaluator, not an advertised Motion node yet:

- Circle: Rodrigues rotation of a supplied position around a supplied origin
  and axis, by an explicit angle in radians. Zero angle is exact identity.
  The public Speed-to-angle mapping and source of the radius are not decided by
  this helper. Coordinates are bounded at1e9 and input angle at1e12 radians.
- Path: an immutable clamped uniform B-spline of degree min(3,N-1), with exact
  endpoints, an adaptively built arc-length lookup and exact analytic tangent.
  Geometry is stored relative to the first point to limit rounding on small
  paths far from zero; exact authored endpoints are retained separately.
  It is one available path representation; the host Motion path construction
  remains a reference gate. Distance queries clamp to its endpoints, with no
  invented looping policy. Zero-length paths keep their position and return
  zero tangent. Nonuniform speed along a straight cubic is also subdivided.
- Look At: a row-major3x3 proper rotation taking a supplied forward direction
  toward a goal, using the shortest arc and weight0..1. It preserves the current
  state at weight0 or a coincident goal. Antiparallel directions use a
  deterministic perpendicular axis. Applying it to an existing basis preserves
  that basis's scale/shear/reflection; it does not replace authored orientation
  with a fresh Euler state. Public goal selection remains unconfirmed.

The path allows 1..256 control points, at most 16385 lookup samples, 262144 span
or subdivision evaluations and depth 16. Subdivision uses each polynomial span's
cubic Bernstein control polygon as an upper bound on arc length; the difference
from its chord is allocated by parameter span. The internal controls' deviation
from a linear parameterization also bounds interpolation error, including a
straight cubic with uneven speed. The tolerance is 1e-6 of the original control
polygon length (minimum absolute tolerance 1e-12); floating point rounding still
applies. A path that cannot meet both error tests within the budget
returns work_limit_exceeded, never a silently coarser curve. Work is cancellable
and failures leave the caller's input untouched. Compiled instances are owned,
immutable and independent of query/frame order.

This layer introduces no parameter ID, match name, graph key, native selector,
wire schema or Core ABI change. `Settings.hpp` continues owning Vec3 and author
value types. The compiled class follows the existing immutable Transform helper
pattern. Build inputs include the new Core implementation; no UI or native
release is made for an unused numeric helper.

The focused MSVC /MT fixture and ASAN run each pass 4759 checks, including
allocation and copy-assignment failure, analytical length comparisons,
degeneracies, cancellation, local-coordinate translation and concurrent reads.
Source 3d6d718 is frozen and passes the full unpublished May2023 SDK build;
the installed 26-file native61/CEP63 pairing remains unchanged. These checks
qualify the mathematical helpers, not reference Motion behavior or AE execution.

## Remaining integration

### Circle graph substage (unpublished)

The append-only Core graph identity is `org.starfieldfx.nodes.motion`, schema1,
with particle input1/output2. Motion-local keys1..6 are mode:uint32,
origin:Vec3, axis:Vec3, angular rate:double in explicit radians/second,
speed random:double percent, and optional existing AgeCurve bytes. Mode1 is
Circle; mode2 now evaluates numeric Look At as described below. Reserved mode0
is Light Path and still rejects until its evaluator exists. No AE disk IDs, author menu or advertised mode is
added by this substage. Old graphs and snapshot3..8 are unchanged; an older Core
rejects an unknown Motion graph rather than silently dropping its behavior.
Full adapter/Core pairing is required before any graph author is published.

Circle rotates the already emitted/integrated distribution around the supplied
origin/axis. Its angle is the time integral of signed angular rate multiplied
by Motion Over Life (percent) and a stable per-node/emitter/particle speed
random factor. Its instantaneous velocity is R*v + omega cross R*(p-origin),
so Auxiliary velocity inheritance sees the actual orbit velocity. Zero rate and
zero integrated angle preserve position; style and authored sprite basis are
unchanged. This is an independent candidate equation, not a measured mapping
of the reference's public Speed100. The pending reference calibration still
controls the author-facing conversion, origin policy and default axis.

Initially supported chains have Force/Transform before Circle; serial Circle
nodes execute in graph order. Different Circle chains of one Particle cannot
merge; a Force/Transform after Circle is explicitly rejected in this temporary
substage, including active zero-weight nodes, instead of being evaluated in an
incorrect coordinate frame. This restriction must be removed with the common
ordered Motion/Transform/Force contract before first-stage author delivery.
Auxiliary prefixes evaluate the parent's Circle at each child birth.

Static graphs compile fixed-size curve coefficients and exact antiderivatives
once per node. Linear/Draw, Hold and the existing shape-preserving Bezier
interpolation share the same values; default is constant100%. Temporal graphs
integrate authored values on the existing absolute30/60/120Hz midpoint lattice
and report the endpoint rate; histories are bounded/cancellable and sampling
is shared by node/time using at most4096 immutable compiled leases, with leases
remaining valid across eviction. Random identity is derived once per particle
and Circle, not at every history tick. Origin/axis are currently sampled frame
geometry, like the existing Transform frame; their animated derivatives are
not yet part of reported velocity. That and ordered downstream frames must be
resolved in the common Motion contract before author delivery. A new optional
sampler metadata proof permits the exact
static clock only when the caller certifies constancy. Equal samples are never
used as that proof. New random purpose22 is appended; old streams are unchanged.
No snapshot stride, Render boundary or Core C ABI change is needed for Circle
position/velocity alone. Light Path remains required, and Look At's author-facing
policy is still open; these numeric Core entries do not close M3-18 or the full goal.

The focused fixture exercises the actual graph evaluator and CPU renderer, not
only a geometry helper: standard and ASAN1764 checks, Transform graph594 and
Model graph559 pass (2026-10-10). Coverage includes exact curve integrals versus
independent quadrature, animated rates, serial/order rejection, immutable graph
and sequence/snapshot round trips, instant orbit velocity and Auxiliary birth
inheritance, actual CPU pixels and Linear chord/Subframe arc behavior,
reverse/concurrent requests, cancellation and every allocation failure through
success. A temporal validation allocation was previously collapsed to
invalid_request; the evaluator now retains the typed allocation/internal error.
Initial fixture Result boolean and Force schema assumptions were corrected;
diagnostics remain under artifacts. This does not qualify AE or unknown public
reference behavior. Source05c37e2 is pushed and git-archive frozen; the full
eight-target May2023 x64 Release /MT build passes with actual exit0 and both
NoPublish switches. The build log/exit/eight hashes and retained26 installed
native61/CEP63 hashes/Core selector are under artifacts/m3-18-motion-circle-*.
There is no author entry and no deployment of this unfinished Motion substage.

### Per-particle orientation and Look At migration (unpublished)

Before Light Path's Orient To Path is authored, each evaluated particle needs
an independent proper rotation after its authored Euler orientation and shared
Transform basis. Append an owned unit quaternion `(w,x,y,z)`, default identity,
to ParticleInstance. Settings.hpp owns this numeric value. Apply it in canonical
world axes after the shared affine basis for Sprite projection (CPU and GPU
scene preparation) and Model geometry. A proper left rotation preserves the
basis's scale/shear/reflection and does not consume one of4096 shared entries
for every distinct particle. Limit To2D uses an XY goal/forward projection.

Snapshot migration is append-only: versions3..8 keep their exact bytes when all
poses are identity. Version9 has a72-byte header, all version8 table counts
(including zeros), a32-byte pose stride field and reserved0, and232-byte
particles: the existing200-byte record followed by four binary64 quaternion
components. Readers accept3..9 and initialize old records to identity; malformed
unit quaternions/header/lengths reject before use. Existing byte/work budgets
remain, without truncation. Old readers reject9 explicitly. Sequence envelope1,
parameter disk IDs and C ABI8 stay unchanged: opaque snapshot versioning is
separate from the fixed ABI prefix. Full Main/Core pairing is required; no
Core-only publication or author release occurs in this substage. Linear shutter
interpolates poses along the shortest unit-quaternion arc; actual Subframe
evaluates each pose. An endpoint-only particle retains its endpoint pose.

Motion schema1 adds optional key7=goal Vec3 and8=forward Vec3. Circle's keys2..4
become descriptor-optional and remain semantically required in mode1. Mode2
requires explicit goal and forward; no public target selection or forward-axis
policy is inferred. The caller supplies forward in the particle's canonical
pre-Euler frame, with any Up Axis choice already accounted for. Look At samples
the goal at the request time and turns the actual Euler/basis/pose-mapped forward
toward it by clamped Motion Over Life percent. Zero weight, coincident goal or a
collapsed basis forward preserves the exact prior pose. Position/velocity,
identity/style and the authored/shared basis stay unchanged. This independent
equation is not proof of reference Starting With/goal selection behavior.

For this intermediate graph contract, Look At follows Circle stages; serial
Look At operations compose properly, and Circle after Look At rejects explicitly.
Force/Transform after Motion still requires the pending complete ordered-frame
contract. These restrictions must be resolved before first-stage author delivery.
Light Path remains required; pose/Look At does not narrow the three-mode goal.

The actual static and temporal graph evaluators now apply the independently
validated Look At values after particle properties and the shared Transform
style. Temporal goals are sampled once per node/request time in a bounded
numeric cache; no AE object enters this layer. CPU Sprite geometry, GPU Sprite
scene preparation and Model geometry consume the same proper world rotation.
Legacy records with identity poses retain versions3..8 and their old stride.

Focused MSVC /MT and ASAN runs each pass1366 checks (2026-10-10), covering
Euler/shear/reflection preservation, serial Look At/Circle-before-Look At,
animated goals, more than4096 distinct orientations, actual CPU pixels and GPU
prepared axes, old3..8/table migration, every truncated9 record, bad quaternion
and header rejection, Linear SLERP, cancellation and every allocation failure
through success. A posed Circle/Cloud now uses its full world axes in the camera
path even without a shared Transform; comparing it with an explicitly authored
3D plane catches the prior billboard branch's dropped depth axis. Circle
regression1766, Transform graph594, Model graph559 and Model particle/snapshot2695
also pass. Logs are artifacts/m3-18-motion-look-at-{tests,asan}.log and
m3-18-look-at-*-regression.log. These are numeric candidate checks, not reference
Starting With/forward policies, GPU hardware or AE qualification. A new frozen
full adapter/Core SDK build is required for this migration before publication;
the prior Circle build cannot qualify these changed shared inputs.

### Point resource capture seam

`MotionPointCapture` is read-only AE adapter infrastructure. A caller supplies
up to 256 composition-local stable layer IDs and explicit local pixel points;
it decides which layers/anchors and which PF time to request. The helper does
not assume Light/Null target rules, Starting With filtering or path ordering.
It converts the supplied effect time to composition time through PFInterface1,
resolves IDs inside the owner's composition, verifies identity/parent comp,
and samples LayerSuite9 layer-to-world matrices once per unique source. SDK row
matrices are transposed for the existing affine inverse helper; source points
are mapped into the owner's layer frame, then into the existing canonical
coordinate system, including PAR and its Z origin.

Only owned numeric points leave the callback; no SDK handle/cache persists.
Malformed IDs/points, missing layers, nonaffine/singular or ill-conditioned owner matrices, invalid
units and nonfinite/out-of-bound results reject. Capture, suite-release or
cancellation failures preserve output. The bounded per-call numeric cache avoids
repeating source matrix sampling for multiple points on one layer. No scripts,
layer edits, host-global caches or idle hooks are added. This seam is compiled
into the main candidate and included in its adapter fingerprint, but is not
invoked by an advertised Motion node or sent across Core ABI yet. Selector/thread
qualification and actual parented/animated AE2023 behavior remain open.

Point capture source 4c9c50c is frozen and passes the full unpublished May2023
eight-target SDK build. Standard and ASAN fake-callback tests each pass 2365
checks. Native61/CEP63's 26 installed hashes and Core selector remain unchanged.

Before actual Light/Null integration, establish AE invalidation dependencies and
mix all sampled source identities/points/times into immutable pre-render state
and the cache GUID. Current main declares camera use, not 3D light use; new light
flags must be implemented and paired in PiPL/runtime before advertising them.
Graph revision alone cannot certify that animated external points are constant.
This is an integration requirement, not a reproduced cache defect in the unused
capture seam. Binding/metadata/ABI changes need their append-only migration plan.

Confirm reference resource/units/Origin Type/Starting With policies before
publishing controls. Then define explicit append-only Motion identity, author
values, curve/history/resource/graph and shutter contracts in this ADR; implement
all three behaviors, native/CEP/preset persistence and full paired SDK build.
Particle/Force/Transform/Auxiliary interactions, reverse frame order, animation,
budgets, undo/reopen and actual AE2023 remain required. Turbulence follows the
first Motion stage; this math milestone does not close either task or the goal.
