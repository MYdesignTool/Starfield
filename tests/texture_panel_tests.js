"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"../cep_panel");
const edits=require(path.join(root,"js/graph_edits.js")),view=require(path.join(root,"js/graph_view.js"));
const presets=require(path.join(root,"js/presets.js")),codec=require(path.join(root,"js/graph_codec.js"));
let checks=0;const equal=(a,b)=>{checks++;assert.deepEqual(a,b);},ok=x=>{checks++;assert.ok(x);},reject=(f,re)=>{checks++;assert.throws(f,re);};
let sequence=500;const next=()=>String(sequence++).padStart(32,"0"),clone=x=>JSON.parse(JSON.stringify(x));
let graph=presets.build("sparks",1080),particle=graph.nodes.find(n=>n.type===edits.types.particle);
function set(key,value){graph=edits.apply(graph,{type:"setParameters",changes:[{nodeId:particle.id,parameterKey:String(key),valueType:3,value}]});particle=graph.nodes.find(n=>n.id===particle.id);}
set(15,3);set(31,55);set(32,66);set(33,7);set(34,2);set(35,0);set(36,1);
const resources=[{id:55,name:"Front",sourceName:"Clip A",texture:true},{id:66,name:"Back",sourceName:"Clip B",texture:true},
    {id:77,name:"Null",sourceName:"",texture:false}];
let projected=view.project(graph,null,{height:1080,width:1920,pixelAspect:1},resources).nodes.find(n=>n.id===particle.id);
const fields=projected.params.filter(p=>Number(p.graphKey)>=31);
equal(fields.map(p=>p.label),["Texture Layer","Dark Side","Texture Time Sample","Texture Color Use","Use Texture Ratio","Ignore Perspective"]);
equal(fields[0].enumValues,[0,55,66]);equal(fields[0].value,2);equal(view.parameterToGraphValue(fields[1],2),55);
equal(fields[2].choices.length,8);equal(fields[2].value,8);equal(fields[3].value,3);
equal(codec.fromHex(codec.toHex(graph)).nodes.find(n=>n.id===particle.id).parameters.find(p=>p.key==="31").value,55);
const old=clone(graph);old.nodes.find(n=>n.id===particle.id).parameters=old.nodes.find(n=>n.id===particle.id).parameters.filter(p=>Number(p.key)<30);
equal(edits.nativeRecordSchema(old.nodes.find(n=>n.id===particle.id)),7);
equal(edits.apply(old,{type:"setParameters",changes:[{nodeId:particle.id,parameterKey:"31",valueType:3,value:55}]}).nodes.find(n=>n.id===particle.id).parameters.find(p=>p.key==="31").value,55);
const text=presets.encode(graph,"Texture preset","My Presets",resources),data=JSON.parse(text),entry=presets.decode(text);
equal(data.version,2);equal(data.resources.length,2);equal(entry.graph.nodes.find(n=>n.id===particle.id).parameters.find(p=>p.key==="31").value,0);
const moved=resources.map(r=>({...r,id:r.id+100}));
const replaced=presets.apply(presets.build("orbit",1080),{type:"applyPreset",presetGraph:entry.graph,presetResources:entry.resources,
    layerResources:moved,mode:"replace",applyRenderSettings:true},next);
equal(replaced.nodes.find(n=>n.type===edits.types.particle).parameters.find(p=>p.key==="31").value,155);
const added=presets.apply(graph,{type:"applyPreset",presetGraph:entry.graph,presetResources:entry.resources,layerResources:moved,
    mode:"add",applyRenderSettings:false},next);
equal(added.nodes.filter(n=>n.type===edits.types.output).length,1);
equal(added.nodes.find(n=>n.id===particle.id).parameters.find(p=>p.key==="31").value,55);
ok(added.nodes.some(n=>n.type===edits.types.particle && n.parameters.some(p=>p.key==="31" && p.value===155)));
reject(()=>presets.apply(graph,{type:"applyPreset",presetGraph:entry.graph,presetResources:entry.resources,layerResources:[],mode:"add"},next),/one matching/);
reject(()=>presets.apply(graph,{type:"applyPreset",presetGraph:entry.graph,presetResources:entry.resources,layerResources:[...moved,moved[0]],mode:"add"},next),/one matching/);
reject(()=>presets.encode(graph,"Missing",null,[]),/metadata is unavailable/);
const malformed=clone(data);malformed.resources.push(malformed.resources[0]);reject(()=>presets.decode(JSON.stringify(malformed)),/Invalid preset texture/);
const unsafe=clone(data);unsafe.version=1;unsafe.graphHex=codec.toHex(graph);reject(()=>presets.decode(JSON.stringify(unsafe)),/resource map/);
const source=fs.readFileSync(path.join(root,"jsx/starfield_gateway.jsx"),"utf8").replace("    function readNativeNode(effect, layer) {",
    "    $.global.textureApi={read:readNativeNode,write:setNodeParameters,clear:function(){nativePropertyIndexes=[];nativeLayerInventory=null;}};\n    function readNativeNode(effect, layer) {");
const global={},context=vm.createContext({$:{global},app:{}});vm.runInContext(source,context);
const api=global.textureApi,realm=x=>vm.runInContext("("+JSON.stringify(x)+")",context);
const props=require("./node_property_fixture.js").nodeControls();
for(const property of new Set(Object.values(props))){property.numKeys=0;property.setValue=function(value){this.value=value;};}
const layer={id:9,width:1920,height:1080,source:{name:"Renderer",pixelAspect:1},hasVideo:true,property(){return {property(){return {};}};}};
const sources=[{...layer,id:55,name:"Front",source:{name:"Clip A"}},{...layer,id:66,name:"Back",source:{name:"Clip B"}},layer];
layer.containingComp={numLayers:3,layer(index){return sources[index-1];}};
const effect={name:"Particle",matchName:"org.starfieldfx.node.particle",property(name){return props[name] || null;}};
let record={id:particle.id,type:particle.type,schemaVersion:particle.schemaVersion,
    parameters:particle.parameters.map(p=>({...p,value:p.type===7?Array.from(p.value):clone(p.value)})),position:{x:100,y:200},outgoing:[]};api.write(effect,realm(record),layer);
equal(props["org.starfieldfx.node.particle-233"].value,1);equal(props["org.starfieldfx.node.particle-234"].value,2);
let read=clone(api.read(effect,layer));
for(let key=31;key<=36;key++)equal(read.parameters.find(p=>p.key===String(key)).value,particle.parameters.find(p=>p.key===String(key)).value);
sources.reverse();api.clear();api.write(effect,realm(record),layer);equal(props["org.starfieldfx.node.particle-233"].value,3);
equal(clone(api.read(effect,layer)).parameters.find(p=>p.key==="31").value,55);
record.parameters.find(p=>p.key==="31").value=9;reject(()=>api.write(effect,realm(record),layer),/other than its renderer/);equal(props["Panel Sync Guard"].value,0);
record.parameters.find(p=>p.key==="31").value=999;api.clear();reject(()=>api.write(effect,realm(record),layer),/no longer exists/);equal(props["Panel Sync Guard"].value,0);
console.log(`texture_panel_tests: ${checks} checks passed; AE2023 qualification remains open`);
