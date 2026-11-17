param([switch]$Run,[string]$CandidateDir='artifacts\plugin\2023\x64\Release')
$ErrorActionPreference='Stop'
if(-not $Run){Write-Host 'Report: load eight candidate node AEX modules in an isolated test EXE and audit actual registered selectors; add -Run. Does not start AE.';exit 0}
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
Push-Location -LiteralPath $taskRepo
try {
    $taskDir='artifacts\native-selector-audit-tests'
    New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
    $taskVars='C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
    $taskBatch=@"
@echo off
call "$taskVars" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe /nologo /std:c++20 /W4 /EHsc /O2 /MT /DMSWindows /DWIN32 /D_WINDOWS /Iae_plugin /Iinclude /IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers /IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers\SP /IAdobeSDK\May2023_AfterEffectsSDK\Examples\Util /Fo$taskDir\ /Fe$taskDir\tests.exe tests\native_selector_audit_tests.cpp
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDir\compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw 'Native selector audit compilation failed.'}
    & (Join-Path $taskRepo "$taskDir\tests.exe") $CandidateDir
    if($LASTEXITCODE -ne 0){throw 'Native selector audit failed.'}
} finally {Pop-Location}