param(
    [Parameter(Mandatory = $true)][string]$SdkRoot,
    [Parameter(Mandatory = $true)][string]$IntermediateDir,
    [Parameter(Mandatory = $true)][string]$ResourceSource,
    [string]$FlagHeaderPath = '',
    [string]$VersionHeaderPath = '',
    [string]$OutFlagsName = 'STARFIELD_OUT_FLAGS',
    [string]$OutFlags2Name = 'STARFIELD_OUT_FLAGS2',
    [switch]$GeneralPlugin
)

$ErrorActionPreference = 'Continue'
if (-not $FlagHeaderPath) { $FlagHeaderPath = Join-Path $PSScriptRoot 'PluginFlags.h' }
if (-not $VersionHeaderPath) { $VersionHeaderPath = Join-Path $PSScriptRoot 'PluginVersion.h' }
$null = New-Item -ItemType Directory -Force -Path $IntermediateDir
$headerDir = Join-Path $SdkRoot 'Examples\Headers'
$toolPath = Join-Path $SdkRoot 'Examples\Resources\PiPLtool.exe'
Push-Location $IntermediateDir
try {
    $batchPath = Join-Path $IntermediateDir 'BuildStarfieldPiPL.cmd'
    $batch = @"
@echo off
cl /nologo /Tc"$ResourceSource" /I "$headerDir" /EP > "StarfieldPiPL.rr"
if errorlevel 1 exit /b %errorlevel%
"$toolPath" "StarfieldPiPL.rr" "StarfieldPiPL.rrc"
if errorlevel 1 exit /b %errorlevel%
cl /nologo /TcStarfieldPiPL.rrc /D MSWindows /EP > "StarfieldPiPL.rc"
exit /b %errorlevel%
"@
    [System.IO.File]::WriteAllText($batchPath, $batch, [System.Text.Encoding]::ASCII)
    & $env:ComSpec /d /c 'BuildStarfieldPiPL.cmd'
    $toolExitCode = $LASTEXITCODE
}
finally {
    Pop-Location
}
if ($toolExitCode -ne 0) { throw "Adobe PiPL resource pipeline failed with exit code $toolExitCode" }

# Guard against a stale PiPL resource. The custom build step tracks the .r file, not
# the headers it includes, so a flags/version edit can silently leave an old PiPL in
# the .aex; AE then rejects the plug-in with "global outflags mismatch". Compare the
# generated resource against the single-source headers and fail loudly instead.
$generatedRc = Join-Path $IntermediateDir 'StarfieldPiPL.rc'
if (-not (Test-Path -LiteralPath $generatedRc)) { throw "PiPL pipeline did not produce $generatedRc" }

$flagText = [System.IO.File]::ReadAllText($FlagHeaderPath)
$versionText = [System.IO.File]::ReadAllText($VersionHeaderPath)
$resourceText = [System.IO.File]::ReadAllText($generatedRc)

$entries = @(
        @{ Name = $OutFlagsName; Text = $flagText; Source = $FlagHeaderPath },
        @{ Name = $OutFlags2Name; Text = $flagText; Source = $FlagHeaderPath },
        @{ Name = 'STARFIELD_VERSION_PACKED'; Text = $versionText; Source = $VersionHeaderPath })
if ($GeneralPlugin) {
    $preprocessed = [IO.File]::ReadAllText((Join-Path $IntermediateDir 'StarfieldPiPL.rr'))
    if ($preprocessed -notmatch 'Kind\s*\{\s*AEGP\s*\}' -or
        $preprocessed -notmatch 'CodeWin64X86\s*\{\s*"StarfieldHostEntry"\s*\}') {
        throw 'General PiPL must declare AEGP and the independent StarfieldHostEntry export.'
    }
    $entries = @($entries | Where-Object { $_.Name -eq 'STARFIELD_VERSION_PACKED' })
}
foreach ($entry in $entries) {
    $match = [regex]::Match($entry.Text, "#define\s+$($entry.Name)\s+(0x[0-9A-Fa-f]+|\d+)")
    if (-not $match.Success) { throw "Could not read $($entry.Name) from $($entry.Source)" }

    $literal = $match.Groups[1].Value
    $value = if ($literal.StartsWith('0x')) {
        [System.Convert]::ToUInt32($literal, 16)
    }
    else {
        [System.Convert]::ToUInt32($literal)
    }

    $valuePattern = "(?<![\d])$value\s*L"
    if ($GeneralPlugin) {
        $low = $value -band 65535
        $high = $value -shr 16
        $valuePattern = '"srev"[\s\S]*?4,\s*0x0,\s*' + $low + ',\s*' + $high + ','
        if ($resourceText -notmatch '"xgEA"' -or $resourceText -notmatch '"StarfieldHostEntry\\0') {
            throw 'Generated General PiPL has an incorrect kind or entry point.'
        }
    }
    if ($resourceText -notmatch $valuePattern) {
        throw "Generated PiPL does not declare $($entry.Name) = $value ($literal). The PiPL resource is stale: delete the intermediate directory and rebuild."
    }
}
