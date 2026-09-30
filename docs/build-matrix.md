# Build and host matrix

## Current build policy (owner direction, 2026-09-27)

AE 2023 on Windows x64 is the only current target. The default script and direct
MSBuild project use the May 2023 SDK and write under `artifacts/plugin/2023/`.
The current candidate passes 11,909 core checks, 678 adapter fake-host checks, and seven
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
guard clears when the native effect reappears. This is CEP-only source and does
not require replacing the installed AEX. The focused graph transaction suite
passes with a regression that simulates graph-only undo, verifies no second
submit, then verifies effect restoration clears the conflict. Since the guard is
session-local, save/reopen while graph and native effects disagree still needs a
host test.
