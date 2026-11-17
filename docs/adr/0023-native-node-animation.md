# ADR 0023: native node animation through owned render inputs

Status: development implementation; AE 2023 owner qualification required.

## STB-21: custom curve publication and drag ownership — 2026-11-17

### Native136 follow-up: do not force a post-return repaint before PF adoption

Owner confirms135's editing gesture works but the curve itself still flashes
back. The remaining path sets PF_EO_UPDATE_NOW after a successful release. The
May2023 AE_EffectUI.h contract schedules that immediate view update after the
event returns, when the call-scoped preview is already gone. The timing fixture
now models old delivered values for that early DRAW and adopted values for the
normal queued repaint; it does not claim to measure AE's internal event order.

Successful curve publication now invalidates the local control without
PF_EO_UPDATE_NOW and returns without another immediate invalidation. Drag moves,
cancellation and failed publication retain immediate updates. The existing
numeric preview scope includes the local invalidation callback, covering a
reentrant paint there as well as during native publication. FORCE_RERENDER,
PF change flags, native receipt/guard checks and the accepted135 gesture remain.
No persistent overlay, delay timer, redraw loop or author/render cache is added;
Undo and subsequent queued drawing use authored values. On 2026-11-17 the owner
confirmed the current issue resolved, accepting the Over Life effect, continuous
editing gesture and curve release flash fixes in native136/panel124. No exact AE
build or complete Undo/save/reopen or MFR qualification was supplied by that reply.

### Native135 follow-up: one gesture and publication-time drawing

Owner confirms native134 Over Life now takes effect, but adding a handle needs
a second click and release briefly shows the previous curve before refreshing.
Inserted handles now set the same drag continuation as existing handles; the
first press/move/release remains a single draft and one final bank publication.

A bounded context/UUID/bank-keyed preview holds only the desired numeric curve
for the synchronous publication call. Reentrant DRAW uses it even if AE supplies
the previous or partially updated bank; reentrant edits do not start another
publication. A token-scoped destructor clears only its own preview, tolerating
context removal. Active previews are excluded from context-cache eviction.
No preview survives the call to conceal Undo or an external author edit, and it
is never a render input, source proof or publication fallback. Failed publication
restores the PF bank and subsequent drawing immediately shows those restored values.

Curve commits invalidate their local control and retain FORCE_RERENDER; they no
longer request REFRESH_UI, which redraws the entire ECW/layer/comp windows. Public
IDs, bank layout, owned native commit API and Core/ABI remain unchanged. Actual
AE interaction and input-to-render latency require owner confirmation; eight
existing134 source-service samples near20ms do not measure that complete latency.

### Native134 follow-up: owned bank requests

Owner reports native133 still fails and preset Apply displays SF-T202/AE516.
The generic error establishes an unpublished bank, not its exact early gate or
the host's delivered parameter count. A production-catalog fixture reproduces
the USER_CHANGED_PARAM count gate rejecting custom EVENT banks with delivered
counts 0/1; this is confirmed offline, not a measured AE count.

Custom editors now capture a typed, complete value bank plus UUID into NativeEdit
and call sync_custom_node_graph_bank. That entry accepts no params[] pointer,
checks the exact bank shape/type/count and source kind, then uses the existing
host lookup to reacquire and verify a unique live kind/UUID with source guard0
and an unguarded renderer. Only authored values are copied into the internal
commit request; caller handles/context or receipts cannot be injected. The
original delivered count is left untouched and is not treated as bank capacity.
Ordinary USER_CHANGED_PARAM retains its strict delivered-array gate, and malformed
owned banks, missing sources, active transactions and guarded renderers fail closed.

Curve and gradient editors share the boundary; shared Motion/Turbulence curve
construction is covered offline. Draft dragging and PF change-flag rollback
remain. Early custom failures report SF-T203 with the precise gate, kind,
parameter and delivered count rather than another indistinguishable SF-T202.
Twenty production curve presets are selected by a controlled modal substitute
and traverse the actual editor/native graph commit at three delivered counts;
the Win32 dialog and real AE behavior remain owner qualification.

The following paragraphs document the preceding native133 stage.

Size, Opacity and Rotation Over Life use the existing constant knot banks and
the same explicit native publication boundary. Linear/Hold/Bezier handle drags
now follow Draw's owned-value draft policy: repaint locally while dragging,
then stage and publish the whole bank once on the final drag event. No graph or
host parameter is written by intermediate moves. Cancellation, context close,
eviction and changed authored values discard the draft; a late release cannot
restart it. An unchanged release does not materialize a default curve.

A custom editor cannot treat an unavailable/guarded/partial source context as
a successful edit. Those cases now return an error to the editor, which restores
its staged PF bank and change flags. Ordinary supervised callbacks retain their
existing deferral behavior. Failed custom curve publication reports SF-T202 when
there is no more specific native error. The original code reproduces a successful
no-op and an unsynchronized saved bank in the guarded offline fixture; this is
a confirmed defect, not proof that the owner's AE report took that branch.

No public parameter, ID, layout, schema, rendering math or ABI changes. Offline
tests cover three curves through actual UI, native commit and Core evaluation;
actual AE2023 operation, Undo/save/reopen and latency remain owner qualification.

Build 20 follow-up: owner build-19 opening error identifies stream -1. Remove the
sampler's delivered num_params >= 610 requirement and the same gate around render
globals. SmartFX supplies no params[] array; registered IDs and PF_CHECKOUT_PARAM /
PF_CHECKIN_PARAM validate stream access. Local May 2023 AE_Effect.h documents
num_params as the input parameters received, and the SDK SmartFX contract requires
non-layer parameter checkout because it does not deliver a parameter array.
Count-zero fixtures reproduce the premature failure, then full graph sampling and
actual CPU pixel regressions pass after repair. Build 19 did not report the count,
so the exact owner-host cause remains inferred. Keep malformed/absent binding and
callback errors strict; diagnostics now include failure phase and delivered count.
812 scoped checks pass. AE host animation acceptance remains open. No ID/layout,
Core ABI, expression or CEP handshake change is required by this correction.

Build 19 follow-up: owner Origin XY keys yield black output; the panel also
rejects its loaded JSX. The startup token mismatch is confirmed and corrected
with paired animation-19 tokens and actual JSX readiness regression coverage.
The old callable PropertyGroup/Effect assumption and catch-all zero fallback
are a hypothesis for black output. Bindings now use documented Layer.effect /
Effect.param methods, isolate unrelated identity probes, and use a finite
unavailable sentinel (-2^40). UI transactions evaluate every bound stream and
check expression enabled state before graph publication; failure restores base
values, expression text and enabled state. Missing/nonfinite render bindings
reject with the exact stream index rather than silently zeroing settings.
Reference: https://helpx.adobe.com/after-effects/desktop/work-with-expressions/expression-language-reference/expression-language-reference.html

Scoped build-19 evidence: 798 adapter/camera/registration checks, actual CPU
alpha/frame-change/reverse-time checks, generated expressions with non-callable
host-object fixtures, and actual JSX startup tests. May 2023 SDK build passes.
No AE session operated; owner Origin XY playback/CEP-closed/reopen gate remains.

Build 17 native edits now take effect according to the owner, but public node
controls were registered CANNOT_TIME_VARY. Removing that flag alone would leave
rendering on a constant graph. P-02J completes both paths.

Native effects own keyframes and user expressions. Identity, topology, layout,
guards and over-life knot banks remain constants. The main effect appends 512
hidden numeric render inputs (indices 98..609, disk IDs 1000..1511). UI commits
bind these through AE expressions to native properties by their eight-word UUID,
so node reorder and rename do not redirect them. These are derived dependencies,
not duplicate authored controls. Capacity overflow rejects a transaction.

Optional graph record 0x8002/version 1 saves typed raw fields and binding slots.
Pre-render checks out only its own PF streams at the requested layer time, then
reuses native-to-core conversion. No AEGP query/mutation runs on render threads.
AE evaluates source keyframes/expressions and tracks their dependencies, including
renders with CEP closed. Failed graph publication restores prior binding expressions.

Main manifest becomes 22; existing indices/disk IDs, node schemas/layout and Core
ABI 2 remain stable. Recreate development effects for qualification; no old-project
migration. Build 18 is unreleased.

Settings are sampled at the requested frame. The stateless kernel applies that
frame's settings to survivors; birth-time emitter trajectories and integrated
animated-force history require a separate history contract. Curve knots stay static.

Qualification: public stopwatches; constant metadata; scalar/point/color sampling
at multiple and reverse-order times without AEGP; rename/reorder ownership;
failed-publication restoration; owner AE interpolation, CEP closed, undo/reopen.


Candidate evidence: 761 scoped adapter/registration checks, actual generated
expression execution and CEP keyed-write protection, and all five May 2023 SDK
Release /MT AEX links pass. No AE session operated; owner qualification pending.
