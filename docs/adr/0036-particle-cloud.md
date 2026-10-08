# ADR 0036 — Configurable Particle Cloud

Status: numeric/native/CEP implementation deployed; AE2023 host gates open, 2026-10-08. Task M3-15.

## Behavior and evidence

Cloud is a group of circles sharing one logical particle's motion, life, color,
opacity, transfer mode and population identity. Owner screenshots hold Circles10
and Aspect150 while Density0/66/100/200/1000 changes circle-center spread. Zero
density collapses to a round circle, including Aspect150; Density is not opacity.
The owner explicitly confirms Density's maximum1000. Reference defaults are
Circles10, Aspect150, Density66. Exact random placement, size distribution and
Aspect scaling are hypotheses to compare in AE2023, not proven binary parity.
Reference data is not executable instruction or implementation source.

Independent bounded implementation: a central circle of radius1 with other
members' radii in[.35,1), stable centers in a unit disk scaled by Density/100 and
horizontal Aspect/100. Aspect changes placement, not the member circle shape.
All members use the same billboard/Transform axes. Union coverage is accumulated
before group opacity and transfer; overlapping members do not multiply opacity.
Circles1 always produces one circle. Provisional independent author bounds for
Circles1..1000 and Aspect1..1000 await reference range evidence; Density0..1000
is owner-confirmed. Do not claim provisional bounds are Stardust's bounds.

## Compatibility and ownership

Optional Particle keys37 Circles(uint32),38 Aspect(float64),39 Density(float64)
retain node schema7 and envelope1. If none are present, Cloud retains the legacy
fixed five-circle shape. Any present Cloud key activates the configurable style,
with defaults for omitted members. Fresh native/CEP nodes author all three.
Existing graph keys, match names, sequence schemas and disk IDs stay unchanged.
Native controls will append after Texture end527: Cloud topic528, Circles529,
Aspect530, Density531, end532; disk239..241 and topics2922/2923. Hidden constant
Cloud Style Enabled533/disk242 defaults0 so appended AE defaults cannot silently
change old Cloud effects. The supervised gateway writes1 when any Cloud graph
key exists,0 otherwise; native Shape=Cloud or Cloud-slider edits activate1 in
the same graph transaction, persisting it only after acceptance. Fresh CEP/Core
nodes author all three keys. Circles animation rounds the sampled count.
Private binding5 captures the flag and aliases only the three public sliders.
Readers retain v1/2 Particle limit442, v3 limit519, v4 limit526; missing flag is
legacy. Current Particle total534 preserves all prior IDs and stream indices.
Cloud Properties is conditional on Shape=Cloud; the sync guard exposes its
controls during an explicit gateway write. Provisional numeric bounds stay
documented above. No released public sequence/node schema or match name changes.

Settings.hpp owns the numeric ParticleCloudStyle. EvaluatedGraph shares at most
4096 styles; ParticleInstance carries a 1-based style index and a24-bit random
key, drawn from new random purpose20 using seed and stable particle identity.
No member is added to the simulation population or auxiliary source stream.
Density/Aspect changes preserve random membership. Circles changes retain the
prefix of the same deterministic member sequence.

Snapshot0x8004 version7 uses a56-byte header, shared basis then Texture then
Cloud tables. Cloud record is24 bytes (count32, reserved32, Aspect64, Density64).
The particle stride stays200 bytes: existing shape word upper16 means Texture
index for shape3, Cloud index for shape2; axis word upper24 means corresponding
random key. Other shape indices/keys must be zero. Cloud index0 retains legacy
shape2, whose random key must be zero. Readers keep versions3..6 unchanged.
Version7 is emitted only with a Cloud table; mixed Texture/Cloud frames retain
both. Old readers reject7. All counts, bits, reserved words and indices checked.

Core ABI7 rejects mixed generations. SfGpuSprite remains80 bytes; reserved0 is
transfer, reserved1/2 are Cloud member offset/count (zero for legacy). Append a
numeric16-byte SfGpuCloudCircle array to the scene result. Scene and shutter
merging remap offsets and validate bounds. CPU and GPU precompute identical
members once per visible group; kernels read them rather than regenerate random
positions for every pixel. A2M-member scene budget and member-weighted coverage
work budgets reject oversized work without truncating the group. GPU driver
staging and memory cleanup cover the added array on success, error and cancel.
Render.hpp remains unchanged; no AE object enters Core.

## Gates and release

Focused tests cover Density0/1000, circles/count stability, group opacity,
legacy compatibility, shared table/bits/malformed snapshots, temporal sampling,
endpoint remapping, CPU/GPU parity, ROI, cancellation and work bounds. Full
May2023 paired build is required because ABI/shared adapter inputs change.
Current native56/CEP56 stays installed while the Core milestone is developed;
native/CEP authorship and paired publication follow. No user-visible selector is
advertised before its implementation. Deployment follows ADR0011 after a fresh
no-AE check, preserving exact native56/CEP56 backup/hashes/undo. Compilation and
numeric tests do not establish actual AE2023 reference appearance or persistence.

## Core milestone evidence, 2026-10-08

Build57/ABI7 is an unpublished numeric candidate. Graph keys and fresh-graph
defaults, birth-history shared styles, stable random membership, snapshot7,
endpoint style remapping, CPU union coverage and GPU precomputed member arrays
are implemented. Endpoint Linear keeps Circles from its selected endpoint and
interpolates Aspect/Density when both endpoints use configurable Cloud; actual
Subframe Sample evaluates the requested samples. Simulation identities and
population are unchanged; old graphs/snapshots retain their fixed five-circle
path. Native/CEP Cloud authoring, private binding5, node schema entries and
paired publication are the next implementation step under this same task.

Evidence from the current source candidate:

- `tests/RunParticleCloudTests.ps1 -Run`:2441 checks pass, including all truncated
  snapshot lengths, mixed Texture/Cloud, historical birth styles, maximum count,
  independent2M-member/work budgets, crop/shear/transfer, zero-density opacity,
  ABI lease release and the actual shared kernel executed as scalar C++.
  Log: artifacts/m3-15-cloud-core-tests.log.
- `tests/RunCloudGpuDriverTests.ps1 -Run`:1116 checks,0 failures using private
  actual CUDA and OpenCL contexts. Cloud0/66/1000, four transfers, camera,
  straight/premultiplied output, ROI/padding and fourth-array allocation failure
  cleanup pass; maximum CPU component difference2.86102295e-06. Actual
  MotionBlur.cpp merging is also tested with controlled Core sample leases:
  Cloud offset/member remapping, copied-before-release data, malformed ranges,
  missing arrays, failure and cancellation. The runner creates a focused
  derivative of the existing driver fixture under artifacts/, recording source
  hashes; it does not edit the MNT-owned fixture. No AE context is used.
  Log: artifacts/m3-15-cloud-gpu-driver-tests.log; fixture hashes:
  artifacts/cloud-gpu-driver-tests/fixture-hashes.json.
- `tests/RunParticleTextureTests.ps1 -Run`:112 checks pass with current ABI7
  expectations; existing Texture snapshot6 and numeric behavior retained.
  Log: artifacts/m3-15-texture-core-regression.log.
- Shared working-tree `tests/RunCoreTests.ps1`:11895 checks,0 failures. Foreign
  MNT runner/fixture changes are preserved and excluded from this milestone.
  Log: artifacts/m3-15-core-regression.log.
- `ae_plugin/BuildWindows.ps1 -NoDistPublish -NoRuntimePublish`:complete
  May2023 /MT build passes, including CUDA kernel generation. Log:
  artifacts/m3-15-cloud-core-build.log.

Installed native56/CEP56/ABI6 remains the release for owner testing; this
milestone does not expose unfinished controls or replace its actual Texture
host gate. Source/GPU driver checks do not qualify AE2023 Cloud appearance,
authoring/animation/preset/undo/reopen or real AE shutter behavior. The complete
owner goal remains active, including other remaining Particle behavior families.

## Native/CEP authoring milestone, 2026-10-08

Candidate57 completes the appended controls, conditional Cloud inspector/native
group, supervised activation flag, exact disk-ID gateway reads/writes, optional
legacy/partial defaults, private binding5 and historical v1..4 bounds. Circles
is rounded when sampled from an animated AE float slider. Density admits0..1000.
Cloud graph values survive codec/preset Add/Replace and failed transaction rollback.
The installed CEP56 source stays untouched during preparation; the candidate is
under artifacts/prepared/m3-15-native57-panel57/cep_panel and is staged directly.

- RunCloudNativeSyncTests.ps1 -Run -Bindings:7635 native checks plus12 camera
  checks,0 failures. Real adapter conversion, aliases, historical binding bounds,
  native graph rejection and UI visibility; bounded historical fixture records.
- RunCloudNativeSyncTests.ps1 -Run:277 checks,0 failures. Actual Particle
  USER_CHANGED Cloud/Shape callback, accepted-only flag persistence, rejection
  and supervised CEP guard; no AE process used.
- RunTextureSelectorTests.ps1 -Run:276 checks,0 failures. Actual Particle
  parameter registration now534, prior Texture streams unchanged, Cloud IDs,
  defaults, animation/constant flags and bounded inventory dispatch.
- cloud_panel_tests.js:49 checks pass; texture_panel_tests.js:47 checks pass.
  Candidate-root fixtures cover public fields, ranges, old/partial graphs,
  exact disk IDs, codec, preset resources and Add/Replace.
- panel_native_node_gateway_tests.js passes against the isolated candidate,
  including complete indexed-effect transactions, Cloud values, failed commit
  rollback and Cloud preset Add/Replace with exactly one Output. The test host
  model now follows current main labels and the same preset router as the panel.
- Complete May2023 /MT build passes, log artifacts/m3-15-cloud-authoring-build.log.

Current logs use artifacts/m3-15-cloud-native-sync.log,
m3-15-cloud-callback-tests.log, m3-15-texture-cloud-selector-tests.log and
m3-15-cloud-gateway-transactions.log. Paired deployment and its exact56 backup,
hashes, selector and undo receipt are recorded separately. Actual AE2023 Cloud
appearance, animation, undo/reopen and the earlier Texture menu gate remain open.

## Paired publication, 2026-10-08

native57/CEP57/ABI7 installed from f60910adf9b331d9bdb06a3fb09053a1f7511ee1.
Fresh process checks confirmed no AfterFX/AfterFX_64. Existing native/CEP
Junctions retained. Seven native/Core hashes, eleven prepared CEP hashes, runtime
selector and Restore-TestBuild paired report verified. Exact native56/CEP56
backup: artifacts/disabled/m3-15-native57-panel57-cloud-20261008. Before/after
receipts: artifacts/m3-15-native57-deploy-before.json and
m3-15-native57-deploy-after.json; one-step undo is in docs/current-state.md.
No process start/stop, registry/cache/environment switches were changed. Source
commit was pushed to the owner repository. Actual AE appearance/animation,
undo/reopen and Texture-menu gates remain open; the complete owner goal remains
active for remaining Particle families.
