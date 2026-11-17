param([switch]$Run)
$ErrorActionPreference='Stop'
if(-not $Run){Write-Host 'Report: compile actual qualification journal scope and verify concurrent/nested/unwind records; add -Run. No AE or installation.';exit 0}
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskDirectory='artifacts\mfr-offline-handoff-20261116\journal-tests'
Push-Location -LiteralPath $taskRepo
try {
    New-Item -ItemType Directory -Force $taskDirectory | Out-Null
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDirectory/records.jsonl"),'')
    $taskBatch=@"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe /nologo /std:c++20 /W4 /WX /permissive- /EHsc /O2 /MT /DMSWindows /DWIN32 /D_WINDOWS /Iae_plugin /Iinclude /IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers /IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers\SP /Fo$taskDirectory\ /Fe$taskDirectory\tests.exe tests\mfr_journal_tests.cpp
if errorlevel 1 exit /b %errorlevel%
$taskDirectory\tests.exe
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDirectory/compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDirectory\compile.cmd"
    if($LASTEXITCODE -ne 0){throw 'Production journal fixture failed.'}
    & (Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe') -B tests/mfr_journal_verify.py
    if($LASTEXITCODE -ne 0){throw 'Production journal records failed verification.'}
} finally {Pop-Location}
