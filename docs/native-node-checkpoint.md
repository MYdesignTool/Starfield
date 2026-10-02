# P-02D native node checkpoint — 2026-10-02

## Build 11: canvas deletion and flat Effect Controls — 2026-10-02

Owner reports unavailable canvas deletion and unnecessary collapsed outer
categories. P-02E adds Delete/Backspace and filters fixed Output out of both
delete and duplicate selections. The context menu pauses polling; marquee/menu
interactions restore canvas keyboard focus. Native effect removal and incident
link updates continue through existing guarded transactions.

All actual outer Output/Emitter/Particle/Appearance/Force topic markers are
removed. Main parameter schema 19 registers indices 1..89; native layout 3
reduces base counts by two. Main/native compiler indices, direct-edit trigger
and gateway bindings are updated together. Surviving disk IDs remain unchanged,
including arbitrary callback ID 31. Gateway native-node-sync-11; packed version
32779 (0x800B). Core, graph schemas and C ABI remain unchanged. No new hidden
topic placeholders or development migration. Use fresh development effects.

Candidate build uses -NoRuntimePublish -NoDistPublish. The May 2023 SDK build
and panel/gateway syntax checks pass; no tests added/rerun. Candidate Core hash
matches the installed build-10 Core exactly.
Build log: artifacts/build11-flat-controls-build.log. Native binary deployment
is required for the layout change; later Core-only changes retain hot updates.
The installed CEP Junction already points to this checkout, so no separate
CEP installation or host-wide settings change is required. Close/reopen CEP
after loading build 11.

Source was pushed as cb53d50 before deployment. **Build 11 is deployed** under
standing authorization after an immediate read-only check found neither AfterFX
nor AfterFX_64. All six installed hashes match the build matrix. Before: build-10
AEXs; after: build-11 AEXs. Pinned Core and selected generation
StarfieldCore-AE18EFC2E9856178.dll remain unchanged; the generation hash was
verified separately. The existing single Plug-ins/Starfield -> dist Junction
is retained. No AE session was started/stopped and no host-wide setting changed.
Backup: artifacts/disabled/p02e-build11-flat-controls-20261002/deployment.json.
Before/after state: artifacts/build11-deploy-before.json and
artifacts/build11-deploy-after.json. Deployment log: artifacts/build11-deployment.log.

One-step undo after deployment (AE closed):

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Deploy-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02e-build11-flat-controls-20261002' -Rollback
```

Owner checks: Delete/Backspace and context-menu deletion of single/multiple
nodes, including selections with fixed Output; corresponding native effects
disappear; directly visible parameters; direct native edits, undo/save/reopen.
SDK compilation does not establish actual AE acceptance.

## Build 10: multi-emitter evaluation and wire editing — 2026-10-02

Owner build-9 evidence is "基本正常但不完全正常", with two cancellation
dialogs and the core's single-emitter restriction. This is not a blanket pass
for every node operation. The owner also reports ineffective wire-click
disconnect and requests port snapping.

G-06/build 10 returns normal render interrupts without return_msg. Each Emitter
partitions births among its own UUID-ordered Particle children; actual live
sequences merge under Output's one cap, preserving identity `(emitter UUID,
local slot)`. Only selected births are simulated into a pre-sized final buffer,
with no per-emitter particle populations/final sort. Force deduplication and
one-pass appearance precedence remain in place. ADR 0015 defines cap/order.

CEP's full-canvas node container was intercepting wire hits. Its empty area
now passes pointer events; wire presses pause automatic refresh and commit one
disconnect on release. Compatible opposite ports highlight/snap within 22 screen
pixels; invalid stages, self-links and cycles do not snap. Existing revisioned
native record transactions persist these operations. The installed CEP Junction
already targets this workspace; no CEP installation/host settings are changed.
Close/reopen CEP after loading the new native build.

Packed 32778 (0x800A); main schema 18, native identity revision 2, graph schemas,
gateway token native-node-sync-6 and C ABI unchanged. No project migration. SDK
build and both CEP JavaScript syntax checks pass; no tests added/rerun. Build
log: artifacts/build10-multi-emitter-build.log. Candidate hashes are in the build
matrix. Candidate publication was disabled with -NoRuntimePublish -NoDistPublish.
Source pushed as c368bbf before deployment. **Build 10 is deployed** after an
immediate read-only check found neither AfterFX nor AfterFX_64, under standing
owner authorization. All six installed hashes match the build matrix; selected
versioned Core hash is verified separately. Before: build-9 AEXs and
StarfieldCore-6D70281C4E756BCA.dll. After: build-10 AEXs and
StarfieldCore-AE18EFC2E9856178.dll. The existing single Plug-ins/Starfield -> dist
Junction is retained. No AE process started/stopped or other host state changed.
Backup: artifacts/disabled/p02d-build10-multi-emitter-20261002/deployment.json.
Before/after records and deployment log are under artifacts/build10-deploy-*.json
and artifacts/build10-deployment.log. Later core-only changes retain hot updates.

One-step undo after deployment (AE closed):

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Deploy-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02d-build10-multi-emitter-20261002' -Rollback
```

Remaining owner checks: multiple Emitter -> Particle paths feeding Output,
wire click disconnect, snapped reconnection, and rapid editing/time changes
without cancellation dialogs. Native node undo and save/reopen remain unqualified.

## Build 9: numeric node disk IDs — 2026-10-02

Owner build-8 evidence: the same duplicate-matchname error persists. Its topic
repair was insufficient. Every node disk ID was a ten-digit FourCC, outside
the SDK's 1..9999 range. The name-length model also predicts Particle/Appearance
collisions in the 40-byte stream-name buffer; host truncation remains inferred.

Build 9 uses shared explicit numeric IDs for controls, curves, layout, connections,
UUID and guard; registration and supervised lookup use the same table. Compile-time
guards check all ID ranges/uniqueness and the effect-name budget. Native identity
revision 2 requires fresh development effects; no FourCC migration. Stream
indices/counts/types, main schema 18 and CEP token native-node-sync-6 stay unchanged.

SDK build passes; log: artifacts/build9-node-numeric-ids-build.log. No tests
added/rerun. **Build 9 is deployed**, packed 32777 (0x8009), after confirming
no AE process, under standing owner authorization. Six hashes match the build
matrix. Before: build-8 AEXs; after: build-9 AEXs. Core/selected hot generation
unchanged; single Plug-ins/Starfield -> dist Junction retained. No AE process
started/stopped. Backup: artifacts/disabled/p02d-build9-numeric-ids-20261002/deployment.json.

Undo (AE closed):

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Deploy-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02d-build9-numeric-ids-20261002' -Rollback
```

Next owner gate: create a fresh Starfield effect; confirm default Particle
creation and one manual Particle addition. Actual AE acceptance remains open.


## Build 8: distinct topic boundary IDs — 2026-10-02

Owner build-7 evidence: Particle creation now fails with
`Duplicate matchname found during FillInStreamsFromCanonicalLayout` inside
`addProperty`, with `canAddProperty=true`. Particle creation remains unqualified.

Source registration reused each node topic's start ID for its end marker.
Build 8 gives GROUP_END independent IDs: `endE`, `endP`, `endF`; the shared
Particle/Appearance registration is corrected together. Compile-time assertions
require distinct start/end IDs. Values, indices, record counts, curves, UUIDs,
schema 18 and the CEP token `native-node-sync-6` are unchanged. These are
unreleased structural ID repairs; development schemas are not migrated.
Use a fresh effect for the creation check. The connection to the reported host
error is a source-based diagnosis until the owner confirms actual creation.

May 2023 SDK build passes with `-NoRuntimePublish -NoDistPublish`; log:
`artifacts/build8-node-topic-ids-build.log`. No tests were added or rerun this
iteration. **Build 8 is deployed**, packed version `32776` (`0x8008`), under
the owner's standing AE-closed authorization. The process check found no AE;
no process was started/stopped. Six installed hashes match the build matrix.
Before: build-7 AEXs; after: build-8 AEXs. Core DLL and selected hot generation
remain unchanged. Existing single `Plug-ins/Starfield -> dist` Junction retained.
Backup: `artifacts/disabled/p02d-build8-topic-ids-20261002/deployment.json`.

Undo (AE closed):

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Deploy-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02d-build8-topic-ids-20261002' -Rollback
```

Next owner gate: add Particle once in AE 2023.5.0 Build 52; capture any persistent
creation error. Previous Emitter/render tests are not repeated.

## Build 7: constant-control registration deployment — 2026-10-02

Owner build-6 evidence: Particle addition fails **inside `addProperty`**, before
gateway identity/parameter writes, with `spatial interpolation method not allowed
for this stream (1)`; `canAddProperty=true`. The module is recognized, but native
instance creation is rejected. Emitter duplication remains owner-confirmed.

Particle/Appearance uniquely register color controls. All node controls used
both CANNOT_TIME_VARY and CANNOT_INTERP. The extra interpolation restriction on
non-spatial color controls is a **hypothesis** for this host failure. Build 7
removes CANNOT_INTERP from node registration; CANNOT_TIME_VARY still guarantees
constant records, and visible controls remain supervised. UUIDs, indices,
parameter types, schema revision 18, native persistence and rendering are unchanged.
Code/PiPL version: `32775` (`0x8007`). CEP token stays `native-node-sync-6`.

Particle/Appearance actual-node fixtures pass 146 checks, including all controls
remaining constant, no interpolation restriction and both supervised colors.
May 2023 SDK candidate build passes with `-NoRuntimePublish -NoDistPublish`.
These are registration/selector checks, **not reproduction of AE's internal
interpolation implementation**. Actual Particle creation remains unqualified.
Build 7 was built on 2026-10-01 and **deployed on 2026-10-02** after the owner
authorized installation and future deployments while AE is absent (ADR 0011).
The pre-deployment process check found no AE process; no process was started or
stopped. All six installed hashes match the build-matrix table. Existing single
`Plug-ins/Starfield -> dist` Junction retained. Before: build-6 AEX set; after:
build-7 AEX set, unchanged Core and selected Core generation.
Backup: `artifacts/disabled/p02d-build7-constant-flags-20261001/deployment.json`.

Undo (AE closed):

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Deploy-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02d-build7-constant-flags-20261001' -Rollback
```

Next owner action is one Particle addition;
capture any persistent error. Previous rendering/Emitter tests are not repeated.

## Build 6: node SmartFX contract correction — 2026-10-01

The owner supplied actual AE startup evidence:
`PF_OutFlag2_FLOAT_COLOR_AWARE requires PF_OutFlag2_SUPPORTS_SMART_RENDER`.
All four internal node modules advertised float awareness but implemented only
legacy `PF_Cmd_RENDER`. This is a confirmed invalid flags/selector contract.
It can block node effects in a 32-bpc project; Particle creation still needs
real-host confirmation after correction.

Build 6 uses node flags2 `0x00001400` (SmartFX + float) in both PiPL/runtime.
The shared node implementation forwards input ROI/time/bounds in pre-render,
copies the input through the host World Transform suite in smart render, and
pairs successful input checkouts with checkin on output/suite/copy failures.
Nodes remain controls-only, menu-hidden effects; MFR/GPU remain disabled. Code/
PiPL version is `32774` (`0x8006`); parameter schema stays revision 18 and the
Core DLL is unchanged.

The owner's transient banner was actually `ResizeObserver loop limit exceeded`.
CEP now coalesces resize work into the next animation frame, ignores unchanged
observed sizes, and avoids redundant inspector-position writes. Only Chromium's
two known resize-delivery warnings are excluded from the script-error banner;
other script exceptions and native mutation failures remain visible. Token:
`native-node-sync-6`.

Checks: actual shared node EffectMain compiled separately for all four kinds,
**276 checks, zero failures**; focused startup/resize/error and native gateway
fixtures pass; May 2023 SDK candidate build succeeds. Fake copy callbacks verify
byte transport/error cleanup, not AE's implementation. **No AE acceptance claim.**
Candidate built with `-NoRuntimePublish -NoDistPublish`. Owner authorized the
specific ADR 0011 command; **build 6 is deployed**, all six file hashes verified.
Existing single `Plug-ins/Starfield -> dist` junction retained. Before: build-5
AEX set; after: build-6 AEX set, unchanged Core/selected Core generation.
Backup: `artifacts/disabled/p02d-build6-node-smartfx-20261001/deployment.json`.
Undo (AE closed): `powershell -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02d-build6-node-smartfx-20261001' -Rollback`.
The agent did not start/stop AE; the owner chose to test. Next owner gate:
fresh default Emitter/Particle and one Particle addition in the failing AE
project, including 32-bpc operation. Earlier rendering checks are not repeated.

## CEP 5b: preserve Particle failure evidence — 2026-10-01

Owner AE evidence: Emitter duplication now works. Particle is still absent by
default and cannot be added. The displayed failure disappears before capture.
**Particle creation and native node acceptance remain open.**

Source inspection found successful background refresh clears the mutation error
both before and inside `adoptState`. CEP 5b retains mutation/bootstrap failures
until manual Refresh or a successful user edit. The banner is selectable and
wraps long errors. A failed default bootstrap is attempted once per target per
panel session; manual Refresh permits another attempt. Ordinary reads continue.
Creation errors include the node match name, creation/identity/parameter stage,
and a diagnostic `canAddProperty` result when creation itself fails. Parameter
write failures include the exact control name. No successful Particle creation
is inferred from static registration, parameter indices, or fake-host checks.

Gateway/loader token: `native-node-sync-5b`. Only CEP sources changed via the
existing junction; build-5 AEX/Core files are unchanged. Close/reopen CEP and
attempt Particle once to capture the persistent actual AE error. Focused startup
checks cover retention and bootstrap retry suppression; the native gateway
fixture covers refusal to create Particle and verifies existing effects/links
survive rollback. These checks passed; the real AE failure detail is pending.

## CEP 5a hotfix after owner feedback — 2026-10-01

The owner reports build 5 creates native Emitter effects but repeatedly shows
`stale_graph`; copied effects do not appear in the canvas, and reopening CEP
loses the canvas. **Build 5 did not pass native node acceptance.**

The offending readonly `ensureNodeEffects` path compared a browser reconstruction
against the actual AE records and rejected reload itself. CEP 5a returns the
current Effect Parade records directly. Only mutation checks a host-produced
opaque authoring stamp, so decimal JSON/codec differences do not pretend that
the owner edited an effect. A targeted fixture reproduces rounded decimal JSON
and verifies a readonly reload performs no compile, copy/edit still works, and
a genuine intervening AE value change rejects before adding any effect.
The exact numerical mismatch in the owner's session was not captured; decimal
rounding is a reproduced hypothesis, not confirmed host evidence.

Numeric native payload CRCs remain diagnostic. A browser's rounded projection
cannot prove native byte equality; commit receipt, advancing native revision and
semantic node-record readback confirm a transaction. Additional supervised
callbacks may advance revision beyond exactly one. Failed graph reads keep the
last valid canvas and do not clear/re-show the error banner on every poll.

The gateway/loader token is `native-node-sync-5a`. This changes only workspace
CEP files through the existing extension junction: **no AEX replacement or AE
restart is needed**. Close/reopen the CEP panel. The actual effects are the source;
existing Emitter copies should become visible. If the Particle effect is absent,
add it from the node context menu and connect it. Partial first initialization now fills missing Emitter/Particle and initial links while ready=0; ready=1 deliberate deletion remains unchanged. Fresh-effect automatic bootstrap,
real render response, undo and reopen still require owner confirmation.


## Current build 5 handoff

- Owner confirmed build 4 fixed layer-selection crashes.
- Remaining initialization failure was the native expression read on non-time-varying
  Graph Snapshot (41). Build 5 removes expressions entirely, including diagnostic reads.
- Nodes are independent hidden AE effects with saved ordinary values, UUID, outgoing
  connections and layout. Output stays on the main render effect.
- First panel synchronization creates Emitter → Particle → Output. Later deletion
  follows the actual Effect Parade; an Output-only graph remains intentionally empty.
- Ordinary main revision 41 and payload checksum halves 90/91 replace the expression
  snapshot. A stale manifest/revision rejects an edit before mutation.
- 756 adapter checks use the actual GraphCarrier. Gateway checks deliberately throw
  on expression access and cover add, duplicate, independent values/curves, movement,
  insert/connect/disconnect, AE reorder/direct deletion, raw Ctrl+D re-key, rollback
  and delete-all. The May 2023 SDK build succeeds.
- Build 5 is deployed and six hashes verified. Backup:
  `artifacts/disabled/p02d-build5-native-streams-20261001/`. One existing Starfield
  junction remains. Core content is unchanged.
- The owner chose to test; no AE session was started by the agent. Reopen CEP and
  use a fresh effect/layer. Verify Emitter and Particle appear as separate effects,
  then add/copy/edit/connect/disconnect/delete and check independent saved values.
  Undo/redo and save/reopen remain separate host gates.
- Automatic node creation with CEP closed, multiple active emitters and more than
  four outgoing connections remain open. Raw AE duplicate re-key now exists in
  the refresh source path, but its real host behavior is not accepted yet.

The earlier build sections below retain failure history, not current deployment state.


## Build 4 correction and current deployment gate

The owner confirms build 3 can render, but selecting the layer crashes AE.
Dump `cf077068-7651-46c6-a82c-4f8d4e451f8e` has the same null read as the prior
dump (`AfterFXLib.dll+0x1931d36`, thread 31152). It loads the main AEX/Core,
not the node AEX modules. Source registers hidden group starts with visible ends;
that hierarchy is a suspected selection/UI fault. Build 4 removes those unused
structural markers and leaves one balanced Output group. The development
parameter schema advances to 17; code and PiPL advance together to `0x8004`.
Fresh effects are required. This is a candidate fix, not a confirmed resolution.

721 adapter checks and the full May 2023 SDK build passed. A focused deployment
fixture verified read-only report mode, one bundle junction, hashes, hot runtime
selection, complete rollback, and preservation of another plug-in. No repeated
CEP/core suites or AE test operations were run. The owner authorized stopping
AE PID 31772 and deploying; build 4 is now installed with all six hashes checked.
AE was not restarted. Layer-selection safety still requires the owner's check.

The installer archived loose Starfield files and the root runtime
junction, then creates only `Plug-ins/Starfield -> dist`. The nested
`dist/StarfieldRuntime` is a real folder. Core-only builds publish there without
touching AEX files. The old single-effect and loose-pair installer scripts were
removed. Backup: `artifacts/disabled/p02d-build4-single-folder-20261001/`.
See the build matrix for exact command, hashes and one-step rollback.
The record below describes build 3 and earlier evidence.

## Implemented source

- Separate Emitter, Particle, Appearance and Force effect instances own values,
  UUID, outgoing connections, percentage curve banks and signed canvas positions.
  The main effect owns Output, its cap/position and the compiled render snapshot.
- Graph edits use ordinary node streams and numeric compile trigger 43. No request
  expression or parameter 90 remains. Snapshot 41 is a read-only expression mirror.
- Main guard 42 suppresses intermediate Output commits. Per-node guards suppress
  intermediate edits. Marker 89 distinguishes first creation from deleting all nodes.
- Failed transactions restore native effects, Output metadata and the compiled
  snapshot. Indexed-group references are reacquired after structural mutations.
- Semantic acknowledgement tolerates AE float/color quantization while checking
  node/edge identity, topology, values, curve knots and complete layout.
- Build-only child environments normalize duplicate Path/PATH names. Candidate
  builds can avoid publishing either dist or the runtime selector.

## Completed checks

The eight focused CEP suites passed: gateway, startup, native-node gateway,
graph transactions, codec, edits, view and layout. The final transaction/native-node
checks also passed after the snapshot exception fix. Adapter: **687 checks, zero
failures**. The full May 2023 SDK x64 Release build succeeded with five AEX modules
and the paired Core. Core algorithms were unchanged and the broader core suite was
not repeated. These results qualify source/build behavior, not AE host behavior.

## Actual AE evidence and failure

Target: **After Effects 2023.5.0 Build 52**, Windows x64.

The initial six-file revision-16 build-2 pair was deployed with hashes checked and
backup record `artifacts/disabled/p02d-native-records-20261001/deployment.json`.
CEP remains the existing junction to this checkout; no preferences, registry keys,
cache directories or additional junctions were modified.

Two CLI launches did not execute the JSX bridge. A File > Scripts > Run Script File
launch then wrote `script_started`, created a clean temporary composition and added
the main renderer. Its last breadcrumb was `renderer_added`; AE crashed while opening
the composition viewer, before gateway initialization and before node operations.
The owner also reported the crash. The test did not save the project.

The crash dump `c991e55b-7da9-4790-a054-7aa0f322affc` records access violation
`0xc0000005` in `AfterFXLib.dll+0x1931d36`, on main thread 29580. The exception stack
contains host/TDB frames and no direct Starfield return frame. Both the candidate
AEX and selected Core were loaded. This evidence does **not** isolate the cause or
exonerate the plug-in. The historical PowerShell dump parsers use incorrect packed
offset assumptions on this dump and cannot be relied upon for its exception fields.

Build 3 updates code and PiPL version together (`0x8003`) after the registered
parameter count changed, and clears group-end definitions rather than inheriting
slider flags/unions or an arbitrary-data handle. AE's log reports plug-in caching;
stale metadata is a hypothesis. These changes are defensive fixes, **not a confirmed
resolution** of the crash. No automatic repeated host runs are scheduled.

## Remaining acceptance

1. Establish safe fresh-effect apply/preview with the build-3 pair.
2. Bootstrap separate node effects; add/copy two Particle nodes with independent values.
3. Disconnect/reconnect/delete, including Output-only deletion, and rendered response.
4. Direct Effect Controls edits, deletion reconciliation and origin unit parity.
5. Undo/redo and save/reopen; menu-hidden add-by-match-name.

Automatic node creation without an open CEP panel is still open. Raw native Ctrl+D
duplicates retain a UUID and are detected as an identity conflict; automatic re-key
and import are not implemented. Multiple active emitters remain a core gate.
The current native record permits at most four outgoing connections per node.

## Deployment and undo

`tools/Deploy-TestBuild.ps1` reports by default. `-Install` archives prior paired
files and selector, copies candidates, verifies hashes and records one-step rollback.
It requires AE closed and the existing single runtime junction. It does not start
or stop processes and does not change system settings.

For the first deployed candidate, with AE closed:

```powershell
powershell -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02d-native-records-20261001' -Rollback
```

Build 3 is now deployed with all six hashes verified. Its backup record is
`artifacts/disabled/p02d-build3-20261001/deployment.json`; current hashes and its
one-step rollback command are in the build matrix. AE was closed by the owner
before this deployment and has not been restarted by the agent. Create fresh effects for development parameter schemas;
older development projects are not migrated.
