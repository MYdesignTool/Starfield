param(
    [Parameter(Mandatory = $true)][string]$SdkRoot,
    [Parameter(Mandatory = $true)][string]$IntermediateDir,
    [Parameter(Mandatory = $true)][string]$ResourceSource
)

$ErrorActionPreference = 'Continue'
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
