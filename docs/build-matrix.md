# Build and host matrix

## Build 13 P-02G candidate — 2026-10-02

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
[Deployment status and rollback](native-node-checkpoint.md).

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
