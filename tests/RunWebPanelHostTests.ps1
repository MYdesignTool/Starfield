param([switch]$Run)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if(-not $Run){Write-Host 'Report only: production WebPanelHost queue with fake AE suites. Add -Run; does not start AE or WebView2.';exit 0}
$taskDirectory='artifacts\webview2\host-tests'
New-Item -ItemType Directory -Force -Path (Join-Path $taskRepo $taskDirectory) | Out-Null
$taskArgs=@('/nologo','/utf-8','/std:c++20','/W4','/WX','/EHsc','/O2','/MT','/DMSWindows','/DWIN32','/D_WINDOWS','/DUNICODE','/D_UNICODE','/D_CRT_SECURE_NO_WARNINGS',
    '/Iinclude','/Iae_plugin','/IAdobeSDK/May2023_AfterEffectsSDK/Examples/Headers','/IAdobeSDK/May2023_AfterEffectsSDK/Examples/Headers/SP',
    '/Iartifacts/webview2/dependencies/sdk-1.0.4258.31/build/native/include',('/Fo'+$taskDirectory+'/'),('/Fe'+$taskDirectory+'/tests.exe'),
    'tests/web_panel_host_tests.cpp','ae_plugin/UiExclusionHost.cpp','/link','artifacts/webview2/dependencies/sdk-1.0.4258.31/build/native/x64/WebView2LoaderStatic.lib',
    'Advapi32.lib','Ole32.lib','OleAut32.lib','Version.lib','Shlwapi.lib','Comctl32.lib','User32.lib','Gdi32.lib')
[IO.File]::WriteAllLines((Join-Path $taskRepo "$taskDirectory/compile.rsp"),$taskArgs,[Text.UTF8Encoding]::new($false))
$taskBatch=@"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl @$taskDirectory/compile.rsp
if errorlevel 1 exit /b %errorlevel%
$taskDirectory\tests.exe
exit /b %errorlevel%
"@
[IO.File]::WriteAllText((Join-Path $taskRepo "$taskDirectory/compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
Push-Location -LiteralPath $taskRepo
try{& $env:ComSpec /d /c "$taskDirectory\compile.cmd";if($LASTEXITCODE -ne 0){throw 'WebPanelHost tests failed.'}}finally{Pop-Location}
