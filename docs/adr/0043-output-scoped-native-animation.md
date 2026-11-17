# ADR 0043: Output-scoped native animation and orphan retirement

- Status: implementation candidate; owner-led AE2023 qualification remains open.
- Task: M3-20 stability repair. New feature development is paused.

## Evidence

Read-only examination of the owner's saved `testproject.aep` on 2026-10-13
found a live graph with Emitter -> Particle -> Output and two disconnected
Turbulence records. One Turbulence effect no longer exists in the saved effect
parade. Its persisted slot142 / stream240 expression still names that missing
effect and checks its UUID. The owner's startup error names stream240. The
decoded evidence and extracted graph remain confined to
`artifacts/m3-20-stability-20261013/`; the original project is unchanged.

Core evaluation already uses only Output ancestors, but native current-frame,
history and idle metadata sampling read every binding record. Consequently a
disconnected missing source can reject a valid Output frame and makes idle
metadata repeatedly retry. This saved-data inconsistency is confirmed. The
operation that left the record behind and other active-node failures remain
unconfirmed; this evidence does not establish AE cache behavior.

## Decision

Use the Output ancestor set, following every upstream edge including resource
dependencies, for native frame sampling, historical plan/proofs and idle
metadata capture. Preserve parked nodes and their authored parameters. Explicit
single-node sampling continues to read the requested node. Active missing,
nonfinite or malformed bindings still fail; no fallback pixels or zero values
are introduced.

For alias handoff, require evaluated old values only for spans consumed by the
old Output graph. A disconnected retired span can use its finite recorded
value: no old render reads it under the same scope rule. The next explicit
graph edit recompiles from the actual native inventory and can remove the
orphan without recreating a deleted effect or a load-time project mutation.

Pure node additions preserve enabled, identical expressions for existing
sources. Removals, retyping, backups and final refresh retain strict rebinding.
Do not change transport timeouts or resurrect withdrawn native78/79 proposals.

Diagnostics identify the failing node, parameter and component. Public IDs,
match names, binding record v11, sequence schema and Core ABI9 are unchanged.
Offline fixtures and compilation can establish code behavior only. No automatic
AE acceptance, process launch or project edit is authorized by this task.

## PERF-02: preparation eligibility scope — 2026-10-25

The static-history selector now reuses this module's Output ancestor collector,
matching current-frame sampling, metadata and historical plans. A parked Force
whose source is absent cannot force an unrelated simple Output through temporal
traversal/frozen particle encoding. The original graph and parked records remain
unchanged. This is an eligibility optimization; all active source proofs and
validation are still required. The bounded linear gravity/drag extension and its
explicit parallel-route exclusions are specified in ADR0026's PERF-02 addendum.

## PERF-04: certified request-local compilation — 2026-10-25

TemporalGraphSampler may return an owned complete constant node, certified by
metadata for one evaluation request, including that request's external resource
snapshot. Missing capability preserves the existing historical sampling path.
TemporalCapture exports only NativeAnimationPlan's already-certified nodes and
attaches the current shutter sample's Look At resource. No host objects escape
the adapter. This C++ capability is internal to the adapter/Core source build;
the loaded Core C ABI remains 9 and Render.hpp is unchanged.

The temporal evaluator compiles certified Emitter, Particle, Force, Transform
and Turbulence values once per request, validates identity/schema and reuses the
compiled values at distinct birth/integration times. It retains historical
integration steps, branch order, numerical functions, cancellation and budgets;
it does not substitute static evaluation for complex graphs. Caches are local
to the evaluator and never survive a frame/request. Dynamic or unproved nodes
retain exact-time sampling. Equal sampled values are never constancy proof.

Capture entry points already receive a graph sampled at the current frame.
When every active bound input has a valid all-time constant proof, they reuse
that graph instead of checking out all aliases a second time. This applies to
temporal processors too, independently of static-history eligibility. Other
NativeAnimationPlan callers retain the default resampling behavior; partially
certified graphs still hoist raw values because binding-record values may be
stale. Selected Transform layer/matrix dependencies remain unproved and follow
the original history path. No dependency check or initial current-frame alias
checkout is removed.

The owner's follow-up screenshot records C472ms/H7.6ms and UI maximum557.4ms;
the reported 5–8second wait is not explained by any one of those counters.
UI loading formerly required at least three complete warm passes, and DRAW
and idle kept separate stamps. A successful capture now records its post-warm
global dependency state in a shared scheduling stamp, including Options.
Warming is intentionally allowed to change lazy dependency state. The stamp
does not certify values: render still compares every individual proof. Missing
proofs and subsequent authored dependency changes retain automatic recapture.
The first post-load capture still evaluates every active alias, preserving the
no-Options lazy-dependency bootstrap contract. No render request or project
write is introduced. This reduces duplicate work but does not establish the
root cause of the entire user-reported delay.

An unchanged UI tick now compares one whole-effect all-time state against the
post-warm stamp instead of validating each alias again before its timer starts.
The stamp also requires its process-local publication token to remain present
in the bounded proof registry; eviction or clearing forces recapture. Callback
PF references are not persistent identity, so state comparison can reuse the
stamp across equivalent callback copies. A token only gates optional UI work
and never substitutes for render's individual PF_State comparisons. Idle timing
now begins before its dependency lookup, covering previously uncounted work.

## PERF-05: immutable proof catalog and preparation failure safety — 2026-10-25

Native proof publications are immutable owned snapshots. A writer builds a
bounded32-publication catalog and its UUID/stream candidate index before taking
the short registry commit lock. Render/SDK callbacks never take the writer lock.
Readers retain one catalog lease. Replacing/clearing/evicting a publication does
not invalidate in-flight candidate pointers, and retired proof/profile/prefix
payloads are released outside registry/writer locks. Allocation failure before
commit preserves the previous catalog and emission prefixes.

For each requested UUID/stream, validation initially schedules only its newest
candidate. A heap merges unresolved candidates in the exact former publication/
proof traversal order; a valid match removes the entire older chain, while an
invalid match schedules the next old candidate. Host state is read once per
distinct stream and every selected proof still requires PF_AreStatesIdentical.
Missing/failed state cannot certify a result. Output order and SDK-call order
remain unchanged. Rate-profile payloads are copied only for selected results,
instead of cloning every historical publication on each preparation.

NativeAnimationPlan indexes current constant certificates once; TemporalCapture
indexes rate/lifetime metadata by UUID for its request. Index-allocation failure
uses the original linear lookup instead of adding a new rendering error.
The plan clears old proof/node/field-constant flags before any new allocation
or SDK access, and clears them on an interrupted preparation. A failed new
preparation must sample actual aliases, never retain a hoisted old Life value.
Null contexts, bad_alloc before proof lookup, reentrant SDK publication and
concurrent replacement require explicit regression coverage.

All host validation, Output scoping, registry/prefix bounds, try-lock timeline
fallback, authored values, ordinary/temporal math, identifiers, disk/schema and
Core ABI9 remain unchanged. Native109 changes this preparation implementation;
STB-01's explicit repair remains intact and startup does not mutate projects.
Microbenchmarks of this catalog are not AE end-to-end or subsecond qualification.

## PERF-06: certified owned numeric values and bounded node storage — 2026-10-26

A constant proof may carry an owned alias number. After warming all active
aliases, UI source inspection proves that the control has no keys/expression.
A fresh enabled OneD alias read must occur inside matching all-time dependency
states; missing, nonfinite or unavailable values cannot publish that number.
The earlier warm value alone is not sufficient. Synthetic Transform fields
retain their existing no-layer/no-inheritance eligibility; selected layers
remain exact-time sampled.

Current-frame and historical hoisting readers require UUID/stream matching and
PF_AreStatesIdentical before reusing the number. They retain the immutable proof
payload for the request. Missing numeric payload, failed validation/allocation,
dynamic inputs and invalid numbers use the existing strict checkout/checkin
path. Disk-record values and equal frame values do not certify constants.
No project mutation or AEGP access occurs in these render readers.

RawNode now allocates only the existing per-kind binding layout. Small Force
nodes no longer carry/copy Particle's approximately26KB dense curve bank.
Serialization versions, accepted indices, legacy seeding and authored numeric
conversion stay unchanged. Static analysis also exposed an unproven curve
search index; explicit bounds precede array reads without changing the valid
curve formula or encoding. Core is rebuilt for the shared header change.

Native110 offline comparisons cover complete current-frame graph bytes, cold
and changed dependency states, callback copies, failures during numeric reads
and strict missing-binding errors. They do not qualify AE preparation latency.
The owner's existing test project and the synthetic render-queue harness are
separate host gates. Subsequent UI work requires fresh owner approval under
the owner's2026-10-26 instruction; existing permission to edit the test project
does not waive the requirement to ask before computer interaction.

## PERF-07: whole-effect proof validation — 2026-10-26

A publication may carry a CHECK_ALL, all-time PF_State only when two matching
whole-effect states enclose its entire UI proof capture, after every active
alias has been warmed. This includes referenced layer parameters, unlike the
existing UI scheduling stamp. Per-source eligibility, individual state pairs
and owned numeric reads remain mandatory; the whole receipt never proves
constancy by itself. A failed or changed outer pair publishes unscoped proofs.

Render retains the immutable catalog, matches UUID/stream as before, and may
validate all already-certified proofs of a publication with one successful
whole-effect comparison. The current whole state is queried once per request;
comparison results are memoized per publication. Missing/failed/changed whole
states fall back to the original per-stream state and per-proof comparison,
including older valid candidates. Unscoped publications retain their existing
SDK call and result order. This explicitly supersedes the requirement above
for an individual render-time comparison only for whole-certified publications.
Opaque callback references and UI scheduling tokens never certify values.

No host handles are retained, no SDK calls occur under registry/writer locks,
and allocation failure remains a safe cache miss. Prefix and publication bounds,
all authored values, dynamic sampling, IDs/schema and Core ABI9 remain unchanged.
Offline call-count reductions are not AE latency or subsecond qualification.

## PERF-08: Output-scoped host resource preparation — 2026-10-26

The adapter's frame work graph must use the same Output ancestor closure as
native sampling, including all resource and Auxiliary parent-reference edges.
Full authored input still undergoes descriptor/structure validation before
scoping. Only callback-owned working nodes/edges are filtered, preserving their
relative order, identities, parameters and opaque optional records. The saved
author graph, effects and binding layout are never changed or repaired.

Motion light/path and shutter preparation, particle simulation, serialization,
texture/Model preparation and material-light selection consume that work graph.
Parked nodes do not request live host resources or force an unrelated shutter
mode. Active missing/invalid resources and cancellation continue to fail.
Core's standalone full-graph validation/evaluation remains unchanged; host
resource capture is an adapter operation on a render slice, not an exemption
from validating the authored graph's types, identities, edges or cycles.

Renderer texture and Model mirror slots still use the complete authored layout.
Resource preparation borrows that immutable layout only during the callback;
it must not renumber active resources after parked entries are removed. Texture
staging indices remain indices into compact owned metadata, distinct from the
original renderer checkout slot. Model count/identity/revision/bounds checks use
the complete recipe layout while geometry capture includes consumed resources
only. No AE types escape the adapter or change Core ABI9/Render.hpp.

Offline evidence must exercise actual pre_render plus original-slot resource
callbacks and compare consumed values. It does not establish AE latency or
authorize deployment or any computer interaction.

## PERF-09: Core request storage and static bounds — 2026-10-26

Core's Emitter/Particle/Force payload storage may be compact by actual node type,
with exact pre-reservation and an original-node-index pointer view. All node
semantic validation, UUID/topological order and parked-node rejection stay the
same. Pointers cannot escape the owning evaluation request or outlive/relocate
their payloads; recursion owns separate storage. Allocation failure must retain
the existing typed failure path, with no partial or stale values reused.

validate_settings already receives an independent by-value argument. Moving its
owned containers into the result removes an internal clone without changing
caller ownership or validation/notice order. Branch-local dead values may be
moved; Auxiliary child settings may be lazily cloned once per route and reused
only where every per-parent modification is reset before simulation. No new
per-particle allocation or numerical formula/order change is allowed.

CRC-table and spline-index bounds must be structurally explicit. Invalid spline
storage is a typed failure, never a zero fallback. Wire/CRC bytes, valid path
results, Settings/Render.hpp and Core ABI9 remain unchanged. MSVC stack reports
must identify compilation flags, and no macro/pragma may hide a warning.
Old113 numerical/encoding and allocation evidence is required; none qualifies
AE preparation latency or continuous editing.

Result's existing variant member may be constructed directly in place rather
than through a by-value intermediate Storage. Public signatures and member
layout stay identical. This removes redundant large-payload moves/stack slots,
not error checks or validation; construction exceptions retain caller handling.
Full Core tests and old/new value/encoding comparisons must cover this shared
implementation change.

Large reader/Settings prvalues may be constructed directly into optional-backed
request scratch through a short-lived factory conversion. Scratch is owned by
one top-level evaluation and bounded to 17 separate recursion depth slots; clear
every value on exit, including failure/exception paths, before storage reuse.
Ordered Force may reuse one lazy buffer because it does not recurse into graph
evaluation. No per-particle scratch allocation, cross-request cache, raw byte
storage or escaping borrowed payload is allowed. Verify nonmovable construction,
factory exceptions, destruction/reuse, allocation failures and complete old/new
values. Fixed frame allocation may slightly increase simple-case storage; report
that cost alongside larger-graph reductions. Standalone stack allocation is not
final LTCG or AE latency evidence. Complete authored Core sources and build
inputs must accompany the adapter-input manifest in the frozen candidate.

## STB-06: internal seed validation in ordered Force — 2026-10-26

Public emitter seeds retain their existing author bounds. After that validation,
signed Particle Seed Shift and Auxiliary mixing produce full32-bit RNG seeds as
specified by ParticleBirth. Ordered Force's physical Settings validation must not
reapply public seed bounds to this internal value. Validate physical controls with
a valid author default seed and restore the exact derived bits only after all
physical notices have been rejected. Do not mask/clamp the internal seed or widen
public author bounds. Static/history ordered response and Auxiliary children must
preserve the same random streams; tests require a full-bit numerical reference,
signed boundaries, strict invalid-author/physical rejection and actual DLL paths.
