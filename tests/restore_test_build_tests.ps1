# Minimal paired rollback exercise, isolated under ignored artifacts.
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskRoot=Join-Path $taskRepo ("artifacts\deploy-test\restore-"+[DateTime]::UtcNow.Ticks)
New-Item -ItemType Directory -Path (Join-Path $taskRoot 'tools') -Force | Out-Null
foreach($taskScript in @('Deploy-TestBuild.ps1','Restore-TestBuild.ps1')) {
    Copy-Item -LiteralPath (Join-Path $taskRepo "tools\$taskScript") -Destination (Join-Path $taskRoot "tools\$taskScript")
}
$taskPluginDir=Join-Path $taskRoot 'artifacts\deploy-test\host'
$taskBundle=Join-Path $taskRoot 'dist'
New-Item -ItemType Directory -Path $taskPluginDir,$taskBundle -Force | Out-Null
$taskNames=@('StarfieldParticle.aex','StarfieldEmitter.aex','StarfieldParticleNode.aex','StarfieldAppearance.aex','StarfieldForce.aex','StarfieldCore.dll')
$taskPrior=@{}
foreach($taskName in $taskNames) {
    $taskKind=if($taskName -eq 'StarfieldCore.dll') {'core-dll'} else {'plugin'}
    $taskSource=Join-Path $taskRoot "artifacts\$taskKind\2023\x64\Release\$taskName"
    New-Item -ItemType Directory -Path (Split-Path -Parent $taskSource) -Force | Out-Null
    [IO.File]::WriteAllText($taskSource,"new $taskName")
    [IO.File]::WriteAllText((Join-Path $taskBundle $taskName),"old $taskName")
    $taskPrior[$taskName]=(Get-FileHash -LiteralPath (Join-Path $taskBundle $taskName)).Hash
}
& (Join-Path $taskRoot 'tools\Deploy-TestBuild.ps1') -PluginDir $taskPluginDir -BackupName paired -Install
$taskBackup=Join-Path $taskRoot 'artifacts\disabled\paired'
$taskTarget=Join-Path $taskRoot 'cep_panel\js\fixture.js'
$taskSaved=Join-Path $taskBackup 'panel\js\fixture.js'
New-Item -ItemType Directory -Path (Split-Path -Parent $taskTarget),(Split-Path -Parent $taskSaved) -Force | Out-Null
[IO.File]::WriteAllText($taskTarget,'new panel')
[IO.File]::WriteAllText($taskSaved,'old panel')
$taskRecord=@{files=@(@{path='cep_panel/js/fixture.js';installedHash=(Get-FileHash $taskTarget).Hash;oldHash=(Get-FileHash $taskSaved).Hash})}
$taskRecord | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $taskBackup 'panel-snapshot.json') -Encoding UTF8
& (Join-Path $taskRoot 'tools\Restore-TestBuild.ps1') -PluginDir $taskPluginDir -BackupName paired
if([IO.File]::ReadAllText($taskTarget) -ne 'new panel') {throw 'Report mode changed a source.'}
[IO.File]::WriteAllText($taskTarget,'later owner edit')
$taskRejected=$false
try {& (Join-Path $taskRoot 'tools\Restore-TestBuild.ps1') -PluginDir $taskPluginDir -BackupName paired -Restore} catch {$taskRejected=$true}
if(-not $taskRejected -or [IO.File]::ReadAllText($taskTarget) -ne 'later owner edit') {throw 'Modified source was overwritten.'}
[IO.File]::WriteAllText($taskTarget,'new panel')
& (Join-Path $taskRoot 'tools\Restore-TestBuild.ps1') -PluginDir $taskPluginDir -BackupName paired -Restore
if([IO.File]::ReadAllText($taskTarget) -ne 'old panel' -or
   [IO.File]::ReadAllText((Join-Path $taskBackup 'candidate-panel\js\fixture.js')) -ne 'new panel') {throw 'Paired source retention/restoration failed.'}
foreach($taskName in $taskNames) {
    if((Get-FileHash -LiteralPath (Join-Path $taskBundle $taskName)).Hash -ne $taskPrior[$taskName]) {throw 'Previous bundle hash was not restored.'}
}
Write-Host 'Paired rollback report, changed-source rejection, candidate retention and six restored bundle hashes passed.'
