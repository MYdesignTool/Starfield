# Build and host matrix

Status: M0/M1 Windows x64 build compiles against both the supplied May 2023 SDK and the 26.5 SDK. The user confirmed the corrected M1 shell loads in AE 2023; the exact AE build is not recorded. The initial 8001 version mismatch was caused by PiPL stage bits and is fixed in both artifacts. Render/lifecycle and current-AE qualification remain open.

M2/G-01/G-02 status: the SmartFX particle build compiles clean against both SDKs (`artifacts/plugin/x64/Release/StarfieldParticle.aex` and `artifacts/plugin/2023/x64/Release/StarfieldParticle.aex`), exports exactly `EffectMain` and `PluginDataEntryFunction2`, and the host-independent core passes 3,828 self-test checks. AE 2023 load/render evidence applies to an earlier eight-control build only; the current 13-control M3-01 build still needs host confirmation, including the Options readout, playback, and preview-resolution geometry.

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

`tests/core_tests.cpp` covers rational-time normalization and overflow, settings validation, simulation determinism and boundaries, graph identities/schema/type/cardinality/cycle validation, sequence codec round-trips and malformed-input rejection, ROI equality against full-frame rendering, world-to-pixel mapping at a downsampled frame grid, source compositing and placement, bit-depth output, and the bounded-work/cancellation paths.

- With CMake available: `cmake -S . -B build && cmake --build build && ctest --test-dir build`.
- On the current Windows machine CMake is not installed, so use `powershell -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1`, which compiles the same sources with the locked MSVC toolset behind a mapped drive letter and runs the executable. Output lands in `artifacts/core-tests/`.

Both paths compile only the host-independent core; the `.aex` itself is still built by `ae_plugin/BuildWindows.ps1`.

## Toolchain lock procedure

Use the newest Adobe AE SDK available at each release cut for the primary build. Keep the May 2023 SDK as the baseline API/ABI compile reference for the AE 2023 minimum. SDK archives, sample source, PiPL binaries, and third-party headers stay outside the source repository; only selected version metadata belongs here.

The May 2023 SDK's `Examples/Util/Param_Utils.h` still calls `strncpy`, which `/sdl` (enabled by `SDLCheck`) escalates to an error, so the project defines `_CRT_SECURE_NO_WARNINGS`. Owned code does not use `strncpy`; the definition exists only to keep the baseline SDK headers compiling.

For reproducible project builds, pin exact stable releases (not floating `latest`) at the initial build cut:

| Component | Planned choice | Lock status |
|---|---|---|
| AE SDK primary | After Effects SDK 26.5 (`AfterEffectsSDK_26.5_win`) | Supplied locally; `.aex` build succeeds |
| AE 2023 baseline SDK | May 2023 SDK (`May2023_AfterEffectsSDK`) | Supplied locally; separate `.aex` build succeeds |
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
| AE 23.0 minimum | Required | After host/SDK availability check | Later qualification |
| Latest maintained 23.x–25.x | Required while claimed | After host/SDK availability check | Later qualification |
| Latest stable 26.x | Required | Beta/stable availability tracked separately | Later qualification |

Only mark a cell supported after installing/loading the signed test artifact, applying it to a project, rendering, and exercising save/reopen. A successful compile is not host compatibility evidence.

## Local SDK inputs

The SDK is provided by the developer from Adobe Developer Console. Configure a local SDK path (do not commit SDK files). The plug-in build consumes Adobe headers and the official PiPL conversion tools from that tree. `ae_plugin/BuildWindows.ps1` maps the checkout to a temporary drive during the build to avoid legacy PiPL tool failures on paths containing spaces or non-ASCII characters. We do not provide placeholder SDK headers or a replacement PiPL compiler.
