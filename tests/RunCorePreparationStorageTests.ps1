param([switch]$Run,[ValidateSet('Before','After')][string]$Mode='After',[switch]$Sanitize)
$ErrorActionPreference='Stop'
if(-not $Run){Write-Output 'Report: compare frozen113/current Core values, bytes, allocations and failures; add -Run. No AE/device operations.';exit 0}
$taskRepo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Push-Location -LiteralPath $taskRepo
try {
    $taskLabel=$Mode.ToLowerInvariant()
    $taskDir='artifacts\perf-09-20261026\'+$taskLabel+'-tests'+$(if($Sanitize){'-asan'}else{''})
    $taskResults='artifacts\perf-09-20261026\'+$taskLabel+'-results'+$(if($Sanitize){'-asan'}else{''})
    New-Item -ItemType Directory -Force $taskDir,$taskResults | Out-Null
    $taskBase=if($Mode -eq 'Before'){'artifacts\prepared\perf08-native113-panel104\source'}else{'.'}
    $taskText=Get-Content CMakeLists.txt -Raw
    $taskBlock=[regex]::Match($taskText,'add_library\(starfield_core\b(?<body>[\s\S]*?)\)').Groups['body'].Value
    if(-not $taskBlock){throw 'Core source list not found.'}
    # The probe exercises complete preparation/evaluation, not raster/device
    # scene APIs. Those unused cpp files were not in the old adapter snapshot.
    $taskSources=@([regex]::Matches($taskBlock,'src/core/[A-Za-z0-9_]+\.cpp') | Where-Object {$_.Value -notmatch '/(CpuRenderer|ModelScene|SpriteScene)\.cpp$'} | ForEach-Object {Join-Path $taskBase $_.Value})
    foreach($taskSource in $taskSources){if(-not(Test-Path -LiteralPath $taskSource)){throw "Missing immutable input: $taskSource"}}
    $taskArgs=@('/nologo','/std:c++20','/W4','/WX','/permissive-','/EHsc','/O2','/MT','/D_CRT_SECURE_NO_WARNINGS',('/I'+(Join-Path $taskBase 'include')),('/Fo'+$taskDir+'\'),('/Fe'+$taskDir+'\tests.exe'),'tests\core_preparation_storage_tests.cpp')+$taskSources
    if($Sanitize){$taskArgs+=@('/fsanitize=address','/Zi',('/Fd'+$taskDir+'\compiler.pdb'))}
    [IO.File]::WriteAllLines((Join-Path $taskRepo "$taskDir/compile.rsp"),$taskArgs,[Text.UTF8Encoding]::new($false))
    $taskModeArg=if($Mode -eq 'Before'){'--baseline'}else{'--current'}
    @"
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe @$taskDir\compile.rsp
if errorlevel 1 exit /b %errorlevel%
$taskDir\tests.exe $taskModeArg $taskResults
exit /b %errorlevel%
"@ | Set-Content "$taskDir/compile.cmd" -Encoding ASCII
    & $env:ComSpec /d /c "$taskDir\compile.cmd"
    if($LASTEXITCODE -ne 0){throw "Core storage $Mode compilation/test failed."}
} finally {Pop-Location}
