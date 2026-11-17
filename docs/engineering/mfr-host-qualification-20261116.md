# Native132 MFR: proposed AE2023 host smoke operation

**Superseded workflow:** the owner's latest instruction is offline testing followed
by owner-operated AE testing. This agent-launch proposal was not approved and will
not be executed. Use [the Chinese manual checklist](ae2023-owner-test-checklist-20261116.md)
and [results template](ae2023-test-results-template.md). The commands below remain
a historical, prepared proposal; they are not an instruction to start or quit AE.

Prepared, not executed. Installed129/124 remains unchanged. This list is the exact
scope requiring owner approval under AGENTS.md / ADR0011: temporary activation of
the experimental bundle, one AE2023 launch/script run, normal test-project shutdown,
and restoration of129/124. No process is forcibly stopped. A nonempty/existing
project is refused. No registry, persistent preference, Adobe cache or other
extension is deliberately modified. Ordinary AE startup may perform its own normal
profile/cache writes; the test does not configure or clean them.

## Reviewable candidate

- Frozen bundle: `artifacts/prepared/mfr01-native132-panel124-host-qualification-v1`.
- `candidate.json` SHA256: `A370317C83DD1A581AE1B000A2DFEC7A95BF8BF767AA853344CEA92574E088EE`.
- 11 binaries, unchanged panel124, 223 native build inputs, test sources and exact
  host script/checker/deployment tool are frozen. Default MFR is off; this isolated
  configuration enables bit27 for the nine effect AEX modules only.
- 333 compiled PiPL/actual GLOBAL_SETUP checks pass; 131 disabled configuration
  also passes333. GPU ownership evidence: 24,043 strict/ASan checks; CUDA/OpenCL each
  pass96 parallel render/copy/cancel cases with byte-exact same-driver serial output.

## Authorized action list to request

Run from `D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust` only after approval.
Immediately before activation, read-only `Get-Process AfterFX,AfterFX_64` must find
neither process. Recheck every candidate file hash against the frozen manifest and
the installed129/124 baseline; abort on any change.

1. Temporarily install only the frozen-equivalent native bundle via the existing
   Junction `D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins\Starfield`
   targeting this checkout's `dist`. Preserve the11 replaced binaries, selector and
   deployment record under `artifacts/disabled/mfr01-native132-panel124-host-test-20261116`:

   ```powershell
   pwsh -NoProfile -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'mfr01-native132-panel124-host-test-20261116' -ArtifactLabel '2023-native132-mfr-qualification' -MfrQualificationTest -IncludeModel -IncludeMaterial -IncludeMotion -IncludeTurbulence -Install
   ```

2. Record `[Diagnostics.Stopwatch]::GetTimestamp()` immediately before this exact
   launch, capture the returned process ID, and monitor this process without
   terminating it:

   ```powershell
   $mfrBeginQpc = [Diagnostics.Stopwatch]::GetTimestamp()
   $mfrProcess = Start-Process -WindowStyle Hidden -PassThru -FilePath 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\AfterFX.exe' -ArgumentList '-r','"D:\Project\Code\AE星辰粒子插件Stardust  v1.6.0b\newStardust\tools\mfr_qualification.jsx"'
   ```

   The script creates only a fresh test project and unique
   `artifacts/mfr-01-20261116/host-native132/run-<timestamp>` outputs/report/AEP.
   It tests factory NodeGraph plus animated camera, and legacy animated-rate
   rendering through all eight pass-through nodes; 8/16/32 project depth, CPU and
   CUDA if available, serial/MFR48-frame pairs. GPU choice is a test-project setting.
   `app.setMultiFrameRenderingConfig` affects only the scripted next render and
   reverts when the script returns. The script closes only its own test project,
   then schedules normal `app.quit()` after returning. It does not kill AE or open
   an owner project. If any existing project is encountered it refuses to proceed;
   do not automatically close it or terminate that process.

3. After normal process exit, record `[Diagnostics.Stopwatch]::GetTimestamp()`.
   Add returned PID plus `hostBeginQpc`/`hostEndQpc` to that unique artifact report;
   run the read-only checker with the bundled Python/Pillow runtime:

   ```powershell
   & 'C:\Users\MY\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' -B tools/check_mfr_qualification.py 'artifacts/mfr-01-20261116/host-native132/run-<actual-timestamp>'
   ```

   Require exact decoded PNG pixels for each serial/MFR pair and overlapping main
   render intervals on distinct threads/frame times within this process/run.
   Pre-render overlap alone, thread IDs, a missing warning or old journal entries
   do not qualify. Journal: `artifacts/graph-render-journal-132.jsonl` only.

4. Once read-only process check again proves AE closed, restore129/124 with the
   **one-step undo** below and verify its11 binary hashes, unchanged29 panel files,
   selector and Junction against the frozen129/124 baseline. Retain candidate,
   evidence and backup. If AE remains running, wait for normal exit or ask the
   owner; this authorization list does not include forced termination.

   ```powershell
   pwsh -NoProfile -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'mfr01-native132-panel124-host-test-20261116' -Rollback
   ```

## Limits

This is a controlled smoke gate, not release qualification. PNG comparisons cover
the exported8-bit projection even when project depth is16/32. Full precision/HDR
export, bound native authored graphs, Light/Motion/texture/Model/Material resource
families, UI edits/cancellation and save/reopen still require acceptance before
default MFR or a supported release. The prior blanket assumption that all AEGP
reads must be UI-only is too strong: the May2023 Resizer sample opts into MFR while
reading camera/transform/stream/comp/light information from Render. That evidence
does not qualify every SmartFX/resource query in this plug-in.

Contracts: [Adobe script command line](https://helpx.adobe.com/after-effects/desktop/automate-in-after-effects/automate-animation/scripts.html),
[scoped MFR scripting API](https://ae-scripting.docsforadobe.dev/general/application/#appsetmultiframerenderingconfig),
local May2023 SDK Resizer sample and [ADR0050](../adr/0050-multiframe-rendering.md).
