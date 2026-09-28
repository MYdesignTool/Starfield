# Build and host matrix

## Current build policy (owner direction, 2026-09-27)

AE 2023 on Windows x64 is the only current target. The default script and direct
MSBuild project use the May 2023 SDK and write under `artifacts/plugin/2023/`.
The four-stage chain (G-03/M3-02) passed 6,188 core checks, the adapter suite
passed 298 fake-host checks, the panel gateway fake-host checks pass, and the
explicit May 2023 SDK build succeeds.
Newer SDK/host adaptation is deferred. The older dual-SDK evidence below is
historical and does not qualify the current binary on newer hosts.

Historical: M0/M1 Windows x64 builds passed against the supplied May 2023 and SDK 26.5 inputs. The user confirmed the corrected M1 shell loads in AE 2023; its exact build is not recorded. The M1-era 8001 version mismatch was corrected. These older artifacts are not the current binary.

Current artifact: `artifacts/plugin/2023/x64/Release/StarfieldParticle.aex`, plug-in build 2 / packed version `0x8002`. Rebuilt on 2026-09-28 with the May 2023 SDK; `dist/StarfieldParticle.aex` has identical bytes. SHA-256: `8135BB08CB0F614C9A999C7C86C040FB5DF6164F0E0173C0B1BB623008DCCECC`. Core suite: 6,188 checks; adapter fake-host suite: 298 checks; panel gateway fake-host checks pass. This exact candidate has not been installed or exercised in AE. Earlier M1/M2 host evidence does not qualify it; the 24-active-parameter build (21 controls plus graph, control source and capture) still needs an AE 2023 host pass.

## Artifact layout

`artifacts/` is Git-ignored and holds only generated files. Every entry is either reproducible from a
command in this file or an externally produced input that must not be edited.

| Path | Contents | Reproduce with |
|---|---|---|
| `plugin/2023/` | Current AE 2023 build: `x64/Release/StarfieldParticle.aex` plus `x64/Release/StarfieldParticle.pdb` and the intermediates under `obj/` | `powershell -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1` |
| `../dist/` | One plainly named installable copy of the current build: `dist/StarfieldParticle.aex` (+ `.pdb`). MSBuild has to keep one directory per SDK target, this is the file a person picks up | Same build; the script publishes it automatically |
| `disabled/` | Rollback binaries, renamed with a timestamp, for example `20260927-232930-StarfieldParticle.aex` (build before the force/appearance controls shipped) | Previous build, kept on purpose |
| `core-tests/`, `adapter-tests/` | Test executables and their object files, so the suites link incrementally: `artifacts/core-tests/core_tests.exe` (core) and `artifacts/adapter-tests/core_tests.exe` (`-Adapter`, fake host) | `powershell -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1 [-Adapter]` |
| `reports/` | Captured readouts and parameter dumps, for example `dump_report.txt` from the scripting-DOM dump | `tools/dump_effect_parameters.jsx` inside After Effects |
| `crash/` | Windows minidumps kept for triage; the analysis tools are `tools/scan_dump.ps1`, `tools/scan2.ps1`, `tools/dump_strings.ps1` | After Effects crash |
| `reference/` | Externally produced reference data, for example `stardust_effect_parameters.txt` from the installed reference product | Reference product, not this repository |

Obsolete build trees (the M1 `m1/` label and the pre-2023 `plugin/x64`, `plugin/obj` targets) were
removed on 2026-09-27; nothing referenced them and the current script always writes `plugin/2023/`.

Installing and rolling back is one command, and it is the owner's to run: `tools/Install-Plugin.ps1`
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

`tests/core_tests.cpp` covers rational-time normalization and overflow, settings validation, simulation determinism and boundaries, graph identities/schema/type/cardinality/cycle validation, sequence codec round-trips and malformed-input rejection, ROI equality against full-frame rendering, world-to-pixel mapping at a downsampled frame grid, source compositing and placement, bit-depth output, the force/appearance chain (closed-form gravity/drag against the analytic solution, age curves, stage-order and single-appearance enforcement, codec round-trip of a four-stage graph), and the bounded-work/cancellation paths.

`tests/graph_parameter_tests.cpp` (`-Adapter`) covers the arbitrary-data callbacks, parameter registration and mapping, the four-stage chain built from the controls, the supervised edit path (Node Graph rewrite, AE Controls isolation, allocation failure), checkout/checkin bookkeeping, and cancellation during host-world copies.

`tests/panel_gateway_tests.js` runs the ExtendScript protocol gateway in a Node fake host and covers multidimensional animation rejection, successful writes, and failed-batch rollback. Run it with `node tests/panel_gateway_tests.js`. This does not replace the AE 2023 panel qualification gate.

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
| Runtime dependencies | None in initial core/plugin shell | Selected |
| PiPL/resource tools | PiPLTool shipped in each selected local Adobe SDK | Both SDK resource pipelines build successfully |

## Release host/architecture matrix

| Host | Windows x64 | Windows ARM64 | macOS Intel + Apple Silicon |
|---|---|---|---|
| AE 2023 (record exact 23.x build) | Current qualification target | Deferred | Deferred |
| Newer AE families | Deferred by owner direction | Deferred | Deferred |

Only mark a cell supported after installing/loading the signed test artifact, applying it to a project, rendering, and exercising save/reopen. A successful compile is not host compatibility evidence.

## Local SDK inputs

The SDK is provided by the developer from Adobe Developer Console. Configure a local SDK path (do not commit SDK files). The plug-in build consumes Adobe headers and the official PiPL conversion tools from that tree. `ae_plugin/BuildWindows.ps1` maps the checkout to a temporary drive during the build to avoid legacy PiPL tool failures on paths containing spaces or non-ASCII characters. We do not provide placeholder SDK headers or a replacement PiPL compiler.
