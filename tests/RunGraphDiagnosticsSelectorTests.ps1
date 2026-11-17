param([switch]$Run,[string]$BinaryDirectory='artifacts/plugin/2023/x64/Release')
$ErrorActionPreference='Stop'
if(-not $Run){Write-Output 'Report: actual main AEX diagnostics selector with read-only fake suites; no AE. Add -Run.';exit 0}
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$taskDir='artifacts\ux-13-20261104\diagnostics-selector-tests'
Push-Location -LiteralPath $taskRoot
try{
    if(-not (Test-Path -LiteralPath 'artifacts/model-ui-exclusion-tests/paired/StarfieldHost.aex')){& tests/RunModelUiExclusionTests.ps1 -Run;if(-not $?){throw 'Host exclusion fixture failed.'}}
    New-Item -ItemType Directory -Force $taskDir | Out-Null
    $taskSources=Get-ChildItem -LiteralPath src/core -Filter '*.cpp' | Where-Object {$_.Name -ne 'GpuRenderer.cpp'} | ForEach-Object {'src/core/'+$_.Name}
    $taskArgs=@('/nologo','/utf-8','/std:c++20','/W4','/WX','/EHsc','/O2','/MT','/DMSWindows','/DWIN32','/D_WINDOWS','/D_CRT_SECURE_NO_WARNINGS','/Iinclude','/Iae_plugin','/IAdobeSDK/May2023_AfterEffectsSDK/Examples/Headers','/IAdobeSDK/May2023_AfterEffectsSDK/Examples/Headers/SP',('/Fo'+$taskDir+'/'),('/Fe'+$taskDir+'/tests.exe'),'tests/graph_diagnostics_selector_tests.cpp')+$taskSources
    [IO.File]::WriteAllLines((Join-Path $taskRoot "$taskDir/compile.rsp"),$taskArgs,[Text.UTF8Encoding]::new($false))
    $taskBatch=@"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe @$taskDir/compile.rsp
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRoot "$taskDir/compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDir\compile.cmd";if($LASTEXITCODE -ne 0){throw 'Selector test compilation failed.'}
    & "$taskDir/tests.exe" (Resolve-Path $BinaryDirectory).Path (Resolve-Path 'artifacts/model-ui-exclusion-tests/paired/StarfieldHost.aex').Path
    if($LASTEXITCODE -ne 0){throw 'Selector probe failed.'}
}finally{Pop-Location}
