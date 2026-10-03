# Isolated bundle: this fixture never reads or replaces the live dist/ files.
$ErrorActionPreference = 'Stop'
$taskRepo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskRoot = Join-Path $taskRepo ('artifacts\deploy-test\retired-node-' + [DateTime]::UtcNow.ToString('yyyyMMddHHmmssfff'))
$taskFixtureRepo = Join-Path $taskRoot 'repo'
$taskPlugins = Join-Path $taskFixtureRepo 'artifacts\deploy-test\ae\Plug-ins'
$taskTools = Join-Path $taskFixtureRepo 'tools'
$taskBundle = Join-Path $taskFixtureRepo 'dist'
$taskRuntime = Join-Path $taskBundle 'StarfieldRuntime'
New-Item -ItemType Directory -Path $taskPlugins,$taskTools,$taskRuntime -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $taskRepo 'tools\Deploy-TestBuild.ps1') -Destination $taskTools
$taskScript = Join-Path $taskTools 'Deploy-TestBuild.ps1'
$taskNames = @('StarfieldParticle.aex','StarfieldEmitter.aex','StarfieldParticleNode.aex','StarfieldForce.aex','StarfieldHost.aex','StarfieldCore.dll')
$taskPrevious = @('StarfieldParticle.aex','StarfieldEmitter.aex','StarfieldParticleNode.aex','StarfieldForce.aex','StarfieldAppearance.aex','StarfieldCore.dll')
$taskOldHashes = @{}
foreach ($taskName in $taskPrevious) {
    $taskTarget = Join-Path $taskBundle $taskName
    [IO.File]::WriteAllText($taskTarget,"old $taskName")
    $taskOldHashes[$taskName] = (Get-FileHash -LiteralPath $taskTarget -Algorithm SHA256).Hash
}
foreach ($taskName in $taskNames) {
    $taskKind = if ($taskName -eq 'StarfieldCore.dll') { 'core-dll' } else { 'plugin' }
    $taskSource = Join-Path $taskFixtureRepo "artifacts\$taskKind\2023\x64\Release\$taskName"
    New-Item -ItemType Directory -Path (Split-Path -Parent $taskSource) -Force | Out-Null
    [IO.File]::WriteAllText($taskSource,"new $taskName")
}
[IO.File]::WriteAllText((Join-Path $taskRuntime 'current.txt'),"old-core.dll`n")
[IO.File]::WriteAllText((Join-Path $taskRuntime 'old-core.dll'),'old runtime')
[IO.File]::WriteAllText((Join-Path $taskPlugins 'Unrelated.aex'),'untouched')
New-Item -ItemType Junction -Path (Join-Path $taskPlugins 'Starfield') -Target $taskBundle | Out-Null
& $taskScript -PluginDir $taskPlugins -BackupName check
if (Test-Path -LiteralPath (Join-Path $taskFixtureRepo 'artifacts\disabled\check')) { throw 'Report changed files.' }
& $taskScript -PluginDir $taskPlugins -BackupName check -Install
$taskRecord = Get-Content -LiteralPath (Join-Path $taskFixtureRepo 'artifacts\disabled\check\deployment.json') -Raw | ConvertFrom-Json
if (Test-Path -LiteralPath (Join-Path $taskBundle 'StarfieldAppearance.aex')) { throw 'Retired node still loadable.' }
foreach ($taskEntry in $taskRecord.files) {
    if ($taskEntry.installedHash -and (Get-FileHash -LiteralPath (Join-Path $taskBundle $taskEntry.name) -Algorithm SHA256).Hash -ne $taskEntry.installedHash) { throw 'Installed hash differs.' }
}
if (@(Get-ChildItem -LiteralPath $taskPlugins -Force | Where-Object Name -Like 'Starfield*').Count -ne 1) { throw 'More than one Starfield root entry.' }
if ([IO.File]::ReadAllText((Join-Path $taskPlugins 'Unrelated.aex')) -ne 'untouched') { throw 'Unrelated plug-in changed.' }
& $taskScript -PluginDir $taskPlugins -BackupName check -Rollback
foreach ($taskName in $taskPrevious) {
    if ((Get-FileHash -LiteralPath (Join-Path $taskBundle $taskName) -Algorithm SHA256).Hash -ne $taskOldHashes[$taskName]) { throw 'Previous bundle hash differs.' }
}
if (Test-Path -LiteralPath (Join-Path $taskBundle 'StarfieldHost.aex')) { throw 'New module was not undone.' }
if ((Get-Content -LiteralPath (Join-Path $taskRuntime 'current.txt') -Raw).Trim() -ne 'old-core.dll') { throw 'Runtime selector not restored.' }
Write-Host 'Isolated report/install/retired-module/hash/single-junction/rollback checks passed.'
