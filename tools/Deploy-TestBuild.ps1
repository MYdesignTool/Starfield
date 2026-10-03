# One development bundle junction. Default mode reports; host mutation is explicit.
param(
    [Parameter(Mandatory=$true)][string]$PluginDir,
    [ValidatePattern('^[A-Za-z0-9._-]+$')][string]$BackupName = 'p02d-build4-single-folder-20261001',
    [switch]$Install,
    [switch]$Rollback
)
$ErrorActionPreference = 'Stop'
if ($Install -and $Rollback) { throw 'Choose one action.' }
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$destination = (Resolve-Path -LiteralPath $PluginDir).Path
$bundle = Join-Path $repo 'dist'
$runtime = Join-Path $bundle 'StarfieldRuntime'
$selector = Join-Path $runtime 'current.txt'
$link = Join-Path $destination 'Starfield'
$oldLink = Join-Path $destination 'StarfieldRuntime'
$oldRuntime = Join-Path $repo 'artifacts\runtime'
$backup = Join-Path $repo "artifacts\disabled\$BackupName"
$recordPath = Join-Path $backup 'deployment.json'
$names = @('StarfieldParticle.aex', 'StarfieldEmitter.aex', 'StarfieldParticleNode.aex',
           'StarfieldForce.aex', 'StarfieldHost.aex', 'StarfieldCore.dll')
$retiredNames = @('StarfieldAppearance.aex')
$allowedNames = @($names) + @($retiredNames)
$rootNames = @($allowedNames) + @($allowedNames | ForEach-Object { [IO.Path]::ChangeExtension($_, '.pdb') })
$sources = @{}
foreach ($name in $names) {
    $kind = if ($name -eq 'StarfieldCore.dll') { 'core-dll' } else { 'plugin' }
    $sources[$name] = Join-Path $repo "artifacts\$kind\2023\x64\Release\$name"
    Write-Host "$($sources[$name]) -> $(Join-Path $bundle $name)"
}
Write-Host "Single AE junction: $link -> $bundle"
Write-Host "Archive loose Starfield binaries and old runtime junction: $backup\host"
Write-Host "Undo: powershell -ExecutionPolicy Bypass -File tools\Deploy-TestBuild.ps1 -PluginDir '$destination' -BackupName '$BackupName' -Rollback"
if (-not $Install -and -not $Rollback) { Write-Host 'Read-only report.'; exit 0 }
$simulation = $destination.StartsWith((Join-Path $repo 'artifacts\deploy-test\'), [StringComparison]::OrdinalIgnoreCase)
if (-not $simulation -and (Get-Process AfterFX, AfterFX_64 -ErrorAction SilentlyContinue)) { throw 'Close AE before replacing an AEX or changing its installation.' }

function Assert-Junction([string]$Path, [string]$Target) {
    $item = Get-Item -LiteralPath $Path -Force
    if ($item.LinkType -ne 'Junction' -or
        [IO.Path]::GetFullPath([string]$item.Target).TrimEnd('\') -ne [IO.Path]::GetFullPath($Target).TrimEnd('\')) {
        throw "Unexpected junction: $Path"
    }
}
function Restore-Deployment($record) {
    if ($record.pluginDir -ne $destination -or $record.bundle -ne $bundle) { throw 'Rollback paths differ from this checkout.' }
    if ($record.linkCreated -and (Test-Path -LiteralPath $link)) {
        Assert-Junction $link $bundle
        # Remove only this validated junction; never recurse into its target.
        [IO.Directory]::Delete($link, $false)
    }
    foreach ($name in $rootNames) {
        $saved = Join-Path $backup "host\$name"
        if (Test-Path -LiteralPath $saved) {
            $target = Join-Path $destination $name
            if (Test-Path -LiteralPath $target) { throw "Unexpected loose file blocks restoration: $target" }
            Move-Item -LiteralPath $saved -Destination $target
        }
    }
    $savedLink = Join-Path $backup 'host\StarfieldRuntime'
    if (Test-Path -LiteralPath $savedLink) {
        Assert-Junction $savedLink $oldRuntime
        if (Test-Path -LiteralPath $oldLink) { throw 'Old runtime path is occupied.' }
        Move-Item -LiteralPath $savedLink -Destination $oldLink
    }
    $archive = Join-Path $backup 'candidate'
    New-Item -ItemType Directory -Path $archive -Force | Out-Null
    foreach ($entry in $record.files) {
        if ($allowedNames -notcontains $entry.name) { throw 'Unknown bundle file in rollback record.' }
        $target = Join-Path $bundle $entry.name
        if (Test-Path -LiteralPath $target) { Move-Item -LiteralPath $target -Destination (Join-Path $archive $entry.name) }
        if ($entry.existed) { Copy-Item -LiteralPath (Join-Path $backup "bundle\$($entry.name)") -Destination $target }
    }
    if (Test-Path -LiteralPath $selector) { Move-Item -LiteralPath $selector -Destination (Join-Path $archive 'current.txt') }
    if ($record.selectorExisted) { Copy-Item -LiteralPath (Join-Path $backup 'bundle\current.txt') -Destination $selector }
}
if ($Rollback) {
    $record = Get-Content -LiteralPath $recordPath -Raw | ConvertFrom-Json
    if (Test-Path -LiteralPath (Join-Path $backup 'rolled-back.txt')) { throw 'Already rolled back.' }
    foreach ($entry in $record.files) {
        if ($allowedNames -notcontains $entry.name) { throw 'Unknown bundle file in rollback record.' }
        $taskTarget = Join-Path $bundle $entry.name
        if (-not $entry.installedHash) {
            if (Test-Path -LiteralPath $taskTarget) { throw "Retired file reappeared after deployment: $taskTarget" }
        } elseif ((Get-FileHash -LiteralPath $taskTarget -Algorithm SHA256).Hash -ne $entry.installedHash) {
            throw "Bundle changed after this deployment: $($entry.name)"
        }
    }
    Restore-Deployment $record
    [IO.File]::WriteAllText((Join-Path $backup 'rolled-back.txt'), 'restored')
    Write-Host 'Previous layout and files restored; candidate retained in backup.'
    exit 0
}
foreach ($source in $sources.Values) { if (-not (Test-Path -LiteralPath $source)) { throw "Missing candidate: $source" } }
if (Test-Path -LiteralPath $backup) { throw "Backup exists: $backup" }
if (Test-Path -LiteralPath $link) { Assert-Junction $link $bundle }
if (Test-Path -LiteralPath $oldLink) { Assert-Junction $oldLink $oldRuntime }
$unknown = Get-ChildItem -LiteralPath $destination -Force | Where-Object {
    $_.Name -like 'Starfield*' -and $_.Name -ne 'Starfield' -and $_.Name -ne 'StarfieldRuntime' -and $rootNames -notcontains $_.Name
}
if ($unknown) { throw "Unexpected Starfield entries: $($unknown.Name -join ', ')" }
if (Test-Path -LiteralPath $runtime) {
    if ((Get-Item -LiteralPath $runtime -Force).LinkType) { throw 'Bundle runtime must be a real subdirectory, not a second junction.' }
}
New-Item -ItemType Directory -Path (Join-Path $backup 'host'), (Join-Path $backup 'bundle'), $bundle, $runtime -Force | Out-Null
$record = [ordered]@{pluginDir=$destination; bundle=$bundle; linkCreated=(-not (Test-Path -LiteralPath $link));
    selectorExisted=(Test-Path -LiteralPath $selector); files=@(); runtimeName=''}
if ($record.selectorExisted) { Copy-Item -LiteralPath $selector -Destination (Join-Path $backup 'bundle\current.txt') }
foreach ($name in $allowedNames) {
    $target = Join-Path $bundle $name
    $exists = Test-Path -LiteralPath $target
    if ($exists) { Copy-Item -LiteralPath $target -Destination (Join-Path $backup "bundle\$name") }
    $hash = if ($names -contains $name) { (Get-FileHash -LiteralPath $sources[$name] -Algorithm SHA256).Hash } else { $null }
    $record.files += [ordered]@{name=$name; existed=$exists; installedHash=$hash}
}
$coreHash = (Get-FileHash -LiteralPath $sources['StarfieldCore.dll'] -Algorithm SHA256).Hash
$record.runtimeName = "StarfieldCore-$($coreHash.Substring(0,16)).dll"
$record | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $recordPath -Encoding UTF8
try {
    foreach ($name in $rootNames) {
        $target = Join-Path $destination $name
        if (Test-Path -LiteralPath $target) { Move-Item -LiteralPath $target -Destination (Join-Path $backup "host\$name") }
    }
    if (Test-Path -LiteralPath $oldLink) { Move-Item -LiteralPath $oldLink -Destination (Join-Path $backup 'host\StarfieldRuntime') }
    foreach ($entry in $record.files) {
        $target = Join-Path $bundle $entry.name
        if (-not $entry.installedHash) {
            if (Test-Path -LiteralPath $target) { Move-Item -LiteralPath $target -Destination (Join-Path $backup $entry.name) }
            continue
        }
        Copy-Item -LiteralPath $sources[$entry.name] -Destination $target -Force
        if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $entry.installedHash) { throw "Hash mismatch: $target" }
    }
    $versioned = Join-Path $runtime $record.runtimeName
    if (Test-Path -LiteralPath $versioned) {
        if ((Get-FileHash -LiteralPath $versioned -Algorithm SHA256).Hash -ne $coreHash) { throw 'Runtime hash collision.' }
    } else { Copy-Item -LiteralPath $sources['StarfieldCore.dll'] -Destination $versioned }
    [IO.File]::WriteAllText($selector, "$($record.runtimeName)`n", [Text.Encoding]::ASCII)
    if ($record.linkCreated) { New-Item -ItemType Junction -Path $link -Target $bundle | Out-Null }
    Assert-Junction $link $bundle
} catch {
    Restore-Deployment $record
    throw
}
foreach ($entry in $record.files) { Write-Host "$($entry.name) SHA256 $($entry.installedHash)" }
Write-Host "Installed. AE plug-in root contains one Starfield junction: $link"
