param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [ValidateSet('x64')][string]$Platform = 'x64',
    [string]$SdkPath = 'AdobeSDK\May2023_AfterEffectsSDK',
    [ValidatePattern('^[A-Za-z0-9._-]+$')][string]$ArtifactLabel = '2023',
    [string]$MSBuildPath = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe'
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not (Test-Path -LiteralPath $MSBuildPath)) { throw "MSBuild not found: $MSBuildPath" }

$driveLetter = @('Z', 'Y', 'X', 'W', 'V') |
    Where-Object { -not (Test-Path -LiteralPath "$_`:\") } |
    Select-Object -First 1
if (-not $driveLetter) { throw 'No free drive letter is available for the temporary build path.' }

$drive = "${driveLetter}:"
& $env:ComSpec /d /c "subst $drive `"$repositoryRoot`""
if ($LASTEXITCODE -ne 0) { throw "Could not map repository to $drive" }
try {
    $aliasSdkPath = Join-Path $drive $SdkPath
    $projectPath = Join-Path $drive 'ae_plugin\Starfield.vcxproj'
    $arguments = @($projectPath, '/t:Build', '/m', "/p:Configuration=$Configuration", "/p:Platform=$Platform", "/p:STARFIELD_AE_SDK_ROOT=$aliasSdkPath")
    if ($ArtifactLabel) {
        $artifactRoot = Join-Path $drive "artifacts\plugin\$ArtifactLabel"
        $outputDir = Join-Path $artifactRoot "$Platform\$Configuration"
        $intermediateDir = Join-Path $artifactRoot "obj\$Platform\$Configuration"
        $arguments += "/p:OutDir=$outputDir\"
        $arguments += "/p:IntDir=$intermediateDir\"
    }
    $arguments += '/v:minimal'
    & $MSBuildPath @arguments
    if ($LASTEXITCODE -ne 0) { throw "MSBuild failed with exit code $LASTEXITCODE" }

    # Publish a plainly named copy where a person can find it. MSBuild's OutDir is
    # artifacts/plugin/<label>/<platform>/<configuration>/, which is the right place to keep
    # one directory per SDK target but a tedious place to fetch an installable file from.
    if ($ArtifactLabel) {
        $publishedDir = Join-Path $repositoryRoot 'dist'
        $builtAex = Join-Path $repositoryRoot "artifacts\plugin\$ArtifactLabel\$Platform\$Configuration\StarfieldParticle.aex"
        if (-not (Test-Path -LiteralPath $builtAex)) { throw "Build reported success but $builtAex is missing." }
        New-Item -ItemType Directory -Force -Path $publishedDir | Out-Null
        Copy-Item -LiteralPath $builtAex -Destination (Join-Path $publishedDir 'StarfieldParticle.aex') -Force
        $builtPdb = [IO.Path]::ChangeExtension($builtAex, '.pdb')
        if (Test-Path -LiteralPath $builtPdb) {
            Copy-Item -LiteralPath $builtPdb -Destination (Join-Path $publishedDir 'StarfieldParticle.pdb') -Force
        }
        Write-Host ''
        Write-Host "Installable build: $(Join-Path $publishedDir 'StarfieldParticle.aex')" -ForegroundColor Green
        Write-Host 'Install or roll back with: powershell -ExecutionPolicy Bypass -File tools\Install-Plugin.ps1 [-Uninstall]'
    }
}
finally {
    & $env:ComSpec /d /c "subst $drive /d" | Out-Null
}
