param(
    [switch]$Run,
    [string]$MSVCVarsPath = 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
)
$ErrorActionPreference='Stop'
$repositoryRoot=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$directory='artifacts\cloud-gpu-driver-tests'
if (-not $Run) {
    Write-Host 'Report: compile a Cloud-only derivative of the existing standalone GPU fixture, then compare private CUDA/OpenCL devices with CPU; add -Run to act. No AE, installation, registry or caches change.'
    exit 0
}
function Replace-One([string]$source,[string]$before,[string]$after) {
    if(([regex]::Matches($source,[regex]::Escape($before))).Count -ne 1){throw "Fixture changed at: $before"}
    return $source.Replace($before,$after)
}
Push-Location -LiteralPath $repositoryRoot
try {
    $null=New-Item -ItemType Directory -Force -Path $directory
    # Read the committed blob, so other tasks' uncommitted fixture work and
    # untracked stubs are not inputs to this reproducible scope.
    $fixture=(& git show 'HEAD:tests/gpu_render_tests.cpp') -join "`n"
    if($LASTEXITCODE -ne 0){throw 'Missing committed GPU fixture'}
    [IO.File]::WriteAllText((Join-Path $repositoryRoot "$directory\fixture-source.cpp"),$fixture,[Text.UTF8Encoding]::new($false))
    if($fixture.Contains('capture_camera(PF_InData*,SfCoreRenderRequest& r) noexcept')) {
        $fixture=Replace-One $fixture 'capture_camera(PF_InData*,SfCoreRenderRequest& r) noexcept' 'capture_camera(PF_InData*,SfCoreRenderRequest& r,const PF_InData*) noexcept'
    }
    $fixture=$fixture.Replace('StarfieldCore_GetApi(3,sizeof(api),&api)','StarfieldCore_GetApi(SF_CORE_ABI_VERSION,sizeof(api),&api)')
    $fixture="#include `"MotionBlur.hpp`"`n"+$fixture
    $fixture=Replace-One $fixture 'void test_scene_api() {' "#include `"cloud_motion_gpu_cases.hpp`"`nvoid test_scene_api() {"
    $fixture=Replace-One $fixture 'v.tile_indices=s.indices.data();return v;' 'v.tile_indices=s.indices.data();v.cloud_circle_count=static_cast<unsigned>(s.cloud_circles.size());v.cloud_circles=s.cloud_circles.data();return v;'
    $fixture=Replace-One $fixture 'for(unsigned shape=0;shape<3;++shape) for(bool camera:{false,true})' 'for(unsigned shape:{2u}) for(double density:{0.,66.,1000.}) for(unsigned transfer:{0u,1u,2u,3u}) for(bool camera:{false,true})'
    $fixture=Replace-One $fixture 'auto r=request(shape,camera);r.frame.alpha_mode=alpha;' @'
auto r=request(shape,camera);r.frame.alpha_mode=alpha;
        auto graph=*r.graph;set(graph.nodes[1],core::graph_keys::kCloudDensity,density);
        set(graph.nodes[1],core::graph_keys::kParticleTransferMode,transfer);
        r.graph=std::make_shared<const core::Graph>(std::move(graph));
'@
    $fixture=Replace-One $fixture 'if(shape==0 && !camera && alpha==core::AlphaMode::straight) {' 'if(shape==2 && density==66 && transfer==0 && !camera && alpha==core::AlphaMode::straight) {'
    $fixture=Replace-One $fixture 'fail_allocation=2;' 'fail_allocation=4;'
    $fixture=Replace-One $fixture 'second allocation failure returned' 'fourth Cloud allocation failure returned'
    $fixture=Replace-One $fixture '    test_smartfx(in,out,setup_output.gpu_data,world);' ''
    $fixture=Replace-One $fixture '    benchmark(in,out,setup_output.gpu_data);' ''
    $fixture=Replace-One $fixture '    test_copy(in,setup_output.gpu_data,world);' ''
    $fixture=Replace-One $fixture '    test_scene_api();' '    test_cloud_motion_merge();'
    if(([regex]::Matches($fixture,'artifacts/gpu-tests/timings\.csv')).Count -ne 2){throw 'Fixture timing paths changed'}
    $fixture=$fixture.Replace('artifacts/gpu-tests/timings.csv','artifacts/cloud-gpu-driver-tests/timings.csv')
    $fixturePath=Join-Path $repositoryRoot "$directory\cloud_gpu_driver_tests.cpp"
    [IO.File]::WriteAllText($fixturePath,$fixture,[Text.UTF8Encoding]::new($false))
    $fingerprints=@("$directory\fixture-source.cpp",'tests\cloud_motion_gpu_cases.hpp','tests\RunCloudGpuDriverTests.ps1',"$directory\cloud_gpu_driver_tests.cpp",'ae_plugin\GpuRender.cpp','ae_plugin\gpu\SpriteKernel.h') | ForEach-Object {
        [ordered]@{path=$_;sha256=(Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash}
    }
    $fingerprints | ConvertTo-Json | Set-Content -LiteralPath "$directory\fixture-hashes.json" -Encoding UTF8
    $sources=@('Time','Render','SequenceCodec','Settings','Geometry','Graph','GraphConstruction','GraphEvaluation','MotionGeometry','MotionPathTravel','EmitterHistory','Random','ParticleSimulation','ParticleTransform','ParticleTexture','PluginApi','CpuRenderer','SpriteScene','ModelGeometry','ModelResources','ModelScene') | ForEach-Object {'src\core\'+$_+'.cpp'}
    $scopeStubs=@'
#include "EmitterHistory.hpp"
#include "EditorPresetPicker.hpp"
namespace starfield::adapter {
PF_Err capture_motion_particles(PF_InData*,PF_OutData*,const core::Graph&,A_long,A_long,
    std::span<const core::RationalTime>,const core::Cancellation&,std::vector<CapturedParticleFrame>&) noexcept {return PF_Err_BAD_CALLBACK_PARAM;}
bool choose_curve_preset(PF_InData*,core::AgeCurve&) noexcept {return false;}
bool choose_gradient_preset(PF_InData*,core::ColorGradient&) noexcept {return false;}
}
'@
    [IO.File]::WriteAllText((Join-Path $repositoryRoot "$directory\scope-stubs.cpp"),$scopeStubs,[Text.UTF8Encoding]::new($false))
    $sources+=@('ae_plugin\GpuRender.cpp','ae_plugin\SmartRender.cpp','ae_plugin\TextureResources.cpp','ae_plugin\WorldBridge.cpp','ae_plugin\NodeEffects.cpp','ae_plugin\ParticleGradientUI.cpp','ae_plugin\MotionBlur.cpp',"$directory\scope-stubs.cpp")
    $headers='AdobeSDK\May2023_AfterEffectsSDK\Examples\Headers'
    $compilerArguments=@('/nologo','/std:c++20','/W4','/permissive-','/EHsc','/O2','/DNDEBUG','/MT','/DMSWindows','/DWIN32','/D_WINDOWS','/D_CRT_SECURE_NO_WARNINGS','/DSTARFIELD_NODE_KIND_EMITTER','/DSTARFIELD_TEST_GPU',
        '/Iinclude','/Iae_plugin','/Itests','/Iartifacts\gpu-build\generated',('/I'+$headers),('/I'+$headers+'\SP'),'/IAdobeSDK\May2023_AfterEffectsSDK\Examples\Util',
        ('/Fo'+$directory+'\'),('/Fe'+$directory+'\cloud_gpu_driver_tests.exe'),("$directory\cloud_gpu_driver_tests.cpp"))+$sources
    [IO.File]::WriteAllLines((Join-Path $repositoryRoot "$directory\compile.rsp"),$compilerArguments,[Text.UTF8Encoding]::new($false))
    $batch=@"
@echo off
call "$MSVCVarsPath" >nul
if errorlevel 1 exit /b %errorlevel%
cl.exe @$directory\compile.rsp
if errorlevel 1 exit /b %errorlevel%
"$directory\cloud_gpu_driver_tests.exe"
exit /b %errorlevel%
"@
    [IO.File]::WriteAllText((Join-Path $repositoryRoot "$directory\compile.cmd"),$batch,[Text.Encoding]::ASCII)
    & $env:ComSpec /d /c "$directory\compile.cmd"
    if($LASTEXITCODE -ne 0){throw "Cloud GPU driver checks failed: $LASTEXITCODE"}
} finally {Pop-Location}
