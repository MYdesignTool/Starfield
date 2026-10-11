param([switch]$Run,[ValidatePattern('^[a-z0-9-]+$')][string]$PreparedName='model-acceptance-panel71',
    [string]$AfterEffectsPath='D:\Software\Adobe\Adobe After Effects 2023\Support Files\AfterFX.exe')
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$taskWrapper=Join-Path $taskRepo ('artifacts/'+$PreparedName+'/Run.jsx')
if(-not $Run){Write-Output "Report only. Authorized -Run starts '$AfterEffectsPath' -r '$taskWrapper' with a hidden window. It creates/deletes only temporary test project items, briefly suppresses script dialogs, never saves the project, writes results under artifacts and leaves AE running. ADR0011 requires explicit authorization for this process start.";exit 0}
if(Get-Process AfterFX,AfterFX_64 -ErrorAction SilentlyContinue){throw 'AE must be closed before launching automated acceptance.'}
if(-not (Test-Path -LiteralPath $AfterEffectsPath) -or -not (Test-Path -LiteralPath $taskWrapper)){throw 'Missing AE executable or prepared acceptance wrapper.'}
$taskExpected='native-presets-71'
$taskInstalled=[IO.File]::ReadAllText((Join-Path $taskRepo 'cep_panel/jsx/starfield_gateway.jsx'))
if(-not $taskInstalled.Contains('var GATEWAY_BUILD = "'+$taskExpected+'"')){throw 'Install the checked CEP71 pair before running acceptance.'}
if(Test-Path -LiteralPath (Join-Path $taskRepo ('artifacts/'+$PreparedName+'/result.json'))){throw 'A result already exists; prepare a fresh wrapper directory to retain evidence.'}
$taskProcess=Start-Process -FilePath $AfterEffectsPath -ArgumentList @('-r',('"'+$taskWrapper+'"')) -WindowStyle Hidden -PassThru
Write-Output "AE acceptance started: PID $($taskProcess.Id). Result: artifacts/$PreparedName/result.json. AE will be left running."
