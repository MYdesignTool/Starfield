param([ValidateSet('All','Panel','Cpp','Scripts')][string]$Group = 'All')

# All current test scopes; no builds/deployment through the live dist Junction.
$ErrorActionPreference = 'Stop'
$taskRepo = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskOutput = Join-Path $taskRepo 'artifacts\all-tests'
New-Item -ItemType Directory -Force -Path $taskOutput | Out-Null
$taskResults = [Collections.Generic.List[object]]::new()

function Invoke-Test([string]$taskName, [string]$taskCommand, [string[]]$taskArguments) {
    $taskLog = Join-Path $taskOutput ($taskName + '.log')
    $taskPriorPreference = $ErrorActionPreference
    try {
        # Windows PowerShell converts native stderr into ErrorRecords. A failed
        # suite must be recorded, not terminate collection of the other suites.
        $ErrorActionPreference = 'Continue'
        & $taskCommand @taskArguments *> $taskLog
        $taskExitCode = $LASTEXITCODE
    } finally { $ErrorActionPreference = $taskPriorPreference }
    $taskResults.Add([pscustomobject]@{suite=$taskName;passed=($taskExitCode -eq 0);exitCode=$taskExitCode;
        log=('artifacts/all-tests/' + $taskName + '.log')})
    $taskLabel = if ($taskExitCode -eq 0) { 'PASS' } else { 'FAIL' }
    Write-Host "$taskLabel $taskName"
    $taskResults.ToArray() | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $taskOutput 'results.json') -Encoding UTF8
}

Push-Location -LiteralPath $taskRepo
try {
    if ($Group -in @('All','Panel')) {
        Invoke-Test 'panel' 'node' @('tests/RunPanelTests.cjs')
    }
    if ($Group -in @('All','Cpp')) {
        foreach ($taskScope in @('', 'Adapter', 'RendererControls', 'CurrentNodes', 'Gpu',
            'EmissionTimeline', 'EmissionCache', 'NativeSync', 'HostBootstrap', 'GradientEditor',
            'ParticleTransform', 'TransformTransport', 'TransformGraph', 'TransformBinding')) {
            $taskArgs = @('-NoProfile','-ExecutionPolicy','Bypass','-File','tests/RunCoreTests.ps1')
            if ($taskScope) { $taskArgs += "-$taskScope" }
            Invoke-Test $(if($taskScope){$taskScope}else{'Core'}) 'powershell' $taskArgs
        }
        foreach ($taskKind in @('Emitter','Particle','Force','Transform')) {
            Invoke-Test "NodeEffects-$taskKind" 'powershell' @('-NoProfile','-ExecutionPolicy','Bypass',
                '-File','tests/RunCoreTests.ps1','-NodeEffects','-NodeKind',$taskKind)
        }
    }
    if ($Group -in @('All','Scripts')) {
        foreach ($taskFile in Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*_tests.ps1' | Sort-Object Name) {
            Invoke-Test $taskFile.BaseName 'powershell' @('-NoProfile','-ExecutionPolicy','Bypass','-File',$taskFile.FullName)
        }
    }
} finally { Pop-Location }
$taskFailures = @($taskResults.ToArray() | Where-Object { -not $_.passed }).Count
Write-Host "$($taskResults.Count) scopes, $taskFailures failures. Logs: artifacts/all-tests/"
if ($taskFailures) { exit 1 }
