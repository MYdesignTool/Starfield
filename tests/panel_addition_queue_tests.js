"use strict";
const assert=require("node:assert/strict"),path=require("node:path");
const panel=process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"..","web_panel");
const codec=require(path.join(panel,"js/graph_codec.js")),edits=require(path.join(panel,"js/graph_edits.js"));
const transactions=require(path.join(panel,"js/graph_transactions.js")),fixture=require("./native_snapshot_fixture.js");
const id=n=>n.toString(16).padStart(32,"0");let checks=0;
function check(value,message){++checks;assert.ok(value,message);}
function harness(extra={}) {
    let graph={version:1,nodes:[fixture.node("emitter",id(1)),fixture.node("particle",id(2)),fixture.node("output",id(3))],edges:[],optionalRecords:[]};
    let revision=8,nextId=20,target="target",inFlight=0,maxInFlight=0;
    const scheduled=[],requests=[],views=[],errors=[],history=[];
    const receipt=()=>fixture.receipt({ok:true,target:{token:target},snapshot:{initialized:true,revision,recordStamp:"s"+revision,
        graphHex:codec.toHex(graph),geometry:{width:1920,height:1080},layerResources:[]}});
    const queue=transactions.createAdditionQueue({codec,edits,idFactory:()=>id(nextId++),
        schedule(fn){scheduled.push(fn);},isCurrentTarget:token=>token===target,
        onView(graph,snapshot,status){views.push({graph,snapshot,status});},onError(result){errors.push(result);},
        call(operation,fields,callback){check(operation==="submitGraph","only the existing guarded submit is used");
            check(fields.baseGraphRevision===revision && fields.baseRecordStamp==="s"+revision,"uses latest confirmed revision/stamp");
            inFlight++;maxInFlight=Math.max(maxInFlight,inFlight);const request={fields,callback};requests.push(request);history.push(request);},...extra});
    return {queue,scheduled,requests,views,errors,history,receipt,
        add(kind="force",position={x:10,y:20}){return queue.enqueue({type:"addNode",nodeType:kind,position},receipt());},
        flush(){check(scheduled.length>0,"batch has a scheduled paint boundary");scheduled.shift()();},
        ack(failure,diagnostics){const request=requests.shift();check(!!request,"one request is awaiting acknowledgement");inFlight--;
            if(failure){request.callback({ok:false,error:{code:failure,message:failure}});return;}
            graph=codec.fromHex(request.fields.graphHex);revision++;
            request.callback({...receipt(),diagnostics});},
        target(value){target=value;},get graph(){return graph;},get maxInFlight(){return maxInFlight;}};
}
check(typeof transactions.createAdditionQueue==="function","continuous addition queue is available");
{
    const h=harness();for(let i=0;i<10;i++)check(h.add(i%2?"auxiliary":"force").ok,"burst accepts every independent addition");
    check(h.views.at(-1).graph.nodes.length===13 && h.requests.length===0,"all ten nodes paint before any host write");
    const ids=h.views.at(-1).graph.nodes.map(n=>n.id);h.flush();check(h.requests.length===1,"one batch starts after paint");
    for(let i=0;i<5;i++)check(h.add("particle").ok,"host wait does not block more additions");
    const allIds=h.views.at(-1).graph.nodes.map(n=>n.id);h.ack();
    check(h.views.at(-1).graph.nodes.length===18,"acknowledgement does not erase later visible additions");
    check(ids.every(id=>h.views.at(-1).graph.nodes.some(n=>n.id===id)),"first batch UUIDs remain stable");
    h.flush();h.ack();check(!h.queue.busy() && h.graph.nodes.length===18,"queue drains after two guarded commits");
    assert.deepEqual(h.graph.nodes.map(n=>n.id),allIds);check(h.maxInFlight===1,"host writes never overlap");
    check(h.errors.length===0 && h.views.at(-1).snapshot.revision===10,"final view owns the saved receipt");
}
{
    const h=harness();h.add();h.flush();for(let i=0;i<9;i++)h.add("particle");
    check(h.requests.length===1 && h.views.at(-1).graph.nodes.length===13,"nine later drops stay visible during the first host wait");
    h.ack();h.flush();h.ack();check(h.history.length===2,"ten sequential drops during a host wait need two commits instead of ten");
}
for(const failure of ["stale_graph","host_timeout","graph_snapshot_unconfirmed"]) {
    const h=harness();h.add();h.flush();h.add("particle");h.ack(failure);
    check(!h.queue.busy() && h.errors.length===1,"failed or unknown write stops further submissions");
    check(h.views.at(-1).graph.nodes.length===3,"failure removes all unsaved previews");
    while(h.scheduled.length)h.flush();check(h.history.length===1,"a failed write is never automatically retried");
}
{
    const h=harness();h.add();h.flush();h.add("particle");h.ack();h.flush();h.ack("stale_graph");
    check(h.views.at(-1).graph.nodes.length===4,"later failure preserves already confirmed additions");
}
{
    const h=harness();h.add();h.flush();h.add("particle");h.ack(null,{cleanupError:"injected"});
    check(!h.queue.busy() && h.errors[0].error.code==="graph_applied_with_warning","cleanup warning stops unsent additions");
    check(h.views.at(-1).graph.nodes.length===4,"cleanup warning retains the confirmed applied batch");
}
{
    const h=harness();h.add();h.flush();h.add("particle");const count=h.views.length;h.target("other");h.ack();
    check(h.views.length===count && !h.queue.busy(),"old target acknowledgement cannot repaint the new target");
    while(h.scheduled.length)h.flush();check(h.history.length===1,"target switch discards unsent work");
}
{
    const h=harness();h.add();const oldFlush=h.scheduled.shift();h.queue.cancel();h.add("particle");oldFlush();
    check(h.history.length===0,"canceled timer cannot flush a later session");h.flush();h.ack();check(h.graph.nodes.length===4,"new session stays usable after cancellation");
}
{
    const h=harness();for(let i=0;i<61;i++)check(h.add().ok,"native author capacity accepts additions");
    check(h.add().error.code==="node_capacity","native author limit rejects before host mutation");
    check(h.views.at(-1).graph.nodes.length===64,"overflow preserves all accepted previews");
    while(h.queue.busy()){if(h.scheduled.length)h.flush();if(h.requests.length)h.ack();}
    check(h.history.length===4 && h.graph.nodes.length===64,"bounded sixteen-node commits preserve all accepted additions");
}
{
    const h=harness();h.add();check(!h.add("unknown").ok,"invalid addition is rejected before submission");
    check(h.queue.busy() && h.views.at(-1).graph.nodes.length===4,"invalid addition cannot discard earlier accepted work");h.flush();h.ack();
}
{
    const h=harness({schedule(fn){throw Error("scheduler unavailable");}});h.add();
    check(!h.queue.busy() && h.history.length===0 && h.errors.length===1,"schedule failure cannot leave a deferred write alive");
}
{
    const h=harness();h.add();h.flush();const cb=h.requests[0].callback;h.ack();const count=h.views.length;
    cb(h.receipt());check(h.views.length===count && h.history.length===1,"duplicate response cannot settle or submit twice");
}
{
    const h=harness();const kinds=["emitter","auxiliary","particle","force","transform","motion","turbulence","model","material"];
    for(const kind of kinds)check(h.add(kind).ok,"mixed node defaults preview without a host round trip");
    const preview=codec.toHex(h.views.at(-1).graph);h.flush();h.ack();
    check(h.history.length===1 && codec.toHex(h.graph)===preview,"mixed defaults and positions survive one normal guarded commit");
    check(h.graph.nodes.filter(n=>n.type===edits.types.output).length===1,"batch cannot duplicate the required Output");
}
{
    const h=harness();h.add();h.flush();const request=h.requests[0],callback=request.callback;
    request.callback=result=>callback({...result,target:{token:"wrong-target"}});h.ack();
    check(h.errors[0].error.code==="stale_target" && !h.queue.busy(),"a mismatched acknowledgement is never adopted");
}
{
    const h=harness();const graph=codec.fromHex(h.receipt().snapshot.graphHex),before=codec.toHex(graph);
    assert.throws(()=>edits.apply(graph,{type:"addNodes",nodes:[{nodeType:"force",nodeId:id(40)},{nodeType:"particle",nodeId:id(40)}]}),/already exists/);
    check(codec.toHex(graph)===before,"a failed batch cannot partially mutate its input graph");
    assert.throws(()=>edits.apply(graph,{type:"addNodes",nodes:Array.from({length:17},(_,i)=>({nodeType:"force",nodeId:id(40+i)}))}),/sixteen/);
    assert.throws(()=>edits.apply(graph,{type:"addNodes",nodes:[{nodeType:"output",nodeId:id(40)}]}),/editable/);
}
console.log(`panel_addition_queue_tests: ${checks} checks passed; delayed host, stable drafts, batching, failure, target change and bounded writes; no AE run`);
