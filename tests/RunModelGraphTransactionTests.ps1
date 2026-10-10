param([switch]$Run,[switch]$Sanitize)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if(-not $Run){Write-Host 'Report: compile/run complete Model transaction with actual backup/SFMG1 code and fake May2023 suites; add -Run to act.';exit 0}
$taskDir=if($Sanitize){'artifacts\model-graph-transaction-asan-tests'}else{'artifacts\model-graph-transaction-tests'}
$taskVars='C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
Push-Location -LiteralPath $taskRepo
try {
    New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
    $taskArgs=@('/nologo','/std:c++20','/W4','/WX','/permissive-','/EHsc','/O2','/MT','/DMSWindows','/DWIN32','/D_WINDOWS',
        '/D_CRT_SECURE_NO_WARNINGS','/DSTARFIELD_MODEL_GRAPH_TRANSACTION_TEST','/Iinclude','/Iae_plugin',
        '/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers','/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Headers\SP',
        ('/Fo'+$taskDir+'\'),('/Fe'+$taskDir+'\tests.exe'),'tests\effect_graph_backup_tests.cpp',
        'ae_plugin\EffectGraphBackup.cpp','ae_plugin\ModelGraphTransaction.cpp','src\core\ModelResources.cpp',
        'src\core\ModelGeometry.cpp','src\core\ModelScene.cpp','src\core\ParticleTransform.cpp','src\core\Render.cpp','src\core\Time.cpp')
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
    if($LASTEXITCODE -ne 0){throw 'Model graph transaction compile/test failed.'}
} finally {Pop-Location}
