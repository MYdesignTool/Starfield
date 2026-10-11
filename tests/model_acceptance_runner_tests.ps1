param([switch]$Run)
$ErrorActionPreference='Stop'
if(-not $Run){Write-Host 'Report: read-only generation preflight fixtures under artifacts. Add -Run; no process is started.';exit 0}
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $taskRepo 'tools/ModelAcceptanceContract.ps1')
$taskDirectory=Join-Path $taskRepo ('artifacts/acceptance-runner-tests/'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $taskDirectory -Force | Out-Null
$taskGateway=Join-Path $taskDirectory 'gateway.jsx';$taskFixture=Join-Path $taskDirectory 'fixtures.json';$taskChecks=0
foreach($taskGeneration in @(71,72)) {
    [IO.File]::WriteAllText($taskGateway,('var GATEWAY_BUILD = "native-presets-'+$taskGeneration+'";'))
    [IO.File]::WriteAllText($taskFixture,(@{version=1;gatewayBuild=('native-presets-'+$taskGeneration)} | ConvertTo-Json))
    if((Get-StarfieldAcceptanceBuild $taskDirectory $taskGateway) -ne ('native-presets-'+$taskGeneration)){throw 'Matching generation failed.'};$taskChecks++
}
foreach($taskBad in @(@{version=1;gatewayBuild='native-presets-71'},@{version=2;gatewayBuild='native-presets-72'},
    @{version=1;gatewayBuild='native-presets-72";bad'},@{version=1;gatewayBuild=@('native-presets-72')},@{version=1})) {
    [IO.File]::WriteAllText($taskFixture,($taskBad | ConvertTo-Json))
    $taskRejected=$false;try{Get-StarfieldAcceptanceBuild $taskDirectory $taskGateway | Out-Null}catch{$taskRejected=$true}
    if(-not $taskRejected){throw 'Invalid/mismatched generation accepted.'};$taskChecks++
}
Write-Host "model_acceptance_runner_tests: $taskChecks checks passed; no AE process or Adobe paths changed."
