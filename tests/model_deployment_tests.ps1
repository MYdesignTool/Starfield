param([switch]$Run,[switch]$UpgradeInstalledModel,[ValidateSet(61,62)][int]$InstalledModelBuild=61)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if(-not $Run){Write-Host 'Report: simulated complete native61/CEP62 deployment and paired rollback under artifacts; add -Run.';exit 0}
$taskOldBuild=if($UpgradeInstalledModel){$InstalledModelBuild}else{60};$taskOldGeneration=if($UpgradeInstalledModel){$InstalledModelBuild+2}else{61}
$taskNewBuild=if($UpgradeInstalledModel){$InstalledModelBuild+1}else{61};$taskNewGeneration=if($UpgradeInstalledModel){$InstalledModelBuild+3}else{62}
$taskRoot=Join-Path $taskRepo ('artifacts/deploy-test/model-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $taskRoot -Force | Out-Null
$taskChecks=0
function Assert-ModelTest([bool]$taskPass,[string]$taskMessage){$script:taskChecks++;if(-not $taskPass){throw $taskMessage}}
function Write-ModelTest([string]$taskPath,[string]$taskText){New-Item -ItemType Directory -Path (Split-Path -Parent $taskPath) -Force | Out-Null;[IO.File]::WriteAllText($taskPath,$taskText,[Text.UTF8Encoding]::new($false))}
function Hash-ModelTest([string]$taskPath){return (Get-FileHash -LiteralPath $taskPath).Hash}
foreach($taskTool in @('Deploy-TestBuild.ps1','Deploy-ModelTestBuild.ps1','Restore-TestBuild.ps1')){New-Item -ItemType Directory -Path (Join-Path $taskRoot 'tools') -Force | Out-Null;Copy-Item -LiteralPath (Join-Path $taskRepo ('tools/'+$taskTool)) -Destination (Join-Path $taskRoot ('tools/'+$taskTool))}
$taskPrepared=Join-Path $taskRoot 'artifacts/prepared/model-test';$taskBundle=Join-Path $taskRoot 'dist';$taskPanelRoot=Join-Path $taskRoot 'cep_panel'
$taskPlugin=Join-Path $taskRoot 'artifacts/deploy-test/host';$taskCep=Join-Path $taskRoot 'artifacts/deploy-test/panel-link'
New-Item -ItemType Directory -Path $taskPlugin,$taskPanelRoot,$taskBundle -Force | Out-Null
New-Item -ItemType Junction -Path (Join-Path $taskPlugin 'Starfield') -Target $taskBundle | Out-Null
New-Item -ItemType Junction -Path $taskCep -Target $taskPanelRoot | Out-Null
$taskNames=@('StarfieldParticle.aex','StarfieldEmitter.aex','StarfieldParticleNode.aex','StarfieldForce.aex','StarfieldTransform.aex','StarfieldHost.aex','StarfieldCore.dll')
$taskOldNative=@();$taskNewNative=@()
foreach($taskName in ($taskNames+@('StarfieldModel.aex'))){$taskNew=Join-Path $taskPrepared ('native-bundle/'+$taskName);Write-ModelTest $taskNew ('new-model-pair-'+$taskName);$taskNewNative+=[pscustomobject]@{name=$taskName;hash=(Hash-ModelTest $taskNew)};
    if($UpgradeInstalledModel -or $taskName -ne 'StarfieldModel.aex'){$taskOld=Join-Path $taskBundle $taskName;Write-ModelTest $taskOld ('old-native'+$taskOldBuild+'-'+$taskName);$taskOldNative+=[pscustomobject]@{name=$taskName;hash=(Hash-ModelTest $taskOld)}}}
$taskOldCore=($taskOldNative | Where-Object name -eq 'StarfieldCore.dll').hash;$taskOldSelector='StarfieldCore-'+$taskOldCore.Substring(0,16)+'.dll'
Write-ModelTest (Join-Path $taskBundle ('StarfieldRuntime/'+$taskOldSelector)) ([IO.File]::ReadAllText((Join-Path $taskBundle 'StarfieldCore.dll')))
Write-ModelTest (Join-Path $taskBundle 'StarfieldRuntime/current.txt') $taskOldSelector
$taskPaths=@('index.html','css/panel.css','js/graph_edits.js','js/native_graph_snapshot.js','js/graph_view.js','js/node_palette.js','js/panel.js','js/graph_transactions.js','jsx/starfield_gateway.jsx','presets.html','js/presets.js','js/preset_manager.js')
$taskRecipeFiles=@();$taskNewPanel=@();$taskOldPanel=@()
foreach($taskPath in ($taskPaths+@('CSXS/manifest.xml','js/model_assets.js','js/model_graph_transactions.js','js/preset_files.js','jsx/preset_file_transport.jsx','jsx/model_transaction_transport.jsx'))){
    $taskNewText=if($taskPath -eq 'CSXS/manifest.xml'){'ExtensionBundleVersion="0.1.0.'+$taskNewGeneration+'"'}elseif($taskPath -eq 'jsx/starfield_gateway.jsx'){'native-presets-'+$taskNewGeneration}else{'new-panel'+$taskNewGeneration+'-'+$taskPath}
    $taskNew=Join-Path $taskPrepared ('cep_panel/'+$taskPath);Write-ModelTest $taskNew $taskNewText;$taskNewPanel+=[pscustomobject]@{path='cep_panel/'+$taskPath;hash=(Hash-ModelTest $taskNew)}
    if($UpgradeInstalledModel -or $taskPath -in $taskPaths -or $taskPath -eq 'CSXS/manifest.xml'){$taskOld=Join-Path $taskPanelRoot $taskPath;Write-ModelTest $taskOld ('old-panel'+$taskOldGeneration+'-'+$taskPath);$taskOldPanel+=[pscustomobject]@{path='cep_panel/'+$taskPath;installedHash=(Hash-ModelTest $taskOld)}
        if($taskPath -in $taskPaths){$taskRecipeFiles+=[pscustomobject]@{path=$taskPath;normalizedSha256=(Hash-ModelTest $taskOld)}}}}
Write-ModelTest (Join-Path $taskRoot 'tools/candidates/model-panel-author-baseline.json') (ConvertTo-Json @{schema=1;files=$taskRecipeFiles} -Depth 4)
Write-ModelTest (Join-Path $taskRoot 'artifacts/m3-16-native60-deploy-after.json') (ConvertTo-Json @{nativeBuild=$taskOldBuild;panelGeneration=$taskOldGeneration;coreAbi=$(if($UpgradeInstalledModel){8}else{7});sourceCommit=('a'*40);native=$taskOldNative;panel=@{files=$taskOldPanel};selector=$taskOldSelector} -Depth 5)
Write-ModelTest (Join-Path $taskPrepared 'source/ae_plugin/PluginVersion.h') ('#define STARFIELD_VERSION_PACKED '+(32768+$taskNewBuild))
Write-ModelTest (Join-Path $taskPrepared 'native-bundle/adapter-inputs.sha256') 'fixture-fingerprint'
$taskCandidate=@{nativeBuild=$taskNewBuild;panelGeneration=$taskNewGeneration;coreAbi=8;buildExitCode=0;sourceCommit=('b'*40);files=$taskNewNative;panelFiles=$taskNewPanel;adapterFingerprintHash=(Hash-ModelTest (Join-Path $taskPrepared 'native-bundle/adapter-inputs.sha256'))}
Write-ModelTest (Join-Path $taskPrepared 'native-bundle/candidate.json') (ConvertTo-Json $taskCandidate -Depth 6)
function Invoke-ModelTest([string]$taskName,[string]$taskScript,[string[]]$taskArguments){
    $taskPreviousPreference=$ErrorActionPreference;$ErrorActionPreference='Continue'
    try{& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $taskRoot ('tools/'+$taskScript)) @taskArguments *> (Join-Path $taskRoot ($taskName+'.log'));$taskCode=$LASTEXITCODE}
    finally{$ErrorActionPreference=$taskPreviousPreference}
    return $taskCode
}
$taskArguments=@('-PreparedName','model-test','-BackupName','model-pair','-PluginDir',$taskPlugin,'-CepJunction',$taskCep)
Assert-ModelTest ((Invoke-ModelTest 'report' 'Deploy-ModelTestBuild.ps1' $taskArguments) -eq 0) 'Complete candidate report failed.'
Assert-ModelTest (-not (Test-Path (Join-Path $taskRoot 'artifacts/disabled/model-pair'))) 'Read-only report wrote a backup.'
Move-Item -LiteralPath (Join-Path $taskPrepared 'native-bundle/StarfieldModel.aex') -Destination (Join-Path $taskPrepared 'held-model.aex')
Assert-ModelTest ((Invoke-ModelTest 'missing-model' 'Deploy-ModelTestBuild.ps1' ($taskArguments+@('-Install'))) -ne 0) 'Missing Model was accepted.'
Assert-ModelTest (-not (Test-Path (Join-Path $taskRoot 'artifacts/disabled/model-pair'))) 'Missing pair mutated installation.'
Move-Item -LiteralPath (Join-Path $taskPrepared 'held-model.aex') -Destination (Join-Path $taskPrepared 'native-bundle/StarfieldModel.aex')
Assert-ModelTest ((Invoke-ModelTest 'install' 'Deploy-ModelTestBuild.ps1' ($taskArguments+@('-Install'))) -eq 0) 'Complete install failed.'
foreach($taskEntry in $taskNewNative){Assert-ModelTest ((Hash-ModelTest (Join-Path $taskBundle $taskEntry.name)) -eq $taskEntry.hash) 'Installed native hash differs.'}
foreach($taskEntry in $taskNewPanel){Assert-ModelTest ((Hash-ModelTest (Join-Path $taskRoot $taskEntry.path)) -eq $taskEntry.hash) 'Installed CEP hash differs.'}
Assert-ModelTest ((Invoke-ModelTest 'partial-replacement' 'Deploy-TestBuild.ps1' @('-PluginDir',$taskPlugin,'-BackupName','partial','-Install')) -ne 0) 'Partial replacement of installed Model was accepted.'
# A source not yet copied during a failed publication may legitimately be absent.
$taskBackup=Join-Path $taskRoot 'artifacts/disabled/model-pair';$taskManifestPath=Join-Path $taskBackup 'panel-snapshot.json'
$taskManifest=Get-Content -LiteralPath $taskManifestPath -Raw | ConvertFrom-Json
$taskMissing='cep_panel/jsx/model_transaction_transport.jsx';Move-Item -LiteralPath (Join-Path $taskRoot $taskMissing) -Destination (Join-Path $taskRoot 'held-new-helper.jsx')
if($UpgradeInstalledModel){
    Copy-Item -LiteralPath (Join-Path $taskBackup ('panel/'+$taskMissing.Substring(10))) -Destination (Join-Path $taskRoot $taskMissing)
    ($taskManifest.files | Where-Object path -eq $taskMissing).installedHash=Hash-ModelTest (Join-Path $taskRoot $taskMissing)
}else{($taskManifest.files | Where-Object path -eq $taskMissing).installedHash=$null}
Write-ModelTest $taskManifestPath ($taskManifest | ConvertTo-Json -Depth 6)
Assert-ModelTest ((Invoke-ModelTest 'rollback' 'Restore-TestBuild.ps1' @('-PluginDir',$taskPlugin,'-BackupName','model-pair','-Restore')) -eq 0) 'Paired rollback failed.'
foreach($taskEntry in $taskOldNative){Assert-ModelTest ((Hash-ModelTest (Join-Path $taskBundle $taskEntry.name)) -eq $taskEntry.hash) 'Restored native60 differs.'}
foreach($taskEntry in $taskOldPanel){Assert-ModelTest ((Hash-ModelTest (Join-Path $taskRoot $taskEntry.path)) -eq $taskEntry.installedHash) 'Restored CEP61 differs.'}
Assert-ModelTest ((Test-Path (Join-Path $taskBundle 'StarfieldModel.aex')) -eq [bool]$UpgradeInstalledModel) 'Model existence after rollback differs from baseline.'
foreach($taskEntry in $taskNewPanel | Where-Object path -notin $taskOldPanel.path){Assert-ModelTest (-not (Test-Path (Join-Path $taskRoot $taskEntry.path))) 'New helper remains after rollback.'}
Assert-ModelTest (([IO.File]::ReadAllText((Join-Path $taskBundle 'StarfieldRuntime/current.txt'))).Trim() -eq $taskOldSelector) 'Old selector not restored.'
Write-Host "model_deployment_tests: $taskChecks checks passed; dummy binaries and sources, no real Adobe paths changed. Fixture: $taskRoot"
