# Read-only deployment invariant: one record policy and one panel generation.
param([string]$PanelRoot)
$ErrorActionPreference='Stop'
$taskRepo=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
if(-not $PanelRoot){$PanelRoot=Join-Path $taskRepo 'web_panel'}
$taskPanel=(Resolve-Path -LiteralPath $PanelRoot).Path
$taskFactory=[IO.File]::ReadAllText((Join-Path $PSScriptRoot 'candidates/native_record_contract.js')).Replace("`r`n","`n").TrimEnd()
foreach($taskName in @('js/graph_edits.js','jsx/starfield_gateway.jsx')) {
    $taskText=[IO.File]::ReadAllText((Join-Path $taskPanel $taskName)).Replace("`r`n","`n")
    $taskMatches=[regex]::Matches($taskText,'(?s)// BEGIN STARFIELD NATIVE RECORD CONTRACT\n(.*?)\n    var nativeRecords=createNativeRecordContract\(\);\n    // END STARFIELD NATIVE RECORD CONTRACT')
    if($taskMatches.Count -ne 1 -or $taskMatches[0].Groups[1].Value -cne $taskFactory){throw "Panel record contract differs from its source: $taskName"}
}
[xml]$taskManifest=[IO.File]::ReadAllText((Join-Path $taskPanel 'CSXS/manifest.xml'))
$taskVersion=[string]$taskManifest.ExtensionManifest.ExtensionBundleVersion
if($taskVersion -notmatch '^0\.1\.0\.(\d+)$'){throw 'Unsupported panel generation format.'}
$taskGeneration=$Matches[1]
foreach($taskExtension in $taskManifest.ExtensionManifest.ExtensionList.Extension){
    if($taskExtension.Version -ne ('0.1.'+$taskGeneration)){throw 'Extension generation differs from bundle.'}
}
foreach($taskName in @('js/panel.js','js/preset_manager.js','jsx/starfield_gateway.jsx')) {
    $taskTokens=[regex]::Matches([IO.File]::ReadAllText((Join-Path $taskPanel $taskName)),'native-presets-(\d+)')
    if($taskTokens.Count -ne 1 -or $taskTokens[0].Groups[1].Value -ne $taskGeneration){throw "Gateway generation differs: $taskName"}
}
foreach($taskName in @('index.html','presets.html')) {
    $taskTokens=[regex]::Matches([IO.File]::ReadAllText((Join-Path $taskPanel $taskName)),'\?v=(\d+)')
    if(-not $taskTokens.Count){throw "Panel cache generation is missing: $taskName"}
    foreach($taskToken in $taskTokens){if($taskToken.Groups[1].Value -ne $taskGeneration){throw "Panel cache generation differs: $taskName"}}
}
Write-Host "Read-only panel invariant passed: CEP$taskGeneration; identical record factories, manifest, gateway and cache generations."
