"use strict";
const assert=require("node:assert/strict"),fs=require("fs"),path=require("path"),vm=require("vm");
const root=path.resolve(__dirname,".."),panel=process.env.STARFIELD_PANEL_ROOT || path.join(root,"cep_panel");
const codec=require(path.join(panel,"js/graph_codec.js")),edits=require(path.join(panel,"js/graph_edits.js")),view=require(path.join(panel,"js/graph_view.js"));
const transactions=require(path.join(panel,"js/graph_transactions.js")),fixture=require("./native_snapshot_fixture.js");
const parser=require("internal/deps/acorn/acorn/dist/acorn"),source=fs.readFileSync(path.join(panel,"js/panel.js"),"utf8");
const body=parser.parse(source,{ecmaVersion:2017}).body[0].expression.callee.body.body;
const names=["previewTopologyGraph","restoreTopologyPreview","confirmTopologySnapshot","afterTopologyPaint","getGraphTransactionClient","applyTopologyEdit"];
const code=names.map(name=>{const n=body.find(n=>n.type==="FunctionDeclaration" && n.id.name===name);assert.ok(n,name);return source.slice(n.start,n.end);}).join("\n");
const id=n=>n.toString(16).padStart(32,"0");let checks=0;
function check(value,message){++checks;assert.ok(value,message);}
function harness(fail=false,hidden=false) {
    const graph={version:1,nodes:[fixture.node("emitter",id(1)),fixture.node("particle",id(2)),fixture.node("output",id(3))],edges:[],optionalRecords:[]};
    const snapshot=fixture.receipt({ok:true,snapshot:{initialized:true,revision:8,graphHex:codec.toHex(graph),geometry:{width:1920,height:1080},layerResources:[]}}).snapshot;
    const projected=view.project(graph,null,snapshot.geometry,[]),trace=[],frames=[],timers=[],pending=[];
    const state={targetToken:"p0-c1-l1",revision:"r8",graphMode:true,graphSnapshot:snapshot,graph,nodes:projected.nodes,edges:projected.edges,
        pending:false,topologyReady:true,layoutPersistence:true,selectedNodeId:id(2),selectedNodeIds:{[id(2)]:true}};
    function call(op,extra,cb){trace.push("host:"+op);pending.push(()=>{
        if(fail){cb({ok:false,error:{code:"injected",message:"Host rejected"}});return;}
        cb(fixture.receipt({ok:true,target:{token:state.targetToken},snapshot:{...snapshot,revision:9,graphHex:extra.graphHex}}));
    });}
    const win={StarfieldGraphCodec:codec,StarfieldGraphEdits:edits,StarfieldGraphView:view,
        StarfieldGraphTransactions:{create(options){return transactions.create({...options,idFactory:()=>id(99),
            modelTransactions:{apply(tx,meshes,cb){call("modelApply",tx,cb);}}});}},
        requestAnimationFrame(fn){assert.equal(this,win,"rAF requires its Window receiver");frames.push(fn);},setTimeout(fn,delay){timers.push({fn,delay});}};
    const context=vm.createContext({window:win,document:{hidden},state,nodePositions:projected.positions,topologyPreview:null,
        graphTransactionClient:null,refreshEpoch:0,refreshInFlight:false,nodePalette:{refresh(){}},call,
        render(){trace.push("paint-state:"+state.nodes.length);},showError(c){trace.push("error:"+c);},clearBanner(){},
        refresh(){trace.push("refresh");},loadGraphSnapshot(){throw Error("Cached receipt must be reused");}});
    vm.runInContext(code,context);
    return {context,state,trace,frames,timers,pending,oldGraph:graph,
        edit(edit){context.edit=edit;vm.runInContext("applyTopologyEdit(edit)",context);},
        paint(){while(frames.length)frames.shift()();const ready=timers.filter(t=>t.delay===0);ready.forEach(t=>t.fn());},
        finish(){pending.shift()();timers.forEach(t=>t.fn());}};
}
for(const [edit,host] of [[{type:"addNode",nodeType:"force"},"submitGraph"],[{type:"addNode",nodeType:"model"},"modelApply"],
    [{type:"deleteNodes",nodeIds:[id(2)]},"modelApply"]]) {
    const h=harness();h.edit(edit);
    check(h.trace[0].startsWith("paint-state:"),"validated draft projects before any host call");
    check(!h.pending.length && h.state.pending,"host submission waits for paint");
    check(h.state.nodes.length===(edit.type==="addNode"?4:2),"new/deleted node is visible immediately");
    check(h.state.graphSnapshot.revision===8,"draft does not invent a saved revision");
    h.paint();check(h.trace.includes("host:"+host) && h.pending.length===1,"host starts after paint");
    check(!h.trace.includes("host:getGraphSnapshot"),"cached planning receipt removes repeated host read");
    h.finish();check(!h.state.pending && h.pending.length===0,"one completed serialized submission");
    check(h.state.graphSnapshot.revision===9,"saved receipt is adopted before background refresh or next edit");
}
{
    const h=harness(true);h.edit({type:"deleteNodes",nodeIds:[id(2)]});h.paint();h.finish();
    check(h.state.graph===h.oldGraph && h.state.nodes.length===3,"failed commit restores exact prior graph");
    check(h.state.selectedNodeId===id(2) && h.state.selectedNodeIds[id(2)],"failure restores deleted selection");
    check(h.trace.includes("error:injected"),"host failure remains visible");
}
for(const hidden of [false,true]) {
    const h=harness(false,hidden);h.edit({type:"addNode",nodeType:"force"});
    h.timers.forEach(t=>t.fn());check(h.pending.length===1,"hidden/throttled panel still submits");
    h.paint();check(h.pending.length===1,"frame and timer paths never double submit");
}
{
    const h=harness();h.edit({type:"addNode",nodeType:"unknown"});
    check(!h.state.pending && h.state.graph===h.oldGraph && !h.pending.length,"invalid edit never previews or mutates host");
}
{
    const h=harness();let scheduled,result,called=0;
    const client=transactions.create({codec,edits,idFactory:()=>id(90),call(){called++;},
        scheduleSubmit(fn){scheduled=fn;throw Error("scheduler failed after queueing");}});
    client.apply({type:"addNode",nodeType:"force"},r=>{result=r;},h.state.targetToken,8,
        {ok:true,target:{token:h.state.targetToken},snapshot:h.state.graphSnapshot});
    check(result.error.code==="graph_preview_failed","scheduler failure reports a failed draft");
    scheduled();check(called===0,"a rejected deferred draft never submits later");
}
console.log(`panel_topology_responsiveness_tests: ${checks} checks passed; actual panel/client, immediate projection, paint before host, receipt reuse, rollback, hidden progress and single submission; no AE run`);
