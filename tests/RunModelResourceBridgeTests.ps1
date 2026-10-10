param([switch]$Run,[switch]$Sanitize)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if(-not $Run){Write-Host 'Report: compile/run Model mirror transactions and immutable resource capture with local May2023 SDK fake callbacks and C ABI pixels; add -Run to act.';exit 0}
$taskDir=if($Sanitize){'artifacts\model-resource-bridge-asan-tests'}else{'artifacts\model-resource-bridge-tests'}
$taskVars='C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
$taskSources=@('Time','Render','Settings','SequenceCodec','Graph','GraphConstruction','GraphEvaluation','MotionGeometry','MotionPathTravel','EmitterHistory',
    'Random','ParticleSimulation','ParticleTransform','ParticleTexture','ModelGeometry','ModelResources','ModelScene','Geometry','PluginApi','CpuRenderer','SpriteScene') |
    ForEach-Object {'src\core\'+$_+'.cpp'}
Push-Location -LiteralPath $taskRepo
try {
    New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
    $taskArgs=@('/nologo','/std:c++20','/W4','/permissive-','/EHsc','/O2','/MT','/DMSWindows','/DWIN32','/D_WINDOWS','/D_CRT_SECURE_NO_WARNINGS',
        '/Iinclude','/Iae_plugin','/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers',
        '/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers\SP','/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Util',
        ('/Fo'+$taskDir+'\'),('/Fe'+$taskDir+'\tests.exe'),'tests\model_resource_bridge_tests.cpp',
        'ae_plugin\ModelControls.cpp','ae_plugin\ModelMirrorParameter.cpp','ae_plugin\ModelMirrorTransaction.cpp','ae_plugin\ModelRenderResources.cpp','ae_plugin\ModelGeometryParameter.cpp')+$taskSources
    $taskLinkArgs=''
    if($Sanitize){$taskArgs+=@('/fsanitize=address','/Zi',('/Fd'+$taskDir+'\compiler.pdb'));$taskLinkArgs="/link /DEBUG /PDB:$taskDir\tests.pdb"}
    [IO.File]::WriteAllLines((Join-Path $taskRepo "$taskDir\compile.rsp"),$taskArgs,[Text.UTF8Encoding]::new($false))
    $taskBatch=@"
@echo off
call "$taskVars" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe @$taskDir\compile.rsp $taskLinkArgs
if errorlevel 1 exit /b %errorlevel%
$taskDir\tests.exe
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDir\compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw 'Model control registration/capture compile/test failed.'}
} finally {Pop-Location}
