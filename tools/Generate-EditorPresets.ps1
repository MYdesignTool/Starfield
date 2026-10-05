param([switch]$Generate)
$ErrorActionPreference='Stop'
$taskRoot=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$catalog=Get-Content -LiteralPath (Join-Path $taskRoot 'schema\editor-presets.json') -Raw | ConvertFrom-Json
if($catalog.version -ne 1){throw 'Unsupported editor catalog version.'}
foreach($curve in $catalog.curves){
    if($curve.points.Count -lt 2 -or $curve.points.Count -gt 64 -or $curve.interpolation -lt 0 -or $curve.interpolation -gt 3){throw "Invalid curve: $($curve.name)"}
    $previous=-1.0
    foreach($point in $curve.points){
        if($point.Count -ne 2 -or $point[0] -le $previous -or $point[0] -lt 0 -or $point[0] -gt 1 -or $point[1] -lt 0 -or $point[1] -gt 100){throw "Invalid knot: $($curve.name)"}
        $previous=$point[0]
    }
    if($curve.points[0][0] -ne 0 -or $curve.points[-1][0] -ne 1){throw 'Curves require both endpoints.'}
}
foreach($gradient in $catalog.gradients){
    if($gradient.stops.Count -lt 2 -or $gradient.stops.Count -gt 8 -or $gradient.interpolation -lt 0 -or $gradient.interpolation -gt 1){throw "Invalid gradient: $($gradient.name)"}
    $previous=-1.0
    foreach($stop in $gradient.stops){
        if($stop.Count -ne 4 -or $stop[0] -le $previous -or $stop[0] -lt 0 -or $stop[0] -gt 1){throw 'Invalid gradient position.'}
        foreach($channel in $stop[1..3]){if($channel -lt 0 -or $channel -gt 1){throw 'Invalid gradient color.'}}
        $previous=$stop[0]
    }
}
Write-Output "Editor catalog: $($catalog.curves.Count) curves, $($catalog.gradients.Count) gradients."
if(-not $Generate){return}
$taskGenerated=Join-Path $taskRoot 'artifacts\generated'
New-Item -ItemType Directory -Path $taskGenerated -Force | Out-Null
$lines=[System.Collections.Generic.List[string]]::new()
$lines.Add('#pragma once')
$lines.Add('#include "starfield/core/AgeCurve.hpp"')
$lines.Add('#include "starfield/core/ColorGradient.hpp"')
$lines.Add('namespace starfield::adapter::editor_presets {')
$lines.Add('struct CurvePreset {const char* name; core::AgeCurve value;};')
$lines.Add('struct GradientPreset {const char* name; core::ColorGradient value;};')
function NumberText($number){([double]$number).ToString('R',[System.Globalization.CultureInfo]::InvariantCulture)}
$lines.Add('inline const CurvePreset curves[] = {')
foreach($curve in $catalog.curves){
    $lines.Add('{"'+$curve.name+'", []{core::AgeCurve c{};c.count='+$curve.points.Count+';c.interpolation=static_cast<core::CurveInterpolation>('+$curve.interpolation+');')
    for($i=0;$i -lt $curve.points.Count;$i++){$p=$curve.points[$i];$lines.Add("c.points[$i]={$(NumberText $p[0]),$(NumberText $p[1])};")}
    $lines.Add('return c;}()},')
}
$lines.Add('};')
$lines.Add('inline const GradientPreset gradients[] = {')
foreach($gradient in $catalog.gradients){
    $lines.Add('{"'+$gradient.name+'", []{core::ColorGradient c{};c.count='+$gradient.stops.Count+';c.interpolation=static_cast<core::ColorInterpolation>('+$gradient.interpolation+');')
    for($i=0;$i -lt $gradient.stops.Count;$i++){$p=$gradient.stops[$i];$lines.Add("c.stops[$i]={$(NumberText $p[0]),{$(NumberText $p[1]),$(NumberText $p[2]),$(NumberText $p[3])}};")}
    $lines.Add('return c;}()},')
}
$lines.Add('}; }')
[IO.File]::WriteAllLines((Join-Path $taskGenerated 'EditorPresetCatalog.hpp'),$lines,[Text.UTF8Encoding]::new($false))
$js='(function(root){"use strict";root.StarfieldEditorPresets='+($catalog | ConvertTo-Json -Depth 10 -Compress)+';}(typeof window!=="undefined"?window:globalThis));'
[IO.File]::WriteAllText((Join-Path $taskRoot 'cep_panel\js\editor_presets.js'),$js+"`n",[Text.UTF8Encoding]::new($false))
