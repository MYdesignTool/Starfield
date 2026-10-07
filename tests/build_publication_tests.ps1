# Focused read-only hash guard tests. No build, deployment, process or host action.
$ErrorActionPreference = 'Stop'
$taskRepo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskParseErrors = $null
$taskAst = [Management.Automation.Language.Parser]::ParseFile((Join-Path $taskRepo 'ae_plugin\BuildWindows.ps1'),[ref]$null,[ref]$taskParseErrors)
if ($taskParseErrors.Count) { throw "BuildWindows parse error: $($taskParseErrors[0].Message)" }
$taskDefinition = $taskAst.Find({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Assert-RuntimeNativePair'},$true)
if (-not $taskDefinition) { throw 'Missing publication guard.' }
. ([scriptblock]::Create($taskDefinition.Extent.Text))
$taskChecks = 0
function Check([bool]$Value,[string]$Message) { $script:taskChecks++; if (-not $Value) { throw $Message } }
function Rejected([string]$Root) {
    $taskRejected = $false
    try { Assert-RuntimeNativePair $Root '2023' 'x64' 'Release' } catch { $taskRejected = $_.Exception.Message -like 'Runtime publication requires*' }
    Check $taskRejected 'Unpaired publication accepted.'
}
$taskScratch = Join-Path $taskRepo "artifacts\transform-publication-tests\$([Guid]::NewGuid().ToString('N'))"
$taskBuilt = Join-Path $taskScratch 'artifacts\plugin\2023\x64\Release'
$taskInstalled = Join-Path $taskScratch 'dist'
New-Item -ItemType Directory -Force -Path $taskBuilt,$taskInstalled | Out-Null
$taskNames = @('StarfieldParticle','StarfieldEmitter','StarfieldParticleNode','StarfieldForce','StarfieldTransform','StarfieldHost')
Rejected $taskScratch
foreach ($taskName in $taskNames) {
    [IO.File]::WriteAllText((Join-Path $taskBuilt "$taskName.aex"),"paired-$taskName")
    [IO.File]::WriteAllText((Join-Path $taskInstalled "$taskName.aex"),"paired-$taskName")
}
Assert-RuntimeNativePair $taskScratch '2023' 'x64' 'Release'
Check $true 'Matching pair accepted.'
foreach ($taskName in $taskNames) {
    [IO.File]::WriteAllText((Join-Path $taskInstalled "$taskName.aex"),"old-$taskName")
    Rejected $taskScratch
    [IO.File]::WriteAllText((Join-Path $taskInstalled "$taskName.aex"),"paired-$taskName")
}
# Missing files on either side reject without touching any remaining files.
Move-Item -LiteralPath (Join-Path $taskBuilt 'StarfieldParticle.aex') -Destination (Join-Path $taskBuilt 'StarfieldParticle.saved')
Rejected $taskScratch
Move-Item -LiteralPath (Join-Path $taskBuilt 'StarfieldParticle.saved') -Destination (Join-Path $taskBuilt 'StarfieldParticle.aex')
Move-Item -LiteralPath (Join-Path $taskInstalled 'StarfieldHost.aex') -Destination (Join-Path $taskInstalled 'StarfieldHost.saved')
Rejected $taskScratch
Move-Item -LiteralPath (Join-Path $taskInstalled 'StarfieldHost.saved') -Destination (Join-Path $taskInstalled 'StarfieldHost.aex')
$taskBefore = @(Get-ChildItem -LiteralPath $taskScratch -File -Recurse | ForEach-Object { "$($_.FullName) $((Get-FileHash -LiteralPath $_.FullName).Hash)" })
Assert-RuntimeNativePair $taskScratch '2023' 'x64' 'Release'
$taskAfter = @(Get-ChildItem -LiteralPath $taskScratch -File -Recurse | ForEach-Object { "$($_.FullName) $((Get-FileHash -LiteralPath $_.FullName).Hash)" })
Check (($taskBefore -join "`n") -eq ($taskAfter -join "`n")) 'Guard mutated fixture files.'
$taskScript = [IO.File]::ReadAllText((Join-Path $taskRepo 'ae_plugin\BuildWindows.ps1'))
$taskGuardAt = $taskScript.IndexOf('Assert-RuntimeNativePair $repositoryRoot')
$taskRuntimeAt = $taskScript.IndexOf('$runtimeDir =')
Check ($taskGuardAt -ge 0 -and $taskGuardAt -lt $taskRuntimeAt) 'Guard must precede every runtime mutation.'
Write-Output "Build publication: $taskChecks checks passed; scratch files remain under artifacts."
