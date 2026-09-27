param(
    [string]$MSVCVarsPath = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not (Test-Path -LiteralPath $MSVCVarsPath)) { throw "vcvars64.bat not found: $MSVCVarsPath" }

# The checkout path contains spaces and non-ASCII characters; the MSVC toolchain in
# this SDK pipeline is only reliable behind a mapped drive letter (same reason
# BuildWindows.ps1 maps one for the PiPL tool).
$driveLetter = @('Z', 'Y', 'X', 'W', 'V') |
    Where-Object { -not (Test-Path -LiteralPath "$_`:\") } |
    Select-Object -First 1
if (-not $driveLetter) { throw 'No free drive letter is available for the temporary build path.' }
$drive = "${driveLetter}:"

& $env:ComSpec /d /c "subst $drive `"$repositoryRoot`""
if ($LASTEXITCODE -ne 0) { throw "Could not map repository to $drive" }

try {
    $aliasRoot = $drive
    $buildDirectory = Join-Path $aliasRoot 'artifacts\core-tests'
    $null = New-Item -ItemType Directory -Force -Path (Join-Path $repositoryRoot 'artifacts\core-tests')

    $sources = @(
        'tests\core_tests.cpp',
        'src\core\Time.cpp',
        'src\core\Render.cpp',
        'src\core\Settings.cpp',
        'src\core\Geometry.cpp',
        'src\core\Random.cpp',
        'src\core\ParticleSimulation.cpp',
        'src\core\CpuRenderer.cpp'
    )

    $responseLines = @(
        '/nologo',
        '/std:c++20',
        '/W4',
        '/permissive-',
        '/EHsc',
        '/O2',
        '/DNDEBUG',
        "/I `"$aliasRoot\include`"",
        "/Fe:`"$buildDirectory\core_tests.exe`"",
        "/Fo:`"$buildDirectory\\`""
    )
    $responseLines += $sources | ForEach-Object { "`"$aliasRoot\$_`"" }

    $responsePath = Join-Path $buildDirectory 'core-tests.rsp'
    [System.IO.File]::WriteAllLines($responsePath, $responseLines, [System.Text.Encoding]::ASCII)

    $batchPath = Join-Path $buildDirectory 'build-core-tests.cmd'
    $batch = @"
@echo off
call "$MSVCVarsPath" >nul
cl @$responsePath
if errorlevel 1 exit /b %errorlevel%
"$buildDirectory\core_tests.exe"
exit /b %errorlevel%
"@
    [System.IO.File]::WriteAllText($batchPath, $batch, [System.Text.Encoding]::ASCII)

    & $env:ComSpec /d /c $batchPath
    $exitCode = $LASTEXITCODE
}
finally {
    & $env:ComSpec /d /c "subst $drive /d" | Out-Null
}

if ($exitCode -ne 0) { throw "Core self-tests failed with exit code $exitCode" }
Write-Host 'Core self-tests passed.'
