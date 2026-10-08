# ADR 0033 — Particle transfer and effect navigation

Status: implementation candidate, 2026-10-08. Task M3-12.

The owner requests particle-to-particle compositing and scrolling Effect Controls
to the native effect selected on the CEP canvas. The four Transfer Mode choices
were supplied by the owner: Normal, Add, Screen, Stencil. Vendor public guidance
defines Stencil as cutting alpha without rendering its own color:
https://superluminal.tv/question/can-i-use-a-particle-to-knock-out-the-alpha-of-the-stardust-layer
No vendor implementation code or binary is used.

## Compositing contract

Operate on premultiplied particle RGBA in the existing painter order (stable
back-to-front depth order when a camera is used). Normal retains source-over.
Add sums premultiplied RGB and retains source-over alpha, allowing HDR values.
Screen uses Cs + Cd - Cs*Cd for premultiplied RGB and source-over alpha, using
the same working-space values as Normal. Stencil multiplies destination RGBA
by 1-source alpha; its own RGB is ignored. Feather and particle opacity supply
source alpha. Later foreground particles can cover a previously cut region.
These formulas are the independent project contract, not a claim of pixel parity
with Stardust. CPU, GPU and every shutter sample must apply the same operation.

## Append-compatible migration

Particle graph key30 is optional uint32 (0..3), default0/Normal. Particle schema7,
other node schemas, graph envelope1 and sequence schema stay unchanged. Old
presets and projects missing this key use Normal.

Append native Particle Transfer Mode disk232 at physical stream519, after the
existing UUID/sync metadata. Existing physical streams1..518, disk IDs, effect
match names, UUID and connection layouts are unchanged. Parameter count becomes
520; authored/binding bounds separately include519 without moving metadata.
Old binding records v1/v2 have no519 and use Normal. New Particle records use
private record0x8002 version3, accepting519 while prohibiting metadata fields.
Version3 Transform entries retain the version2 attachment constants.

Particle snapshots use record0x8004 version5 when a non-Normal transfer is present.
It retains the version4 forty-byte header and 200-byte particle stride; shape
occupies bits0..7 and transfer bits8..9 of the shape word. All other bits must be
zero. Versions3/4 decode as Normal. Normal-only snapshots retain v3/v4 encoding.
GPU sprite reserved[0] conveys mode0..3; its eighty-byte layout is unchanged.
Core ABI5 protects against mixing an old GPU kernel or snapshot decoder with
new transfer data. A full paired native/Core/CEP build is required. Build52 uses
packed version32820; prior native51/CEP51 remains a paired rollback.

## Navigation contract

Use the documented AE HIDDEN stream flag reveal behavior: un-hiding a control
without SKIP_REVEAL_WHEN_UNHIDDEN expands parents and scrolls it into view.
An explicit canvas selection clears other selected properties, selects the UUID
resolved effect, and executes the session-resident Starfield Host AEGP command
"Starfield Reveal Selected Effect". The command resolves the native composition
selection and reveals the first visible control of that Starfield effect (Max
Particles for Output). It accepts native Effect/Effect Stream and effect-group
StreamRef collection forms, bounds selection/child traversal to1024, and releases
every borrowed/owned reference during the synchronous callback. The command is
registered in the Window menu through public AE APIs; a missing helper reports
an actionable selection error. AEGP lifecycle hooks remain in StarfieldHost,
whose code stays resident for the session. No parameter/guard value is written,
no graph is compiled, and no navigation timer/polling, OS automation or host-wide
setting is added. Actual scrolling in AE2023 remains an owner host gate.

## Evidence

Full May2023 /MT native/Core build52 passes with -NoDistPublish and
-NoRuntimePublish (artifacts/m3-12-native52-build.log). Six staged CEP52 scripts
compile syntactically. Core mode propagates through graph/temporal evaluation,
snapshot transport, CPU compositing and GPU sprite/kernel payload. CEP authoring
fills missing key30 with Normal for existing graphs and imported Add/Replace
presets, preventing a host snapshot/default mismatch. Existing native metadata
indices remain unchanged; native callbacks now derive UUID/guard indices directly
rather than assuming the guard is the final parameter.

The initial parameter-triggered navigation candidate was discarded before
deployment: even restoring a trigger value could invalidate a rendered frame.
Final navigation uses the UI command and non-undoable visibility operation.
Native collection/reveal behavior and scrolling still require host exercise.
No implementation tests were added or run in this request. Compilation is not
host qualification. Actual blending, navigation, undo and project reopen remain
gates. The final second full build and six-script syntax compilation both pass;
their evidence refers to the command-based navigation candidate.
