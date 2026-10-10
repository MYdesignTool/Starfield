param([switch]$Run,[switch]$Sanitize)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if(-not $Run){Write-Host 'Report: run six-choice Shape mapping and PF event/publication fixture with a resident Host token DLL; add -Run to act.';exit 0}
$taskDir=if($Sanitize){'artifacts\particle-shape-ui-asan-tests'}else{'artifacts\particle-shape-ui-tests'}
$taskVars='C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
Push-Location -LiteralPath $taskRepo
try {
    New-Item -ItemType Directory -Path "$taskDir\host","$taskDir\exe" -Force | Out-Null
    $taskBase='/nologo /std:c++20 /W4 /permissive- /EHsc /O2 /MT /Iae_plugin'
    if($Sanitize){$taskBase+=' /fsanitize=address /Zi'}
    $taskSdk='/DMSWindows /DWIN32 /D_WINDOWS /D_CRT_SECURE_NO_WARNINGS /DSTARFIELD_NODE_KIND_PARTICLE /Iinclude /IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers /IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers\SP /IAdobeSDK\May2023_AfterEffectsSDK\Examples\Util'
    $taskBatch=@"
@echo off
call "$taskVars" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe $taskBase /LD /Fo$taskDir\host\ /Fe$taskDir\host\StarfieldHost.aex /Fd$taskDir\host\compiler.pdb tests\ui_exclusion_module_fixture.cpp ae_plugin\UiExclusionHost.cpp /link /PDB:$taskDir\host\host.pdb
if errorlevel 1 exit /b %errorlevel%
cl.exe $taskBase $taskSdk /Fo$taskDir\exe\ /Fe$taskDir\tests.exe /Fd$taskDir\exe\compiler.pdb tests\particle_shape_ui_tests.cpp ae_plugin\ParticleShapeUI.cpp ae_plugin\NodeEffects.cpp /link user32.lib /PDB:$taskDir\exe\tests.pdb
if errorlevel 1 exit /b %errorlevel%
$taskDir\tests.exe $taskDir\host\StarfieldHost.aex
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDir\compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw 'Particle Shape UI compile/test failed.'}
} finally {Pop-Location}
