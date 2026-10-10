param([switch]$Prepare,[string]$DestinationName='m3-17-model-panel65',
    [ValidateRange(62,9999)][int]$PanelGeneration=65)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskRecipe=Join-Path $PSScriptRoot 'candidates/model-panel-author-baseline.json'
$taskPatch=Join-Path $PSScriptRoot 'candidates/model-panel-author.patch'
$taskAllowed=@('index.html','css/panel.css','js/graph_edits.js','js/native_graph_snapshot.js','js/graph_view.js',
    'js/node_palette.js','js/panel.js','js/graph_transactions.js','jsx/starfield_gateway.jsx',
    'presets.html','js/presets.js','js/preset_manager.js')
if($DestinationName -notmatch '^[a-z0-9][a-z0-9-]{0,63}$'){throw 'Use a simple candidate directory name.'}
$taskRelative='artifacts/prepared/'+$DestinationName
$taskDestination=[IO.Path]::GetFullPath((Join-Path $taskRepo $taskRelative))
$taskPrepared=[IO.Path]::GetFullPath((Join-Path $taskRepo 'artifacts/prepared'))+[IO.Path]::DirectorySeparatorChar
if(-not $taskDestination.StartsWith($taskPrepared,[StringComparison]::OrdinalIgnoreCase)){throw 'Candidate must stay under artifacts/prepared.'}
if(-not $Prepare){Write-Host "Report: prepare isolated Model author source at $taskDestination; add -Prepare to act. Live CEP is never written.";exit 0}
if(Test-Path -LiteralPath $taskDestination){throw 'Candidate directory already exists; choose a new name. No files were replaced.'}
$taskBaseline=Get-Content -LiteralPath $taskRecipe -Raw | ConvertFrom-Json
if($taskBaseline.schema -ne 1 -or $taskBaseline.files.Count -ne $taskAllowed.Count){throw 'Invalid candidate recipe.'}
function Get-SourceHash([string]$taskPath){
    $taskText=[IO.File]::ReadAllText($taskPath).Replace("`r`n","`n")
    $taskHash=[Security.Cryptography.SHA256]::Create()
    try {return ([BitConverter]::ToString($taskHash.ComputeHash([Text.Encoding]::UTF8.GetBytes($taskText)))).Replace('-','')}
    finally {$taskHash.Dispose()}
}
foreach($taskFile in $taskBaseline.files){
    if($taskFile.path -notin $taskAllowed){throw 'Candidate recipe contains an unexpected source path.'}
    $taskSource=Join-Path $taskRepo ('cep_panel/'+$taskFile.path)
    if((Get-SourceHash $taskSource) -ne $taskFile.normalizedSha256){throw "CEP baseline differs at $($taskFile.path); rebuild the candidate delta before preparation."}
}
# The patch can modify only these existing panel files, beneath the isolated copy.
foreach($taskLine in [IO.File]::ReadAllLines($taskPatch)){
    if($taskLine.StartsWith('--- ') -or $taskLine.StartsWith('+++ ')){
        if($taskLine -notmatch '^(--- a|\+\+\+ b)/cep_panel/(.+)$' -or $Matches[2] -notin $taskAllowed){throw 'Patch contains an unexpected target path.'}
    }
}
New-Item -ItemType Directory -Path $taskDestination -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $taskRepo 'cep_panel') -Destination (Join-Path $taskDestination 'cep_panel') -Recurse
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'candidates/model_assets.js') -Destination (Join-Path $taskDestination 'cep_panel/js/model_assets.js')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'candidates/model_graph_transactions.js') -Destination (Join-Path $taskDestination 'cep_panel/js/model_graph_transactions.js')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'candidates/preset_files.js') -Destination (Join-Path $taskDestination 'cep_panel/js/preset_files.js')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'candidates/preset_file_transport.jsx') -Destination (Join-Path $taskDestination 'cep_panel/jsx/preset_file_transport.jsx')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'candidates/model_transaction_transport.jsx') -Destination (Join-Path $taskDestination 'cep_panel/jsx/model_transaction_transport.jsx')
foreach($taskFile in $taskBaseline.files){
    if((Get-SourceHash (Join-Path $taskDestination ('cep_panel/'+$taskFile.path))) -ne $taskFile.normalizedSha256){throw 'Copied baseline changed during preparation.'}
}
Push-Location -LiteralPath $taskRepo
try {
    if([IO.File]::ReadAllText($taskPatch).Trim().Length){
        & git apply --check --directory=$taskRelative $taskPatch
        if($LASTEXITCODE -ne 0){throw 'Candidate patch preflight failed. Live CEP is unchanged.'}
        & git apply --directory=$taskRelative $taskPatch
        if($LASTEXITCODE -ne 0){throw 'Candidate patch failed. Live CEP is unchanged.'}
    }
} finally {Pop-Location}
$taskManifest=Join-Path $taskDestination 'cep_panel/CSXS/manifest.xml'
$taskManifestText=[regex]::Replace([IO.File]::ReadAllText($taskManifest),'0\.1\.0\.\d+',('0.1.0.'+$PanelGeneration))
$taskManifestText=[regex]::Replace($taskManifestText,'Version="0\.1\.\d+"',('Version="0.1.'+$PanelGeneration+'"'))
[IO.File]::WriteAllText($taskManifest,$taskManifestText,[Text.UTF8Encoding]::new($false))
foreach($taskPage in @('index.html','presets.html')){
    $taskPagePath=Join-Path $taskDestination ('cep_panel/'+$taskPage)
    $taskPageText=[regex]::Replace([IO.File]::ReadAllText($taskPagePath),'\?v=\d+',('?v='+$PanelGeneration))
    [IO.File]::WriteAllText($taskPagePath,$taskPageText,[Text.UTF8Encoding]::new($false))
}
foreach($taskGatewayPath in @('jsx/starfield_gateway.jsx','js/panel.js','js/preset_manager.js')){
    $taskGatewayFile=Join-Path $taskDestination ('cep_panel/'+$taskGatewayPath)
    $taskGatewayText=[regex]::Replace([IO.File]::ReadAllText($taskGatewayFile),'native-presets-\d+',('native-presets-'+$PanelGeneration))
    [IO.File]::WriteAllText($taskGatewayFile,$taskGatewayText,[Text.UTF8Encoding]::new($false))
}
[ordered]@{basePanel=$taskBaseline.basePanel;panelGeneration=$PanelGeneration;candidatePath=(Join-Path $taskDestination 'cep_panel');published=$false;
    modelAssetsSha256=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'candidates/model_assets.js') -Algorithm SHA256).Hash;
    modelGraphTransactionsSha256=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'candidates/model_graph_transactions.js') -Algorithm SHA256).Hash;
    presetFilesSha256=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'candidates/preset_files.js') -Algorithm SHA256).Hash;
    presetFileTransportSha256=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'candidates/preset_file_transport.jsx') -Algorithm SHA256).Hash;
    modelTransactionTransportSha256=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'candidates/model_transaction_transport.jsx') -Algorithm SHA256).Hash;
    patchSha256=(Get-FileHash -LiteralPath $taskPatch -Algorithm SHA256).Hash} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskDestination 'candidate.json') -Encoding UTF8
Write-Host "Prepared isolated Model author candidate: $taskDestination"
