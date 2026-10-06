param(
    [switch]$Adapter,
    [switch]$RendererControls,
    [switch]$NodeEffects,
    [switch]$CurrentNodes,
    [switch]$Gpu,
    [switch]$EmissionTimeline,
    [switch]$EmissionCache,
    [switch]$NativeSync,
    [switch]$HostBootstrap,
    [switch]$GradientEditor,
    [switch]$ParticleTransform,
    [switch]$TraceIncludes,
    [ValidatePattern('^([A-Za-z0-9_./-]+)?$')][string]$PresetCatalog = '',
    [ValidateSet('Emitter', 'Particle', 'Force')][string]$NodeKind = 'Particle',
    [string]$MSVCVarsPath = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
)

$ErrorActionPreference = 'Stop'
if (([int]$RendererControls.IsPresent + [int]$Adapter.IsPresent + [int]$NodeEffects.IsPresent + [int]$CurrentNodes.IsPresent + [int]$Gpu.IsPresent + [int]$EmissionCache.IsPresent + [int]$EmissionTimeline.IsPresent + [int]$NativeSync.IsPresent + [int]$HostBootstrap.IsPresent + [int]$GradientEditor.IsPresent + [int]$ParticleTransform.IsPresent) -gt 1) { throw 'Select only one test scope.' }
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
    $testFolder = if ($ParticleTransform) { 'particle-transform-tests' } elseif ($GradientEditor) { 'gradient-editor-tests' } elseif ($HostBootstrap) { 'host-bootstrap-tests' } elseif ($EmissionCache) { 'emission-cache-tests' } elseif ($Gpu) { 'gpu-tests' } elseif ($EmissionTimeline) { 'emission-timeline-tests' } elseif ($RendererControls) { 'renderer-control-tests' } elseif ($NativeSync) { 'native-sync-tests' } elseif ($CurrentNodes) { 'current-node-tests' } elseif ($NodeEffects) { "node-effect-tests\$NodeKind" } elseif ($Adapter -or $RendererControls) { 'adapter-tests' } else { 'core-tests' }
    $buildDirectory = Join-Path $aliasRoot "artifacts\$testFolder"
    $null = New-Item -ItemType Directory -Force -Path (Join-Path $repositoryRoot "artifacts\$testFolder")

    $sources = @(
        'tests\core_tests.cpp',
        'src\core\Time.cpp',
        'src\core\Render.cpp',
        'src\core\SequenceCodec.cpp',
        'src\core\Settings.cpp',
        'src\core\Geometry.cpp',
        'src\core\Graph.cpp',
        'src\core\GraphConstruction.cpp',
        'src\core\GraphEvaluation.cpp',
        'src\core\EmitterHistory.cpp',
        'src\core\Random.cpp',
        'src\core\ParticleSimulation.cpp',
        'src\core\ParticleTransform.cpp',
        'src\core\PluginApi.cpp',
        'src\core\CpuRenderer.cpp',
        'src\core\SpriteScene.cpp'
    )

    if ($Adapter -or $RendererControls) {
        $sources[0] = 'tests\graph_parameter_tests.cpp'
        $sources += @('ae_plugin\GraphParameter.cpp', 'ae_plugin\GraphCarrier.cpp', 'ae_plugin\NativeGraphCommit.cpp', 'ae_plugin\Parameters.cpp', 'ae_plugin\WorldBridge.cpp')
    }
    if ($Gpu) { $sources[0] = 'tests\gpu_render_tests.cpp'; $sources += @('ae_plugin\GpuRender.cpp','ae_plugin\SmartRender.cpp','ae_plugin\WorldBridge.cpp','ae_plugin\NodeEffects.cpp') }
    if ($EmissionCache) { $sources[0] = 'tests\emission_cache_tests.cpp'; $sources += 'ae_plugin\NativeTemporalCache.cpp' }
    if ($CurrentNodes) { $sources[0] = 'tests\current_node_core_tests.cpp' }
    if ($EmissionTimeline) { $sources[0] = 'tests\emission_timeline_tests.cpp' }
    if ($NativeSync) {
        $sources[0] = 'tests\native_sync_tests.cpp'
        $sources += @('tests\camera_capture_tests.cpp', 'ae_plugin\NodeGraphSync.cpp', 'ae_plugin\Camera.cpp',
            'ae_plugin\GraphCarrier.cpp', 'ae_plugin\NativeGraphCommit.cpp', 'ae_plugin\GraphParameter.cpp', 'ae_plugin\NativeNodeGraph.cpp',
            'ae_plugin\Parameters.cpp', 'ae_plugin\WorldBridge.cpp', 'ae_plugin\EmitterHistoryCapture.cpp', 'ae_plugin\NativeTemporalCache.cpp', 'ae_plugin\NativeTemporalUI.cpp')
    }
    if ($NodeEffects) { $sources = @('tests\node_effect_tests.cpp', 'ae_plugin\NodeEffects.cpp', 'ae_plugin\GpuRender.cpp','ae_plugin\ParticleGradientUI.cpp') }
    if ($Gpu) { $sources += 'ae_plugin\ParticleGradientUI.cpp' }
    if ($GradientEditor) { $sources = @('tests\gradient_editor_tests.cpp','ae_plugin\ParticleGradientUI.cpp') }
    if ($HostBootstrap) { $sources = @('tests\host_bootstrap_tests.cpp') }
    if ($ParticleTransform) { $sources = @('tests\particle_transform_tests.cpp','src\core\ParticleTransform.cpp') }

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
    if ($Gpu -or $NodeEffects) { $responseLines += "/I `"$aliasRoot\artifacts\gpu-build\generated`"" }
    if ($RendererControls) { $responseLines += '/DSTARFIELD_TEST_RENDERER_CONTROLS' }
    if ($TraceIncludes) { $responseLines += '/showIncludes' }
    if ($EmissionCache -or $Gpu -or $Adapter -or $RendererControls -or $NodeEffects -or $NativeSync -or $HostBootstrap -or $GradientEditor) {
        $sdkHeaders = "$aliasRoot\AdobeSDK\May2023_AfterEffectsSDK\Examples\Headers"
        $responseLines += @('/DMSWindows', '/DWIN32', '/D_WINDOWS', '/D_CRT_SECURE_NO_WARNINGS',
            "/I `"$aliasRoot\ae_plugin`"", "/I `"$sdkHeaders`"", "/I `"$sdkHeaders\SP`"",
            "/I `"$aliasRoot\AdobeSDK\May2023_AfterEffectsSDK\Examples\Util`"")
    }
    if ($Gpu) { $responseLines += "/DSTARFIELD_NODE_KIND_EMITTER" }
    if ($NodeEffects) { $responseLines += "/DSTARFIELD_NODE_KIND_$($NodeKind.ToUpperInvariant())" }
    if ($NativeSync) { $responseLines += '/DSTARFIELD_NODE_KIND_EMITTER' }
    $responseLines += $sources | ForEach-Object { "`"$aliasRoot\$_`"" }

    $responsePath = Join-Path $buildDirectory 'core-tests.rsp'
    [System.IO.File]::WriteAllLines($responsePath, $responseLines, [System.Text.Encoding]::ASCII)

    $batchPath = Join-Path $buildDirectory 'build-core-tests.cmd'
    if($PresetCatalog -and -not $CurrentNodes){throw 'PresetCatalog belongs to CurrentNodes checks.'}
    $taskTestArgs = if($PresetCatalog){' "'+$PresetCatalog+'"'}else{''}
    $batch = @"
@echo off
call "$MSVCVarsPath" >nul
cl @$responsePath
if errorlevel 1 exit /b %errorlevel%
"$buildDirectory\core_tests.exe"$taskTestArgs
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
Write-Host "$testFolder passed."
