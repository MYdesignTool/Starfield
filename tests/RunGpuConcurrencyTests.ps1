param([switch]$Run,[switch]$Analyze,[switch]$Sanitize,[switch]$Before)
$ErrorActionPreference='Stop'
if(-not $Run){Write-Host 'Report: production GPU adapter overlap/reentry/cancellation/fault tests with fake host/driver; add -Run. No AE or host changes.';exit 0}
if($Analyze -and $Sanitize){throw 'Choose -Analyze or -Sanitize.'}
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskMode=if($Before){'before'}elseif($Analyze){'analysis'}elseif($Sanitize){'asan'}else{'normal'}
$taskDir="artifacts\mfr-01-20261116\gpu-$taskMode"
Push-Location -LiteralPath $taskRepo
try {
    New-Item -ItemType Directory -Force $taskDir | Out-Null
    $taskFlags=if($Analyze){'/analyze'}elseif($Sanitize){'/fsanitize=address /Zi'}else{''}
    if($Before){
        # Frozen pre-change source, not HEAD (which predates unrelated dirty work).
        $taskOld='artifacts/prepared/mfr01-native130-panel124-prerequisite-v1/source/ae_plugin/GpuRender.cpp'
        if((Get-FileHash -LiteralPath $taskOld -Algorithm SHA256).Hash -ne 'B74A73F2A2ED63E493FA78FDEC3E1AE84DF9CA7C9224FDA1774FE0C3FDCC4C8A'){throw 'Frozen negative-control source hash mismatch.'}
        $taskFlags+=' /DSTARFIELD_GPU_IMPLEMENTATION=\"../'+$taskOld+'\"'
    }
    $taskBatch=@"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe /nologo /std:c++20 /W4 /WX /permissive- /EHsc /O2 /MT /DMSWindows /DWIN32 /D_WINDOWS /D_CRT_SECURE_NO_WARNINGS $taskFlags /Iinclude /Iae_plugin /Iartifacts\gpu-build\generated /IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers /IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers\SP /Fo$taskDir\ /Fd$taskDir\compiler.pdb /Fe$taskDir\tests.exe tests\gpu_concurrency_tests.cpp
if errorlevel 1 exit /b %errorlevel%
$taskDir\tests.exe
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDir/compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw "GPU concurrency fixture failed: $LASTEXITCODE"}
} finally {Pop-Location}
