param([switch]$Run,[switch]$Analyze,[switch]$Sanitize)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if(-not $Run){Write-Host 'Report only: production native deletion with fake May2023 SDK, no AE/browser. Add -Run.';exit 0}
if($Analyze -and $Sanitize){throw 'Choose analysis or ASan.'}
$taskDir=if($Analyze){'artifacts\native-graph-edit-analysis'}elseif($Sanitize){'artifacts\native-graph-edit-asan'}else{'artifacts\native-graph-edit-tests'}
New-Item -ItemType Directory -Path (Join-Path $taskRepo $taskDir) -Force | Out-Null
$taskArgs=@('/nologo','/utf-8','/std:c++20','/W4','/WX','/permissive-','/EHsc','/O2','/MT','/DMSWindows','/DWIN32','/D_WINDOWS','/D_CRT_SECURE_NO_WARNINGS','/DSTARFIELD_TEST_NO_JOURNAL',
    '/Iinclude','/Iae_plugin','/IAdobeSDK/May2023_AfterEffectsSDK/Examples/Headers','/IAdobeSDK/May2023_AfterEffectsSDK/Examples/Headers/SP',('/Fo'+$taskDir+'/'),('/Fe'+$taskDir+'/tests.exe'),
    'tests/native_graph_edit_tests.cpp','ae_plugin/NativeGraphEdit.cpp','ae_plugin/EffectGraphBackup.cpp','ae_plugin/UiExclusionHost.cpp')
if($Analyze){$taskArgs+='/analyze'}
if($Sanitize){$taskArgs+=@('/fsanitize=address','/Zi',('/Fd'+$taskDir+'/compiler.pdb'))}
[IO.File]::WriteAllLines((Join-Path $taskRepo "$taskDir/compile.rsp"),$taskArgs,[Text.UTF8Encoding]::new($false))
$taskBatch=@"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe @$taskDir/compile.rsp
if errorlevel 1 exit /b %errorlevel%
$taskDir\tests.exe
exit /b %errorlevel%
"@
[IO.File]::WriteAllText((Join-Path $taskRepo "$taskDir\compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
Push-Location -LiteralPath $taskRepo
try{& $env:ComSpec /d /c "$taskDir\compile.cmd";if($LASTEXITCODE -ne 0){throw 'Native graph edit tests failed.'}}finally{Pop-Location}


