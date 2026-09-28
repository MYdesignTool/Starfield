# AE adapter (M2/M3-01)

The adapter registers 21 render controls plus graph data, source selection and explicit
capture controls (24 active non-input parameters), declares SmartFX, and renders immutable
emitter/force/appearance/output snapshots through the
deterministic CPU renderer. Legacy mode preserves animated slider values. It composites
particles over the input layer. Host types stop at this directory: `Parameters.*` converts AE controls into core settings,
`WorldBridge.*` converts checked-out pixel worlds into owned core buffers and back,
`SmartRender.*` runs the pre-render/render pair and maps core errors onto AE errors,
and `EffectMain.cpp` is dispatch plus the global flags.

Files:

| File | Role |
|---|---|
| `EffectMain.cpp` | Entry points, selector dispatch, global setup flags |
| `Parameters.*` | Manifest registration, `PF_CHECKOUT_PARAM` snapshot, and the scoped checkin guard |
| `GraphParameter.*` | Host arbitrary-data ownership, persistence callbacks, graph checkout and explicit control capture |
| `SmartRender.*` | `PF_Cmd_SMART_PRE_RENDER` / `PF_Cmd_SMART_RENDER`, cancellation, error mapping |
| `WorldBridge.*` | Host world ↔ core buffer conversion (8/16/32 bpc, rowbytes, alpha) plus shared host geometry helpers |
| `Diagnostics.*` | Options-button readout of current host values and the process-global most recent render geometry (read-only support tool) |
| `PluginFlags.h` | Single source for the PiPL and runtime out-flags |
| `PluginVersion.h` | Single source for the PiPL and runtime version |
| `StarfieldPiPL.r` | PiPL resource; flags and version are included from the two headers above |
| `BuildPiPL.ps1` | Runs Adobe's PiPL pipeline and fails the build if the generated resource disagrees with those headers |
| `Starfield.vcxproj` | Windows x64 MSBuild project for the effect module |

The effect advertises `PF_OutFlag2_SUPPORTS_SMART_RENDER` and
`PF_OutFlag2_FLOAT_COLOR_AWARE` together with `PF_OutFlag_DEEP_COLOR_AWARE`,
`PF_OutFlag_PIX_INDEPENDENT`, `PF_OutFlag_USE_OUTPUT_EXTENT`, and
`PF_OutFlag_I_DO_DIALOG` (the diagnostic readout). It deliberately does **not** advertise
MFR/threaded rendering, GPU or Compute Cache. Graph data is a hidden AE arbitrary parameter,
not sequence data. The corrected 24-parameter candidate was loaded in AE 2023.5.0 Build 52;
the owner reports that emitter placement now looks correct at Full and Quarter preview.
Source compositing, 8/16/32-bpc output, effect-state retention after save/reopen, duplicate,
undo/redo, and panel connection still need host confirmation. The Options geometry belongs
to whichever effect instance rendered most recently, so it may not describe the current
comp. See `docs/compatibility-matrix.md` for the exact evidence boundary.

The Windows x64 MSBuild project uses the local May 2023 Adobe SDK by default and follows
Adobe's PiPL resource conversion pipeline. It writes build outputs under `artifacts/plugin/`;
override `STARFIELD_AE_SDK_ROOT` to build against another local SDK.

From the repository root, build with `powershell -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1`.
The script maps the workspace to a temporary drive letter because Adobe's legacy PiPL
toolchain does not reliably parse paths containing spaces or non-ASCII characters. To
spell out the defaults, pass `-SdkPath 'AdobeSDK\May2023_AfterEffectsSDK' -ArtifactLabel 2023`.
Both commands write `artifacts/plugin/2023/x64/Release/StarfieldParticle.aex`.
Current qualification targets AE 2023 only; newer-host adaptation is deferred.

CMake builds only the host-independent core plus its self-tests
(`tests/core_tests.cpp`; on a machine without CMake use
`powershell -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1`). The Windows MSBuild
project remains the authoritative AE module build because it also runs PiPLTool and
compiles the generated resource.
