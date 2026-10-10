# ADR 0039 — Motion geometry and first three modes

Status: numeric geometry implemented; public Motion contract remains proposed,
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

Confirm reference resource/units/Origin Type/Starting With policies before
publishing controls. Then define explicit append-only Motion identity, author
values, curve/history/resource/graph and shutter contracts in this ADR; implement
all three behaviors, native/CEP/preset persistence and full paired SDK build.
Particle/Force/Transform/Auxiliary interactions, reverse frame order, animation,
budgets, undo/reopen and actual AE2023 remain required. Turbulence follows the
first Motion stage; this math milestone does not close either task or the goal.
