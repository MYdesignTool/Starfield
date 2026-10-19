# Owner-approved retention for this checkout's actual deployment backups only.
param([ValidateRange(3,100)][int]$Keep=5,[switch]$Prune)
$ErrorActionPreference='Stop'
Import-Module Microsoft.PowerShell.Utility
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskRoot=[IO.Path]::GetFullPath((Join-Path $taskRepo 'artifacts/disabled')).TrimEnd('\')
if(-not(Test-Path -LiteralPath $taskRoot)){Write-Host 'No deployment backups.';exit 0}
if((Get-Item -LiteralPath $taskRoot -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Backup root must not be a link.'}
$taskRecords=@(Get-ChildItem -LiteralPath $taskRoot -Directory | Where-Object {
    -not ($_.Attributes -band [IO.FileAttributes]::ReparsePoint) -and (Test-Path -LiteralPath (Join-Path $_.FullName 'deployment.json'))
} | ForEach-Object {
    $taskManifest=Get-Item -LiteralPath (Join-Path $_.FullName 'deployment.json')
    $taskRecord=Get-Content -LiteralPath $taskManifest.FullName -Raw -Encoding UTF8 | ConvertFrom-Json
    [pscustomobject]@{name=$_.Name;path=$_.FullName;time=$taskManifest.LastWriteTimeUtc;manifest=$taskManifest.FullName;record=$taskRecord}
})
$taskOwnedPlugins=@($taskRecords | Where-Object {$_.record.bundle -eq (Join-Path $taskRepo 'dist')} |
    ForEach-Object {$_.record.pluginDir} | Where-Object {$_} | Sort-Object -Unique)
$taskCandidates=@($taskRecords | Where-Object {
    $_.record.bundle -eq (Join-Path $taskRepo 'dist') -or
    (-not $_.record.bundle -and $_.record.pluginDir -in $taskOwnedPlugins -and
     $_.record.runtimeName -match '^StarfieldCore-[A-Fa-f0-9]+\.dll$' -and
     @($_.record.files).Count -gt 0 -and @($_.record.files | Where-Object {$_.name -notmatch '^Starfield[A-Za-z0-9]+\.(aex|dll)$'}).Count -eq 0)
} | Sort-Object time -Descending)
$taskRetained=@($taskCandidates | Select-Object -First $Keep)
$taskOld=@($taskCandidates | Select-Object -Skip $Keep)
[ordered]@{retained=@($taskRetained.name);remove=@($taskOld.name);root=$taskRoot;action=$(if($Prune){'Prune'}else{'Report'})} | ConvertTo-Json -Depth 5
if(-not $Prune){Write-Host 'Report only. Add -Prune after owner authorizes deletion of older backups.';exit 0}
if(-not $taskOld.Count){Write-Host 'No older deployment backups to prune.';exit 0}
if(Get-Process AfterFX,AfterFX_64 -ErrorAction SilentlyContinue){throw 'Close AE before pruning deployment backups.'}
$taskEvidence=Join-Path $taskRepo ('artifacts/rollback-retention/'+(Get-Date -Format 'yyyyMMdd-HHmmssfff'))
New-Item -ItemType Directory -Path $taskEvidence -Force | Out-Null
$taskPlans=@()
foreach($taskEntry in $taskOld){
    $taskAbsolute=[IO.Path]::GetFullPath($taskEntry.path)
    if(-not $taskAbsolute.StartsWith($taskRoot+'\',[StringComparison]::OrdinalIgnoreCase) -or
       (Split-Path -Parent $taskAbsolute) -ne $taskRoot){throw 'Deletion target escapes the exact backup root.'}
    $taskStack=[Collections.Generic.Stack[string]]::new();$taskStack.Push($taskAbsolute)
    $taskLinks=@();$taskBytes=[long]0;$taskFiles=0
    while($taskStack.Count){
        foreach($taskChild in Get-ChildItem -LiteralPath $taskStack.Pop() -Force){
            if($taskChild.Name -eq '.git'){throw "Refusing a Git checkout in backup: $taskAbsolute"}
            if($taskChild.Attributes -band [IO.FileAttributes]::ReparsePoint){$taskLinks+=@{path=$taskChild.FullName;directory=[bool]$taskChild.PSIsContainer};continue}
            if($taskChild.PSIsContainer){$taskStack.Push($taskChild.FullName)}else{$taskBytes+=$taskChild.Length;++$taskFiles}
        }
    }
    Copy-Item -LiteralPath $taskEntry.manifest -Destination (Join-Path $taskEvidence ($taskEntry.name+'-deployment.json'))
    $taskPlans+=[pscustomobject]@{name=$taskEntry.name;path=$taskAbsolute;bytes=$taskBytes;files=$taskFiles;links=$taskLinks}
}
@{retained=@($taskRetained.name);remove=$taskPlans;policyKeep=$Keep} | ConvertTo-Json -Depth 8 |
    Set-Content -LiteralPath (Join-Path $taskEvidence 'before.json') -Encoding UTF8
foreach($taskEntry in $taskPlans){
    if(-not ([IO.Path]::GetFullPath($taskEntry.path)).StartsWith($taskRoot+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Invalid deletion target.'}
    # Do not traverse archived Junctions/symlinks or touch their targets.
    foreach($taskLink in $taskEntry.links){
        if(-not ([IO.Path]::GetFullPath($taskLink.path)).StartsWith($taskEntry.path+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Invalid archived link path.'}
        if($taskLink.directory){[IO.Directory]::Delete($taskLink.path,$false)}else{[IO.File]::Delete($taskLink.path)}
    }
    Remove-Item -LiteralPath $taskEntry.path -Recurse -Force
}
foreach($taskEntry in $taskRetained){if(-not(Test-Path -LiteralPath $taskEntry.manifest)){throw 'Retained backup is missing.'}}
$taskSummary=@{retained=@($taskRetained.name);removed=@($taskPlans.name);freedBytes=[long](($taskPlans | Measure-Object bytes -Sum).Sum);evidence=$taskEvidence}
$taskSummary | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $taskEvidence 'after.json') -Encoding UTF8
$taskSummary | ConvertTo-Json -Depth 5
