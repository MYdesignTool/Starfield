# Build and host matrix

Status: M0/M1 Windows x64 build compiles against both the supplied May 2023 SDK and the 26.5 SDK. The user confirmed the corrected M1 shell loads in AE 2023; the exact AE build is not recorded. The initial 8001 version mismatch was caused by PiPL stage bits and is fixed in both artifacts. Render/lifecycle and current-AE qualification remain open.

## Toolchain lock procedure

Use the newest Adobe AE SDK available at each release cut for the primary build. Keep the May 2023 SDK as the baseline API/ABI compile reference for the AE 2023 minimum. SDK archives, sample source, PiPL binaries, and third-party headers stay outside the source repository; only selected version metadata belongs here.

For reproducible project builds, pin exact stable releases (not floating `latest`) at the initial build cut:

| Component | Planned choice | Lock status |
|---|---|---|
| AE SDK primary | After Effects SDK 26.5 (`AfterEffectsSDK_26.5_win`) | Supplied locally; `.aex` build succeeds |
| AE 2023 baseline SDK | May 2023 SDK (`May2023_AfterEffectsSDK`) | Supplied locally; separate `.aex` build succeeds |
| Visual Studio Build Tools | 2026 18.7.8 | Installed; MSBuild build succeeds |
| MSVC toolset / compiler | v145 / 14.51.36231; compiler 19.51.36248 | Locked for current Windows build |
| Windows SDK | 10.0.26100.0 | Installed and selected |
| CMake | Optional; latest stable at core-build cut | Not required for AE module build |
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
