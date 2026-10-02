# P-02D native node checkpoint — 2026-10-02

## Build 19 P-02J follow-up — 2026-10-02

Owner rejected build 18: gateway_missing despite the JSX animation-18 token;
Origin XY keys did not animate and produced black output. Panel expected sync-14,
so the gateway/panel mismatch is confirmed. Both now use animation-19, and the
startup regression executes the actual JSX readiness function.

The old binding expression called intermediate host objects and swallowed all
errors into zero. This is a hypothesis for the AE black output, not a confirmed
host diagnosis. Generated bindings now use Layer.effect(index)/Effect.param(index),
catch only unrelated-effect identity probes, and return an unavailable sentinel
when no UUID matches. UI installation evaluates every installed binding and checks
its enabled state before publication. Failure restores previous values/expressions;
UI and render diagnostics include the failed binding stream. Render still samples
only its own PF inputs; no Core ABI, parameter ID or node layout changes.

Packed version 32787 / 0x8013. Native sync 747, camera 12 and renderer controls 39
checks pass (798 total). Pixel regression renders the sampled graphs with the
actual CPU backend, checking visible alpha, changed frames and reverse-time
repeatability. Generated-expression tests use non-callable host-object fixtures,
missing identities and source failures; actual JSX/panel handshake tests pass.
All five May 2023 SDK Release /MT AEXs build with no warnings/errors in the log.
Logs: artifacts/build19-*. No AE session operated. AE Origin XY interpolation,
CEP-closed playback and reopen remain owner qualification gates.

An initial mistyped runner switch selected the old broad Core suite (269 failures,
including outdated graph/codec expectations); this is not counted as passing
evidence. No broad-suite qualification is claimed by this scoped follow-up.

## Build 18 P-02J candidate — 2026-10-02

Owner confirms build 17 native edits work, then reports almost all stopwatches are
unavailable. Public native controls no longer carry CANNOT_TIME_VARY. Main manifest
22 appends 512 hidden expression-capable numeric dependencies at indices 98..609,
disk IDs 1000..1511. UUID/property/component bindings are installed transactionally
on the UI path. Optional record 0x8002 saves typed raw fields and slots; own PF
checkouts sample each requested frame, including points, RGB, opacity, Life and
Force. The native conversion code is shared. No render-time AEGP acquisition.
Failed graph publication restores changed expressions. CEP edits keyed values at
current comp time, protects user expressions, and never asks constant metadata
for expression state. No authored keyframes are stored on the main effect.

Packed version 32786 / 0x8012; node IDs/schemas/layout 6 and Core ABI 2 unchanged.
Native sync 404/camera 12, four node registration suites 306, renderer registration
39 checks pass (761 total). Actual generated expressions pass JavaScript execution
with reordered/same-name/duplicated peers; focused CEP keyframe preservation checks
pass. May 2023 SDK x64 Release /MT final candidate builds all five AEXs with
-NoRuntimePublish -NoDistPublish; no compiler warnings/errors found in its log.
Logs: artifacts/build18-*. No AE session operated. These checks do not qualify
AE expressions or keyframe playback. Recreate effects on a fresh test layer;
owner tests stopwatches, interpolation, CEP-closed rendering, undo and reopen.
Animation uses current-frame settings; birth/history integration is separate.
See ADR 0023.


### Build 18 deployment completed

Source 176e05260dccafc4d51117f265bf0433d6816dcf pushed before deployment.
Immediate read-only process check found no AfterFX/AfterFX_64. Installed through
the existing single Plug-ins/Starfield -> dist Junction under standing permission.
No AE process started/stopped. Six installed candidate hashes, selected/pinned
Core parity, prior build-17 bundle hashes and selector backups verified.
Before/after records: artifacts/build18-deploy-before.json / build18-deploy-after.json.
Backup: artifacts/disabled/p02j-build18-native-animation-20261002.
No AE playback qualification claimed; test on a fresh layer with recreated effects.

| Installed file | SHA-256 |
|---|---|
| StarfieldParticle.aex | 1497C4CA80550C2FF07766F311D49496E278B5458368CE6BD61E2A7FE672168F |
| StarfieldEmitter.aex | 60F84E262B7CF011A305851F4B0A1A7877F2865D3ED29AF45EC04A6E69D031A7 |
| StarfieldParticleNode.aex | B482E90CBE96190F401C18CD23448A62D5E7F9C693AE15C6F6091D7BD8B1777B |
| StarfieldAppearance.aex | 29F2B8B5358F421CCEE957880FE3CCA2B3BA8B5DDFE38737A6BA9A711DE890BE |
| StarfieldForce.aex | 7C549D970996D39D568F274919E154F75F8067B41665BD8BC59B2F6325D0BCAA |
| StarfieldCore.dll | 0D3C8D672DE171D70DF699C3B8E14A9133E91AB43F74F14B8617F7517E3DA5E7 |

One-step rollback (close AE first):

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Deploy-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02j-build18-native-animation-20261002' -Rollback
```

## Build 17 candidate: delivery failure in build 16

Owner screenshot reports `delivery (parameter 4, stream -1, error 516)`. The
receiver did not advance the edit to its context phase; the screenshot does not
distinguish an ignored selector from an AEGP API rejection. Build-16 host acceptance
failed, so the generic context patch did not resolve the observed problem.

Build 17 removes the inter-effect generic edit call and main edit selector. Every
native node AEX includes a shared graph compiler/publisher and codec. Native UI
callbacks directly compile sibling node records with the node's own AEGP ID,
substitute the callback's new value by UUID/index, then save/readback/rollback the
main renderer streams. No main registration or renderer callback context is needed.
Normal AE arbitrary-data ownership callbacks remain. NativeEdit is only a local
borrowed UI context; main/node schemas, public IDs, graph persistence and Core ABI
stay unchanged. CEP continues using its existing supervised commit path.

Native sync 348/camera capture 12 scoped checks pass. The generic API deliberately
returns 516 if invoked, and the fixture leaves the main AEGP registration absent.
Actual native callback edits still save new values and the generic call count is
zero. Existing receipt typing, exact integer checks, graph/scalar failure phases,
rollback and balanced-resource checks remain. Logs: artifacts/build17-native-sync.log.
Renderer controls 38 scoped checks and final May 2023 SDK x64 Release /MT build
pass at packed 32785 / 0x8011, including all four independent node links, with
-NoRuntimePublish -NoDistPublish. No compiler warnings/errors found in the build
log. Logs: artifacts/build17-renderer-controls.log and build17-sdk.log. Candidate
source is pushed before the paired deployment. No AE session
operated; Effect Controls behavior with CEP closed, undo/redo and reopen need owner
qualification.

### Build 17 deployment completed

Source `b0e9e6b45342e9df1b6c182b9bfc834fa12182a5` was pushed before installation.
Immediate read-only checks found neither AfterFX nor AfterFX_64. Standing permission
covered the existing single Plug-ins/Starfield -> newStardust/dist Junction install.
No process started/stopped and no other host setting changed. All six installed
candidate hashes, unchanged selected Core, prior build-16 bundle hashes and selector
backup verified. Records: artifacts/build17-deploy-before.json, build17-deploy-after.json
and build17-deploy.log. Owner AE native editing qualification remains open.

| Installed file | SHA-256 |
|---|---|
| StarfieldParticle.aex | C909F341C018A7D6C6445AAD842848269D47A51AC0562763FFED021034B0CC49 |
| StarfieldEmitter.aex | CB05F0E5192F64025E19442873A73D8A7CD832656A4DDEC99B4B16AC01FB9EF8 |
| StarfieldParticleNode.aex | 446EE0548FC5BC844A37159FF8FC600C8D6D88287ADF5EAE72E3BFFE5621A2CF |
| StarfieldAppearance.aex | F2BF85BB5850AB39A8F848232ABD0F93039FAF3313DB5EB730F5DCFF4F0EC9BA |
| StarfieldForce.aex | C5C51B13A688F87586FB002D712CE31C44F1FA8CA67B3E4EFA8963CBE468CB36 |
| StarfieldCore.dll | 0D3C8D672DE171D70DF699C3B8E14A9133E91AB43F74F14B8617F7517E3DA5E7 |

One-step rollback to build 16, with AE closed (from repository root):

```powershell
powershell -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02i-build17-direct-native-commit-20261002' -Rollback
```

## Build 16 candidate: shared native edit error 516

Owner reports every tried Effect Controls edit fails, including Origin parameter 4.
Build 15 host acceptance therefore failed. Its generic callback required a complete
98-entry params array/input image and PF context that the inter-effect contract does
not promise. The old message did not identify the actual failing phase; this is the
identified incorrect assumption, not proof of the precise AE rejection site.

Build 16 request v2 borrows the native UI caller's renderer/layer refs and suite /
handle callbacks for the synchronous call; dimensions/PAR/time are numeric only.
Missing geometry/time is read from source item/layer. The main generic handler can
receive null in_data/params and reads only eight needed main controls via AEGP.
Node values remain separate AE effects; a single callback value is substituted by
UUID/index while compiling the saved graph. No schema, parameter ID or Core ABI change.

Review follow-up: scratch params are not persistence. Publish scalars / publish
graph and verify scalars / verify graph are distinct diagnostic stages with stream
indices. The four scalars are source 2, integer revision <= 16777215 and two 16-bit
CRC halves, so exact equality survives float storage. Continuous node controls are
not compared by that receipt code. OneD/ARB types are checked before union reads.
Graph remains last; failure restores every attempted stream write.

Scoped evidence: native sync 330 and camera capture 12 checks pass. This exercises
null generic callback context, missing UI geometry/time, delayed native values,
wrong receipt types, float quantization at maximum revision, scalar/graph failures,
readback rejection, rollback and balanced handles. Logs: artifacts/build16-native-sync.log.
Renderer controls 38 scoped checks pass. Final May 2023 SDK x64 Release /MT build
passes at packed 32784 / 0x8010 with -NoRuntimePublish -NoDistPublish, with no compiler
warnings/errors found in its log. Logs: artifacts/build16-renderer-controls.log
and build16-sdk.log. Candidate source is pushed before the paired deployment.
No AE session operated; native editing, undo/redo and reopen need owner qualification.

### Build 16 deployment completed

Source `76801bb237fe7ae974823cb4a453c222972457d5` was pushed to
origin/codex/m3-01b-emitter-dimensions before deployment. Immediate read-only checks
found neither AfterFX nor AfterFX_64. Standing authorization covered the install
through the existing single Plug-ins/Starfield -> newStardust/dist Junction.
No process or other host setting changed. Installed candidate hashes, selected
unchanged Core, all six prior build-15 bundle hashes and runtime selector backups
were verified. Records: artifacts/build16-deploy-before.json, build16-deploy-after.json
and build16-deploy.log. Real AE native editing acceptance remains open.

| Installed file | SHA-256 |
|---|---|
| StarfieldParticle.aex | 1E1BF19D0750089127D787610521BBFBAB00EED6A7A417FEDAB3CC2C24B4A43E |
| StarfieldEmitter.aex | 8A3F7C631036EB34644DCF1B6E930143B239C6BA5C6978B5366F419BF8440C52 |
| StarfieldParticleNode.aex | 25E7E3AD43636D7A9FC0D11C1BE6B0E2D10B3A6980913C34D5D914F89942359F |
| StarfieldAppearance.aex | B58885BBB083D8D5762C79190A44EF8EFE7C3610FE4254A215781D56BFEBAB54 |
| StarfieldForce.aex | 1FA7458C52F02A1D3228F9D5638A9CB78A55309428DC5B3532FA903211941433 |
| StarfieldCore.dll | 0D3C8D672DE171D70DF699C3B8E14A9133E91AB43F74F14B8617F7517E3DA5E7 |

One-step rollback to build 15, with AE closed (from repository root):

```powershell
powershell -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02i-build16-generic-context-20261002' -Rollback
```

## Build 15 candidate: native Effect Controls commits

P-02I replaces synthetic supervised inter-effect calls with COMPLETELY_GENERAL,
carrying the edited value by UUID. Main graph/revision/checksum/source are saved
with AEGP streams, read back byte-for-byte and restored on failure. Ignored calls
cannot report a successful edit. No layout, schema, public ID or Core ABI changes.

Minimum checks: native sync 252 and camera capture 12, all passing; renderer
registration/pre-render 38 passing. The sync fixture retains old source values
until callback return and discards callback-array edits, then verifies the actual
saved graph. It covers Origin at Quarter, scalar values, angles/popups, Life,
opacity/color, Force, independent curves, missing UUIDs and failed/ignored writes.
Scratch logs: artifacts/build15-native-sync.log and build15-renderer-controls.log.
May 2023 SDK x64 Release /MT candidate build passes at packed 32783 (0x800F),
with -NoRuntimePublish -NoDistPublish; source push precedes deployment.
No AE session operated; native edits with CEP closed, undo and reopen remain open.

### Build 15 deployment completed

Source `96ecf6d` was pushed to origin/codex/m3-01b-emitter-dimensions before
installation. Immediate read-only checks found neither AfterFX nor AfterFX_64.
Standing authorization covered one Deploy-TestBuild install through the existing
Plug-ins/Starfield -> newStardust/dist Junction. No process started/stopped.
All six installed candidate hashes, selected Core and build-14 backup hashes /
selector verified. Core is unchanged. Records: artifacts/build15-deploy-before.json,
build15-deploy-after.json and build15-deploy.log. Real AE acceptance remains open.

| Installed file | SHA-256 |
|---|---|
| StarfieldParticle.aex | C17A593165C31289903554D31E91E603B82B62DD2808C5A607D5F61CA481983D |
| StarfieldEmitter.aex | 6146192F9BFEF116844FB03EAFA09FDE4D08B1907F8566012C239DA6FA64B356 |
| StarfieldParticleNode.aex | 8FD45094CCAC4ACBFE474446FFC4ABF5EEA864F28B8E7E4375280DFAA336CBF7 |
| StarfieldAppearance.aex | F5FD6510B78CB812B5CD118CFCAFBB982B01094F846DE1AEFA53272FA83308A7 |
| StarfieldForce.aex | 46F49B38926973562E2713411560AD515D4344F3978B811CB5FB371C78025157 |
| StarfieldCore.dll | 0D3C8D672DE171D70DF699C3B8E14A9133E91AB43F74F14B8617F7517E3DA5E7 |

One-step rollback, with AE closed (from repository root):

```powershell
powershell -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02i-build15-native-control-commit-20261002' -Rollback
```

### Appearance review

Confirmed: gateway registers Appearance, native base counts are Particle 43 and
Appearance 42, both use add_particle_parameters (the boolean adds Life), and CEP
still permits creating Appearance. The default chain omits it. Native compiler
conditional offsets correspond to those layouts and do not alone imply wrong
reads. Core treats Appearance as an active optional downstream override, rejects
multiple active overrides per Particle stream, and replaces appearance values
before one apply_appearance call. It is overlapping design, not unreachable code.
No causal link to delayed native edits is established. Removal would touch Core
schemas, node modules/build/deploy manifests and CEP; it is not silently folded
into this synchronization patch. Owner's requested Particle naming remains the
default topology.

## Build 14: ordinary Force, renderer globals and population default

P-02H implements scalar Gravity/random, separate Wind XYZ, Spin/frequency/resist/
delay and Air Density, plus a separately saved Wind/Spin percentage curve. It
keeps independent native effects and uses analytic, stateless forcing. Main adds
functional Time Remapping and Preview/Particle chance. Fresh cap and CEP examples
use 1000000. Low-count scenes do not allocate the full cap. The invalid SDK
Headers/Win include is removed from editor/MSBuild/test inputs.

Candidate: packed 32782 (0x800E), main 21/native layout 6/Force 2/Output 3/Core
ABI 2; fresh effects and no development migration. May 2023 SDK x64 Release /MT
build passes with -NoRuntimePublish -NoDistPublish. Core 58, actual Force selector
79 and scoped main registration/pre-render checkout 38 checks pass; current-node
CEP checks and six source parses pass. Scratch logs are artifacts/build14-*.
Real AE behavior, exact reference motion and remaining main capabilities remain
open. See ADR 0021 and reference-main-force-comparison.md. Source is pushed before
the one paired AEX deployment; no AE process started/stopped or GUI operated.

### Build 14 deployment completed

Source `6a32f94822d517fefd40e8e718497ae9ecff3a0e` was pushed to
`origin/codex/m3-01b-emitter-dimensions` before installation. Read-only checks
confirmed neither AfterFX nor AfterFX_64 was running. Standing owner permission
covered this deployment; no process or host-wide setting changed.

Retained the existing `Plug-ins/Starfield -> newStardust/dist` Junction. All six
installed hashes equal the final candidates; selected Core and build-13 backup
hashes/selector also verified. Before/after records are
`artifacts/build14-deploy-before.json` / `artifacts/build14-deploy-after.json`.

| Installed file | SHA-256 |
|---|---|
| StarfieldParticle.aex | 9C24EED3596756920BEF02E72C9F1BCFFCB4A63DA0D8B917E456091D43A8C58E |
| StarfieldEmitter.aex | 833D16809723722E1272A644068F8FE074DFEDBEB69393B2606842796FDDB370 |
| StarfieldParticleNode.aex | B8CEA3F99D03132ABE2884E2B2B3AE7CB56DB3E4451B4C2689F3A86E54329FD1 |
| StarfieldAppearance.aex | 8CABACD37761C255023458181C75723766CA19C1160FDC70DEE9132E088FFD16 |
| StarfieldForce.aex | CD2C35088409E25FCF90FC70D86A86884E4388BB5A66ADF24F565E6D9E096B6A |
| StarfieldCore.dll | 0D3C8D672DE171D70DF699C3B8E14A9133E91AB43F74F14B8617F7517E3DA5E7 |

Runtime selector: `StarfieldCore-0D3C8D672DE171D7.dll`.
Backup: `artifacts/disabled/p02h-build14-reference-force-globals-20261002`.
With AE closed, one-step rollback from the checkout:

```powershell
powershell -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02h-build14-reference-force-globals-20261002' -Rollback
```

Owner AE tests remain pending; deployment verification does not establish them.

## Build 13: current-node interaction, camera and Auxiliary — 2026-10-02

P-02G fixes setup-only uu.id/change_flags aliasing in native edit dispatch,
selects the UUID-owned native effect on CEP node click and skips unchanged node
manifest writes. The source/Particle/Force/Output architecture remains independent
native effects with immutable compiled render graphs.

Unified Emitter schema 5/native layout 5 exposes Default/Auxiliary mode, parent
particle input, chance, life-window and inheritance percentages. Child positions
are sampled at birth and survive parent death. The deterministic comp-zero clock
and percentage inheritance kernel are independent implementations, not a claim
of reference timing equivalence. Auxiliary live count and advanced reference
source controls remain follow-ups. CEP mode changes disconnect parent wires;
native mode changes require those wires disconnected first.

Core ABI 2 carries numeric camera matrices. Main PiPL/runtime add I_USE_3D_CAMERA;
SmartRender captures SDK geometry and the Core projects and sorts visible sprites,
with inverse layer mappings before AE's later transform. Default view geometry is
used when available; missing default geometry retains flat output. Non-square PAR,
extreme 3D layer angles and all actual camera behavior remain owner host gates.

Packed 32781 (0x800D), main manifest 20, identity 3/layout 5, gateway
native-node-sync-13. Require fresh development effects; no old-layout migration.
May 2023 SDK /MT candidate builds. Owner-authorized minimal scopes pass:
Core 28, native sync 14, camera capture 12, Emitter 77, Particle 73 checks;
three focused CEP suites pass. No AE GUI session operated or broad tests run.
Candidate build uses -NoRuntimePublish -NoDistPublish. Build and scoped logs stay
under artifacts/build13-*.

Source pushed as **2751a56** before deployment. **Build 13 is installed** under
standing authorization after an immediate read-only check found neither AfterFX
nor AfterFX_64. All six installed hashes equal the candidate, the selected Core
equals the pinned DLL, and all six saved build-12 backup hashes plus its selector
equal the before-state record. Runtime selects StarfieldCore-25F80103DAB9E950.dll.
The existing single Plug-ins/Starfield -> dist Junction remains unchanged.
No process starts/stops or registry/CEP host-setting changes occurred.

Before/after: artifacts/build13-deploy-before.json and build13-deploy-after.json.
Deployment log: artifacts/build13-deploy.log. Backup record:
artifacts/disabled/p02g-build13-current-node-interaction-20261002/deployment.json.
CEP still uses the existing source Junction; close/reopen its panel. Fresh native
development effects are required for layout 5. No actual AE host acceptance is
claimed from the installed hashes or fake-host checks.

One-step plugin rollback after closing AE (CEP source is tracked separately):

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Deploy-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02g-build13-current-node-interaction-20261002' -Rollback
```

Owner acceptance: native XY/Z and other edits, correct effect selection after
duplicate/reorder, camera movement/zoom, parent/child stream wiring, multiple
parents, inheritance/chance/life windows, copies, undo and save/reopen.

## Build 12: shared Particle, reference controls and curve rendering — 2026-10-02

P-02F removes all four single-source blockers: Particle's port limit, editor
edge replacement, generic emitter-merge rejection and evaluator one-parent
assumption. Each (Emitter, Particle) pair gets a branch; properties/forces are
planned once per Particle. Own emitter births/UUIDs and global Output cap remain.
Outgoing native storage remains four links per node. Emitter copies retain
mapped outgoing connections; mixed copies use the copied Emitter.

Life (Seconds) defaults to 2, caps at 10000 and uses CEP step 0.1. Normal AE
slider bounds are separated from typed bounds for Life, size, counts/seed,
dimensions, speed, Origin Z, angles, Disc and Drag. CEP steps are explicit.
Implemented names/units/order/defaults follow the observed reference table.
Origin XY is a true 2D point and Origin Z a centered scalar; opacity and
Speed Random display percent. Emitter schema 4 stores random percent in key 22,
independently of base Speed including zero. Native layout 4/identity 3 reserves
removed Origin ID 106 and uses 123/124; main manifest 20 keeps 89 streams.
Packed 32780; gateway native-node-sync-12. Envelope/C ABI unchanged; no migration.

Base Size/Opacity never rewrite curve points. Separate projected-value and
canonical-record getters fix summary shadowing. Output counts actual surviving
branches across all emitters. Renderer skips zero-alpha sprites and disables
the default arbitrary coverage cutoff; row-wise cancellation, storage/ROI,
particle/graph limits and opt-in finite work budgets remain. Visible large
sprites still incur full render work; output is neither dropped nor approximated.

May 2023 SDK /MT candidate and four JavaScript parse checks pass. No test suite
added/run; owner AE host checks remain pending. Candidate build leaves dist and
runtime selection unchanged. Build log and before/candidate capture are under
artifacts/build12-shared-particle-controls-build.log and build12-deploy-before.json.
The existing CEP Junction points to source; reopen the panel with build 12.
Use a fresh development layer/main effect to exercise the new layout/defaults.

Source pushed as cadab2a before deployment. **Build 12 is installed** under
standing authorization after immediate read-only checks found neither AfterFX
nor AfterFX_64. All six installed hashes match the candidate; the selected
runtime Core also matches StarfieldCore.dll. Core selection changed from
StarfieldCore-AE18EFC2E9856178.dll to StarfieldCore-3FC7633A1CFC8165.dll.
Before/after state: artifacts/build12-deploy-before.json and build12-deploy-after.json;
deployment log: artifacts/build12-deploy.log. Build-11 AEX/Core and selector
backups are retained and their six hashes match the before-state record.
Backup: artifacts/disabled/p02f-build12-shared-particle-controls-20261002/deployment.json.
The existing single Starfield -> dist Junction is retained. No process starts,
stops, registry changes or other host changes are part of this deployment.

One-step plugin rollback after closing AE (CEP is tracked separately by Git):

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Deploy-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02f-build12-shared-particle-controls-20261002' -Rollback
```

Owner gates: shared inputs and downstream fields, copies, Life/default/steps,
XY/Z preservation, Speed=0 random percent retention, both Over Life curves,
direct AE edits, cancellation, native sync, undo and save/reopen.

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
