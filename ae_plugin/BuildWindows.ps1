param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [ValidateSet('x64')][string]$Platform = 'x64',
    [string]$SdkPath = 'AdobeSDK\May2023_AfterEffectsSDK',
    [ValidatePattern('^[A-Za-z0-9._-]+$')][string]$ArtifactLabel = '2023',
    [switch]$CoreOnly,
    [switch]$NoRuntimePublish,
    [switch]$NoDistPublish,
    [string]$PythonPath = (Join-Path $env:USERPROFILE '.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'),
    [string]$NvrtcPath = 'artifacts\gpu-build\nvrtc-12.4.127\nvidia\cuda_nvrtc\bin\nvrtc64_120_0.dll',
    [string]$MSBuildPath = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe'
)

$ErrorActionPreference = 'Stop'
function Assert-RuntimeNativePair([string]$RepositoryRoot,[string]$Label,[string]$Architecture,[string]$BuildConfiguration) {
    # The artifact fingerprint proves which AEX was BUILT, not which AEX is
    # installed. An unpublished full build must not enable an incompatible hot
    # Core update. Verify the complete native pair before touching the selector.
    foreach ($taskModule in @('StarfieldParticle','StarfieldEmitter','StarfieldParticleNode','StarfieldForce','StarfieldTransform','StarfieldHost')) {
        $taskBuiltNative = Join-Path $RepositoryRoot "artifacts\plugin\$Label\$Architecture\$BuildConfiguration\$taskModule.aex"
        $taskInstalledNative = Join-Path $RepositoryRoot "dist\$taskModule.aex"
        if (-not (Test-Path -LiteralPath $taskBuiltNative) -or -not (Test-Path -LiteralPath $taskInstalledNative) -or
            (Get-FileHash -LiteralPath $taskBuiltNative -Algorithm SHA256).Hash -ne
            (Get-FileHash -LiteralPath $taskInstalledNative -Algorithm SHA256).Hash) {
            throw "Runtime publication requires the matching installed native bundle ($taskModule). Use a paired deployment, or -NoRuntimePublish for a candidate."
        }
    }
}
Import-Module Microsoft.PowerShell.Utility
$repositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not $CoreOnly -and -not $NoDistPublish -and
    (Get-Process AfterFX, AfterFX_64 -ErrorAction SilentlyContinue)) {
    throw 'Close AE before publishing AEX files into dist, or build with -NoDistPublish -NoRuntimePublish.'
}
if (-not (Test-Path -LiteralPath $MSBuildPath)) { throw "MSBuild not found: $MSBuildPath" }

if (-not $CoreOnly) {
    & (Join-Path $repositoryRoot 'tools\Generate-EditorPresets.ps1') -Generate
    $resolvedNvrtc = Join-Path $repositoryRoot $NvrtcPath
    if (-not (Test-Path -LiteralPath $resolvedNvrtc)) { throw 'Missing workspace NVRTC build input. Run tools/Prepare-GpuBuildInputs.ps1 -Download first; no system installation is needed.' }
    & $PythonPath (Join-Path $repositoryRoot 'tools\Build-GpuKernels.py') --nvrtc $resolvedNvrtc --output (Join-Path $repositoryRoot 'artifacts\gpu-build\generated\GpuKernels.hpp')
    if ($LASTEXITCODE -ne 0) { throw 'GPU kernel compilation failed.' }
}

# A CoreOnly build is safe only while every source that feeds the installed AEX
# and the cross-DLL ABI remains unchanged from the last full build. Algorithm
# sources linked only into the DLL (CpuRenderer and PluginApi.cpp) are absent.
# Historical graph/simulation sources also feed the AEX and must be fingerprinted.
$adapterInputs = @(
    'include\starfield\core\ParticleBirth.hpp',
    'ae_plugin\CoreLoader.cpp', 'ae_plugin\CoreLoader.hpp',
    'ae_plugin\Camera.cpp', 'ae_plugin\Camera.hpp',
    'ae_plugin\MotionBlur.cpp', 'ae_plugin\MotionBlur.hpp', 'include\starfield\core\MotionBlur.hpp',
    'ae_plugin\Diagnostics.cpp', 'ae_plugin\Diagnostics.hpp',
    'ae_plugin\EffectMain.cpp', 'ae_plugin\GraphCarrier.cpp', 'ae_plugin\GraphCarrier.hpp',
    'ae_plugin\NativeGraphCommit.cpp', 'ae_plugin\NativeGraphCommit.hpp',
    'ae_plugin\GraphParameter.cpp', 'ae_plugin\GraphParameter.hpp',
    'ae_plugin\NativeTemporalCache.cpp', 'ae_plugin\NativeTemporalCache.hpp',
    'ae_plugin\NativeTemporalUI.cpp', 'ae_plugin\NativeTemporalUI.hpp',
    'ae_plugin\NativeBootstrap.hpp', 'ae_plugin\StarfieldHost.cpp',
    'ae_plugin\StarfieldHostPiPL.r', 'ae_plugin\StarfieldHost.vcxproj',
    'ae_plugin\NativeNodeGraph.cpp', 'ae_plugin\NativeNodeGraph.hpp', 'ae_plugin\NodeRecord.hpp',
    'ae_plugin\TransformLayout.hpp', 'ae_plugin\TransformBinding.hpp',
    'ae_plugin\Parameters.cpp', 'ae_plugin\Parameters.hpp',
    'ae_plugin\SmartRender.cpp', 'ae_plugin\SmartRender.hpp',
    'ae_plugin\TextureResources.cpp', 'ae_plugin\TextureResources.hpp',
    'ae_plugin\PresetsUI.cpp', 'ae_plugin\PresetsUI.hpp', 'ae_plugin\MainLauncher.hpp',
    'ae_plugin\TextureLayerInventory.hpp',
    'ae_plugin\GpuRender.cpp', 'ae_plugin\GpuRender.hpp', 'ae_plugin\gpu\SpriteKernel.h',
    'tools\Build-GpuKernels.py',
    'ae_plugin\EmitterHistoryCapture.cpp', 'ae_plugin\EmitterHistory.hpp',
    'ae_plugin\WorldBridge.cpp', 'ae_plugin\WorldBridge.hpp',
    'ae_plugin\PluginFlags.h', 'ae_plugin\PluginVersion.h', 'ae_plugin\BuildPiPL.ps1',
    'ae_plugin\StarfieldPiPL.r', 'ae_plugin\Starfield.vcxproj',
    'ae_plugin\NodeEffectFlags.h', 'ae_plugin\NodeEffectMain.cpp',
    'ae_plugin\ParticleGradientUI.cpp', 'ae_plugin\ParticleGradientUI.hpp', 'ae_plugin\ParticleLayout.hpp', 'ae_plugin\GradientEditorModel.hpp',
    'ae_plugin\EditorPresetPicker.cpp', 'ae_plugin\EditorPresetPicker.hpp',
    'schema\editor-presets.json', 'tools\Generate-EditorPresets.ps1',
    'ae_plugin\NodeEffects.cpp', 'ae_plugin\NodeEffects.hpp', 'ae_plugin\NodeGraphSync.cpp',
    'ae_plugin\NodeGraphSync.hpp', 'ae_plugin\EffectReveal.hpp', 'ae_plugin\NodeEffect.vcxproj',
    'ae_plugin\NodeEmitterPiPL.r', 'ae_plugin\NodeParticlePiPL.r', 'ae_plugin\NodeForcePiPL.r',
    'ae_plugin\NodeTransformPiPL.r',
    'include\starfield\core\AgeCurve.hpp',
    'include\starfield\core\Error.hpp', 'include\starfield\core\Geometry.hpp',
    'include\starfield\core\Graph.hpp', 'include\starfield\core\GraphEvaluation.hpp',
    'include\starfield\core\EmitterHistory.hpp',
    'include\starfield\core\EmissionTimeline.hpp',
    'include\starfield\core\ColorGradient.hpp', 'src\core\TemporalEvaluation.hpp', 'src\core\TransformEvaluation.hpp',
    'include\starfield\core\ParticleSimulation.hpp',
    'include\starfield\core\ParticleTransform.hpp',
    'include\starfield\core\ParticleCloud.hpp',
    'include\starfield\core\Random.hpp',
    'include\starfield\core\PluginApi.h', 'include\starfield\core\Render.hpp',
    'include\starfield\core\SequenceCodec.hpp', 'include\starfield\core\Settings.hpp',
    'include\starfield\core\Time.hpp', 'schema\parameters.json',
    'schema\node-parameters.json',
    'src\core\Geometry.cpp', 'src\core\Graph.cpp',
    'src\core\GraphConstruction.cpp', 'src\core\Render.cpp',
    'src\core\GraphEvaluation.cpp', 'src\core\EmitterHistory.cpp',
    'src\core\ParticleSimulation.cpp', 'src\core\ParticleTransform.cpp', 'src\core\Random.cpp',
    'src\core\ParticleTexture.cpp', 'include\starfield\core\ParticleTexture.hpp',
    'src\core\SequenceCodec.cpp', 'src\core\Settings.cpp', 'src\core\Time.cpp'
)
$adapterFingerprint = ($adapterInputs | ForEach-Object {
    $path = Join-Path $repositoryRoot $_
    if (-not (Test-Path -LiteralPath $path)) { throw "Missing adapter input: $path" }
    "$_ $((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash)"
}) -join "`n"
$fingerprintPath = Join-Path $repositoryRoot "artifacts\plugin\$ArtifactLabel\$Platform\$Configuration\adapter-inputs.sha256"
if ($CoreOnly) {
    if (-not (Test-Path -LiteralPath $fingerprintPath) -or
        [IO.File]::ReadAllText($fingerprintPath) -ne $adapterFingerprint) {
        throw 'Adapter or C ABI sources changed since the last full build. Rebuild and install the matching AEX before CoreOnly.'
    }
}

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
    $coreProjectPath = Join-Path $drive 'ae_plugin\StarfieldCore.vcxproj'
    $coreArguments = @($coreProjectPath, '/t:Build', '/m', "/p:Configuration=$Configuration", "/p:Platform=$Platform")
    $arguments = @($projectPath, '/t:Build', '/m', "/p:Configuration=$Configuration", "/p:Platform=$Platform", "/p:STARFIELD_AE_SDK_ROOT=$aliasSdkPath")
    if ($ArtifactLabel) {
        $artifactRoot = Join-Path $drive "artifacts\plugin\$ArtifactLabel"
        $outputDir = Join-Path $artifactRoot "$Platform\$Configuration"
        $intermediateDir = Join-Path $artifactRoot "obj\$Platform\$Configuration"
        $arguments += "/p:OutDir=$outputDir\"
        $arguments += "/p:IntDir=$intermediateDir\"
        $coreOutputDir = Join-Path $drive "artifacts\core-dll\$ArtifactLabel\$Platform\$Configuration"
        $coreIntermediateDir = Join-Path $drive "artifacts\core-dll\$ArtifactLabel\obj\$Platform\$Configuration"
        $coreArguments += "/p:OutDir=$coreOutputDir\"
        $coreArguments += "/p:IntDir=$coreIntermediateDir\"
    }
    $coreArguments += '/v:minimal'
    & $MSBuildPath @coreArguments
    if ($LASTEXITCODE -ne 0) { throw "Core DLL MSBuild failed with exit code $LASTEXITCODE" }

    $builtCore = Join-Path $repositoryRoot "artifacts\core-dll\$ArtifactLabel\$Platform\$Configuration\StarfieldCore.dll"
    if (-not (Test-Path -LiteralPath $builtCore)) { throw "Core build reported success but $builtCore is missing." }

    if (-not $CoreOnly) {
    $arguments += '/v:minimal'
    & $MSBuildPath @arguments
    if ($LASTEXITCODE -ne 0) { throw "MSBuild failed with exit code $LASTEXITCODE" }

    foreach ($nodeKind in @('Emitter', 'Particle', 'Force', 'Transform')) {
        $nodeOutputDir = Join-Path $drive "artifacts\plugin\$ArtifactLabel\$Platform\$Configuration"
        $nodeIntermediateDir = Join-Path $drive "artifacts\plugin\$ArtifactLabel\obj\$Platform\$Configuration\node-$nodeKind"
        $nodeArguments = @(
            (Join-Path $drive 'ae_plugin\NodeEffect.vcxproj'), '/t:Build', '/m',
            "/p:Configuration=$Configuration", "/p:Platform=$Platform", "/p:NodeKind=$nodeKind",
            "/p:STARFIELD_AE_SDK_ROOT=$aliasSdkPath", "/p:OutDir=$nodeOutputDir\", "/p:IntDir=$nodeIntermediateDir\",
            '/v:minimal'
        )
        & $MSBuildPath @nodeArguments
        if ($LASTEXITCODE -ne 0) { throw "$nodeKind node effect MSBuild failed with exit code $LASTEXITCODE" }
    }
    $hostArguments = @((Join-Path $drive 'ae_plugin\StarfieldHost.vcxproj'), '/t:Build', '/m',
        "/p:Configuration=$Configuration", "/p:Platform=$Platform", "/p:STARFIELD_AE_SDK_ROOT=$aliasSdkPath",
        "/p:OutDir=$outputDir\", "/p:IntDir=$intermediateDir\host\", '/v:minimal')
    & $MSBuildPath @hostArguments
    if ($LASTEXITCODE -ne 0) { throw "General AEGP MSBuild failed with exit code $LASTEXITCODE" }
    [IO.File]::WriteAllText($fingerprintPath, $adapterFingerprint, [Text.Encoding]::ASCII)
    }

    # Runtime publication is last: a failed build must leave the currently
    # selected DLL unchanged. Use NoRuntimePublish for an uninstalled candidate
    # so the single linked bundle keeps selecting its current generation.
    if (-not $NoRuntimePublish) {
    if ($CoreOnly -or $NoDistPublish -or -not $ArtifactLabel) {
        Assert-RuntimeNativePair $repositoryRoot $ArtifactLabel $Platform $Configuration
    }
    $runtimeDir = Join-Path $repositoryRoot 'dist\StarfieldRuntime'
    New-Item -ItemType Directory -Force -Path $runtimeDir | Out-Null
    $builtCoreHash = (Get-FileHash -LiteralPath $builtCore -Algorithm SHA256).Hash
    $coreHash = $builtCoreHash.Substring(0, 16)
    $runtimeName = "StarfieldCore-$coreHash.dll"
    $runtimeCore = Join-Path $runtimeDir $runtimeName
    if (Test-Path -LiteralPath $runtimeCore) {
        if ((Get-FileHash -LiteralPath $runtimeCore -Algorithm SHA256).Hash -ne $builtCoreHash) {
            throw "Existing runtime DLL has the expected name but different contents: $runtimeCore"
        }
    } else {
        $runtimeTemp = Join-Path $runtimeDir "$runtimeName.$PID.tmp"
        try {
            Copy-Item -LiteralPath $builtCore -Destination $runtimeTemp -Force
            if ((Get-FileHash -LiteralPath $runtimeTemp -Algorithm SHA256).Hash -ne $builtCoreHash) {
                throw "Runtime DLL copy failed hash verification: $runtimeTemp"
            }
            [IO.File]::Move($runtimeTemp, $runtimeCore)
        } finally {
            if (Test-Path -LiteralPath $runtimeTemp) { Remove-Item -LiteralPath $runtimeTemp }
        }
    }
    $manifest = Join-Path $runtimeDir 'current.txt'
    $manifestTemp = Join-Path $runtimeDir "current.$PID.tmp"
    $manifestBackup = Join-Path $runtimeDir "current.$PID.previous"
    [IO.File]::WriteAllText($manifestTemp, "$runtimeName`n", [Text.Encoding]::ASCII)
    try {
        if ([IO.File]::Exists($manifest)) {
            [IO.File]::Replace($manifestTemp, $manifest, $manifestBackup)
        } else {
            [IO.File]::Move($manifestTemp, $manifest)
        }
    } finally {
        if (Test-Path -LiteralPath $manifestTemp) { Remove-Item -LiteralPath $manifestTemp }
        if (Test-Path -LiteralPath $manifestBackup) { Remove-Item -LiteralPath $manifestBackup }
    }
    Write-Host "Selected runtime core: $runtimeCore" -ForegroundColor Green
    } else {
        Write-Host 'Runtime selection unchanged (-NoRuntimePublish).' -ForegroundColor Yellow
    }

    # Publish a plainly named copy where a person can find it. MSBuild's OutDir is
    # artifacts/plugin/<label>/<platform>/<configuration>/, which is the right place to keep
    # one directory per SDK target but a tedious place to fetch an installable file from.
    if ($ArtifactLabel -and -not $CoreOnly -and -not $NoDistPublish) {
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
        foreach ($nodeModule in @('StarfieldEmitter', 'StarfieldParticleNode', 'StarfieldForce', 'StarfieldTransform', 'StarfieldHost')) {
            $builtNodeAex = Join-Path $repositoryRoot "artifacts\plugin\$ArtifactLabel\$Platform\$Configuration\$nodeModule.aex"
            if (-not (Test-Path -LiteralPath $builtNodeAex)) { throw "Node build reported success but $builtNodeAex is missing." }
            Copy-Item -LiteralPath $builtNodeAex -Destination (Join-Path $publishedDir "$nodeModule.aex") -Force
            $builtNodePdb = [IO.Path]::ChangeExtension($builtNodeAex, '.pdb')
            if (Test-Path -LiteralPath $builtNodePdb) {
                Copy-Item -LiteralPath $builtNodePdb -Destination (Join-Path $publishedDir "$nodeModule.pdb") -Force
            }
            Write-Host "Node module: $(Join-Path $publishedDir "$nodeModule.aex")" -ForegroundColor Green
        }
        Copy-Item -LiteralPath $builtCore -Destination (Join-Path $publishedDir 'StarfieldCore.dll') -Force
        $builtCorePdb = [IO.Path]::ChangeExtension($builtCore, '.pdb')
        if (Test-Path -LiteralPath $builtCorePdb) {
            Copy-Item -LiteralPath $builtCorePdb -Destination (Join-Path $publishedDir 'StarfieldCore.pdb') -Force
        }
        Write-Host "Core DLL: $(Join-Path $publishedDir 'StarfieldCore.dll')" -ForegroundColor Green
    }
}
finally {
    & $env:ComSpec /d /c "subst $drive /d" | Out-Null
}
