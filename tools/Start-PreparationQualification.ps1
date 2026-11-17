param([Parameter(Mandatory=$true)][string]$PreparedDirectory,[switch]$Start)
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskPrepared=(Resolve-Path -LiteralPath $PreparedDirectory).Path
$taskArtifacts=Join-Path $taskRoot 'artifacts'
if(-not $taskPrepared.StartsWith($taskArtifacts+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Prepared directory must be in checkout artifacts.'}
$taskExe='D:\Software\Adobe\Adobe After Effects 2023\Support Files\AfterFX.exe'
$taskScript=Join-Path $taskPrepared 'Run.jsx'
if(-not(Test-Path -LiteralPath $taskScript)){throw 'Prepare Run.jsx first.'}
Write-Output ('Start-Process -FilePath ''{0}'' -ArgumentList @(''-r'', ''"{1}"'') -WorkingDirectory ''{2}'' -WindowStyle Hidden -PassThru' -f $taskExe,$taskScript,$taskPrepared)
Write-Output 'Starts AE2023 and its native graph/WebView2 panel; refuses existing/recovered projects. Creates five temporary compositions, submits bounded 30k/50k graphs, renders15 single frames and saves only the checkout-local report/test project. No original project is opened. AE stays running.'
Write-Output 'Undo: close the test AE window normally. ADR0011 requires exact-action owner approval for -Start.'
if(-not $Start){Write-Output 'Read-only report. No AE process started.';exit 0}
if(@(Get-Process AfterFX,AfterFX_64 -ErrorAction SilentlyContinue).Count){throw 'AE already running; refusing to reuse its project.'}
foreach($taskName in @('result.json','qualification.aep','launch.json')){if(Test-Path -LiteralPath (Join-Path $taskPrepared $taskName)){throw 'Prior run evidence exists; use a fresh prepared directory.'}}
if(-not(Test-Path -LiteralPath $taskExe)){throw 'AE2023 executable unavailable.'}
$taskHashes=@(foreach($taskFile in Get-ChildItem (Join-Path $taskRoot 'dist') -File | Where-Object {$_.Extension -eq '.aex' -or $_.Name -eq 'StarfieldCore.dll'}){
    [ordered]@{name=$taskFile.Name;hash=(Get-FileHash -LiteralPath $taskFile.FullName -Algorithm SHA256).Hash}})
if($taskHashes.Count -ne 11){throw 'Incomplete installed development bundle.'}
$taskProcess=Start-Process -FilePath $taskExe -ArgumentList @('-r',('"'+$taskScript+'"')) -WorkingDirectory $taskPrepared -WindowStyle Hidden -PassThru
[ordered]@{pid=$taskProcess.Id;started=(Get-Date -Format o);script=$taskScript;files=$taskHashes;scriptHash=(Get-FileHash -LiteralPath $taskScript -Algorithm SHA256).Hash} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $taskPrepared 'launch.json') -Encoding UTF8
Write-Output "Dispatched AE PID $($taskProcess.Id); dispatch is not execution or qualification. Inspect result.json."
