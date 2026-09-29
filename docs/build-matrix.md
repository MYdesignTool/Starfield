# Build and host matrix

## Current build policy (owner direction, 2026-09-27)

AE 2023 on Windows x64 is the only current target. The default script and direct
MSBuild project use the May 2023 SDK and write under `artifacts/plugin/2023/`.
The four-stage chain (G-03/M3-02) passed 6,196 core checks, the adapter suite
passed 395 fake-host checks, the panel gateway/startup fake-host checks pass, and the
explicit May 2023 SDK build succeeds. All four suites were re-run against this working
tree on 2026-09-28: 6,196 / 395 / gateway / startup, zero failures.
Newer SDK/host adaptation is deferred. The older dual-SDK evidence below is
historical and does not qualify the current binary on newer hosts.

Historical: M0/M1 Windows x64 builds passed against the supplied May 2023 and SDK 26.5 inputs. The user confirmed the corrected M1 shell loads in AE 2023; its exact build is not recorded. The M1-era 8001 version mismatch was corrected. These older artifacts are not the current binary.

Superseded monolithic artifact: plug-in build 2 / packed version `0x8002`,
184,832 bytes, SHA-256 `C0830F649A149990942B40531E25E23C0841FA6BED9C9805B9D3E233AB1ABCAB`.
It is backed up under `artifacts/disabled/h01-ae2023-crt-before-20260928`.
AE 2023.5.0 Build 52 visually confirmed transparent particle output on the preceding
candidate `D22D43BAD15C5173867907369B2EF3293A3FD601C308158665A1F3FD0AB0816B`.

H-01 AE 2023.5.0 Build 52 host candidate: installed `StarfieldParticle.aex`
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

## Artifact layout

`artifacts/` is Git-ignored and holds only generated files. Every entry is either reproducible from a
command in this file or an externally produced input that must not be edited.

| Path | Contents | Reproduce with |
|---|---|---|
| `plugin/2023/` | Current AE 2023 build: `x64/Release/StarfieldParticle.aex` plus `x64/Release/StarfieldParticle.pdb` and the intermediates under `obj/` | `powershell -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1` |
| `core-dll/2023/` | Separately built `StarfieldCore.dll` and intermediates | `powershell -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -CoreOnly` |
| `runtime/` | Content-addressed `StarfieldCore-<SHA16>.dll` files and atomic `current.txt` selection for manual hot reload | Either full or `-CoreOnly` build |
| `loader-tests/` | Loader harness and its workspace-local simulated runtime directory | Build `tests/CoreLoaderTests.vcxproj`, then run it with `dist/StarfieldCore.dll` and `dist/StarfieldParticle.aex` |
| `../dist/` | Paired installation files: `dist/StarfieldParticle.aex` and `dist/StarfieldCore.dll` with matching PDBs | Full build; `-CoreOnly` intentionally does not update the release pair |
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

`tests/panel_gateway_tests.js` runs the ExtendScript protocol gateway in a Node fake host and covers multidimensional animation rejection, successful writes, and failed-batch rollback. `tests/panel_startup_tests.js` runs the CEP client in a fake DOM/host and confirms transient startup `no_target` recovers without a manual Refresh. Run these with `node tests/panel_gateway_tests.js` and `node tests/panel_startup_tests.js`. Neither replaces the AE 2023 panel qualification gate.

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

## Local SDK inputs

The SDK is provided by the developer from Adobe Developer Console. Configure a local SDK path (do not commit SDK files). The plug-in build consumes Adobe headers and the official PiPL conversion tools from that tree. `ae_plugin/BuildWindows.ps1` maps the checkout to a temporary drive during the build to avoid legacy PiPL tool failures on paths containing spaces or non-ASCII characters. We do not provide placeholder SDK headers or a replacement PiPL compiler.
