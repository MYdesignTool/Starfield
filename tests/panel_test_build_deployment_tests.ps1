param([switch]$Run)
$ErrorActionPreference='Stop'
if(-not $Run){Write-Host 'Report only. Add -Run for dummy files/Junctions under artifacts/deploy-test; no Adobe files are changed.';exit 0}
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskRoot=Join-Path $taskRepo ('artifacts/deploy-test/panel-es3-'+[guid]::NewGuid().ToString('N'))
$taskAllowed=[IO.Path]::GetFullPath((Join-Path $taskRepo 'artifacts/deploy-test'))+'\'
if(-not [IO.Path]::GetFullPath($taskRoot).StartsWith($taskAllowed,[StringComparison]::OrdinalIgnoreCase)){throw 'Fixture escapes artifacts.'}
$taskChecks=0
function Check-PanelTest([bool]$taskPass,[string]$taskMessage){$script:taskChecks++;if(-not $taskPass){throw $taskMessage}}
function Write-PanelTest([string]$taskPath,[string]$taskText){New-Item -ItemType Directory -Path (Split-Path -Parent $taskPath) -Force | Out-Null;[IO.File]::WriteAllText($taskPath,$taskText,[Text.UTF8Encoding]::new($false))}
function Hash-PanelTest([string]$taskPath){return (Get-FileHash -LiteralPath $taskPath).Hash}
foreach($taskTool in @('Deploy-PanelTestBuild.ps1','Deploy-TestBuild.ps1','Restore-TestBuild.ps1')){
    New-Item -ItemType Directory -Path (Join-Path $taskRoot 'tools') -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $taskRepo ('tools/'+$taskTool)) -Destination (Join-Path $taskRoot ('tools/'+$taskTool))
}
New-Item -ItemType Directory -Path (Join-Path $taskRoot 'tests') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $taskRepo 'tests/extendscript_syntax_tests.js') -Destination (Join-Path $taskRoot 'tests/extendscript_syntax_tests.js')
$taskLive=Join-Path $taskRoot 'cep_panel';$taskBundle=Join-Path $taskRoot 'dist'
$taskPlugin=Join-Path $taskRoot 'artifacts/deploy-test/host';$taskCep=Join-Path $taskRoot 'artifacts/deploy-test/cep'
$taskCandidate=Join-Path $taskRoot 'artifacts/prepared/panel-test/cep_panel'
New-Item -ItemType Directory -Path $taskLive,$taskBundle,$taskPlugin -Force | Out-Null
New-Item -ItemType Junction -Path (Join-Path $taskPlugin 'Starfield') -Target $taskBundle | Out-Null
New-Item -ItemType Junction -Path $taskCep -Target $taskLive | Out-Null
$taskNames=@('StarfieldParticle.aex','StarfieldEmitter.aex','StarfieldParticleNode.aex','StarfieldForce.aex',
    'StarfieldTransform.aex','StarfieldModel.aex','StarfieldHost.aex','StarfieldCore.dll')
$taskNative=@()
foreach($taskName in $taskNames){Write-PanelTest (Join-Path $taskBundle $taskName) ('native61-'+$taskName);$taskNative += @{name=$taskName;hash=(Hash-PanelTest (Join-Path $taskBundle $taskName))}}
$taskCoreHash=($taskNative | Where-Object name -eq 'StarfieldCore.dll').hash
$taskSelector='StarfieldCore-'+$taskCoreHash.Substring(0,16)+'.dll'
Write-PanelTest (Join-Path $taskBundle ('StarfieldRuntime/'+$taskSelector)) 'native61-StarfieldCore.dll'
Write-PanelTest (Join-Path $taskBundle 'StarfieldRuntime/current.txt') $taskSelector
$taskPaths=@('index.html','css/panel.css','js/graph_edits.js','js/native_graph_snapshot.js','js/graph_view.js','js/node_palette.js',
    'js/panel.js','js/graph_transactions.js','jsx/starfield_gateway.jsx','presets.html','js/presets.js','js/preset_manager.js',
    'CSXS/manifest.xml','js/model_assets.js','js/model_graph_transactions.js','js/preset_files.js','jsx/preset_file_transport.jsx','jsx/model_transaction_transport.jsx')
$taskOldPanel=@();$taskNewPanel=@()
foreach($taskPath in $taskPaths){
    Write-PanelTest (Join-Path $taskLive $taskPath) ('old62-'+$taskPath)
    $taskText=if($taskPath -eq 'CSXS/manifest.xml'){'ExtensionBundleVersion="0.1.0.63"'}
        elseif($taskPath -in @('jsx/starfield_gateway.jsx','js/panel.js','js/preset_manager.js')){'var GATEWAY_BUILD = "native-presets-63";'}
        elseif($taskPath -in @('index.html','presets.html')){'script.js?v=63'}else{'// candidate63 '+$taskPath}
    Write-PanelTest (Join-Path $taskCandidate $taskPath) $taskText
    $taskOldPanel += @{path='cep_panel/'+$taskPath;installedHash=(Hash-PanelTest (Join-Path $taskLive $taskPath))}
    $taskNewPanel += @{path='cep_panel/'+$taskPath;hash=(Hash-PanelTest (Join-Path $taskCandidate $taskPath))}
}
Write-PanelTest (Join-Path $taskRoot 'artifacts/prepared/panel-test/candidate.json') (ConvertTo-Json @{basePanel=62;panelGeneration=63})
Write-PanelTest (Join-Path $taskRoot 'artifacts/m3-17-native61-deploy-after.json') (ConvertTo-Json @{nativeBuild=61;coreAbi=8;panelGeneration=62;sourceCommit=('a'*40);native=$taskNative;panel=@{files=$taskOldPanel};selector=$taskSelector} -Depth 6)
function Invoke-PanelTest([string]$taskName,[string]$taskTool,[string[]]$taskArguments){
    $taskPreference=$ErrorActionPreference;$ErrorActionPreference='Continue'
    try{& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $taskRoot ('tools/'+$taskTool)) @taskArguments *> (Join-Path $taskRoot ($taskName+'.log'));$taskCode=$LASTEXITCODE}
    finally{$ErrorActionPreference=$taskPreference}
    return $taskCode
}
$taskArguments=@('-PreparedName','panel-test','-BackupName','panel-pair','-PluginDir',$taskPlugin,'-CepJunction',$taskCep)
Check-PanelTest ((Invoke-PanelTest 'report' 'Deploy-PanelTestBuild.ps1' $taskArguments) -eq 0) 'Report failed.'
Check-PanelTest (-not (Test-Path -LiteralPath (Join-Path $taskRoot 'artifacts/disabled/panel-pair'))) 'Report created a backup.'
$taskGateway=Join-Path $taskCandidate 'jsx/starfield_gateway.jsx'
Write-PanelTest $taskGateway 'var GATEWAY_BUILD = "native-presets-63"; var byte=0;'
Check-PanelTest ((Invoke-PanelTest 'reserved-word' 'Deploy-PanelTestBuild.ps1' ($taskArguments+@('-Install'))) -ne 0) 'ES3 reserved word accepted.'
Check-PanelTest (-not (Test-Path -LiteralPath (Join-Path $taskRoot 'artifacts/disabled/panel-pair'))) 'Invalid syntax created a backup.'
Write-PanelTest $taskGateway 'var GATEWAY_BUILD = "native-presets-63";'
$taskClient=Join-Path $taskCandidate 'js/panel.js'
Write-PanelTest $taskClient 'var GATEWAY_BUILD = "native-presets-62";'
Check-PanelTest ((Invoke-PanelTest 'mixed-generation' 'Deploy-PanelTestBuild.ps1' ($taskArguments+@('-Install'))) -ne 0) 'Mixed generation accepted.'
Write-PanelTest $taskClient 'var GATEWAY_BUILD = "native-presets-63";'
Check-PanelTest ((Invoke-PanelTest 'install' 'Deploy-PanelTestBuild.ps1' ($taskArguments+@('-Install'))) -eq 0) 'Install failed.'
foreach($taskFile in $taskNative){Check-PanelTest ((Hash-PanelTest (Join-Path $taskBundle $taskFile.name)) -eq $taskFile.hash) 'Native bytes changed.'}
foreach($taskFile in $taskNewPanel){Check-PanelTest ((Hash-PanelTest (Join-Path $taskRoot $taskFile.path)) -eq $taskFile.hash) 'Candidate panel hash differs.'}
$taskBackup=Join-Path $taskRoot 'artifacts/disabled/panel-pair'
$taskSaved=Get-Content -LiteralPath (Join-Path $taskBackup 'deployment.json') -Raw | ConvertFrom-Json
Check-PanelTest ([bool]$taskSaved.modelCandidate) 'Model omitted from native backup.'
$taskRestoreArguments=@('-PluginDir',$taskPlugin,'-BackupName','panel-pair')
Check-PanelTest ((Invoke-PanelTest 'restore-report' 'Restore-TestBuild.ps1' $taskRestoreArguments) -eq 0) 'Paired restore report failed.'
# Fake only the process lookup inside the artifact fixture. No actual process is
# started/stopped and every filesystem target is this checked dummy repository.
$taskRestoreWrapper=Join-Path $taskRoot 'restore-fixture.ps1'
Write-PanelTest $taskRestoreWrapper @'
$ErrorActionPreference='Stop'
function global:Get-Process { param($Name,$ErrorAction) }
& (Join-Path $PSScriptRoot 'tools/Restore-TestBuild.ps1') -PluginDir (Join-Path $PSScriptRoot 'artifacts/deploy-test/host') -BackupName 'panel-pair' -Restore
'@
$taskPreference=$ErrorActionPreference;$ErrorActionPreference='Continue'
try{& powershell -NoProfile -ExecutionPolicy Bypass -File $taskRestoreWrapper *> (Join-Path $taskRoot 'restore.log');$taskCode=$LASTEXITCODE}
finally{$ErrorActionPreference=$taskPreference}
Check-PanelTest ($taskCode -eq 0) 'Paired restore failed.'
foreach($taskFile in $taskNative){Check-PanelTest ((Hash-PanelTest (Join-Path $taskBundle $taskFile.name)) -eq $taskFile.hash) 'Restored native differs.'}
foreach($taskFile in $taskOldPanel){Check-PanelTest ((Hash-PanelTest (Join-Path $taskRoot $taskFile.path)) -eq $taskFile.installedHash) 'Old panel not restored.'}
Check-PanelTest ([IO.File]::ReadAllText((Join-Path $taskBundle 'StarfieldRuntime/current.txt')).Trim() -eq $taskSelector) 'Selector changed.'
Write-Host "panel_test_build_deployment_tests: $taskChecks checks passed; fixture $taskRoot; no Adobe writes or actual process mutations."
