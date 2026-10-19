"use strict";
const assert=require("node:assert/strict"),fs=require("fs"),path=require("path"),vm=require("vm");
const root=path.resolve(__dirname,".."),panel=process.env.STARFIELD_PANEL_ROOT || path.join(root,"web_panel");
const codec=require(path.join(panel,"js/graph_codec.js")),edits=require(path.join(panel,"js/graph_edits.js")),view=require(path.join(panel,"js/graph_view.js"));
const transactions=require(path.join(panel,"js/graph_transactions.js")),fixture=require("./native_snapshot_fixture.js");
const parser=require("internal/deps/acorn/acorn/dist/acorn"),source=fs.readFileSync(path.join(panel,"js/panel.js"),"utf8");
const body=parser.parse(source,{ecmaVersion:2017}).body[0].expression.callee.body.body;
const names=["previewTopologyGraph","restoreTopologyPreview","confirmTopologySnapshot","afterTopologyPaint","loadModelModules","getGraphTransactionClient","applyTopologyEdit"];
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
for(const [edit,host] of [[{type:"addNode",nodeType:"force"},"submitGraph"],[{type:"addNode",nodeType:"model"},"submitGraph"],
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
// Execute the real refresh/adoption branches: the AE Controls legacy response
// must not fabricate a Force chain after the canonical bootstrap failed.
{
    const functions=["adoptState","retainGraphOnReadFailure","refresh"].map(name=>{
        const n=body.find(n=>n.type==="FunctionDeclaration" && n.id.name===name);return source.slice(n.start,n.end);
    }).join("\n");
    const graph={version:1,nodes:[fixture.node("emitter",id(1)),fixture.node("particle",id(2)),fixture.node("output",id(3))],edges:[],optionalRecords:[]};
    const snapshot=fixture.receipt({ok:true,snapshot:{initialized:true,revision:8,graphHex:codec.toHex(graph),geometry:{width:1920,height:1080},layerResources:[]}}).snapshot;
    const response={ok:true,target:{token:"p0-c1-l71",comp:"Comp1",layer:"new layer"},revision:"r0",controlSource:"AE Controls",
        nodes:[{id:"emitter"},{id:"particle"},{id:"force"},{id:"output"}],edges:[{id:"legacy"}],layoutPersistence:true};
    let nextSnapshot={ok:false,error:{code:"node_effect_sync_failed",message:"No saved receipt"}},renders=0;
    const state={targetToken:null,revision:null,graph:null,graphSnapshot:null,nodes:[],edges:[],pending:false};
    const banner={className:"banner",textContent:"",classList:{contains(name){return banner.className===name;}}};
    const context=vm.createContext({window:{StarfieldGraphView:view,StarfieldGraphCodec:codec},state,nodePositions:{},
        lastFullRefreshAt:0,lastPulseStamp:null,refreshEpoch:0,refreshInFlight:false,startupRetryTimer:null,startupRetryAttempt:0,
        failedGraphInitializations:{},resolvedTarget:false,retainedError:false,latestFrameStatusResponse:null,
        elements:{banner,targetLine:{},modeLine:{},revisionLine:{},resolutionLine:{}},nodePalette:{refresh(){}},
        canvasInteractionActive(){return false;},resetViewForTarget(){},syncTargetLock(){},updateFrameStatus(){},
        applyProjectNodeLayout(){return false;},render(){renders++;},
        showError(c,m){banner.className="error";banner.textContent=c+": "+m;},clearBanner(){banner.className="banner";banner.textContent="";},
        call(op,extra,cb){check(op==="getPanelState","refresh reads panel state");cb(response);},
        loadGraphSnapshot(token,cb){cb(nextSnapshot);}});
    vm.runInContext(functions,context);
    context.refresh(false,false);
    check(state.nodes.length===0 && state.edges.length===0 && !state.graph && !state.topologyReady,
        "failed fresh bootstrap shows no legacy Force chain");
    check(banner.textContent.includes("node_effect_sync_failed") && renders===1,"original bootstrap error stays visible with empty canvas");
    response.controlSource="Node Graph";nextSnapshot={ok:true,snapshot};context.refresh(false,false);
    check(state.topologyReady && state.nodes.length===3 && state.graph && banner.textContent==="","later success adopts canonical graph and clears error");
    const savedGraph=state.graph,savedNodes=state.nodes,savedRenders=renders;
    nextSnapshot={ok:false,error:{code:"read_failed",message:"Unavailable"}};context.refresh(false,false);
    check(state.graph===savedGraph && state.nodes===savedNodes && renders===savedRenders && !state.topologyReady,
        "same-target transient failure retains confirmed canvas without authorizing stale receipt");
    response.target={...response.target,token:"p0-c1-l72"};response.controlSource="AE Controls";context.refresh(false,false);
    check(state.targetToken==="p0-c1-l72" && !state.graph && !state.graphSnapshot && state.nodes.length===0,
        "failed different target clears prior graph and legacy fallback");
    check(banner.textContent.includes("read_failed"),"target switch retains failure evidence");
    context.adoptState(response,null);
    check(state.nodes===response.nodes,"explicit legacy adoption outside failed canonical refresh remains supported");
}
console.log(`panel_topology_responsiveness_tests: ${checks} checks passed; actual panel/client, projection, receipt reuse, rollback and failed-bootstrap display; no AE run`);
