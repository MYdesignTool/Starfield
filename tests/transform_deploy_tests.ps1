# Isolated bundle simulation. All files and the test junction remain in artifacts.
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskScratch=Join-Path $taskRepo "artifacts\transform-deploy-tests\$([Guid]::NewGuid().ToString('N'))"
$taskTools=Join-Path $taskScratch 'tools'
$taskPlugins=Join-Path $taskScratch 'artifacts\deploy-test\host'
$taskDist=Join-Path $taskScratch 'dist'
$taskBuild=Join-Path $taskScratch 'artifacts\plugin\2023\x64\Release'
$taskCore=Join-Path $taskScratch 'artifacts\core-dll\2023\x64\Release'
New-Item -ItemType Directory -Path $taskTools,$taskPlugins,$taskDist,$taskBuild,$taskCore -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $taskRepo 'tools\Deploy-TestBuild.ps1') -Destination $taskTools
$taskScript=Join-Path $taskTools 'Deploy-TestBuild.ps1'
$taskOldNames=@('StarfieldParticle.aex','StarfieldEmitter.aex','StarfieldParticleNode.aex','StarfieldForce.aex','StarfieldHost.aex','StarfieldCore.dll')
$taskNames=@($taskOldNames)+@('StarfieldTransform.aex')
foreach($taskName in $taskNames) {
    $taskDir=if($taskName -eq 'StarfieldCore.dll'){$taskCore}else{$taskBuild}
    [IO.File]::WriteAllText((Join-Path $taskDir $taskName),"candidate-$taskName")
}
foreach($taskName in $taskOldNames){[IO.File]::WriteAllText((Join-Path $taskDist $taskName),"baseline-$taskName")}
$taskOldHash=(Get-FileHash -LiteralPath (Join-Path $taskDist 'StarfieldCore.dll')).Hash
$taskRuntime=Join-Path $taskDist 'StarfieldRuntime'
New-Item -ItemType Directory -Path $taskRuntime | Out-Null
$taskOldRuntime="StarfieldCore-$($taskOldHash.Substring(0,16)).dll"
Copy-Item -LiteralPath (Join-Path $taskDist 'StarfieldCore.dll') -Destination (Join-Path $taskRuntime $taskOldRuntime)
[IO.File]::WriteAllText((Join-Path $taskRuntime 'current.txt'),"$taskOldRuntime`n")
New-Item -ItemType Junction -Path (Join-Path $taskPlugins 'Starfield') -Target $taskDist | Out-Null
$taskChecks=0
function Check([bool]$ok,[string]$why){$script:taskChecks++;if(-not $ok){throw $why}}
& $taskScript -PluginDir $taskPlugins -BackupName keep46 -KeepNative -Install | Out-Null
Check (-not (Test-Path -LiteralPath (Join-Path $taskDist 'StarfieldTransform.aex'))) 'KeepNative exposed an unrelated candidate.'
& $taskScript -PluginDir $taskPlugins -BackupName native47 -Install | Out-Null
$taskManifest=Get-Content -LiteralPath (Join-Path $taskScratch 'artifacts\disabled\native47\deployment.json') -Raw | ConvertFrom-Json
$taskEntry=$taskManifest.files | Where-Object name -eq 'StarfieldTransform.aex'
Check ($null -ne $taskEntry -and -not $taskEntry.existed -and $taskEntry.installedHash) 'New module did not retain its prior absence.'
foreach($taskName in $taskNames){
    $taskDir=if($taskName -eq 'StarfieldCore.dll'){$taskCore}else{$taskBuild}
    Check ((Get-FileHash -LiteralPath (Join-Path $taskDist $taskName)).Hash -eq (Get-FileHash -LiteralPath (Join-Path $taskDir $taskName)).Hash) 'Installed hash differs.'
}
& $taskScript -PluginDir $taskPlugins -BackupName native47 -Rollback | Out-Null
Check (-not (Test-Path -LiteralPath (Join-Path $taskDist 'StarfieldTransform.aex'))) 'Rollback did not remove the new module from the live bundle.'
Check (Test-Path -LiteralPath (Join-Path $taskScratch 'artifacts\disabled\native47\candidate\StarfieldTransform.aex')) 'Rollback did not retain the candidate.'
foreach($taskName in $taskOldNames){Check ([IO.File]::ReadAllText((Join-Path $taskDist $taskName)) -eq "baseline-$taskName") 'Rollback did not restore the baseline.'}
Check ([IO.File]::ReadAllText((Join-Path $taskRuntime 'current.txt')).Trim() -eq $taskOldRuntime) 'Rollback did not restore the selector.'
Write-Output "Transform deployment: $taskChecks checks passed; isolated scratch remains under artifacts."
