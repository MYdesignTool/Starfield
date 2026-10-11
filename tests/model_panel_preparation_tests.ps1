param([switch]$Run)
$ErrorActionPreference='Stop'
if(-not $Run){Write-Host 'Report only. Add -Run to prepare a nested dummy source archive under artifacts/deploy-test; no live panel or Adobe writes.';exit 0}
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskAllowed=[IO.Path]::GetFullPath((Join-Path $taskRepo 'artifacts/deploy-test'))+'\'
$taskRoot=[IO.Path]::GetFullPath((Join-Path $taskAllowed ('model-prepare-'+[guid]::NewGuid().ToString('N'))))
if(-not $taskRoot.StartsWith($taskAllowed,[StringComparison]::OrdinalIgnoreCase)){throw 'Fixture escapes artifacts.'}
$taskTools=Join-Path $taskRoot 'tools'
New-Item -ItemType Directory -Path (Join-Path $taskTools 'candidates') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $taskRepo 'cep_panel') -Destination (Join-Path $taskRoot 'cep_panel') -Recurse
Copy-Item -LiteralPath (Join-Path $taskRepo 'tools/Prepare-ModelPanelCandidate.ps1') -Destination $taskTools
foreach($taskHelper in @('model_assets.js','model_graph_transactions.js','preset_files.js','preset_file_transport.jsx','model_transaction_transport.jsx')){
    Copy-Item -LiteralPath (Join-Path $taskRepo ('tools/candidates/'+$taskHelper)) -Destination (Join-Path $taskTools 'candidates')
}
function Hash-PreparedText([string]$taskPath){
    $taskSha=[Security.Cryptography.SHA256]::Create()
    try{return [BitConverter]::ToString($taskSha.ComputeHash([Text.Encoding]::UTF8.GetBytes([IO.File]::ReadAllText($taskPath).Replace("`r`n","`n")))).Replace('-','')}
    finally{$taskSha.Dispose()}
}
$taskRecipe=Get-Content -LiteralPath (Join-Path $taskRepo 'tools/candidates/model-panel-author-baseline.json') -Raw | ConvertFrom-Json
foreach($taskEntry in $taskRecipe.files){$taskEntry.normalizedSha256=Hash-PreparedText (Join-Path $taskRoot ('cep_panel/'+$taskEntry.path))}
$taskRecipe | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $taskTools 'candidates/model-panel-author-baseline.json') -Encoding UTF8
$taskCss=Join-Path $taskRoot 'cep_panel/css/panel.css'
$taskFirstLine=([IO.File]::ReadAllText($taskCss).Replace("`r`n","`n").Split("`n"))[0]
$taskMarker='/* Model preparation fixture marker. */'
$taskPatch="--- a/cep_panel/css/panel.css`n+++ b/cep_panel/css/panel.css`n@@ -1,1 +1,2 @@`n+$taskMarker`n $taskFirstLine`n"
[IO.File]::WriteAllText((Join-Path $taskTools 'candidates/model-panel-author.patch'),$taskPatch,[Text.UTF8Encoding]::new($false))
$taskGeneration=[int]$taskRecipe.basePanel+1
& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $taskTools 'Prepare-ModelPanelCandidate.ps1') -Prepare -DestinationName nested-patch -PanelGeneration $taskGeneration *> (Join-Path $taskRoot 'prepare.log')
if($LASTEXITCODE){throw ('Nested preparation failed; '+$taskRoot)}
$taskChecks=0
function Check-PreparedPanel([bool]$taskPass,[string]$taskMessage){$script:taskChecks++;if(-not $taskPass){throw $taskMessage}}
$taskCandidate=Join-Path $taskRoot 'artifacts/prepared/nested-patch/cep_panel'
Check-PreparedPanel ([IO.File]::ReadAllText((Join-Path $taskCandidate 'css/panel.css')).StartsWith($taskMarker)) 'git apply silently skipped the nested archive patch.'
foreach($taskEntry in $taskRecipe.files){Check-PreparedPanel ((Hash-PreparedText (Join-Path $taskRoot ('cep_panel/'+$taskEntry.path))) -eq $taskEntry.normalizedSha256) 'Source baseline changed.'}
Check-PreparedPanel ([IO.File]::ReadAllText((Join-Path $taskCandidate 'jsx/starfield_gateway.jsx')).Contains('native-presets-'+$taskGeneration)) 'Candidate generation differs.'
Check-PreparedPanel (Test-Path -LiteralPath (Join-Path $taskCandidate 'jsx/model_transaction_transport.jsx')) 'Paired Model helper missing.'
Write-Output "model_panel_preparation_tests: $taskChecks checks passed; nested archive patch applied; fixture $taskRoot; no live panel or Adobe writes."
