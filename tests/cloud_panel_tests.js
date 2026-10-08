"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"../cep_panel");
const edits=require(path.join(root,"js/graph_edits.js")),view=require(path.join(root,"js/graph_view.js"));
const presets=require(path.join(root,"js/presets.js")),codec=require(path.join(root,"js/graph_codec.js"));
let checks=0;const equal=(a,b)=>{checks++;assert.deepEqual(a,b);},ok=x=>{checks++;assert.ok(x);},reject=(f,re)=>{checks++;assert.throws(f,re);};
let sequence=800;const next=()=>String(sequence++).padStart(32,"0"),clone=x=>JSON.parse(JSON.stringify(x));
const particle=g=>g.nodes.find(n=>n.type===edits.types.particle),value=(n,k)=>n.parameters.find(p=>p.key===String(k))?.value;
function change(g,key,v,type=key===37 || key===15?3:4){return edits.apply(g,{type:"setParameters",changes:[{nodeId:particle(g).id,parameterKey:String(key),valueType:type,value:v}]});}
const fresh=presets.build("sparks",1080);
equal([37,38,39].map(k=>value(particle(fresh),k)),[10,150,66]);
let graph=change(fresh,15,2);graph=change(graph,39,1000);graph=change(graph,37,33);graph=change(graph,38,225.5);
equal(edits.nativeRecordSchema(particle(graph)),7);
let projected=view.project(graph,null,{height:1080,width:1920,pixelAspect:1}).nodes.find(n=>n.id===particle(graph).id);
equal(projected.params.filter(p=>Number(p.graphKey)>=37).map(p=>[p.label,p.value,p.min,p.max]),
    [["Circles",33,1,1000],["Aspect",225.5,1,1000],["Density",1000,0,1000]]);
ok(!projected.params.some(p=>p.graphKey==="16" || (Number(p.graphKey)>=31 && Number(p.graphKey)<=36)));
for(const shape of [0,1,3])equal(view.project(change(graph,15,shape),null,{height:1080,width:1920,pixelAspect:1}).nodes
    .find(n=>n.id===particle(graph).id).params.filter(p=>Number(p.graphKey)>=37).length,0);
for(const [key,v,type] of [[37,0,3],[37,1001,3],[37,2.2,3],[37,3,4],[38,0,4],[38,1001,4],[39,-1,4],[39,1001,4],[39,1,3],[39,NaN,4],[39,Infinity,4]])
    reject(()=>change(graph,key,v,type),/Invalid Cloud|finite|unsupported/i);
const old=codec.fromHex(codec.toHex(graph));particle(old).parameters=particle(old).parameters.filter(p=>Number(p.key)<37);
equal(edits.nativeRecordSchema(particle(old)),7);
const oldText=codec.toHex(old);
projected=view.project(old,null,{height:1080,width:1920,pixelAspect:1}).nodes.find(n=>n.id===particle(old).id);
equal(projected.params.filter(p=>Number(p.graphKey)>=37).map(p=>p.value),[10,150,66]);equal(codec.toHex(old),oldText);
equal(particle(change(old,3,20)).parameters.filter(p=>Number(p.key)>=37).length,0);
equal([37,38,39].map(k=>value(particle(change(old,15,2)),k)),[10,150,66]);
equal(value(particle(change(old,39,0)),39),0);
equal([37,38,39].map(k=>value(particle(codec.fromHex(codec.toHex(graph))),k)),[33,225.5,1000]);
const entry=presets.decode(presets.encode(graph,"Cloud 1000","My Presets"));
for(const mode of ["add","replace"]) {
    const applied=presets.apply(fresh,{type:"applyPreset",presetGraph:entry.graph,presetResources:entry.resources,
        layerResources:[],mode,applyRenderSettings:true},next);
    equal(applied.nodes.filter(n=>n.type===edits.types.output).length,1);
    ok(applied.nodes.some(n=>n.type===edits.types.particle && value(n,37)===33 && value(n,38)===225.5 && value(n,39)===1000));
}
const source=fs.readFileSync(path.join(root,"jsx/starfield_gateway.jsx"),"utf8").replace("    function readNativeNode(effect, layer) {",
    "    $.global.cloudApi={read:readNativeNode,write:setNodeParameters,clear:function(){nativePropertyIndexes=[];nativeLayerInventory=null;}};\n    function readNativeNode(effect, layer) {");
const global={},context=vm.createContext({$:{global},app:{}});vm.runInContext(source,context);
const api=global.cloudApi,realm=x=>vm.runInContext("("+JSON.stringify(x)+")",context);
const props=require("./node_property_fixture.js").nodeControls();
for(const property of new Set(Object.values(props))){property.numKeys=0;property.setValue=function(v){this.value=v;};}
const layer={id:9,width:1920,height:1080,source:{name:"Renderer",pixelAspect:1},hasVideo:true};
layer.containingComp={numLayers:1,layer(){return layer;}};
const effect={name:"Particle",matchName:"org.starfieldfx.node.particle",property(name){return props[name] || null;}};
function record(g){const n=particle(g);return {id:n.id,type:n.type,schemaVersion:n.schemaVersion,parameters:n.parameters.map(p=>({...p,
    value:p.type===7?Array.from(p.value):clone(p.value)})),position:{x:100,y:200},outgoing:[]};}
api.write(effect,realm(record(graph)),layer);
equal([239,240,241,242].map(k=>props["org.starfieldfx.node.particle-"+k].value),[33,225.5,1000,1]);
equal([37,38,39].map(k=>value(clone(api.read(effect,layer)),k)),[33,225.5,1000]);
api.write(effect,realm(record(old)),layer);equal(props["org.starfieldfx.node.particle-242"].value,0);
equal(clone(api.read(effect,layer)).parameters.filter(p=>Number(p.key)>=37).length,0);
const partial=record(old);partial.parameters.push({key:"39",type:4,value:1000});api.write(effect,realm(partial),layer);
equal([239,240,241,242].map(k=>props["org.starfieldfx.node.particle-"+k].value),[10,150,1000,1]);
for(const [key,v,type] of [[37,1001,3],[37,3.5,3],[38,0,4],[39,-1,4],[39,1001,4],[39,1,3]]) {
    const invalid=record(graph);invalid.parameters.find(p=>p.key===String(key)).value=v;
    invalid.parameters.find(p=>p.key===String(key)).type=type;
    reject(()=>api.write(effect,realm(invalid),layer),/Invalid Cloud/);equal(props["Panel Sync Guard"].value,0);
}
// A display-name decoy must never replace an exact disk-ID property.
props.Density={name:"Density",value:12,setValue(){throw Error("decoy used");}};
api.clear();api.write(effect,realm(record(graph)),layer);equal(props["org.starfieldfx.node.particle-241"].value,1000);
const missing=props["org.starfieldfx.node.particle-241"];
delete props["org.starfieldfx.node.particle-241"];delete props["org.starfieldfx.node.particle-0241"];
api.clear();reject(()=>api.write(effect,realm(record(graph)),layer),/Node effect parameter is missing.*disk 241/);
equal(props["Panel Sync Guard"].value,0);props[missing.matchName]=missing;
console.log(`Cloud panel: ${checks} checks passed.`);
