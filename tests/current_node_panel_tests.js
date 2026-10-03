"use strict";
const assert=require("node:assert/strict"), fs=require("node:fs"), vm=require("node:vm"), path=require("node:path");
const edits=require("../cep_panel/js/graph_edits.js"), view=require("../cep_panel/js/graph_view.js");
const snapshot=require("../cep_panel/js/native_graph_snapshot.js"), codec=require("../cep_panel/js/graph_codec.js");
const uuid=n=>n.toString(16).padStart(32,"0");let next=10;
const output={id:uuid(255),type:edits.types.output,schemaVersion:4,parameters:[{key:"1",type:3,value:1000000},{key:"7",type:3,value:30}]};
assert.equal(edits.types.appearance,undefined,"Appearance is not registered as a node");
assert.ok(!fs.readFileSync(path.join(__dirname,"../cep_panel/index.html"),"utf8").includes('data-node-type="appearance"'),"Appearance has no creation action");
let graph={version:1,nodes:[output],edges:[],optionalRecords:[]};
const add=kind=>{graph=edits.apply(graph,{type:"addNode",nodeType:kind},()=>uuid(next++));return graph.nodes[graph.nodes.length-1];};
const emitter=add("emitter"), particle=add("particle"), auxiliary=add("auxiliary"), child=add("particle");
assert.equal(auxiliary.schemaVersion,6);
assert.equal(emitter.parameters.find(p=>p.key==="17").value,1,"Uniform is the default");
assert.equal(auxiliary.parameters.find(p=>p.key==="23").value,0,"Auxiliary uses independent Emitting timing");
assert.deepEqual(view.project(graph,null,{width:3840,height:2160,pixelAspect:1}).nodes.find(n=>n.id===particle.id).params.slice(0,5).map(p=>p.label),
    ["Shape","Life (Seconds)","Life Random","Size (Pixels)","Size Y (Pixels)"]);
assert.equal(view.parameterToGraphValue(view.project(graph,null).nodes.find(n=>n.kind==="output").params.find(p=>p.graphKey==="7"),3),120);
assert.equal(auxiliary.parameters.find(p=>p.key==="31").value,1);
for(const [from,to] of [[emitter.id,particle.id],[particle.id,auxiliary.id],[auxiliary.id,child.id],[child.id,output.id]])
    graph=edits.apply(graph,{type:"connect",from,to},()=>uuid(next++));
assert.equal(graph.edges.find(e=>e.destinationNode===auxiliary.id).destinationPort,"2");
assert.throws(()=>edits.apply(graph,{type:"connect",from:particle.id,to:emitter.id},()=>uuid(next++)),/Auxiliary/);
assert.throws(()=>edits.apply(graph,{type:"connect",from:child.id,to:auxiliary.id},()=>uuid(next++)),/cycle/);
const projected=view.project(graph,null,{width:3840,height:2160,pixelAspect:1});
const withForce=edits.apply(graph,{type:"addNode",nodeType:"force"},()=>uuid(next++));
const forceView=view.project(withForce,null,{width:3840,height:2160,pixelAspect:1}).nodes.find(n=>n.kind==="force");
assert.deepEqual(forceView.params.map(p=>p.label),["Gravity","Gravity random","Wind X","Wind Y","Wind Z","Spin","Spin Frequency","Spin resist","Spin Delay (Seconds)","Air Density"]);
assert.equal(forceView.params[0].kind,"slider");
assert.deepEqual(view.parameterToGraphValue(forceView.params[0],2160),[0,-1,0]);
assert.deepEqual(view.parameterToGraphValue(forceView.params[2],1080),[.5,0,0]);
assert.deepEqual(view.parameterToGraphValue(forceView.params[3],1080),[0,-.5,0]);
const wideForce=view.project(withForce,null,{width:3840,height:2160,pixelAspect:2}).nodes.find(n=>n.kind==="force");
assert.deepEqual(view.parameterToGraphValue(wideForce.params[2],1080),[1,0,0],"Wind X converts rectangular pixels independently");
assert.equal(forceView.params.find(p=>p.label==="Spin Delay (Seconds)").scrubStep,.1);
assert.equal(forceView.params.find(p=>p.label==="Spin Frequency").scrubStep,.01);
assert.equal(forceView.curveParameterKeys.size,"9");
assert.deepEqual(forceView.curves.size.points,[{age:0,value:100},{age:1,value:100}]);
const manifest=JSON.parse(fs.readFileSync(path.join(__dirname,"../schema/parameters.json")));
const unique=(values,label)=>assert.equal(new Set(values).size,values.length,label+" must be unique");
unique(manifest.parameters.map(p=>p.id),"Main public stream IDs");
unique(manifest.parameters.map(p=>p.key),"Main public parameter keys");
const binding=manifest.nativeRenderBindings;
const registeredIndices=manifest.parameters.map(p=>p.id).concat(manifest.topics.flatMap(t=>[t.index,t.endIndex]),
    Array.from({length:binding.count},(_,i)=>binding.firstIndex+i));
unique(registeredIndices,"Main registered indices including topics and aliases");
assert.deepEqual(registeredIndices.slice().sort((a,b)=>a-b),Array.from({length:615},(_,i)=>i+1),
    "The manifest describes each current main parameter exactly once");
unique(manifest.parameters.map(p=>p.diskId===undefined?p.id:p.diskId).concat(
    manifest.topics.flatMap(t=>[t.diskId,t.endDiskId]),Array.from({length:binding.count},(_,i)=>binding.firstDiskId+i)),
    "Main disk IDs including topics and aliases");
assert.equal(manifest.parameters.find(p=>p.key==="particle_count").default,1000000);
const acceleration=manifest.parameters.find(p=>p.key==="acceleration"), sampling=manifest.parameters.find(p=>p.key==="time_sampling_hz");
assert.deepEqual([acceleration.id,acceleration.diskId,acceleration.default],[614,1611,1]);
assert.deepEqual(acceleration.choices,["GPU","CPU"]);assert.deepEqual(acceleration.values,[0,1]);
assert.deepEqual([sampling.id,sampling.diskId,sampling.default],[611,1601,1]);
assert.deepEqual(sampling.values,[30,60,120]);
const auxView=projected.nodes.find(n=>n.id===auxiliary.id), emitterView=projected.nodes.find(n=>n.id===emitter.id);
assert.equal(auxView.label,"Auxiliary");assert.equal(auxView.inputPort,"2");assert.equal(emitterView.inputPort,null);
assert.deepEqual(auxView.params.slice(0,7).map(p=>p.label),["Type","Emitting","Particles Per Second","Origin XY","Origin Z","Speed","Speed Random"]);
assert.equal(emitterView.params.some(p=>p.label.startsWith("Inherit")),false);
assert.equal(auxView.params.find(p=>p.label==="Emit Chance").scrubStep,1);
assert.equal(projected.nodes.find(n=>n.id===particle.id).params.find(p=>p.graphKey==="11").scrubStep,.1);
assert.equal(view.activeEmitterParameters(graph),null,"never present an inaccurate auxiliary live count");
const changed=edits.apply(graph,{type:"setParameters",changes:[{nodeId:auxiliary.id,parameterKey:"31",valueType:3,value:0}]});
assert.equal(changed.edges.some(e=>e.destinationNode===auxiliary.id),false,"disabling Auxiliary source disconnects parent stream in same transaction");
const transactions=require("../cep_panel/js/graph_transactions.js");
const base={initialized:true,graphHex:codec.toHex(graph),geometry:{width:3840,height:2160,pixelAspect:1},revision:1};
let addedGraph,transactionResult;
const client=transactions.create({codec,edits,idFactory:()=>uuid(next++),call(operation,fields,callback){
    if(operation==="getGraphSnapshot") callback({ok:true,snapshot:base});
    else {
        addedGraph=codec.fromHex(fields.graphHex);
        callback({ok:true,snapshot:{initialized:true,revision:2,graphHex:fields.graphHex}});
    }
}});
client.apply({type:"addNode",nodeType:"emitter"},result=>{transactionResult=result;});
assert.equal(transactionResult.ok,true,"transaction confirms graph acknowledgement");
const addedEmitter=addedGraph.nodes.find(n=>!graph.nodes.some(existing=>existing.id===n.id));
assert.equal(addedEmitter.parameters.find(p=>p.key==="12").value,100/2160,"new speed is 100 pixels/s in current layer geometry");
const response={ok:true,snapshot:{initialized:true,revision:1,checksum:"0",nativeNodes:graph.nodes.filter(n=>n.type!==edits.types.output).map(n=>({
    id:n.id,type:n.type,schemaVersion:n.schemaVersion,parameters:n.parameters,position:{x:0,y:0},
    outgoing:graph.edges.filter(e=>e.sourceNode===n.id).map(e=>({id:e.id,target:e.destinationNode}))
})),renderer:{id:output.id,maxParticles:1000,position:{x:0,y:0}}}};
const decoded=codec.fromHex(snapshot.normalize(response).snapshot.graphHex);
assert.equal(decoded.edges.find(e=>e.destinationNode===auxiliary.id).destinationPort,"2");

// Minimal fake host for UUID-resolved transient Effect Controls selection.
let writes=0,undo=0;
function effect(matchName,id) {
    const props={}; if(id) for(let i=0;i<8;i++) props["Node UUID "+i]={name:"Node UUID "+i,value:parseInt(id.slice(i*4,i*4+4),16)};
    return {matchName,name:"Same Name",selected:false,props,property(key){return props[key]||null;}};
}
const main=effect("org.starfieldfx.particle"), first=effect("org.starfieldfx.node.emitter",emitter.id), second=effect("org.starfieldfx.node.emitter",auxiliary.id);
for(const [index,name] of [[39,"Graph Revision"],[40,"Panel Graph Sync Guard"],[41,"Commit Graph Edit"],[42,"Graph Edit Receipt"],[87,"Node Effects Ready"],[88,"Graph Checksum High"],[89,"Graph Checksum Low"]]) {
    main.props[name]=main.props[index]={name,propertyIndex:index,value:0,setValue(){writes++;}};
}
const items=[main,first,second], previouslySelected={selected:true};
const layer={id:29,name:"Layer",selected:true,selectedProperties:[previouslySelected],property(){return {numProperties:items.length,property:i=>items[i-1]};}};
function CompItem(){}const comp=new CompItem();Object.assign(comp,{id:17,name:"Comp",numLayers:1,layer:()=>layer});
const host={};vm.runInNewContext(fs.readFileSync(path.join(__dirname,"../cep_panel/jsx/starfield_gateway.jsx"),"utf8"),{$:{global:host},CompItem,
    app:{project:{activeItem:comp,rootFolder:{id:5}},beginUndoGroup(){undo++;},endUndoGroup(){undo++;}}});
const select=(nodeId,token="p5-c17-l29")=>JSON.parse(host.SFLD_selectNodeEffect(JSON.stringify({protocol:"org.starfieldfx.panel",version:1,operation:"selectNodeEffect",nodeId,target:{token}})));
assert.equal(select(auxiliary.id).ok,true);assert.equal(second.selected,true);assert.equal(first.selected,false);
assert.equal(previouslySelected.selected,false);assert.equal(select(output.id).ok,true);assert.equal(main.selected,true);
items.reverse();assert.equal(select(emitter.id).ok,true,"effect reorder keeps UUID selection");
assert.equal(select(uuid(100)).error.code,"missing_node");assert.equal(select(emitter.id,"stale").error.code,"stale_target");
assert.equal(writes,0);assert.equal(undo,0);
console.log("Current node panel interaction checks passed.");
