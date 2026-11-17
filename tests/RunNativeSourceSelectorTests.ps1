param([switch]$Run,[string]$BinaryDirectory='artifacts/plugin/2023/x64/Release')
$ErrorActionPreference='Stop'
if(-not $Run){Write-Output 'Report: call actual built node AEX source selectors with fake PF suites; no AE. Add -Run.';exit 0}
$taskRepo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$taskDir='artifacts\native-source-selector-tests'
Push-Location -LiteralPath $taskRepo
try {
    & tests/RunModelUiExclusionTests.ps1 -Run
    if(-not $?){throw 'Isolated Host exclusion fixture failed'}
    $taskBinary=(Resolve-Path -LiteralPath $BinaryDirectory).Path
    $taskHost=(Resolve-Path artifacts/model-ui-exclusion-tests/paired/StarfieldHost.aex).Path
    New-Item -ItemType Directory -Force $taskDir | Out-Null
    $taskArgs=@('/nologo','/std:c++20','/W4','/WX','/EHsc','/O2','/MT','/DMSWindows','/DWIN32','/D_WINDOWS','/D_CRT_SECURE_NO_WARNINGS',
        '/Iinclude','/Iae_plugin','/IAdobeSDK/May2023_AfterEffectsSDK/Examples/Headers','/IAdobeSDK/May2023_AfterEffectsSDK/Examples/Headers/SP',
        ('/Fo'+$taskDir+'/'),('/Fe'+$taskDir+'/tests.exe'),'tests/native_source_selector_tests.cpp')
    [IO.File]::WriteAllLines((Join-Path $taskRepo "$taskDir/compile.rsp"),$taskArgs,[Text.UTF8Encoding]::new($false))
    $taskBatch=@"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe @$taskDir/compile.rsp
if errorlevel 1 exit /b %errorlevel%
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDir/compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw 'Actual native source selector compilation failed'}
    & "$taskDir/tests.exe" $taskBinary $taskHost
    if($LASTEXITCODE -ne 0){throw 'Actual native source selector regression failed'}
} finally {Pop-Location}
