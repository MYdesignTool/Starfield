# Removes every trace this project left outside its own repository, in one pass, and prints
# what it found and did so the result can be audited or reversed.
#
#   * moves any StarfieldParticle.aex out of the After Effects plug-in folders into
#     artifacts/disabled/ (kept, not deleted)
#   * removes CEP caches (%TEMP%\cep_cache, %LOCALAPPDATA%\Temp\cep_cache)
#   * renames %APPDATA%\Adobe\CEP to CEP.bak-<timestamp> so CEP rebuilds its per-user state
#     (nothing is deleted, the folder is only renamed)
#   * stops stray CEP host processes, but only when After Effects is not running
#
# Run:  powershell -ExecutionPolicy Bypass -File artifacts\cleanup_cep.ps1

$ErrorActionPreference = 'Continue'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$hold = Join-Path $repoRoot 'artifacts\disabled'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'

Write-Host '=== 1. processes ==='
$afterFx = @(Get-Process AfterFX -ErrorAction SilentlyContinue)
Write-Host ("AfterFX running: {0}" -f $afterFx.Count)
foreach ($name in 'CEPHtmlEngine', 'CEPServiceManager', 'AdobeCEPServiceManager') {
    $procs = @(Get-Process -Name $name -ErrorAction SilentlyContinue)
    if ($procs.Count -eq 0) { continue }
    if ($afterFx.Count -gt 0) {
        Write-Host ("{0}: {1} running, NOT stopped because After Effects is open" -f $name, $procs.Count)
    } else {
        $procs | ForEach-Object { Write-Host ("stopping {0} (pid {1})" -f $_.ProcessName, $_.Id); Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue }
    }
}

Write-Host ''
Write-Host '=== 2. our plug-in binary in host folders ==='
New-Item -ItemType Directory -Force -Path $hold | Out-Null
$searchRoots = @(
    'D:\Software\Adobe\Adobe After Effects 2023',
    'C:\Program Files\Adobe\Common\Plug-ins',
    'C:\Program Files (x86)\Common Files\Adobe\Plug-ins'
)
$moved = 0
foreach ($root in $searchRoots) {
    if (-not (Test-Path -LiteralPath $root)) { continue }
    foreach ($file in @(Get-ChildItem -LiteralPath $root -Recurse -Filter 'StarfieldParticle.aex' -ErrorAction SilentlyContinue)) {
        $destination = Join-Path $hold ("{0}-{1}" -f $stamp, $file.Name)
        Write-Host ("found: {0}" -f $file.FullName)
        try {
            Move-Item -LiteralPath $file.FullName -Destination $destination -Force -ErrorAction Stop
            Write-Host ("  moved to: {0}" -f $destination)
            $moved++
        } catch {
            Write-Host ("  COULD NOT MOVE ({0}); remove it manually" -f $_.Exception.Message) -ForegroundColor Yellow
        }
    }
}
if ($moved -eq 0) { Write-Host 'no plug-in binary found in the host folders' }

Write-Host ''
Write-Host '=== 3. CEP caches ==='
foreach ($cache in @((Join-Path $env:TEMP 'cep_cache'), (Join-Path $env:LOCALAPPDATA 'Temp\cep_cache'))) {
    if (Test-Path -LiteralPath $cache) {
        Remove-Item -LiteralPath $cache -Recurse -Force -ErrorAction SilentlyContinue
        Write-Host ("removed: {0} (exists now: {1})" -f $cache, (Test-Path -LiteralPath $cache))
    } else {
        Write-Host ("absent: {0}" -f $cache)
    }
}

Write-Host ''
Write-Host '=== 4. CEP per-user state ==='
$cepPrefs = Join-Path $env:APPDATA 'Adobe\CEP'
if (Test-Path -LiteralPath $cepPrefs) {
    $backup = "$cepPrefs.bak-$stamp"
    try {
        Rename-Item -LiteralPath $cepPrefs -NewName (Split-Path -Leaf $backup) -ErrorAction Stop
        Write-Host ("renamed: {0} -> {1}" -f $cepPrefs, $backup)
    } catch {
        Write-Host ("could not rename CEP state: {0}" -f $_.Exception.Message) -ForegroundColor Yellow
    }
} else {
    Write-Host 'no %APPDATA%\Adobe\CEP folder'
}

Write-Host ''
Write-Host '=== 5. final state ==='
Write-Host ("junction org.starfieldfx.panel present: {0}" -f (Test-Path -LiteralPath (Join-Path $env:APPDATA 'Adobe\CEP\extensions\org.starfieldfx.panel')))
Write-Host ("%APPDATA%\Adobe\CEP present: {0}" -f (Test-Path -LiteralPath $cepPrefs))
foreach ($version in 'CSXS.11', 'CSXS.12') {
    $key = "HKCU:\Software\Adobe\$version"
    if (Test-Path $key) {
        $value = (Get-ItemProperty -Path $key -Name PlayerDebugMode -ErrorAction SilentlyContinue).PlayerDebugMode
        Write-Host ("{0}: PlayerDebugMode='{1}'" -f $version, $value)
    } else {
        Write-Host ("{0}: key absent" -f $version)
    }
}
Write-Host ("disabled binaries kept in: {0}" -f $hold)
Write-Host ''
Write-Host 'Next: start After Effects. If extension panels are still missing, use'
Write-Host 'Window > Workspace > Reset to Saved Layout once; that rebuilds the panel layout'
Write-Host 'without touching your preferences.'
