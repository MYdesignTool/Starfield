param([switch]$Run,[Parameter(Mandatory=$true)][string]$PanelRoot)
$ErrorActionPreference='Stop'
if(-not $Run){Write-Host 'Report: complete isolated Model panel/gateway regression and actual registration DOM matrix; add -Run. No AE project, process or installed files are changed.';exit 0}
$taskRepo=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$taskPanel=(Resolve-Path -LiteralPath $PanelRoot).Path
$taskPrefix=[IO.Path]::GetFullPath((Join-Path $taskRepo 'artifacts/prepared'))+'\'
if(-not $taskPanel.StartsWith($taskPrefix,[StringComparison]::OrdinalIgnoreCase)){throw 'Audit an isolated prepared panel under artifacts/prepared.'}
$taskLogRoot=Join-Path $taskRepo ('artifacts/m3-17-panel-audit-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $taskLogRoot -Force | Out-Null
function Run-AuditCommand([string]$taskName,[scriptblock]$taskCommand){
    $taskPreviousAction=$ErrorActionPreference
    try{$ErrorActionPreference='Continue'; & $taskCommand *> (Join-Path $taskLogRoot ($taskName+'.log'));$taskCode=$LASTEXITCODE}
    finally{$ErrorActionPreference=$taskPreviousAction}
    if($taskCode -ne 0){throw "Audit failed at $taskName; $taskLogRoot"}
    Get-Content -LiteralPath (Join-Path $taskLogRoot ($taskName+'.log')) -Tail 1
}
Push-Location $taskRepo
try{
    $env:STARFIELD_PANEL_ROOT=$taskPanel
    $env:STARFIELD_NODE_CATALOG_DIR=(Join-Path $taskRepo 'artifacts')
    foreach($taskKind in @('Emitter','Particle','Force','Transform')){
        $env:STARFIELD_TEST_NODE_CATALOG='1'
        & powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1 -NodeEffects -NodeKind $taskKind *> ('artifacts/m3-17-full-audit-catalog-'+$taskKind.ToLowerInvariant()+'.log')
        if($LASTEXITCODE){throw "Actual $taskKind registration test failed."}
    }
    & powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelNativeBindingTests.ps1 -Run *> artifacts/m3-17-full-audit-catalog-model.log
    if($LASTEXITCODE){throw 'Actual Model registration/binding test failed.'}
    if(Test-Path -LiteralPath (Join-Path $taskPanel 'js/graph_edits.js')){
        $taskHasMaterial=[IO.File]::ReadAllText((Join-Path $taskPanel 'js/graph_edits.js')).Contains('material:{version:1')
        if($taskHasMaterial){
            & powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunMaterialNativeBindingTests.ps1 -Run *> artifacts/m3-17-full-audit-catalog-material.log
            if($LASTEXITCODE){throw 'Actual Material registration/binding test failed.'}
            $env:STARFIELD_DOM_MATERIAL='1'
            Run-AuditCommand 'material_panel_tests' { & node tests/material_panel_tests.js }
        }
    }
    Remove-Item Env:STARFIELD_TEST_NODE_CATALOG
    Run-AuditCommand 'acceptance-fixtures' { & node tools/prepare_model_acceptance.cjs --prepare $taskPanel artifacts/model-acceptance-panel71 }
    Run-AuditCommand 'acceptance-harness' { & node --expose-internals tests/model_acceptance_harness_tests.js }
    foreach($taskTest in @('native_record_contract_tests','model_graph_transport_tests','model_gateway_lifecycle_tests','model_asset_gateway_tests','model_panel_author_tests','model_preset_file_tests','panel_native_node_gateway_tests')){
        Run-AuditCommand $taskTest { & node ('tests/'+$taskTest+'.js') }
    }
    Run-AuditCommand 'extendscript_syntax_tests' { & node --expose-internals tests/extendscript_syntax_tests.js }
    $env:STARFIELD_DOM_FULL_GRAPH='1'
    foreach($taskWrappers in @('0','1')){foreach($taskInvalidate in @('0','1')){foreach($taskLegacy in @('','emitter','particle','force')){
        $env:STARFIELD_DOM_STABLE_EFFECTS=$taskWrappers
        $env:STARFIELD_DOM_INVALIDATE_COMMIT=$taskInvalidate
        $env:STARFIELD_DOM_LEGACY_KIND=$taskLegacy
        $env:STARFIELD_DOM_REORDER_CONNECTIONS='1'
        Run-AuditCommand ('dom-'+$taskWrappers+'-'+$taskInvalidate+'-'+$(if($taskLegacy){$taskLegacy}else{'current'})) { & node tests/model_graph_gateway_dom_tests.js }
    }}}
    $env:STARFIELD_DOM_LEGACY_KIND=''
    $env:STARFIELD_DOM_STALE_GATEWAY_API='1'
    Run-AuditCommand 'dom-reloaded-gateway' { & node tests/model_graph_gateway_dom_tests.js }
    $env:STARFIELD_DOM_STALE_GATEWAY_API=''
    $env:STARFIELD_DOM_RELOAD_ON_COMMIT='1'
    Run-AuditCommand 'dom-gateway-reloaded-during-commit' { & node tests/model_graph_gateway_dom_tests.js }
    Write-Output "Model panel audit passed; logs: $taskLogRoot; actual AE acceptance remains separate."
}finally{
    foreach($taskVariable in @('STARFIELD_PANEL_ROOT','STARFIELD_NODE_CATALOG_DIR','STARFIELD_TEST_NODE_CATALOG','STARFIELD_DOM_FULL_GRAPH','STARFIELD_DOM_STABLE_EFFECTS','STARFIELD_DOM_INVALIDATE_COMMIT','STARFIELD_DOM_LEGACY_KIND','STARFIELD_DOM_REORDER_CONNECTIONS','STARFIELD_DOM_STALE_GATEWAY_API','STARFIELD_DOM_RELOAD_ON_COMMIT','STARFIELD_DOM_MATERIAL')){
        Remove-Item -LiteralPath ('Env:'+ $taskVariable) -ErrorAction SilentlyContinue
    }
    Pop-Location
}
