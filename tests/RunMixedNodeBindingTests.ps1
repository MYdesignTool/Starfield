param([switch]$Run,[switch]$Sanitize)
$ErrorActionPreference='Stop'
if(-not $Run){Write-Output 'Report: compile actual native mixed-node binding transactions with local May2023 SDK fake callbacks, then execute generated expressions in JS fixtures. Add -Run to act; no AE process is started.';exit 0}
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskDir=if($Sanitize){'artifacts\mixed-node-binding-asan-tests'}else{'artifacts\mixed-node-binding-tests'}
$taskCore=@('Time','Render','SequenceCodec','Settings','Geometry','Graph','GraphConstruction','GraphEvaluation',
    'MotionGeometry','MotionPathTravel','EmitterHistory','Random','ParticleSimulation','ParticleTransform','ParticleTexture',
    'ModelGeometry','ModelResources','ModelScene','PluginApi','CpuRenderer','SpriteScene') | ForEach-Object {'src\core\'+$_+'.cpp'}
$taskSources=@('tests\native_sync_tests.cpp','tests\camera_capture_tests.cpp','ae_plugin\NodeGraphSync.cpp','ae_plugin\Camera.cpp',
    'ae_plugin\GraphCarrier.cpp','ae_plugin\NativeGraphCommit.cpp','ae_plugin\GraphParameter.cpp','ae_plugin\NativeNodeGraph.cpp',
    'ae_plugin\Parameters.cpp','ae_plugin\WorldBridge.cpp','ae_plugin\EmitterHistoryCapture.cpp','ae_plugin\NativeTemporalCache.cpp',
    'ae_plugin\NativeTemporalUI.cpp','ae_plugin\MotionBlur.cpp','ae_plugin\ModelControls.cpp','ae_plugin\ModelGeometryParameter.cpp',
    'ae_plugin\ModelMirrorParameter.cpp','ae_plugin\ModelMirrorTransaction.cpp')+$taskCore
Push-Location -LiteralPath $taskRepo
try {
    New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
    $taskArgs=@('/nologo','/std:c++20','/W4','/permissive-','/EHsc','/O2','/MT','/DNDEBUG','/DMSWindows','/DWIN32','/D_WINDOWS',
        '/D_CRT_SECURE_NO_WARNINGS','/DSTARFIELD_NODE_KIND_EMITTER','/Iinclude','/Iae_plugin',
        '/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers','/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers\SP',
        '/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Util',('/Fo'+$taskDir+'\'),('/Fe'+$taskDir+'\tests.exe'))+$taskSources
    $taskLink='/link /STACK:16777216'
    # This aggregate fixture keeps many graphs in main's frame. ASAN redzones
    # require a larger test-executable stack; production DLL flags stay intact.
    if($Sanitize){$taskArgs+=@('/fsanitize=address','/Zi',('/Fd'+$taskDir+'\compiler.pdb'));$taskLink="/link /DEBUG /STACK:16777216 /PDB:$taskDir\tests.pdb"}
    [IO.File]::WriteAllLines("$taskDir\compile.rsp",$taskArgs,[Text.Encoding]::ASCII)
    $taskTransformArgs=@('/nologo','/std:c++20','/W4','/permissive-','/EHsc','/O2','/MT','/Iinclude','/Iae_plugin',
        ('/Fo'+$taskDir+'\'),('/Fe'+$taskDir+'\transform.exe'),'tests\transform_binding_tests.cpp')
    $taskTransformLink=''
    if($Sanitize){$taskTransformArgs+=@('/fsanitize=address','/Zi',('/Fd'+$taskDir+'\transform-compiler.pdb'));$taskTransformLink="/link /DEBUG /PDB:$taskDir\transform.pdb"}
    [IO.File]::WriteAllLines("$taskDir\transform.rsp",$taskTransformArgs,[Text.Encoding]::ASCII)
    $taskBatch=@"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl.exe @$taskDir\compile.rsp $taskLink
if not "%errorlevel%"=="0" exit /b %errorlevel%
$taskDir\tests.exe
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl.exe @$taskDir\transform.rsp $taskTransformLink
if not "%errorlevel%"=="0" exit /b %errorlevel%
$taskDir\transform.exe
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText("$taskDir\compile.cmd",$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw 'Mixed native binding compile/test failed.'}
    foreach($taskScript in @('tests/native_animation_expression_tests.js','tests/model_transform_expression_tests.js','tests/transform_binding_expression_tests.js')){
        & node $taskScript
        if($LASTEXITCODE -ne 0){throw "Generated native expression execution failed: $taskScript"}
    }
} finally {Pop-Location}
