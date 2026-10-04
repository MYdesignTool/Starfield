"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm");
const edits=require("../cep_panel/js/graph_edits.js"),view=require("../cep_panel/js/graph_view.js");
const global={};
const source=fs.readFileSync(require.resolve("../cep_panel/jsx/starfield_gateway.jsx"),"utf8")
    .replace("    function readNativeNode(effect, layer) {","    $.global.testRead=readNativeNode;\n    function readNativeNode(effect, layer) {")
    .replace("    function setNodeParameters(effect, node, layer) {","    $.global.testWrite=setNodeParameters;\n    function setNodeParameters(effect, node, layer) {")
    .replace("    function ensureNativeNodeEffects(layer, nodes, writeExisting, previousNodes) {",
        "    $.global.testEnsure=ensureNativeNodeEffects;\n    function ensureNativeNodeEffects(layer, nodes, writeExisting, previousNodes) {");
const context=vm.createContext({$:{global},app:{}});
vm.runInContext(source,context);
const layer={width:1920,height:1080,source:{pixelAspect:1}};
function effect(kind) {
    const values={"Panel Sync Guard":0,"Node Layout X":0,"Node Layout Y":0,"Outgoing Connection Count":0,
        "Auxiliary Source":0,"Shape":1,"Life (Seconds)":2,"Life Random":0,"Size (Pixels)":10,"Size Y (Pixels)":10,
        "Size Random":0,"Opacity":100,"Opacity Random":0,"Particle Color":1,"Color":[1,1,1,1],"Particle Feather":0,"Up Axis":3,
        "Size Over Life":100,"Opacity Over Life":100,"Size Curve Count":0,"Opacity Curve Count":0,"Color Gradient":2,
        "Orient To":1,"Angle X":0,"Angle Y":0,"Angle Z":0,"Angle Random":0,"Speed X":0,"Speed Y":0,"Speed Z":0,
        "Speed Random":0,"Limit to 2D":2,"Type":1,"Emitting":1,"Particles Per Second":100,"Origin XY":[960,540],"Origin Z":0,
        "Speed":100,"Size X":100,"Size Y":100,"Size Z":100,"Disc Size":.05,"Direction":2,"Direction Span":60,"Random Seed":1000,
        "Velocity X":0,"Velocity Y":0,"Velocity Z":0,"Particle Size":10,"Velocity Random":0,"Emit Chance":100,
        "Emit Life Start":0,"Emit Life End":100,"Inherit Velocity":0,"Inherit Size":0,"Inherit Opacity":0,"Inherit Color":0,
        "Gravity":0,"Gravity random":0,"Air Density":0,"Wind X":0,"Wind Y":0,"Wind Z":0,
        "Spin":0,"Spin Frequency":0,"Spin resist":0,"Spin Delay (Seconds)":0,"Wind and Spin Curve Count":0};
    for(let i=0;i<8;i++) {
        values["Node UUID "+i]=i===7?1:0;
        values[`Color Gradient ${i} Position`]=i===1?100:0;
        values[`Color Gradient ${i} Color`]=[1,1,1,1];
        for(const bank of ["Size","Opacity"]) {
            values[`${bank} Curve ${i} Age`]=i/7;values[`${bank} Curve ${i} Value`]=100;
        }
    }
    for(let slot=0;slot<4;slot++) for(let word=0;word<8;word++) {
        values[`Connection ${slot} Target UUID ${word}`]=0;
        values[`Connection ${slot} Edge UUID ${word}`]=0;
    }
    const props=Object.fromEntries(Object.entries(values).map(([name,value])=>[name,{name,value,numKeys:0,
        canSetExpression:false,writes:0,setValue(v){this.value=v;this.writes++;}}]));
    return {matchName:"org.starfieldfx.node."+kind,props,property:name=>props[name]||null};
}
const uuid=n=>n.toString(16).padStart(32,"0");let graph={version:1,nodes:[],edges:[],optionalRecords:[]};
const hosts=[],authoredNodes=[];
for(const kind of ["particle","emitter","auxiliary","force"]) {
    graph=edits.apply(graph,{type:"addNode",nodeType:kind},()=>uuid(graph.nodes.length+1));
    const authored=structuredClone(graph.nodes.at(-1)),host=effect(kind==="auxiliary"?"emitter":kind);
    authored.position={x:0,y:0};authored.outgoing=[];
    if(kind==="particle") {
        const changes={"14":50,"15":1,"16":30,"17":2,"18":[30,60,90],"19":25,"20":[40,50,60],"21":75,"22":0,"23":40,"24":1,"12":1};
        for(const p of authored.parameters) if(p.key in changes) p.value=changes[p.key];
        authored.parameters.find(p=>p.key==="13").value=Array.from(view.encodeGradient([
            {position:0,color:[1,0,0]},{position:.3,color:[0,1,0]},{position:1,color:[0,0,1]}]));
    }
    context.payload=JSON.stringify(authored);
    global.testWrite(host,vm.runInContext("JSON.parse(payload)",context),layer);
    const restored=global.testRead(host,layer);
    assert.equal(restored.schemaVersion,kind==="particle"?5:kind==="force"?2:6);
    for(const p of authored.parameters) {
        const actual=restored.parameters.find(v=>v.key===p.key);
        assert.ok(actual,`${kind} key ${p.key} round trips`);
        const expected=Array.isArray(p.value)||ArrayBuffer.isView(p.value)?Array.from(p.value):p.value;
        const value=Array.isArray(actual.value)?Array.from(actual.value,v=>v===0?0:v):actual.value===0?0:actual.value;
        assert.deepEqual(value,expected,`${kind} key ${p.key} retains its own value`);
    }
    if(kind==="particle") {
        assert.equal(host.props["Color Gradient 1 Position"].value,30);
        assert.equal(host.props["Angle Y"].value,60);
        assert.equal(host.props["Speed Y"].value,50);
    } else if(kind!=="force") {
        assert.equal(host.props["Emitting"].value,1);
        assert.equal(host.props["Auxiliary Source"].value,kind==="auxiliary"?1:0);
    }
    assert.equal(host.props["Panel Sync Guard"].value,0);
    for(let word=0;word<8;word++) host.props["Node UUID "+word].value=parseInt(authored.id.slice(word*4,word*4+4),16);
    hosts.push(host);authoredNodes.push(authored);
}
const parade={numProperties:hosts.length,property:i=>hosts[i-1]};
layer.property=name=>name==="ADBE Effect Parade"?parade:null;
const inRealm=value=>{context.payload=JSON.stringify(value);return vm.runInContext("JSON.parse(payload)",context);};
const before=structuredClone(authoredNodes),connected=structuredClone(before);
connected[0].outgoing=[{id:uuid(90),target:connected[3].id}];
connected[3].position={x:100,y:200};
const publicWrites=()=>hosts.map(host=>Object.fromEntries(Object.entries(host.props)
    .filter(([name])=>!/^(Node |Connection |Outgoing )/.test(name)).map(([name,p])=>[name,p.writes])));
const untouched=publicWrites();
global.testEnsure(layer,inRealm(connected),true,inRealm(before));
assert.deepEqual(publicWrites(),untouched,"Particle -> Force and layout edits never rewrite authored controls");
assert.equal(hosts[0].props["Outgoing Connection Count"].value,1);
assert.equal(hosts[3].props["Node Layout X"].value,100);
global.testEnsure(layer,inRealm(before),true,inRealm(connected));
assert.deepEqual(publicWrites(),untouched,"rollback only restores changed connection/layout records");
assert.equal(hosts[0].props["Outgoing Connection Count"].value,0);
assert.equal(hosts[3].props["Node Layout X"].value,0);
assert.throws(()=>global.testWrite(hosts[0],inRealm(authoredNodes[3]),layer),/type mismatch/);
console.log("Reference Particle/Emitter/Force round-trip and topology-only commit/rollback checks passed.");
