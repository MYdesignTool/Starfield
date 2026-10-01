# Focused deployment/rollback check entirely within the checkout.
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$fixture = Join-Path $repo 'artifacts\deploy-test\single-folder-build4'
$backupName = 'single-folder-build4-check'
$backup = Join-Path $repo "artifacts\disabled\$backupName"
$script = Join-Path $repo 'tools\Deploy-TestBuild.ps1'
$taskShell = Join-Path $PSHOME 'powershell.exe'
$bundle = Join-Path $repo 'dist'
$names = @('StarfieldParticle.aex', 'StarfieldEmitter.aex', 'StarfieldParticleNode.aex',
           'StarfieldAppearance.aex', 'StarfieldForce.aex', 'StarfieldCore.dll')
if (Test-Path -LiteralPath $fixture) { throw 'Fixture exists; preserve it for inspection.' }
if (Test-Path -LiteralPath $backup) { throw 'Prior check backup exists.' }
New-Item -ItemType Directory -Path $fixture -Force | Out-Null
$priorHashes = @{}
foreach ($name in $names) {
    $source = Join-Path $bundle $name
    Copy-Item -LiteralPath $source -Destination (Join-Path $fixture $name)
    $priorHashes[$name] = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
}
$thirdParty = Join-Path $fixture 'UnrelatedEffect.aex'
[IO.File]::WriteAllText($thirdParty, 'untouched')
New-Item -ItemType Junction -Path (Join-Path $fixture 'StarfieldRuntime') -Target (Join-Path $repo 'artifacts\runtime') | Out-Null
& $taskShell -NoProfile -ExecutionPolicy Bypass -File $script -PluginDir $fixture -BackupName $backupName
if ($LASTEXITCODE -ne 0 -or (Test-Path -LiteralPath $backup)) { throw 'Report mode modified deployment state.' }
& $taskShell -NoProfile -ExecutionPolicy Bypass -File $script -PluginDir $fixture -BackupName $backupName -Install
if ($LASTEXITCODE -ne 0) { throw 'Fixture installation failed.' }
$entry = Get-Item -LiteralPath (Join-Path $fixture 'Starfield') -Force
if ($entry.LinkType -ne 'Junction' -or [string]$entry.Target -ne $bundle) { throw 'Single bundle junction missing.' }
if (@(Get-ChildItem -LiteralPath $fixture -Force | Where-Object Name -Like 'Starfield*').Count -ne 1) { throw 'Loose entries remain.' }
$runtime = Join-Path $bundle 'StarfieldRuntime'
if ((Get-Item -LiteralPath $runtime -Force).LinkType) { throw 'Runtime must be an ordinary subdirectory.' }
$record = Get-Content -LiteralPath (Join-Path $backup 'deployment.json') -Raw | ConvertFrom-Json
foreach ($entry in $record.files) {
    if ((Get-FileHash -LiteralPath (Join-Path $bundle $entry.name) -Algorithm SHA256).Hash -ne $entry.installedHash) { throw 'Installed hash mismatch.' }
}
$selected = (Get-Content -LiteralPath (Join-Path $runtime 'current.txt') -Raw).Trim()
if ($selected -ne $record.runtimeName -or -not (Test-Path -LiteralPath (Join-Path $runtime $selected))) { throw 'Hot runtime selection missing.' }
& $taskShell -NoProfile -ExecutionPolicy Bypass -File $script -PluginDir $fixture -BackupName $backupName -Rollback
if ($LASTEXITCODE -ne 0) { throw 'Fixture rollback failed.' }
if (Test-Path -LiteralPath (Join-Path $fixture 'Starfield')) { throw 'Created junction was not undone.' }
if ((Get-Item -LiteralPath (Join-Path $fixture 'StarfieldRuntime') -Force).LinkType -ne 'Junction') { throw 'Original runtime junction was not restored.' }
foreach ($name in $names) {
    if ((Get-FileHash -LiteralPath (Join-Path $fixture $name) -Algorithm SHA256).Hash -ne $priorHashes[$name]) { throw 'Loose file was not restored.' }
    if ((Get-FileHash -LiteralPath (Join-Path $bundle $name) -Algorithm SHA256).Hash -ne $priorHashes[$name]) { throw 'Previous bundle file was not restored.' }
}
if ([IO.File]::ReadAllText($thirdParty) -ne 'untouched') { throw 'Third-party file changed.' }
Write-Host 'Single-folder report/install/hash/rollback/third-party preservation checks passed.'
