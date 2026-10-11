"use strict";
// Acceptance scheduling/cleanup/fixture self-test; all AE operations simulated.
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=path.resolve(__dirname,".."),panel=process.env.STARFIELD_PANEL_ROOT;if(!panel)throw Error("Set STARFIELD_PANEL_ROOT.");
const fixture=JSON.parse(fs.readFileSync(path.join(root,"artifacts/model-acceptance-panel71/fixtures.json"),"utf8")),codec=require(path.join(panel,"js/graph_codec.js"));
const source=fs.readFileSync(path.join(root,"tools/Run-ModelAcceptance.jsx"),"utf8");let checks=0;const eq=(a,b,message)=>{checks++;assert.deepEqual(a,b,message);};
const parser=require("internal/deps/acorn/acorn/dist/acorn");
for(const text of [source,fs.readFileSync(path.join(root,"artifacts/model-acceptance-panel71/Run.jsx"),"utf8")]){parser.parse(text,{ecmaVersion:3,allowReserved:"never"});checks++;}
let reports=0;vm.runInNewContext(source,{$:{global:{},writeln(){reports++;}}});eq(reports,1);
for(const mode of ["pass","native-failure","export-failure","busy","timeout"]){
    let serial=10,queue=[],writes=[],suppressed=0,revision=11,nodes=[],renderer={},plan=null,started=0,exported=null,now=5000;
    const created=[],items=[],host={};
    function item(name){const v={id:serial++,name,remove(){assert.ok(created.includes(v),"Never remove an existing owner item.");items.splice(items.indexOf(v),1);}};items.push(v);created.push(v);return v;}
    function CompItem(){}const previous=new CompItem();Object.assign(previous,{id:2,name:"Owner",numLayers:1,layer:()=>ownerLayer,openInViewer(){app.project.activeItem=previous;}});
    const ownerLayer={selected:true};items.push(previous,{id:3,name:"Owner footage"});
    let solids=null;const app={version:"23.5x52",project:{activeItem:previous,rootFolder:{id:0},get numItems(){return items.length;},item:i=>items[i-1],items:{
        addFolder:name=>item(name),addComp(name){const c=new CompItem(),raw=item(name),layers=[];Object.assign(c,raw,{openInViewer(){app.project.activeItem=c;},get numLayers(){return layers.length;},layer:i=>layers[i-1],layers:{addSolid(color,name){if(!solids)solids=item("Solids");
            const source=item(name+" source");source.parentFolder=solids;const l={id:serial++,source,selected:false,property:()=>({addProperty(){}})};layers.unshift(l);return l;}}});items[items.indexOf(raw)]=c;created[created.indexOf(raw)]=c;
            c.remove=function(){assert.ok(created.includes(c));items.splice(items.indexOf(c),1);};return c;}}},
        beginSuppressDialogs(){suppressed++;},endSuppressDialogs(){suppressed--;},scheduleTask(script){queue.push(()=>vm.runInContext(script,context));return queue.length;},cancelTask(){}};
    const config={version:1,gatewayBuild:fixture.gatewayBuild,repo:root,fixture:"fixture",result:"result"};host.__SFLD_modelAcceptanceConfig=config;
    function File(name){this.name=name;}File.prototype.open=function(){return true;};File.prototype.close=function(){};File.prototype.read=function(){return JSON.stringify(fixture);};File.prototype.write=function(s){writes.push(JSON.parse(s));};
    const api={recordSchema:n=>n.schemaVersion};host.SFLD_modelTransactionReady=()=>fixture.gatewayBuild;host.__SFLD_modelTransactionAPI=api;
    const snap=()=>({initialized:true,revision,recordStamp:"r"+revision,nativeNodes:JSON.parse(JSON.stringify(nodes)),renderer});
    function reply(value){return JSON.stringify({ok:true,...value});}
    host.SFLD_ensureNodeEffects=()=>reply({snapshot:snap()});host.SFLD_getGraphSnapshot=()=>reply({snapshot:snap()});
    host.SFLD_beginModelGraphTransaction=text=>{plan=JSON.parse(text);started++;const parsed=codec.parse(Buffer.from(plan.graphHex,"hex"));
        for(const n of plan.nodeManifest)for(const p of n.parameters)if(n.type.endsWith("particle") && ["31","32"].includes(p.key)){
            const actual=parsed.nodes.find(a=>a.id===n.id).parameters.find(a=>a.key===p.key);eq(actual.value,p.value);}
        eq(plan.baseGraphRevision,revision);eq(plan.baseRecordStamp,"r"+revision);eq(/^[0-9a-f]{32}$/.test(plan.transactionId) && !/^0{32}$/.test(plan.transactionId),true);
        host.__SFLD_modelGraphTransactionV1={id:plan.transactionId,state:"uploading"};return reply({transactionId:plan.transactionId});};
    host.SFLD_writeModelGraphAssetPage=()=>reply({});host.SFLD_queueModelGraphTransaction=()=>{host.__SFLD_modelGraphTransactionV1.state="queued";
        queue.push(()=>{host.__SFLD_modelGraphTransactionV1.state=mode==="timeout"?"executing":"complete";if(mode!=="native-failure"){nodes=plan.nodeManifest;renderer=plan.rendererManifest;revision++;}});return reply({});};
    host.SFLD_readModelGraphTransaction=()=>reply({state:host.__SFLD_modelGraphTransactionV1.state,result:mode==="native-failure"?{ok:false,error:{code:"graph_commit_failed",message:"injected native failure"}}:{ok:true,committed:true,diagnostics:{cleanupError:0,undoError:0}}});
    host.SFLD_releaseModelGraphTransaction=()=>{host.__SFLD_modelGraphTransactionV1=null;return reply({});};
    host.SFLD_beginModelAssetExport=text=>{const request=JSON.parse(text);eq(request.baseGraphRevision,revision);exported=plan.assets.find(a=>a.nodeId===request.nodeId);return reply({state:"queued"});};
    host.SFLD_readModelAssetPage=()=>reply({state:"ready",pageCount:1,hex:mode==="export-failure"?"00":fixture.steps.find(s=>s.assets.some(a=>a.nodeId===exported.nodeId)).assets.find(a=>a.nodeId===exported.nodeId).meshHex});
    host.SFLD_releaseModelAsset=()=>reply({});if(mode==="busy")host.__SFLD_modelGraphTransactionV1={state:"executing",id:"owner"};
    const context=vm.createContext({CompItem,File,app,$:{global:host,writeln(){},evalFile(){}},Date:class extends Date{getTime(){return now;}}});
    if(mode==="busy"){assert.throws(()=>vm.runInContext(source,context));eq(items.length,2);eq(started,0);continue;}
    vm.runInContext(source,context);let count=0;while(queue.length && host.__SFLD_modelAcceptanceRun.running){assert.ok(++count<1000);now+=mode==="timeout"?40000:250;queue.shift()();}
    const result=writes[writes.length-1];eq(result.running,false);eq(result.status,mode==="pass"?"passed":"failed",result.error);eq(suppressed,0);
    if(mode==="timeout"){assert.ok(result.cleanup.startsWith("Deferred"));checks++;eq(items.length>2,true);}
    else{eq(items.map(i=>i.id),[2,3]);eq(app.project.activeItem,previous);eq(ownerLayer.selected,true);}
    if(mode==="pass"){eq(result.cases.length,8);eq(started,8);eq(result.cases.every(c=>c.ok),true);}
}
console.log(`model_acceptance_harness_tests: ${checks} checks passed; eight production fixtures, native/export failures, busy safety, timeout ownership and cleanup; no actual AE run`);
