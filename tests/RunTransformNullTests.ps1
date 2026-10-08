param([string]$MSVCVarsPath='C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
Push-Location -LiteralPath $taskRepo
try {
    $taskFolder='artifacts\transform-null-ui-tests'
    New-Item -ItemType Directory -Path $taskFolder -Force | Out-Null
    $taskExe="$taskFolder\tests.exe"
    $taskResponse="$taskFolder\compile.rsp"
    @('/nologo','/std:c++20','/W4','/O2','/DNDEBUG','/MT','/EHsc','/DMSWindows','/DWIN32','/D_WINDOWS',
      '/I "ae_plugin"','/I "include"','/I "AdobeSDK\May2023_AfterEffectsSDK\Examples\Headers"',
      '/I "AdobeSDK\May2023_AfterEffectsSDK\Examples\Util"',
      '/I "AdobeSDK\May2023_AfterEffectsSDK\Examples\Headers\SP"',
      "/Fo$taskFolder\", "/Fe$taskExe",'tests\transform_null_ui_tests.cpp','ae_plugin\TransformNullUI.cpp') |
        Set-Content -LiteralPath $taskResponse -Encoding ASCII
    $taskBatch="$taskFolder\compile.cmd"
    @('@echo off',('call "{0}" >nul' -f $MSVCVarsPath),'if errorlevel 1 exit /b %errorlevel%',
      ('cl.exe @"{0}"' -f $taskResponse),'exit /b %errorlevel%') | Set-Content -LiteralPath $taskBatch -Encoding ASCII
    & $env:ComSpec /d /c $taskBatch
    if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
    & (Join-Path $taskRepo $taskExe)
    exit $LASTEXITCODE
} finally {Pop-Location}
