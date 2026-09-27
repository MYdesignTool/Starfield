# Installs the current build into the local After Effects plug-ins folder, or takes it
# back out. It touches exactly one .aex (plus its .pdb) and nothing else; both directions
# print the files involved and keep whatever they replaced under artifacts/disabled/, so
# the change is reversible in one step.
#
#   install or update:  powershell -ExecutionPolicy Bypass -File tools\Install-Plugin.ps1
#   roll back        :  powershell -ExecutionPolicy Bypass -File tools\Install-Plugin.ps1 -Uninstall
#   explicit targets :  -Source <aex>  -PluginDir "<...>Support Files\Plug-ins"
#
# The plug-ins folder is read from Adobe's per-version registry value
# (HKLM\SOFTWARE\Adobe\After Effects\<version>\PluginInstallPath); pass -PluginDir when
# that value is absent. Writing into Program Files needs an elevated shell.
#
# The owner runs this, not an agent: an install is a host change and ADR 0011 requires
# explicit per-action authorization for those.

param(
    [switch]$Uninstall,
    [string]$Source = '',
    [string]$PluginDir = ''
)

$ErrorActionPreference = 'Stop'

$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$pluginName = 'StarfieldParticle.aex'
$backupDir = Join-Path $repositoryRoot 'artifacts\disabled'

function Get-PluginDirectory {
    if ($PluginDir) {
        if (-not (Test-Path -LiteralPath $PluginDir)) { throw "PluginDir does not exist: $PluginDir" }
        return (Resolve-Path -LiteralPath $PluginDir).Path
    }
    $found = @()
    Get-ChildItem 'HKLM:\SOFTWARE\Adobe\After Effects' -ErrorAction SilentlyContinue |
        Sort-Object PSChildName -Descending | ForEach-Object {
            $value = (Get-ItemProperty -LiteralPath $_.PSPath -ErrorAction SilentlyContinue).PluginInstallPath
            if ($value -and (Test-Path -LiteralPath $value)) { $found += (Resolve-Path -LiteralPath $value).Path }
        }
    if ($found.Count -eq 0) {
        throw 'No After Effects plug-ins folder found; pass -PluginDir "<...>\Support Files\Plug-ins".'
    }
    return $found[0]
}

function Show-File([string]$label, [string]$path) {
    if (-not (Test-Path -LiteralPath $path)) { Write-Host "$label (none)"; return }
    $item = Get-Item -LiteralPath $path
    Write-Host ("{0} {1}  {2} bytes  {3}" -f $label, $item.FullName, $item.Length, $item.LastWriteTime)
}

if (Get-Process -Name AfterFX, AfterFX_64 -ErrorAction SilentlyContinue) {
    throw 'After Effects is running and holds the plug-in file. Close it first.'
}

$target = Join-Path (Get-PluginDirectory) $pluginName
New-Item -ItemType Directory -Force -Path $backupDir | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'

if ($Uninstall) {
    if (-not (Test-Path -LiteralPath $target)) { Write-Host "Nothing installed at $target"; exit 0 }
    $keep = Join-Path $backupDir "$stamp-$pluginName"
    Move-Item -LiteralPath $target -Destination $keep
    $pdb = [IO.Path]::ChangeExtension($target, '.pdb')
    if (Test-Path -LiteralPath $pdb) { Move-Item -LiteralPath $pdb -Destination (Join-Path $backupDir "$stamp-StarfieldParticle.pdb") }
    Write-Host "Removed: $target" -ForegroundColor Yellow
    Write-Host "Kept:    $keep"
    exit 0
}

if ($Source -and -not [IO.Path]::IsPathRooted($Source)) { $Source = Join-Path $repositoryRoot $Source }
$resolvedSource = if ($Source) { $Source } else { Join-Path $repositoryRoot "dist\$pluginName" }
if (-not (Test-Path -LiteralPath $resolvedSource)) {
    throw "No build to install at $resolvedSource. Run ae_plugin\BuildWindows.ps1 first, or pass -Source."
}
$resolvedSource = (Resolve-Path -LiteralPath $resolvedSource).Path

if (Test-Path -LiteralPath $target) {
    Show-File 'replaced:' $target
    $keep = Join-Path $backupDir "$stamp-$pluginName"
    Move-Item -LiteralPath $target -Destination $keep
    Write-Host "backup:   $keep"
} else {
    Write-Host "replaced: (none - no previous $pluginName)"
}

try {
    Copy-Item -LiteralPath $resolvedSource -Destination $target -Force
} catch [System.UnauthorizedAccessException] {
    throw "Access denied writing $target. Run this from an elevated PowerShell."
}
$sourcePdb = [IO.Path]::ChangeExtension($resolvedSource, '.pdb')
if (Test-Path -LiteralPath $sourcePdb) {
    Copy-Item -LiteralPath $sourcePdb -Destination ([IO.Path]::ChangeExtension($target, '.pdb')) -Force
}

Show-File 'installed:' $target
Write-Host 'Start After Effects to load it. Undo with -Uninstall.' -ForegroundColor Green
