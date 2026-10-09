"use strict";
// Isolated unpublished author candidate; no AE, CUSTOM_VALUE, file IO or mesh export.
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=process.env.STARFIELD_PANEL_ROOT;
if(!root)throw new Error("Set STARFIELD_PANEL_ROOT to the isolated Model CEP candidate.");
const load=name=>require(path.join(root,"js",name+".js"));
const codec=load("graph_codec"),edits=load("graph_edits"),view=load("graph_view"),snapshots=load("native_graph_snapshot"),transactions=load("graph_transactions");
const layout=load("graph_layout");
let checks=0,serial=1;
const equal=(a,b)=>{checks++;assert.deepEqual(a,b);},ok=value=>{checks++;assert.ok(value);},reject=(fn,pattern)=>{checks++;assert.throws(fn,pattern);};
const uuid=n=>n.toString(16).padStart(32,"0"),next=()=>uuid(serial++),value=(node,key)=>node.parameters.find(p=>p.key===String(key)).value;
let graph={version:1,nodes:[],edges:[],optionalRecords:[]};
for(const nodeType of ["emitter","particle","model","output"])graph=edits.apply(graph,{type:"addNode",nodeType,position:{x:serial*100,y:serial*50}},next);
const initialPositions=layout.resolve(graph),initialOutput=graph.nodes.find(n=>n.type===edits.types.output);
initialPositions[uuid(255)]=initialPositions[initialOutput.id];delete initialPositions[initialOutput.id];
initialOutput.id=uuid(255);graph=layout.set(graph,initialPositions);
function nativeManifest(graph){const positions=layout.resolve(graph);return graph.nodes.filter(n=>n.type!==edits.types.output).map(n=>({
    id:n.id,type:n.type,schemaVersion:n.schemaVersion,position:positions[n.id],
    parameters:n.parameters.map(p=>({key:p.key,type:p.type,value:p.type===7?Array.from(p.value):p.value})),
    outgoing:graph.edges.filter(e=>e.sourceNode===n.id).map(e=>({id:e.id,target:e.destinationNode}))
}));}
const node=kind=>graph.nodes.find(n=>n.type===edits.types[kind]);
const model=node("model"),particle=node("particle"),emitter=node("emitter"),output=node("output");
equal(model.schemaVersion,1);equal(value(model,13),0);equal(value(model,1),new Uint8Array(16));
equal(Array.from({length:6},(_,i)=>new DataView(value(model,12).buffer).getFloat64(i*8,true)),[-.5,-.5,-.5,.5,.5,.5]);
equal(edits.nativeRecordSchema(model),1);
for(const [from,to,outPort,inPort,allowed] of [
    [emitter,particle,"1","1",true],[model,particle,"1","3",true],[model,particle,"1","1",false],
    [model,output,"1","1",false],[particle,model,"2","1",false],[particle,particle,"2","3",false],
    [emitter,particle,"1","3",false],[model,emitter,"1","2",false]])
    equal(edits.canConnect(graph,from.id,to.id,outPort,inPort),allowed);
for(const [from,to] of [[emitter,particle],[model,particle],[particle,output]])graph=edits.apply(graph,{type:"connect",from:from.id,to:to.id},next);
equal(graph.edges.find(e=>e.sourceNode===model.id).destinationPort,"3");
equal(codec.fromHex(codec.toHex(graph)).edges.length,3);
const projected=view.project(graph).nodes.find(n=>n.id===model.id);
equal(projected.inputPort,null);equal(projected.outputPort,"1");equal(projected.params.length,15);
equal(projected.params.filter(p=>p.graphKey==="4").map(p=>p.label),["Offset X","Offset Y","Offset Z"]);
equal(view.project(graph).nodes.find(n=>n.id===particle.id).modelInputPort,"3");
for(const key of ["1","2","3","12"])reject(()=>edits.apply(graph,{type:"setParameters",changes:[{nodeId:model.id,parameterKey:key,valueType:7,value:[]}]}),/owned by the native import/);

const gateway=fs.readFileSync(path.join(root,"jsx/starfield_gateway.jsx"),"utf8").replace("    function readNativeNode(effect, layer) {",
    "    $.global.modelApi={read:readNativeNode,write:setNodeParameters,validate:validateNodeManifest};\n    function readNativeNode(effect, layer) {");
const global={},context=vm.createContext({$:{global},app:{}});vm.runInContext(gateway,context);
const api=global.modelApi,realm=x=>vm.runInContext("("+JSON.stringify(x)+")",context),copy=x=>JSON.parse(JSON.stringify(x));
let writes=0,arbReads=0;
const properties={};
function add(name,disk,initial=0){const p={name,matchName:"org.starfieldfx.node.model-"+disk,value:initial,numKeys:0,
    setValue(v){writes++;this.value=Array.isArray(v)?v.slice():v;}};properties[name]=properties[p.matchName]=p;}
add("Source",1501,1);add("Mesh Revision",1504,0);
for(const [start,label,initial] of [[5,"Offset",0],[8,"Angle",0],[11,"Scale",100]])for(let axis=0;axis<3;axis++)add(label+" "+"XYZ"[axis],1500+start+axis,initial);
for(const [i,name] of ["Flip X","Flip Y","Flip Z","Center","Normalize"].entries())add(name,1514+i);
for(const [i,name] of ["Mesh Min X","Mesh Min Y","Mesh Min Z","Mesh Max X","Mesh Max Y","Mesh Max Z"].entries())add(name,1519+i,i<3?-.5:.5);
add("Panel Sync Guard",4094);add("Node Layout X",4019);add("Node Layout Y",4020);add("Outgoing Connection Count",4021);
for(let i=0;i<8;i++)add("Node UUID "+i,4100+i,parseInt(model.id.substr(i*4,4),16));
for(let slot=0;slot<4;slot++)for(let chunk=0;chunk<8;chunk++){
    add("Connection "+slot+" Target UUID "+chunk,4200+slot*16+chunk);
    add("Connection "+slot+" Edge UUID "+chunk,4208+slot*16+chunk);
}
const effect={name:"Model",matchName:"org.starfieldfx.node.model",property(key){
    if(key==="Model Mesh" || key==="org.starfieldfx.node.model-1503"){arbReads++;throw new Error("CUSTOM_VALUE must never be read");}
    return properties[key] || null;
}};
const layer={width:1920,height:1080,source:{pixelAspect:1}};
let manifest=nativeManifest(graph),modelRecord=manifest.find(n=>n.id===model.id);
api.write(effect,realm(modelRecord),layer);equal(properties["Panel Sync Guard"].value,0);
let read=copy(api.read(effect,layer));equal(edits.nativeRecordSchema(read),1);equal(read.modelAsset,{revision:0,bounds:[-.5,-.5,-.5,.5,.5,.5]});equal(arbReads,0);
const bounds=[-2.125,-1e9,-.00025,3.75,125000,.875];
properties["Mesh Revision"].value=17;
for(let i=0;i<6;i++)properties[["Mesh Min X","Mesh Min Y","Mesh Min Z","Mesh Max X","Mesh Max Y","Mesh Max Z"][i]].value=bounds[i];
read=copy(api.read(effect,layer));equal(value(read,2),0);equal(read.modelAsset.revision,17);
const changeSource=source=>({type:"setParameters",modelAssets:{[model.id]:read.modelAsset},changes:[{nodeId:model.id,parameterKey:"13",valueType:3,value:source}]});
graph=edits.apply(graph,changeSource(1));
modelRecord=nativeManifest(graph).find(n=>n.id===model.id);
equal(value(modelRecord,2),17);equal(value(modelRecord,1),Array.from(Buffer.from(model.id,"hex")));
api.write(effect,realm(modelRecord),layer);read=copy(api.read(effect,layer));equal(value(read,13),1);
equal(value(read,12),value(modelRecord,12));equal(properties["Source"].value,2);equal(arbReads,0);
for(const [key,v] of [[4,[2,-3,4]],[5,[-120,90,45]],[6,[-100,50,200]],[7,1],[8,1],[9,1],[10,1],[11,1]]){
    value(modelRecord,key);modelRecord.parameters.find(p=>p.key===String(key)).value=v;api.write(effect,realm(modelRecord),layer);
    equal(value(copy(api.read(effect,layer)),key),v);equal(properties["Panel Sync Guard"].value,0);
}
for(const mutation of [r=>{value(r,1)[15]^=1;},r=>{r.parameters.find(p=>p.key==="2").value=18;},r=>{value(r,12)[0]^=1;},
    r=>{r.parameters.find(p=>p.key==="4").value=[1,2];},r=>{r.parameters.push(r.parameters[0]);}]){
    const bad=copy(modelRecord);mutation(bad);const before=writes;reject(()=>api.write(effect,realm(bad),layer),/Model|geometry|resource/);equal(writes,before);
}
graph=edits.apply(graph,changeSource(0));modelRecord=nativeManifest(graph).find(n=>n.id===model.id);
api.write(effect,realm(modelRecord),layer);read=copy(api.read(effect,layer));equal(value(read,2),0);equal(read.modelAsset.revision,17);equal(read.modelAsset.bounds,bounds);

const renderer={id:output.id,position:{x:400,y:250},maxParticles:1000000};
function response(){return {ok:true,target:{token:"pinned"},snapshot:{initialized:true,revision:serial,checksum:"00000000",
    nativeNodes:manifest.map(n=>n.id===model.id?copy(api.read(effect,layer)):copy(n)),renderer,geometry:{height:1080,width:1920,pixelAspect:1}}};}
const normalized=snapshots.normalize(response());equal(normalized.ok,true);
const snapshotGraph=codec.fromHex(normalized.snapshot.graphHex);equal(snapshotGraph.edges.find(e=>e.sourceNode===model.id).destinationPort,"3");
let submitted=0,result;
const client=transactions.create({codec,edits,idFactory:next,call(operation,request,callback){
    equal(operation,"submitGraph");submitted++;api.write(effect,realm(request.nodeManifest.find(n=>n.id===model.id)),layer);serial++;callback(response());
}});
client.apply({type:"setParameters",changes:[{nodeId:model.id,parameterKey:"13",valueType:3,value:1}]},r=>{result=r;},"pinned",serial,response());
equal(submitted,1);equal(result.ok,true);equal(value(copy(api.read(effect,layer)),2),17);equal(arbReads,0);
const deleted=edits.apply(graph,{type:"deleteNodes",nodeIds:[model.id]});equal(deleted.edges.length,2);ok(deleted.nodes.every(n=>n.id!==model.id));
console.log(`model_panel_author_tests: ${checks} checks passed; isolated CEP candidate; AE2023 qualification remains open`);
