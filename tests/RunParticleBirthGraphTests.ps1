param([switch]$Run)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskDir='artifacts\particle-birth-graph-tests'
if(-not $Run){Write-Host 'Report: compile/run birth graph and temporal/Auxiliary/cap regression with MSVC /MT. Add -Run to act.';exit 0}
$taskVars='C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
$taskSources=@('Time','Render','SequenceCodec','Settings','Geometry','Graph','GraphConstruction','GraphEvaluation','EmitterHistory',
    'Random','ParticleSimulation','ParticleTransform','ParticleTexture','PluginApi','CpuRenderer','SpriteScene','ModelGeometry','ModelResources','ModelScene') | ForEach-Object {'src\core\'+$_+'.cpp'}
Push-Location -LiteralPath $taskRepo
try {
    New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
    $taskArgs=@('/nologo','/std:c++20','/W4','/permissive-','/EHsc','/O2','/DNDEBUG','/MT','/Iinclude',
        ('/Fo'+$taskDir+'\'),('/Fe'+$taskDir+'\tests.exe'),'tests\particle_birth_graph_tests.cpp')+$taskSources
    [IO.File]::WriteAllLines((Join-Path $taskRepo "$taskDir\compile.rsp"),$taskArgs,[Text.UTF8Encoding]::new($false))
    $taskBatch=@"
@echo off
call "$taskVars" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe @$taskDir\compile.rsp
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDir\compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw 'Birth graph compilation failed.'}
    & (Join-Path $taskRepo "$taskDir\tests.exe")
    if($LASTEXITCODE -ne 0){throw 'Birth graph tests failed.'}
} finally {Pop-Location}
