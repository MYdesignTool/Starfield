# Restore a paired development bundle and its saved CEP sources. Default reports.
param(
    [Parameter(Mandatory=$true)][string]$PluginDir,
    [Parameter(Mandatory=$true)][ValidatePattern('^[A-Za-z0-9._-]+$')][string]$BackupName,
    [switch]$Restore
)
$ErrorActionPreference = 'Stop'
$taskRepo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskBackup = Join-Path $taskRepo "artifacts\disabled\$BackupName"
$taskManifest = Join-Path $taskBackup 'panel-snapshot.json'
if (-not (Test-Path -LiteralPath $taskManifest)) { throw 'Paired panel snapshot is missing; use the original deployment rollback for binary-only backups.' }
$taskRecord = Get-Content -LiteralPath $taskManifest -Raw -Encoding UTF8 | ConvertFrom-Json
$taskPanelDirectory=if(Test-Path -LiteralPath (Join-Path $taskRepo 'web_panel')){'web_panel'}else{'cep_panel'}
$taskPanelRoot = [IO.Path]::GetFullPath((Join-Path $taskRepo $taskPanelDirectory)) + '\'
$taskRestoreDirectory=if($taskRecord.restoreSourceDirectory){[string]$taskRecord.restoreSourceDirectory}else{$taskPanelDirectory}
if($taskRestoreDirectory -cnotin @('web_panel','cep_panel')){throw 'Invalid restore source directory.'}
if($taskRestoreDirectory -cne $taskPanelDirectory -and (Test-Path -LiteralPath (Join-Path $taskRepo $taskRestoreDirectory))){throw 'Restore source rename target already exists.'}
$taskSavedRoot = [IO.Path]::GetFullPath((Join-Path $taskBackup 'panel')) + '\'
$taskCandidateRoot = [IO.Path]::GetFullPath((Join-Path $taskBackup 'candidate-panel')) + '\'
$taskEntries = @()
foreach ($taskEntry in $taskRecord.files) {
    if ([string]$taskEntry.path -notmatch '^(web_panel|cep_panel)/[A-Za-z0-9_./-]+$' -or
        ([string]$taskEntry.path).Split('/') -contains '..') { throw 'Invalid paired panel path.' }
    $taskRelative = ([string]$taskEntry.path).Substring(([string]$taskEntry.path).IndexOf('/')+1)
    $taskTarget = [IO.Path]::GetFullPath((Join-Path $taskPanelRoot $taskRelative))
    $taskSaved = [IO.Path]::GetFullPath((Join-Path $taskSavedRoot $taskRelative))
    $taskRetained = [IO.Path]::GetFullPath((Join-Path $taskCandidateRoot $taskRelative))
    if (-not $taskTarget.StartsWith($taskPanelRoot,[StringComparison]::OrdinalIgnoreCase) -or
        -not $taskSaved.StartsWith($taskSavedRoot,[StringComparison]::OrdinalIgnoreCase) -or
        -not $taskRetained.StartsWith($taskCandidateRoot,[StringComparison]::OrdinalIgnoreCase)) { throw 'Paired panel path escapes its root.' }
    # Old records predate the existed field and always have a saved source.
    $taskExisted = -not ($taskEntry.PSObject.Properties.Name -contains 'existed') -or [bool]$taskEntry.existed
    $taskRemoved = ($taskEntry.PSObject.Properties.Name -contains 'removed') -and $taskEntry.removed -eq $true
    if($taskRemoved -and (-not $taskExisted -or $taskEntry.installedHash -or -not $taskEntry.oldHash)) {
        throw 'Invalid retired panel source record.'
    }
    $taskPresent=Test-Path -LiteralPath $taskTarget
    if($taskPresent){
        if($taskRemoved){throw "Retired panel source reappeared after deployment: $taskTarget"}
        if ((Get-FileHash -LiteralPath $taskTarget -Algorithm SHA256).Hash -ne $taskEntry.installedHash) {
            throw "Panel source changed after deployment; refusing to overwrite: $taskTarget"
        }
    }elseif(-not $taskRemoved -and ($taskEntry.installedHash -or $taskExisted)){throw 'Expected paired panel source is missing.'}
    if ($taskExisted) {
        if ((Get-FileHash -LiteralPath $taskSaved -Algorithm SHA256).Hash -ne $taskEntry.oldHash) { throw "Saved panel hash mismatch: $taskSaved" }
    } elseif ($taskEntry.oldHash -or (Test-Path -LiteralPath $taskSaved)) { throw 'Unexpected pre-deployment source for a newly added panel file.' }
    $taskEntries += [pscustomobject]@{target=$taskTarget;saved=$taskSaved;retained=$taskRetained;oldHash=$taskEntry.oldHash;existed=$taskExisted;present=$taskPresent}
}
if (-not $taskEntries.Count) { throw 'Paired panel snapshot is empty.' }
Write-Host "Restore previous bundle through Deploy-TestBuild.ps1 and $($taskEntries.Count) verified panel source files."
Write-Host "Keep candidate panel sources under $taskCandidateRoot"
Write-Host "Source folder after restore: $taskRestoreDirectory (legacy manifest prefixes remain compatible)."
if (-not $Restore) { Write-Host 'Read-only report. Add -Restore for the explicit action.'; exit 0 }
if (Get-Process AfterFX,AfterFX_64 -ErrorAction SilentlyContinue) { throw 'Close AE before restoring a paired build.' }
# Retain every candidate source before the first source is overwritten.
foreach ($taskEntry in $taskEntries) {
    if (Test-Path -LiteralPath $taskEntry.retained) { throw "Candidate retention path already exists: $($taskEntry.retained)" }
}
foreach ($taskEntry in $taskEntries) {
    if(-not $taskEntry.present){continue}
    New-Item -ItemType Directory -Path (Split-Path -Parent $taskEntry.retained) -Force | Out-Null
    Copy-Item -LiteralPath $taskEntry.target -Destination $taskEntry.retained
}
& (Join-Path $PSScriptRoot 'Deploy-TestBuild.ps1') -PluginDir $PluginDir -BackupName $BackupName -Rollback
foreach ($taskEntry in $taskEntries) {
    if ($taskEntry.existed) {
        Copy-Item -LiteralPath $taskEntry.saved -Destination $taskEntry.target -Force
        if ((Get-FileHash -LiteralPath $taskEntry.target -Algorithm SHA256).Hash -ne $taskEntry.oldHash) { throw 'Restored panel hash mismatch.' }
    } else {
        # Exact validated checkout file; candidate bytes were retained above.
        if(Test-Path -LiteralPath $taskEntry.target){Remove-Item -LiteralPath $taskEntry.target}
        if (Test-Path -LiteralPath $taskEntry.target) { throw 'New panel source remains after rollback.' }
    }
}
if($taskRestoreDirectory -cne $taskPanelDirectory){
    # Both exact direct children were validated above; never move outside repo.
    Rename-Item -LiteralPath $taskPanelRoot.TrimEnd('\') -NewName $taskRestoreDirectory
}
Write-Host 'Previous paired bundle and panel restored; candidate sources retained. The source rollback is visible in Git.'
