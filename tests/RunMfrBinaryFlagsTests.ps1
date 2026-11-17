param([switch]$Run,[ValidatePattern('^artifacts[/\\][A-Za-z0-9._/\\-]+$')][string]$Candidate='artifacts/plugin/2023-native132-mfr-qualification/x64/Release',[switch]$Disabled,[int]$PackedVersion=32900)
$ErrorActionPreference='Stop'
if(-not $Run){Write-Host 'Report: load nine actual effect AEX files in an isolated test process, compare GLOBAL_SETUP with embedded PiPL and expected MFR bit; add -Run. No AE or installation.';exit 0}
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskCandidate=(Resolve-Path -LiteralPath (Join-Path $taskRepo $Candidate)).Path
if(-not $taskCandidate.StartsWith($taskRepo+'\artifacts\',[StringComparison]::OrdinalIgnoreCase)){throw 'Candidate must be inside repository artifacts.'}
Push-Location -LiteralPath $taskRepo
try {
    $taskDir='artifacts\mfr-01-20261116\binary-flags';New-Item -ItemType Directory -Force $taskDir | Out-Null
    $taskMode=if($Disabled){'off'}else{'on'}
    $taskBatch=@"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe /nologo /std:c++20 /W4 /WX /permissive- /EHsc /O2 /MT /DMSWindows /DWIN32 /D_WINDOWS /IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers /Fo$taskDir\ /Fe$taskDir\tests.exe tests\mfr_binary_flags_tests.cpp
if errorlevel 1 exit /b %errorlevel%
$taskDir\tests.exe "$Candidate" $taskMode $PackedVersion
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDir/compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw "Actual MFR binary flags check failed: $LASTEXITCODE"}
} finally {Pop-Location}
