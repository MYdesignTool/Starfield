param([switch]$Run,[switch]$Bindings,[switch]$MotionCallbacks,[switch]$ModelCallbacks,[switch]$Sanitize)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if(-not $Run){Write-Host 'Report: compile/run actual Particle Cloud callback and native binding fixtures. Add -Run to act.';exit 0}
$taskExclusion=Join-Path $taskRepo 'artifacts/model-ui-exclusion-tests/paired/StarfieldHost.aex'
if(-not(Test-Path -LiteralPath $taskExclusion) -or (Get-Item -LiteralPath $taskExclusion).LastWriteTime -lt (Get-Item -LiteralPath (Join-Path $taskRepo 'ae_plugin/UiExclusionHost.cpp')).LastWriteTime) {
    & (Join-Path $PSScriptRoot 'RunModelUiExclusionTests.ps1') -Run
    if(-not $?){throw 'Source callback exclusion fixture failed.'}
}
$taskDir=if($ModelCallbacks){'artifacts\model-native-callback-tests'}elseif($MotionCallbacks){'artifacts\motion-native-callback-tests'}elseif($Bindings){'artifacts\cloud-native-binding-tests'}else{'artifacts\cloud-native-sync-tests'}
if($Sanitize){$taskDir+='-asan'}
$taskVars='C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
$taskSources=@('tests\native_sync_tests.cpp','tests\camera_capture_tests.cpp','ae_plugin\NodeGraphSync.cpp','ae_plugin\Camera.cpp',
    'ae_plugin\GraphCarrier.cpp','ae_plugin\NativeGraphCommit.cpp','ae_plugin\GraphParameter.cpp','ae_plugin\NativeNodeGraph.cpp',
    'ae_plugin\Parameters.cpp','ae_plugin\WorldBridge.cpp','ae_plugin\EmitterHistoryCapture.cpp','ae_plugin\NativeTemporalCache.cpp',
    'ae_plugin\NativeTemporalUI.cpp','ae_plugin\MotionBlur.cpp','ae_plugin\MotionLookAtCapture.cpp','ae_plugin\LightCapture.cpp',
    'ae_plugin\MotionLightSources.cpp','ae_plugin\MotionPointCapture.cpp','ae_plugin\ModelGeometryParameter.cpp',
    'ae_plugin\ModelMirrorParameter.cpp','ae_plugin\ModelMirrorTransaction.cpp')
$taskSources+=@('Time','Render','SequenceCodec','Settings','Geometry','Graph','GraphConstruction','GraphEvaluation','TurbulenceNoise','MotionLightPathClock','MotionLightPathResource','MotionGeometry','MotionPathTravel','EmitterHistory',
    'Random','ParticleSimulation','ParticleTransform','ParticleTexture','PluginApi','CpuRenderer','SpriteScene','ModelGeometry','ModelResources','ModelScene')|ForEach-Object {'src\core\'+$_+'.cpp'}
Push-Location -LiteralPath $taskRepo
try {
    New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
    $taskArgs=@('/nologo','/std:c++20','/W4','/permissive-','/EHsc','/O2','/DNDEBUG','/MT','/DMSWindows','/DWIN32','/D_WINDOWS',
        '/D_CRT_SECURE_NO_WARNINGS','/DSTARFIELD_TEST_NO_JOURNAL',
        '/Iinclude','/Iae_plugin','/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers',
        '/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers\SP','/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Util',
        ('/Fo'+$taskDir+'\'),('/Fe'+$taskDir+'\tests.exe'))+$taskSources
    $taskArgs+=if($ModelCallbacks){@('/DSTARFIELD_NODE_KIND_MODEL','/DSTARFIELD_MODEL_CALLBACK_TEST')}elseif($MotionCallbacks){@('/DSTARFIELD_NODE_KIND_MOTION','/DSTARFIELD_MOTION_CALLBACK_TEST')}elseif($Bindings){'/DSTARFIELD_NODE_KIND_EMITTER'}else{@('/DSTARFIELD_NODE_KIND_PARTICLE','/DSTARFIELD_CLOUD_CALLBACK_TEST')}
    if($Sanitize){$taskArgs+=@('/fsanitize=address','/Zi',('/Fd'+$taskDir+'\compiler.pdb'))}
    [IO.File]::WriteAllLines((Join-Path $taskRepo "$taskDir\compile.rsp"),$taskArgs,[Text.UTF8Encoding]::new($false))
    $taskBatch=@"
@echo off
call "$taskVars" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe @$taskDir\compile.rsp
if errorlevel 1 exit /b %errorlevel%
$taskDir\tests.exe
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDir\compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw 'Cloud callback fixture compilation failed.'}
    if($LASTEXITCODE -ne 0){throw "Cloud native fixture failed: $LASTEXITCODE"}
} finally {Pop-Location}
