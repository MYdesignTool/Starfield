# Build and host matrix

## Owner acceptance / main integration - 2026-10-04

The owner reports the build31 issues are basically resolved and explicitly
authorizes merging this installed version into main. This is an accepted
development checkpoint for AE 2023, superseding the previous merge hold.
No new exact timing numbers or exhaustive host matrix results were supplied;
unreported render-queue/MFR/new-host gates remain open. Development continues
on a separate branch for the Particle gradient editor and reference controls.
The installed build31 and its paired rollback remain unchanged by integration.

## M3-06 / build 31 - post-load bootstrap and Appearance removal

Build30 failed owner no-Options startup qualification: W57/57 P57 E0 but
Temporal0/57, PF193372 and history1671.3ms (CUDA16.3ms). Build31 (32799/0x801F)
adds a session-resident General AEGP for bounded, read-only main-thread idle
certification after load; sequence callbacks only maintain SFU1. Actual generic
PF context availability and no-Options first/reopen preview remain owner gates.
See ADR0026 for the protocol, lifetime rules, retry/state checks and limitations.

Owner explicitly requires complete Appearance removal without compatibility.
Only Emitter (including Auxiliary), Particle, Force and Output remain registered.
Particle owns style/Over Life; connection/layout edits only write node records.
The retired AEX is archived on installation; one-step rollback restores it and
removes the new StarfieldHost.aex. Main manifest24/IDs, remaining native schemas,
C ABI3 and SFU1 remain paired. Old graphs containing Appearance must be recreated.
Gateway native-idle-31; reopen CEP. Main integration waits for AE qualification.

### Build31 deployment - 2026-10-04

Source `934c211` is pushed on `codex/m3-01b-emitter-dimensions`. A read-only
process check immediately before installation found neither AfterFX nor
AfterFX_64 running. Deploy-TestBuild.ps1 installed through the existing single
Plug-ins/Starfield -> newStardust/dist Junction. Six current file hashes match
candidates; all six old build30 hashes match retained backups. Appearance is
absent from the loadable bundle and archived with its exact previous hash.
StarfieldHost is newly installed. Thirteen CEP source snapshots retain both old
and installed hashes. Core runtime selector matches the installed Core hash.
The read-only paired Restore-TestBuild report passes. No process was started or
stopped; registry, caches, CEP installation and host-wide settings were untouched.

| Installed file | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | `D7DFF0B2C449D8AF7E1C53C4CD7B18D0CCB595853151D62F3A4D6566E439BCC2` |
| StarfieldEmitter.aex | `8842F67B29587A42B3C01D28B2F0B1E0B76D36E9E59498F9F297EBC418169949` |
| StarfieldParticleNode.aex | `824418E24382B6829896B342D02F0A081F1DB3E555D38FCB41C211C6C9678038` |
| StarfieldForce.aex | `35D4F02938F376AEA273CD9C7CFB883B924C2C4981D11938D79CE026ED5101B8` |
| StarfieldHost.aex | `8DC6B3DE8BEF589D939DB1333BF6DE256F96779D3227328BBDDE16717A4C8B98` |
| StarfieldCore.dll | `651E3685631A87BB70DEE723A74F57FAE1C75296440A73F74D9FE83E009539AA` |

Runtime: StarfieldCore-651E3685631A87BB.dll. Before/after evidence is
artifacts/build31-deploy-before.json and build31-deploy-after.json; deployment
and rollback report are build31-deploy.log and build31-rollback-check.log.
Backup: artifacts/disabled/m306-build31-idle-particle-style-20261004.
The pre-edit CEP snapshot was verified before binary installation; an initial
snapshot path/encoding check refused deployment without changing the bundle.

One-step undo to the exact previous bundle and CEP sources, with AE closed:

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Restore-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'm306-build31-idle-particle-style-20261004' -Restore
```

Undo restores build30, including its known Options workaround and Appearance,
and archives the new Host/candidate sources. Rollback has not been executed.
Actual AE no-Options first/reopen performance and Particle -> Force interaction
remain owner qualification gates; source is not merged into main.

### Build31 candidate checks - 2026-10-04

May 2023 SDK, Release x64 /MT build passes for main, Emitter, ParticleNode,
Force, StarfieldHost and paired Core. Candidate construction used
`-NoDistPublish -NoRuntimePublish`, leaving the installed build30 untouched.
Evidence: artifacts/build31-native-build.log.

| Scope | Checks | Failures | Evidence under artifacts/ |
| --- | ---: | ---: | --- |
| Core regression, including rejected retired Appearance type | 11,895 | 0 | build31-core-regression.log |
| Native synchronization / camera capture | 5,363 + 12 | 0 | build31-native-sync.log |
| Current nodes, gradient, rotation, Auxiliary and camera | 379 | 0 | build31-current-core.log |
| General AEGP lifetime, bounded scan and optional misses | 14 | 0 | build31-host-bootstrap.log |
| Emitter / Particle / Force native effects | 84 / 85 / 80 | 0 | build31-node-Emitter.log / build31-node-Particle.log / build31-node-Force.log |

Current panel, Particle/Emitter/Force round trips, topology-only commit/rollback,
and startup/retained-error JavaScript suites pass. Logs are
build31-current-panel.log, build31-node-connection-tests.log and
build31-panel-startup.log. Isolated deployment report/install/retired-module/
installed-hash/single-junction/exact rollback checks pass in
build31-isolated-deploy.log; the fixture never touches the real installation.
These checks do not establish actual AE generic-context availability, first-load
performance, reopen performance, or Particle-to-Force host interaction.

## Build 30 deployment - 2026-10-03

Source 755a733 is pushed. A read-only process check immediately before deployment
found neither AfterFX nor AfterFX_64 running. Deploy-TestBuild installed build30
through the existing single Plug-ins/Starfield -> newStardust/dist Junction.
All six installed hashes match candidates; all six retained build29 hashes match
the before-state. Core and selected runtime are byte-identical to build29/27.
Thirteen unchanged CEP source files have verified paired snapshots. The read-only
Restore-TestBuild report passes. No process or host-wide setting was changed.

| Installed file | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | `C363C9DA6A872EB1F6905CFB1C015BF2A9673C08D0B72D8ED8955104D305BFAF` |
| StarfieldEmitter.aex | `05472720F3062538DA37D243AEFC20944AEA595F6F6626D00E80879BF64E18C4` |
| StarfieldParticleNode.aex | `FA4D21E91C6A476249AECC53150753E72303F3EBF844CA25BD966006FF26F7D8` |
| StarfieldAppearance.aex | `6C6599B2CA20DA5A9E7386320C108218B369F5EE931F85D5346295A46C654ADE` |
| StarfieldForce.aex | `FEC758EAA84A546240091885A4B1168E09AA3A14AF64A9D06C62393A833AAFAD` |
| StarfieldCore.dll | `5637EA2266B32AEED6A191FD18ED33C0B50DA83F221DC3FB54DAE3ED60D18B05` |

Runtime: StarfieldCore-5637EA2266B32AEE.dll.
Before/after: artifacts/build30-deploy-before.json and build30-deploy-after.json.
Backup: artifacts/disabled/m306-build30-evaluate-restored-aliases-20261003/bundle.
Evidence: artifacts/build30-native-sync.log, build30-native-build.log,
build30-deploy.log and build30-rollback-check.log. All five AEXs and Core build
without compiler warnings with May 2023 SDK Release /MT; 5,157 scoped checks pass.

One-step undo to build29, with AE closed:

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Deploy-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'm306-build30-evaluate-restored-aliases-20261003' -Rollback
```

Build29 still requires Options for the owner-tested speed improvement. This
rollback preserves that known behavior, not automatic startup qualification.
After restoring build29, the retained build29-rollback-to27.ps1 chain documented
below remains available. Neither rollback has been executed.

Actual build30 AE startup performance remains unqualified. Requested check:
open original 10,000 PPS / Life 2 project WITHOUT Options and preview an uncached
frame; save/close/reopen and preview another uncached frame WITHOUT Options.
Only afterwards collect Options showing Build30, previous W/P/E/B, render
Static/Temporal/PF/N and elapsed wait. No-Options success and reopen confirmation
remain required before the owner-authorized main integration.

## M3-06 / build 30 - evaluate restored bindings before state certification

The owner rejects build29 startup performance: Options is still required. The
2026-10-03 screenshot reports B2 (two sequence capture attempts), Temporal 0/57,
PF58 / N2/2, followed by 57 valid proofs after Options. This establishes that
bootstrap ran but does not establish usable render proofs. It reports no last
GPU frame and preparation at time zero; it is not a preview latency measurement.

Source inspection identifies a missing step: Options' binding transaction reads
all active aliases after expressions before capture, whereas startup only read
PF states and source metadata. Lazy expression dependency discovery may change a
pre-evaluation state during first rendering. This is a hypothesis about AE, not
host confirmation. Build30 (32798 / 0x801E) evaluates all recorded active aliases
through callback-local AEGP UI streams BEFORE any proof state is captured. It
checks OneD type, expression enabled, finite value and unavailable sentinel.
Only successfully evaluated components are eligible for the existing source
metadata plus before/after all-time state certification and render revalidation.
No expression, source value/key, project stream or selection is written.

Scoped fake-host tests model a dependency generation changing on first expression
evaluation. A negative control demonstrates that pre-evaluation tokens become
invalid. Cold setup and save/reopen then retain proofs after first render reads;
read/type/nonfinite/disabled-expression failures publish no proof for that input
and release references. Denied PF checkout/checkin in sequence callbacks, absent
params, source keys/expressions and worker/render-only exclusions remain covered.
5,157 scoped checks pass (native sync 5,145; camera 12); evidence is
artifacts/build30-native-sync.log. Native build/deployment is recorded separately.

Diagnostics snapshot the previous automatic reader BEFORE Options refreshes it:
W is evaluated/active aliases, P published proofs, E optional warmup error; B
remains sequence attempts. Build30/proofs is the subsequent explicit Options
result. Process-global readings may belong to another effect callback.
Actual AE no-Options first preview and save/reopen are still mandatory gates.
The owner-authorized main integration waits for those results. Core ABI3, public
IDs, match names, graph codec, SFU1 sequence schema, CEP and PiPL flags are unchanged.
Owned scope: NativeNodeGraph, NativeTemporalCache, Diagnostics, PluginVersion,
scoped native sync fixture and these M3-06/ADR0026 records.

## Build 29 deployment - 2026-10-03

Source `e4111ab` is pushed. After the owner closed AE, the read-only process check
immediately before installation found no AfterFX/AfterFX_64. Deploy-TestBuild.ps1
installed build29 (32797/0x801D) through the existing single
Plug-ins/Starfield -> newStardust/dist Junction. All six installed hashes match
candidates and all six build28 backup hashes match the before-state. Core and its
selected runtime remain byte-identical to build27/26/25, retaining ABI3 evidence.
Thirteen unchanged CEP files have paired verified snapshots. The normal rollback
report passes. No process, registry, CEP manifest or host-wide setting was changed.

| Installed file | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | `40814E561F0305270A09FD33B44F55D25A1E7B3E990186342977C25BCE8C530E` |
| StarfieldEmitter.aex | `78E0D8DB76AB6412C8FC547E472C460C19A5DB508C2798AA803DA2FF5F4EF937` |
| StarfieldParticleNode.aex | `3E3D536F9A7A11E828826ADA9F51FA5795C010416A6AAA27ABFDDC9AF33A75CA` |
| StarfieldAppearance.aex | `910BF74394111EF62318671421E5D14EAF6CF4906B8E5E161CA4592970FAB584` |
| StarfieldForce.aex | `2C2B6084F7BC7C999AB5C8731049D8D46A7EC0EE7850264ADE38C0255EB76E03` |
| StarfieldCore.dll | `5637EA2266B32AEED6A191FD18ED33C0B50DA83F221DC3FB54DAE3ED60D18B05` |

Runtime: `StarfieldCore-5637EA2266B32AEE.dll`.
Before/after: artifacts/build29-deploy-before.json and build29-deploy-after.json.
Backup: artifacts/disabled/m306-build29-sequence-stream-read-20261003/bundle.
Evidence: artifacts/build29-deploy.log and build29-rollback-check.log.

The current before-state is build28, which the owner rejected for the sequence
checkout error. For recovery to the owner-tested build27 (with its Options startup
workaround), an ignored, local helper verifies both retained stages and defaults
to a read-only report. It performs the two existing Restore-TestBuild steps only
with -Restore. Newer candidates remain in their backups. The hash chain is verified
without executing rollback: artifacts/build29-stable-rollback-check.log.
With AE closed, one command restores build27:

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\artifacts\build29-rollback-to27.ps1' -Restore
```

3,974 scoped checks pass, including sequence fixtures that deny PF checkout/checkin;
all five AEXs and Core build with May 2023 SDK Release /MT without compiler warnings.
Owner testing is requested: open original project without Options, preview uncached
frame, save/close/reopen and preview without Options, then collect full Options
Static/Temporal, PF/N, B and elapsed wait. Removal of the direct forbidden call is
verified; actual AE startup availability/speed is not yet qualified. Main integration
remains pending this result under the owner's existing authorization.

## M3-06 / build 29 - remove forbidden sequence parameter callbacks

Owner rejects build28: AE2023 reports effect cannot use checkout/checkin callbacks
in SEQUENCE_RESETUP (25:83), both on project open and during use. The absent-array
fallback added in build28 was invalid. Its fake host allowed a callback AE forbids;
that coverage did not qualify the host selector. Automatic startup remains unproven.

Build29 (32797 / 0x801D) never reads sequence params[] and never calls PF parameter
checkout/checkin in SETUP/RESETUP. On the recorded main UI thread only, it obtains
the current effect's graph stream through AEGP_PFInterfaceSuite1/StreamSuite6,
checks ARB type, copies the graph while the returned value is alive, then reads
source metadata through the existing reader. Value, stream and effect references
are released in reverse order within the callback. Missing graph/type/suites is
an optional miss; it neither rejects project opening nor publishes a guessed proof.
All-time owned-alias states still bracket source metadata and are checked in render.
Worker/render-only resetup skips AEGP entirely. No render-thread AEGP, retained
source handle, idle hook or cross-effect generic message is added.

Qualification: 3,974 scoped checks pass (native sync 3,962; camera 12). The full
May 2023 SDK Release /MT candidate builds all five AEXs and Core without compiler
warnings. Evidence: artifacts/build29-native-sync.log and build29-native-build.log.

Schema SFU1 remains exactly four flat bytes; graph codec, node controls, public
IDs, Core ABI3, CEP and PiPL flags are unchanged. Full paired AEX deployment is
required. Focused tests now deny PF checkout/checkin in sequence callbacks, pass
an undersized params array, omit callback functions, inject graph read/type failures,
and check stream/ARB cleanup, recovery, keys and worker exclusions. Actual AE
open/run without 25:83, no-Options first preview and save/reopen performance are
mandatory gates before the owner-authorized main integration.

## Build 28 deployment - 2026-10-03

Source `2714b8e` is pushed. A read-only process check immediately before
Deploy-TestBuild.ps1 found no AfterFX/AfterFX_64. The existing single
Plug-ins/Starfield -> newStardust/dist Junction now holds build28 (32796/0x801C).
All six installed hashes match candidates and all six backup hashes match the
before-state. Selected Core is byte-identical to build27/26/25, preserving that
ABI3 qualification. Thirteen unchanged CEP sources have paired verified copies;
Restore-TestBuild.ps1 read-only verification passes. No process, registry, CEP
manifest, other extension or host-wide setting was changed.

| Installed file | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | `35D143F918CF1BC33600FA58AD199E7DDD58343EDCE4BA4A8D4910D43889766E` |
| StarfieldEmitter.aex | `B092B9BD37E5D8733D84D0E3BA42E3AA4CDB6BD1BA066D5ED1B5126E733FE242` |
| StarfieldParticleNode.aex | `00F47C9CA212661F219ED6FE47D2F98BE7CCFA5B12FD77DF6744E2ECD0ECE839` |
| StarfieldAppearance.aex | `86354CCF4E3B1A3E0C0B0B754F52CEB895BE0EB33291238549A5D0299881958F` |
| StarfieldForce.aex | `FC756C59AC2143B6F3F7AA723159FDE40A00FF5B64B5D74A9F188B015E0B439A` |
| StarfieldCore.dll | `5637EA2266B32AEED6A191FD18ED33C0B50DA83F221DC3FB54DAE3ED60D18B05` |

Selected runtime: `StarfieldCore-5637EA2266B32AEE.dll`.
Evidence: artifacts/build28-deploy-before.json, build28-deploy-after.json,
build28-deploy.log and build28-rollback-check.log. Backup build27:
artifacts/disabled/m306-build28-startup-proofs-20261003/bundle.

With AE closed, one-step rollback to build27:

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Restore-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'm306-build28-startup-proofs-20261003' -Restore
```

3,627 scoped checks and the warning-free full May 2023 SDK Release /MT build pass.
Owner AE feedback qualifies build27 at about 1-2 seconds after Options, but
reopening requires Options again. Build28 must be tested by opening the original
project and previewing an uncached frame before Options; then save/close/reopen
and preview before Options again. Only after that preview read Options B,
Static/Temporal, PF/N. First legacy null-data open and non-null saved reopen are
separate gates. The owner-authorized main merge remains pending this result.

## M3-06 / build 28 - initialize history proofs when opening projects

The owner confirms build27 cuts uncached-frame preparation from 6-8 seconds to
approximately 1-2 seconds, but only after clicking Options, including after saving
and reopening. This qualifies a visible improvement, not full frame-time parity.
The constant/key metadata registry is process-local; the DRAW refresh added in
build26 is not delivered for every unopened/unselected main effect.

Build28 (32796 / 0x801C) reads optional source metadata in main-thread
SEQUENCE_SETUP/RESETUP, before preview needs it. Worker/render-only callbacks
perform no AEGP source queries. Options and DRAW remain optional refresh routes;
source keys/expressions and all-time PF states are revalidated after every load.
No equality-of-values shortcut is used. Diagnostics B counts sequence refresh
attempts; Static/Temporal and PF/N indicate actual render-side use.

Qualification: 3,627 scoped checks pass (native sync 3,615; camera 12); all five
AEXs and Core build with May 2023 SDK Release /MT without compiler warnings.
Evidence: artifacts/build28-native-sync.log and build28-native-build.log.

Private main sequence-data schema SFU1 consists of exactly four byte-ordered ASCII
bytes, with no handles, references, PF states, graph values or cached particles.
Legacy null data is provisioned during setup/resetup and also during flatten/save;
unknown non-null schema/size is rejected without replacement. SETDOWN releases the
host allocation. Saved non-null data provides a RESETUP opportunity on reopening.
An old null-data project may lack that callback; first cold legacy open and first
save/reopen are separate AE acceptance gates. Build28's partial/absent-array checkout fallback was subsequently rejected by AE
(25:83); build29 replaces it with an AEGP graph stream read. Source metadata can
be unavailable while siblings restore; later DRAW can retry. Fake-host results
cannot establish AE callback order or availability.

Scope: EffectMain, NativeTemporalUI, Diagnostics, PluginVersion, scoped native
sync tests and these records; no Core/Render transport, source node controls,
public parameter IDs, match names, graph codec, CEP or PiPL flag changes. The
private sequence migration is recorded in ADR0026. Full paired AEX deployment
is necessary. The owner-authorized main merge awaits no-Options reopen validation.

## Build 27 deployment - 2026-10-03

Source `a625321` is pushed. Immediately before installation, a read-only process
check found neither AfterFX nor AfterFX_64. Deploy-TestBuild.ps1 replaced build26
through the existing single Plug-ins/Starfield -> newStardust/dist Junction.
All six installed hashes match candidates; all six backup hashes match the before
state. Core remains byte-identical to build25/26, retaining their ABI3 qualification.
Thirteen unchanged CEP files have a paired verified snapshot. No process, registry,
CEP manifest or host-wide setting was changed. Host root still contains one Starfield
entry. Restore-TestBuild.ps1 read-only verification succeeds.

| Installed file | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | `D3CD28BECF8F5B0EF3BE2661849E350D7B990B6DFE3F25BF975AD1E3500816B6` |
| StarfieldEmitter.aex | `834F649C26B918FB8F86946B2EF6305982BC2AFA1BEF5895E9C883DBF230B4DC` |
| StarfieldParticleNode.aex | `7BB26742AC6BBE446C04BE52A5F10FE776C5E476CD303C5907DBC79390185894` |
| StarfieldAppearance.aex | `BB77C076CCCF7FB6D09A0836EE56892839FA2DD287A2D30A9E06428DA584982E` |
| StarfieldForce.aex | `2E5C8ECB93965CA0322F483205330AF6317E801E5240FF25A9A4A38914969B84` |
| StarfieldCore.dll | `5637EA2266B32AEED6A191FD18ED33C0B50DA83F221DC3FB54DAE3ED60D18B05` |

Selected Core: `StarfieldCore-5637EA2266B32AEE.dll`.
Before/after: artifacts/build27-deploy-before.json and build27-deploy-after.json.
Backup: artifacts/disabled/m306-build27-acyclic-bindings-20261003/bundle.
With AE closed, one-step rollback to build26:

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Restore-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'm306-build27-acyclic-bindings-20261003' -Restore
```

Open the existing test project and click main Options once: `Bindings v27` confirms
owned expression refresh. Then evaluate uncached PPS10,000/Life2 frames and provide
elapsed seconds plus the actual Options readout. The new lookup defect is proved in
source/generated-expression regressions; improvement of the owner's 6-8 second AE
wait is not yet measured. 3,383 scoped C++ checks and JS regressions pass; all five
AEXs build with May 2023 SDK Release /MT. Main integration awaits this qualification.

## M3-06 / build 27 - native dependency lookup and constant-node reuse

Owner AE2023 feedback for build26 confirms that delay no longer grows with time,
but uncached frames still take a subjective 6-8 seconds even with CEP closed.
Options reports roughly 39 ms preparation and 11 ms CUDA rendering in one sample.
Those selector timings exclude AE work before/after entry. The earlier screenshot
with 56/57 constants used Life queries; the fully certified Static path used zero
birth-node queries. Therefore the proposed (node, birth-time) cache-miss analysis
cannot by itself explain the observed Static-frame wait.

Source inspection finds a distinct dependency defect: UUID lookup scanned every
effect and read numeric properties before establishing that they were UUID fields.
Emitter UUID indices 99..106 and Particle indices 143..150 overlap the renderer's
98..609 expression inputs. This could introduce self/cross-renderer expression
dependencies. Build27 (32795 / 0x801B) skips the expression's own effect by
propertyGroup(1).propertyIndex and gates other effects by the fixed hidden property
name before reading all eight UUID words. Effect display names/order remain free.
A generated-expression regression proves the old lookup reads renderer values and
the new lookup performs zero such reads. Actual AE latency impact remains a hypothesis.

Constant nodes are converted once per frame and reused across every birth time,
including mixed/Force graphs; dynamic nodes retain exact-time sampling. Equality
of sampled values never certifies a node. Static simple graphs reuse the already
sampled current-frame graph without a second batch of alias checkouts. Dependency
changes discard the optimization. No render-thread AEGP access is introduced.

Options upgrades the generated owned bindings in existing development projects;
click it once after deployment. Source keys/expressions and graph values are preserved.
Diagnostics include node requests/actual samples (N), separate main GPU setup and
UI refresh maxima, and the render's own prepared time (p). A time/duration-mismatched
SmartFX pair is rejected before borrowed pixel access, with equivalent rational times
accepted. Process-global last-call diagnostics do not measure full AE preview time.

Qualification: 3,383 scoped C++ checks pass (native sync 2,938; camera 12;
actual CUDA/OpenCL drivers 433), plus generated-expression/keyframe JS checks.
May 2023 SDK Release /MT builds all five AEXs and Core without compiler warnings.
Evidence: artifacts/build27-native-sync.log, build27-expressions.log,
build27-gpu.log and build27-native-build.log. These are not AE frame-time results.

Main/native IDs, parameter counts, schemas, PiPL flags, CEP native-gpu-24 and Core
C ABI3 stay unchanged. Full paired AEX deployment is required. AE 6-8 second latency,
key/expression edits, copy/reorder/undo/reopen remain owner gates; M3-06 stays open.
The requested main integration is pending this candidate's qualification.

## Build 26 deployment — 2026-10-03

Source `e1fb3d6` is pushed. Read-only process checks immediately before installation
found no AfterFX/AfterFX_64. Deploy-TestBuild.ps1 upgraded build25 to build26 through
exactly the existing Plug-ins/Starfield -> newStardust/dist Junction. No process,
registry, CEP manifest or host-wide setting was changed. The host root still has
exactly one Starfield entry. All six installed hashes match the candidates; all
six retained hashes match the before-state. Core and its selected runtime are
byte-identical to build25, so its five-function ABI3 evidence remains applicable.

| Installed file | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | `843F5CC4A6115D11A0E0D2EB51AF65BF1752842561730D80FFE62816EE2AC4EA` |
| StarfieldEmitter.aex | `377B63010CF2E11C0F352CC1D6CF2E7D24EB7D78781980413C38444B36B8C980` |
| StarfieldParticleNode.aex | `AE08BFC54D2EDA25A1B106042A5094129B5F3FFD48DB23584A77188B8BFBC85F` |
| StarfieldAppearance.aex | `7156C7E8CB33C2B48CCEB38420B631A4110E2F701F6BE484C6E4714273A10C8F` |
| StarfieldForce.aex | `4B7B2D236493B9FFBAF68221ABAA07400CF8F8E68E1086BBEF769CD2761F1458` |
| StarfieldCore.dll | `5637EA2266B32AEED6A191FD18ED33C0B50DA83F221DC3FB54DAE3ED60D18B05` |

Selected Core: `StarfieldCore-5637EA2266B32AEE.dll`. Evidence:
artifacts/build26-deploy-before.json and artifacts/build26-deploy-after.json.
Backup: artifacts/disabled/m306-build26-render-proofs-20261003/bundle.
Thirteen unchanged CEP files and a paired restore manifest are retained in panel/;
Restore-TestBuild.ps1's read-only verification passes. With AE closed, one-step
rollback to build25:

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Restore-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'm306-build26-render-proofs-20261003' -Restore
```

3,253 scoped checks pass; the five AEXs build with May 2023 SDK Release /MT.
Parameter layouts, CEP token and Core ABI stay compatible with build25. Owner
AE performance remains open; request actual Options readouts after t=2/100 frames
at PPS10,000 / Life2. The new diagnostics distinguish render preparation from
Smart Render wall time and real renderer proof use from optional UI metadata.

## M3-06 / build 26 — UI/render proof matching and actual preparation costs

Owner reports build25 remains slow or becomes slower at later frames. Its
Options `History` count was measured in the UI and did not prove render-side
use. The strict equality of UI/render `effect_ref` was an invalid instance-key
assumption: the SDK defines it as an opaque callback reference. Whether this
was the owner's actual failure remains an AE qualification question.

Build26 (32794 / 0x801A) matches proofs by current graph node UUID, owned alias
index, and AE's all-time dependency-state comparison. A different callback
reference alone no longer rejects a proof or prefix. Copied layers with unequal
states remain isolated. Prefix versions are bounded and matched by host state;
UI analytic-profile publication also invalidates sampled prefixes created by
render copies. Comparison errors and unavailable proofs retain exact sampling.

Main CUSTOM_UI is implemented through a zero-sized COMP registration and EVENT
DRAW handler; PiPL/runtime flags agree at 0x02008466. On AE's recorded main thread,
DRAW refreshes numeric metadata after dependency changes. Equal all-time non-layer
states skip source reads. No overlay, ECW area, project write, undo record or
rerender request is created. Worker/non-DRAW/reentrant calls are ignored before
SDK access. This automatic path depends on the main effect receiving composition
DRAW events (selection and visible layer controls); Options remains an explicit
refresh when no DRAW callback arrives. No render/sequence-worker AEGP query is added.

Options now snapshots real last pre-render costs before UI refresh: total,
controls/history/scene milliseconds, Static/Temporal path, constant/total inputs,
actual history PF checkouts, PPS and Life queries. Smart Render wall time includes
GPU upload, dispatch, synchronization, resource cleanup and host world checkouts.
These are coherent process-global
last-call diagnostics, not an instance-specific or full preview-time measurement.
UI proofs are labelled separately. Main/native parameter layouts and IDs, CEP
native-gpu-24, and Core C ABI3 remain unchanged; full paired AEX build is required.

Focused checks cover distinct UI/render references with equal dependencies,
copy isolation, key/expression invalidation, undo, prefix publication, static
history bypass, UI callback/throttling/thread safety and real preparation timing.
3,253 scoped checks pass: native sync 2,671, camera 12, emission cache 157,
CUDA/OpenCL drivers 413. May 2023 SDK Release /MT builds all five AEXs and Core.
Actual AE t=2/10/100 performance, cross-project state equality, UI event delivery,
key/expression edits and reopen remain owner gates. M3-06 stays open.

## Build 25 deployment — 2026-10-03

Source commit `5c53b59` is pushed. The May 2023 SDK Release /MT full build succeeds;
3,480 scoped C++ checks pass. Candidate Core ABI3 exposes all five functions and
rejects ABI2. Read-only checks immediately before install found no AfterFX/AfterFX_64.
Installed through Deploy-TestBuild.ps1 and the existing single
Plug-ins/Starfield -> newStardust/dist Junction. Exactly one Starfield host entry
remains. No AE process launch/stop or host-wide configuration change.

| Installed file | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | `BFDBF245CEC086337D37E9DD32818BE28D6881B40D176352B33D69525CC8CE72` |
| StarfieldEmitter.aex | `4CCF9E7D2F9126F7DD5BA6B88ECDE2F0A868FCBFC34636021F9CEA117AB15800` |
| StarfieldParticleNode.aex | `00471B42C29D8FC669781BB31CC2871EF12E95ECAABCF1FFE28A771DF3189268` |
| StarfieldAppearance.aex | `34E1EB7E4AA8CA39A9E9577D31934FC9B9456840280C6A9C6EE16C3021087FE4` |
| StarfieldForce.aex | `8534C8A1EA839FF6E23201B246D37139FED270FB19DDEF098D824B641A3F3B83` |
| StarfieldCore.dll | `5637EA2266B32AEED6A191FD18ED33C0B50DA83F221DC3FB54DAE3ED60D18B05` |

Selected runtime: `StarfieldCore-5637EA2266B32AEE.dll`; selected/pinned Core hashes agree.
All six installed hashes equal the candidates and all six retained backup hashes
equal the build24 before-state. Records: `artifacts/build25-deploy-before.json` and
`artifacts/build25-deploy-after.json`. Backup:
`artifacts/disabled/m306-build25-temporal-cache-20261003/bundle`. Thirteen unchanged
CEP files are retained byte-for-byte under panel/, checked against baseline 8857924
with normalized line endings, plus a verified paired restore manifest. Restore's
read-only report passes. With AE closed, one-step rollback to build24:

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Restore-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'm306-build25-temporal-cache-20261003' -Restore
```

Existing build24 effects and CEP native-gpu-24 remain compatible. Open the main
Options once on an existing project to capture optimization metadata; the readout
shows `History: N certified inputs` and forces rerender. External native edits may
require another Options recapture for full optimization. Owner AE timings remain
open; core benchmarks are not AE preview timings. No AE render output was created.

## M3-06 / build 25 — bounded native history preparation

Owner confirms build24 runs on CUDA device0, but 10,000 PPS / Life 2, no keys
or extra nodes, becomes slower at later frames. The reverse birth loop continues
through expired births because the 1,000,000 Output cap exceeds the live population.
The previous global Life bound is 10,000 seconds; it is not the authored Life.

Build25 (32793 / 0x8019) certifies native source controls on supervised main UI
commits or Options: no source expression/keys, matching all-time owned-alias PF_State
before/after metadata capture. Render validates with PF_ParamUtilsSuite3; no render
AEGP read or retained source handles. Certified fields are hoisted into a frame-local plan.
Simple static Default Emitter/Particle/Output graphs with Life Random 0 use the
closed-form alive-slot path, skipping historical checkouts and snapshot encoding.
Other temporal graphs use certified Life bounds and optional emission prefix leases.
Linear/hold Life bounds use the complete key envelope; Life Random only shortens Life.
Bezier/expression Life retains the safe global bound. PPS constants/linear/hold keys
use analytic integration; nonlinear sources reuse fixed-lattice prefixes under equal
all-time dependency stamps and 30/60/120 Hz. Changes/errors/contention/eviction fall
back safely. Complete cached/uncached particle snapshots compare byte-identically.

3,480 focused C++ checks pass: native sync 2,101, camera 12, emission timelines 397,
emission cache 131, current nodes 379, CUDA/OpenCL drivers 409, main controls 51.
At cap1,000,000 / PPS10,000 / Life2, t=100 and t=10,000 both perform 20,000 Life
queries; core temporal evaluation measured 25.442/27.272 ms. The certified static
core path measured 2.421/2.033 ms. These exclude AE checkout, GPU dispatch and host
preview overhead and are not an AE frame-time claim.

Main/native layouts, public IDs, CEP native-gpu-24 token and C ABI3 stay unchanged.
A full paired build is required because the adapter and statically linked Core contract
both change. Existing build24 effects may remain. For an existing project, click main
Options once to capture metadata; it shows `History: N certified inputs` and requests
rerender. External Effect Controls/key/expression edits may require Options recapture
for full optimization; invalid proofs preserve correct historical rendering meanwhile.
Automatic recapture after every external edit remains an open workflow gate.

Owner AE gates: real end-to-end timings at t=2/10/100, key/expression invalidation,
reverse seek, undo/reopen and surrounding effects. No AE process was operated by the
agent. Do not close M3-06 from compilation or standalone timings.

## Build 24 deployment — 2026-10-03

Source commit `38c4b92` is pushed to the owner repository. Read-only process checks
before install found no AfterFX/AfterFX_64. Installed via Deploy-TestBuild.ps1 and
the existing single Plug-ins/Starfield -> newStardust/dist Junction; host root
still contains exactly one Starfield entry. No AE process launch/stop or host-wide
configuration change. CEP uses the existing Junction to this checkout.

| Installed file | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | `3AFCA47F6FA01C2BBE42814905AE102AF28F33ED14FE5D47CE38FADAE9334BB6` |
| StarfieldEmitter.aex | `7AA109EE683D58D8C81C993D85BA55FAA44A8788619F6D7CBE10C195149C93A5` |
| StarfieldParticleNode.aex | `819CDCAA94FA2E7C2DAEA5C36177E6575F03377BC5EF6B1599F9DCEA27DFFBB0` |
| StarfieldAppearance.aex | `1E322FFB4E2B1EBBE14603B8D7F2F190420A1434EEC85EE76E9667B7994C2260` |
| StarfieldForce.aex | `F537CECD5C29D705C1AD9306F3ADD20007A426FD11444A7C25A06B013A178FD0` |
| StarfieldCore.dll | `132AC20B775611D48F39C6DCCD55F0EA6932DBA1EAE7DD462698A0CDD594E001` |

Selected runtime: `StarfieldCore-132AC20B775611D4.dll`; selected and pinned Core hashes agree.
All six retained old hashes agree with build23 before-state. Before/after records:
`artifacts/build24-deploy-before.json` and `artifacts/build24-deploy-after.json`.
Backup: `artifacts/disabled/m306-build24-native-gpu-20261003/bundle`; 13 previous CEP
sources from commit 24de13f are retained under panel/, with verified paired manifest.
The read-only paired restore report passes. Close AE for one-step undo:

```powershell
powershell -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'm306-build24-native-gpu-20261003' -Restore
```

Fresh main effect required (616 registered params, manifest 24). Reopen CEP for
`native-gpu-24`. Acceleration defaults to GPU. Options reports the last executed
main frame path; no AE GPU selector/render qualification was performed by the agent.
No AE render output was created. Temporal metadata/cache gates remain open.


## M3-06 / build 24 — AE native CUDA/OpenCL candidate

Build 24 (32792 / 0x8018) implements host-proposed CUDA/OpenCL device setup/setdown,
per-frame SmartFX eligibility and SMART_RENDER_GPU into AE GPU_BGRA128 output.
Main Acceleration is GPU by default, with a CPU selection at index 614/disk 1611.
The four control-node effects also implement GPU pass-through to avoid our own
CPU image copy between nodes. Main/node PiPL and runtime capability flags agree.
Unimplemented frameworks or initialization failures reject GPU support cleanly.
MFR, Compute Cache and DirectX support remain disabled; no private graphics context.

Core C ABI 3 adds an immutable, typed, ROI-relative sprite/tile scene and paired
release callback. CPU simulation/history/projection prepare the scene; GPU kernels
perform deterministic ordered rasterization. CPU/GPU share camera/shape geometry.
There is no full-frame CPU staging image or GPU image readback in the product GPU
path. Numeric scene uploads still cost time. GPU scene budgets reject before GPU
output checkout and negotiate CPU without truncating particles.

May 2023 SDK full Release /MT build succeeds. 2,432 scoped C++ checks and focused
JS round trips pass. 409 actual-driver checks exercise CUDA and OpenCL (including
an out-of-order OpenCL queue), main SmartFX and the real node-effect GPU selectors,
Circle/Rectangle/Cloud, camera, HDR/alpha, padded rows/ROI, cancellation, partial
allocation failure, ABI rejection and cleanup. Small-scene maximum float component
difference versus CPU is 0.00000175834; this is scoped evidence, not a global error bound.
A standalone 512x512, 20,001-sprite benchmark takes roughly 3.4–3.7 ms preparation
plus 1.2–1.6 ms allocation/upload/kernel/synchronization, versus 67–69 ms CPU rendering.
AE parameter/history capture, device startup and surrounding host effects are excluded.
Actual AE GPU dispatch, preview scales/ROI, render queue and end-to-end timings remain
owner qualification gates. Do not claim AE GPU support from these tests alone.

Fresh main effects are required (main manifest/schema 24, registered count 616
including implicit input). Existing IDs 1..612 and native node layouts are unchanged.
Reopen CEP for native-gpu-24. Full paired AEX/Core deployment is required for ABI 3;
no old-project migration. AE-certified PPS metadata and cross-frame prefix-cache
invalidation/reuse remain open; build 24 still samples native PPS.

### GPU build input and artifact layout

`tools/Prepare-GpuBuildInputs.ps1` reports by default. Its explicit `-Download`
action downloads NVIDIA's official PyPI NVRTC 12.4.127 Windows wheel into
`artifacts/gpu-build/nvrtc-12.4.127/`, verifies SHA-256
`A961B2F1D5F17B14867C619CEB99EF6FCEC12E46612711BCEC78EB05068A60EC`, and retains
its bundled license. No installer, pip environment, registry, PATH, driver or
system runtime is changed. BuildWindows invokes `tools/Build-GpuKernels.py`, which
verifies both DLL hashes and generates our embedded PTX/OpenCL header under
`artifacts/gpu-build/generated/`. NVRTC/compiler DLLs are never deployed.
CUDA uses the system Driver API and AE's context/stream; OpenCL uses AE's actual
context/device/queue. PTX build target compute_52 uses CUDA 12.4 PTX; a driver that
cannot load it rejects setup and AE can use CPU. No AE-bundled CUDA runtime dependency.

Scoped tests: `tests/RunCoreTests.ps1 -Gpu`, `-CurrentNodes`, `-NativeSync`,
`-EmissionTimeline`, `-RendererControls`, and `-NodeEffects -NodeKind <kind>`;
`node tests/gpu_panel_tests.js` plus existing current-node/reference-gateway,
startup, native-expression and gradient checks. Standalone GPU timings are recorded
in ignored `artifacts/gpu-tests/timings.csv`; these are not AE frame benchmarks.


## M3-06 / build 23 — Particle controls and temporal preparation

AE 2023 / May 2023 SDK candidate, packed version 32791 (0x8017). Emitting now
means Default / Once / Sequenced / Randomized; Auxiliary Source is separate.
Unordered Point/Box/Sphere/Disc sources use default ordering for Sequenced/Randomized;
Once births an initial PPS-sized batch. Direction defaults to Uniform.
Particle schema 4 adds Life Random, Circle/Rectangle/Cloud, Size Y, Feather,
Up Axis, Orient To, Angle X/Y/Z and spin Speed X/Y/Z with native AE Angle controls,
Angle/Speed Random and Limit to 2D. Existing four color modes/gradient are retained;
the initial gradient colors are visible in Effect Controls. Cloud is an independent,
fixed five-circle procedural cluster, not the complete reference Cloud settings.
Shapes and rotation preserve transparent output in 8/16/32-bpc CPU rendering.

Main manifest 23 appends Simulation Settings 610..612: Time Sampling at index 611 /
disk 1601 offers 30/60/120 Hz, default 30. Emitter schema 6/base 31 and Particle
schema 4/base 75 require fresh development effects. Output schema 4 carries Hz.
Transient 0x8004 version 2 preserves the added sprite fields in paired AEX/Core;
C ABI 2 and Render.hpp remain unchanged. Reopen CEP for ready marker
native-particle-controls-23. No old-project compatibility path is added.

Historical capture now decodes bindings once per frame and samples one node
without copying the entire graph; Force conversions share a bounded evaluation
cache. Core EmissionTimeline implements metadata-certified rate*t, exact linear/hold
integration and fixed-lattice prefix reuse/inversion. 367 focused checks prove those
Core paths. **AE rate metadata capture, reliable invalidation, and prefix reuse across
AE frames are still open; the current adapter still samples PPS.** No constant curve
is guessed from equal samples, and no render-thread AEGP read is introduced.

GPU ADR 0026 follows host CUDA/OpenCL negotiation, GPU_DEVICE_SETUP/SETDOWN,
per-frame GPU_RENDER_POSSIBLE and SMART_RENDER_GPU into AE GPU_BGRA128 worlds.
The May 2023 SDK has no DirectX contract. GPU handlers/kernels, main Acceleration
selection, actual AE GPU qualification and end-to-end timings remain open. Build 23
is CPU-only and retains disabled GPU/MFR/Compute Cache capability flags.

Scoped qualification: 2,017 C++ checks across current-node behavior/pixels,
native sync/camera, emission timelines, all four node registrations and main
renderer controls; focused JS checks cover production gateway round trips,
generated UUID expressions/keyframe preservation, gradient encoding, panel
startup/error retention and node interactions. All pass. The paired five AEXs
and Core build with the May 2023 SDK Release /MT. No AE session is operated;
owner host rendering/keyframes/undo/reopen remain open.

### Build 23 deployment — 2026-10-03

Source `5950362` is pushed to `codex/m3-01b-emitter-dimensions` in the owner repo.
A read-only check confirmed neither AfterFX nor AfterFX_64 running. Deployment
used the standing authorization and Deploy-TestBuild.ps1, through the existing
single Plug-ins/Starfield Junction. No AE process was started/stopped or operated.
The existing CEP extension Junction still points to this checkout's cep_panel;
no host/profile/registry/cache settings or extension Junction were changed.

Verified all six installed/candidate hashes, six previous bundle backup hashes,
the old selector backup and the selected/pinned Core hash. Before/after records:
artifacts/build23-deploy-before.json and artifacts/build23-deploy-after.json.
Selected Core: `StarfieldCore-32BC9BF7387D312A.dll`.

| Installed file | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | `D78257C2E102C49830E04810143AD8F49D5E44F9BEF56E788199D3F21815A550` |
| StarfieldEmitter.aex | `466FEB12A813FF81E4D72F18702D43401792A78073A6557E72A6F9777453F183` |
| StarfieldParticleNode.aex | `B622FCAF4D472EEA46C72BC5A47A57B6B7C72D453EDA6FF788347FD605BBFFE1` |
| StarfieldAppearance.aex | `F7F2CC49443160127E2B10183C34692C4557C5B940D9C459B707CAAE260DB7BA` |
| StarfieldForce.aex | `30453C25AAC0E9D8EF85FD29512AE54AF76600B8916B296BBFB1EB3746331E26` |
| StarfieldCore.dll | `32BC9BF7387D312A08D7A3BCF7CB0740D4D85B3AA6A98C409E56F46D7D5D9389` |

Backup: artifacts/disabled/m306-build23-particle-controls-20261003/bundle.
Thirteen baseline CEP files from 2d4dff6 are saved in its panel/ directory, with
old/installed hashes in panel-snapshot.json. Restore-TestBuild.ps1's report and
isolated rollback regression verify report-only behavior, refusal to overwrite
subsequent source edits, retained candidate sources and restored bundle hashes.

One-step paired rollback, with AE closed, from this checkout:

```powershell
powershell -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'm306-build23-particle-controls-20261003' -Restore
```

This restores the prior AEX/Core and CEP source files and retains the candidate.
The restored CEP files appear as Git working-tree changes. Do not use a
binary-only rollback for this paired native-layout/CEP update. Fresh development
main/Emitter/Particle effects and a reopened CEP panel are required for build 23;
owner AE behavior/animation acceptance remains open. GPU implementation and AE
rate metadata/invalidation are still open M3-06 work.
## M3-05 / build 22 — temporal controls, Particle color and native Angle

Owner confirms build 21 Origin birth behavior. Build 22 integrates historical
PPS, samples all existing birth controls at actual birth times, and integrates
Force over lived time for ordinary/Auxiliary systems. Particle Color uses four
modes and a saved 2–8-stop gradient. Emitter rotations use native AE Angle controls
with turns/degrees, a dial and keyframes. Fresh Emitter/Particle effects required.
See [parameter time audit](temporal-parameter-audit.md) and ADR 0025 for the full
control matrix, precision, bounds and remaining reference/AE gates.

1,420 scoped C++ checks and focused generated-expression/startup/gradient JS
checks pass; all five May 2023 SDK Release /MT AEXs and paired Core DLL build.
No AE session operated. Owner AE 2023.5.0 Build 52 qualification remains open.
Build 22 is 32790/0x8016; native identity 3/layout metadata 7; main IDs/manifest,
Render.hpp and C ABI 2 unchanged. The CEP ready marker remains animation-19.


### Build 22 deployment — 2026-10-03

Source commit `5d4acdb7bc68643927fd8519203c79796b0e5241` is pushed to the current branch. A read-only check found
neither AfterFX nor AfterFX_64 running. The standing owner authorization was used
through Deploy-TestBuild.ps1; no process was started/stopped and no registry/cache
preferences were changed. The sole Starfield Junction still targets newStardust/dist.
Six installed/candidate hashes, selected/pinned Core and all six prior bundle
backup hashes plus the old selector were verified. Before/after records:
artifacts/build22-deploy-before.json and artifacts/build22-deploy-after.json.
Selected Core: `StarfieldCore-A99ACA9486FDFA56.dll`.

| Installed file | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | `FA9116D4C41F41C5E4C963D9B71C0DC8CD0BB23A9CC684C243E27B1C8F4CA96A` |
| StarfieldEmitter.aex | `F89D9F6610976835DC14D9804A38F94D0BC3FAC85396A7B98A8E541E6C1C816D` |
| StarfieldParticleNode.aex | `54B021B771C435B5848E44EC86A15D705B0435E125D23485A5047BB93ACA9853` |
| StarfieldAppearance.aex | `4B99E6C4215F92AB8FBEBA8E857755F9DC581E3129DE874CAE51C64A7CF8D8D5` |
| StarfieldForce.aex | `A924F1D8E235017B1B589E1FC581C32BCB93141EE76D9454AB5CDDF732C112EF` |
| StarfieldCore.dll | `A99ACA9486FDFA566D286B22A1831FF234C7FBCA4FE2E33CFDFD252515DDCDA9` |

One-step rollback to build 21 (with AE closed):

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Deploy-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'm305-build22-temporal-controls-color-20261003' -Rollback
```

AE has not loaded this build under agent operation. Owner host qualification,
cache invalidation, gradient undo/reopen and performance remain open.

## Build 21 M3-03 — emitter Origin at birth

Owner confirms build 20 renders and Origin keys animate, but survivors follow the
current emitter position. Birth-position semantics are implemented separately from
current-frame parameter sampling (ADR 0024). Pre-render queries owned Origin XY/Z
dependencies at selected particles' birth times, memoized by emitter UUID/time.
Its transient optional record 0x8003/version 1 passes immutable positions to the
DLL. Primary particles retain their birth origin plus normal velocity/force motion;
Auxiliary children use historical parent positions and their own birth offsets.
No project graph writes, process-global state, AE handles in Core, public ID,
Render.hpp or C ABI 2 changes. Paired PiPL/runtime wide-time flags let AE track
historical dependencies; no cross-frame history cache is kept.

Native sync 792, camera 12, portable current-node 78 and renderer controls 39
checks pass (921 total). Nonlinear/subframe origins, frozen codec transport, old
particles staying at birth positions, Auxiliary parents, forward/reverse pixels,
unchanged project graph, failed checkout and quiet cancellation are covered.
All five May 2023 SDK Release /MT AEXs and the paired Core DLL build without
compiler warnings/errors. Logs: artifacts/build21-*. No AE session operated.
Owner AE interpolation, prior-key cache invalidation and reopen remain open gates.

History is bounded to 1,000,000 unique origins (48 bytes each); host sampling uses
up to one-million ticks/s, reduced to fit its 32-bit numerator. Other animated
birth controls/emission clocks and integrated Force history remain separate work.
The main AEX's birth planner shares GraphEvaluation/ParticleSimulation/Random
source with Core; these inputs are now part of the adapter fingerprint, requiring
paired builds when planning changes. CEP keeps its unchanged animation-19 token.

### Build 21 deployment completed

Source 369f0ddca599441fd49bca9f193089f28e518cb4 pushed before deployment.
Immediate read-only process check confirmed no AfterFX/AfterFX_64. Installed
through the existing single Plug-ins/Starfield -> dist Junction under standing
permission. All six candidate/installed and prior build-20 backup hashes verified.
Selected/pinned Core both match the new generation; no AE process started/stopped.
Packed version 32789 / 0x8015; Core ABI 2 and CEP animation-19 token unchanged.
Records: artifacts/build21-deploy-before.json / build21-deploy-after.json.
Backup: artifacts/disabled/m303-build21-origin-birth-history-20261002.
Owner AE birth-position/key interpolation/cache invalidation/reopen gates remain open.

| Installed file | SHA-256 |
|---|---|
| StarfieldParticle.aex | F18E70874E72E2BF7C7E8E75AF673F51CE371D74CAC715010AFE8022D04915FE |
| StarfieldEmitter.aex | 6FEBA4002EA1665461289F1CB4E3E216098E01A48D985AE838DDEB6724221C91 |
| StarfieldParticleNode.aex | DBB68AA4E586B8D06DD80CDADCA4780B4546E3E258AD336E517C0CAFCBCE7DA2 |
| StarfieldAppearance.aex | DDB0C0B4E492A527943F452B141A2A1AE23C0F4C817AA7BA6B34054C777538B3 |
| StarfieldForce.aex | 329B263357AD7043616700371294FA8A25876A9D04BFAEB2B83D4EDB95BA998C |
| StarfieldCore.dll | 5D4ECE4496D9D1F0C14938B8902A71FDC368429DB1A07D93429D58D4EC094566 |

One-step rollback (close AE first):

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Deploy-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'm303-build21-origin-birth-history-20261002' -Rollback
```

## Build 20 P-02J follow-up — 2026-10-02

Owner rejects build 19 on opening: animation binding stream -1 unavailable (516).
The sampler rejected num_params below 610 before checking out any binding. That
field counts delivered parameters, not registered streams; SmartFX delivers no
params[] array. The previous fixtures always supplied the registered count, hiding
this path. Reproduction with num_params=0 failed the build-19 native suite (30
failures); removing that gate restores the same samples and visible frame pixels.
The owner screenshot does not expose the actual count; attribution of that host
error to this gate remains an inference until owner AE playback qualification.

Sampling now relies on registered stream IDs and checkout/checkin callback errors.
The main render checkout also reads Time Remapping/Preview controls without a
delivered-array count gate. Failures identify record/callback/checkout/value/
checkin/conversion phase, stream index and delivered count. Invalid records,
missing callbacks and invalid expression values still reject; no zero fallback.
No parameter ID/layout/Core ABI, expression format or CEP changes. The paired
animation-19 gateway token intentionally stays unchanged.

Native sync 761, camera 12 and renderer controls 39 checks pass (812 total).
The full main graph checkout and CPU pixel test use num_params=0 at forward,
intermediate and reverse times. Counts 1 and 610 also succeed; actual checkout
errors, missing callbacks and malformed records retain precise rejection phases.
Time Remapping/Preview values and failure propagation are exercised with count 0.
May 2023 SDK x64 Release /MT builds all five AEXs with no compiler warnings/errors.
Generated-expression and actual JSX startup suites pass; CEP sources are unchanged.
Logs: artifacts/build20-*. AE interpolation, CEP-closed playback and reopen remain
owner qualification gates. No AE session operated by the agent.

### Build 20 deployment completed

Source eeb3e66f4798966e8a5b2195edb2917cb48ef857 pushed before deployment.
Immediate read-only process check confirmed no AfterFX/AfterFX_64. Standing
permission used the existing single Plug-ins/Starfield -> dist Junction. All six
installed/candidate hashes, prior build-19 backup hashes and selector verified.
Core pinned/selected parity is unchanged. No AE process started/stopped.
Records: artifacts/build20-deploy-before.json / build20-deploy-after.json.
Backup: artifacts/disabled/p02j-build20-smartfx-count-fix-20261002.
Packed version 32788 / 0x8014; owner AE opening/playback gate remains open.

| Installed file | SHA-256 |
|---|---|
| StarfieldParticle.aex | C75170A9B674AFAD53E2661678CED110BD8087BD9A6A65640F056DA5B430E352 |
| StarfieldEmitter.aex | 75981C8755C8B055980F0937D69400C3AD8B0E66850C3665685223D328B0AAA9 |
| StarfieldParticleNode.aex | 93CB305299757F78FBB3312EE8D6560241586504738B381D249DEF45EB0B8EC8 |
| StarfieldAppearance.aex | D5F53D818D10369A4F56CD4ED1C71C9BB1BC21F862E737D2E541BE3CAF912598 |
| StarfieldForce.aex | 8DCAA69EE4B7975B3AE3146AB1034F1DA7810C4C4F2DBD48C675DB9FEF7E34B4 |
| StarfieldCore.dll | 0D3C8D672DE171D70DF699C3B8E14A9133E91AB43F74F14B8617F7517E3DA5E7 |

One-step rollback (close AE first):

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Deploy-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02j-build20-smartfx-count-fix-20261002' -Rollback
```

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

### Build 19 deployment completed

Source dd8daa0cff4659a5a1a2232ede5dc4e22efe8eae pushed before deployment.
Immediate read-only process check confirmed no AfterFX/AfterFX_64. Installed using
standing permission through the existing single Plug-ins/Starfield -> dist Junction.
No AE process started/stopped. All six installed/candidate hashes and prior bundle
backup hashes/selector verified; pinned/selected Core unchanged and equal.
Records: artifacts/build19-deploy-before.json / build19-deploy-after.json.
Backup: artifacts/disabled/p02j-build19-animation-binding-fix-20261002.
Recreate effects on a fresh test layer for owner qualification: saved development
projects can retain build-18 generated expressions until a new UI graph commit.

| Installed file | SHA-256 |
|---|---|
| StarfieldParticle.aex | E4AAD9EB1DC510C933A7828A47B74A23C113B0D79054DB6292C631C0E119D973 |
| StarfieldEmitter.aex | A4F82D131731EB8EFCCE7F297F2695C48A37EB764BC90011722265DAEEC02A4A |
| StarfieldParticleNode.aex | 36AB1F8500213C629850197948E2BE2A211C02AEE73032E38756B5F610F67404 |
| StarfieldAppearance.aex | 146BB219DE5CB4E5719615AD26E8414A27674F88129C8539A6D9F916C575C241 |
| StarfieldForce.aex | 662AB626F4D54DECAAA7A54442A58BDF509A6F1D33E7456ED7BEDCBD5894B9D2 |
| StarfieldCore.dll | 0D3C8D672DE171D70DF699C3B8E14A9133E91AB43F74F14B8617F7517E3DA5E7 |

One-step rollback (close AE first):

```powershell
powershell -ExecutionPolicy Bypass -File 'D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\Deploy-TestBuild.ps1' -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02j-build19-animation-binding-fix-20261002' -Rollback
```

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

## Build 17 P-02I follow-up — 2026-10-02

Packed 32785 / 0x8011. Build 16 failed owner qualification at delivery / 516.
Native nodes now directly compile/publish the renderer snapshot with their own
registered AEGP ID; the generic transport and main edit handler are removed.
All node AEXs link the shared publisher/compiler/graph codec. Main 21/native identity
3/layout 6, node schemas and Core ABI 2 unchanged. Native sync 348/camera 12 checks
and renderer controls 38 scoped checks pass without main registration or a working
generic API. Final May 2023 SDK x64 Release /MT build passes with -NoRuntimePublish
-NoDistPublish, including all four independent node AEX links. No compiler warnings /
errors found in the build log. Deployment evidence follows in the native checkpoint;
real AE acceptance remains open. Scratch outputs stay under artifacts/build17-*.

Source b0e9e6b pushed before AE-absent build-17 installation through the existing
single Junction. All six installed hashes, unchanged selected Core and build-16
bundle/selector backups verified. Before/after state and one-step rollback are in
native-node-checkpoint.md. No AE session operated.

## Build 16 P-02I follow-up — 2026-10-02

Packed 32784 / 0x8010; private synchronous native request v2. Main 21/native identity
3/layout 6, node schemas and Core ABI 2 unchanged. Build 15 owner acceptance failed:
all tried native edits reported error 516. Build 16 no longer trusts generic PF
callback params/context; it uses borrowed node context and eight main AEGP streams.
Scalar and graph publication/readback phases are distinct; typed integer receipts
use exact checks, with host float storage exercised. Native sync 330/camera 12
and renderer controls 38 scoped checks pass. Final May 2023 SDK x64 Release /MT
build passes with -NoRuntimePublish -NoDistPublish; no compiler warnings/errors
were found in its log. Deployment evidence follows in the native checkpoint;
real AE acceptance remains open. Scratch output stays under artifacts/build16-*.

Source 76801bb pushed before AE-absent deployment. Existing single Junction and
unchanged selected Core retained; six installed candidate hashes and build-15
backup hashes/selector verified. Before/after records and one-step rollback are
in native-node-checkpoint.md. No AE process operated.

## Build 15 P-02I candidate — 2026-10-02

Packed 32783 / 0x800F; main 21/native identity 3/layout 6; node schemas and Core
ABI 2 unchanged. Native edits use acknowledged generic requests and explicit
renderer stream publication. Scoped native 252/camera 12 and renderer controls
38 checks pass; this is fake-host evidence, not AE acceptance. One paired AEX
update is required because both node transport and renderer entrypoint change.
Candidate build and deployment records stay under artifacts/build15-*; see
native-node-checkpoint.md and ADR 0022 for remaining AE qualification.

Source 96ecf6d pushed before AE-absent deployment. Existing single Junction and
Core generation retained; six installed/selected Core hashes and build-14 backups
verified. Before/after state and one-step rollback are in the native checkpoint.

## Build 14 P-02H candidate — 2026-10-02

Packed 32782 / 0x800E; main 21/native identity 3/layout 6; Emitter 5, Particle 2,
Force 2, Output 3; gateway native-node-sync-14; Core ABI 2 unchanged. Build uses
May 2023 SDK x64 Release /MT with -NoRuntimePublish -NoDistPublish. Scoped checks:
58 Core, 79 actual Force selectors, 38 main setup/immutable live-global checkout,
current-node CEP suite and six source parses. Headers/Win is not present in this
SDK and is removed from every build/editor input. Fresh effects required.
Deployment/hash/rollback evidence is recorded in native-node-checkpoint.md;
compiler/scoped evidence is distinct from AE host qualification.

Build 14 source 6a32f94 pushed before AE-absent installation. Existing single
Junction retained; six installed hashes, selected Core and build-13 backups
verified. Runtime selects StarfieldCore-0D3C8D672DE171D7.dll. Deployment log and
before/after records stay under artifacts/build14-*. One-step rollback is in
native-node-checkpoint.md; host behavior still needs owner verification.

## Current build 13 P-02G deployment — 2026-10-02

Packed 32781 (0x800D), main manifest 20, native identity 3/layout 5, Emitter
graph schema 5, gateway native-node-sync-13, Core ABI 2. Adds native edit dispatch,
transient UUID effect selection, camera projection and unified Auxiliary emission.
Paired candidate builds with the May 2023 SDK x64 Release /MT using
-NoRuntimePublish -NoDistPublish. Main camera flag is implemented and statically
matched with PiPL. No old-development-layout migration; fresh effects required.

Minimum requested checks pass: 28 Core, 14 native sync, 12 camera capture,
77 Emitter and 73 Particle checks; three focused CEP suites. JavaScript syntax
checks are recorded separately. No AE process started/stopped or GUI session run.
Host qualification and remaining reference controls stay open (ADR 0020).
Logs: artifacts/build13-current-node-build.log, build13-current-node-tests.log,
build13-native-sync-tests.log, build13-emitter-tests.log and build13-particle-tests.log.
Six JavaScript sources parse. Source pushed as 2751a56, then installed with AE
absent through the existing single Junction. Installed and selected Core hashes
match; saved build-12 files/selector match before-state. Runtime now selects
StarfieldCore-25F80103DAB9E950.dll. Backup p02g-build13-current-node-interaction-20261002.
[Deployment status and rollback](native-node-checkpoint.md).

| Installed file | SHA-256 |
|---|---|
| `StarfieldParticle.aex` | `9ADA49D5C68831404470E52BEEBDC5E54177764529BD672CE588442BE650A79E` |
| `StarfieldEmitter.aex` | `B1FB35C7943CAD53D98A4DB95F5F97B838E0520D45E4D8F77509912396FC7C05` |
| `StarfieldParticleNode.aex` | `93BD55F46B760D49E3CD08048AEFD79EB2592A98A36F4402135B8D3462DDC3F2` |
| `StarfieldAppearance.aex` | `5E95C8FE843E619291F62B7A7A4D674D3A49127C4F300A29AE08CA9FF84E6A50` |
| `StarfieldForce.aex` | `B1C3499D8999F9C86B2DD787A9B86AEA4FFE4D0B8ABAD2852AA9F6C8CC33A2BA` |
| `StarfieldCore.dll` | `25F80103DAB9E950664E841E47F65E603887B4469D2F96427694187D1C3DA71D` |

## Previous build 12 P-02F deployment — 2026-10-02

Packed 32780 (0x800C); main manifest 20, native identity 3/layout 4,
Emitter graph schema 4, gateway native-node-sync-12. Build 12 adds shared
Particle inputs, reference control units/splits, explicit CEP steps and exact
cancellable rasterization with no default coverage cutoff. No development
migration; graph envelope and C ABI unchanged.

May 2023 SDK x64 Release /MT build passes using -NoRuntimePublish -NoDistPublish.
Four changed JavaScript sources parse. No tests added/run; no AE session operated.
Source pushed as cadab2a before deployment. Installed with AE absent under
standing authorization through the existing single Starfield Junction. All six
installed hashes and selected Core verified. Build 11 is backed up under
p02f-build12-shared-particle-controls-20261002. Runtime now selects
StarfieldCore-3FC7633A1CFC8165.dll. Owner host acceptance remains pending.
Build log: artifacts/build12-shared-particle-controls-build.log.
Before/candidate state: artifacts/build12-deploy-before.json.
[Checkpoint and rollback](native-node-checkpoint.md).

| Installed file | SHA-256 |
|---|---|
| `StarfieldParticle.aex` | `7F0D1BDEB47C8F505889F85C839BC390C4382BB46F603718FAFD205C8255D2A8` |
| `StarfieldEmitter.aex` | `8ECCF65D05E2B9C7A8A97C690F3FD98103CBEB6F71228FB89E809D7DFE86F1AE` |
| `StarfieldParticleNode.aex` | `A58A228761E28C5BA9B3EBE56857993E963CB890BD0D74F94628452B6EE5DC9F` |
| `StarfieldAppearance.aex` | `EB5C46F24C777201E453BBA9D50DC3825AEAB575CE60C350742BAB55B6649744` |
| `StarfieldForce.aex` | `9E9C7163538A80919FE850A48ACECF0028F9A18909AD546902298F9EAA7A484E` |
| `StarfieldCore.dll` | `3FC7633A1CFC81654DE1B5F4ADB85CA898FB987417248B21EF177597C038AAD7` |

## Previous build 11 P-02E deployment — 2026-10-02

Packed 32779 (0x800B); main schema 19, native identity revision 2/layout 3,
gateway native-node-sync-11. All outer parameter topic markers are removed;
surviving disk IDs and Core/graph contracts remain unchanged. Canvas Delete/
Backspace/context-menu actions preserve fixed Output and remove editable nodes.
Build uses -NoRuntimePublish -NoDistPublish; SDK compilation and both CEP syntax
checks pass. No tests added/rerun and no AE session operated.
Actual host deletion and flat control display remain owner acceptance gates.
Source pushed as cb53d50 before deployment. Installed with AE absent under
standing authorization; all six hashes verified. Build-10 AEXs backed up under
p02e-build11-flat-controls-20261002. Core and selected runtime generation remain
unchanged; one existing Starfield Junction retained.
[Deployment status, hashes and rollback](native-node-checkpoint.md).

| Installed file | SHA-256 |
|---|---|
| `StarfieldParticle.aex` | `A6A5DB560F49AC418CF0DC378119BA0BBDD9687BA07D79A78743D4230C9FF442` |
| `StarfieldEmitter.aex` | `290329A2827CF7784F188605284478F0D61030693AF68124FA3CF109DF5483F4` |
| `StarfieldParticleNode.aex` | `EA44DF8C78B9EA78CCF5D3B6F9945C6959456773629111F57E01A64FEA0AEE7C` |
| `StarfieldAppearance.aex` | `BBAC23C24702598ADE33887D137C994E50427ED69180A64E268D976C729A0032` |
| `StarfieldForce.aex` | `4F3B9353644950DEBEA8857EE169C2DC1A9DE5009F5079C5493CDDE9CF951F9B` |
| `StarfieldCore.dll` (unchanged) | `AE18EFC2E9856178B858444BA0E0F7FEC5A4BB7763D53334C873C635A52B280D` |

## Previous build 10 G-06 deployment — 2026-10-02

Packed 32778 (0x800A); main schema 18, native identity revision 2, graph schemas
and CEP native-node-sync-6 unchanged. Multi-emitter evaluation shares one Output
cap; normal cancellation has no dialog message. CEP wire disconnect/port snapping
uses the existing native transactions and installed source Junction.
SDK build and both CEP syntax checks pass; no test suites added/rerun.
Build used -NoRuntimePublish -NoDistPublish. Source pushed as c368bbf before
deployment. Installed under standing authorization after confirming AE absent;
all six installed hashes verified. Owner acceptance remains pending. Before:
build-9 AEXs and Core generation 6D70281C4E756BCA. After: build-10 AEXs and
selected StarfieldCore-AE18EFC2E9856178.dll (its hash also verified). One existing
Starfield Junction retained; no AE process start/stop or other host changes.

| Installed file | SHA-256 |
|---|---|
| `StarfieldParticle.aex` | `13D03304FCA41528BD1DE10FAF04B05A1C3D436C2A62ADB0D6AD6D8B059741F3` |
| `StarfieldEmitter.aex` | `A41FB2A3155DEDC16371565D4ABB54BB2BA1898058AAD09A2CB87EE5F253BE2D` |
| `StarfieldParticleNode.aex` | `BDA6885E48EA6B854C5106F74809B73E5A201A70E2B0B8BD51017DC06535E88C` |
| `StarfieldAppearance.aex` | `AE8CEE33812F38DF7CD2DE778B7A36999C84435C494F4F17FA16DEF82CAE3288` |
| `StarfieldForce.aex` | `322529E0D6EF6F77AC6B6686941CB0CDCC7A3F0B113022F4976832526AB5B8DE` |
| `StarfieldCore.dll` | `AE18EFC2E9856178B858444BA0E0F7FEC5A4BB7763D53334C873C635A52B280D` |

Backup name: p02d-build10-multi-emitter-20261002.
[Before/after state, owner gates and one-step rollback](native-node-checkpoint.md).

## Current build 9 numeric-ID deployment — 2026-10-02

Packed 32777 (0x8009); native identity revision 2, main schema 18 and CEP
native-node-sync-6. All node disk IDs are explicit numbers in 1..9999.
Compile-time range/uniqueness/name-budget guards and the SDK build pass. No tests
added/rerun. Deployed with AE absent under standing authorization; six hashes
verified. Before: build-8 AEXs; after: build-9 AEXs, unchanged Core/hot generation.
Fresh development effects required. Actual Particle creation awaits owner testing.

| File | Installed SHA-256 |
|---|---|
| `StarfieldParticle.aex` | `9F4D55B100263F73911CAB051EB3B7EE707DB117450231B2EF44C88632891E78` |
| `StarfieldEmitter.aex` | `A7F271A713829018B110E7EDF6D4A607085D8BD228990A5D4410D0F22DBD57A2` |
| `StarfieldParticleNode.aex` | `C64652F0F60EEE73C289BA93A24DB11831CD515BCDEA00088923B9EC047F4F51` |
| `StarfieldAppearance.aex` | `C1B265E7CC53FFCA81EDF04C9D710EC6F5677EF9EDEA037B9AD1643BB7F41FB7` |
| `StarfieldForce.aex` | `26F9E96DD1AA43006DCC0B9C5A8E32F4A6138989F37B38E1A7E812B03BF5CA4E` |
| `StarfieldCore.dll (unchanged)` | `6D70281C4E756BCAA65D24E2B4ED0CD06A1C6786BEBEB607983F8DB069B52BA2` |

Single Starfield Junction retained; backup p02d-build9-numeric-ids-20261002.
[Undo and next owner action](native-node-checkpoint.md).


## Previous build 8 topic-ID deployment — 2026-10-02

Packed version `32776` (`0x8008`); schema 18 and CEP `native-node-sync-6` unchanged.
Node GROUP_END IDs are now distinct from GROUP_START, correcting duplicate
structural identities found after the owner's actual duplicate-matchname error.
Value IDs and indices are unchanged. May 2023 SDK build passes without automatic
dist/runtime publication. No tests added/rerun. Deployed under standing owner
authorization after an AE-absent process check; all six installed hashes verified.
Before: build-7 AEXs; after: build-8 AEXs. Core and hot generation are unchanged.

| File | Installed SHA-256 |
|---|---|
| `StarfieldParticle.aex` | `E8336F1705011A618F6FC54F592E22B786436BF6A5DF0486BFFDC39D147931B1` |
| `StarfieldEmitter.aex` | `FF9C735A28D9575AEC5B654479831A20DDAAA281CDEAF773AE41160A84DA49A3` |
| `StarfieldParticleNode.aex` | `64AF2079171FD74F50AA3383B562A41C003E63D6A7A9A0D557272BC599FA12B4` |
| `StarfieldAppearance.aex` | `3A3BD037ABA6CB67D3C3EEAC11C76A5BB96960F29F73685D3F06466E501FE490` |
| `StarfieldForce.aex` | `34A513B3A78E8DBFCEB4601ADCF2E1B6BC4C5C238BE1D1D0F4E34D14D0E86FD4` |
| `StarfieldCore.dll` (unchanged) | `6D70281C4E756BCAA65D24E2B4ED0CD06A1C6786BEBEB607983F8DB069B52BA2` |

Existing single `Plug-ins/Starfield -> dist` Junction retained. Backup name:
`p02d-build8-topic-ids-20261002`; undo and host gate:
[checkpoint](native-node-checkpoint.md). Actual Particle creation awaits owner testing.

## Previous build 7 constant-control deployment — 2026-10-02

Packed version `32775` (`0x8007`); schema revision 18 unchanged. Build removes
redundant CANNOT_INTERP from constant node parameters after actual AE reports a
spatial-interpolation failure during Particle creation. Cause remains a hypothesis
until owner retest. Particle/Appearance checks pass 146 checks; May 2023 SDK build
passes. Built 2026-10-01 without dist/runtime publication; deployed 2026-10-02
under owner authorization after confirming AE was absent. All six installed hashes
match the candidate. Before: build-6 AEXs; after: build-7 AEXs, unchanged Core and
selected hot generation. Future AE-closed deployments are authorized in ADR 0011.
CEP is unchanged (`native-node-sync-6`).

| File | Installed SHA-256 |
|---|---|
| `StarfieldParticle.aex` | `0EAC147D841BBF8FA493B04CE3CFBB806BFE90E3FBB6FE207E2EBA839B0A4E3F` |
| `StarfieldEmitter.aex` | `5429EACC73C391F0D7DE05DCE9311D7ECE24BB05069A8E91711D334B00C26C51` |
| `StarfieldParticleNode.aex` | `571ED1E6A135C058032402D77B1947D41EA7D8837D4E5378AAEF8DDBEEDC81F5` |
| `StarfieldAppearance.aex` | `4AF94D7549D5F2F7A421C3F3177C03222B9E6151BF0C0BD15F2B07639AB4B639` |
| `StarfieldForce.aex` | `EB89EF8DDC5F36E4BA0D47830C99E5EA18AC8DA0D1901D464AD926A1FE4D2BB3` |
| `StarfieldCore.dll` (unchanged) | `6D70281C4E756BCAA65D24E2B4ED0CD06A1C6786BEBEB607983F8DB069B52BA2` |

Deploy/undo uses the existing single Starfield -> dist junction, backup name
`p02d-build7-constant-flags-20261001`; [checkpoint](native-node-checkpoint.md).

## Previous build 6 SmartFX deployment — 2026-10-01

Owner AE error confirms float-aware internal node effects lacked SmartFX. All
four node modules now implement smart pre-render/render passthrough and use
flags2 `0x00001400` in code/PiPL. Packed version: **32774 (`0x8006`)**. No public
parameter type, match name or project-record change; schema remains revision 18.
May 2023 SDK build used `-NoRuntimePublish -NoDistPublish`; 276 actual-node
selector/registration checks and focused CEP checks pass. Source pushed as
`1277561` before the explicitly authorized ADR 0011 deployment. Installed six
hashes match the table; AE behavior remains unqualified until owner testing.

| File | Installed SHA-256 |
|---|---|
| `StarfieldParticle.aex` | `DBD984923DED0F79F8CD7F93ADD5197F1C823BF620ECD1A32FD568DD293E7C5A` |
| `StarfieldEmitter.aex` | `1BC7B0E4ADE174AE715CCB3F44305ABEC1A96F66377610273C5412E012C44532` |
| `StarfieldParticleNode.aex` | `B17EF692AB5059A16826B53748FA95AEF5D02B213A7A0E0E860BAAA24CA20DEC` |
| `StarfieldAppearance.aex` | `1E164BEC00566727BB3466CD81D7EA6D9A5AE0CB7D7065B95B35026919BFF698` |
| `StarfieldForce.aex` | `96811DDA7DD25EF4923D8CF0E939C04B0D32FD66945E185781F28C21592AACE0` |
| `StarfieldCore.dll` (unchanged) | `6D70281C4E756BCAA65D24E2B4ED0CD06A1C6786BEBEB607983F8DB069B52BA2` |

Install/undo uses `tools/Deploy-TestBuild.ps1` with the existing single
`Plug-ins/Starfield -> dist` junction and backup name
`p02d-build6-node-smartfx-20261001`. No new plugin folder is required.
Before: build-5 AEXs. After: build-6 AEXs; Core and selected hot generation are
unchanged. Backup record: `artifacts/disabled/p02d-build6-node-smartfx-20261001/deployment.json`.
CEP gateway token: `native-node-sync-6`; details in [the checkpoint](native-node-checkpoint.md).

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


## Current revision-18 build 5 deployment — 2026-10-01

Build 4 selection safety is owner-confirmed. Build 5 removes the expression
snapshot that still blocked initialization. Independent AE node effects own
authoring records; main numeric revision/checksum and commit/receipt acknowledge
a compiled graph. Code/PiPL both use **32773 (`0x8005`)**. Fresh effects only.

The May 2023 SDK x64 build and 756 adapter checks pass. Targeted gateway,
transaction and startup checks pass. The owner chose to test AE; the agent
did not launch a session. Actual node operations/undo/reopen remain host gates.

Deployment is complete and six hashes match their candidates. Only the existing
`Plug-ins/Starfield -> newStardust/dist` junction is present. Core content is
unchanged; its selected generation remains `StarfieldCore-6D70281C4E756BCA.dll`.

| File | SHA-256 |
|---|---|
| StarfieldParticle.aex | `0E60C605DBF235EB53A95B3EBDB6E9F37F048CC7B2E9CB6734B0D510FBEC0DD8` |
| StarfieldEmitter.aex | `8677BFB1E514A6170C68EA5B93F4600A03BF159E232263C082F4D771EFF4A84A` |
| StarfieldParticleNode.aex | `5725E4CF6FEA9193ABC79B5412C1C8D57280867A75A08A1E760F60A9AB6D6BB5` |
| StarfieldAppearance.aex | `6FA861895302BC46CB100ED3749B8DA33AEE246714CB4B3A979A7C5D51F17DBA` |
| StarfieldForce.aex | `68DC61AF360311F2070C8A2E067B2E67884C241EC453725100F479B7BED9709F` |
| StarfieldCore.dll | `6D70281C4E756BCAA65D24E2B4ED0CD06A1C6786BEBEB607983F8DB069B52BA2` |

Backup: `artifacts/disabled/p02d-build5-native-streams-20261001/`.
One-step undo with AE closed, from this checkout:

```powershell
powershell -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02d-build5-native-streams-20261001' -Rollback
```

Earlier deployed-build sections below are historical.


## Current revision-17 build 4 deployment — 2026-10-01

Owner evidence: build 3 renders particles, but selecting its layer crashes AE.
The latest dump repeats the prior null read in `AfterFXLib.dll+0x1931d36`.
Build 4 replaces unused hidden structural groups with scalar slots; Output is
the renderer's only structural topic. This targets a suspected ECW hierarchy
defect; it is not yet a confirmed crash resolution. Code/PiPL are `32772` (`0x8004`).

The May 2023 SDK full build passes; **721 adapter checks, zero failures**.
The focused checkout-contained deployment check passes report/install/hash/
single-junction/rollback/third-party preservation gates. Core and CEP algorithms
are unchanged and their broader checks were not repeated.

Build 4 is installed with all six hashes verified, after the owner's explicit
authorization to stop AE PID 31772 and deploy. The plug-in root now contains
only one Starfield entry, a junction; loose binaries and the previous runtime
junction are archived under `artifacts/disabled/p02d-build4-single-folder-20261001/host`.
AE has not been restarted or tested by the agent. Build 4 was built with
`-NoRuntimePublish -NoDistPublish`. Installation uses one junction:

```text
Plug-ins/Starfield -> newStardust/dist
  StarfieldParticle.aex + four internal node AEX files
  StarfieldCore.dll
  StarfieldRuntime/          ordinary directory; no second junction
    current.txt + versioned Core DLLs
```

`BuildWindows.ps1 -CoreOnly` now publishes only the nested runtime generation and
atomic manifest. Full AEX publication refuses to run while AE is running.
The retired monolithic/loose-file install scripts are removed.

Executed deployment command, from the checkout (requires AE closed):

```powershell
powershell -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02d-build4-single-folder-20261001' -Install
```

The same command with `-Rollback` instead of `-Install` restores the recorded
previous layout/files. Earlier deployment commands below are historical and
their backup record formats must not be passed to the new single-folder script.

Candidate SHA-256:

| File | SHA-256 |
|---|---|
| StarfieldParticle.aex | `A0BA94BA4378C41D7F0947013385D460E8818E69288A7190FCAC1D6946895B3F` |
| StarfieldEmitter.aex | `C0CA84C91AE7A12D54DF1C2FFF7DF6C6B320C74B2DB8C298B20D339D34BEBA54` |
| StarfieldParticleNode.aex | `8DD515F30DFC046BC9A4D40DD9B3257BC0464B4683C56CF8955BD3843C2303E3` |
| StarfieldAppearance.aex | `6E89D7F5E570E17A02D994AD4A45E72E3AF807ADD1EC180B1306E5AF071278BC` |
| StarfieldForce.aex | `9A451494901F481693C559D3A474B09BE9E99F4B59731A037A09760805893AA4` |
| StarfieldCore.dll | `6D70281C4E756BCAA65D24E2B4ED0CD06A1C6786BEBEB607983F8DB069B52BA2` |

## Prior revision-16 build 3 — 2026-10-01

The complete May 2023 SDK x64 Release pair is **installed** under
`D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins`.
Code and PiPL share packed version `32771` (`0x8003`). Installed hashes:

| File | SHA-256 |
|---|---|
| StarfieldParticle.aex | `0EC7EE78A9E949FCAE98A3C3416BCC651705C54C3ABB526162CB67C0560C92EE` |
| StarfieldEmitter.aex | `2041538D2B52E59A5657161BF6E94E252A9674FF9A23131A4AD74EF9B3333C3A` |
| StarfieldParticleNode.aex | `734C6932077D169EECDA8D779F6C294FF0AD7EAA780EEE0274F71032893DDBB8` |
| StarfieldAppearance.aex | `DD6A129BDF77E852252F36EC07CEBF64C6D8C9A8C47F01864125CEB9C83D7110` |
| StarfieldForce.aex | `6A03CBD210B1CC1FB4BD32AA1DA115FDF13E6D1AECA97EE2EC8E7FE9E3310ACF` |
| StarfieldCore.dll | `6D70281C4E756BCAA65D24E2B4ED0CD06A1C6786BEBEB607983F8DB069B52BA2` |

The existing runtime junction selects `StarfieldCore-6D70281C4E756BCA.dll`.
CEP remains junctioned to this checkout. There is no new junction, registry or
preference change. The prior build-2 pair and selector are archived under
`artifacts/disabled/p02d-build3-20261001/`, with `deployment.json`.

Build and checks: 687 adapter checks, zero failures; eight focused CEP suites pass.
No broader core rerun. Candidate build command (from the repository root):

```powershell
node tools/Run-WithBuildEnvironment.cjs powershell.exe -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -NoRuntimePublish -NoDistPublish
```

The wrapper normalizes duplicate environment-name casing for its build child only.
The two switches prevent accidental publication into an active development install.
Installation is an explicit, hash-verified step in `tools/Deploy-TestBuild.ps1`.
With AE closed, undo this final deployment in one step:

```powershell
powershell -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02d-build3-20261001' -Rollback
```

**Host status:** the prior revision-16 build-2 attempt crashed after renderer apply
and composition-viewer opening. Build 3 is deployed but has not been started in AE.
See [the native-node checkpoint](native-node-checkpoint.md) for exact evidence and
remaining gates. Sections below are historical candidates, not the current install.

## Current build policy (owner direction, 2026-09-27)

AE 2023 on Windows x64 is the only current target. The default script and direct
MSBuild project use the May 2023 SDK and write under `artifacts/plugin/2023/`.
The earlier revision-15 candidate passed 11,909 core checks, 678 adapter fake-host checks, and seven
focused CEP graph codec/edit/view/transaction, native-node gateway, gateway, and startup suites.
The May 2023 SDK build succeeds; candidate hashes and host qualification status are
recorded below. Earlier counts are historical.
Newer SDK/host adaptation is deferred. The older dual-SDK evidence below is
historical and does not qualify the current binary on newer hosts.

## M3-06 revision-13 percentage-curve candidate (2026-09-30)

The current candidate implements per-Particle lifetime, fixed 0–100% Size and
Opacity curves that multiply their independent base values, and straight-alpha
encoding at the AE render boundary. Base Size remains in full-resolution pixels;
base Opacity remains in 0…1. Curve point zero is retained from the project bank and
is not overwritten by either base control. A second adapter issue was found during
the follow-up review: the CEP gateway had declared the hidden Size and Opacity point
streams with pre-release ranges of 100000 and 1. Both now use the same 0–100 range,
and a gateway regression confirms that editing Size preserves an existing Opacity
curve with ordinates above 1.

The May 2023 SDK full build produced `dist/StarfieldParticle.aex`, SHA-256
`B646EF14937700FB31CBA8A957076899231A4D472C9EF6A1DAB460FBB4426560`, and
`dist/StarfieldCore.dll`, SHA-256
`DEF5B7804EBEB6653EE70A4F5B13EC95F14366184D35839DFB5A5B5EA4999F2B`.
`artifacts/runtime/current.txt` selects `StarfieldCore-DEF5B7804EBEB665.dll`.
Core tests passed 11,905 checks. The panel graph-view, graph-edit, graph-transaction,
gateway, and startup suites passed; `git diff --check` passed. The native adapter was
compiled as part of the successful build. AE was not modified or used for this
candidate. In particular, straight-alpha compositing, branch lifetime, and curve
save/undo behavior still need the owner’s AE 2023 pass.

## P-02D initial native node module packaging (2026-09-30)

The full May 2023 SDK build now also produces three separate node AEX files:
`StarfieldEmitter.aex`, `StarfieldParticleNode.aex`, and `StarfieldForce.aex`.
Each module contains one PiPL/match name and a transparent pass-through callback;
the build verifies PiPL/runtime version and flags. The main
`StarfieldParticle.aex` remains the renderer and graph Output, with no separate
Output AEX. Max Particles is now owned by the fixed Output graph node (Output schema
2); Emitter schema 3 no longer stores it. Evaluation validates this Output value
and applies it as the global particle cap. CEP exposes the field in the Output
inspector and graph edits target the Output node. Focused regressions confirm the
captured value is stored on Output and the core respects its cap.

The current full build produced `StarfieldParticle.aex`, SHA-256
`CF6E08544CE81495932DDA3CD0B240C29A899E5EC435303D3170FA48555F616D`, and
`StarfieldCore.dll`, SHA-256
`55B877A8F66D357649CD72938FB883A3C9F5CF584A07ED516E7494828D7C918E`.
Emitter, Particle, and Force node module hashes are recorded in
`docs/compatibility-matrix.md`. Core tests pass 11,909 checks; adapter fake-host
tests pass 678 checks; graph-view, graph-edit, graph-transaction, gateway, and
startup tests pass. The build used `-NoRuntimePublish`, so it did not change
`artifacts/runtime/current.txt`. No candidate files were installed in AE 2023.

This initial packaging candidate predates the CEP transaction wiring below.

## P-02D node synchronization source candidate (2026-09-30)

The current source adds/removes native Emitter, Particle, Appearance, and Force
effect instances from revision-checked CEP graph transactions, writes each
editable node's controls, and commits the canonical snapshot to the main
Starfield Particle effect inside one undo group. The fixed Output terminal stays
visible in the graph, is omitted from the native-node manifest, and has no AEX.
The pinned target token uses project/comp/layer IDs, so adding node effects does
not change it when AE renumbers the Effect Parade.

The May 2023 SDK x64 Release build produced these artifacts:

| Artifact | SHA-256 |
|---|---|
| `StarfieldParticle.aex` | `CF6E08544CE81495932DDA3CD0B240C29A899E5EC435303D3170FA48555F616D` |
| `StarfieldCore.dll` | `55B877A8F66D357649CD72938FB883A3C9F5CF584A07ED516E7494828D7C918E` |
| `StarfieldEmitter.aex` | `CC19F9DED39165FB904ECB726AC0624045D4F2649D85D84BC387DB821F4F5FC7` |
| `StarfieldParticleNode.aex` | `31BA2D394E7C8628D77A9648C7D775A8B8EA09485CCF8BD520AB7240986057D7` |
| `StarfieldAppearance.aex` | `70709CCB724CEF7FBF43CF5118BEEF042744278703137B3F67F469F1F703586A` |
| `StarfieldForce.aex` | `CF6796CCF026ED1BBFD41CF98847C8A9E838139C81F112D6F7D364F7CC8DA7EE` |

Core tests passed 11,909 checks; adapter fake-host tests passed 678 checks; all
seven focused CEP codec, edit, graph-view, transaction, native-node gateway,
gateway, and startup suites passed. The latest native-node fake-host regression
exercises two independent Emitter and Particle instances, two Force instances,
emitter dimensions, UUID streams, failed-add cleanup/retry, Output exclusion,
selective deletion, and graph commit. The full build used `-NoRuntimePublish`;
the build itself left `artifacts/runtime/current.txt` unchanged. The
owner-authorized deployment then
backed up the prior main AEX and runtime selector under
`artifacts/disabled/p02d-node-sync-20260930/`, copied the five AEX files into
`D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins`, and selected
`StarfieldCore-55B877A8F66D3576.dll` through the existing `StarfieldRuntime`
junction. Installed hashes match the table. AE remains closed, so this candidate
has not yet been loaded or host-tested. Effect Parade operations, direct native
Effect Controls edits, undo/redo, duplicate identity, save/reopen, and immediate
render response remain host gates. The read-only rollback report is
`powershell -ExecutionPolicy Bypass -File tools/Rollback-P02D-Candidate.ps1
-PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins'`;
append `-Rollback` to restore the backed-up main AEX and Core selector and move
the newly added node AEX files into the backup folder.

## P-02D initial direct node-control synchronization candidate (2026-09-30)

The initial source prototype forwards a node effect's constant parameter edit
through `PF_Cmd_USER_CHANGED_PARAM` to the main renderer's supervised graph
callback. CEP batch writes raise a node-local guard to avoid committing each
intermediate field. Node controls are marked non-time-varying to match the
constant-value graph contract. Output remains a visible logical terminal owned
by `StarfieldParticle.aex`; no Output module exists in `dist/`.

The May 2023 SDK x64 Release build passed. The focused native-node gateway check
passed for Emitter creation, independent values, dimensions, duplicate-ID
rejection, duplicate cleanup, and graph commit. This candidate was built with
`-NoRuntimePublish`; the Core hash matches the selected runtime, so no runtime
selector change was needed. With AE closed, the five AEX files were backed up
under `artifacts/disabled/p02d-direct-node-sync-20260930/` and the candidate
files copied to the AE 2023 plug-in directory. Installed hashes match this
table; AE has not yet loaded or exercised the candidate. Its hashes are:

| Artifact | SHA-256 |
|---|---|
| `StarfieldParticle.aex` | `91889136D24DA756CF181756D1426B8BA0D6BFB63234FAE21BC053FE3DB9F546` |
| `StarfieldCore.dll` | `55B877A8F66D357649CD72938FB883A3C9F5CF584A07ED516E7494828D7C918E` |
| `StarfieldEmitter.aex` | `99EAA591F53C520DE07CE29AD905D2DAD18616976445BB68EA0B8849BDD611CC` |
| `StarfieldParticleNode.aex` | `4557DADACEA74E1F5114D5653C80A2D257FEDFE08EE2DE382136D9FBEEC28680` |
| `StarfieldForce.aex` | `E4826E794D2F01E9BB626F50A8047A5F2CBCCF788FCD122E8BB4C73DC176E0D8` |
| `StarfieldAppearance.aex` | `06A6FD566826854AD80A1B31FB9F3B46020740CBE42BF71AF4FD2D671217C555` |

AE callback delivery, cache/render response, undo, and project-reopen behavior
remain unqualified. To undo this deployment while AE is closed, copy the five
same-named files from
`artifacts/disabled/p02d-direct-node-sync-20260930/` back to
`D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins\`.
The compiler emitted the existing C4819 code-page warning for non-ASCII text
in `Parameters.hpp`; the build completed successfully.
## P-02D retained graph-commit nonce correction (2026-09-30)

Review found that CEP leaves its last graph transaction nonce in the hidden
`Graph Edit Commit` parameter. Native node edits carry a separate zero-nonce
request, but the main effect previously rejected them against that retained CEP
nonce before inspecting the request envelope. The main renderer now recognizes
the node request first and only applies the retained commit nonce to ordinary
CEP graph transactions.

The May 2023 SDK x64 Release build passed. With AE closed, only the rebuilt main
renderer AEX was copied into the plug-in directory; the four node AEX files and
Core DLL/selector were left as installed by the previous candidate. The old
main AEX is backed up at
`artifacts/disabled/p02d-node-nonce-fix-20260930/StarfieldParticle.aex`.

| Artifact | SHA-256 |
|---|---|
| `StarfieldParticle.aex` | `3277A6F65567D58E67CD64F4C72AB7603CB38BC0720CC9CF3C5EAB4F0ACCAFEF` |
| Previous `StarfieldParticle.aex` backup | `91889136D24DA756CF181756D1426B8BA0D6BFB63234FAE21BC053FE3DB9F546` |

AE has not loaded this fix. An `aerender` probe against the authorized test
project found neither a comp named `Comp 1` nor a saved render-queue item at
index 1, so no frame was rendered; its temporary output was removed. A comp name
is needed for the next host render check. To roll back while AE is closed, copy
the backup AEX above to
`D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins\StarfieldParticle.aex`.

## P-02D hide internal node modules from the Effects menu (2026-09-30)

The four node modules now set `PF_OutFlag_I_AM_OBSOLETE` in both their PiPL
resources and `PF_Cmd_GLOBAL_SETUP` response. The May 2023 SDK documents this as
keeping an effect out of the AE Effects menu while still invoking existing
instances. A compile-time flag assertion and the PiPL generation guard both
passed. CEP continues to request nodes by their stable match names; adding a
new hidden node through that path is an AE 2023 host gate.

The May 2023 SDK x64 Release build completed with the existing C4819 code-page
warning from `Parameters.hpp`. The four node AEX candidates in `dist/` are:

| Artifact | SHA-256 |
|---|---|
| `StarfieldEmitter.aex` | `16C9FA717264D445BB7ECBFEE92920DC318D3774212FDA4364E8D1E72B0A9494` |
| `StarfieldParticleNode.aex` | `2439EBE6550A3369D4EBB3894931CFEDFA7544F64DC9DAF5D55CC130E65E9169` |
| `StarfieldAppearance.aex` | `0AF3A3C450E5B092DA462D11FD9DD71F5716F14D01FFD0C6F16ECCD9D5502CBF` |
| `StarfieldForce.aex` | `E755C294E0B5654F662132E4AECE3490A701B982D054A8BB493E01825711EF5E` |

The four node AEX candidates were installed on 2026-09-30 into
`D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins` after
backing up the replaced modules under
`artifacts/disabled/p02d-hide-node-menu-20260930/`. Installed SHA-256 values
matched the table above. AE was closed during replacement and has not loaded
this candidate yet, so menu visibility and CEP add-by-match-name behavior still
need an AE 2023 host check. The main renderer and Core were not copied: this
full build's outputs differ and contain no change required by the menu task.
The build used `-NoRuntimePublish`; `artifacts/runtime/current.txt` still
selects `StarfieldCore-55B877A8F66D3576.dll`, and the installed main AEX remains
`3277A6F65567D58E67CD64F4C72AB7603CB38BC0720CC9CF3C5EAB4F0ACCAFEF`.

## M3-06 Particle lifetime and curve authoring candidate (2026-09-30)

The May 2023 SDK full build includes per-Particle lifetime in explicit graph
evaluation, the CEP Particle inspector mapping, pixel-unit labels for Size and
Size Over Life, transformed SVG pointer coordinates, and a non-destructive
interpolation selector. Candidate AEX SHA-256:
`062130857B58B83EDA03358F85E352DADE4B76921356F845DEC7B89A97490481`.
Paired Core DLL SHA-256:
`5783F369984611AD3B3843AB28B0841AB1C4754E4330FC13E40E1C753C8E2E14`;
runtime manifest selects `StarfieldCore-5783F369984611AD.dll`. The build updated
this checkout's ignored artifacts, runtime, and `dist/` copies; the copies match
the candidate hashes. No AEX or Core file was installed into AE. No test suite
was run for this task. The owner's report of dark translucent white particles
over a blue background remains open for AE qualification; source inspection found
the core applies premultiplied alpha once and the adapter copies those channels
to AE's pixel structs, so the output contract is unchanged pending that check.
The core suite passed 11,082 checks before adding a focused per-Particle lifetime
assertion; the updated graph-view, graph-edit, graph-transaction, and gateway
checks passed. Automatic review blocked rerunning the core suite after that test
file edit, so the new assertion still needs execution.

## M3-05 Particle variation build (2026-09-30)

The May 2023 SDK build appends the revision-11 Size Random and Opacity Random controls,
their Particle/Appearance graph values, and stable per-particle attenuation after the
age curves. The candidate AEX SHA-256 is
`D367A3830A23F312E1D3150DBB392ADDF9E5073A182E40836AAA86458A188E6D`; the paired Core
DLL SHA-256 is
`D77BD11088A5D54B479FDA19FFED5183F8E8D85CC4490D0AB931F74B1678CC97`. The runtime
manifest selects `StarfieldCore-D77BD11088A5D54B.dll`. The build updated the workspace's
ignored `dist/` and runtime copies; it did not replace the installed AE plug-in. The
core suite passed 11,082 checks, the adapter fake-host suite passed 676 checks, and the
graph-view, graph-edit, gateway, and startup CEP checks passed. The AEX has not been
qualified in AE 2023; visible variation, undo, and save/reopen remain owner checks.

## P-02C over-life curve build (2026-09-30)

The May 2023 SDK build includes the project-owned Size/Opacity curve banks, CEP
curve editor, dynamic graph transaction integration, and the adapter fix that keeps
AE Controls appearance endpoints enabled during graph projection.
`artifacts/plugin/2023/x64/Release/StarfieldParticle.aex` SHA-256:
`8ACF50103F2B00E50291308332091E49DD359D6EEF92FAEE2DBEE4EDE7E9C185`.
The paired Core DLL is `artifacts/core-dll/2023/x64/Release/StarfieldCore.dll`,
SHA-256: `94A26570D09B9F97838BBE5696CC8DA04934C6430D22AC2E5C8B305CA4B4E46B`.
The build script also updated this checkout's `dist/` copies and content-addressed
`artifacts/runtime/current.txt`. The core suite passed 9,780 checks, including custom
curves on Particle branches and legacy Appearance chains, plus disconnect/reconnect
coverage for every edge in the default chain; the adapter fake-host suite passed 660
checks. The graph-view and panel-gateway suites passed for this update. The AEX has not
been copied into the plug-in directory. That directory still had AEX SHA-256
`EA1F8B15FE1925FEBA357C39A179AD1DF541BD41FCBA2C9F61D9981C444D4169` and pinned Core
SHA-256 `A4F104B5858DE5938F87B93D4B59FF89A5E324CD238DFDB3AD67B31327CD2545` when
inspected. Its `StarfieldRuntime` junction points to this checkout's `artifacts/runtime`;
the build selected `StarfieldCore-94A26570D09B9F97.dll`. Smart Render uses
`acquire_core()` and the Effect Controls Options action calls `reload_core()`. No graph,
curve, undo, or save/reopen host check has been made on this candidate. The plug-in was
not installed or replaced during this build.

## M3-01B direct emitter dimensions (2026-09-30)

Pre-release manifest revision 12 uses direct full-resolution layer-pixel Size X/Y/Z
values (0–100000, default 100 px) for Box and Sphere. The `Disc Size` control retains
the planar Disc diameter; Point and Disc ignore the axis dimensions. `CpuRenderer`
passes layer height and pixel aspect to the core, which converts the dimensions to
canonical world units before sampling. The paired May 2023 SDK candidate is
`artifacts/plugin/2023/x64/Release/StarfieldParticle.aex`, SHA-256
`1DBAA18313010837BC24977962D8E4299BF11702B4C3D928CC0A7084F702DA78`, and
`artifacts/core-dll/2023/x64/Release/StarfieldCore.dll`, SHA-256
`6E660BB1C4369D07DA4F6383531A7CEE85BC0A10C722FD98D6F66F33F26FABFA`. The full build
updated the checkout's `dist/` pair and runtime manifest; no host plug-in file was
replaced. Core tests pass 11,874 checks, the adapter fake-host suite passes 676 checks,
and panel graph-view, gateway, and startup checks pass. AE visual/project-lifecycle
qualification remains open.

## M3-06 requested-alpha output candidate (2026-09-30)

The renderer now encodes its premultiplied internal accumulation in the requested
`FrameSpec::alpha_mode`. The AE adapter candidate requests straight output after the
previous paired build rendered white particles darker than an opaque blue lower layer
(background sample `[0,108,255,255]`; particle sample `[16,98,209,255]`). Core tests
passed 11,889 checks, including requested straight output at 8/16/32 bpc and rejection
of an invalid alpha-mode enum; the May 2023
SDK build succeeded. Candidate AEX SHA-256 is
`E1F155B8BE0ECEC8FA5A06ADE74C8A00E0420FD8E4F4AC2C5D1F0228CEF7AE95`; paired Core
SHA-256 is `0C37D709166AF8DD3E5244F87B36DAC5035B788E8067FEC35B4A0979167C4DFA`, and
runtime `current.txt` selects `StarfieldCore-0C37D709166AF8DD.dll`.

The host still has the earlier AEX SHA-256 `1DBAA183…` at the plug-in root. Its
`StarfieldRuntime` Junction selects the new Core, but that AEX still requests
premultiplied output. The new straight-alpha AEX has not yet been installed or rendered
in AE. The planned `Plug-ins\dist` Junction and sibling `dist\StarfieldRuntime`
Junction await the exact installation authorization; no plugin-directory change was
made for this candidate.

Read-only AE 2023 plug-in path audit (2026-09-30): the only discovered
`StarfieldParticle.aex` is at `D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins\StarfieldParticle.aex`
with SHA-256 `1DBAA183…`. The root `Plug-ins\StarfieldRuntime` Junction points to
this checkout's `artifacts/runtime`, and `current.txt` selects
`StarfieldCore-0C37D709166AF8DD.dll`. The plugin `dist` child path is absent. AE was
not running during this audit.

Historical: M0/M1 Windows x64 builds passed against the supplied May 2023 and SDK 26.5 inputs. The user confirmed the corrected M1 shell loads in AE 2023; its exact build is not recorded. The M1-era 8001 version mismatch was corrected. These older artifacts are not the current binary.

Superseded monolithic artifact: plug-in build 2 / packed version `0x8002`,
184,832 bytes, SHA-256 `C0830F649A149990942B40531E25E23C0841FA6BED9C9805B9D3E233AB1ABCAB`.
It is backed up under `artifacts/disabled/h01-ae2023-crt-before-20260928`.
AE 2023.5.0 Build 52 visually confirmed transparent particle output on the preceding
candidate `D22D43BAD15C5173867907369B2EF3293A3FD601C308158665A1F3FD0AB0816B`.

Initial H-01 AE 2023.5.0 Build 52 host candidate: installed `StarfieldParticle.aex`
SHA-256 `B7362B01AC0E935D8AD596A70D61690DA4D586EEEC3E939328BA1BDC420069D5`
and pinned `StarfieldCore.dll` SHA-256
`A4F104B5858DE5938F87B93D4B59FF89A5E324CD238DFDB3AD67B31327CD2545`.
The development junction currently selects `StarfieldCore-095219764514FFCA.dll`,
SHA-256 `095219764514FFCA1A5C3CFD36D78E6C8368ECF32C6C17576A8C26F2FA584564`.
`BuildWindows.ps1 -CoreOnly` rebuilds
the DLL and atomically updates `artifacts/runtime/current.txt` without rebuilding
the `.aex`; the observed `.aex` SHA-256 remained unchanged. Automated evidence:
6,196 core checks, 395 adapter checks, and a versioned-DLL loader harness pass.
Runtime publication verifies the full DLL hash before exposing a new versioned
filename, and refuses an existing filename with different contents; only then
does it replace `current.txt`.
The CoreOnly adapter-input fingerprint guard was exercised by temporarily
changing only its generated artifact and confirming it refused the build;
the original fingerprint was restored afterwards.
The first `/MD` split adapter crashed in AE before loading Core: the host's
app-local `MSVCP140.dll` is version 14.00.24210.0, older than the v145 toolset's
STL. Both Windows modules now use `/MT` in Release (`/MTd` in Debug), and
`dumpbin /dependents` lists only `KERNEL32.dll` for each. The rebuilt pair
passed the host smoke checks recorded in `docs/compatibility-matrix.md`.

Generation bookkeeping, verified on 2026-09-29: the full May 2023 SDK build for the
compact Options readout produced `dist/StarfieldParticle.aex`, the build-tree AEX,
and the installed AEX at SHA-256
`7BFE7092325C9AEE9E777DEDBFE31D5042249F0A4A78A11B35E23BAE0A1D3EB9`.
The former installed `B7362B01…` AEX is backed up under
`artifacts/disabled/StarfieldParticle-before-options-20260929.aex`. The full build
also published `dist/StarfieldCore.dll` SHA-256
`095219764514FFCA1A5C3CFD36D78E6C8368ECF32C6C17576A8C26F2FA584564`,
which matches `artifacts/runtime/current.txt` and its selected versioned DLL. The
pinned fallback DLL in the host directory still has SHA-256 `A4F104B5…`; the host
test loaded the selected versioned DLL. `dist/` now reproduces the selected Core
generation, and the development junction remains active. This AEX-only update was
qualified for the Options readout in AE 2023.5.0 Build 52; the older hot-reload smoke
checks remain recorded against `B7362B01…`.

The subsequent manifest parser build produced and installed `StarfieldParticle.aex`
SHA-256 `AEF074782242E9C76781DDF0FC43C197064F387C38D1AF0FE680490BB7EFC034`.
The preceding `7BFE7092…` AEX is backed up as
`artifacts/disabled/StarfieldParticle-before-manifest-20260929.aex`; after AE exits,
copying that file over the installed AEX restores it. The selected versioned Core,
pinned fallback Core and valid `current.txt` were unchanged. The loader harness
passed with a new two-line manifest rejection case, and AE 2023.5.0 Build 52
reported the malformed manifest while retaining visible particles. After the valid
manifest was restored, Options reported `Core: current DLL` in the same AE process.
See `docs/compatibility-matrix.md` for the exact host scope.

The layout-parameter build was rebuilt with the May 2023 SDK on 2026-09-29 and
installed once while AE was closed. Installed `StarfieldParticle.aex` SHA-256 is
`901AD65F50C71C992EA07EEA624610E1E7DF0B1794D4372CE10C4B12D0FE5727`; the prior
`AEF07478…` file is preserved at
`artifacts/disabled/StarfieldParticle-before-layout-parameters-20260929.aex`.
The existing `StarfieldRuntime` junction and selected
`StarfieldCore-095219764514FFCA.dll` were unchanged. The AEX registers the eight
project-saved node-layout streams. The owner subsequently confirmed the interface
and plug-in load without issue; a dedicated node-move save/reopen and undo/redo
pass is still required before project-layout persistence is host-qualified.

AE 2023.5.0 Build 52 `aerender` also exported frames 51–53 of `Comp 1` from
`testproject.aep` as 3840×2160 premultiplied RGBA PSD sequences with MFR off.
The current split AEX/Core and the prior AE-qualified monolith `D22D43BA…`
produced identical decoded channel pixels on all three frames. A different Core
changed 7,818 pixels on frame 51 as a cache control. The tracked
`tools/Compare-PsdFrames.cjs` compares flattened pixels while ignoring changing
PSD resource metadata. The actual exports and scratch decode output are ignored
under `artifacts/reports/h01-parity/`. `aerender` could not write to the Unicode
checkout path directly, so each run temporarily mapped the checkout to `Z:`
and removed that mapping afterwards. The original AEX and manifest were restored.
See `docs/compatibility-matrix.md` for scope, hashes and remaining host gates.

## Artifact layout

`artifacts/` is Git-ignored and holds only generated files. Every entry is either reproducible from a
command in this file or an externally produced input that must not be edited.

| Path | Contents | Reproduce with |
|---|---|---|
| `plugin/2023/` | Current AE 2023 build: `x64/Release/StarfieldParticle.aex` plus `x64/Release/StarfieldParticle.pdb` and the intermediates under `obj/` | `powershell -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1` |
| `node-effect-tests/<kind>/` | Actual internal node selector/registration fake-host checks, compiler response files and executable | `powershell -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1 -NodeEffects -NodeKind Particle` (Emitter/Appearance/Force also supported) |
| `core-dll/2023/` | Separately built `StarfieldCore.dll` and intermediates | `powershell -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -CoreOnly` |
| `runtime/` | Content-addressed `StarfieldCore-<SHA16>.dll` files and atomic `current.txt` selection for manual hot reload | Either full or `-CoreOnly` build |
| `loader-tests/` | Loader harness and its workspace-local simulated runtime directory | Build `tests/CoreLoaderTests.vcxproj`, then run it with `dist/StarfieldCore.dll` and `dist/StarfieldParticle.aex` |
| `../dist/` | Main renderer/Core pair plus `StarfieldEmitter.aex`, `StarfieldParticleNode.aex`, `StarfieldAppearance.aex`, and `StarfieldForce.aex` node modules, with matching PDBs | Full build; `-CoreOnly` intentionally does not update AEX files |
| `disabled/` | Rollback binaries, renamed with a timestamp, for example `20260927-232930-StarfieldParticle.aex` (build before the force/appearance controls shipped) | Previous build, kept on purpose |
| `core-tests/`, `adapter-tests/` | Test executables and their object files, so the suites link incrementally: `artifacts/core-tests/core_tests.exe` (core) and `artifacts/adapter-tests/core_tests.exe` (`-Adapter`, fake host) | `powershell -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1 [-Adapter]` |
| `reports/` | Captured readouts and parameter dumps, for example `dump_report.txt` from the scripting-DOM dump | `tools/dump_effect_parameters.jsx` inside After Effects |
| `crash/` | Windows minidumps kept for triage; the analysis tools are `tools/scan_dump.ps1`, `tools/scan2.ps1`, `tools/dump_strings.ps1` | After Effects crash |
| `reference/` | Externally produced reference data, for example `stardust_effect_parameters.txt` from the installed reference product | Reference product, not this repository |

Obsolete build trees (the M1 `m1/` label and the pre-2023 `plugin/x64`, `plugin/obj` targets) were
removed on 2026-09-27; nothing referenced them and the current script always writes `plugin/2023/`.

For the H-01 developer build, `tools/Deploy-HotCore.ps1 -PluginDir <AE 2023 Plug-ins>`
is read-only by default and prints the exact AEX, DLL, junction and rollback
paths. After ADR 0011 authorization, `-Install` backs up the old files and
creates the development junction; `-Rollback` restores the previous files and
removes only that junction. The older `tools/Install-Plugin.ps1` is retained for
monolithic rollback and refuses the H-01 paired build. For older builds it
copies `dist/StarfieldParticle.aex` into the plug-ins folder recorded by Adobe's
`PluginInstallPath` registry value (or `-PluginDir`), renames whatever it replaces into
`artifacts/disabled/` with a timestamp, refuses while After Effects is running, and moves the file
back out with `-Uninstall`. It never edits the registry and never deletes anything.

## PiPL and runtime flags must be regenerated together

AE rejects a plug-in whose PiPL `AE_Effect_Global_OutFlags` disagree with the values returned by
`PF_Cmd_GLOBAL_SETUP` ("global outflags mismatch"). That happened once because MSBuild's
`CustomBuild` step only tracked `StarfieldPiPL.r`, so editing `PluginFlags.h` left a stale PiPL
inside the `.aex`. Two guards now make that failure impossible to ship silently:

- `Starfield.vcxproj` declares `AdditionalInputs` (the metadata MSBuild uses as the dependency
  list for `CustomBuild`) for `PluginFlags.h` and `PluginVersion.h`, so a header edit triggers
  regeneration. `Outputs` stays declared: without it MSBuild skips the custom build entirely
  (MSB8018).
- `BuildPiPL.ps1` reads the flag and version values from those headers and fails the build when
  the generated `StarfieldPiPL.rc` does not declare them.

## Core self-test

`tests/core_tests.cpp` covers rational-time normalization and overflow, settings validation, simulation determinism and boundaries, graph identities/schema/type/cardinality/cycle validation, sequence codec round-trips and malformed-input rejection, ROI equality against full-frame rendering, world-to-pixel mapping at a downsampled frame grid, transparent particle output and premultiplied alpha, bit-depth output, the force/appearance chain (closed-form gravity/drag against the analytic solution, age curves, stage-order and single-appearance enforcement, codec round-trip of a four-stage graph), the bounded-work/cancellation paths, and the new core C ABI's byte-for-byte render parity, inspect, invalid-request and result-release paths.

`tests/core_loader_tests.cpp` loads two uniquely named copies of the built DLL
from an `artifacts/loader-tests/` runtime directory. It confirms manual switch,
cache-identity change, old-generation execution while leased, unload after the
final lease, and fallback on malformed, missing or ABI-incompatible DLLs. Four
concurrent callers also retain leases and call the old or new DLL API across 32
successive reloads. A real 64×64 particle render pauses inside its cancellation
callback while the selected DLL changes; it then finishes on its pinned generation,
releases its result and allows that DLL to unload. These checks passed on
2026-09-28. AE render/cache invalidation still requires a host check.

`tests/graph_parameter_tests.cpp` (`-Adapter`) covers the arbitrary-data callbacks, parameter registration and mapping, the four-stage chain built from the controls, the supervised edit path (Node Graph rewrite, AE Controls isolation, allocation failure), checkout/checkin bookkeeping, and cancellation during host-world copies.

`tests/panel_gateway_tests.js` runs the ExtendScript protocol gateway in a Node fake host and covers multidimensional animation rejection, successful writes, and failed-batch rollback. `tests/panel_native_node_gateway_tests.js` exercises node AEX creation/removal, independent values, emitter dimensions, identity streams, and graph commit. `tests/panel_startup_tests.js` runs the CEP client in a fake DOM/host and confirms transient startup `no_target` recovers without a manual Refresh. The graph codec, edit planner, projection and revision-checked transaction coordinator have focused tests in `tests/panel_graph_codec_tests.js`, `tests/panel_graph_edit_tests.js`, `tests/panel_graph_view_tests.js` and `tests/panel_graph_transaction_tests.js`. Run the panel checks with `node tests/panel_graph_codec_tests.js`, `node tests/panel_graph_edit_tests.js`, `node tests/panel_graph_view_tests.js`, `node tests/panel_graph_transaction_tests.js`, `node tests/panel_native_node_gateway_tests.js`, `node tests/panel_gateway_tests.js` and `node tests/panel_startup_tests.js`. None replaces the AE 2023 panel qualification gate.

- With CMake available: `cmake -S . -B build && cmake --build build && ctest --test-dir build`.
- On the current Windows machine CMake is not installed, so use `powershell -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1`, which compiles the same sources with the locked MSVC toolset behind a mapped drive letter and runs the executable. Output lands in `artifacts/core-tests/`.

Both paths compile only the host-independent core; the `.aex` itself is still built by `ae_plugin/BuildWindows.ps1`.

## Toolchain lock procedure

Use the supplied May 2023 SDK for the current build. SDK archives, sample source, PiPL binaries, and third-party headers stay outside the source repository; only selected version metadata belongs here.

The May 2023 SDK's `Examples/Util/Param_Utils.h` still calls `strncpy`, which `/sdl` (enabled by `SDLCheck`) escalates to an error, so the project defines `_CRT_SECURE_NO_WARNINGS`. Owned code does not use `strncpy`; the definition exists only to keep the baseline SDK headers compiling.

For reproducible project builds, pin exact stable releases (not floating `latest`) at the initial build cut:

| Component | Planned choice | Lock status |
|---|---|---|
| AE SDK primary | May 2023 SDK (`May2023_AfterEffectsSDK`) | Current default; G-03 `.aex` build passed |
| Additional local SDK | SDK 26.5 (`AfterEffectsSDK_26.5_win`) | Historical build evidence only; current adaptation deferred |
| Visual Studio Build Tools | 2026 18.7.8 | Installed; MSBuild build succeeds |
| MSVC toolset / compiler | v145 / 14.51.36231; compiler 19.51.36248 | Locked for current Windows build |
| Windows SDK | 10.0.26100.0 | Installed and selected |
| CMake | Optional; latest stable at core-build cut | Not installed here; `tests/RunCoreTests.ps1` builds the core self-tests with the locked toolset instead |
| Ninja | Optional; not used by current MSBuild project | Not required |
| Language | C++20 for owned core; SDK adapter follows SDK sample ABI requirements | Selected |
| Runtime dependencies | `/MT` Release and `/MTd` Debug for both AEX and Core; C ABI owns all allocation within its module | Selected after AE 2023 crash triage; host pair loads |
| PiPL/resource tools | PiPLTool shipped in each selected local Adobe SDK | Both SDK resource pipelines build successfully |

## Release host/architecture matrix

| Host | Windows x64 | Windows ARM64 | macOS Intel + Apple Silicon |
|---|---|---|---|
| AE 2023 (record exact 23.x build) | Current qualification target | Deferred | Deferred |
| Newer AE families | Deferred by owner direction | Deferred | Deferred |

Only mark a cell supported after installing/loading the signed test artifact, applying it to a project, rendering, and exercising save/reopen. A successful compile is not host compatibility evidence.

## P-02D graph mailbox capability fix (2026-09-30)

The owner's node-copy attempt showed `graph_commit_failed` because AE rejected
writes to `Graph Edit Request.expression` and `.expressionEnabled`. The request
parameter had been marked `PF_ParamFlag_CANNOT_TIME_VARY`, which made the hidden
mailbox unavailable to the CEP scripting setter. The source now leaves only this
request stream expression-capable; its expression remains disabled after use.
The gateway checks `canSetExpression` before changing node effects and fails
without opening an undo group if the running host cannot write the mailbox.

Panel refresh now loads the saved graph snapshot in either control mode, so the
initial sync can materialize native node effects while the canvas remains editable
only in Node Graph mode. Target discovery continues polling while no layer/effect
is selected, allowing the panel to notice a newly applied main effect without
manual Refresh. Repeated refreshes skip an empty undo group once all node effects
already exist.

The May 2023 SDK x64 Release build passed. Focused `panel_native_node_gateway`
and `panel_startup` suites passed, along with `git diff --check`. The build used
`-NoRuntimePublish`; the Core selector is unchanged. The main AEX candidate is
SHA-256 `6301092C5D0E7A4C9B3FC646488A3F364BF76A97FF5115B75DCE83C1F1B22897`.
The previous installed main AEX (`3277A6F65567D58E67CD64F4C72AB7603CB38BC0720CC9CF3C5EAB4F0ACCAFEF`)
was renamed to `artifacts/disabled/p02d-mailbox-fix-20260930/StarfieldParticle.aex`.
After the owner authorized closing AE, PID 22800 was ended without saving and the
candidate was copied into `D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins\StarfieldParticle.aex`.
The installed hash matches the candidate. AE is closed and has not loaded it yet.
Graph add/copy/delete, automatic node creation, undo, persistence, and native
Effect Parade deletion remain host checks. Revision 14 now adds a project-owned
readiness marker and source reconciliation that removes a manually deleted node
and its incident edges from the graph. The revision-14 main AEX is installed;
the previously installed mailbox-fix AEX is backed up at
`artifacts/disabled/p02d-node-delete-sync-20260930/StarfieldParticle.aex`. To roll
back, while AE is closed, restore that backup to the plug-in path.

## Local SDK inputs

The SDK is provided by the developer from Adobe Developer Console. Configure a local SDK path (do not commit SDK files). The plug-in build consumes Adobe headers and the official PiPL conversion tools from that tree. `ae_plugin/BuildWindows.ps1` maps the checkout to a temporary drive during the build to avoid legacy PiPL tool failures on paths containing spaces or non-ASCII characters. We do not provide placeholder SDK headers or a replacement PiPL compiler.

## P-02D reverse deletion reconciliation build (2026-09-30)

The May 2023 SDK Release build succeeded with:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -SdkPath 'AdobeSDK\May2023_AfterEffectsSDK' -ArtifactLabel 2023 -NoRuntimePublish
```

The build compiles schema revision 14, including hidden renderer parameter
`Node Effects Ready` (ID 89). Main AEX SHA-256:
`9B3D75B2AA9E1EC1DED90F0993DCB7E66DD28FFC91DDB11968E656C1D9204017`.
The build used `-NoRuntimePublish`, so the selected Core DLL did not change.
MSVC emitted existing C4819 code-page warnings for non-ASCII comments in
`Parameters.hpp`; compilation and PiPL generation succeeded. The candidate was
installed at `D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins\StarfieldParticle.aex` after confirming AE was closed. Its installed hash matches the build. The previous AEX was backed up under `artifacts/disabled/p02d-node-delete-sync-20260930/`; AE remains closed and has not loaded the new build.

Focused verification passed: `panel_native_node_gateway_tests.js`,
`panel_graph_transaction_tests.js`, `panel_startup_tests.js`, the adapter suite
(682 checks, 0 failures), and `git diff --check`. Host verification remains open
for first materialization, manual node-effect deletion, undo/redo reconciliation,
and project reopen.

## P-02D undo conflict guard (CEP source, 2026-09-30)

The CEP transaction client now remembers which native Effect Parade deletions it
has already reconciled during the current panel session. If AE undo restores a
graph node before restoring its native effect, refresh reports
`native_node_undo_conflict` and does not submit a second graph deletion. The
guard clears when the native effect reappears. Reconciliation also rereads the
graph and compares its revision to the one inspected before submitting the
deletion, so an intervening graph edit cannot be pruned using stale information.
This is CEP-only source and does not require replacing the installed AEX. The
focused graph transaction suite passes with regressions for graph-only undo,
restoration, and a stale inspection; it verifies that neither conflict submits
a second or stale graph edit. Since the undo guard is session-local, save/reopen
while graph and native effects disagree still needs a host test.

## P-02B fresh graph request carrier (revision 15, 2026-09-30)

The owner still saw `canSetExpression=false` on the request stream after the
revision-14 candidate was installed. The cause is unconfirmed. Revision 15 keeps
index 42 as an inert legacy slot and appends a new active request at index 90.
The CEP gateway and native graph callback both target index 90.

The May 2023 SDK Release build passed. The focused
`panel_native_node_gateway_tests.js` check and adapter fake-host suite passed
(689 adapter checks, 0 failures). Main AEX SHA-256:
`EFAEE26451AC42B68D7FAFDD22A710514EA3A7E65BC5276CD7FE6B5493447DB7`.
It is installed at the AE 2023 plug-in path. The revision-14 AEX is backed up at
`artifacts/disabled/p02b-carrier-v15-20260930/StarfieldParticle.aex`. The selected
Core DLL hash matches the build, so Core was not replaced. AE is closed and has
not loaded revision 15; retry one node copy/add operation, then check deletion
and reconnect only if the carrier is accepted.

## P-02B carrier lookup correction (CEP source, 2026-09-30)

The gateway previously resolved the carrier by display name before its registered
parameter index. Since revision 15 reuses the old request label at a new index,
an effect instance retaining the earlier name could cause the gateway to inspect
the legacy request at index 42. Carrier resolution now checks the registered
index first and verifies `propertyIndex`; the name fallback is accepted only for
that same index. If index 90 is absent, the gateway reports the missing registered
slot instead of using a same-name legacy property.

The focused native-node gateway fake-host test passes with the stale-name collision
and missing-index cases. This is CEP-only and does not require another AEX build or
replacement. AE 2023 still needs to confirm the real property mapping and expression
capability in the owner's project.
