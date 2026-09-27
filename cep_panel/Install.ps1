# Installs or removes the Starfield node editor panel as a per-user CEP extension.
#
#   install everything:  powershell -ExecutionPolicy Bypass -File cep_panel\Install.ps1
#   remove everything:   powershell -ExecutionPolicy Bypass -File cep_panel\Install.ps1 -Uninstall
#   isolate the registry:powershell -ExecutionPolicy Bypass -File cep_panel\Install.ps1 -RegistryOnly
#   isolate the panel:   powershell -ExecutionPolicy Bypass -File cep_panel\Install.ps1 -PanelOnly
#
# Two pieces, deliberately separable, because both touch the host beyond this extension:
#
#   * the per-user PlayerDebugMode key, which is what lets an UNSIGNED extension load at all
#     (it applies to every CEP extension in the host while it is set);
#   * the extension link itself.
#
# If other extension panels stop opening, run -Uninstall first, restart After Effects, then
# reinstall one piece at a time so the failing half is identified. See README.md.

param(
    [switch]$Uninstall,
    [switch]$RegistryOnly,
    [switch]$PanelOnly
)

$ErrorActionPreference = 'Stop'

$bundleId = 'org.starfieldfx.panel'
$source = $PSScriptRoot
$extensionsRoot = Join-Path $env:APPDATA 'Adobe\CEP\extensions'
$target = Join-Path $extensionsRoot $bundleId
$cepVersions = @('CSXS.11', 'CSXS.12')

function Remove-ExtensionLink {
    if (Test-Path -LiteralPath $target) {
        $item = Get-Item -LiteralPath $target -Force
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            [IO.Directory]::Delete($target, $false)   # removes the junction, not the folder
            Write-Host "Removed extension link: $target"
        } else {
            Write-Host "Not a junction, leaving it alone: $target" -ForegroundColor Yellow
        }
    } else {
        Write-Host "No extension link at $target"
    }
}

function Remove-DebugKey {
    foreach ($version in $cepVersions) {
        $key = "HKCU:\Software\Adobe\$version"
        if (Test-Path $key) {
            Remove-ItemProperty -Path $key -Name PlayerDebugMode -ErrorAction SilentlyContinue
            Write-Host "Cleared PlayerDebugMode for $version"
        }
    }
}

function Add-DebugKey {
    foreach ($version in $cepVersions) {
        $key = "HKCU:\Software\Adobe\$version"
        New-Item -Path $key -Force | Out-Null
        New-ItemProperty -Path $key -Name PlayerDebugMode -PropertyType String -Value 1 -Force | Out-Null
        Write-Host "Enabled unsigned extensions for $version"
    }
}

function Add-ExtensionLink {
    if (-not (Test-Path (Join-Path $source 'CSXS\manifest.xml'))) {
        throw 'manifest.xml not found next to this script; run it from the cep_panel folder.'
    }
    New-Item -ItemType Directory -Force -Path $extensionsRoot | Out-Null
    Remove-ExtensionLink | Out-Null
    $result = & cmd /c "mklink /J `"$target`" `"$source`"" 2>&1
    if (-not (Test-Path (Join-Path $target 'CSXS\manifest.xml'))) {
        throw "Could not link the extension: $result"
    }
    Write-Host "Linked: $target -> $source"
}

if ($Uninstall) {
    Remove-ExtensionLink
    Remove-DebugKey
} elseif ($RegistryOnly) {
    Add-DebugKey
} elseif ($PanelOnly) {
    Add-ExtensionLink
} else {
    Add-DebugKey
    Add-ExtensionLink
    Write-Host ''
    Write-Host 'Next: restart After Effects, then Window > Extensions > Starfield Node Editor.'
    Write-Host 'Qualification steps are in cep_panel/README.md.'
}

Write-Host 'Restart After Effects if it is running.'
