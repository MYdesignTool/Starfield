"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),panelPath=require("./panel_test_paths.cjs");
const api=require(panelPath("js/optional_modules.js"));
let appended=[],timers=new Map(),serial=0;
const head={appendChild(script){appended.push(script);script.parentNode=this;},removeChild(script){script.parentNode=null;}};
const root={document:{head,createElement(){return {}; }},setTimeout(fn){timers.set(++serial,fn);return serial;},clearTimeout(id){timers.delete(id);}};
let loader=api.create(root),a,b;
assert.equal(appended.length,0,"startup performs no optional load");
loader.loadModel("92",r=>a=r);loader.loadModel("92",r=>b=r);
assert.equal(appended.length,1,"concurrent operations share one loading sequence");
assert.equal(appended[0].src,"js/model_assets.js?v=92");
root.StarfieldModelAssets={create(){}};appended[0].onload();
assert.equal(appended.length,2);assert.equal(appended[1].src,"js/model_graph_transactions.js?v=92");
root.StarfieldModelGraphTransactions={create(){}};appended[1].onload();
assert.equal(a.ok,true);assert.equal(b.ok,true);assert.equal(timers.size,0);
loader.loadModel("92",r=>a=r);assert.equal(appended.length,2,"loaded modules are reused");
delete root.StarfieldModelGraphTransactions;loader.loadModel("92",r=>a=r);
loader.loadModel("93",r=>b=r);assert.equal(b.error.code,"module_generation_mismatch");
appended.at(-1).onerror();assert.equal(a.ok,false);assert.equal(timers.size,0);
loader.loadModel("92",r=>a=r);Array.from(timers.values())[0]();assert.equal(a.ok,false);assert.equal(timers.size,0);
loader.loadModel("../wrong",r=>a=r);assert.equal(a.error.code,"module_generation_invalid");
// Execute the browser transaction factory with both optional modules absent.
// Default Cube/Force additions must commit without invoking its loader; deletion
// must wait for the loader, and loader failure must perform no host mutation.
const codec=require(panelPath("js/graph_codec.js")),edits=require(panelPath("js/graph_edits.js")),layout=require(panelPath("js/graph_layout.js"));
const snapshots=require(panelPath("js/native_graph_snapshot.js")),fixture=require("./native_snapshot_fixture.js");
const uid=n=>n.toString(16).padStart(32,"0");
const graph={version:1,nodes:[fixture.node("emitter",uid(1)),fixture.node("particle",uid(2)),fixture.node("output",uid(3))],edges:[],optionalRecords:[]};
const base=fixture.receipt({ok:true,target:{token:"p0-c1-l1"},snapshot:{initialized:true,revision:4,graphHex:codec.toHex(graph)}});
function browser(){const w={StarfieldGraphLayout:layout,StarfieldNativeGraphSnapshot:snapshots};
    vm.runInNewContext(fs.readFileSync(panelPath("js/graph_transactions.js"),"utf8"),{window:w});return w.StarfieldGraphTransactions;}
for(const type of ["force","model"]){let loads=0,calls=[],result;
    const client=browser().create({codec,edits,idFactory:()=>uid(99),loadModelModules(){loads++;},call(op,request,cb){calls.push(op);cb(fixture.receipt({ok:true,target:base.target,snapshot:{...base.snapshot,revision:5,graphHex:request.graphHex}}));}});
    client.apply({type:"addNode",nodeType:type},r=>result=r,"p0-c1-l1",4,base);
    assert.equal(result.ok,true);assert.deepEqual(calls,["submitGraph"]);assert.equal(loads,0,type+" needs no asset/backup modules");
}
let loads=0,calls=0,result,resume;
const client=browser().create({codec,edits,idFactory:()=>uid(99),loadModelModules(cb){loads++;resume=cb;},call(){calls++;}});
client.apply({type:"deleteNodes",nodeIds:[uid(2)]},r=>result=r,"p0-c1-l1",4,base);
assert.equal(loads,1);assert.equal(calls,0);assert.equal(result,undefined,"full transaction waits for optional modules");
resume({ok:false,error:{code:"optional_module_load_failed",message:"injected"}});
assert.equal(result.error.code,"optional_module_load_failed");assert.equal(calls,0);
let loadSuccess,fullCommits=0;
const success=browser().create({codec,edits,idFactory:()=>uid(99),loadModelModules(cb){loadSuccess=cb;},call(){throw Error("full deletion cannot use ordinary submit");}});
success.apply({type:"deleteNodes",nodeIds:[uid(2)]},r=>result=r,"p0-c1-l1",4,base);
loadSuccess({ok:true,assets:require(panelPath("js/model_assets.js")),transactions:{create(){return {apply(tx,meshes,cb){
    fullCommits++;assert.deepEqual(Array.from(meshes),[]);cb(fixture.receipt({ok:true,target:base.target,snapshot:{...base.snapshot,revision:5,graphHex:tx.graphHex}}));}};}}});
assert.equal(result.ok,true);assert.equal(fullCommits,1,"full transaction starts once after successful loading");
loadSuccess({ok:true});assert.equal(fullCommits,1,"duplicate loader callbacks cannot replay mutations");
console.log("Optional modules: demand-only loading, sequencing, reuse, coalescing, failure/deadline, no Cube asset transaction and no mutation before module availability passed.");
