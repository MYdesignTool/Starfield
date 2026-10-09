"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"../cep_panel");
const edits=require(path.join(root,"js/graph_edits.js")),view=require(path.join(root,"js/graph_view.js"));
const presets=require(path.join(root,"js/presets.js")),codec=require(path.join(root,"js/graph_codec.js"));
let checks=0;const eq=(a,b)=>{checks++;assert.deepEqual(a,b);},ok=x=>{checks++;assert.ok(x);},reject=(f,re)=>{checks++;assert.throws(f,re);};
const clone=x=>JSON.parse(JSON.stringify(x)),particle=g=>g.nodes.find(n=>n.type===edits.types.particle);
const fields=n=>n.parameters.filter(p=>p.key==="40" || p.key==="41").map(p=>[p.key,p.type,p.value]);
function change(g,k,v,t=k===40?2:4){return edits.apply(g,{type:"setParameters",changes:[{nodeId:particle(g).id,parameterKey:String(k),valueType:t,value:v}]});}
const fresh=presets.build("sparks",1080);eq(fields(particle(fresh)),[["40",2,0],["41",4,100]]);
const old=codec.fromHex(codec.toHex(fresh));particle(old).parameters=particle(old).parameters.filter(p=>Number(p.key)<40);
const oldHex=codec.toHex(old);eq(edits.nativeRecordSchema(particle(old)),7);
const projected=view.project(old,null,{height:1080,width:1920,pixelAspect:1}).nodes.find(n=>n.id===particle(old).id);
eq(projected.params.filter(p=>Number(p.graphKey)>=40).map(p=>[p.label,p.value,p.min,p.max]),
   [["Shift Seed",0,-2147483648,2147483647],["Birth Chance",100,0,100]]);
eq(codec.toHex(old),oldHex);eq(fields(particle(change(old,3,20,4))),[]);
eq(fields(particle(change(old,40,-43))),[["41",4,100],["40",2,-43]]);
eq(fields(particle(change(old,41,25.5))),[["40",2,0],["41",4,25.5]]);
for(const v of [-2147483648,-1,0,2147483647]) {
    const g=change(fresh,40,v);eq(fields(particle(codec.fromHex(codec.toHex(g)))),[["40",2,v],["41",4,100]]);
}
for(const v of [0,25.5,100])eq(fields(particle(change(fresh,41,v)))[1],["41",4,v]);
for(const [k,v,t] of [[40,-2147483649,2],[40,2147483648,2],[40,.5,2],[40,0,3],[40,0,4],
                      [41,-1,4],[41,101,4],[41,50,2],[41,NaN,4],[41,Infinity,4],[40,"3",2]])
    reject(()=>change(fresh,k,v,t),/Invalid Birth|finite|unsupported/i);
let sequence=900;const next=()=>String(sequence++).padStart(32,"0");
const configured=change(change(fresh,40,-43),41,25.5),entry=presets.decode(presets.encode(configured,"Birth controls","My Presets"));
for(const mode of ["add","replace"]) {
    const applied=presets.apply(old,{type:"applyPreset",presetGraph:entry.graph,presetResources:entry.resources,
        layerResources:[],mode,applyRenderSettings:true},next);
    eq(applied.nodes.filter(n=>n.type===edits.types.output).length,1);
    ok(applied.nodes.some(n=>n.type===edits.types.particle && JSON.stringify(fields(n))===JSON.stringify([["40",2,-43],["41",4,25.5]])));
}
const source=fs.readFileSync(path.join(root,"jsx/starfield_gateway.jsx"),"utf8").replace("    function readNativeNode(effect, layer) {",
    "    $.global.birthApi={read:readNativeNode,write:setNodeParameters,clear:function(){nativePropertyIndexes=[];nativeLayerInventory=null;}};\n    function readNativeNode(effect, layer) {");
const global={},context=vm.createContext({$:{global},app:{}});vm.runInContext(source,context);
const api=global.birthApi,realm=x=>vm.runInContext("("+JSON.stringify(x)+")",context);
const props=require("./node_property_fixture.js").nodeControls();
for(const p of new Set(Object.values(props))){p.numKeys=0;p.setValue=function(v){this.value=v;};}
const layer={id:9,width:1920,height:1080,source:{name:"Renderer",pixelAspect:1},hasVideo:true};
layer.containingComp={numLayers:1,layer(){return layer;}};
const effect={name:"Particle",matchName:"org.starfieldfx.node.particle",property(name){return props[name] || null;}};
const disk=k=>props["org.starfieldfx.node.particle-"+k];
function record(g){const n=particle(g);return {...n,parameters:n.parameters.map(p=>({...p,value:p.type===7?Array.from(p.value):clone(p.value)})),position:{x:100,y:200},outgoing:[]};}
api.write(effect,realm(record(configured)),layer);eq([242,243,244,245].map(k=>disk(k).value),[1,-43,25.5,1]);
eq(fields(clone(api.read(effect,layer))),[["40",2,-43],["41",4,25.5]]);
api.write(effect,realm(record(old)),layer);eq(disk(245).value,0);eq(fields(clone(api.read(effect,layer))),[]);
const partial=record(old);partial.parameters.push({key:"41",type:4,value:0});api.write(effect,realm(partial),layer);
eq([243,244,245].map(k=>disk(k).value),[0,0,1]);eq(disk(242).value,1);
for(const [k,v,t] of [[40,2147483648,2],[40,.5,2],[40,0,3],[41,-1,4],[41,101,4],[41,50,2]]) {
    const invalid=record(configured),p=invalid.parameters.find(p=>p.key===String(k));p.value=v;p.type=t;
    reject(()=>api.write(effect,realm(invalid),layer),/Invalid Birth/);eq(props["Panel Sync Guard"].value,0);
}
props["Shift Seed"]={value:11,setValue(){throw Error("name decoy used");}};
api.clear();api.write(effect,realm(record(configured)),layer);eq(disk(243).value,-43);
disk(245).value=2;reject(()=>api.read(effect,layer),/Invalid Birth activation/);disk(245).value=1;
const missing=disk(244);delete props[missing.matchName];delete props["org.starfieldfx.node.particle-0244"];
api.clear();reject(()=>api.write(effect,realm(record(configured)),layer),/Node effect parameter is missing.*disk 244/);
eq(props["Panel Sync Guard"].value,0);
console.log(`Birth panel: ${checks} checks passed.`);
