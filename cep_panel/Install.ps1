# Links this folder into the user CEP extensions directory, or removes the link again.
#
#   install: powershell -ExecutionPolicy Bypass -File cep_panel\Install.ps1
#   remove : powershell -ExecutionPolicy Bypass -File cep_panel\Install.ps1 -Uninstall
#
# Registry: this script NEVER writes it, in either direction.
#
# Unsigned extensions only load while HKCU\Software\Adobe\CSXS.<n>\PlayerDebugMode is "1",
# and that key is host-wide: every other unsigned panel in the same host depends on it.
# Clearing it once silently disabled all of the owner's panels, and a restart could not
# bring them back because host state is not a cache. So installing or removing one panel
# must not have that side effect, and this script only reports the value (read-only).
# Setting it, if it is ever missing, is a deliberate manual owner action - see README.md.

param(
    [switch]$Uninstall
)

$ErrorActionPreference = 'Stop'

$bundleId = 'org.starfieldfx.panel'
$source = $PSScriptRoot
$extensionsRoot = Join-Path $env:APPDATA 'Adobe\CEP\extensions'
$target = Join-Path $extensionsRoot $bundleId

function Show-DebugKeyStatus {
    Write-Host 'read-only registry check (this script changes nothing):'
    foreach ($version in 'CSXS.11', 'CSXS.12') {
        $key = "HKCU:\Software\Adobe\$version"
        if (Test-Path $key) {
            $value = (Get-ItemProperty -Path $key -Name PlayerDebugMode -ErrorAction SilentlyContinue).PlayerDebugMode
            Write-Host ("  {0}\PlayerDebugMode = '{1}'" -f $version, $value)
        } else {
            Write-Host ("  {0}: key absent" -f $version)
        }
    }
}

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

if ($Uninstall) {
    Remove-ExtensionLink
    Write-Host 'PlayerDebugMode is left exactly as it is.'
    Show-DebugKeyStatus
    exit 0
}

if (-not (Test-Path (Join-Path $source 'CSXS\manifest.xml'))) {
    throw 'manifest.xml not found next to this script; run it from the cep_panel folder.'
}

New-Item -ItemType Directory -Force -Path $extensionsRoot | Out-Null
if (Test-Path -LiteralPath $target) {
    $item = Get-Item -LiteralPath $target -Force
    if (-not ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        Write-Host "A real folder is already at $target, so it is left alone." -ForegroundColor Yellow
        Write-Host 'Rename or remove it yourself if you want this script to link the repository instead.'
        exit 1
    }
    Remove-ExtensionLink | Out-Null
}

$result = & cmd /c "mklink /J `"$target`" `"$source`"" 2>&1
if (-not (Test-Path (Join-Path $target 'CSXS\manifest.xml'))) {
    throw "Could not link the extension: $result"
}
Write-Host "Linked: $target -> $source"
Show-DebugKeyStatus
Write-Host ''
Write-Host 'Next: restart After Effects, then Window > Extensions > Starfield Node Editor.'
Write-Host 'Qualification steps are in cep_panel/README.md.'
