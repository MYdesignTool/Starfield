# Restore the AE 2023 plug-in pair backed up for the P-02D candidate.
# Default mode reports exact paths and changes nothing.
param(
    [Parameter(Mandatory = $true)][string]$PluginDir,
    [string]$BackupDir = '',
    [switch]$Rollback
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$pluginPath = (Resolve-Path -LiteralPath $PluginDir).Path
$runtimePath = Join-Path $repositoryRoot 'artifacts\runtime'
if (-not $BackupDir) {
    $BackupDir = Join-Path $repositoryRoot 'artifacts\disabled\p02d-node-sync-20260930'
} elseif (-not [IO.Path]::IsPathRooted($BackupDir)) {
    $BackupDir = Join-Path $repositoryRoot $BackupDir
}
$backupPath = (Resolve-Path -LiteralPath $BackupDir).Path
$backupRoot = [IO.Path]::GetFullPath((Join-Path $repositoryRoot 'artifacts\disabled')).TrimEnd('\') + '\'
if (-not $backupPath.StartsWith($backupRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing backup outside artifacts/disabled: $backupPath"
}
$mainName = 'StarfieldParticle.aex'
$nodeNames = @('StarfieldEmitter.aex', 'StarfieldParticleNode.aex',
               'StarfieldAppearance.aex', 'StarfieldForce.aex')
$targetMain = Join-Path $pluginPath $mainName
$targetManifest = Join-Path $runtimePath 'current.txt'
$priorMain = Join-Path $backupPath $mainName
$priorManifest = Join-Path $backupPath 'current.txt'
$candidateHash = 'CF6E08544CE81495932DDA3CD0B240C29A899E5EC435303D3170FA48555F616D'
$priorHash = 'B646EF14937700FB31CBA8A957076899231A4D472C9EF6A1DAB460FBB4426560'
$priorCoreName = 'StarfieldCore-DEF5B7804EBEB665.dll'
$candidateCoreName = 'StarfieldCore-55B877A8F66D3576.dll'

$link = Get-Item -LiteralPath (Join-Path $pluginPath 'StarfieldRuntime') -Force
if ($link.LinkType -ne 'Junction' -or
    [IO.Path]::GetFullPath([string]$link.Target).TrimEnd('\') -ne
    [IO.Path]::GetFullPath($runtimePath).TrimEnd('\')) {
    throw 'StarfieldRuntime is not the expected junction to this checkout.'
}
if (-not (Test-Path -LiteralPath $priorMain) -or -not (Test-Path -LiteralPath $priorManifest)) {
    throw 'The backed-up main AEX or runtime selector is missing.'
}

Write-Host "Plug-in directory: $pluginPath"
Write-Host "Candidate AEX: $targetMain"
Write-Host "Candidate Core selection: $targetManifest ($candidateCoreName)"
Write-Host "Restore AEX from: $priorMain"
Write-Host "Restore Core selection from: $priorManifest"
if (-not $Rollback) {
    Write-Host 'Read-only report. Pass -Rollback to restore the backed-up AE 2023 candidate.'
    exit 0
}

if (Get-Process -Name AfterFX, AfterFX_64 -ErrorAction SilentlyContinue) {
    throw 'Close After Effects before changing its loaded plug-in files.'
}
if ((Get-FileHash -LiteralPath $targetMain -Algorithm SHA256).Hash -ne $candidateHash) {
    throw 'The installed main AEX is not the P-02D candidate; refusing to overwrite a newer version.'
}
if ((Get-FileHash -LiteralPath $priorMain -Algorithm SHA256).Hash -ne $priorHash) {
    throw 'The backed-up main AEX hash does not match the recorded prior installation.'
}
if ((Get-Content -LiteralPath $priorManifest -Raw).Trim() -ne $priorCoreName) {
    throw 'The backed-up Core selector does not match the recorded prior installation.'
}
$candidateArchive = Join-Path $backupPath 'candidate-StarfieldParticle.aex'
if (Test-Path -LiteralPath $candidateArchive) {
    throw "Candidate archive already exists: $candidateArchive"
}

Move-Item -LiteralPath $targetMain -Destination $candidateArchive
foreach ($name in $nodeNames) {
    $nodePath = Join-Path $pluginPath $name
    if (Test-Path -LiteralPath $nodePath) {
        Move-Item -LiteralPath $nodePath -Destination (Join-Path $backupPath $name)
    }
}
Copy-Item -LiteralPath $priorMain -Destination $targetMain
$manifestTemp = Join-Path $runtimePath 'current.rollback.tmp'
Copy-Item -LiteralPath $priorManifest -Destination $manifestTemp -Force
Move-Item -LiteralPath $manifestTemp -Destination $targetManifest -Force

if ((Get-FileHash -LiteralPath $targetMain -Algorithm SHA256).Hash -ne $priorHash -or
    (Get-Content -LiteralPath $targetManifest -Raw).Trim() -ne $priorCoreName) {
    throw 'Rollback verification failed; preserve the backup folder and inspect the restored files.'
}
Write-Host 'Prior main AEX and Core selection restored. Candidate files were retained in the backup folder.' -ForegroundColor Green
