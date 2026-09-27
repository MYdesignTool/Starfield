# Reports every trace this project left outside its own repository, and changes the host
# only for the actions you name explicitly on the command line.
#
#   report only (default): powershell -ExecutionPolicy Bypass -File tools\cleanup_host_traces.ps1
#   move our plug-in out : ... -MovePluginBinaries   (delegates to tools\Install-Plugin.ps1 -Uninstall)
#   clear CEP caches     : ... -ClearCepCaches
#   stop stray CEP procs : ... -StopCepProcesses     (refused while After Effects runs)
#   rename CEP user state: ... -ResetCepState        (rename only, nothing deleted)
#
# It NEVER writes the registry. The PlayerDebugMode values it prints are read-only and are
# deliberately left alone: that key is host-wide, every other unsigned extension depends on
# it, and clearing it once disabled all of the owner's panels for good (ADR 0011).
#
# The default is a report because this script exists for triage: an agent that is asked to
# "check what the host looks like" must not change it while looking. Every action is a
# separate, explicit switch, and none of them deletes a file.

param(
    [switch]$MovePluginBinaries,
    [switch]$ClearCepCaches,
    [switch]$StopCepProcesses,
    [switch]$ResetCepState
)

$ErrorActionPreference = 'Continue'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$disabledDir = Join-Path $repoRoot 'artifacts\disabled'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$acting = $MovePluginBinaries -or $ClearCepCaches -or $StopCepProcesses -or $ResetCepState
if (-not $acting) {
    Write-Host 'Report only. Pass a switch to change anything (see the header).' -ForegroundColor Cyan
}

Write-Host ''
Write-Host '=== processes ==='
$afterFx = @(Get-Process AfterFX -ErrorAction SilentlyContinue)
Write-Host ("AfterFX running: {0}" -f $afterFx.Count)
foreach ($name in 'CEPHtmlEngine', 'CEPServiceManager', 'AdobeCEPServiceManager') {
    $procs = @(Get-Process -Name $name -ErrorAction SilentlyContinue)
    if ($procs.Count -eq 0) { continue }
    if (-not $StopCepProcesses) {
        Write-Host ("{0}: {1} running (not stopping; pass -StopCepProcesses)" -f $name, $procs.Count)
        continue
    }
    if ($afterFx.Count -gt 0) {
        Write-Host ("{0}: {1} running, NOT stopped because After Effects is open" -f $name, $procs.Count) -ForegroundColor Yellow
        continue
    }
    $procs | ForEach-Object {
        Write-Host ("stopping {0} (pid {1})" -f $_.ProcessName, $_.Id)
        Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
    }
}

Write-Host ''
Write-Host '=== our plug-in binary in host folders ==='
if ($MovePluginBinaries) {
    & powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Install-Plugin.ps1') -Uninstall
} else {
    Write-Host 'not moved; pass -MovePluginBinaries to move it into artifacts\disabled\'
}
foreach ($root in @('C:\Program Files\Adobe\Common\Plug-ins', 'C:\Program Files (x86)\Common Files\Adobe\Plug-ins')) {
    if (-not (Test-Path -LiteralPath $root)) { continue }
    foreach ($file in @(Get-ChildItem -LiteralPath $root -Recurse -Filter 'StarfieldParticle.aex' -ErrorAction SilentlyContinue)) {
        Write-Host ("found in a shared plug-in folder: {0}" -f $file.FullName) -ForegroundColor Yellow
    }
}

Write-Host ''
Write-Host '=== CEP caches ==='
foreach ($cache in @((Join-Path $env:TEMP 'cep_cache'), (Join-Path $env:LOCALAPPDATA 'Temp\cep_cache'))) {
    if (-not (Test-Path -LiteralPath $cache)) {
        Write-Host ("absent: {0}" -f $cache)
        continue
    }
    if (-not $ClearCepCaches) {
        Write-Host ("present (not removed; pass -ClearCepCaches): {0}" -f $cache)
        continue
    }
    Remove-Item -LiteralPath $cache -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host ("removed: {0} (exists now: {1})" -f $cache, (Test-Path -LiteralPath $cache))
}

Write-Host ''
Write-Host '=== CEP per-user state ==='
$cepPrefs = Join-Path $env:APPDATA 'Adobe\CEP'
if (-not (Test-Path -LiteralPath $cepPrefs)) {
    Write-Host 'no %APPDATA%\Adobe\CEP folder'
} elseif (-not $ResetCepState) {
    Write-Host ("present (not renamed; pass -ResetCepState to rename it): {0}" -f $cepPrefs)
} else {
    $backup = "$cepPrefs.bak-$stamp"
    try {
        Rename-Item -LiteralPath $cepPrefs -NewName (Split-Path -Leaf $backup) -ErrorAction Stop
        Write-Host ("renamed: {0} -> {1}" -f $cepPrefs, $backup)
    } catch {
        Write-Host ("could not rename CEP state: {0}" -f $_.Exception.Message) -ForegroundColor Yellow
    }
}

Write-Host ''
Write-Host '=== final state ==='
Write-Host ("%APPDATA%\Adobe\CEP present: {0}" -f (Test-Path -LiteralPath $cepPrefs))
Write-Host 'read-only registry check (nothing above changes it):'
foreach ($version in 'CSXS.11', 'CSXS.12') {
    $key = "HKCU:\Software\Adobe\$version"
    if (Test-Path $key) {
        $value = (Get-ItemProperty -Path $key -Name PlayerDebugMode -ErrorAction SilentlyContinue).PlayerDebugMode
        Write-Host ("  {0}\PlayerDebugMode = '{1}'" -f $version, $value)
    } else {
        Write-Host ("  {0}: key absent" -f $version)
    }
}
Write-Host ("disabled binaries kept in: {0}" -f $disabledDir)
Write-Host ''
Write-Host 'Next: start After Effects. If extension panels are still missing, run'
Write-Host 'Window > Workspace > Reset to Saved Layout once; that rebuilds the panel layout'
Write-Host 'without touching your preferences or the registry.'
