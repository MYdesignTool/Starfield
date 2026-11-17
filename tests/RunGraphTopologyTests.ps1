param([switch]$Run,[switch]$Sanitize)
$ErrorActionPreference='Stop'
if(-not $Run){Write-Host 'Report only: Core topology/projector offline checks. Add -Run.';exit 0}
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$taskDir=if($Sanitize){'artifacts\ux-13-20261104\topology-asan-tests'}else{'artifacts\ux-13-20261104\topology-tests'}
New-Item -ItemType Directory -Force -Path (Join-Path $taskRoot $taskDir) | Out-Null
$taskSources=Get-ChildItem -LiteralPath (Join-Path $taskRoot 'src/core') -Filter '*.cpp' | Where-Object {$_.Name -notin @('GpuRenderer.cpp')} | ForEach-Object {'src/core/'+$_.Name}
$taskArgs=@('/nologo','/std:c++20','/utf-8','/W4','/permissive-','/EHsc','/O2','/MT','/Iinclude',('/Fo'+$taskDir+'/'),('/Fe'+$taskDir+'/tests.exe'),'tests/graph_topology_tests.cpp')+$taskSources
if($Sanitize){$taskArgs+=@('/fsanitize=address','/Zi',('/Fd'+$taskDir+'/tests.pdb'))}
[IO.File]::WriteAllLines((Join-Path $taskRoot "$taskDir/compile.rsp"),$taskArgs,[Text.UTF8Encoding]::new($false))
$taskBatch=@"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe @$taskDir/compile.rsp
if errorlevel 1 exit /b %errorlevel%
$taskDir\tests.exe
exit /b %errorlevel%
"@
[IO.File]::WriteAllText((Join-Path $taskRoot "$taskDir/compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
Push-Location -LiteralPath $taskRoot
try{& $env:ComSpec /d /c "$taskDir\compile.cmd";if($LASTEXITCODE -ne 0){throw 'Topology checks failed.'}}finally{Pop-Location}
