"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm");
const presets=require("../cep_panel/js/presets.js"),codec=require("../cep_panel/js/graph_codec.js"),layout=require("../cep_panel/js/graph_layout.js"),edits=require("../cep_panel/js/graph_edits.js"),view=require("../cep_panel/js/graph_view.js"),transactions=require("../cep_panel/js/graph_transactions.js");
const output="000000000000000000000000000000ff";
const built=presets.catalog.map(p=>presets.build(p.id,1080));
assert.equal(built.length,6);
for(const graph of built){assert.equal(presets.validate(graph),true);assert.ok(codec.toHex(graph).length/2<24*1024);assert.equal(graph.nodes.filter(n=>n.type===edits.types.output).length,1);assert.equal(graph.nodes.find(n=>n.type===edits.types.output).id,output);assert.ok(view.project(graph,{height:1080,width:1920,pixelAspect:1}));}
const base=built[0],before=codec.toHex(base);let next=100;
const factory=()=>String(next++).padStart(32,"0");
const add=presets.apply(base,{type:"applyPreset",presetId:"sparks",mode:"add",applyRenderSettings:false,layerHeightPixels:1080},factory);
assert.equal(codec.toHex(base),before,"adding never mutates the inspected base");assert.equal(add.nodes.length,base.nodes.length+3);assert.equal(new Set(add.nodes.map(n=>n.id)).size,add.nodes.length);assert.equal(add.nodes.filter(n=>n.type===edits.types.output).length,1);
for(const node of codec.fromHex(before).nodes)assert.deepEqual(add.nodes.find(n=>n.id===node.id),node,"existing node values and identities are retained");
assert.equal(add.edges.length,base.edges.length+3);assert.equal(new Set(add.edges.map(e=>e.id)).size,add.edges.length);assert.equal(presets.validate(add),true);
let alternate=codec.fromHex(before);alternate.nodes.find(n=>n.type===edits.types.output).parameters[0].value=12345;
const replace=presets.apply(alternate,{type:"applyPreset",presetId:"trails",mode:"replace",applyRenderSettings:false,layerHeightPixels:720},factory);
assert.equal(replace.nodes.find(n=>n.type===edits.types.output).parameters[0].value,12345);assert.ok(!replace.nodes.some(n=>base.nodes.some(old=>old.id===n.id && n.id!==output)));assert.equal(presets.validate(replace),true);
const settings=presets.apply(alternate,{type:"applyPreset",presetId:"sparks",mode:"replace",applyRenderSettings:true,layerHeightPixels:1080},factory);assert.equal(settings.nodes.find(n=>n.type===edits.types.output).parameters[0].value,1000000);
const envelope=presets.encode(add,"Original Setup","My Presets"),imported=presets.decode(envelope);assert.equal(imported.name,"Original Setup");assert.equal(codec.toHex(imported.graph),codec.toHex(add));
assert.throws(()=>presets.decode('{"format":"other"}'),/supported/);assert.throws(()=>presets.encode(base,""),/name/);
const unsafe=JSON.parse(envelope);unsafe.graphHex="00";assert.throws(()=>presets.decode(JSON.stringify(unsafe)));
const bad=codec.fromHex(before);bad.nodes[0].schemaVersion=6;assert.throws(()=>presets.validate(bad),/schema/);
const cycle=codec.fromHex(before);cycle.edges.push({id:factory(),sourceNode:cycle.nodes[2].id,sourcePort:"2",destinationNode:cycle.nodes[1].id,destinationPort:"1"});assert.throws(()=>presets.validate(cycle));
const dirty=codec.fromHex(before);dirty.optionalRecords.push("0280010008000000");assert.equal(presets.authoring(dirty).optionalRecords.filter(r=>r.startsWith("0280")).length,0,"export excludes derived animation bindings");
assert.equal(presets.validate(presets.apply(base,{type:"applyPreset",presetGraph:imported.graph,mode:"add",applyRenderSettings:false},factory)),true);
// Exercise the exact existing transaction and its float/color readback validation.
let hostGraph=codec.fromHex(before),revision=1,calls=[];
function snapshot(){return {ok:true,target:{token:"target"},snapshot:{initialized:true,revision:revision,recordStamp:"record-"+revision,graphHex:codec.toHex(hostGraph),geometry:{height:1080,width:1920,pixelAspect:1}}};}
const client=transactions.create({codec,edits:{apply:presets.apply},idFactory:factory,call:(operation,fields,callback)=>{
    calls.push(operation);if(operation==="getGraphSnapshot")return callback(snapshot());assert.equal(operation,"submitGraph");assert.equal(fields.baseGraphRevision,revision);assert.equal(fields.baseRecordStamp,"record-"+revision);assert.equal(fields.target.token,"target");hostGraph=codec.fromHex(fields.graphHex);
    for(const node of hostGraph.nodes)for(const p of node.parameters){if(node.type===edits.types.particle && p.key==="13"){p.value=view.encodeGradient(view.decodeGradient(p.value).map(s=>({position:Math.fround(s.position*100)/100,color:s.color.map(c=>Math.round(c*255)/255)})));}}
    revision++;callback(snapshot());
}});
let result;client.apply({type:"applyPreset",presetId:"sparks",mode:"add",applyRenderSettings:true},r=>result=r,"target",1);assert.equal(result.ok,true,result.error && result.error.message);assert.deepEqual(calls,["getGraphSnapshot","submitGraph"]);
calls=[];client.apply({type:"applyPreset",presetId:"sparks",mode:"replace"},r=>result=r,"target",1);assert.equal(result.ok,false);assert.equal(result.error.code,"stale_graph");assert.deepEqual(calls,["getGraphSnapshot"]);
// Test the real gateway file APIs with fake user-chosen files; imports are never eval'd.
const global={},state={file:null,writes:0,read:0,closed:0};
const fakeFile={exists:false,name:"My.sfldpreset",fsName:"user-chosen.sfldpreset",length:envelope.length,error:"",open(mode){this.mode=mode;return true;},read(){state.read++;return envelope;},write(text){state.writes++;assert.equal(text,envelope);return true;},close(){state.closed++;return true;}};
function File(){return fakeFile;}File.openDialog=File.saveDialog=()=>state.file;
vm.runInNewContext(fs.readFileSync(require.resolve("../cep_panel/jsx/starfield_gateway.jsx"),"utf8"),{$:{global},app:{},File,confirm:()=>true});
const request=(operation,fields={})=>JSON.stringify({protocol:"org.starfieldfx.panel",version:1,operation,...fields});
assert.equal(JSON.parse(global.SFLD_writePresetFile(request("writePresetFile",{text:envelope}))).cancelled,true);assert.equal(state.writes,0);
state.file=fakeFile;assert.equal(JSON.parse(global.SFLD_readPresetFile(request("readPresetFile"))).text,envelope);assert.equal(state.read,1);
assert.equal(JSON.parse(global.SFLD_writePresetFile(request("writePresetFile",{text:envelope}))).ok,true);assert.equal(state.writes,1);assert.equal(state.closed,2);
assert.equal(JSON.parse(global.SFLD_writePresetFile(request("writePresetFile",{text:'{"format":"invalid"}'}))).ok,false);assert.equal(state.writes,1);
fakeFile.length=999999;assert.equal(JSON.parse(global.SFLD_readPresetFile(request("readPresetFile"))).ok,false);assert.equal(state.read,1);
fs.writeFileSync("artifacts/preset-catalog.hex",built.map(g=>codec.toHex(g)).join("\n"));
console.log("Preset catalog, Add/Replace, UUIDs, settings, codec, atomic transaction and actual file gateway checks passed.");
