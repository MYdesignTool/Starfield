param(
    [switch]$Run,
    [string]$MSVCVarsPath = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
)
$ErrorActionPreference = 'Stop'
$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$directory = 'artifacts\particle-texture-tests'
if (-not $Run) {
    Write-Host "Report: compile and run tests/particle_texture_tests.cpp under $directory with MSVC /MT. Add -Run to act."
    exit 0
}
if (-not (Test-Path -LiteralPath $MSVCVarsPath)) { throw "Missing MSVC setup: $MSVCVarsPath" }
$sources = @('Time','Render','SequenceCodec','Settings','Geometry','Graph','GraphConstruction',
    'GraphEvaluation','MotionGeometry','MotionPathTravel','EmitterHistory','Random','ParticleSimulation','ParticleTransform',
    'ParticleTexture','PluginApi','CpuRenderer','SpriteScene','ModelGeometry','ModelResources','ModelScene') | ForEach-Object { 'src\core\' + $_ + '.cpp' }
$arguments = @('/nologo','/std:c++20','/W4','/permissive-','/EHsc','/O2','/DNDEBUG','/MT','/Iinclude',
    ('/Fo' + $directory + '\'), ('/Fe' + $directory + '\particle_texture_tests.exe'), 'tests\particle_texture_tests.cpp') + $sources
Push-Location -LiteralPath $repositoryRoot
try {
    $null = New-Item -ItemType Directory -Force -Path $directory
    [IO.File]::WriteAllLines((Join-Path $repositoryRoot "$directory\compile.rsp"), $arguments, [Text.UTF8Encoding]::new($false))
    $batch = @"
@echo off
call "$MSVCVarsPath" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe @$directory\compile.rsp
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $repositoryRoot "$directory\compile.cmd"), $batch, [Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$directory\compile.cmd"
    if ($LASTEXITCODE -ne 0) { throw "Texture test compilation failed: $LASTEXITCODE" }
    & (Join-Path $repositoryRoot "$directory\particle_texture_tests.exe")
    if ($LASTEXITCODE -ne 0) { throw "Texture tests failed: $LASTEXITCODE" }
} finally { Pop-Location }
