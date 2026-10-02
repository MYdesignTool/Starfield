# ADR 0023: native node animation through owned render inputs

Status: development implementation; AE 2023 owner qualification required.

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
