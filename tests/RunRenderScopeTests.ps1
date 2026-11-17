param([switch]$Run,[switch]$Analyze,[switch]$Sanitize)
$ErrorActionPreference='Stop'
if(-not $Run){Write-Output 'Report: memory-only actual SmartFX preparation scope; add -Run. No AE/device operations.';exit 0}
if($Analyze -and $Sanitize){throw 'Choose Analyze or Sanitize.'}
$taskRepo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$taskDir=if($Analyze){'artifacts\render-scope-analysis-tests'}elseif($Sanitize){'artifacts\render-scope-asan-tests'}else{'artifacts\render-scope-tests'}
Push-Location -LiteralPath $taskRepo
try {
    New-Item -ItemType Directory -Force $taskDir | Out-Null
    $taskCore=@('Time','Render','SequenceCodec','Settings','Geometry','Graph','GraphConstruction','GraphEvaluation','TurbulenceNoise','MotionLightPathClock','MotionLightPathResource','MotionGeometry','MotionPathTravel','EmitterHistory','Random','ParticleSimulation','ParticleTransform','ParticleTexture','ModelGeometry','ModelResources','ModelScene') | ForEach-Object {'src\core\'+$_+'.cpp'}
    $taskFlags=if($Analyze){@('/analyze')}elseif($Sanitize){@('/fsanitize=address','/Zi',('/Fd'+$taskDir+'\compiler.pdb'))}else{@()}
    $taskArgs=@('/nologo','/std:c++20','/W4','/WX','/permissive-','/EHsc','/O2','/Gy','/MT','/DMSWindows','/DWIN32','/D_WINDOWS','/D_CRT_SECURE_NO_WARNINGS','/Iinclude','/Iae_plugin','/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers','/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers\SP',('/Fo'+$taskDir+'\'),('/Fe'+$taskDir+'\tests.exe'),'tests\render_scope_tests.cpp','ae_plugin\SmartRender.cpp','ae_plugin\WorldBridge.cpp')+$taskCore+$taskFlags
    [IO.File]::WriteAllLines((Join-Path $taskRepo "$taskDir/compile.rsp"),$taskArgs,[Text.UTF8Encoding]::new($false))
    @"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe @$taskDir\compile.rsp
if errorlevel 1 exit /b %errorlevel%
$taskDir\tests.exe
exit /b %errorlevel%
"@ | Set-Content "$taskDir/compile.cmd" -Encoding ASCII
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw 'Render scope fixture failed.'}
} finally {Pop-Location}
