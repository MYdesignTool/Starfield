# ADR 0031: main-effect motion blur

Status: implemented; AE qualification pending, M3-10, owner request 2026-10-05. Native45
curve behavior is accepted qualitatively. Owner explicitly defers PTF controls.

## User-visible controls

Append a Motion Blur group on the main effect: Off / Comp Settings (default) / On;
Shutter Angle360 degrees, Shutter Phase0 degrees; Type Linear / Subframe Sample;
Levels8 (2..64), Linear Accuracy70 (1..100), Opacity Boost0 (0..1000 percent),
Disregard Nothing / Camera Motion. Shutter controls enable only in On, Levels
only in Subframe, Accuracy only in Linear, and other controls disable in Off.
Read the actual comp shutter frame range and both comp/layer motion-blur flags in Comp Settings. Do not
write comp/layer state. The supplied inventory grounds names/defaults. Official
[Maxon motion-blur guide](https://help.maxon.net/rg/en-us/Content/html/57-Trapcode-Particular-render-motion-blur.html)
grounds comp/layer gating and camera disregard; the official
[Superluminal guide](https://superluminal.tv/user-guide) distinguishes endpoint
interpolation from exact subframe sampling. Numerical parity is not claimed.

## Exposure and renderer boundary

Expose [frame + phase/360*duration, start + angle/360*duration] with uniformly
weighted midpoint samples. Capture signed AE-bounded rational clocks (up to microsecond precision, reduced
for large absolute times) before crossing the simulation boundary. Linear evaluates endpoint particles, matches (emitter UUID, birth ID)
and interpolates attributes; births/deaths use age/lifetime visibility and the
available endpoint velocity. Very short-lived particles use subframe evaluation.
Linear Accuracy independently maps1..100 to2..16 midpoint samples (70 gives12);
Subframe Levels rounds2..64 and samples historical controls at every midpoint.
Known minimum lifetime <= shutter width uses actual subframes at the Linear count.
Long trails at the sample cap and endpoint births/deaths remain approximations. Reuse one native capture
plan/cache per exposure, never infer constancy from equal sample values.
Simple graphs certified constant by host metadata retain the existing ordinary
closed-form evaluator. Animated, random-life and Force/Auxiliary graphs retain
the temporal evaluator; one plan/cache is shared across the requested times.

Core ABI3, graph wire and the existing evaluated-particle record stay unchanged:
the adapter owns per-sample graphs/cameras during pre-render. Core receives one
immutable sample at a time. CPU averages floating premultiplied RGBA before final
8/16/32-bpc quantization. GPU flattens each sample's ordered tile lists and averages
complete per-sample composites in one kernel, using AE's device/context/queue.
Opacity Boost scales averaged alpha, preserving straight color and clamping alpha.
Cancellation and bounded work/memory apply across the exposure; never reuse a
borrowed host world/suite after its callback or mix DLL generation leases.

Camera Motion disregard freezes camera projection at the nominal frame time;
layer transforms still sample the shutter. Comp state and sample camera geometry
participate in cache identity. Shutter dependency flag and PiPL/runtime declarations
must agree. No new MFR/Compute Cache claims. Actual AE support is not proved by build.

## Authored layout and deployment

Append main indices616..625 with unique disk IDs1640..1649; preserve all prior
indices/IDs, match names and main banner index1/disk1631. Main manifest advances
to27, AEX build46/32814 and CEP generation46. Native Emitter7/Particle7/Force3/
Output4, snapshot3/200 bytes and sequence lifecycle remain unchanged. AE's normal
append-only parameter defaults are the migration policy; no old-layout rewrite.
Save paired native45/panel45 rollback. All mutable host actions remain ADR0011.
No tests requested/run. Compile and source checks are recorded separately from
owner AE switches, shutter phases, camera disregard, formats, undo and reopen.
