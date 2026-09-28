# AE 2023 developer deployment for ADR 0012. Default mode only reports paths.
# Host writes require the owner's separate, exact ADR 0011 authorization.
param(
    [switch]$Install,
    [switch]$Rollback,
    [ValidatePattern('^[A-Za-z0-9._-]+$')][string]$BackupName = '',
    [Parameter(Mandatory=$true)][string]$PluginDir
)

$ErrorActionPreference = 'Stop'
if ($Install -and $Rollback) { throw 'Choose exactly one action.' }

$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$pluginDirPath = (Resolve-Path -LiteralPath $PluginDir).Path
$sourceAex = Join-Path $root 'dist\StarfieldParticle.aex'
$sourceCore = Join-Path $root 'dist\StarfieldCore.dll'
$sourceAexPdb = Join-Path $root 'dist\StarfieldParticle.pdb'
$sourceCorePdb = Join-Path $root 'dist\StarfieldCore.pdb'
$runtimeSource = Join-Path $root 'artifacts\runtime'
$targetAex = Join-Path $pluginDirPath 'StarfieldParticle.aex'
$targetCore = Join-Path $pluginDirPath 'StarfieldCore.dll'
$targetAexPdb = Join-Path $pluginDirPath 'StarfieldParticle.pdb'
$targetCorePdb = Join-Path $pluginDirPath 'StarfieldCore.pdb'
$targetLink = Join-Path $pluginDirPath 'StarfieldRuntime'
$disabledDir = Join-Path $root 'artifacts\disabled'
$recordPath = Join-Path $disabledDir 'hotcore-install.json'

Write-Host "AEX: $sourceAex -> $targetAex"
Write-Host "DLL: $sourceCore -> $targetCore"
Write-Host "Symbols: $sourceAexPdb -> $targetAexPdb"
Write-Host "Symbols: $sourceCorePdb -> $targetCorePdb"
Write-Host "Development junction: $targetLink -> $runtimeSource"
Write-Host "Rollback record: $recordPath"
if ($BackupName) { Write-Host "Prior files backup: $(Join-Path $disabledDir $BackupName)" }
if (-not $Install -and -not $Rollback) {
    Write-Host 'Read-only report. Pass -Install or -Rollback after the host change is authorized.'
    exit 0
}
$workspaceSimulation = $pluginDirPath.StartsWith(
    (Join-Path $root 'artifacts\deploy-test\'), [StringComparison]::OrdinalIgnoreCase)
if (-not $workspaceSimulation -and (Get-Process -Name AfterFX, AfterFX_64 -ErrorAction SilentlyContinue)) {
    throw 'Close After Effects before changing its loaded plug-in files.'
}

function Remove-OurJunction {
    if (-not (Test-Path -LiteralPath $targetLink)) { return }
    $item = Get-Item -LiteralPath $targetLink -Force
    if ($item.LinkType -ne 'Junction' -or
        [IO.Path]::GetFullPath([string]$item.Target).TrimEnd('\') -ne
        [IO.Path]::GetFullPath($runtimeSource).TrimEnd('\')) {
        throw "Refusing to remove an unexpected runtime path: $targetLink"
    }
    # Nonrecursive: Directory.Delete removes the junction itself, not its target.
    [IO.Directory]::Delete($targetLink, $false)
}

function Assert-OurJunction {
    if (-not (Test-Path -LiteralPath $targetLink)) {
        throw "Expected runtime junction is missing: $targetLink"
    }
    $item = Get-Item -LiteralPath $targetLink -Force
    if ($item.LinkType -ne 'Junction' -or
        [IO.Path]::GetFullPath([string]$item.Target).TrimEnd('\') -ne
        [IO.Path]::GetFullPath($runtimeSource).TrimEnd('\')) {
        throw "Unexpected runtime path: $targetLink"
    }
}

$names = @('StarfieldParticle.aex', 'StarfieldCore.dll',
           'StarfieldParticle.pdb', 'StarfieldCore.pdb')

if ($Rollback) {
    if (-not (Test-Path -LiteralPath $recordPath)) { throw "No rollback record: $recordPath" }
    $record = Get-Content -LiteralPath $recordPath -Raw | ConvertFrom-Json
    if ($record.pluginDir -ne $pluginDirPath) { throw 'Rollback target differs from installation record.' }
    if ($record.linkCreated) { Assert-OurJunction }
    New-Item -ItemType Directory -Force -Path $disabledDir | Out-Null
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmssfff'
    foreach ($name in $names) {
        $target = Join-Path $pluginDirPath $name
        if (Test-Path -LiteralPath $target) {
            Move-Item -LiteralPath $target -Destination (Join-Path $disabledDir "$stamp-current-$name")
        }
    }
    if ($record.linkCreated) { Remove-OurJunction }
    foreach ($name in $names) {
        $previous = Join-Path $record.backupDir $name
        if (Test-Path -LiteralPath $previous) {
            Move-Item -LiteralPath $previous -Destination (Join-Path $pluginDirPath $name)
        }
    }
    Move-Item -LiteralPath $recordPath -Destination (Join-Path $disabledDir "$stamp-hotcore-install.json")
    Write-Host 'Previous plug-in files restored. Runtime junction removed.' -ForegroundColor Green
    exit 0
}

if (Test-Path -LiteralPath $recordPath) { throw 'Previous hot-core installation is still active; roll it back first.' }
foreach ($required in @($sourceAex, $sourceCore, $sourceAexPdb, $sourceCorePdb,
                        (Join-Path $runtimeSource 'current.txt'))) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Missing build artifact: $required" }
}
if (Test-Path -LiteralPath $targetLink) { throw "Runtime path already exists: $targetLink" }

$stamp = Get-Date -Format 'yyyyMMdd-HHmmssfff'
$backupDir = Join-Path $disabledDir $(if ($BackupName) { $BackupName } else { "hotcore-$stamp" })
if (Test-Path -LiteralPath $backupDir) { throw "Backup directory already exists: $backupDir" }
New-Item -ItemType Directory -Force -Path $backupDir | Out-Null
try {
    foreach ($name in $names) {
        $target = Join-Path $pluginDirPath $name
        if (Test-Path -LiteralPath $target) {
            Move-Item -LiteralPath $target -Destination (Join-Path $backupDir $name)
        }
    }
    Copy-Item -LiteralPath $sourceAex -Destination $targetAex
    Copy-Item -LiteralPath $sourceCore -Destination $targetCore
    Copy-Item -LiteralPath $sourceAexPdb -Destination $targetAexPdb
    Copy-Item -LiteralPath $sourceCorePdb -Destination $targetCorePdb
    New-Item -ItemType Junction -Path $targetLink -Target $runtimeSource | Out-Null
    $record = [ordered]@{
        pluginDir = $pluginDirPath
        backupDir = $backupDir
        linkCreated = $true
        aexSha256 = (Get-FileHash -LiteralPath $targetAex -Algorithm SHA256).Hash
        coreSha256 = (Get-FileHash -LiteralPath $targetCore -Algorithm SHA256).Hash
    }
    $record | ConvertTo-Json | Set-Content -LiteralPath $recordPath -Encoding UTF8
    Write-Host "Installed AEX SHA-256: $($record.aexSha256)"
    Write-Host "Installed DLL SHA-256: $($record.coreSha256)"
    Write-Host "One-step undo: powershell -ExecutionPolicy Bypass -File tools\Deploy-HotCore.ps1 -Rollback -PluginDir '$pluginDirPath'" -ForegroundColor Green
} catch {
    if (Test-Path -LiteralPath $targetLink) { Remove-OurJunction }
    foreach ($name in $names) {
        $target = Join-Path $pluginDirPath $name
        $previous = Join-Path $backupDir $name
        if (Test-Path -LiteralPath $target) {
            Move-Item -LiteralPath $target -Destination (Join-Path $disabledDir "$stamp-failed-$name")
        }
        if (Test-Path -LiteralPath $previous) { Move-Item -LiteralPath $previous -Destination $target }
    }
    throw
}
