# ADR 0050: multi-frame rendering admission

- Task: MFR-01, owner AE warning report, 2026-11-16.
- Status: native132/panel124 experimental qualification pairing deployed at owner's explicit request; default builds keep MFR disabled and real AE qualification remains open.
- Scope: AE2023 / May2023 SDK only; no new host or GPU qualification inferred.

Owner handoff update: offline verification is complete for the current candidate;
actual AE tests will be performed by the owner using
engineering/ae2023-owner-test-checklist-20261116.md. After offline handoff the owner
explicitly requested deployment; native132/panel124 was installed with AE closed,
verified against the immutable candidate and backed up for129/124 rollback. The
previous agent-launch proposal remains superseded; no agent AE launch/quit occurred.

Default main and hidden node PiPL/runtime flags omit threaded rendering. The
installed experimental132 pairing sets the flag solely for owner qualification,
not as a supported-release claim. The main native
pre-render path also queries camera, lights, layer metadata and Motion targets through
AEGP suites; their exact AE2023 render-context/thread guarantees must be resolved or
replaced by independently fresh owned data before release. Native131 now removes the
shared GPU device mutex: OpenCL requests create their own kernel objects from the
immutable compiled program; CUDA supplies launch arguments per call. Allocation,
upload, chunk dispatch, drain-before-free and per-thread context scope remain owned
by the selector. No driver/host callback is under a plug-in device lock. This does
not invent a new device lifetime: setup/setdown remain host-owned and require AE
qualification. A process-wide mutex around host calls is not an MFR solution.

The first isolated change removes the cached emission timeline's long-held lock.
Each request copies the proven prefix under a short C++ data lock, then owns its
timeline while historical host callbacks execute. Destruction may publish it only
if the prefix generation still matches. A competing or reentrant publication wins;
the stale request remains numerically valid but cannot replace the newer cache.
Invalidation/eviction leaves active copies owned; profiles, lattice, work/cancel
limits, host dependency comparisons and maximum retained prefix capacity stay fixed.
No SDK/host callback executes while this prefix lock is held.

These changes do not change Core or the render contract,
write project data, or qualify any AE build. Default/release capability remains disabled
until both the main effect and pass-through nodes satisfy their selectors, shared
state/resource contracts, PiPL/runtime agreement, and real host acceptance.

To make that acceptance possible, native132 adds an explicitly experimental build
configuration. `BuildWindows.ps1 -MfrQualification` requires both NoPublish switches,
RenderJournal and a reserved `*-mfr-qualification` artifact label. It sets bit27 in
both runtime and PiPL for the main effect and eight node AEX modules; the AEGP panel
does not claim an effect flag. Default flags remain unchanged. Compiled PiPL versus
actual GLOBAL_SETUP is checked by loading all nine AEX files in an isolated fixture.
This configuration is not a supported release, cannot be routinely deployed, and
requires an explicitly approved host test before activation. Deploy-TestBuild selects
its separate artifact label only with `-MfrQualificationTest` and verifies provenance.

The qualification-only journal records selector begin/end, process/thread IDs, QPC,
module and frame time. Admission requires overlapping render intervals on distinct
threads and distinct frame times; pre-render overlap, different thread IDs, active
counters or old-process records alone do not qualify. The prepared smoke harness
refuses an existing project, uses scoped MFR settings, closes only its own project
and requests normal shutdown after the script returns. PNG smoke output does not
qualify full precision export or the remaining resource families. See the exact
host operation/rollback list in engineering/mfr-host-qualification-20261116.md.

The May2023 SDK's own Resizer sample advertises MFR and performs PFInterface camera,
layer transform, stream, composition, item-dimension and light queries from Render.
That is primary evidence against treating every AEGP read as UI-only. It is not a
blanket guarantee for every suite/version/SmartFX path in this project; unexercised
resource families remain gates rather than being replaced with stale UI snapshots.

Primary contracts: local May2023 AE_Effect.h SUPPORTS_THREADED_RENDERING comments;
[Adobe SDK MFR guide](https://ae-plugins.docsforadobe.dev/effect-details/multi-frame-rendering-in-ae/),
[Adobe thread-safety tooling](https://github.com/adobe/ae-plugin-thread-safety).
GPU contracts: [Khronos thread safety](https://www.khronos.org/assets/uploads/developers/library/2011_GDC_OpenCL/OpenCL_GDC-Mar11.pdf),
[NVIDIA cuLaunchKernel guidance](https://forums.developer.nvidia.com/t/thread-safety-of-culaunchkernel/107459).
