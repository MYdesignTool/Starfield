# Report by default. Starting AE is a separate ADR0011 host action; the caller
# must obtain the owner's exact-command authorization before using -Start.
param([switch]$Start)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskExe='D:\Software\Adobe\Adobe After Effects 2023\Support Files\AfterFX.exe'
$taskScript=Join-Path $PSScriptRoot 'WebPanelQualification.jsx'
Write-Host ('Start-Process -FilePath ''{0}'' -ArgumentList @(''-r'', ''"{1}"'') -WindowStyle Normal -PassThru' -f $taskExe,$taskScript)
Write-Host "Test project/report: $taskRepo\artifacts\webview2\ae-qualification (no existing project is closed)."
Write-Host 'The native panels start WebView2 child processes using the explicit checkout profile. No Runtime installation, registry or shared CEP changes.'
Write-Host 'Undo: close the test AE window normally; no process is force-stopped by this script.'
if(-not $Start){Write-Host 'Read-only report. Add -Start only after exact-action owner authorization.';exit 0}
if(Get-Process AfterFX,AfterFX_64 -ErrorAction SilentlyContinue){throw 'AE already runs; refusing to start another instance or reuse its project.'}
if(-not (Test-Path -LiteralPath $taskExe)){throw 'AE2023 executable not found.'}
$taskBuilt=Join-Path $taskRepo 'artifacts/plugin/2023/x64/Release/StarfieldHost.aex'
$taskInstalled=Join-Path $taskRepo 'dist/StarfieldHost.aex'
if((Get-FileHash -LiteralPath $taskBuilt -Algorithm SHA256).Hash -cne (Get-FileHash -LiteralPath $taskInstalled -Algorithm SHA256).Hash){throw 'Installed native Host differs from the tested candidate.'}
$taskDirectory=Join-Path $taskRepo 'artifacts/webview2/ae-qualification'
New-Item -ItemType Directory -Force -Path $taskDirectory | Out-Null
$taskProcess=Start-Process -FilePath $taskExe -ArgumentList @('-r',('"'+$taskScript+'"')) -WindowStyle Normal -PassThru
[ordered]@{pid=$taskProcess.Id;exe=$taskExe;script=$taskScript;started=(Get-Date -Format o);nativeHostSha256=(Get-FileHash -LiteralPath $taskInstalled -Algorithm SHA256).Hash} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskDirectory 'launch.json') -Encoding UTF8
Write-Host "Started test AE process $($taskProcess.Id). Close it normally when finished; existing CEP is still enabled."
