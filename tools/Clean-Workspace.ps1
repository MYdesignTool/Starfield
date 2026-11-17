param([switch]$Clean)

# Workspace-only cleanup. Default is a report; installed dist, SDK inputs,
# rollback backups, prepared candidates, dumps and Git worktrees are excluded.
$ErrorActionPreference = 'Stop'
$taskRepo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskArtifacts = Join-Path $taskRepo 'artifacts'
$taskCandidates = [Collections.Generic.List[object]]::new()

function Assert-WorkspacePath([string]$taskPath) {
    $taskAbsolute = [IO.Path]::GetFullPath($taskPath)
    if (-not $taskAbsolute.StartsWith($taskArtifacts + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Cleanup target escaped artifacts: $taskAbsolute"
    }
    $taskCurrent = $taskAbsolute
    while ($taskCurrent -ne $taskRepo) {
        if (Test-Path -LiteralPath $taskCurrent) {
            if ((Get-Item -LiteralPath $taskCurrent -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Cleanup refuses a reparse point: $taskCurrent"
            }
        }
        $taskCurrent = Split-Path -Parent $taskCurrent
    }
}

function Get-SafeFiles([string]$taskPath) {
    Assert-WorkspacePath $taskPath
    foreach ($taskEntry in Get-ChildItem -LiteralPath $taskPath -Force) {
        if ($taskEntry.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "Cleanup refuses a linked descendant: $($taskEntry.FullName)"
        }
        if ($taskEntry.PSIsContainer) { Get-SafeFiles $taskEntry.FullName }
        else { $taskEntry }
    }
}

# Explicit regenerable directories, never a broad recursive artifacts delete.
foreach ($taskRelative in @('editor', 'plugin\2023\obj', 'core-dll\2023\obj', 'g05-compile')) {
    $taskPath = Join-Path $taskArtifacts $taskRelative
    if (Test-Path -LiteralPath $taskPath) {
        $taskFiles = @(Get-SafeFiles $taskPath)
        $taskCandidates.Add([pscustomobject]@{path=$taskPath;kind='directory';files=$taskFiles.Count;
            bytes=($taskFiles | Measure-Object Length -Sum).Sum})
    }
}

$taskTemporaryExtensions = @('.obj','.exe','.pdb','.lib','.exp','.ilk','.idb','.pch','.iobj','.ipdb')
foreach ($taskRelative in @('core-tests','adapter-tests','renderer-control-tests','node-effect-tests',
    'current-node-tests','gpu-tests','emission-timeline-tests','emission-cache-tests','native-sync-tests',
    'host-bootstrap-tests','gradient-editor-tests','particle-transform-tests','transform-transport-tests',
    'transform-graph-tests','transform-binding-tests','loader-tests','report-verification-20261007')) {
    $taskPath = Join-Path $taskArtifacts $taskRelative
    if (Test-Path -LiteralPath $taskPath) {
        foreach ($taskFile in Get-SafeFiles $taskPath) {
            if ($taskFile.Extension.ToLowerInvariant() -in $taskTemporaryExtensions) {
                $taskCandidates.Add([pscustomobject]@{path=$taskFile.FullName;kind='file';files=1;bytes=$taskFile.Length})
            }
        }
    }
}
foreach ($taskRelative in @('plugin\2023\x64\Release','core-dll\2023\x64\Release')) {
    $taskPath = Join-Path $taskArtifacts $taskRelative
    if (Test-Path -LiteralPath $taskPath) {
        foreach ($taskFile in Get-SafeFiles $taskPath) {
            if ($taskFile.Name -like 'StarfieldAppearance.*' -or $taskFile.Extension -in @('.exp','.lib','.ilk')) {
                $taskCandidates.Add([pscustomobject]@{path=$taskFile.FullName;kind='file';files=1;bytes=$taskFile.Length})
            }
        }
    }
}
$taskPlan = @($taskCandidates.ToArray())
$taskBytes = ($taskPlan | Measure-Object bytes -Sum).Sum
Write-Host ("{0} targets, {1:N2} MiB regenerable files." -f $taskPlan.Count, ($taskBytes / 1MB))
if (-not $Clean) {
    $taskPlan | Select-Object path,kind,files,@{Name='MiB';Expression={[math]::Round($_.bytes/1MB,2)}}
    return
}
if (Get-Process cl,link,MSBuild -ErrorAction SilentlyContinue) {
    throw 'A compiler/linker/MSBuild is running. Finish builds before cleaning their outputs.'
}
$taskRecordDirectory = Join-Path $taskArtifacts 'workspace-cleanup-20261007'
New-Item -ItemType Directory -Force -Path $taskRecordDirectory | Out-Null
$taskStamp = [DateTime]::UtcNow.ToString('yyyyMMddHHmmssfff')
$taskRecord = Join-Path $taskRecordDirectory ('removed-' + $taskStamp + '.json')
$taskPlan | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $taskRecordDirectory ('planned-' + $taskStamp + '.json')) -Encoding UTF8
$taskRemoved = [Collections.Generic.List[object]]::new()
$taskSkipped = [Collections.Generic.List[object]]::new()
function Remove-SafeFile($taskFile) {
    Assert-WorkspacePath $taskFile.FullName
    try {
        Remove-Item -LiteralPath $taskFile.FullName -Force -ErrorAction Stop
        $taskRemoved.Add([pscustomobject]@{path=$taskFile.FullName;bytes=$taskFile.Length})
    } catch {
        # The editor may own its browse database even when no build is running.
        # Keep locked files; cleanup must not stop any host/editor process.
        $taskSkipped.Add([pscustomobject]@{path=$taskFile.FullName;reason=$_.Exception.Message})
        Write-Host "Kept unavailable file: $($taskFile.FullName)"
    }
}
function Remove-EmptyDirectories([string]$taskDirectory) {
    Assert-WorkspacePath $taskDirectory
    foreach ($taskChild in Get-ChildItem -LiteralPath $taskDirectory -Directory -Force) {
        Assert-WorkspacePath $taskChild.FullName
        Remove-EmptyDirectories $taskChild.FullName
    }
    if (-not @(Get-ChildItem -LiteralPath $taskDirectory -Force).Count) {
        Remove-Item -LiteralPath $taskDirectory -Force
    }
}
foreach ($taskItem in $taskPlan) {
    Assert-WorkspacePath $taskItem.path
    if ($taskItem.kind -eq 'directory') {
        foreach ($taskFile in @(Get-SafeFiles $taskItem.path)) { Remove-SafeFile $taskFile }
        Remove-EmptyDirectories $taskItem.path
    } else {
        Remove-SafeFile (Get-Item -LiteralPath $taskItem.path -Force)
    }
}
[pscustomobject]@{removed=$taskRemoved.ToArray();skipped=$taskSkipped.ToArray()} |
    ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $taskRecord -Encoding UTF8
Write-Host ("Removed {0} files ({1:N2} MiB); kept {2} unavailable files. Inventory: {3}" -f
    $taskRemoved.Count,(($taskRemoved | Measure-Object bytes -Sum).Sum / 1MB),$taskSkipped.Count,$taskRecord)
