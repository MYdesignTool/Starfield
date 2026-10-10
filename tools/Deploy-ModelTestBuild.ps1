# Promote only a complete, frozen AE2023 Model pair. Default is read-only.
param([Parameter(Mandatory=$true)][ValidatePattern('^[a-z0-9][a-z0-9-]{0,63}$')][string]$PreparedName,
    [ValidatePattern('^[A-Za-z0-9._-]+$')][string]$BaselineName='m3-16-native60-deploy-after',
    [ValidatePattern('^[A-Za-z0-9._-]+$')][string]$BackupName='m3-17-native61-panel62-model-20261010',
    [string]$PluginDir='D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins',
    [string]$CepJunction='C:\Program Files (x86)\Common Files\Adobe\CEP\extensions\cep_panel',
    [switch]$Install)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskPluginDir=$PluginDir
$taskPrepared=Join-Path $taskRepo ('artifacts/prepared/'+$PreparedName)
$taskBackup=Join-Path $taskRepo ('artifacts/disabled/'+$BackupName)
$taskPanelRoot=Join-Path $taskRepo 'cep_panel'
$taskBundle=Join-Path $taskRepo 'dist'
$taskBaseline=Get-Content -LiteralPath (Join-Path $taskRepo ('artifacts/'+$BaselineName+'.json')) -Raw -Encoding UTF8 | ConvertFrom-Json
$taskFrozen=Get-Content -LiteralPath (Join-Path $taskPrepared 'native-bundle/candidate.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$taskBaselineBuild=if($taskBaseline.nativeBuild){[int]$taskBaseline.nativeBuild}else{60}
$taskBaselinePanel=if($taskBaseline.panelGeneration){[int]$taskBaseline.panelGeneration}else{61}
$taskBaselineAbi=if($taskBaseline.coreAbi){[int]$taskBaseline.coreAbi}else{7}
$taskPairAllowed=($taskFrozen.nativeBuild -eq 61 -and $taskFrozen.panelGeneration -eq 62 -and $taskBaselineBuild -eq 60 -and $taskBaselinePanel -eq 61) -or
    ($taskFrozen.nativeBuild -eq 62 -and $taskFrozen.panelGeneration -eq 64 -and $taskBaselineBuild -eq 61 -and $taskBaselinePanel -eq 63) -or
    ($taskFrozen.nativeBuild -eq 63 -and $taskFrozen.panelGeneration -eq 65 -and $taskBaselineBuild -eq 62 -and $taskBaselinePanel -eq 64)
if(-not $taskPairAllowed -or $taskFrozen.coreAbi -ne 8 -or $taskFrozen.buildExitCode -ne 0 -or $taskFrozen.sourceCommit -notmatch '^[0-9a-f]{40}$'){throw 'Invalid complete Model candidate identity/build evidence.'}
$taskReleaseStem='m3-17-native'+$taskFrozen.nativeBuild
$taskExpected=@('StarfieldParticle.aex','StarfieldEmitter.aex','StarfieldParticleNode.aex','StarfieldForce.aex','StarfieldTransform.aex','StarfieldModel.aex','StarfieldHost.aex','StarfieldCore.dll')
if($taskFrozen.files.Count -ne 8 -or @($taskFrozen.files.name | Select-Object -Unique).Count -ne 8){throw 'Expected eight distinct native/Core files.'}
foreach($taskEntry in $taskFrozen.files){
    if($taskEntry.name -notin $taskExpected -or (Get-FileHash -LiteralPath (Join-Path $taskPrepared ('native-bundle/'+$taskEntry.name))).Hash -ne $taskEntry.hash){throw 'Frozen native pair differs.'}
}
$taskFingerprint=Join-Path $taskPrepared 'native-bundle/adapter-inputs.sha256'
if((Get-FileHash -LiteralPath $taskFingerprint).Hash -ne $taskFrozen.adapterFingerprintHash){throw 'Frozen adapter fingerprint differs.'}
$taskPackedVersion=32768+[int]$taskFrozen.nativeBuild
if([IO.File]::ReadAllText((Join-Path $taskPrepared 'source/ae_plugin/PluginVersion.h')) -notmatch ('STARFIELD_VERSION_PACKED '+$taskPackedVersion+'\b')){throw 'Frozen native version differs.'}
function Assert-ModelJunction([string]$taskPath,[string]$taskTarget){$taskItem=Get-Item -LiteralPath $taskPath -Force;
    if($taskItem.LinkType -ne 'Junction' -or [IO.Path]::GetFullPath([string]$taskItem.Target).TrimEnd('\') -ne [IO.Path]::GetFullPath($taskTarget).TrimEnd('\')){throw 'Existing Junction differs.'};return $taskItem}
$taskLink=Assert-ModelJunction (Join-Path $taskPluginDir 'Starfield') $taskBundle
$taskCepLink=Assert-ModelJunction $CepJunction $taskPanelRoot
if(@(Get-ChildItem -LiteralPath $taskPluginDir -Force | Where-Object Name -like 'Starfield*').Count -ne 1){throw 'Unexpected host Starfield entries.'}
foreach($taskEntry in $taskBaseline.native){if((Get-FileHash -LiteralPath (Join-Path $taskBundle $taskEntry.name)).Hash -ne $taskEntry.hash){throw 'Installed native baseline differs.'}}
if($taskBaselineBuild -eq 60 -and (Test-Path -LiteralPath (Join-Path $taskBundle 'StarfieldModel.aex'))){throw 'Unexpected installed Model before full promotion.'}
if($taskBaselineBuild -ge 61 -and (@($taskBaseline.native).Count -ne 8 -or @($taskBaseline.panel.files).Count -ne 18 -or
    'StarfieldModel.aex' -notin @($taskBaseline.native.name))){throw 'Incomplete installed Model pair.'}
foreach($taskEntry in $taskBaseline.panel.files){if((Get-FileHash -LiteralPath (Join-Path $taskRepo $taskEntry.path)).Hash -ne $taskEntry.installedHash){throw 'Installed CEP baseline differs.'}}
$taskSelector=(Get-Content -LiteralPath (Join-Path $taskBundle 'StarfieldRuntime/current.txt') -Raw).Trim()
if($taskSelector -ne $taskBaseline.selector -or (Get-FileHash -LiteralPath (Join-Path $taskBundle ('StarfieldRuntime/'+$taskSelector))).Hash -ne ($taskBaseline.native | Where-Object name -eq 'StarfieldCore.dll').hash){throw 'Installed baseline runtime differs.'}
$taskRecipe=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'candidates/model-panel-author-baseline.json') -Raw | ConvertFrom-Json
foreach($taskEntry in $taskRecipe.files){$taskLive=Join-Path $taskPanelRoot $taskEntry.path;$taskBytes=[Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($taskLive).Replace("`r`n","`n"));$taskSha=[Security.Cryptography.SHA256]::Create();
    try{$taskHash=([BitConverter]::ToString($taskSha.ComputeHash($taskBytes))).Replace('-','')}finally{$taskSha.Dispose()}
    if($taskHash -ne $taskEntry.normalizedSha256){throw 'Live candidate baseline changed since preparation.'}}
$taskPaths=@($taskRecipe.files.path)+@('CSXS/manifest.xml','js/model_assets.js','js/model_graph_transactions.js','js/preset_files.js','jsx/preset_file_transport.jsx','jsx/model_transaction_transport.jsx')
if($taskPaths.Count -ne 18 -or $taskFrozen.panelFiles.Count -ne 18){throw 'Expected eighteen paired panel sources.'}
$taskPanel=[ordered]@{baselineCommit=$taskBaseline.sourceCommit;candidateCommit=$taskFrozen.sourceCommit;files=@()}
foreach($taskRelative in $taskPaths){
    $taskOld=Join-Path $taskPanelRoot $taskRelative;$taskNew=Join-Path $taskPrepared ('cep_panel/'+$taskRelative)
    $taskFrozenEntry=@($taskFrozen.panelFiles | Where-Object path -eq ('cep_panel/'+$taskRelative))
    if($taskFrozenEntry.Count -ne 1 -or (Get-FileHash -LiteralPath $taskNew).Hash -ne $taskFrozenEntry[0].hash){throw 'Frozen panel differs.'}
    $taskExisted=Test-Path -LiteralPath $taskOld
    $taskPanel.files+=[pscustomobject]@{path='cep_panel/'+$taskRelative;existed=$taskExisted;oldHash=$(if($taskExisted){(Get-FileHash -LiteralPath $taskOld).Hash}else{$null});installedHash=$taskFrozenEntry[0].hash}
}
if([IO.File]::ReadAllText((Join-Path $taskPrepared 'cep_panel/CSXS/manifest.xml')) -notmatch ('ExtensionBundleVersion="0\.1\.0\.'+$taskFrozen.panelGeneration+'"') -or [IO.File]::ReadAllText((Join-Path $taskPrepared 'cep_panel/jsx/starfield_gateway.jsx')) -notmatch ('native-presets-'+$taskFrozen.panelGeneration+'\b')){throw 'Candidate CEP generation differs.'}
$taskProcesses=@(Get-Process AfterFX,AfterFX_64 -ErrorAction SilentlyContinue | Select-Object ProcessName,Id)
Write-Host ('AE process count: '+$taskProcesses.Count)
Write-Host ('Complete native'+$taskFrozen.nativeBuild+'/Core ABI8/CEP'+$taskFrozen.panelGeneration+' test pair; actual AE2023 qualification remains open.')
Write-Host "Undo (AE closed): powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir '$taskPluginDir' -BackupName '$BackupName' -Restore"
if(-not $Install){Write-Host 'Read-only report. Add -Install to publish.';exit 0}
if($taskProcesses.Count -or (Get-Process AfterFX,AfterFX_64 -ErrorAction SilentlyContinue)){throw 'Close AE before publication.'}
if(Test-Path -LiteralPath $taskBackup){throw 'Backup already exists.'}
$taskSavedPanel=Join-Path $taskPrepared 'baseline-panel';$taskSavedOutputs=Join-Path $taskPrepared 'baseline-build-output'
if((Test-Path -LiteralPath $taskSavedPanel) -or (Test-Path -LiteralPath $taskSavedOutputs)){throw 'Prepared baseline already exists.'}
foreach($taskEntry in $taskPanel.files){if($taskEntry.existed){$taskSaved=Join-Path $taskSavedPanel $taskEntry.path.Substring(10);New-Item -ItemType Directory -Path (Split-Path -Parent $taskSaved) -Force | Out-Null;
    Copy-Item -LiteralPath (Join-Path $taskRepo $taskEntry.path) -Destination $taskSaved;if((Get-FileHash -LiteralPath $taskSaved).Hash -ne $taskEntry.oldHash){throw 'Saved panel differs.'}}}
New-Item -ItemType Directory -Path $taskSavedOutputs -Force | Out-Null
foreach($taskEntry in $taskFrozen.files){$taskKind=if($taskEntry.name -eq 'StarfieldCore.dll'){'core-dll'}else{'plugin'};$taskOutput=Join-Path $taskRepo ('artifacts/'+$taskKind+'/2023/x64/Release/'+$taskEntry.name);
    if(Test-Path -LiteralPath $taskOutput){Copy-Item -LiteralPath $taskOutput -Destination (Join-Path $taskSavedOutputs $taskEntry.name)}
    New-Item -ItemType Directory -Path (Split-Path -Parent $taskOutput) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $taskPrepared ('native-bundle/'+$taskEntry.name)) -Destination $taskOutput -Force
    if((Get-FileHash -LiteralPath $taskOutput).Hash -ne $taskEntry.hash){throw 'Staged output differs.'}}
$taskOutputFingerprint=Join-Path $taskRepo 'artifacts/plugin/2023/x64/Release/adapter-inputs.sha256'
if(Test-Path -LiteralPath $taskOutputFingerprint){Copy-Item -LiteralPath $taskOutputFingerprint -Destination (Join-Path $taskSavedOutputs 'adapter-inputs.sha256')}
Copy-Item -LiteralPath $taskFingerprint -Destination $taskOutputFingerprint -Force
[ordered]@{checkedAt=[DateTimeOffset]::Now.ToString('o');nativeBuild=$taskBaselineBuild;panelGeneration=$taskBaselinePanel;coreAbi=$taskBaselineAbi;native=$taskBaseline.native;panel=$taskPanel;selector=$taskSelector;processes=$taskProcesses;link=$taskLink.Target;cepLink=$taskCepLink.Target} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $taskRepo ('artifacts/'+$taskReleaseStem+'-deploy-before.json')) -Encoding UTF8
if(Get-Process AfterFX,AfterFX_64 -ErrorAction SilentlyContinue){throw 'AE started before publication.'}
& (Join-Path $PSScriptRoot 'Deploy-TestBuild.ps1') -PluginDir $taskPluginDir -BackupName $BackupName -IncludeModel -Install *> (Join-Path $taskRepo ('artifacts/'+$taskReleaseStem+'-native-deploy.log'))
if(-not $?){throw 'Native publication failed.'}
Copy-Item -LiteralPath $taskSavedPanel -Destination (Join-Path $taskBackup 'panel') -Recurse
$taskPanel | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $taskBackup 'panel-snapshot.json') -Encoding UTF8
try {
    if(Get-Process AfterFX,AfterFX_64 -ErrorAction SilentlyContinue){throw 'AE started before paired panel publication.'}
    foreach($taskEntry in $taskPanel.files){$taskTarget=Join-Path $taskRepo $taskEntry.path;
        if($taskEntry.existed){if((Get-FileHash -LiteralPath $taskTarget).Hash -ne $taskEntry.oldHash){throw 'Live panel changed before publication.'}}elseif(Test-Path -LiteralPath $taskTarget){throw 'New panel source appeared before publication.'}
        Copy-Item -LiteralPath (Join-Path $taskPrepared $taskEntry.path) -Destination $taskTarget -Force
        if((Get-FileHash -LiteralPath $taskTarget).Hash -ne $taskEntry.installedHash){throw 'Published panel differs.'}}
} catch {
    # A partial source copy still has an exact paired manifest for restoration.
    foreach($taskEntry in $taskPanel.files){$taskTarget=Join-Path $taskRepo $taskEntry.path;if(Test-Path -LiteralPath $taskTarget){$taskEntry.installedHash=(Get-FileHash -LiteralPath $taskTarget).Hash}else{$taskEntry.installedHash=$null}}
    $taskPanel | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $taskBackup 'panel-snapshot.json') -Encoding UTF8
    if(-not (Get-Process AfterFX,AfterFX_64 -ErrorAction SilentlyContinue)){
        & (Join-Path $PSScriptRoot 'Restore-TestBuild.ps1') -PluginDir $taskPluginDir -BackupName $BackupName -Restore *> (Join-Path $taskRepo ('artifacts/'+$taskReleaseStem+'-failed-restore.log'))
    }
    throw
}
$taskAfter=[ordered]@{checkedAt=[DateTimeOffset]::Now.ToString('o');nativeBuild=$taskFrozen.nativeBuild;panelGeneration=$taskFrozen.panelGeneration;coreAbi=8;sourceCommit=$taskFrozen.sourceCommit;native=@();panel=$taskPanel;selector=(Get-Content -LiteralPath (Join-Path $taskBundle 'StarfieldRuntime/current.txt') -Raw).Trim();backup=$taskBackup;hostQualification='open';link=$taskLink.Target;cepLink=$taskCepLink.Target}
foreach($taskEntry in $taskFrozen.files){$taskHash=(Get-FileHash -LiteralPath (Join-Path $taskBundle $taskEntry.name)).Hash;if($taskHash -ne $taskEntry.hash){throw 'Installed native differs.'};$taskAfter.native+=[pscustomobject]@{name=$taskEntry.name;hash=$taskHash}}
$taskCoreHash=($taskAfter.native | Where-Object name -eq 'StarfieldCore.dll').hash
if($taskAfter.selector -ne ('StarfieldCore-'+$taskCoreHash.Substring(0,16)+'.dll') -or (Get-FileHash -LiteralPath (Join-Path $taskBundle ('StarfieldRuntime/'+$taskAfter.selector))).Hash -ne $taskCoreHash){throw 'Published selector differs.'}
foreach($taskEntry in $taskBaseline.native){if((Get-FileHash -LiteralPath (Join-Path $taskBackup ('bundle/'+$taskEntry.name))).Hash -ne $taskEntry.hash){throw 'Saved native baseline differs.'}}
& (Join-Path $PSScriptRoot 'Restore-TestBuild.ps1') -PluginDir $taskPluginDir -BackupName $BackupName *> (Join-Path $taskRepo ('artifacts/'+$taskReleaseStem+'-rollback-report.log'))
if(-not $?){throw 'Paired rollback report failed.'}
$taskAfter | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $taskRepo ('artifacts/'+$taskReleaseStem+'-deploy-after.json')) -Encoding UTF8
Write-Host 'Eight native/Core and eighteen panel hashes verified; paired rollback report passed. AE qualification remains open.'
