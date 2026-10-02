"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm");
const view=require("../cep_panel/js/graph_view.js"),edits=require("../cep_panel/js/graph_edits.js");
const stops=[{position:0,color:[1,0,0]},{position:.3,color:[0,1,0]},{position:1,color:[0,0,1]}];
const bytes=view.encodeGradient(stops);
assert.deepEqual(view.decodeGradient(bytes),stops);
assert.throws(()=>view.encodeGradient([{position:0,color:[1,1,1]},{position:0,color:[1,1,1]}]),/Unordered/);
assert.throws(()=>view.decodeGradient(bytes.slice(0,-1)),/Malformed/);
const hdr=view.encodeGradient([{position:0,color:[3,0,0]},{position:1,color:[0,2,0]}]);
assert.equal(view.decodeGradient(hdr)[0].color[0],3,"gradient codec preserves float colors");
const global={};
const source=fs.readFileSync(require.resolve("../cep_panel/jsx/starfield_gateway.jsx"),"utf8")
    .replace("    function writeNodeColorGradient(effect, bytes) {",
        "    $.global.testGradientWrite=writeNodeColorGradient;\n    function writeNodeColorGradient(effect, bytes) {");
vm.runInNewContext(source,{$:{global},app:{}});
const values=new Map(),properties=[];
for(let i=0;i<8;i++) for(const suffix of ["Position","Color"])
    properties.push({name:`Color Gradient ${i} ${suffix}`,value:suffix==="Color"?[1,1,1,1]:0,numKeys:0,canSetExpression:false,
        get expressionEnabled(){throw new Error("constant bank must not query expressions");},
        setValue(value){this.value=value;values.set(this.name,value);}});
properties.push({name:"Color Gradient Count",value:2,numKeys:0,canSetExpression:false,setValue(value){values.set(this.name,value);}});
const effect={matchName:"org.starfieldfx.node.particle",numProperties:properties.length,
    property(index){return typeof index==="number"?properties[index-1]:properties.find(p=>p.name===index);}};
global.testGradientWrite(effect,Array.from(bytes));
assert.equal(values.get("Color Gradient Count"),3);
assert.equal(values.get("Color Gradient 1 Position"),.3);
assert.deepEqual(Array.from(values.get("Color Gradient 2 Color")),[0,0,1,1]);
const count=values.size,bad=Array.from(bytes);bad[1]=8;
assert.throws(()=>global.testGradientWrite(effect,bad),/Invalid/);
assert.equal(values.size,count,"malformed gradient never partially writes native banks");
const nodeId="00000000000000000000000000000001",copyId="00000000000000000000000000000002";
let graph=edits.apply({nodes:[],edges:[],optionalRecords:[]},{type:"addNode",nodeType:"particle"},()=>nodeId);
assert.equal(graph.nodes[0].schemaVersion,3);
assert.equal(graph.nodes[0].parameters.find(p=>p.key==="12").value,0);
assert.equal(view.decodeGradient(graph.nodes[0].parameters.find(p=>p.key==="13").value).length,2);
graph=edits.apply(graph,{type:"setParameters",changes:[{nodeId,parameterKey:"13",valueType:7,value:bytes}]});
graph=edits.apply(graph,{type:"duplicateNodes",nodeIds:[nodeId]},()=>copyId);
assert.equal(graph.nodes.length,2);
assert.deepEqual(view.decodeGradient(graph.nodes[1].parameters.find(p=>p.key==="13").value),stops);
assert.notEqual(graph.nodes[0].parameters.find(p=>p.key==="13").value,graph.nodes[1].parameters.find(p=>p.key==="13").value);
console.log("Particle gradient codec and actual JSX native-bank checks passed.");
