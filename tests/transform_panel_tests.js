"use strict";
// Ordinary authoring and UI tests; actual AE Null/render qualification is separate.
const fs=require("node:fs"),path=require("node:path"),vm=require("node:vm"),assert=require("node:assert/strict");
const panelRoot=process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"..","cep_panel");
const edits=require(path.join(panelRoot,"js/graph_edits.js")),view=require(path.join(panelRoot,"js/graph_view.js"));
const codec=require(path.join(panelRoot,"js/graph_codec.js")),layout=require(path.join(panelRoot,"js/graph_layout.js"));
const snapshots=require(path.join(panelRoot,"js/native_graph_snapshot.js")),transactions=require(path.join(panelRoot,"js/graph_transactions.js"));
const presets=require(path.join(panelRoot,"js/presets.js"));
let checks=0;function equal(a,b){checks++;assert.deepEqual(a,b);}function ok(x){checks++;assert.ok(x);}function rejects(f,re){checks++;assert.throws(f,re);}
const id=n=>n.toString(16).padStart(32,"0"),plain=x=>JSON.parse(JSON.stringify(x));
let serial=1;const next=()=>id(serial++);
let graph=presets.build("sparks",1080);
graph=edits.apply(graph,{type:"addNode",nodeType:"transform",position:{x:400,y:200}},next);
let transform=graph.nodes.at(-1);
equal(transform.type,edits.types.transform);equal(transform.schemaVersion,1);equal(transform.parameters.length,7);
graph=edits.apply(graph,{type:"disconnect",from:graph.nodes[2].id,to:graph.nodes[3].id});
graph=edits.apply(graph,{type:"connect",from:graph.nodes[2].id,to:transform.id},next);
graph=edits.apply(graph,{type:"connect",from:transform.id,to:graph.nodes[3].id},next);
ok(edits.canConnect(graph,transform.id,graph.nodes[1].id,"2","1")===false); // Would form a cycle.
const geometry={width:1920,height:1080,pixelAspect:2},resources=[{id:77,name:"Null <one>"},{id:88,name:"Null two"}];
function projected(g=graph){return view.project(g,null,geometry,resources).nodes.find(n=>n.id===transform.id);}
let node=projected();
equal(node.params.map(p=>p.label),["Inherit Motion (Null Layer)","Anchor XY","Anchor Z","Position X","Position Y","Position Z",
    "Rotation X","Rotation Y","Rotation Z","Scale X","Scale Y","Scale Z","Particles Scale","Particles Opacity"]);
equal(node.params[1].value,[960,540]);equal(node.params[2].value,0);equal(node.params[0].enumValues,[0,77,88]);
equal(view.parameterToGraphValue(node.params[0],2),77);
for(const parameter of node.params.slice(1)) {
    const input=parameter.label==="Anchor XY"?[1200,400]:parameter.label.startsWith("Scale")?-50:
        parameter.label==="Particles Opacity"?40:parameter.label==="Particles Scale"?200:25;
    const value=view.parameterToGraphValue(parameter,input);
    const altered=edits.apply(graph,{type:"setParameters",changes:[{nodeId:transform.id,parameterKey:parameter.graphKey,valueType:parameter.graphType,value}]});
    const roundtrip=projected(altered).params.find(p=>p.key===parameter.key).value;
    if(Array.isArray(input))equal(roundtrip,input);else {checks++;assert.ok(Math.abs(roundtrip-input)<1e-9);}
}
rejects(()=>edits.apply(graph,{type:"setParameters",changes:[{nodeId:transform.id,parameterKey:"8",valueType:3,value:-1}]}),/available layer/);
rejects(()=>edits.apply(graph,{type:"setParameters",changes:[{nodeId:transform.id,parameterKey:"7",valueType:7,value:new Uint8Array(132)}]}),/sampled/);
const graphWithNull=edits.apply(graph,{type:"setParameters",changes:[{nodeId:transform.id,parameterKey:"8",valueType:3,value:77}]});
equal(projected(graphWithNull).params[0].value,2);
equal(view.project(graphWithNull,null,geometry,[]).nodes.find(n=>n.id===transform.id).params[0].choices,["None","Unavailable layer (77)"]);
const optional=codec.fromHex(codec.toHex(graph)),optionalTransform=optional.nodes.find(n=>n.id===transform.id);
optionalTransform.parameters=optionalTransform.parameters.filter(p=>p.key!=="8");
equal(projected(optional).params[0].enumValues,[0,77,88]);
equal(edits.apply(optional,{type:"setParameters",changes:[{nodeId:transform.id,parameterKey:"8",valueType:3,value:77}]}).nodes.find(n=>n.id===transform.id).parameters.at(-1).value,77);
const exported=presets.decode(presets.encode(graph,"Transform setup"));equal(exported.graph.nodes.find(n=>n.id===transform.id).type,edits.types.transform);
equal(exported.graph.nodes.find(n=>n.id===transform.id).parameters,transform.parameters);
const tfPresetAdd=presets.apply(presets.build("orbit",1080),{type:"applyPreset",presetGraph:exported.graph,mode:"add",applyRenderSettings:false},next);
equal(tfPresetAdd.nodes.filter(n=>n.type===edits.types.transform).length,1);
const tfPresetReplace=presets.apply(presets.build("orbit",1080),{type:"applyPreset",presetGraph:exported.graph,mode:"replace",applyRenderSettings:false},next);
equal(tfPresetReplace.nodes.filter(n=>n.type===edits.types.transform).length,1);
equal(tfPresetReplace.nodes.length,graph.nodes.length);
rejects(()=>presets.encode(graphWithNull,"Unsafe binding"),/Set Inherit Motion to None/);
rejects(()=>presets.decode(JSON.stringify({format:"org.starfieldfx.preset",version:1,name:"Foreign ID",graphHex:codec.toHex(graphWithNull)})),/Set Inherit Motion to None/);
const combined=presets.apply(graphWithNull,{type:"applyPreset",presetId:"orbit",mode:"add",applyRenderSettings:false,layerHeightPixels:1080},next);
equal(combined.nodes.find(n=>n.id===transform.id).parameters.find(p=>p.key==="8").value,77);
const gatewaySource=fs.readFileSync(path.join(panelRoot,"jsx/starfield_gateway.jsx"),"utf8").replace("    function readNativeNode(effect, layer) {",
    "    $.global.tf={read:readNativeNode,write:setNodeParameters,validate:validateNodeManifest,resources:validateTransformResources,index:layerResourceIndex,inventory:layerInventory,clear:function(){nativePropertyIndexes=[];nativeLayerInventory=null;}};\n    function readNativeNode(effect, layer) {");
const global={},context=vm.createContext({$:{global},app:{}});vm.runInContext(gatewaySource,context);
const api=global.tf,realm=x=>vm.runInContext("("+JSON.stringify(x)+")",context);
const labels=["Inherit Motion (Null Layer)","Anchor XY","Anchor Z","Position X","Position Y","Position Z","Rotation X","Rotation Y","Rotation Z",
    "Scale X","Scale Y","Scale Z","Particles Scale","Particles Opacity"];
const layer={...geometry,source:{pixelAspect:2}},comp={time:2,numLayers:3};layer.containingComp=comp;
let layers=[{id:77,name:"Null <one>"},{id:88,name:"Null two"},{id:99,name:"Camera",camera:true}];
for(const l of layers)l.property=()=>({property:()=>l.camera?null:{value:[0,0,0]}});
comp.layer=i=>{if(!layers[i-1])throw Error("Missing layer index");return layers[i-1];};
const props={},writes=[];
function control(name,value,matchName){const p={name,value,matchName,numKeys:0,setValue(v){writes.push(name);this.value=v;},setValueAtTime(time,v){writes.push([name,time]);this.value=v;}};props[name]=p;if(matchName)props[matchName]=p;return p;}
labels.forEach((label,i)=>control(label,i===1?[960,540]:i>=9?100:0,"org.starfieldfx.node.transform-"+(1401+i)));
for(const [label,value] of [["Panel Sync Guard",0],["Node Layout X",400],["Node Layout Y",200],["Outgoing Connection Count",0]])control(label,value);
for(let i=0;i<8;i++)control("Node UUID "+i,i===7?parseInt(transform.id.slice(-4),16):0);
for(let slot=0;slot<4;slot++)for(let word=0;word<8;word++){control(`Connection ${slot} Target UUID ${word}`,0);control(`Connection ${slot} Edge UUID ${word}`,0);}
const effect={matchName:"org.starfieldfx.node.transform",name:"Transform",property:name=>props[name]||null,propertyGroup:()=>layer};
const first=plain(api.read(effect,layer));equal(first.parameters,transform.parameters);equal(edits.nativeRecordSchema(first),1);
api.clear();equal(plain(api.inventory(layer).entries).map(x=>x.id),[77,88]);equal(api.index(layer,77),1);
layers=[layers[1],layers[0],layers[2]];api.clear();equal(api.index(layer,77),2);
let authored=plain(first);authored.parameters.find(p=>p.key==="8").value=77;
authored.parameters.find(p=>p.key==="1").value=[.2,-.3,.4];authored.parameters.find(p=>p.key==="2").value=[.1,.2,.3];
authored.parameters.find(p=>p.key==="3").value=[10,20,30];authored.parameters.find(p=>p.key==="4").value=[-50,0,250];
authored.parameters.find(p=>p.key==="5").value=200;authored.parameters.find(p=>p.key==="6").value=40;
api.validate(realm([authored]));api.resources(realm([authored]),layer);api.write(effect,realm(authored),layer);
equal(props[labels[0]].value,2);equal(props["Position X"].value,54);equal(props["Position Y"].value,-216);equal(props["Position Z"].value,324);
equal(props["Rotation X"].value,-10);equal(props["Rotation Y"].value,20);equal(props["Rotation Z"].value,-30);
const captured=plain(api.read(effect,layer));for(const p of captured.parameters){const expected=authored.parameters.find(x=>x.key===p.key);
    equal(p.type,expected.type);const values=Array.isArray(p.value)?p.value:[p.value],reference=Array.isArray(expected.value)?expected.value:[expected.value];
    equal(values.length,reference.length);values.forEach((v,i)=>{checks++;assert.ok(Math.abs(v-reference[i])<1e-9);});}
equal(props["Panel Sync Guard"].value,0);ok(!captured.parameters.some(p=>p.key==="7"));
props["Position X"].numKeys=2;authored.parameters.find(p=>p.key==="2").value[0]=.2;
api.write(effect,realm(authored),layer);ok(writes.some(x=>Array.isArray(x)&&x[0]==="Position X"&&x[1]===2));
const bad=plain(authored);bad.parameters.find(p=>p.key==="8").value=999;
rejects(()=>api.resources(realm([bad]),layer),/no longer exists/);
const bigPoint=plain(authored);bigPoint.parameters.find(p=>p.key==="1").value=[100,0,0];
rejects(()=>api.resources(realm([bigPoint]),layer),/native point range/);
const bigAngle=plain(authored);bigAngle.parameters.find(p=>p.key==="3").value=[-32768,0,0];
rejects(()=>api.resources(realm([bigAngle]),layer),/native angle range/);
bad.parameters.find(p=>p.key==="8").value=77;bad.parameters.find(p=>p.key==="4").value=[10001,0,0];
rejects(()=>api.validate(realm([bad])),/out of range/);
bad.parameters.find(p=>p.key==="4").value=[1,1,1];bad.parameters.push({key:"7",type:7,value:[]});
rejects(()=>api.validate(realm([bad])),/unsupported/);
// A real ordinary-record snapshot/ack keeps the Null ID, rather than comparing
// a stale sampled matrix or persisting layer indices in the portable graph.
let nodeManifest;
const tfGraph=layout.set({version:1,nodes:[transform,graph.nodes[3]],edges:[],optionalRecords:[]},{[transform.id]:{x:400,y:200},[graph.nodes[3].id]:{x:500,y:300}});
function snapshot(g,revision){const positions=layout.resolve(g);return {initialized:true,revision,recordStamp:"stamp",geometry,layerResources:resources,
    nativeNodes:g.nodes.filter(n=>n.type!==edits.types.output).map(n=>({...plain(n),position:positions[n.id],outgoing:[]})),
    renderer:{id:graph.nodes[3].id,position:positions[graph.nodes[3].id],maxParticles:1000000}};}
const client=transactions.create({codec,edits,call(operation,fields,callback){
    if(operation==="getGraphSnapshot")callback({ok:true,target:{token:"target"},snapshot:snapshot(tfGraph,1)});
    else{nodeManifest=fields.nodeManifest;const updated=codec.fromHex(fields.graphHex);callback({ok:true,target:{token:"target"},snapshot:snapshot(updated,2)});}
}});
let outcome;client.apply({type:"setParameters",changes:[{nodeId:transform.id,parameterKey:"8",valueType:3,value:77}]},response=>outcome=response,"target");
ok(outcome.ok);equal(nodeManifest[0].parameters.find(p=>p.key==="8").value,77);ok(!nodeManifest[0].parameters.some(p=>p.key==="7"));
equal(snapshots.normalize({ok:true,snapshot:snapshot(tfGraph,1)}).snapshot.layerResources,resources);
console.log(`transform_panel_tests: ${checks} checks passed; host qualification remains open`);
