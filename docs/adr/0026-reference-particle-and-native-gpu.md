# ADR 0026: reference particle controls and AE native GPU rendering

Status: implementation in progress, M3-06. Owner requests reference parameter
names/order, proper Emitting timing, Uniform default, Life Random, particle
shapes/rotation/color controls, performance improvements and GPU by default.

GPU decision revised on 2026-10-03 following the owner's SDK review. The private
Direct3D 12 renderer proposal has been withdrawn before implementation.
This ADR records a design, not a completed or AE-qualified GPU implementation.

Emitting is Default / Once / Sequenced / Randomized. Once births a first-frame
batch using the initial PPS. Sequenced/Randomized apply where the source has
ordered samples; unordered point/volume sources retain their default behavior.
Auxiliary is a separate structural source flag set by its creation action;
Emitting never switches a source into Auxiliary. Fresh development effects are
required; no migration of the incorrect mode or changed native layout is added.

Particle controls follow the observed inventory order. Implement supported
procedural Circle / Rectangle / Cloud, Life Random, Size Y, feather, Up Axis,
Orient To (None / Motion(particle) / Emitter), native Angle X/Y/Z, Angle Random,
Speed X/Y/Z, Speed Random, Limit to 2D and the existing Color/gradient modes.
Texture/Face/Model and external target orientation need actual source contracts
before they are offered. Shape, orientation mode and randomness are birth properties;
Motion(particle) follows instantaneous particle velocity; Emitter looks at the
particle birth position. Circle/Cloud remain camera-facing; Rectangle can use
3D axes or Limit to 2D. The three procedural types are an independent subset;
Cloud currently uses a fixed cluster rather than the reference Cloud controls.
Spin speed advances with particle age. Life Random independently shortens birth
life in the range [life * (1-percent/100), life]. This distribution is independent,
not a claim of exact Stardust numerical parity.

M3-06 owns Settings/Render/ParticleInstance, Graph/evaluation/history codecs,
CPU/GPU rendering and platform sources, the C render transport, native node/main
parameters and schemas, CEP gateway/editor, build/version/fingerprints and docs.
The immutable transient particle record is versioned for the additional fields;
main/native layouts and schema revisions are documented with fresh effects.
Render.hpp changes belong to this task; AE SDK objects remain in ae_plugin.

## GPU framework and host version

AE proposes a framework/device; Starfield accepts only a backend it implements
and can initialize on that device. Dispatch uses the actual what_gpu/device_index,
not an inferred GPU vendor, a preferred local adapter or a private graphics
context. The owner scope remains AE 2023, May 2023 SDK.

That SDK's PF_GPU_Framework enum contains NONE, OPENCL, METAL and CUDA. It has
neither a DirectX framework nor PF_OutFlag2_SUPPORTS_DIRECTX_RENDERING. Modern
SDK documentation describing DirectX must not be applied to this older contract.
The Windows implementation targets CUDA and OpenCL. It rejects unimplemented
frameworks; Metal and newer-host DirectX adaptation remain outside this card.

CUDA uses the Driver API with AE's context and queue. OpenCL uses AE's context,
device and command queue. Do not depend on AE's bundled CUDA Runtime DLL version.
ParticleGL's instancing/data-layout ideas are reference material; its SDL window,
OpenGL context and mutable wall-clock simulation do not enter this plug-in.
This follows the existing M0 policy in roadmap.md and reference-inventory.md.

## Required selectors and negotiation

1. GLOBAL_SETUP and PiPL must agree on SUPPORTS_GPU_RENDER_F32. Enable the
   capability only for a build containing the real GPU handlers and kernels;
   compilation alone is not an AE host acceptance result.
2. GPU_DEVICE_SETUP queries PF_GPUDeviceSuite1::GetDeviceInfo, checks the proposed
   framework/device and creates a per-device gpu_data object. Set the device GPU
   support flag only after successful initialization. Rejection sets no GPU
   support flag, allowing AE to select CPU rendering.
3. SMART_PRE_RENDER sets PF_RenderOutputFlag_GPU_RENDER_POSSIBLE only when the
   selected Acceleration setting, accepted device, frame and graph can use that
   backend. This is an additional required gate, not just the global flag.
   CPU selection or an unsupported case clears this flag so AE can negotiate
   another framework or CPU.
4. SMART_RENDER_GPU pairs the existing layer/output checkouts, queries the actual
   GPU world format and obtains its buffer with GetGPUWorldData. GPU world data
   is not PF_LayerDef::data; that member is null for GPU renders. Render directly
   into AE's GPU_BGRA128 output, respecting rowbytes, ROI and the channel order.
5. GPU_DEVICE_SETDOWN releases gpu_data and every backend resource after submitted
   work no longer uses them. Device access/context acquisition is balanced on
   success, initialization failure, cancellation and exceptions.

All explicit device/pinned allocations use the GPU device suite. AE output worlds
are borrowed for the selector; never free them or store them in project data.
Submission uses AE's queue and observes the backend's completion rules before
freeing uploaded inputs. Once AE has delivered a GPU output, the CPU pixel writer
cannot be used as an error fallback into that world. Setup/pre-render rejection
is the normal CPU fallback; an execution failure returns a documented host error.
Cancellation remains an interruption without an error dialog or a CPU rerender.

## Core boundary and performance

The portable Core prepares immutable particle/scene data with numeric geometry
and camera values. AE suites, GPU worlds, context/queue ownership and framework
dispatch stay inside ae_plugin/. A new typed scene transport needs an explicit
ABI revision and paired AEX/Core build; opaque GPU pointers must not be added to
the current CPU-only SfCoreRenderRequest or passed into CpuParticleRenderer.

The CPU renderer remains the deterministic comparison and selectable fallback.
The native GPU path does not allocate or encode an entire CPU RGBA output and
does not read its rendered frame back merely to copy it into AE. GPU simulation
and tile/bin rasterization are measured separately from parameter/history capture
and scene upload; they must preserve birth-time controls, arbitrary-time evaluation,
Auxiliary ancestry, stable identities and transparency ordering.

Shared output resources eliminate our GPU-to-CPU-to-AE image round trip. They do
not make the complete pipeline transfer-free: CPU-authored particle attributes,
history samples and bin/index data still need uploads until those stages are
resident on the device. AE may also convert or transfer frames between other
effects. Do not label the whole pipeline "zero transfer".

Optimize repeated full-graph parsing/copying in historical sampling and cache
force field conversions within one evaluation. Measure these CPU costs before
attributing slow rendering entirely to rasterization. No history is cached across
AE frames without a reliable invalidation contract.

## Qualification gates

- Scoped selector tests: framework acceptance/rejection, CPU preference, pre-render
  eligibility, padded rows/ROI, GPU_BGRA128 ordering, device cleanup and cancellation.
- Actual backend kernels: transparency/color/camera parity against CPU, empty and
  dense scenes, deterministic ordering, bounded memory and failure cleanup.
- Owner AE 2023.5.0 Build 52: actual proposed framework and GPU selector execution,
  keyframes/temporal sampling, Auxiliary, CPU/GPU switch, render queue and reopen.
- Timings separate history/scene preparation, upload, kernel and total frame cost.
  No GPU badge, speedup or supported-host claim from a compiler result alone.

Build 23 is CPU-only and keeps GPU capability flags disabled. Its control/timing
phase is implemented: Emitter schema 6/base 31, Particle schema 4/base 75,
Output schema 4, main manifest 23 (Time Sampling 611/disk 1601) and transient
particle snapshot 0x8004 version 2 (184 bytes/particle). Fresh effects and a paired
AEX/Core are required; no project migration. The C ABI remains 2 because sprite
transport still uses bounded opaque graph records, not a new public C structure.

EmissionTimeline implements certified constants, exact linear/hold keys and
fixed-frequency prefix reuse with deterministic inversion. The sampler interface
can return an explicitly certified profile; unknown/expression/nonlinear curves
retain sampled integration. AE metadata extraction and dependency invalidation
are not yet connected, so build 23 does not claim its native PPS is O(1).
A prepared native plan removes full-graph copying/redecoding per historical birth;
Force value conversions share a bounded per-evaluation cache. Cross-frame history
reuse requires a complete AE dependency token and remains a separate open gate.

MFR and Compute Cache remain separate, unimplemented milestones.

References: https://superluminal.tv/user-guide;
https://ae-plugins.docsforadobe.dev/effect-basics/command-selectors/;
https://ae-plugins.docsforadobe.dev/intro/gpu-build-instructions/;
local May 2023 AE_Effect.h and AE_EffectGPUSuites.h;
local observed Stardust parameter inventory and read-only ParticleGL.

## Build 24 implementation checkpoint

GPU integration is now implemented as an AE 2023 candidate, not host-qualified.
The May 2023 selectors accept only actual CUDA/OpenCL proposals. System Driver/ICD
entry points use AE-owned contexts/queues. CUDA modules use build-time PTX, OpenCL
programs compile our source on the offered device. No context/device/queue is created
by the product. Explicit buffers and pinned inputs use the PF GPU device suite;
output/input GPU worlds are borrowed and validated for format, device and size.
Uploads are explicitly completed before kernels, including out-of-order OpenCL queues.
Cancellation is polled between 128-row batches. Setup failure is a CPU negotiation;
once GPU output is delivered, failures return a host error and never call the CPU writer.

C ABI 3 adds SfGpuSprite (80 bytes), SfCoreGpuSceneResult, prepare_gpu_scene and
release_gpu_scene. Loader requires all ABI 3 functions and rejects ABI 2. Core owns
scene arrays until the same leased generation releases them. The adapter uploads
three arrays: projected sprites, tile offsets, ordered tile indices. Stable camera
sorting/order matches CPU; one GPU thread owns each pixel. No floating-point atomic
compositing, private graphics context, AE pointer in Core, or product image readback.
Simulation/temporal sampling remain on CPU. CPU and GPU share projection/shape math;
GPU arithmetic is float, so parity is bounded rather than bit-exact raster equality.

GPU scene limits are 32M tile indices and 512M tile pixel visits; an over-budget
scene reports unsupported eligibility during pre-render, clearing GPU_RENDER_POSSIBLE.
It is never truncated. The four separate native node AEXs also implement GPU setup,
pre-render eligibility, SMART_RENDER_GPU copy, and setdown; their pass-through kernels
preserve pixel bits/cropped world origins without staging allocation or CPU image copies.
Node controls/layout IDs are unchanged. Main appends GPU Rendering 613..615,
Acceleration 614/disk 1611 (GPU=1 / CPU=2); Output key 6 stores GPU=0 / CPU=1.
Main manifest/schema 24 and C ABI 3 require a fresh main effect and paired deployment.
The CEP readiness token is native-gpu-24. Time Sampling and all native layouts stay
as in build 23. Options reports the last executed main frame's CUDA/OpenCL/CPU path;
this readout is process-global, like the existing geometry diagnostic.

409 actual-driver/fake-host GPU checks plus the remaining scoped suites pass,
2,432 C++ checks total. Scope and timing caveats are in build-matrix.md. AE process
launch/UI/GPU render is not performed; owner host acceptance remains open. Metadata-
certified AE PPS profiles and reliable cross-frame cache reuse are still open.

## Build 25 temporal performance contract

Owner AE evidence: CUDA device 0 executes, but 10,000 PPS / Life 2 with no keys or
other nodes becomes slower at later frames. Native temporal preparation still
checks out every birth's controls and walks expired births up to the 10,000-second
global Life bound. GPU raster timings exclude this preparation.

Read authored animation metadata only on the main effect's supervised UI edit / Options
path. Certify an unanimated field only when its source has no expression or keys.
Capture the owned numeric alias's all-time PF_State before and after reading metadata;
accept only equal states. Store numeric proofs, never source stream/effect handles.
Pre-render verifies those proofs with PF_ParamUtilsSuite3, without AEGP calls. UI
UPDATE_PARAMS_UI is excluded because that selector returns random PF_State values.
Any unavailable/changed proof retains historical sampling; equal sampled values are
never evidence of constancy. Fresh proof capture may be needed after external edits.

Certified static Emitter/Particle/Output graphs (Default timing, Life Random 0)
use the existing closed-form alive-slot evaluator. Other temporal graphs hoist
certified constant fields once per frame, use a certified Life upper bound where
available, and share fixed-lattice emission prefixes only under equal all-time rate
dependency states and Hz. Caches are bounded, optional, and discarded on changed
states. Missing suites, eviction or contention use uncached computation. Authored
expressions/nonlinear interpolation never enter the analytic key path.

Public parameter IDs, native layouts, and C ABI 3 remain unchanged. Actual AE
end-to-end timing and cache invalidation remain owner qualification gates.
