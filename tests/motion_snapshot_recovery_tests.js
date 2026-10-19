"use strict";
// Real snapshot reader and panel refresh/edit functions; no AE or project writes.
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const panel=process.env.STARFIELD_PANEL_ROOT;if(!panel)throw Error("Set STARFIELD_PANEL_ROOT to the candidate.");
const codec=require(path.join(panel,"js/graph_codec.js")),edits=require(path.join(panel,"js/graph_edits.js"));
const snapshots=require(path.join(panel,"js/native_graph_snapshot.js")),fixture=require("./native_snapshot_fixture.js");
const parser=require("internal/deps/acorn/acorn/dist/acorn"),source=fs.readFileSync(path.join(panel,"js/panel.js"),"utf8");
const body=parser.parse(source,{ecmaVersion:2017}).body[0].expression.callee.body.body;
const code=["isStartupRetryable","retainGraphOnReadFailure","refresh","applyTopologyEdit"].map(name=>{
    const node=body.find(n=>n.type==="FunctionDeclaration" && n.id.name===name);assert.ok(node,name);return source.slice(node.start,node.end);
}).join("\n");
let checks=0,serial=10;const id=()=>String(serial++).padStart(32,"0"),token="p0-c1-l29";
const eq=(actual,expected,message)=>{checks++;assert.deepEqual(actual,expected,message);};
let graph={version:1,nodes:[],edges:[],optionalRecords:[]};
for(const kind of ["emitter","particle","motion","motion","motion","output"])graph=edits.apply(graph,{type:"addNode",nodeType:kind},id);
const motions=graph.nodes.filter(n=>n.type===edits.types.motion);
for(let mode=0;mode<3;mode++)graph=edits.apply(graph,{type:"setParameters",changes:[{nodeId:motions[mode].id,parameterKey:"1",valueType:3,value:mode}]});
function receipt(value,target=token){return fixture.receipt({ok:true,target:{token:target},snapshot:{initialized:true,revision:9,graphHex:codec.toHex(value),geometry:{width:1920,height:1080},layerResources:[]}});}
let result=snapshots.normalize(receipt(graph));eq(result.ok,true,"whole graph with all three modes loads");
eq(codec.fromHex(result.snapshot.graphHex).nodes.filter(n=>n.type===edits.types.motion).map(n=>n.parameters.find(p=>p.key==="1").value),[0,1,2]);
const appended=edits.apply(codec.fromHex(result.snapshot.graphHex),{type:"addNode",nodeType:"motion"},id);
result=snapshots.normalize(receipt(appended));eq(result.ok,true,"adding Circle to existing Look At graph acknowledges");
eq(codec.fromHex(result.snapshot.graphHex).nodes.filter(n=>n.type===edits.types.motion).map(n=>n.parameters.find(p=>p.key==="1").value),[0,1,2,1]);
const invalid=codec.fromHex(codec.toHex(graph));invalid.nodes.find(n=>n.id===motions[2].id).parameters.find(p=>p.key==="1").value=3;
const rejected=snapshots.normalize(receipt(invalid));eq(rejected.ok,false);eq(rejected.error.code,"invalid_native_node_record");eq(rejected.target.token,token,"read failure retains resolved target identity");
const other=snapshots.normalize(receipt(invalid,"p0-c2-l1"));
function harness(response,snapshotResponse){
    const confirmed=snapshots.normalize(receipt(graph)).snapshot,trace=[],state={targetToken:token,revision:"r8",nodes:graph.nodes,edges:graph.edges,
        graph,graphSnapshot:confirmed,graphMode:true,topologyReady:true,pending:false};
    const context=vm.createContext({state,refreshEpoch:0,refreshInFlight:false,startupRetryTimer:null,startupRetryAttempt:0,
        lastPulseStamp:"old",retainedError:null,STARTUP_RETRY_DELAYS_MS:[100],resolvedTarget:true,
        elements:{targetLine:{},banner:{},autoRefresh:{checked:true}},nodePalette:{refresh(){trace.push("palette");}},
        window:{setTimeout(){throw Error("Unexpected delayed retry");},clearTimeout(){}},canvasInteractionActive:()=>false,
        call(operation,extra,callback){trace.push(operation);callback(response);},showError(code){trace.push(code);},clearBanner(){},
        render(){trace.push("render");},restoreTopologyPreview(){},syncTargetLock(){},failedGraphInitializations:{},
        adoptState(response,snapshot){trace.push("adopt");state.topologyReady=!!snapshot;state.graphSnapshot=snapshot;},
        loadGraphSnapshot(target,callback){trace.push("fresh-snapshot");callback(snapshotResponse);},
        getGraphTransactionClient(){return {apply(edit,callback,target,revision,received){trace.push("submit");eq(received.snapshot.revision,9,"edit uses fresh receipt");callback({ok:false,error:{code:"injected",message:"stop after submission"}});}};}
    });
    vm.runInContext(code,context);return {context,state,trace,confirmed};
}
{
    const h=harness(rejected,rejected);h.context.refresh(false,false);
    eq(h.state.nodes,graph.nodes,"normalization failure keeps exact canvas");eq(h.state.graphSnapshot,h.confirmed);eq(h.state.targetToken,token);eq(h.state.revision,"r8");eq(h.state.topologyReady,false);
    eq(h.trace.includes("render"),false,"failure never paints an empty canvas");
    h.context.applyTopologyEdit({type:"addNode",nodeType:"motion"});eq(h.trace.includes("fresh-snapshot"),true,"retained graph requires a fresh read before editing");eq(h.trace.includes("submit"),false,"invalid fresh record never submits stale graph");eq(h.state.pending,false);
}
{
    const h=harness(other,other);h.context.refresh(false,false);eq(h.state.nodes.length,0,"a different target does not retain this target's graph");eq(h.state.targetToken,null);
}
{
    const h=harness({ok:true,target:{token}},rejected);h.context.refresh(false,false);eq(h.state.graphSnapshot,h.confirmed,"snapshot-stage failure also keeps confirmed graph");eq(h.state.topologyReady,false);eq(h.trace.includes("adopt"),false);
}
{
    const valid=snapshots.normalize(receipt(graph)),h=harness(rejected,valid);h.context.refresh(false,false);h.context.applyTopologyEdit({type:"addNode",nodeType:"motion"});
    eq(h.trace.includes("submit"),true,"valid fresh receipt restores edit progress");eq(h.state.pending,false);
}
console.log(`motion_snapshot_recovery_tests: ${checks} checks passed; three-mode whole-graph load/add, strict invalid rejection, target-scoped canvas retention and fresh-receipt recovery; no AE run.`);
