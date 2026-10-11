param([switch]$Run,[switch]$Sanitize)
$ErrorActionPreference='Stop'
if(-not $Run){Write-Host 'Report: explicit graph command fake-SDK regression. Add -Run; no AE process is launched.';exit 0}
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskDir=if($Sanitize){'artifacts\graph-commit-host-asan-tests'}else{'artifacts\graph-commit-host-tests'}
Push-Location -LiteralPath $taskRepo
try {
    New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
    $taskArgs=@('/nologo','/std:c++20','/W4','/WX','/permissive-','/EHsc','/MT','/O2','/DMSWindows','/DWIN32','/D_WINDOWS','/D_CRT_SECURE_NO_WARNINGS',
        '/Iinclude','/Iae_plugin','/IAdobeSDK/May2023_AfterEffectsSDK/Examples/Headers','/IAdobeSDK/May2023_AfterEffectsSDK/Examples/Headers/SP',
        ('/Fo'+$taskDir+'\'),('/Fe'+$taskDir+'\tests.exe'),'tests/graph_commit_host_tests.cpp','ae_plugin/UiExclusionHost.cpp')
    if($Sanitize){$taskArgs+=@('/fsanitize=address','/Zi',('/Fd'+$taskDir+'/compiler.pdb'))}
    [IO.File]::WriteAllLines((Join-Path $taskRepo ($taskDir+'/compile.rsp')),$taskArgs,[Text.Encoding]::ASCII)
    $taskBatch=@"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl.exe @$taskDir\compile.rsp
if not "%errorlevel%"=="0" exit /b %errorlevel%
$taskDir\tests.exe
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo ($taskDir+'/compile.cmd')),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c ($taskDir+'\compile.cmd')
    if($LASTEXITCODE -ne 0){throw 'Explicit graph command regression failed.'}
} finally {Pop-Location}
