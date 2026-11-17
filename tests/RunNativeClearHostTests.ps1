param([switch]$Run,[switch]$Analyze,[switch]$Sanitize)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if(-not $Run){Write-Output 'Report: actual Host/native Clear fixture, no AE. Add -Run.';exit 0}
if($Analyze -and $Sanitize){throw 'Choose analysis or ASan.'}
$taskDir=if($Analyze){'artifacts\native-clear-host-analysis'}elseif($Sanitize){'artifacts\native-clear-host-asan'}else{'artifacts\native-clear-host-tests'}
New-Item -ItemType Directory -Force -Path (Join-Path $taskRepo $taskDir) | Out-Null
$taskArgs=@('/nologo','/utf-8','/std:c++20','/W4','/WX','/EHsc','/O2','/MT','/DMSWindows','/DWIN32','/D_WINDOWS','/DUNICODE','/D_UNICODE','/D_CRT_SECURE_NO_WARNINGS',
 '/Iinclude','/Iae_plugin','/IAdobeSDK/May2023_AfterEffectsSDK/Examples/Headers','/IAdobeSDK/May2023_AfterEffectsSDK/Examples/Headers/SP',
 '/Iartifacts/webview2/dependencies/sdk-1.0.4258.31/build/native/include',('/Fo'+$taskDir+'/'),('/Fe'+$taskDir+'/tests.exe'),
 'tests/native_clear_host_tests.cpp','ae_plugin/NativeGraphEdit.cpp','ae_plugin/EffectGraphBackup.cpp','ae_plugin/UiExclusionHost.cpp')
if($Analyze){$taskArgs+='/analyze'}
if($Sanitize){$taskArgs+=@('/fsanitize=address','/Zi',('/Fd'+$taskDir+'/compiler.pdb'))}
$taskArgs+=@('/link','artifacts/webview2/dependencies/sdk-1.0.4258.31/build/native/x64/WebView2LoaderStatic.lib','Advapi32.lib','Ole32.lib','OleAut32.lib','Version.lib','Shlwapi.lib','Comctl32.lib','User32.lib','Gdi32.lib')
[IO.File]::WriteAllLines((Join-Path $taskRepo "$taskDir/compile.rsp"),$taskArgs,[Text.UTF8Encoding]::new($false))
$taskBatch=@"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl @$taskDir/compile.rsp
if errorlevel 1 exit /b %errorlevel%
$taskDir\tests.exe
exit /b %errorlevel%
"@
[IO.File]::WriteAllText((Join-Path $taskRepo "$taskDir/compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
Push-Location $taskRepo
try{& $env:ComSpec /d /c "$taskDir\compile.cmd";if($LASTEXITCODE -ne 0){throw 'Native Clear Host fixture failed.'}}finally{Pop-Location}
