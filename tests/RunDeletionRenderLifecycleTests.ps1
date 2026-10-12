param([switch]$Run,[switch]$Sanitize)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if(-not $Run){Write-Host 'Report: actual complete backup and render-entry lifecycle. Run RunAliasHandoffTests first to create the seven-kind graph; add -Run to act.';exit 0}
$taskDir=if($Sanitize){'artifacts\deletion-render-lifecycle-asan'}else{'artifacts\deletion-render-lifecycle-tests'}
$taskVars='C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
$taskSources=@('tests\deletion_render_lifecycle_tests.cpp','tests\camera_capture_tests.cpp','ae_plugin\NodeGraphSync.cpp','ae_plugin\Camera.cpp',
    'ae_plugin\GraphCarrier.cpp','ae_plugin\NativeGraphCommit.cpp','ae_plugin\GraphParameter.cpp','ae_plugin\NativeNodeGraph.cpp',
    'ae_plugin\Parameters.cpp','ae_plugin\WorldBridge.cpp','ae_plugin\EmitterHistoryCapture.cpp','ae_plugin\NativeTemporalCache.cpp',
    'ae_plugin\NativeTemporalUI.cpp','ae_plugin\MotionBlur.cpp','ae_plugin\LightCapture.cpp','ae_plugin\ModelGeometryParameter.cpp',
    'ae_plugin\ModelMirrorParameter.cpp','ae_plugin\ModelMirrorTransaction.cpp','ae_plugin\EffectGraphBackup.cpp')
$taskSources+=@('Time','Render','SequenceCodec','Settings','Geometry','Graph','GraphConstruction','GraphEvaluation','MotionGeometry','MotionPathTravel','EmitterHistory',
    'Random','ParticleSimulation','ParticleTransform','ParticleTexture','PluginApi','CpuRenderer','SpriteScene','ModelGeometry','ModelResources','ModelScene')|
    ForEach-Object {'src\core\'+$_+'.cpp'}
Push-Location -LiteralPath $taskRepo
try {
    New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
    $taskArgs=@('/nologo','/std:c++20','/W4','/permissive-','/EHsc','/O2','/DNDEBUG','/MT','/DMSWindows','/DWIN32','/D_WINDOWS',
        '/DSTARFIELD_NODE_KIND_EMITTER','/D_CRT_SECURE_NO_WARNINGS','/Iinclude','/Iae_plugin',
        '/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers','/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers\SP',
        '/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Util',('/Fo'+$taskDir+'\'),('/Fe'+$taskDir+'\tests.exe'))+$taskSources
    $taskLinkArgs=''
    if($Sanitize){$taskArgs+=@('/fsanitize=address','/Zi',('/Fd'+$taskDir+'\compiler.pdb'));$taskLinkArgs="/link /DEBUG /PDB:$taskDir\tests.pdb"}
    [IO.File]::WriteAllLines((Join-Path $taskRepo "$taskDir\compile.rsp"),$taskArgs,[Text.UTF8Encoding]::new($false))
    $taskBatch=@"
@echo off
call "$taskVars" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe @$taskDir\compile.rsp $taskLinkArgs
if errorlevel 1 exit /b %errorlevel%
$taskDir\tests.exe artifacts\seven-kind-native-graph.bin
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDir\compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw 'Deletion render lifecycle compile/test failed.'}
} finally {Pop-Location}
