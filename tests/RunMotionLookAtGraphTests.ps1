param([switch]$Run,[switch]$Sanitize)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskDir=if($Sanitize){'artifacts\motion-look-at-graph-asan'}else{'artifacts\motion-look-at-graph-tests'}
if(-not $Run){Write-Host 'Report: build/run Look At graph, history and renderer fixture with MSVC /MT. Add -Run to act.';exit 0}
$taskVars='C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
$taskSources=@('Time','Render','SequenceCodec','Settings','Geometry','Graph','GraphConstruction','GraphEvaluation','EmitterHistory',
    'Random','ParticleSimulation','ParticleTransform','ParticleTexture','CpuRenderer','SpriteScene',
    'ModelGeometry','ModelResources','ModelScene','MotionGeometry') | ForEach-Object {'src\core\'+$_+'.cpp'}
Push-Location -LiteralPath $taskRepo
try {
    New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
    $taskArgs=@('/nologo','/std:c++20','/W4','/permissive-','/EHsc','/O2','/MT','/Iinclude',
        ('/Fo'+$taskDir+'\'),('/Fe'+$taskDir+'\tests.exe'),'tests\motion_look_at_graph_tests.cpp')+$taskSources
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
    if($LASTEXITCODE -ne 0){throw 'Look At graph compile/test failed.'}
} finally {Pop-Location}
