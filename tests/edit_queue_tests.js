"use strict";
const assert=require("node:assert/strict"),path=require("node:path");
const root=process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"../web_panel"),edits=require(path.join(root,"js/graph_edits.js")),codec=require(path.join(root,"js/graph_codec.js")),queueApi=require(path.join(root,"js/edit_queue.js"));
let checks=0;function check(value,message){checks++;assert.ok(value,message);}
const uuid=n=>n.toString(16).padStart(32,"0"),target={token:"p0-c88-l111"};
function initial(){return {version:1,nodes:[{id:uuid(255),type:edits.types.output,schemaVersion:4,parameters:[{key:"1",type:3,value:1000}]}],edges:[],optionalRecords:[]};}
function snapshot(graph,revision,height=1080){return {initialized:true,revision,checksum:"00000001",projectScope:"D:/test.aep",geometry:{height,width:1920},graphHex:codec.toHex(graph),nativeNodes:graph.nodes.filter(n=>n.type!==edits.types.output).map(n=>({id:n.id,type:n.type,parameters:n.parameters,outgoing:graph.edges.filter(e=>e.sourceNode===n.id).map(e=>({target:e.destinationNode,id:e.id}))}))};}
function fixture(){let now=0,events=[],serial=0,epoch=1,current=true,hostGraph=initial(),revision=7,height=1080,lastGraph=hostGraph,lastStatus,errors=[],inflight=0,peak=0,submitted=[],settled=0,fail=false,delay=1100,held;
    function later(fn,ms){events.push({time:now+ms,id:serial++,fn});}
    const q=queueApi.create({codec,edits,schedule:fn=>later(fn,16),isCurrent:(token,scope,e)=>current && e===epoch,
        onView:(g,s,status)=>{lastGraph=g;lastStatus=status;},onError:(r,info)=>errors.push({r,info}),onSettled:()=>settled++,
        submit:(base,g,done,items)=>{inflight++;peak=Math.max(peak,inflight);check(inflight===1,"one host writer");check(base.snapshot.revision===revision,"rebase only onto our confirmed ACK");submitted.push({base,g,done,items});held=done;
            later(()=>{inflight--;if(fail){fail=false;done({ok:false,error:{code:"graph_edit_outcome_unknown",message:"Injected missing receipt"}});return;}hostGraph=g;revision++;const response={ok:true,target,snapshot:snapshot(hostGraph,revision,height)};done(response);done(response);},delay);}
    });
    return {q,get graph(){return lastGraph;},get status(){return lastStatus;},get errors(){return errors;},get peak(){return peak;},get submitted(){return submitted;},get revision(){return revision;},get host(){return hostGraph;},get settled(){return settled;},
        enqueue(edit){return q.enqueue(edit,{ok:true,target,snapshot:snapshot(hostGraph,revision,height)},epoch);},
        until(time){let count=0;for(;;){events.sort((a,b)=>a.time-b.time || a.id-b.id);if(!events.length || events[0].time>time)break;const event=events.shift();now=event.time;event.fn();check(++count<20000,"bounded callbacks");}now=time;},
        fail(){fail=true;},change(){epoch++;current=false;q.cancel();},late(){if(held)held({ok:true,target,snapshot:snapshot(hostGraph,revision+100,height)});},delay(ms){delay=ms;},height(value){height=value;}};
}
let random=20261104;const next=()=>{random=(Math.imul(random,1664525)+1013904223)>>>0;return random;};
{
    const f=fixture();f.enqueue({type:"addNode",nodeType:"model"});f.until(2000);
    const model=f.host.nodes.find(n=>n.type===edits.types.model);model.parameters.find(p=>p.key==="1").value[15]=1;model.parameters.find(p=>p.key==="13").value=1;
    f.enqueue({type:"addNode",nodeType:"force"});f.enqueue({type:"duplicateNodes",nodeIds:[model.id]});f.until(2020);
    check(f.submitted.at(-1).items.length===1 && f.submitted.at(-1).items[0].type==="addNode","mesh copy waits for earlier batch ACK");
    f.until(3150);check(f.submitted.at(-1).items.length===1 && f.submitted.at(-1).items[0].type==="duplicateNodes","imported mesh copy has its own transaction");
    const copied=f.graph.nodes.find(n=>n.type===edits.types.model && n.id!==model.id);f.enqueue({type:"duplicateNodes",nodeIds:[copied.id]});f.until(10000);
    for(const batch of f.submitted)for(const item of batch.items)for(const copy of item.modelResourceCopies || [])check(batch.base.snapshot.nativeNodes.some(n=>n.id===copy.sourceId),"imported copy source exists in confirmed host receipt");
    check(!f.errors.length && !f.q.busy(),"resource dependency batches drain without retry");
}
{
    const f=fixture();f.enqueue({type:"addNode",nodeType:"force"});f.until(20);
    f.enqueue({type:"addNode",nodeType:"emitter"});f.enqueue({type:"addNode",nodeType:"turbulence"});
    const ids=f.graph.nodes.map(n=>n.id).sort();f.height(2160);f.until(1200);
    const batch=f.submitted.at(-1);check(batch.base.snapshot.geometry.height===2160,"next batch uses changed acknowledged geometry");
    const emitter=batch.g.nodes.find(n=>n.type===edits.types.emitter),turb=batch.g.nodes.find(n=>n.type===edits.types.turbulence);
    check(Math.abs(emitter.parameters.find(p=>p.key==="12").value*2160-100)<1e-9,"queued Emitter default stays 100px/s after height change");
    const fresh=edits.apply(initial(),{type:"addNode",nodeType:"turbulence",layerHeightPixels:2160},()=>uuid(1000)).nodes.find(n=>n.type===edits.types.turbulence);
    for(const key of ["2","11"])check(Math.abs(turb.parameters.find(p=>p.key===key).value-fresh.parameters.find(p=>p.key===key).value)<1e-12,"queued Turbulence default rebases to current units");
    f.until(10000);check(JSON.stringify(f.host.nodes.map(n=>n.id).sort())===JSON.stringify(ids),"geometry rebase preserves assigned identities");check(!f.errors.length && !f.q.busy(),"changed geometry completes without retry or admission failure");
}
{
    const f=fixture();f.enqueue({type:"addNode",nodeType:"force"});f.until(2000);const node=f.graph.nodes.find(n=>n.type!==edits.types.output);
    for(let i=0;i<64;i++)check(f.enqueue({type:"connect",from:node.id,to:uuid(255)}).ok,"bounded intent capacity accepts idempotent edits");
    check(f.enqueue({type:"connect",from:node.id,to:uuid(255)}).error.code==="edit_queue_full","backpressure rejects only unaccepted extra intent");
    f.until(100000);check(!f.q.busy() && !f.errors.length,"full queue drains with a single writer");
}
for(const interval of [333,500]){
    const f=fixture();let accepted=0,expectedLimits=0;
    for(let i=0;i<360;i++){f.until(i*interval);const nodes=f.graph.nodes.filter(n=>n.type!==edits.types.output);let edit;
        if(nodes.length<3 || (nodes.length<35 && next()%4===0))edit={type:"addNode",nodeType:["force","particle","auxiliary","transform"][next()%4],position:{x:i,y:10}};
        else {const n=nodes[next()%nodes.length],other=f.graph.nodes[next()%f.graph.nodes.length];switch(next()%4){case 0:edit={type:"deleteNodes",nodeIds:[n.id]};break;case 1:edit={type:"detachNodes",nodeIds:[n.id]};break;case 2:edit={type:"connect",from:n.id,to:other.id};break;default:edit={type:"duplicateNodes",nodeIds:[n.id]};}}
        const response=f.enqueue(edit);if(response.ok)accepted++;else {check(["connection_capacity","edit_queue_full"].includes(response.error.code),response.error.message);expectedLimits++;}
        check(f.graph.nodes.length<=64,"bounded preview node count");
    }
    const expected=codec.toHex(f.graph);f.until(1000000);check(!f.q.busy(),"queue drains");check(f.errors.length===0,"valid pressure has no service failure");check(f.peak===1,"at most one writer");check(codec.toHex(f.host)===expected,"no accepted edit lost after slow/duplicate ACKs");
    check(f.submitted.length<accepted,"coalesces unsent traffic");console.log(`${interval}ms input: ${accepted} accepted, ${f.submitted.length} host batches, ${expectedLimits} explicit capacity limits`);
}
{
    const f=fixture();f.enqueue({type:"addNode",nodeType:"force"});const id=f.graph.nodes.find(n=>n.type!==edits.types.output).id;f.until(20);
    f.enqueue({type:"connect",from:id,to:uuid(255)});f.until(2000);check(f.graph.nodes.some(n=>n.id===id),"stable add identity on replay");f.until(10000);check(f.host.edges.some(e=>e.sourceNode===id),"later connection uses newly confirmed source");
}
{
    const f=fixture();f.fail();f.enqueue({type:"addNode",nodeType:"force"});f.until(20);f.enqueue({type:"addNode",nodeType:"particle"});f.until(10000);
    check(f.errors.length===1 && !f.q.busy() && f.submitted.length===1,"unknown outcome stops all unsent work without retry");f.late();check(f.errors.length===1 && f.submitted.length===1,"late duplicate cannot resume failure");
}
{
    const f=fixture();f.enqueue({type:"addNode",nodeType:"force"});f.until(20);f.change();const before=codec.toHex(f.graph);f.until(10000);f.late();check(codec.toHex(f.graph)===before && f.errors.length===0 && !f.q.busy(),"target cancellation ignores old callback");
}
{
    const g=edits.apply(initial(),{type:"addNode",nodeType:"force"},()=>uuid(1));const s=snapshot(g,7),n=g.nodes.find(n=>n.type!==edits.types.output);
    const connected=edits.apply(g,{type:"connect",from:n.id,to:uuid(255)},()=>uuid(100));const d=queueApi.delta(s,connected,g);check(d && d.removed.length===0 && d.rows.length===1 && d.rows[0].expected.length===0,"link-only native delta");
    const scalar=n.parameters.find(p=>p.type===4);const value=edits.apply(g,{type:"setParameters",changes:[{nodeId:n.id,parameterKey:scalar.key,valueType:4,value:5}]});check(queueApi.delta(s,value,g)===null,"control values retain guarded value transaction");
}
console.log(`${checks} queue checks passed; virtual input clock, no AE timing qualification`);
