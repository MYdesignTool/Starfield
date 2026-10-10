param([switch]$Run,[switch]$Sanitize)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if(-not $Run){Write-Host 'Report: compile/run private Host/effect UI exclusion in isolated DLLs and a message-only test window; add -Run to act.';exit 0}
$taskDir=if($Sanitize){'artifacts\model-ui-exclusion-asan-tests'}else{'artifacts\model-ui-exclusion-tests'}
$taskVars='C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
Push-Location -LiteralPath $taskRepo
try {
    New-Item -ItemType Directory -Path "$taskDir\paired","$taskDir\mismatched","$taskDir\exe" -Force | Out-Null
    $taskBase='/nologo /std:c++20 /W4 /WX /permissive- /EHsc /O2 /MT /Iae_plugin'
    if($Sanitize){$taskBase+=' /fsanitize=address /Zi'}
    $taskBatch=@"
@echo off
call "$taskVars" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe $taskBase /LD /DSTARFIELD_MISMATCHED_HOST /Fo$taskDir\mismatched\ /Fe$taskDir\mismatched\StarfieldHost.aex /Fd$taskDir\mismatched\compiler.pdb tests\ui_exclusion_module_fixture.cpp /link /PDB:$taskDir\mismatched\host.pdb
if errorlevel 1 exit /b %errorlevel%
cl.exe $taskBase /LD /Fo$taskDir\paired\ /Fe$taskDir\paired\StarfieldHost.aex /Fd$taskDir\paired\compiler.pdb tests\ui_exclusion_module_fixture.cpp ae_plugin\UiExclusionHost.cpp /link /PDB:$taskDir\paired\host.pdb
if errorlevel 1 exit /b %errorlevel%
cl.exe $taskBase /Fo$taskDir\exe\ /Fe$taskDir\tests.exe /Fd$taskDir\exe\compiler.pdb tests\model_ui_exclusion_tests.cpp /link user32.lib /PDB:$taskDir\exe\tests.pdb
if errorlevel 1 exit /b %errorlevel%
$taskDir\tests.exe $taskDir\mismatched\StarfieldHost.aex $taskDir\paired\StarfieldHost.aex
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDir\compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw 'Model UI exclusion compile/test failed.'}
} finally {Pop-Location}
