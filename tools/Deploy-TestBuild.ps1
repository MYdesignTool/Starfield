# Deploy the paired native-node test build. Default mode reports; no registry,
# preferences, CEP switches or process operations. Existing runtime junction only.
param(
    [Parameter(Mandatory=$true)][string]$PluginDir,
    [ValidatePattern('^[A-Za-z0-9._-]+$')][string]$BackupName = 'p02d-native-records-20261001',
    [switch]$Install,
    [switch]$Rollback
)
$ErrorActionPreference = 'Stop'
if ($Install -and $Rollback) { throw 'Choose one action.' }
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$destination = (Resolve-Path -LiteralPath $PluginDir).Path
$backup = Join-Path $repo "artifacts\disabled\$BackupName"
$runtime = Join-Path $repo 'artifacts\runtime'
$selector = Join-Path $runtime 'current.txt'
$names = @('StarfieldParticle.aex', 'StarfieldEmitter.aex', 'StarfieldParticleNode.aex',
           'StarfieldAppearance.aex', 'StarfieldForce.aex', 'StarfieldCore.dll')
$sources = @{}
foreach ($name in $names) {
    $subdir = if ($name -eq 'StarfieldCore.dll') { 'core-dll' } else { 'plugin' }
    $sources[$name] = Join-Path $repo "artifacts\$subdir\2023\x64\Release\$name"
    Write-Host "$($sources[$name]) -> $(Join-Path $destination $name)"
}
Write-Host "Backup and undo record: $backup"
Write-Host "Runtime selector: $selector"
if (-not $Install -and -not $Rollback) { Write-Host 'Read-only report; pass -Install or -Rollback.'; exit 0 }
if (Get-Process AfterFX, AfterFX_64 -ErrorAction SilentlyContinue) { throw 'Close AE before deployment or rollback.' }
$link = Get-Item -LiteralPath (Join-Path $destination 'StarfieldRuntime') -Force
if ($link.LinkType -ne 'Junction' -or
    [IO.Path]::GetFullPath([string]$link.Target).TrimEnd('\') -ne [IO.Path]::GetFullPath($runtime).TrimEnd('\')) {
    throw 'StarfieldRuntime must be the existing junction to this checkout; no junction is changed.'
}
$recordPath = Join-Path $backup 'deployment.json'
if ($Rollback) {
    $record = Get-Content -LiteralPath $recordPath -Raw | ConvertFrom-Json
    if ($record.pluginDir -ne $destination) { throw 'Undo target differs from recorded installation.' }
    foreach ($entry in $record.files) {
        $target = Join-Path $destination $entry.name
        if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $entry.installedHash) {
            throw "Installed file changed after deployment: $target"
        }
    }
    $archive = Join-Path $backup 'candidate'
    if (Test-Path -LiteralPath $archive) { throw 'This deployment has already been rolled back.' }
    New-Item -ItemType Directory -Path $archive | Out-Null
    foreach ($entry in $record.files) {
        $target = Join-Path $destination $entry.name
        Move-Item -LiteralPath $target -Destination (Join-Path $archive $entry.name)
        if ($entry.existed) { Copy-Item -LiteralPath (Join-Path $backup $entry.name) -Destination $target }
    }
    Copy-Item -LiteralPath (Join-Path $backup 'current.txt') -Destination $selector -Force
    Write-Host 'Prior files and runtime selector restored; candidate files retained.'
    exit 0
}
foreach ($source in $sources.Values) { if (-not (Test-Path -LiteralPath $source)) { throw "Missing candidate: $source" } }
if (Test-Path -LiteralPath $backup) { throw "Backup already exists: $backup" }
New-Item -ItemType Directory -Path $backup -Force | Out-Null
Copy-Item -LiteralPath $selector -Destination (Join-Path $backup 'current.txt')
$record = [ordered]@{ pluginDir=$destination; files=@(); runtimeName='' }
foreach ($name in $names) {
    $target = Join-Path $destination $name
    $record.files += [ordered]@{ name=$name; existed=(Test-Path -LiteralPath $target);
        installedHash=(Get-FileHash -LiteralPath $sources[$name] -Algorithm SHA256).Hash }
}
try {
    foreach ($entry in $record.files) {
        $target = Join-Path $destination $entry.name
        if ($entry.existed) { Move-Item -LiteralPath $target -Destination (Join-Path $backup $entry.name) }
        Copy-Item -LiteralPath $sources[$entry.name] -Destination $target
        if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $entry.installedHash) { throw "Copy verification failed: $target" }
    }
    $coreHash = (Get-FileHash -LiteralPath $sources['StarfieldCore.dll'] -Algorithm SHA256).Hash
    $record.runtimeName = "StarfieldCore-$($coreHash.Substring(0,16)).dll"
    $runtimeCore = Join-Path $runtime $record.runtimeName
    if (Test-Path -LiteralPath $runtimeCore) {
        if ((Get-FileHash -LiteralPath $runtimeCore -Algorithm SHA256).Hash -ne $coreHash) { throw 'Runtime hash collision.' }
    } else { Copy-Item -LiteralPath $sources['StarfieldCore.dll'] -Destination $runtimeCore }
    [IO.File]::WriteAllText($selector, "$($record.runtimeName)`n", [Text.Encoding]::ASCII)
    $record | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $recordPath -Encoding UTF8
    $published = Join-Path $repo 'dist'
    New-Item -ItemType Directory -Path $published -Force | Out-Null
    foreach ($name in $names) { Copy-Item -LiteralPath $sources[$name] -Destination (Join-Path $published $name) -Force }
} catch {
    foreach ($entry in $record.files) {
        $target = Join-Path $destination $entry.name
        $previous = Join-Path $backup $entry.name
        if (Test-Path -LiteralPath $previous) {
            if (Test-Path -LiteralPath $target) { Move-Item -LiteralPath $target -Destination (Join-Path $backup "failed-$($entry.name)") }
            Copy-Item -LiteralPath $previous -Destination $target
        } elseif (-not $entry.existed -and (Test-Path -LiteralPath $target)) {
            Move-Item -LiteralPath $target -Destination (Join-Path $backup "failed-$($entry.name)")
        }
    }
    Copy-Item -LiteralPath (Join-Path $backup 'current.txt') -Destination $selector -Force
    throw
}
foreach ($entry in $record.files) { Write-Host "$($entry.name) SHA256 $($entry.installedHash)" }
Write-Host "Undo: powershell -ExecutionPolicy Bypass -File tools\Deploy-TestBuild.ps1 -PluginDir '$destination' -BackupName '$BackupName' -Rollback"
