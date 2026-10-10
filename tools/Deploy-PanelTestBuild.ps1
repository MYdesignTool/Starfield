# Retain the exact installed native/Core pair; publish an independently checked
# CEP patch through the existing single-Junction deployment and rollback tools.
param(
    [Parameter(Mandatory=$true)][ValidatePattern('^[a-z0-9][a-z0-9-]{0,63}$')][string]$PreparedName,
    [ValidatePattern('^[A-Za-z0-9._-]+$')][string]$BaselineName='m3-17-native61-deploy-after',
    [ValidatePattern('^[A-Za-z0-9._-]+$')][string]$BackupName='m3-17-native61-panel63-es3-20261010',
    [string]$PluginDir='D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins',
    [string]$CepJunction='C:\Program Files (x86)\Common Files\Adobe\CEP\extensions\cep_panel',
    [switch]$Install
)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskPrepared=Join-Path $taskRepo ('artifacts/prepared/'+$PreparedName)
$taskCandidate=Join-Path $taskPrepared 'cep_panel'
$taskInfo=Get-Content -LiteralPath (Join-Path $taskPrepared 'candidate.json') -Raw | ConvertFrom-Json
$taskBaseline=Get-Content -LiteralPath (Join-Path $taskRepo ('artifacts/'+$BaselineName+'.json')) -Raw | ConvertFrom-Json
$taskNativeSource=if($taskBaseline.nativeSourceCommit){$taskBaseline.nativeSourceCommit}else{$taskBaseline.sourceCommit}
$taskBundle=Join-Path $taskRepo 'dist'
$taskLive=Join-Path $taskRepo 'cep_panel'
$taskBackup=Join-Path $taskRepo ('artifacts/disabled/'+$BackupName)
$taskSnapshot=Join-Path $taskBackup 'panel-snapshot.json'
$taskSimulation=[IO.Path]::GetFullPath($PluginDir).StartsWith((Join-Path $taskRepo 'artifacts/deploy-test/'),[StringComparison]::OrdinalIgnoreCase)
function Assert-PanelJunction([string]$taskPath,[string]$taskTarget){
    $taskLink=Get-Item -LiteralPath $taskPath -Force
    if($taskLink.LinkType -ne 'Junction' -or @($taskLink.Target).Count -ne 1 -or
        [IO.Path]::GetFullPath([string]$taskLink.Target[0]).TrimEnd('\') -ne [IO.Path]::GetFullPath($taskTarget).TrimEnd('\')){throw "Unexpected Junction: $taskPath"}
}
function Hash-PanelFile([string]$taskPath){return (Get-FileHash -LiteralPath $taskPath -Algorithm SHA256).Hash}
function Check-PanelHost(){if(-not $taskSimulation -and (Get-Process AfterFX,AfterFX_64 -ErrorAction SilentlyContinue)){throw 'Close AE before publishing the CEP patch.'}}
Assert-PanelJunction (Join-Path $PluginDir 'Starfield') $taskBundle
Assert-PanelJunction $CepJunction $taskLive
if($taskBaseline.nativeBuild -ne 61 -or $taskBaseline.coreAbi -ne 8 -or @($taskBaseline.native).Count -ne 8 -or
    $taskInfo.basePanel -ne $taskBaseline.panelGeneration -or $taskInfo.panelGeneration -le $taskInfo.basePanel){throw 'Unexpected installed/candidate pairing.'}
$taskNativeNames=@('StarfieldParticle.aex','StarfieldEmitter.aex','StarfieldParticleNode.aex','StarfieldForce.aex',
    'StarfieldTransform.aex','StarfieldModel.aex','StarfieldHost.aex','StarfieldCore.dll')
if(@($taskBaseline.native.name | Sort-Object -Unique).Count -ne 8){throw 'Duplicate native baseline entries.'}
foreach($taskNative in $taskBaseline.native){
    if($taskNative.name -notin $taskNativeNames -or (Hash-PanelFile (Join-Path $taskBundle $taskNative.name)) -ne $taskNative.hash){throw 'Installed native pair differs from the saved receipt.'}
}
$taskSelectorPath=Join-Path $taskBundle 'StarfieldRuntime/current.txt'
$taskSelector=[IO.File]::ReadAllText($taskSelectorPath).Trim()
$taskCoreHash=($taskBaseline.native | Where-Object name -eq 'StarfieldCore.dll').hash
if($taskSelector -ne $taskBaseline.selector -or $taskSelector -ne ('StarfieldCore-'+$taskCoreHash.Substring(0,16)+'.dll') -or
    (Hash-PanelFile (Join-Path $taskBundle ('StarfieldRuntime/'+$taskSelector))) -ne $taskCoreHash){throw 'Selected Core runtime differs.'}
$taskPaths=@('index.html','css/panel.css','js/graph_edits.js','js/native_graph_snapshot.js','js/graph_view.js','js/node_palette.js',
    'js/panel.js','js/graph_transactions.js','jsx/starfield_gateway.jsx','presets.html','js/presets.js','js/preset_manager.js',
    'CSXS/manifest.xml','js/model_assets.js','js/model_graph_transactions.js','js/preset_files.js','jsx/preset_file_transport.jsx','jsx/model_transaction_transport.jsx')
$taskFiles=@($taskBaseline.panel.files)
if($taskFiles.Count -ne $taskPaths.Count -or @($taskFiles.path | Sort-Object -Unique).Count -ne $taskPaths.Count){throw 'Incomplete/duplicate panel baseline.'}
$taskEntries=@()
foreach($taskFile in $taskFiles){
    if($taskFile.path -notin @($taskPaths | ForEach-Object {'cep_panel/'+$_})){throw 'Unexpected panel path.'}
    $taskRelative=$taskFile.path.Substring('cep_panel/'.Length)
    if((Hash-PanelFile (Join-Path $taskLive $taskRelative)) -ne $taskFile.installedHash){throw "Installed panel changed: $taskRelative"}
    $taskEntries += [pscustomobject]@{path=$taskFile.path;existed=$true;oldHash=$taskFile.installedHash;
        installedHash=(Hash-PanelFile (Join-Path $taskCandidate $taskRelative))}
}
$taskGeneration=[int]$taskInfo.panelGeneration
foreach($taskRelative in @('jsx/starfield_gateway.jsx','js/panel.js','js/preset_manager.js')){
    $taskText=[IO.File]::ReadAllText((Join-Path $taskCandidate $taskRelative))
    if($taskText -notmatch ('native-presets-'+$taskGeneration+'\b') -or $taskText -match ('native-presets-(?!'+$taskGeneration+'\b)\d+')){throw 'Mixed gateway generations.'}
}
foreach($taskPage in @('index.html','presets.html')){
    $taskText=[IO.File]::ReadAllText((Join-Path $taskCandidate $taskPage))
    if($taskText -notmatch ('\?v='+$taskGeneration+'\b') -or $taskText -match ('\?v=(?!'+$taskGeneration+'\b)\d+')){throw 'Mixed page cache generations.'}
}
if([IO.File]::ReadAllText((Join-Path $taskCandidate 'CSXS/manifest.xml')) -notmatch ('ExtensionBundleVersion="0\.1\.0\.'+$taskGeneration+'"')){throw 'Candidate manifest generation differs.'}
& node --expose-internals (Join-Path $taskRepo 'tests/extendscript_syntax_tests.js') $taskCandidate
if($LASTEXITCODE -ne 0){throw 'ExtendScript ES3 syntax preflight failed.'}
Write-Host "Verified eight native/Core and eighteen panel baseline files; candidate CEP$taskGeneration."
Write-Host "Rollback: powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir '$PluginDir' -BackupName '$BackupName' -Restore"
if(-not $Install){Write-Host 'Read-only report. Add -Install to publish after AE closes.';exit 0}
Check-PanelHost
if(Test-Path -LiteralPath $taskBackup){throw 'Backup already exists; refusing to overwrite it.'}
$taskBefore=[ordered]@{checkedAt=(Get-Date).ToString('o');nativeBuild=61;nativeSourceCommit=$taskNativeSource;panelGeneration=$taskBaseline.panelGeneration;
    native=$taskBaseline.native;panel=$taskBaseline.panel;selector=$taskSelector;candidateGeneration=$taskGeneration}
$taskBefore | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $taskRepo ('artifacts/'+$BackupName+'-before.json')) -Encoding UTF8
# This command retains installed native files and selector exactly and records the
# full Model-aware native backup for the established paired Restore tool.
& (Join-Path $PSScriptRoot 'Deploy-TestBuild.ps1') -PluginDir $PluginDir -BackupName $BackupName -KeepNative -Install
foreach($taskEntry in $taskEntries){
    $taskRelative=$taskEntry.path.Substring('cep_panel/'.Length)
    $taskSaved=Join-Path $taskBackup ('panel/'+$taskRelative)
    New-Item -ItemType Directory -Path (Split-Path -Parent $taskSaved) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $taskLive $taskRelative) -Destination $taskSaved
    if((Hash-PanelFile $taskSaved) -ne $taskEntry.oldHash){throw 'Panel backup changed during preparation.'}
}
$taskPanelRecord=[ordered]@{baselineCommit=$taskBaseline.sourceCommit;candidateCommit=(& git -C $taskRepo rev-parse HEAD);files=$taskEntries}
function Save-PanelSnapshot(){ $taskPanelRecord | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $taskSnapshot -Encoding UTF8 }
Save-PanelSnapshot
try{
    Check-PanelHost
    foreach($taskEntry in $taskEntries){
        $taskRelative=$taskEntry.path.Substring('cep_panel/'.Length)
        $taskSource=Join-Path $taskCandidate $taskRelative
        if((Hash-PanelFile $taskSource) -ne $taskEntry.installedHash -or
            (Hash-PanelFile (Join-Path $taskLive $taskRelative)) -ne $taskEntry.oldHash){throw 'Panel changed after preflight.'}
        Copy-Item -LiteralPath $taskSource -Destination (Join-Path $taskLive $taskRelative) -Force
        if((Hash-PanelFile (Join-Path $taskLive $taskRelative)) -ne $taskEntry.installedHash){throw 'Published panel hash mismatch.'}
    }
    foreach($taskNative in $taskBaseline.native){if((Hash-PanelFile (Join-Path $taskBundle $taskNative.name)) -ne $taskNative.hash){throw 'Native pair changed during CEP publication.'}}
    if([IO.File]::ReadAllText($taskSelectorPath).Trim() -ne $taskSelector){throw 'Core selector changed during CEP publication.'}
    & (Join-Path $PSScriptRoot 'Restore-TestBuild.ps1') -PluginDir $PluginDir -BackupName $BackupName
    $taskAfter=[ordered]@{checkedAt=(Get-Date).ToString('o');nativeBuild=61;coreAbi=8;panelGeneration=$taskGeneration;
        sourceCommit=$taskPanelRecord.candidateCommit;nativeSourceCommit=$taskNativeSource;native=$taskBaseline.native;panel=$taskPanelRecord;selector=$taskSelector;
        backup=$taskBackup;hostQualification='open'}
    $taskAfter | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $taskRepo ('artifacts/'+$BackupName+'-after.json')) -Encoding UTF8
    Write-Host "Published native61 / CEP$taskGeneration; twenty-six hashes and paired rollback preflight verified."
}catch{
    $taskFailure=$_
    foreach($taskEntry in $taskEntries){$taskEntry.installedHash=Hash-PanelFile (Join-Path $taskRepo $taskEntry.path)}
    Save-PanelSnapshot
    if(-not (Get-Process AfterFX,AfterFX_64 -ErrorAction SilentlyContinue)){
        & (Join-Path $PSScriptRoot 'Restore-TestBuild.ps1') -PluginDir $PluginDir -BackupName $BackupName -Restore
    }else{Write-Warning "AE is running; saved paired backup remains at $taskBackup. Close AE before the printed rollback."}
    throw $taskFailure
}
