"use strict";
const panelPath=require("./panel_test_paths.cjs");
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm");
const snapshots=require(panelPath("js/native_graph_snapshot.js"));
const codec=require(panelPath("js/graph_codec.js")),view=require(panelPath("js/graph_view.js"));
const global={};
const source=fs.readFileSync(panelPath("jsx/starfield_gateway.jsx"),"utf8")
    .replace("    function validateRendererManifest(record) {","    $.global.testValidateRenderer=validateRendererManifest;\n    function validateRendererManifest(record) {")
    .replace("    function writeRendererRecord(resolved, record) {","    $.global.testWriteRenderer=writeRendererRecord;\n    function writeRendererRecord(resolved, record) {")
    .replace("    function readRendererRecord(resolved) {","    $.global.testReadRenderer=readRendererRecord;\n    function readRendererRecord(resolved) {");
vm.runInNewContext(source,{$:{global},app:{}});
const values={"Max Particles":1000000,"Layout Output X":0,"Layout Output Y":0,
    "Time Remapping On / Off":0,"Time (Seconds)":0,"Preview":0,"Particle chance":100,"Time Sampling":1,"Acceleration":1};
const props=require("./node_property_fixture.js").mainControls();
for(const [name,value] of Object.entries(values)) {
    if(!props[name]) props[name]={name};
    props[name].value=value;
}
for(const p of new Set(Object.values(props))) Object.assign(p,{numKeys:0,setValue(v){this.value=v;}});
const resolved={target:{effect:{property:name=>props[name]||null},comp:{time:0}}};
for(const acceleration of [0,1])for(const hz of [30,60,120]) {
    const record={id:"000000000000000000000000000000ff",maxParticles:1000000,position:{x:0,y:0},acceleration,timeSamplingHz:hz};
    global.testValidateRenderer(record);global.testWriteRenderer(resolved,record);
    assert.equal(props.Acceleration.value,acceleration+1);
    const restored=global.testReadRenderer(resolved);
    assert.equal(restored.acceleration,acceleration);assert.equal(restored.timeSamplingHz,hz);
    const response=snapshots.normalize({ok:true,snapshot:{initialized:true,nativeNodes:[],renderer:restored}});
    assert.ok(response.ok);
    const graph=codec.fromHex(response.snapshot.graphHex);
    assert.equal(graph.nodes[0].parameters.find(p=>p.key==="6").value,acceleration);
    assert.equal(graph.nodes[0].parameters.find(p=>p.key==="7").value,hz);
    const projected=view.project(graph).nodes.find(n=>n.kind==="output");
    const preference=projected.params.find(p=>p.label==="Acceleration");
    assert.deepEqual(preference.choices,["GPU","CPU"]);assert.deepEqual(preference.enumValues,[0,1]);
    assert.equal(view.parameterToGraphValue(preference,1),0);assert.equal(view.parameterToGraphValue(preference,2),1);
    for(const name of ["Preview","Time Remapping On / Off"]) {
        const param=projected.params.find(p=>p.label===name);
        assert.equal(view.parameterToGraphValue(param,1),0);assert.equal(view.parameterToGraphValue(param,2),1);
    }
}
assert.throws(()=>global.testValidateRenderer({id:"000000000000000000000000000000ff",maxParticles:1000000,position:{x:0,y:0},acceleration:2}),/Acceleration/);
console.log("GPU preference and time sampling native/CEP round trips passed.");
