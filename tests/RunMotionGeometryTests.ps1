param([switch]$Run,[switch]$Sanitize)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskDir=if($Sanitize){'artifacts\motion-geometry-tests-asan'}else{'artifacts\motion-geometry-tests'}
if(-not $Run){Write-Host 'Report: build/run independent Motion geometry with MSVC /MT. Add -Run to act.';exit 0}
$taskVars='C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
Push-Location -LiteralPath $taskRepo
try{
    New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
    $taskArgs=@('/nologo','/std:c++20','/W4','/permissive-','/EHsc','/O2','/MT','/Iinclude',
        ('/Fo'+$taskDir+'\'),('/Fe'+$taskDir+'\tests.exe'),'tests\motion_geometry_tests.cpp','src\core\MotionGeometry.cpp')
    if($Sanitize){$taskArgs += @('/fsanitize=address','/Zi',('/Fd'+$taskDir+'\compile.pdb'),('/link /DEBUG /PDB:'+ $taskDir+'\tests.pdb'))}
    [IO.File]::WriteAllLines((Join-Path $taskRepo "$taskDir\compile.rsp"),$taskArgs,[Text.UTF8Encoding]::new($false))
    $taskBatch=@"
@echo off
call "$taskVars" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe @$taskDir\compile.rsp
if errorlevel 1 exit /b %errorlevel%
"$taskDir\tests.exe"
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDir\compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw "Motion geometry compilation or tests failed (exit $LASTEXITCODE)."}
}finally{Pop-Location}
