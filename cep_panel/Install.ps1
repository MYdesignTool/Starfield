# Installs or removes the Starfield node editor panel as a per-user CEP extension, so the
# manual steps in README.md become one command.
#
#   install:    powershell -ExecutionPolicy Bypass -File cep_panel\Install.ps1
#   uninstall:  powershell -ExecutionPolicy Bypass -File cep_panel\Install.ps1 -Uninstall
#
# It creates a directory junction from %APPDATA%\Adobe\CEP\extensions\org.starfieldfx.panel
# to this folder and enables unsigned extensions for the AE 2023 CEP version. The junction
# means an edit here is picked up by AE without copying files around.

param([switch]$Uninstall)

$ErrorActionPreference = 'Stop'

$bundleId = 'org.starfieldfx.panel'
$source = $PSScriptRoot
$extensionsRoot = Join-Path $env:APPDATA 'Adobe\CEP\extensions'
$target = Join-Path $extensionsRoot $bundleId
$cepVersions = @('CSXS.11', 'CSXS.12')

function Remove-Extension {
    if (Test-Path -LiteralPath $target) {
        $item = Get-Item -LiteralPath $target -Force
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            # Remove the junction only; the real folder stays untouched.
            [IO.Directory]::Delete($target, $false)
            Write-Host "Removed extension link: $target"
        } else {
            Write-Host "Not a junction, leaving it alone: $target" -ForegroundColor Yellow
        }
    } else {
        Write-Host "Nothing to remove at $target"
    }
    foreach ($version in $cepVersions) {
        $key = "HKCU:\Software\Adobe\$version"
        if (Test-Path $key) {
            Remove-ItemProperty -Path $key -Name PlayerDebugMode -ErrorAction SilentlyContinue
            Write-Host "Cleared PlayerDebugMode for $version"
        }
    }
    Write-Host 'Restart After Effects if it is running.'
}

function Install-Extension {
    if (-not (Test-Path (Join-Path $source 'CSXS\manifest.xml'))) {
        throw "manifest.xml not found next to this script; run it from the cep_panel folder."
    }
    foreach ($version in $cepVersions) {
        $key = "HKCU:\Software\Adobe\$version"
        New-Item -Path $key -Force | Out-Null
        New-ItemProperty -Path $key -Name PlayerDebugMode -PropertyType String -Value 1 -Force | Out-Null
        Write-Host "Enabled unsigned extensions for $version"
    }
    New-Item -ItemType Directory -Force -Path $extensionsRoot | Out-Null
    Remove-Extension | Out-Null
    $result = & cmd /c "mklink /J `"$target`" `"$source`"" 2>&1
    if (-not (Test-Path (Join-Path $target 'CSXS\manifest.xml'))) {
        throw "Could not link the extension: $result"
    }
    Write-Host "Linked: $target -> $source"
    Write-Host ''
    Write-Host 'Next: restart After Effects, then Window > Extensions > Starfield Node Editor.'
    Write-Host 'Qualification steps are in cep_panel/README.md.'
}

if ($Uninstall) { Remove-Extension } else { Install-Extension }
