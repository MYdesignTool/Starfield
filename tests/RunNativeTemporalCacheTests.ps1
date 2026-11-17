param([switch]$Run,[switch]$Analyze,[switch]$Sanitize,[switch]$Benchmark)
$ErrorActionPreference='Stop'
if(-not $Run){Write-Host 'Report: native proof cache correctness, reentrancy, concurrency, failures and optional benchmark; add -Run. No AE operations.';exit 0}
if($Analyze -and $Sanitize){throw 'Choose -Analyze or -Sanitize.'}
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskDir=if($Analyze){'artifacts\native-temporal-cache-analysis-tests'}elseif($Sanitize){'artifacts\native-temporal-cache-asan-tests'}else{'artifacts\native-temporal-cache-tests'}
Push-Location -LiteralPath $taskRepo
try {
    New-Item -ItemType Directory -Force $taskDir | Out-Null
    $taskFlags=if($Analyze){'/analyze'}elseif($Sanitize){'/fsanitize=address /Zi'}else{''}
    $taskArgs=if($Benchmark){' --benchmark'}else{''}
    $taskBatch=@"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe /nologo /std:c++20 /W4 /WX /permissive- /EHsc /O2 /MT /DMSWindows /DWIN32 /D_WINDOWS /D_CRT_SECURE_NO_WARNINGS $taskFlags /Iinclude /Iae_plugin /IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers /IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers\SP /Fo$taskDir\ /Fd$taskDir\compiler.pdb /Fe$taskDir\tests.exe tests\native_temporal_cache_tests.cpp ae_plugin\NativeTemporalCache.cpp src\core\Time.cpp
if errorlevel 1 exit /b %errorlevel%
$taskDir\tests.exe$taskArgs
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDir/compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw 'Native temporal cache fixture failed.'}
} finally {Pop-Location}
