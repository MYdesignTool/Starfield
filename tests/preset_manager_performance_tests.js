"use strict";
const panelPath=require("./panel_test_paths.cjs");

const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm");
const codec=require(panelPath("js/graph_codec.js")),edits=require(panelPath("js/graph_edits.js"));
const presets=require(panelPath("js/presets.js")),transactions=require(panelPath("js/graph_transactions.js"));
const snapshots=require(panelPath("js/native_graph_snapshot.js")),fixture=require("./native_snapshot_fixture.js");
const gatewaySource=fs.readFileSync(panelPath("jsx/starfield_gateway.jsx"),"utf8");
const probe={};vm.runInNewContext(gatewaySource,{$:{global:probe}});
const ready=probe.SFLD_ready(),generation=ready.split("/").pop();
const calls=[],timers=new Map();let timerId=0,loads=0,revision=1,initialized=true,changeBeforeSubmit=false;
let graph=presets.build(presets.catalog[0].id,1080);
function response(payload){return JSON.stringify({protocol:"org.starfieldfx.panel",version:1,gatewayBuild:generation,...payload});}
function snapshot(){return fixture.receipt({ok:true,target:{token:"target"},snapshot:{initialized,revision,
    recordStamp:"records-"+revision,graphHex:codec.toHex(graph),geometry:{width:1920,height:1080,pixelAspect:1}}});}
const host={SFLD_ready:()=>ready,File:function(path){this.path=path;},$:{evalFile(){loads++;host.SFLD_ready=()=>ready;}}};
host.SFLD_getState=()=>{calls.push("getState");return response({ok:true,target:{token:"target",comp:"Comp",layer:"Particles"}});};
host.SFLD_getGraphSnapshot=()=>{calls.push("getGraphSnapshot");return response(snapshot());};
host.SFLD_syncGraphSnapshot=()=>{calls.push("syncGraphSnapshot");initialized=true;revision++;return response(snapshot());};
function commit(request){
    assert.equal(request.target.token,"target");
    if(changeBeforeSubmit){revision++;changeBeforeSubmit=false;}
    if(request.baseGraphRevision!==revision || request.baseRecordStamp!=="records-"+revision)
        return response({ok:false,error:{code:"stale_graph",message:"The native graph changed."}});
    graph=codec.fromHex(request.graphHex);revision++;return response(snapshot());
}
host.SFLD_submitGraph=text=>{calls.push("submitGraph");return commit(JSON.parse(text));};
class Element {
    constructor(){this.children=[];this.listeners={};this.value="";this.checked=true;}
    set textContent(text){this.text=text;this.children=[];}get textContent(){return this.text;}
    addEventListener(type,callback){this.listeners[type]=callback;}
    appendChild(child){this.children.push(child);}setAttribute(){}
    getContext(){return new Proxy({},{get:()=>()=>{}});}
}
const elements={};
const document={getElementById:id=>elements[id] || (elements[id]=new Element()),createElement:()=>new Element()};
const window={StarfieldPresets:presets,StarfieldGraphCodec:codec,StarfieldGraphTransactions:{create(options){
    return transactions.create({...options,modelTransactions:{apply(fields,assets,callback){
        assert.deepEqual(Array.from(assets),[]);calls.push("modelApply");callback(JSON.parse(commit(fields)));
    }}});
}},
    StarfieldModelAssets:require(panelPath("js/model_assets.js")),StarfieldPresetFiles:require(panelPath("js/preset_files.js")),
    StarfieldNativeGraphSnapshot:snapshots,close(){},__adobe_cep__:{getSystemPath:()=>"C:/test/web_panel",
        evalScript(script,callback){callback(vm.runInNewContext(script,host));}}};
const context={window,document,console,Date,JSON,Math,
    setTimeout(callback,delay){timers.set(++timerId,{callback,delay});return timerId;},clearTimeout:id=>timers.delete(id)};
vm.runInNewContext(fs.readFileSync(panelPath("js/preset_manager.js"),"utf8"),context);
assert.deepEqual(calls,["getState","getGraphSnapshot"]);
elements.all.listeners.click();elements.grid.children[0].listeners.click();
for(const mode of ["add","replace"]){
    calls.length=0;const beforeLoads=loads,beforeCount=graph.nodes.length;
    elements[mode].listeners.click();
    assert.deepEqual(calls,["getGraphSnapshot",mode==="add"?"submitGraph":"modelApply"],mode+" uses one planning read and one graph commit; "+elements.status.textContent);
    assert.equal(loads,beforeLoads,"matching-generation writes reuse gateway code in the same host turn");
    assert.equal(timers.size,0,"completed operations leave no timeout backlog");
    assert.equal(elements.status.className,"");
    assert.equal(graph.nodes.length,mode==="add"?beforeCount+3:4);
}
calls.length=0;changeBeforeSubmit=true;const beforeConflict=codec.toHex(graph);
elements.add.listeners.click();
assert.deepEqual(calls,["getGraphSnapshot","submitGraph"]);
assert.equal(codec.toHex(graph),beforeConflict,"a stale prepared receipt cannot overwrite current state");
assert.match(elements.status.textContent,/stale_graph/);
initialized=false;calls.length=0;elements.add.listeners.click();
assert.deepEqual(calls,["getGraphSnapshot","syncGraphSnapshot","submitGraph"],"uninitialized targets still explicitly synchronize");
assert.equal(elements.status.className,"");
assert.equal(edits.schemaVersion("emitter"),7);
console.log("Preset manager performance checks passed: initialized Add/Replace planning, ordinary/full transaction routing, atomic writes, conflict preservation and explicit bootstrap; full host transaction is simulated.");
