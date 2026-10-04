# Only an isolated checkout under artifacts; no live installation is modified.
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskFixture=Join-Path $taskRepo ('artifacts\deploy-test\paired-panel-'+[DateTime]::UtcNow.ToString('yyyyMMddHHmmssfff')+'\repo')
$taskPlugins=Join-Path $taskFixture 'artifacts\deploy-test\ae\Plug-ins'
$taskTools=Join-Path $taskFixture 'tools'
$taskPanel=Join-Path $taskFixture 'cep_panel'
$taskBundle=Join-Path $taskFixture 'dist'
$taskBackup=Join-Path $taskFixture 'artifacts\disabled\check'
New-Item -ItemType Directory -Path $taskPlugins,$taskTools,$taskPanel,$taskBundle -Force | Out-Null
foreach($taskName in @('Deploy-TestBuild.ps1','Restore-TestBuild.ps1')) {
    Copy-Item -LiteralPath (Join-Path $taskRepo ('tools\'+$taskName)) -Destination $taskTools
}
$taskNames=@('StarfieldParticle.aex','StarfieldEmitter.aex','StarfieldParticleNode.aex','StarfieldForce.aex','StarfieldHost.aex','StarfieldCore.dll')
$taskOldHashes=@{}
foreach($taskName in $taskNames) {
    $taskPath=Join-Path $taskBundle $taskName
    [IO.File]::WriteAllText($taskPath,'old '+$taskName)
    $taskOldHashes[$taskName]=(Get-FileHash -LiteralPath $taskPath -Algorithm SHA256).Hash
    $taskKind=if($taskName -eq 'StarfieldCore.dll'){'core-dll'}else{'plugin'}
    $taskSource=Join-Path $taskFixture "artifacts\$taskKind\2023\x64\Release\$taskName"
    New-Item -ItemType Directory -Path (Split-Path -Parent $taskSource) -Force | Out-Null
    [IO.File]::WriteAllText($taskSource,'new '+$taskName)
}
New-Item -ItemType Junction -Path (Join-Path $taskPlugins 'Starfield') -Target $taskBundle | Out-Null
& (Join-Path $taskTools 'Deploy-TestBuild.ps1') -PluginDir $taskPlugins -BackupName check -Install
New-Item -ItemType Directory -Path (Join-Path $taskBackup 'panel') -Force | Out-Null
[IO.File]::WriteAllText((Join-Path $taskBackup 'panel\index.html'),'old panel')
[IO.File]::WriteAllText((Join-Path $taskPanel 'index.html'),'new panel')
[IO.File]::WriteAllText((Join-Path $taskPanel 'gradient_editor.js'),'new gradient')
$taskRecord=@{files=@(
    # Legacy record without existed still restores the saved source.
    @{path='cep_panel/index.html';oldHash=(Get-FileHash -LiteralPath (Join-Path $taskBackup 'panel\index.html')).Hash;installedHash=(Get-FileHash -LiteralPath (Join-Path $taskPanel 'index.html')).Hash},
    @{path='cep_panel/gradient_editor.js';existed=$false;oldHash=$null;installedHash=(Get-FileHash -LiteralPath (Join-Path $taskPanel 'gradient_editor.js')).Hash}
)}
$taskManifest=Join-Path $taskBackup 'panel-snapshot.json'
$taskRecord | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $taskManifest -Encoding UTF8
$taskRestore=Join-Path $taskTools 'Restore-TestBuild.ps1'
& powershell -NoProfile -ExecutionPolicy Bypass -File $taskRestore -PluginDir $taskPlugins -BackupName check
if($LASTEXITCODE -ne 0){throw 'Read-only paired report failed.'}
if(Test-Path -LiteralPath (Join-Path $taskBackup 'candidate-panel')){throw 'Read-only report retained files.'}
[IO.File]::WriteAllText((Join-Path $taskPanel 'gradient_editor.js'),'changed after deploy')
$ErrorActionPreference='Continue'
& powershell -NoProfile -ExecutionPolicy Bypass -File $taskRestore -PluginDir $taskPlugins -BackupName check -Restore *> (Join-Path $taskFixture 'rejected-change.log')
$ErrorActionPreference='Stop'
if($LASTEXITCODE -eq 0 -or (Test-Path -LiteralPath (Join-Path $taskBackup 'candidate-panel'))){throw 'Changed candidate was overwritten.'}
[IO.File]::WriteAllText((Join-Path $taskPanel 'gradient_editor.js'),'new gradient')
$taskRecord.files[1].path='cep_panel/../dist/StarfieldParticle.aex'
$taskRecord | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $taskManifest -Encoding UTF8
$ErrorActionPreference='Continue'
& powershell -NoProfile -ExecutionPolicy Bypass -File $taskRestore -PluginDir $taskPlugins -BackupName check -Restore *> (Join-Path $taskFixture 'rejected-path.log')
$ErrorActionPreference='Stop'
if($LASTEXITCODE -eq 0){throw 'Path traversal was accepted.'}
$taskRecord.files[1].path='cep_panel/gradient_editor.js'
$taskRecord | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $taskManifest -Encoding UTF8
& powershell -NoProfile -ExecutionPolicy Bypass -File $taskRestore -PluginDir $taskPlugins -BackupName check -Restore
if($LASTEXITCODE -ne 0){throw 'Paired restore failed.'}
if([IO.File]::ReadAllText((Join-Path $taskPanel 'index.html')) -ne 'old panel'){throw 'Old source not restored.'}
if(Test-Path -LiteralPath (Join-Path $taskPanel 'gradient_editor.js')){throw 'New source remains loadable.'}
if([IO.File]::ReadAllText((Join-Path $taskBackup 'candidate-panel\gradient_editor.js')) -ne 'new gradient'){throw 'New source not retained.'}
if([IO.File]::ReadAllText((Join-Path $taskBackup 'candidate-panel\index.html')) -ne 'new panel'){throw 'Changed source not retained.'}
foreach($taskName in $taskNames) {
    if((Get-FileHash -LiteralPath (Join-Path $taskBundle $taskName)).Hash -ne $taskOldHashes[$taskName]){throw 'Old binary not restored.'}
}
Write-Host 'Isolated paired report, hash/path rejection, legacy manifest, added source retention and binary rollback passed.'
