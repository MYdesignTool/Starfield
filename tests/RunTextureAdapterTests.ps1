param([switch]$Run)
$ErrorActionPreference='Stop'
$repositoryRoot=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$directory='artifacts\texture-adapter-tests'
if(-not $Run){Write-Host 'Report: compile and run the bounded fake SmartFX texture fixture with May2023 SDK /MT. Add -Run to act.';exit 0}
$msvcVars='C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
$sources=@('Time','Render','SequenceCodec','Settings','Geometry','Graph','GraphConstruction','GraphEvaluation','MotionGeometry','MotionPathTravel','EmitterHistory','Random','ParticleSimulation','ParticleTransform','ParticleTexture') | ForEach-Object {'src\core\'+$_+'.cpp'}
$arguments=@('/nologo','/std:c++20','/W4','/permissive-','/EHsc','/O2','/DNDEBUG','/MT','/DMSWindows','/DWIN32','/D_WINDOWS','/D_CRT_SECURE_NO_WARNINGS','/Iinclude','/Iae_plugin',
    '/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers','/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers\SP',
    ('/Fo'+$directory+'\'),('/Fe'+$directory+'\texture_adapter_tests.exe'),'tests\texture_adapter_tests.cpp','ae_plugin\TextureResources.cpp','ae_plugin\WorldBridge.cpp')+$sources
Push-Location -LiteralPath $repositoryRoot
try {
    $null=New-Item -ItemType Directory -Force -Path $directory
    [IO.File]::WriteAllLines((Join-Path $repositoryRoot "$directory\compile.rsp"),$arguments,[Text.UTF8Encoding]::new($false))
    $batch=@"
@echo off
call "$msvcVars" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe @$directory\compile.rsp
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $repositoryRoot "$directory\compile.cmd"),$batch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$directory\compile.cmd"
    if($LASTEXITCODE -ne 0){throw "Texture adapter fixture compilation failed: $LASTEXITCODE"}
    & (Join-Path $repositoryRoot "$directory\texture_adapter_tests.exe")
    if($LASTEXITCODE -ne 0){throw "Texture adapter fixture failed: $LASTEXITCODE"}
} finally {Pop-Location}
