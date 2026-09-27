# AE adapter (M1)

This is the first native effect shell. It has its own match name, exports one effect entry point, responds to core lifecycle selectors, and passes the input through with the AE copy callback. It deliberately declares no MFR/threaded-rendering flag and does not yet register particle controls or persist graph data.

The Windows x64 MSBuild project uses the local Adobe SDK 26.5 by default and follows Adobe's PiPL resource conversion pipeline. It writes build outputs under `artifacts/m1/`; override `STARFIELD_AE_SDK_ROOT` to build against another local SDK. The effect declares only the 8/16bpc behavior implemented by this pass-through shell, and deliberately does not advertise MFR, SmartFX, or float rendering.

From the repository root, build with `powershell -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1`. The script maps the workspace to a temporary drive letter because Adobe's legacy PiPL toolchain does not reliably parse paths containing spaces or non-ASCII characters. To create a separate 2023 SDK build, pass `-SdkPath 'AdobeSDK\May2023_AfterEffectsSDK' -ArtifactLabel 2023`. CMake remains available for the host-independent core; the Windows MSBuild project is the authoritative AE module build because it also runs PiPLTool and compiles its generated resource.
