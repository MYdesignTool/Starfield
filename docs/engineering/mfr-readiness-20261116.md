# MFR-01: AE2023 readiness and remaining gates

The original warning was accurate for129/124, whose effects omit the threaded bit.
After offline validation the owner explicitly requested deployment: experimental
native132/panel124 is now installed through the existing Junction, with all nine
effect PiPL/runtime declarations enabled and129/124 rollback verified. It is not a
release or host support claim. Default builds and the native131 prerequisite keep
MFR disabled. Deployment evidence: artifacts/mfr-deploy-20261116.
The owner will perform the actual AE tests using
ae2023-owner-test-checklist-20261116.md and ae2023-test-results-template.md.
The earlier agent-launch proposal in mfr-host-qualification-20261116.md is
superseded, not an outstanding request to start AE.

GPU request ownership is implemented: no device mutex across host/driver callbacks,
request-local OpenCL kernel arguments, existing per-call CUDA launch arguments and
drain-before-free cleanup. Strict analysis and ASan each pass 24,043 production
adapter checks, including 400 render plus 400 copy requests, reentry, cancellation,
allocation/upload/argument/enqueue/sync failures and padding. The frozen old adapter
fails the simultaneous host-callback check. Real CUDA/OpenCL each pass four-thread
96-frame render/copy/cancel runs with exact same-driver serial pixels and the prior
CPU tolerance. Full SDK131 and isolated SDK132 builds pass; all nine compiled AEX
PiPL/runtime declarations pass 333 checks in each configuration. No AE process has
been started and installed129/124 has not changed.

## Implemented prerequisite

`NativeTemporalCache.cpp` previously returned a shared timeline while retaining its
mutex across the caller's historical SDK checkouts. It now copies the proven prefix
under a short data lock, owns the frame copy, and publishes on release only when its
captured cache revision still matches. No host call is inside this lock. Eviction or
UI invalidation does not invalidate an active frame; stale completion cannot replace
a newer publication. Failure is still an optimization miss, not fabricated values.
The 131072-segment retained bound, proof/state validation and Core numerical lattice
are unchanged. Copy cost scales with the retained sampled prefix; normal analytic
profiles have no sampled segment vector. This is a safety preparation, not a measured
AE performance improvement.

An actual old-code negative control rejects a simultaneous second timeline request.
The new test checks independent copies, no partially visible append, stale completion,
and four workers/800 out-of-order frames against uncached integrals and birth times.
Strict analysis and ASan pass; detailed logs are in artifacts/mfr-01-20261116.

## Gates before advertising MFR

| Path | Current evidence | Work still required |
| --- | --- | --- |
| Graph/parameter checkout | Owned frame graph; native alias sampling uses PF checkouts, guards and mutation epochs | Real AE concurrent pre-render/UI-edit/cancel and copied-layer dependency tests |
| Emission cache | Frame-owned copy and revision-checked publication; parallel numeric test | Real host timing, long sampled-prefix copy cost and many-emitter memory budget |
| Frame/result lifetime | `PreRenderState` is built locally, transferred to AE and consumed as const; generation lease retained | Repeated/concurrent smart-render, teardown/cancel and live Core-generation switch stress |
| Sequence callbacks | Flat lifecycle marker; render path does not use writable sequence_data | Concurrent setup/resetup/setdown and save/reopen in exact AE2023 build |
| Camera | PFInterface plus Layer suite queries, including calls from request construction | Establish exact supported callback/thread context; replace unqualified calls with fresh PF-owned numeric inputs where needed |
| Lights / Motion targets / Light Path | Composition inventory and per-time AEGP layer/stream queries | Fresh per-frame owned capture, dependencies and historical sampling with a documented worker-safe boundary |
| Texture layer metadata | AEGP composition/layer/item queries; image pixels use SmartFX checkout | Worker-safe time/stretch/identity metadata and texture dependency validation |
| Model / Material | Geometry mirrors are PF-owned; optional inherited transforms and lights use host queries | Test inherited transform/light preparation with worker-safe resource path |
| GPU main and node copy | Request-owned OpenCL kernel state; no device lock spans callbacks; real CUDA/OpenCL parallel/serial/CPU comparisons pass | Actual AE borrowed-device/world lifetime, host GPU negotiation and teardown |
| Hidden nodes | CPU/GPU pass-through implemented, threaded flag absent | Same-thread/overlapping-frame world ownership and GPU-device teardown qualification for every node module |
| PiPL/runtime | Default off; experimental132 sets bit27 consistently for nine effect modules; actual embedded/runtime checks pass | Approved actual AE qualification; do not deploy experimental flags as a supported release |

The general AEGP threading guidance and PFInterface callback restrictions are not a
blanket guarantee for every query from simultaneous SmartFX callbacks. Existing
single-render behavior or successful fake suites cannot resolve this. Each resource
family needs a supported render-context contract or a replacement using fresh owned
inputs. A global render mutex, waiting for UI idle from a worker, or silently omitting
camera/light/texture data would defeat the requested behavior or risk deadlock/stale
frames and are not accepted shortcuts.

## Concrete next implementation boundary

Keep the current Core C ABI and numeric scene boundary. Resource capture should
produce owned camera/light/target/layer-time records tied to effect identity, graph
revision/checksum, request time and complete dependency proof before worker use.
UI-only metadata must not be reused for a different frame merely because the layer
or current timeline value matches. If new PF mirrors are needed, first specify appended
IDs and migration in a separate resource-capture ADR; existing IDs cannot be reused.

GPU requests now own mutable kernel arguments and allocations; CUDA context scope
lasts through cleanup. CPU work is not serialized around this path. Actual AE device
destruction and outstanding-frame contract still require qualification. Compute Cache
is optional; no writable render sequence data is introduced.

Final acceptance must exercise serial versus parallel output in actual AE2023 for
8/16/32-bpc, preview and export, multiple instances/layers, animated rates/lifetimes,
camera/lights/Motion, texture/Model/Material, motion blur, cancellation, UI edits and
save/reopen. Verify overlapping host frame callbacks and pixel equivalence, not only
that the warning disappeared. Starting AE, changing its preferences or running a
host test project requires the separate per-action authorization in ADR0011.

References: local May2023 AE_Effect.h lines833 onward; AE_GeneralPlug.h PFInterface
contract; [SDK MFR requirements](https://ae-plugins.docsforadobe.dev/effect-details/multi-frame-rendering-in-ae/),
[effect use of AEGP suites](https://ae-plugins.docsforadobe.dev/aegps/cheating-effect-usage-of-aegp-suites/),
[Adobe thread-safety tool](https://github.com/adobe/ae-plugin-thread-safety).
