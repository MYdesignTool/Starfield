# ADR 0039 — Motion geometry and first three modes

Status: numeric geometry and all three numeric graph modes implemented;
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
the prior Circle build cannot qualify these changed shared inputs. Source
da17da8 is now pushed/frozen and its full double-NoPublish SDK session67021
returned actual terminal exit0. After an initial approval-service usage failure,
the build log/exit and all eight new output hashes were read back. All26 installed
native61/CEP63 hashes and its Core selector/runtime still match. Evidence is
artifacts/m3-18-motion-look-at-sdk-* and motion-look-at-installed-hashes.json.
No Motion author is deployed; this does not qualify the remaining reference
policies, GPU hardware or AE execution.

### Light Path numeric travel seam (implemented, unpublished)

Extract Circle's fixed-storage validated curve clock into a shared immutable
helper without changing its integral or endpoint value conventions. Add an
interval integral so an explicitly delayed path clock can integrate from the
supplied delay to the particle's actual age. Over Life uses the actual age/life
fraction; it is not restarted after delay. Nonnegative delay seconds and signed
canonical distance units/second are numeric inputs, not a claimed mapping of
the reference Max Delay or Speed controls. Speed attenuation takes an explicit
stable random sample0..1; this seam does not assign a Delay Random distribution.

A compiled travel helper owns the supplied1..256 points, existing bounded
B-spline/arc-length table, validated clock and settings. It reports displacement
S(distance)-S(0), preserving the existing emitted distribution and exact zero
motion, and adds the clipped path derivative to the caller's velocity. This is
an independent displacement-field candidate, not evidence that the reference
anchors particles this way. The current scalar query clamps to endpoints, as
the geometry helper already does; loop/extrapolation/reference end policy is
still open and cannot be advertised from this choice. Caller-supplied historical
distance/rate can use the same spatial query without rebuilding the spline.

Optional numeric path alignment composes a proper rotation from the actual
Euler/shared-affine/pose forward to the signed tangent, using the existing pose
helper and XY projection for Limit To2D. Zero motion or zero tangent preserves
the exact existing pose. Commit position, velocity and pose together only after
every input/output check succeeds. Queries allocate no storage, and compiled
copies preserve strong assignment failure safety. Expose capacity-based owned
storage size so the later graph/resource cache can enforce a total byte budget.

This seam adds no public control, graph mode evaluator, native ID or ABI change.
Actual mode0 graph/history/Auxiliary integration, resource dependencies and
whole-request work/memory budgets remain required next; do not replace them with
passing helper tests or publish the three-mode author prematurely. The complete
ordered Force/Transform/Motion contract and owner reference calibration remain
requirements of M3-18.

MSVC /MT standard and ASAN each pass1216 checks (2026-10-10), including an
independent segmented quadrature for all four curve interpolation modes,
unchanged Circle clock values, a short late interval at lifetime1e6, delay and
explicit random sample, preserved emitted offset/identity/style, signed tangent
and Limit To2D, Euler/shear/reflection, exact signed-zero/zero-length state,
atomic output/basis failure, every path/copy allocation failure and cancellation,
no-allocation unordered queries and four concurrent readers. Logs are
artifacts/m3-18-motion-path-travel-{tests,asan}.log/exit.txt. Circle graph1766 and
Look At graph1366 regressions pass at actual exit0 after clock extraction;
path-clock-*-regression.log/exit.txt. CMake and the Core SDK project compile
MotionPathTravel.cpp, and adapter fingerprints include the shared new headers.
Source874f852 is now pushed/git-archive frozen and its full eight-target May2023
x64 Release /MT build passes with actual terminal exit0 and both NoPublish
switches. MotionPathTravel.cpp is compiled in Core; new shared Circle clock
code is compiled in Main/Core. Eight new hashes and all26 unchanged installed
native61/CEP63 hashes/selector/runtime are recorded under motion-path-travel-sdk-*
and motion-path-travel-installed-hashes.json; prior Look At artifacts are not
used as this build's proof. Mode0 remains explicitly rejected by the graph
until actual history/resource/ordered integration and budgets are implemented.

### Light Path graph/history contract

Append Motion-local numeric keys9(path points),10(canonical units/second),
11(nonnegative explicit delay seconds),12(orient flag0/1). Reuse5(speed random),
6(Over Life),8(explicit forward, required when alignment is enabled). Mode0
requires9/10; other modes retain their existing keys. This unpublished schema1
extension changes no released AE ID, sequence or C ABI. Older evaluators reject
the new keys/mode; Main/Core shared inputs must receive a full paired build.

Points use an owned SFMP1 packet:16-byte little-endian header (magic SFMP u32,
version1 u16, stride24 u16, count1..256 u32, reserved0 u32), followed by XYZ
binary64 records. Exact length/header/finite coordinate bounds are checked before
allocation. The graph envelope continues to own its checksum. No host reference,
file path, layer identity or pointer occurs in this numeric packet.

Validate every authored node, including parked ones; only active path geometry
is adaptively compiled. The existing20M request work budget includes point
validation, curve work and adaptive compilation cancellation polls. A separate
32MiB cumulative compiled-path capacity budget is shared across Auxiliary prefix
evaluation and temporal caches. Charges are not refunded on eviction, providing
a conservative request bound without silently dropping tables or coarsening.
Current geometry is keyed by node/time and immutable leases survive eviction.
Clock-only history samples use a separate bounded4096-entry cache; geometry is
not rebuilt at each historical speed midpoint. All failures remain typed.

Delay is captured at particle birth. Integrate speed/random/curve from
birth+delay on the absolute30/60/120Hz midpoint lattice, at the actual age/life
fraction. Explicit caller metadata alone can certify a constant path clock and
permit analytic interval integration; equal samples never prove constancy.
Current geometry and current rate supply displacement/instantaneous velocity.
Animated geometry derivatives and reference endpoint/delay policies remain open
before public author delivery.

Apply displacement/velocity before Particle/Transform style, then path alignment
and Look At in chain order using the final Euler/shared-affine forward. Preserve
the numeric helper's atomic all-state API and provide independently validated
position/orientation phases for the graph. Serial paths and Circle→Path→Look At
are supported in this stage. Positional Motion after Look At, Circle after Path,
post-Motion Force/Transform and merging different Motion chains remain explicit
rejections pending the complete ordered-frame contract. Auxiliary parents use
the same path/history state and actual instantaneous velocity at child birth.
These temporary rejections do not reduce M3-18's eventual required interactions.

This defines a numeric candidate, not a reference behavior or published Light
Path author. External light selection/order, dependency invalidation/GUID,
selector/thread qualification, units and reference delay/end policies must be
completed before the three-mode native/CEP pairing is released.

The numeric graph/history stage is implemented2026-10-10. MSVC /MT standard and
ASAN each pass8288 checks: SFMP1 max-size roundtrip/every truncation/header/number
validation before allocation, actual graph/codec/snapshot9, metadata versus
midpoint clocks, animated speed and birth delay, all four analytic curves,
30/60/120Hz partial intervals, more than4096 historical samples, serial path
alignment after Euler/reflected affine and downstream Look At, Auxiliary
position/velocity, actual CPU pixels/GPU preparation and shutter samples.
Whole-request32MiB storage rejection is exercised in ordinary and historical
graphs; analytic curve work is charged against20M. Cancellation, every injected
allocation failure, unchanged source graph and concurrent requests are covered.
Circle1772, Look At1372, travel1216, Transform594, Model graph559 and Model
particle2695 regressions pass; actual exit0 logs are in artifacts/m3-18-path-graph-*
and motion-path-graph-{tests,asan}.log/exit.txt. Counts include new graph scratch
allocations in the neighbouring Motion failure scans. Sourcecf64261 is pushed
and git-archive frozen; the full May2023 x64 Release /MT eight-target build passes
at actual terminal exit0 (session18162), explicitly using both NoPublish switches.
Main/Core compile the new geometry/travel shared inputs. The new eight hashes,
terminal result and unchanged26 installed61/63 hashes/selector/runtime are in
motion-path-graph-sdk-* and motion-path-graph-installed-hashes.json. These numeric
tests and compilation do not qualify reference controls, AE callbacks or GPU
hardware; the three-mode author and full ordered/resource contract remain open.

### Ordered affine particle frame migration (unpublished)

Motion followed by an inherited affine Transform cannot in general be represented
by one proper quaternion after a shared basis: L*Q*B is not Q'*L*B for shear or
nonuniform scale. Allocating a distinct shared entry per particle would also
wrongly impose the4096 shared-basis limit on a larger particle population.
Append an owned row-major3x3 ParticleMotionAffine value(default identity), with
finite coefficient cap1e12. Settings.hpp owns it; ParticleInstance carries it.
The final axis is Q*A*B*Euler*axis. The new A is independent per particle and
may be singular or reflected, preserving full affine behavior without inverse
or Euler decomposition. Apply a downstream Transform's sprite linear map L by
setting A'=L*Q*A and Q'=identity; identity L preserves the old Q/A exactly.
Centre/velocity use the Transform centre map, and particle size/opacity use its
separate scalar controls. Commit all fields only after validation succeeds.

Snapshot10 migration is append-only:80-byte header = snapshot9's72 bytes plus
affineStride72 u32/reserved0 u32; particle stride304 = old200 + quaternion32 +
affine72 binary64. Emit10 only if any A is nonidentity; otherwise keep exact
existing3..9 formats. Readers accept3..10, initializing old A to identity and
checking new stride/header/length/finite coefficient bounds before returning a
snapshot. Preserve all existing shared tables and variable Model preflight.
C ABI8/sequence1/released AE IDs remain unchanged, but shared Main/Core inputs
require a new full paired build before author delivery. Old readers reject10.
CPU/GPU scene preparation/Model apply the same Q*A*B frame; Linear shutter
interpolates A coefficients alongside existing shared affine interpolation and
SLERPs the remaining Q. This endpoint affine approximation can be singular;
actual Subframe continues evaluating each ordered frame directly.

Plan the Particle's common pre-Motion Transform/Force prefix as before, then
execute Motion and downstream Transform stages in graph order, including
Look At→Path/Circle and Motion→Transform→Motion. Particle's authored life style
and Euler are set before the ordered stages; each Look At uses that stage's
centre and forward. No-Motion graphs retain their released evaluator behavior.
Post-Motion Force and merging genuinely different modifier chains remain open
for the next ordered-force phase; explicit temporary rejection is retained.
This numeric stage does not decide public target/unit/resource policies or
claim reference/AE behavior. All three authors and the complete interactions
remain M3-18 requirements.

The implemented ordinary/history/Auxiliary stage uses the ordered frame rather
than moving every centre before applying all orientations. The focused /MT and
ASAN fixture each passes4141 checks: independent L*Q*A arithmetic, singular and
reflected transforms, exact identity/atomic failure/no allocation, prefix and
serial Transform, reverse Motion order, animated sampled Transform, Path alignment
after shear,30/60/120Hz/partial/reverse histories,5001 distinct frames, CPU pixels,
GPU prepared axes, Model Q*A*B, snapshot3..10 byte round trips/all10 truncations/
bad headers/numbers/Model preflight, Linear approximation versus actual subframe,
Auxiliary birth inheritance, cancellation/allocation failures and concurrent
requests. Adjacent Circle1842/Look At1377/Path8432, Transform594, Model graph559
and Model particle2695 pass. Frozen complete SDK evidence must accompany this
new shared-input stage; previous Light Path SDK evidence does not cover it.
No Motion author, released ID/schema/ABI change or deployed pairing is claimed.

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
