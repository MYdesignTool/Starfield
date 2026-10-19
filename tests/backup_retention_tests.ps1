$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskFixture=Join-Path $taskRepo ('artifacts/retention-tests/'+[guid]::NewGuid())
New-Item -ItemType Directory -Path (Join-Path $taskFixture 'tools') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $taskRepo 'tools/Prune-DeploymentBackups.ps1') -Destination (Join-Path $taskFixture 'tools')
$taskGuard=Join-Path $taskFixture 'survivor';New-Item -ItemType Directory -Path $taskGuard | Out-Null
Set-Content -LiteralPath (Join-Path $taskGuard 'keep.txt') -Value 'untouched'
foreach($taskIndex in 1..7){
    $taskDir=Join-Path $taskFixture ('artifacts/disabled/version'+$taskIndex)
    New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
    $taskRecord=if($taskIndex -eq 1){@{pluginDir=(Join-Path $taskFixture 'host');runtimeName='StarfieldCore-1234.dll';files=@(@{name='StarfieldCore.dll'})}}
        else{@{bundle=(Join-Path $taskFixture 'dist');pluginDir=(Join-Path $taskFixture 'host')}}
    $taskRecord | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $taskDir 'deployment.json') -Encoding UTF8
    (Get-Item -LiteralPath (Join-Path $taskDir 'deployment.json')).LastWriteTimeUtc=[datetime]'2026-01-01'+[timespan]::FromDays($taskIndex)
    Set-Content -LiteralPath (Join-Path $taskDir 'old.bin') -Value 'payload'
}
New-Item -ItemType Junction -Path (Join-Path $taskFixture 'artifacts/disabled/version1/archived-link') -Target $taskGuard | Out-Null
& (Join-Path $taskFixture 'tools/Prune-DeploymentBackups.ps1') -Keep 5 | Out-Null
if(-not(Test-Path (Join-Path $taskFixture 'artifacts/disabled/version1/old.bin'))){throw 'Report mutated backup'}
& (Join-Path $taskFixture 'tools/Prune-DeploymentBackups.ps1') -Keep 5 -Prune | Out-Null
if(Test-Path (Join-Path $taskFixture 'artifacts/disabled/version2')){throw 'Old backup not pruned'}
foreach($taskIndex in 3..7){if(-not(Test-Path (Join-Path $taskFixture ('artifacts/disabled/version'+$taskIndex+'/old.bin')))){throw 'Recent backup removed'}}
if((Get-Content -LiteralPath (Join-Path $taskGuard 'keep.txt') -Raw).Trim() -cne 'untouched'){throw 'Junction target touched'}
Write-Host 'Backup retention: report-only, newest-five retention, manifests and non-traversing archived Junction deletion passed.'
