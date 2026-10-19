"use strict";
// Execute the actual gateway comparator and read-only diagnostics without AE.
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),panel=require("./panel_test_paths.cjs");
const source=fs.readFileSync(panel("jsx/starfield_gateway.jsx"),"utf8").replace(
    "    function verifyAuthoredMotionModes(authored, saved, layer) {",
    "    $.global.verifyMotion=verifyAuthoredMotionModes;\n    function verifyAuthoredMotionModes(authored, saved, layer) {");
const host={},context=vm.createContext({$:{global:host},app:{version:"23.5x52 fixture"}});
vm.runInContext(source,context);assert.equal(typeof host.verifyMotion,"function");
const id="bd05c9fc81e34a2088e0f6c06c7e62dc",kind="org.starfieldfx.node.motion";
let reads=0,writes=0,throwSamples=false,duplicate=false,missing=false,sampleMode=1,ordinaryMode=2;
const controls={};
function property(name,matchName,value){return {name,matchName,get value(){reads++;return typeof value==="function"?value():value;},
    setValue(){writes++;throw Error("Diagnostic must never write");}};}
for(let i=0;i<8;i++)controls["Node UUID "+i]=property("Node UUID "+i,"uuid"+i,parseInt(id.slice(i*4,i*4+4),16));
controls["Panel Sync Guard"]=property("Panel Sync Guard","guard",0);
controls["Motion Type"]=property("Motion Type",kind+"-1701",()=>{if(throwSamples)throw Error("sample denied");return ordinaryMode;});
controls["Motion Type"].valueAtTime=function(time,pre){assert.ok(time===0||time===2.5);assert.equal(pre,true);reads++;if(throwSamples)throw Error("sample denied");return sampleMode;};
const effect={matchName:kind,get numProperties(){return Object.keys(controls).length;},property(key){
    reads++;return typeof key==="number"?Object.values(controls)[key-1]:key===kind+"-1701"?controls["Motion Type"]:controls[key]||null;
}};
const layer={containingComp:{time:2.5},property(key){assert.equal(key,"ADBE Effect Parade");reads++;return {
    numProperties:missing?0:duplicate?2:1,property(){return effect;}
};}};
const record=(value,type="org.starfieldfx.nodes.motion")=>({id,type,parameters:[{key:"1",type:3,value}]});
let checks=0;
function eq(actual,expected,message){checks++;assert.deepEqual(actual,expected,message);}
function reject(saved=record(0),authored=record(1)){
    let error;try{host.verifyMotion([authored],saved?[saved]:[],layer);}catch(e){error=e;}
    assert.ok(error,"mismatch must reject");assert.match(error.message,/Published Motion Type differs from authored/);
    return {message:error.message,evidence:JSON.parse(error.message.split(" Motion readback: ")[1])};
}
host.verifyMotion([record(1)],[record(1)],layer);
host.verifyMotion([record(1,"org.starfieldfx.nodes.force")],[],layer);
eq(reads,0,"passing/non-Motion comparisons perform no host reads");
let result=reject();eq(result.evidence.node,id);eq(result.evidence.host,"23.5x52 fixture");
eq(result.evidence.gateway,source.match(/var GATEWAY_BUILD = "([^"]+)"/)[1]);
eq(result.evidence.guard,0);eq(result.evidence.named,2);eq(result.evidence.disk,2);
eq(result.evidence.namedZero,1);eq(result.evidence.diskZero,1);eq(result.evidence.namedCurrent,1);eq(result.evidence.diskCurrent,1);
eq(result.evidence.time,2.5);assert.match(result.message,/expected 1, actual 0/);
ordinaryMode=sampleMode=3;result=reject(record(2));eq(result.evidence.diskZero,3,"actual native drift remains a failure");
ordinaryMode=sampleMode=1;result=reject();eq(result.evidence.disk,1,"later matching value does not hide the original snapshot failure");
duplicate=true;result=reject();assert.match(result.evidence.guard,/Multiple AE node effects share/);duplicate=false;
missing=true;result=reject(null);assert.match(result.message,/actual missing/);assert.match(result.evidence.named,/Unique Motion source unavailable/);missing=false;
throwSamples=true;result=reject();assert.match(result.evidence.diskZero,/sample denied/);eq(result.evidence.guard,0,"one failed probe does not hide other evidence");throwSamples=false;
result=reject(record(1,"org.starfieldfx.nodes.force"));assert.match(result.message,/expected 1, actual 1/);
const sample=controls["Motion Type"].valueAtTime;delete controls["Motion Type"].valueAtTime;
result=reject();assert.match(result.evidence.diskZero,/unavailable/);eq(result.evidence.disk,1);controls["Motion Type"].valueAtTime=sample;
// Even a diagnostics infrastructure error must preserve the mismatch.
vm.runInContext("JSON.stringify=function(){throw new Error('diagnostic serialization denied');}",context);
assert.throws(()=>host.verifyMotion([record(1)],[record(0)],layer),/Published Motion Type differs.*Motion readback unavailable/);
eq(writes,0,"all diagnostics are read-only");
console.log(`Motion readback diagnostics: ${checks} value checks plus rejection/identity/getter failure controls passed; success path has zero host reads.`);
