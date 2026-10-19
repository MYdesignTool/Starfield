"use strict";
// P-02M retirement boundary; saved gradient/curve codecs are covered separately
// by particle_gradient_tests and graph/preset/native-gateway regression suites.
const panelPath=require("./panel_test_paths.cjs");
const fs=require("node:fs"),assert=require("node:assert/strict");
const source=fs.readFileSync(panelPath("js/panel.js"),"utf8");
const html=fs.readFileSync(panelPath("index.html"),"utf8");
const css=fs.readFileSync(panelPath("css/panel.css"),"utf8");
assert.ok(!/inspector|numericScrub|curveDrag|function onEdit\(|function applyChanges\(/i.test(source),
    "retired editor state and callbacks must not return");
assert.ok(!/call\("setParameters"|type:\s*"setParameters"/.test(source),
    "the graph panel cannot initiate parameter-edit requests");
assert.ok(!/inspector|parameter-grid|age-curve|gradient-editor/.test(html+css));
for(const name of ["curve_editor","gradient_editor","editor_presets"]){
    assert.ok(!html.includes("js/"+name+".js"));
    assert.ok(!fs.existsSync(panelPath("js/"+name+".js")),"unused editor script is physically retired: "+name);
}
const generator=fs.readFileSync(require("node:path").join(__dirname,"../tools/Generate-EditorPresets.ps1"),"utf8");
assert.ok(!generator.includes("cep_panel\\js\\editor_presets.js"),"native generation cannot resurrect the CEP script");
assert.ok(generator.includes("EditorPresetCatalog.hpp"),"native AE editor presets still have their build input");
console.log("CEP editor retirement checks passed: no DOM/styles/handlers/value-edit requests/helper files or regenerating output; native presets preserved.");
