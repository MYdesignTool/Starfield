param([switch]$Run,[switch]$Sanitize)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if(-not $Run){Write-Host 'Report: production Model chooser path with fake dialog/host checks; add -Run.';exit 0}
Push-Location -LiteralPath $taskRepo
try {
    if(-not(Test-Path -LiteralPath 'artifacts/model-ui-exclusion-tests/paired/StarfieldHost.aex')){
        & tests/RunModelUiExclusionTests.ps1 -Run
        if(-not $?){throw 'UI exclusion fixture failed.'}
    }
    $taskDir=if($Sanitize){'artifacts\model-import-ui-asan-tests'}else{'artifacts\model-import-ui-tests'}
    New-Item -ItemType Directory -Path $taskDir,'artifacts/model-import-ui-tests' -Force | Out-Null
    $taskFlags=if($Sanitize){'/fsanitize=address /Zi'}else{''}
    $taskBatch=@"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe /nologo /std:c++20 /W4 /permissive- /EHsc /O2 /MT /DMSWindows /DWIN32 /D_WINDOWS /D_CRT_SECURE_NO_WARNINGS $taskFlags /Iinclude /Iae_plugin /IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers /IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers\SP /Fo$taskDir\ /Fd$taskDir\compiler.pdb /Fe$taskDir\tests.exe tests\model_import_ui_tests.cpp
if errorlevel 1 exit /b %errorlevel%
$taskDir\tests.exe
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDir\compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw 'Model import UI regression failed.'}
} finally {Pop-Location}
