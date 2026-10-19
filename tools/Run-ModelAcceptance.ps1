param([switch]$Run,[switch]$UseRunningInstance,[ValidatePattern('^[a-z0-9-]+$')][string]$PreparedName='model-acceptance-panel71',
    [string]$AfterEffectsPath='D:\Software\Adobe\Adobe After Effects 2023\Support Files\AfterFX.exe')
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskWrapper=Join-Path $taskRepo ('artifacts/'+$PreparedName+'/Run.jsx')
if(-not $Run){Write-Output "Report only. Authorized -Run starts '$AfterEffectsPath' -r '$taskWrapper' with a hidden window. -UseRunningInstance dispatches to the single existing AE instance; it does not close or save its project. It creates/deletes only temporary test project items, briefly suppresses script dialogs, never saves the project, writes results under artifacts and leaves AE running. ADR0011 requires explicit authorization for this process start.";exit 0}
$taskExisting=@(Get-Process AfterFX,AfterFX_64 -ErrorAction SilentlyContinue)
if($UseRunningInstance){if($taskExisting.Count -ne 1){throw 'UseRunningInstance requires exactly one existing AE process.'}}
elseif($taskExisting.Count){throw 'AE must be closed before launching automated acceptance.'}
if(-not (Test-Path -LiteralPath $AfterEffectsPath) -or -not (Test-Path -LiteralPath $taskWrapper)){throw 'Missing AE executable or prepared acceptance wrapper.'}
. (Join-Path $PSScriptRoot 'ModelAcceptanceContract.ps1')
$taskExpected=Get-StarfieldAcceptanceBuild (Split-Path -Parent $taskWrapper) (Join-Path $taskRepo 'web_panel/jsx/starfield_gateway.jsx')
if(Test-Path -LiteralPath (Join-Path $taskRepo ('artifacts/'+$PreparedName+'/result.json'))){throw 'A result already exists; prepare a fresh wrapper directory to retain evidence.'}
$taskDirectory=Split-Path -Parent $taskWrapper
$taskProcess=Start-Process -FilePath $AfterEffectsPath -ArgumentList @('-r',('"'+$taskWrapper+'"')) -WorkingDirectory $taskDirectory -WindowStyle Hidden -PassThru
Write-Output "AE acceptance dispatch started: PID $($taskProcess.Id), existing AE IDs $($taskExisting.Id -join ','). Entry: artifacts/$PreparedName/entry.json. Result: artifacts/$PreparedName/result.json. Dispatch alone does not confirm script execution. AE will be left running."
