param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [ValidateSet('x64')][string]$Platform = 'x64',
    [string]$SdkPath = 'AdobeSDK\AfterEffectsSDK_26.5_win',
    [ValidatePattern('^[A-Za-z0-9._-]+$')][string]$ArtifactLabel = '',
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
}
finally {
    & $env:ComSpec /d /c "subst $drive /d" | Out-Null
}
