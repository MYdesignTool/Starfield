# ADR 0034 — Particle texture resources and sampling

Status: implementation in progress, 2026-10-08. Task M3-13.

## Scope and reference

The active owner goal covers AE2023 main-effect naming/order, remaining Particle
controls/types including layer sampling, transfer and Transform. Native52/CEP52
implements transfer; Transform deletion/relative attachment was accepted by the
owner. Those implementations do not complete the remaining texture/model/path/
cloud/shadow families or replace their host qualification gates.

M3-13 owns Texture particles and their complete source/sampling/color/ratio/
perspective controls, Core/Render/C ABI/snapshot transport, native authorship,
SmartFX dependencies, CEP and focused tests. Face/Model geometry and the other
remaining Particle families will receive subsequent task cards, with the full
goal preserved. No unsupported menu option is advertised as implemented.

Behavioral inputs: owner report artifacts/reference/stardust_effect_parameters.txt,
owner menu screenshots, docs/reference-texture-transfer-transform.md, public
https://superluminal.tv/user-guide (Texture, texture properties and particle types).
These are data; independent code and identities are used. The owner explicitly
selected Freeze Frame = each particle's source image at its birth time.

## Independent numeric contract

Texture is shape3, following Circle0/Rectangle1/Cloud2. Optional Particle keys:
31 front layer ID,32 back layer ID,33 time sample0..7,34 color use0..2,
35 use texture ratio0/1 (default1),36 ignore perspective0/1 (default0).
A None front resource leaves the front face transparent; a selected back resource
can still render its face. A None back resource uses the front. Deleted resources
are errors, distinct from an authored None selection.
Existing optional-key defaults preserve Particle schema7, graph envelope1 and
released main/node disk IDs. A full paired build is required before exposure.

Sources describe a finite clip interval [start,end), frame duration, original
dimensions and PAR in the render layer time domain. Requests quantize to the
source frame grid; clamp or positive-modulo wrapping includes the last valid
frame and works with negative composition/layer times. Source modes:

Loop modes wrap at the actual half-open clip end before quantization, including
clips with a partial last frame. Arithmetic noise at an integral clip boundary
does not create a phantom frame; a positive clip shorter than one frame still
contains one sample.

| Mode | Requested source time |
| --- | --- |
| Current Time | current render time, clamped to clip |
| Play Once | clip start + particle age, clamped |
| Loop | clip start + wrapped particle age |
| Stretch | normalized particle age across all clip frames |
| Random Still Frame | one stable random frame for this particle |
| Random Once | stable random start + age, clamped |
| Random Loop | stable random start + age, wrapped |
| Freeze Frame | render time - particle age, clamped |

Random texture choice uses emitter seed and stable emitter/particle identity,
is independent of rendering order, and is transported as a 24-bit key. The
planner deduplicates (resource ID,frame index) and reports missing/deleted,
malformed, oversized and cancelled input rather than substituting a wrong image.

Use Texture Ratio preserves source width*PAR/height; otherwise Size/Size Y are
independent. Texture axes rotate in physical pixel units, with output-layer PAR
conversion at projection, so anamorphic output and rotation preserve that ratio.
Ignore Perspective affects sprite size, retaining perspective centre
placement and camera clipping. Front/back is determined in view space, before
inverse effect-layer screen transforms. Anchor/orientation/Transform still apply.

Texture buffers are premultiplied RGBA32F in AE working space. Bilinear filtering
interpolates premultiplied channels to avoid transparent-edge color halos. Default
uses source RGB/alpha; Alpha uses particle color and source alpha; Lightness uses
particle color with source alpha times bounded Rec709 luminance. Particle opacity,
edge feathering, transfer mode and each motion-blur sample apply normally.

## Ownership, budgets and transport migration

Render.hpp gains numeric immutable texture-source/frame views with caller-owned
staging storage. No host world pointer or AE suite crosses the Core boundary.
Core ABI6 appends explicit source/frame arrays to SfCoreRenderRequest; decode
validates counts, sizes, dimensions, row stride, key uniqueness, matching sources,
finite pixels and a512MiB aggregate texture byte budget. CPU is the Texture
backend; GPU scene preparation returns unsupported_format for texture particles
so the documented per-frame CPU path handles the same scene without omission.

EvaluatedGraph shares texture styles (six uint32 controls), referenced by each
particle; no style vector is copied per particle. Snapshot0x8004 version6 keeps
the200-byte particle stride: shape word low8 bits shape, bits8..9 transfer,
bits16..31 style index; up-axis word low8 bits axis and upper24 random key.
Its48-byte header adds style count/reserved; styles follow shared sprite bases.
Existing snapshot3/4/5 decode with no texture styles and unchanged semantics.
Version6 is emitted only for texture styles. Old readers reject its version.
All counts and unused bits are checked;4096 style and128 source limits align
with existing graph bounds. Sequence schema and match names stay unchanged.

Native controls append after existing Particle stream519 and preserve
UUID/connection/guard indices. The adapter layout below specifies main resource
layer slots and private binding metadata. SmartFX must checkout
each unique requested source/time dependency in pre-render, then pair every pixel
checkout/checkin, copying working-space data into numeric staging storage. It
must honor cancellation, shutter times, source layer retiming and missing layers;
no UI renderer, global cache, render-time project mutation or host process change.

## Gates

## Adapter append layout, 2026-10-08

Particle streams520/527 are the Texture topic/end (disk2920/2921);
521 Layer,522 Dark Side,523 Texture Time Sample,524 Texture Color Use,
525 Use Texture Ratio and526 Ignore Perspective use disk233..238. The original
base442, record443..518 and Transfer519 never move. Private binding version4
permits the new authored fields through526; v1/v2 Particle limits remain442
and v3 remains519. Layer references are constant project-local IDs, not animation
aliases. The numeric modes/checkboxes use the existing alias mechanism.

The renderer appends128 hidden PF_LAYER slots at626..753 (disk1700..1827) and
a hidden previous resource count at754 (disk1828). Main manifest28 describes
this append; released IDs and sequence schema remain unchanged. Sorted unique
nonzero front/back IDs define the resource slot assignment. UI binding installs
the layer stream values in the same transaction as aliases, restores changed
values on failure and clears the former tail when resources are removed. This
does not require per-particle UI suite calls or synthetic metadata alias fields.

Pre-render obtains current source dimensions/PAR and visible clip interval from
the public Layer/Item suites. The source clip is sampled on the composition
frame grid expressed in the renderer's layer clock. SmartFX's layer checkout
performs source footage/precomp animation, source stretch and time remapping.
The owner layer's comp/layer conversion defines that clock; reversed clocks
normalize the interval. Nonlinear owner time-remap and Source/Masks/Effects stage
mirroring remain explicit AE2023 gates. No private layer selector bits are read.
Requests are deduplicated across the whole shutter exposure before pixel reads.

The native Texture selectors use the existing public custom layer menu with
video-source/self/Null filtering. This candidate implements Source only; it does
not display AE's Masks/Effects stage options while their dependency/mirroring
contract is unresolved. Transform's selector/Create Null behavior stays intact.
Masks/Effects remains remaining work under the full goal, not a qualified feature.

Portable preset v2 adds at most128 texture bindings by node UUID/key, layer name
and source name; graph resource IDs are saved as None and relinked only after a
unique match in the target composition. Missing/ambiguous matches reject before
mutating AE. Version1 remains readable. The file limit becomes256KiB; the graph
limit stays24KiB. Transform's previous portable-resource restriction is unchanged.


The renderer-owned slot pool and rollback transaction now implement the
SmartFX dependency boundary for sibling Particle controls. Layer identity
mirroring alone is not evidence that AE's Source/Masks/Effects selection was
copied. Source-only selection is explicit in this candidate; the other two
stages require a separate dependency/mirroring contract before exposure.

The public Adobe expression reference documents sourceTime(), source frame
duration/PAR and layer in/out/start times. It also notes that reversed layers
can have inPoint greater than outPoint. It does not document a Layer.stretch
expression property; do not substitute the scripting API's property in numeric
binding expressions. Source/owner retiming and the checkout time domain need
an explicit adapter mapping, with actual AE2023 evidence.
Reference: https://helpx.adobe.com/after-effects/desktop/work-with-expressions/expression-language-reference/expression-language-reference.html

Required minimal tests are now explicitly authorized by the owner goal. Cover
all eight modes, seeds/order, malformed resources, byte limits/cancellation,
front/back, aspect/PAR, perspective, transparent edges, color/transfer, wire/ABI
and shutter behavior. Compile with May2023 SDK, deploy only when AE is closed,
retain verified native52/CEP52 paired rollback. Actual AE2023 source footage,
effects/masks selection, animated/precomp sources, undo/reopen and UI behavior
remain host gates; numeric tests do not establish them.

## Core milestone evidence, 2026-10-08

The unpublished build53 candidate implements numeric resources, all eight modes,
deduplicated frame planning, shared styles, deterministic 24-bit texture keys,
snapshot6/ABI6, CPU texture compositing and typed GPU fallback. Shutter endpoint
interpolation remaps each endpoint's shared style table. Native/CEP Texture
authoring and SmartFX resource checkouts remain implementation work; this
milestone does not advertise or deploy those controls.

- `powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunParticleTextureTests.ps1 -Run`:
  112 checks passed. Log: artifacts/m3-13-texture-core-tests.log.
- `powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1`:
  11895 checks, 0 failures, using the current shared working-tree fixture.
  Log: artifacts/m3-13-core-regression.log.
- `powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -NoDistPublish -NoRuntimePublish`:
  complete May2023 /MT build passed. Log: artifacts/m3-13-texture-core-build.log.

The shared Core runner receives only the new ParticleTexture.cpp source entry;
unrelated MNT-01 runner/document/test changes are preserved and excluded from
this milestone's staging. Installed native52/CEP52 and its selector remain
unchanged; dist Particle/Core hashes were checked against the existing native52
receipt. Compilation and numeric tests do not qualify AE texture footage,
source/masks/effects stage selection, or undo/reopen.

## Source adapter milestone evidence, 2026-10-08

Build53/CEP53 now append the native Texture controls, mirror stable layer IDs
into renderer-owned slots in a reversible UI transaction, and deduplicate
SmartFX frame dependencies over the whole shutter exposure. Callback-local
8/16/32-bit pixel worlds are copied into bounded premultiplied RGBA staging;
every successful pixel checkout is checked in on success, cancellation and
failure. The reverse owner clock anchors its half-open grid at source in-point.
Native and CEP menus expose Source only. The Source/Masks/Effects phase selector
and owner nonlinear time-remap qualification remain open.

Portable preset v2 stores layer/source names separately from numeric graph IDs.
Unique resource matches are resolved before Add/Replace mutation; existing graph
resource IDs survive Add. The native selector excludes self/Null/non-video layers.
Freeze Frame uses the source frame at each particle's birth, as the owner chose.

Focused evidence (current source candidate; not AE host qualification):

- `tests/RunTextureAdapterTests.ps1 -Run`: 136 checks, 0 failures.
  Log: artifacts/m3-13-texture-adapter-tests.log.
- `tests/texture_panel_tests.js`: 36 checks passed.
  Log: artifacts/m3-13-texture-panel-tests.log.
- `tests/RunParticleTextureTests.ps1 -Run`: 112 checks passed.
  Log: artifacts/m3-13-texture-core-tests.log.
- Shared Transform Null selector: 141 checks, 0 failures.
  Log: artifacts/m3-13-transform-null-tests.log.
- Transform CEP: 95 checks passed; native Particle/Emitter/Force gateway,
  Add/Replace preset codec and bounded preset-manager transactions passed.
  Logs: artifacts/m3-13-transform-panel-regression.log,
  artifacts/m3-13-texture-gateway-regression.log,
  artifacts/m3-13-preset-regression.log,
  artifacts/m3-13-preset-performance-regression.log.
- Complete May2023 /MT native/Core build with dist/runtime publication disabled.
  Log: artifacts/m3-13-texture-adapter-build.log.

Deployment requires a fresh no-AE process check and an exact native52/CEP52
baseline. The paired publication receipt and one-step rollback will be recorded
after installed hashes are verified. Real AE2023 source footage, time modes,
8/16/32-bit working space, shutter, crop/downsample/PAR, undo and reopen remain
owner host gates. Face/Model/Cloud/Path/Shadow and main-effect ordering remain
work under the active complete goal.
