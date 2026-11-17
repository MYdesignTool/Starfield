param([switch]$Run)
$ErrorActionPreference='Stop'
if(-not $Run){Write-Host 'Report: actual frozen129/132 Core DLL serial/parallel ABI9 ownership, pixels and cancellation; add -Run. No AE or installation.';exit 0}
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskDirectory='artifacts\mfr-offline-handoff-20261116\core-dll-tests'
$taskHashes=@('207993D7EA0AE525CBD18CB359D300571D385D5E96AF4073FC1F0C84EAFA07B7','6BB2CE206259324812FA950BA14A9D20828FC35EF513E6722A1BF28FA520D3AB','3BE7B0AA090FE4C19A90BC27D153B82710A7877A3263D77C93CC5CFA3FF465AE','05A4BF5A6B972C50E8C75E8BBB9AA01575269D8C4AA9468C0551F12626EC2E3B')
Push-Location -LiteralPath $taskRepo
try {
    foreach($taskCandidate in @('stb20-native129-panel124-v2','mfr01-native132-panel124-host-qualification-v1')){
        $taskRoot="artifacts/prepared/$taskCandidate";$taskManifest=Get-Content -LiteralPath "$taskRoot/candidate.json" -Raw | ConvertFrom-Json
        $taskCore=@($taskManifest.files | Where-Object name -eq 'StarfieldCore.dll')
        if($taskCore.Count -ne 1 -or (Get-FileHash -LiteralPath "$taskRoot/native-bundle/StarfieldCore.dll").Hash -ne $taskCore[0].hash){throw 'Frozen Core generation hash mismatch.'}
    }
    for($taskIndex=0;$taskIndex -lt 4;$taskIndex++){if((Get-FileHash -LiteralPath "artifacts/ux-16-20261116/scene-$taskIndex.bin").Hash -ne $taskHashes[$taskIndex]){throw 'Independent UX-16 graph fixture hash mismatch.'}}
    New-Item -ItemType Directory -Force $taskDirectory | Out-Null
    $taskBatch=@"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe /nologo /std:c++20 /W4 /WX /permissive- /EHsc /O2 /MT /Iinclude /Fo$taskDirectory\ /Fe$taskDirectory\tests.exe tests\core_dll_concurrency_tests.cpp
if errorlevel 1 exit /b %errorlevel%
$taskDirectory\tests.exe
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $taskRepo "$taskDirectory/compile.cmd"),$taskBatch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$taskDirectory\compile.cmd"
    if($LASTEXITCODE -ne 0){throw 'Actual Core DLL concurrency fixture failed.'}
} finally {Pop-Location}
