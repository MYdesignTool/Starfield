# AE adapter (M2/M3-01/M3-01B)

The adapter registers 24 render controls plus graph data, source selection and explicit
capture controls (27 active non-input parameters), declares SmartFX, and renders immutable
emitter/force/appearance/output snapshots through the versioned C ABI in
`StarfieldCore.dll`, which owns the deterministic CPU renderer. Legacy mode preserves animated slider values. The effect writes particles
over transparent black and does not composite the input layer's pixels. Host types stop at this
directory: `Parameters.*` converts AE controls into core settings, `WorldBridge.*` converts the AE
output world to and from owned core buffers,
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
| `Diagnostics.*` | Options-button manual core reload plus host-value and recent-render-geometry readout |
| `CoreLoader.*` | Runtime DLL selection, ABI validation, immutable generation leases and failed-reload fallback |
| `PluginFlags.h` | Single source for the PiPL and runtime out-flags |
| `PluginVersion.h` | Single source for the PiPL and runtime version |
| `StarfieldPiPL.r` | PiPL resource; flags and version are included from the two headers above |
| `BuildPiPL.ps1` | Runs Adobe's PiPL pipeline and fails the build if the generated resource disagrees with those headers |
| `Starfield.vcxproj` | Windows x64 MSBuild project for the effect module |
| `StarfieldCore.vcxproj` | Independently buildable Windows x64 particle core DLL |

The effect advertises `PF_OutFlag2_SUPPORTS_SMART_RENDER`,
`PF_OutFlag2_REVEALS_ZERO_ALPHA`, and
`PF_OutFlag2_FLOAT_COLOR_AWARE` and `PF_OutFlag2_I_MIX_GUID_DEPENDENCIES`
together with `PF_OutFlag_DEEP_COLOR_AWARE`,
`PF_OutFlag_PIX_INDEPENDENT`, `PF_OutFlag_USE_OUTPUT_EXTENT`, and
`PF_OutFlag_I_DO_DIALOG` (the diagnostic readout). The zero-alpha flag keeps AE from trimming
transparent input pixels before this source-independent particle render. It deliberately does **not** advertise
MFR/threaded rendering, GPU or Compute Cache. Graph data is a hidden AE arbitrary parameter,
not sequence data. The corrected 24-parameter candidate was loaded in AE 2023.5.0 Build 52;
the owner reports that emitter placement now looks correct at Full and Quarter preview.
The owner reports 8/16/32-bpc rendering and time consistency. On 2026-09-28, AE 2023.5.0 Build 52
confirmed candidate `D22D43BAD15C5173867907369B2EF3293A3FD601C308158665A1F3FD0AB0816B` renders particles
over the transparency grid, with no solid input pixels left in the output. The
current H-01 split build was loaded in AE 2023.5.0 Build 52: its Full/Quarter
preview updated on manual Core reload without restarting AE, and a missing
Core kept the prior generation. Visual 8/16/32-bpc output, transparency, and
save/close/reopen passed. The two modules use `/MT` in Release to avoid AE's
old app-local MSVC runtime. In-flight AE render switching and exact pixel
comparison to the monolith remain open. The CEP panel populated its form after the project and effect layer
were selected, without a manual Refresh. Effect-state retention after save/reopen,
duplicate, undo/redo, and remaining panel checks have the evidence boundary recorded in
`docs/compatibility-matrix.md`. The Options geometry belongs
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
They also write `artifacts/core-dll/2023/x64/Release/StarfieldCore.dll`
and publish the matched pair into `dist/`. For frequent renderer changes use
`powershell -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -CoreOnly`.
That command leaves `StarfieldParticle.aex` untouched and atomically selects a
content-addressed DLL in `dist/StarfieldRuntime/current.txt`. With the explicitly
authorized developer junction installed, click the effect's **Options** button
to load the selected generation and request a refreshed AE frame. The previous
generation remains usable until its in-flight render leases and owned pixel
results are released. The developer installation is one junction from
`Plug-ins/Starfield` to `dist`; the runtime is a real subdirectory within it.
Full builds refuse AEX publication while AE runs. Candidate builds use
`-NoRuntimePublish -NoDistPublish`; `tools/Deploy-TestBuild.ps1` reports unless
`-Install` or `-Rollback` is explicitly passed after authorization.
Current qualification targets AE 2023 only; newer-host adaptation is deferred.

CMake builds the host-independent core, shared core ABI, and self-tests
(`tests/core_tests.cpp`; on a machine without CMake use
`powershell -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1`). The Windows MSBuild
project remains the authoritative AE module build because it also runs PiPLTool and
compiles the generated resource.
