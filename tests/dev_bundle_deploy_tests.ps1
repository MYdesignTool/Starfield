# Single-folder transition/rollback, entirely within an isolated checkout.
# No dependency on or writes through the real dist/ development installation.
$ErrorActionPreference = 'Stop'
$taskRepo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskFixtureRepo = Join-Path $taskRepo ('artifacts\deploy-test\single-folder-' + [Guid]::NewGuid().ToString('N') + '\repo')
$taskTools = Join-Path $taskFixtureRepo 'tools'
$taskPlugins = Join-Path $taskFixtureRepo 'artifacts\deploy-test\host'
$taskBundle = Join-Path $taskFixtureRepo 'dist'
$taskRuntime = Join-Path $taskFixtureRepo 'artifacts\runtime'
New-Item -ItemType Directory -Force -Path $taskTools,$taskPlugins,$taskBundle,$taskRuntime | Out-Null
Copy-Item -LiteralPath (Join-Path $taskRepo 'tools\Deploy-TestBuild.ps1') -Destination $taskTools
$taskScript = Join-Path $taskTools 'Deploy-TestBuild.ps1'
$taskNames = @('StarfieldParticle.aex','StarfieldEmitter.aex','StarfieldParticleNode.aex',
    'StarfieldForce.aex','StarfieldTransform.aex','StarfieldHost.aex','StarfieldCore.dll')
$taskPrevious = @('StarfieldParticle.aex','StarfieldEmitter.aex','StarfieldParticleNode.aex',
    'StarfieldAppearance.aex','StarfieldForce.aex','StarfieldCore.dll')
$taskPrior = @{}
foreach ($taskName in $taskPrevious) {
    $taskPath = Join-Path $taskPlugins $taskName
    [IO.File]::WriteAllText($taskPath, "old $taskName")
    $taskPrior[$taskName] = (Get-FileHash -LiteralPath $taskPath).Hash
}
foreach ($taskName in $taskNames) {
    $taskKind = if ($taskName -eq 'StarfieldCore.dll') { 'core-dll' } else { 'plugin' }
    $taskSource = Join-Path $taskFixtureRepo "artifacts\$taskKind\2023\x64\Release\$taskName"
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $taskSource) | Out-Null
    [IO.File]::WriteAllText($taskSource, "new $taskName")
}
[IO.File]::WriteAllText((Join-Path $taskRuntime 'current.txt'), 'old-core.dll')
[IO.File]::WriteAllText((Join-Path $taskRuntime 'old-core.dll'), 'old runtime')
$taskThirdParty = Join-Path $taskPlugins 'UnrelatedEffect.aex'
[IO.File]::WriteAllText($taskThirdParty,'untouched')
New-Item -ItemType Junction -Path (Join-Path $taskPlugins 'StarfieldRuntime') -Target $taskRuntime | Out-Null
$taskBackup = Join-Path $taskFixtureRepo 'artifacts\disabled\check'
& $taskScript -PluginDir $taskPlugins -BackupName check
if (Test-Path -LiteralPath $taskBackup) { throw 'Report mode wrote a backup.' }
& $taskScript -PluginDir $taskPlugins -BackupName check -Install
$taskLink = Get-Item -LiteralPath (Join-Path $taskPlugins 'Starfield') -Force
if ($taskLink.LinkType -ne 'Junction' -or [string]$taskLink.Target -ne $taskBundle) { throw 'Single bundle junction missing.' }
if (@(Get-ChildItem -LiteralPath $taskPlugins -Force | Where-Object Name -Like 'Starfield*').Count -ne 1) { throw 'Loose Starfield entries remain.' }
if (Test-Path -LiteralPath (Join-Path $taskBundle 'StarfieldAppearance.aex')) { throw 'Retired Appearance installed.' }
$taskRecord = Get-Content -LiteralPath (Join-Path $taskBackup 'deployment.json') -Raw | ConvertFrom-Json
foreach ($taskEntry in $taskRecord.files) {
    if ($taskEntry.installedHash -and (Get-FileHash -LiteralPath (Join-Path $taskBundle $taskEntry.name)).Hash -ne $taskEntry.installedHash) {
        throw 'Installed candidate hash mismatch.'
    }
}
$taskSelected = (Get-Content -LiteralPath (Join-Path $taskBundle 'StarfieldRuntime\current.txt') -Raw).Trim()
if ($taskSelected -ne $taskRecord.runtimeName) { throw 'Runtime selection mismatch.' }
if ((Get-Item -LiteralPath (Join-Path $taskBundle 'StarfieldRuntime') -Force).LinkType) { throw 'Bundle runtime is a link.' }
& $taskScript -PluginDir $taskPlugins -BackupName check -Rollback
if (Test-Path -LiteralPath (Join-Path $taskPlugins 'Starfield')) { throw 'Created bundle link remains after rollback.' }
if ((Get-Item -LiteralPath (Join-Path $taskPlugins 'StarfieldRuntime') -Force).LinkType -ne 'Junction') { throw 'Original runtime link not restored.' }
foreach ($taskName in $taskPrevious) {
    if ((Get-FileHash -LiteralPath (Join-Path $taskPlugins $taskName)).Hash -ne $taskPrior[$taskName]) { throw 'Original loose file not restored.' }
}
if ([IO.File]::ReadAllText($taskThirdParty) -ne 'untouched') { throw 'Unrelated file changed.' }
Write-Host 'Isolated single-folder report/install/hash/retired-node/rollback checks passed.'
