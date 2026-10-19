"use strict";
const panelPath=require("./panel_test_paths.cjs");
// Real gateway against controls inventoried from PF_Cmd_PARAMS_SETUP; no AE.
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const root=path.resolve(__dirname,".."),edits=require(panelPath("js/graph_edits")),view=require(panelPath("js/graph_view"));
const catalogPath=process.argv[2] || path.join(root,"artifacts/m3-20-followup-20261013/m3-17-full-audit-catalog-motion.log");
const raw=fs.readFileSync(catalogPath),text=raw.toString(raw[0]===255&&raw[1]===254?"utf16le":"utf8").replace(/^\uFEFF/,"");
const line=text.split(/\r?\n/).find(s=>s.startsWith("NODE_CONTROL_CATALOG:"));assert.ok(line,"actual Motion catalog required");
const catalog=JSON.parse(line.slice("NODE_CONTROL_CATALOG:".length));assert.equal(catalog.kind,"motion");
const host={};let source=fs.readFileSync(process.env.STARFIELD_GATEWAY_SOURCE || panelPath("jsx/starfield_gateway.jsx"),"utf8")
    .replace("    function readNativeNode(effect, layer) {","    $.global.testRead=readNativeNode;\n    function readNativeNode(effect, layer) {")
    .replace("    function setNodeParameters(effect, node, layer) {","    $.global.testWrite=setNodeParameters;\n    function setNodeParameters(effect, node, layer) {");
if(process.env.STARFIELD_EXTENDSCRIPT_CONDITIONAL_MODEL==="1")source=require("./extendscript_conditionals.cjs").modelLeftAssociation(source);
const context=vm.createContext({$:{global:host},app:{}});vm.runInContext(source,context);
const controls=catalog.controls.map(c=>({name:c.name,matchName:"org.starfieldfx.node.motion-"+c.disk,
    propertyIndex:c.index,numKeys:0,canSetExpression:false,value:structuredClone(c.value),
    valueAtTime(time,preExpression){assert.equal(time,0);assert.equal(preExpression,true);return structuredClone(this.value);},
    setValue(value){if(c.minimum!==undefined)assert.ok(Number.isInteger(value)&&value>=c.minimum&&value<=c.maximum,c.name+" range");this.value=structuredClone(value);}}));
const byName=Object.fromEntries(controls.map(c=>[c.name,c])),byMatch=Object.fromEntries(controls.map(c=>[c.matchName,c]));
const snapshotProperties=process.env.STARFIELD_MOTION_SNAPSHOT_PROPERTIES==="1";
const effect={matchName:"org.starfieldfx.node.motion",name:"Motion fixture",numProperties:controls.length,
    property(key){const p=typeof key==="number"?controls[key-1]:byName[key]||byMatch[key]||null;
        if(!p || !snapshotProperties || p.name!=="Motion Type")return p;
        const captured=p.value;return {...p,get value(){return captured;},valueAtTime(){return captured;}};
    }};
const layer={width:1920,height:1080,source:{pixelAspect:1},property(name){
    return name==="ADBE Effect Parade"?{numProperties:1,property(index){return index===1?effect:null;}}:null;
}},uuid="000000000000000000000000000000aa";
for(let i=0;i<8;i++)byName["Node UUID "+i].value=parseInt(uuid.slice(i*4,i*4+4),16);
let graph=edits.apply({version:1,nodes:[],edges:[],optionalRecords:[]},{type:"addNode",nodeType:"motion"},()=>uuid),checks=0;
const realm=node=>{context.payload=JSON.stringify(node);return vm.runInContext("JSON.parse(payload)",context);};
function author(){const node=structuredClone(graph.nodes[0]);node.position={x:0,y:0};node.outgoing=[];
    node.parameters.forEach(p=>{if(ArrayBuffer.isView(p.value))p.value=Array.from(p.value);});
    const setter=byName["Motion Type"].setValue;
    byName["Motion Type"].setValue=()=>{throw Error("Script must not compete with the native selector writer");};
    try{host.testWrite(effect,realm(node),layer);}finally{byName["Motion Type"].setValue=setter;}
    const intent=byName["Panel Sync Guard"].value;
    assert.equal(intent,4+value(node,1),"script leaves verified numeric author intent");
    if(snapshotProperties)host.testRead(effect,layer); // old Property value retained by baseline gateway
    // Simulate the existing native88 AEGP owner, not a scripting popup setter.
    byName["Motion Type"].value=intent===5?1:intent===4?2:3;
    return host.testRead(effect,layer);}
function eq(a,b,message){checks++;assert.equal(a,b,message);}
function change(key,value,type=4){graph=edits.apply(graph,{type:"setParameters",changes:[{nodeId:uuid,parameterKey:String(key),valueType:type,value}]});}
function value(node,key){return node.parameters.find(p=>p.key===String(key)).value;}
// An AE last-used/default value must be replaced by the submitted Circle mode.
byName["Motion Type"].value=2;
let restored=author();eq(value(restored,1),1,"new Circle stays Core1");eq(byName["Motion Type"].value,1,"Circle writes native1");
if(snapshotProperties){console.log("Motion cached-Property regression: old wrapper2, fresh host1; Circle remains Core1.");process.exit(0);}
// The first getter can be a stale last-used/default delivery. A new Motion
// still explicitly writes its selector instead of treating the getter as saved.
const modeControl=byName["Motion Type"],modeSetter=modeControl.setValue;
let actualMode=3,maskedGetter=true,modeWrites=0;
Object.defineProperty(modeControl,"value",{configurable:true,get(){if(maskedGetter){maskedGetter=false;return 1;}return actualMode;},set(v){actualMode=v;}});
modeControl.setValue=function(v){modeWrites++;modeSetter.call(this,v);};
restored=author();eq(actualMode,1,"native Circle intent overrides stale/default getter and live Look At");eq(modeWrites,0,"no competing script selector write per batch");
Object.defineProperty(modeControl,"value",{configurable:true,writable:true,value:actualMode});modeControl.setValue=modeSetter;
change(4,Math.PI);restored=author();eq(byName.Speed.value,50,"Circle Speed50 is authored");eq(value(restored,17),50,"shared percent alias matches Circle speed");
change(1,0,3);change(17,125);restored=author();eq(value(restored,1),0,"Light Path stays Core0");eq(byName["Motion Type"].value,2,"Light Path writes native2");
eq(byName.Speed.value,125,"Light Path Speed125 is authored");eq(value(restored,4),125*(2*Math.PI/100),"angular alias matches percent source");
change(1,2,3);restored=author();eq(value(restored,1),2,"Look At stays Core2");eq(byName["Motion Type"].value,3,"Look At writes native3");
change(1,1,3);change(4,-Math.PI);restored=author();eq(byName.Speed.value,-50,"negative Circle speed remains available");eq(value(restored,17),0,"inactive Path speed stays in its supported range");
change(1,0,3);restored=author();eq(byName.Speed.value,0,"switching a negative Circle speed to Path uses its nonnegative source");
change(1,1,3);change(4,2*Math.PI);restored=author();eq(value(restored,1),1,"Circle survives a complete mode round trip");
// A cached scalar getter is not the authority after native publication. The
// nonanimated selector uses AE's pre-expression sample when that API exists.
actualMode=3;
Object.defineProperty(modeControl,"value",{configurable:true,get(){return 3;},set(v){actualMode=v;}});
modeControl.valueAtTime=function(time,preExpression){assert.equal(time,0);assert.equal(preExpression,true);return actualMode;};
restored=author();eq(value(restored,1),1,"Motion sampled getter preserves native Circle while scalar wrapper is stale Look At");
eq(byName["Panel Sync Guard"].value,5,"Circle author retains explicit native intent until graph publication");
// A required sample failure must not return a plausible ordinary value.
modeControl.valueAtTime=function(){throw Error("injected sample failure");};
assert.throws(()=>host.testRead(effect,layer),/injected sample failure/);checks++;
delete modeControl.valueAtTime;
assert.throws(()=>host.testRead(effect,layer),/Cannot sample Motion Type/);checks++;
modeControl.valueAtTime=function(time,preExpression){assert.equal(time,0);assert.equal(preExpression,true);return this.value;};
Object.defineProperty(modeControl,"value",{configurable:true,writable:true,value:actualMode});
const schema=vm.runInNewContext(fs.readFileSync(path.join(root,"tools/candidates/native_record_contract.js"),"utf8")+"\ncreateNativeRecordContract()");eq(schema.schema(graph.nodes[0]),1,"preparation contract accepts the same current record");
change(1,2,3);eq(schema.schema(graph.nodes[0]),1,"preparation contract accepts existing Look At mode");
console.log(`Motion gateway: ${checks} checks, 0 failures; actual registration, three modes, speeds and complete author/readback.`);
