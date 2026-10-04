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
$taskPanelRoot = [IO.Path]::GetFullPath((Join-Path $taskRepo 'cep_panel')) + '\'
$taskSavedRoot = [IO.Path]::GetFullPath((Join-Path $taskBackup 'panel')) + '\'
$taskCandidateRoot = [IO.Path]::GetFullPath((Join-Path $taskBackup 'candidate-panel')) + '\'
$taskEntries = @()
foreach ($taskEntry in $taskRecord.files) {
    if ([string]$taskEntry.path -notmatch '^cep_panel/[A-Za-z0-9_./-]+$' -or
        ([string]$taskEntry.path).Split('/') -contains '..') { throw 'Invalid paired panel path.' }
    $taskRelative = ([string]$taskEntry.path).Substring('cep_panel/'.Length)
    $taskTarget = [IO.Path]::GetFullPath((Join-Path $taskPanelRoot $taskRelative))
    $taskSaved = [IO.Path]::GetFullPath((Join-Path $taskSavedRoot $taskRelative))
    $taskRetained = [IO.Path]::GetFullPath((Join-Path $taskCandidateRoot $taskRelative))
    if (-not $taskTarget.StartsWith($taskPanelRoot,[StringComparison]::OrdinalIgnoreCase) -or
        -not $taskSaved.StartsWith($taskSavedRoot,[StringComparison]::OrdinalIgnoreCase) -or
        -not $taskRetained.StartsWith($taskCandidateRoot,[StringComparison]::OrdinalIgnoreCase)) { throw 'Paired panel path escapes its root.' }
    if ((Get-FileHash -LiteralPath $taskTarget -Algorithm SHA256).Hash -ne $taskEntry.installedHash) {
        throw "Panel source changed after deployment; refusing to overwrite: $taskTarget"
    }
    # Old records predate the existed field and always have a saved source.
    $taskExisted = -not ($taskEntry.PSObject.Properties.Name -contains 'existed') -or [bool]$taskEntry.existed
    if ($taskExisted) {
        if ((Get-FileHash -LiteralPath $taskSaved -Algorithm SHA256).Hash -ne $taskEntry.oldHash) { throw "Saved panel hash mismatch: $taskSaved" }
    } elseif ($taskEntry.oldHash -or (Test-Path -LiteralPath $taskSaved)) { throw 'Unexpected pre-deployment source for a newly added panel file.' }
    $taskEntries += [pscustomobject]@{target=$taskTarget;saved=$taskSaved;retained=$taskRetained;oldHash=$taskEntry.oldHash;existed=$taskExisted}
}
if (-not $taskEntries.Count) { throw 'Paired panel snapshot is empty.' }
Write-Host "Restore previous bundle through Deploy-TestBuild.ps1 and $($taskEntries.Count) verified CEP source files."
Write-Host "Keep candidate panel sources under $taskCandidateRoot"
if (-not $Restore) { Write-Host 'Read-only report. Add -Restore for the explicit action.'; exit 0 }
if (Get-Process AfterFX,AfterFX_64 -ErrorAction SilentlyContinue) { throw 'Close AE before restoring a paired build.' }
# Retain every candidate source before the first source is overwritten.
foreach ($taskEntry in $taskEntries) {
    if (Test-Path -LiteralPath $taskEntry.retained) { throw "Candidate retention path already exists: $($taskEntry.retained)" }
}
foreach ($taskEntry in $taskEntries) {
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
        Remove-Item -LiteralPath $taskEntry.target
        if (Test-Path -LiteralPath $taskEntry.target) { throw 'New panel source remains after rollback.' }
    }
}
Write-Host 'Previous paired bundle and panel restored; candidate sources retained. The CEP source rollback is visible in Git.'
