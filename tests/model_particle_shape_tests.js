"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=process.env.STARFIELD_PANEL_ROOT;if(!root)throw new Error("Set STARFIELD_PANEL_ROOT to the isolated Model candidate.");
const load=name=>require(path.join(root,"js",name+".js"));
const edits=load("graph_edits"),view=load("graph_view"),codec=load("graph_codec"),presets=load("presets");
let checks=0,writes=0,serial=1000;
const eq=(a,b)=>{checks++;assert.deepEqual(a,b);},ok=x=>{checks++;assert.ok(x);},reject=(f,p)=>{checks++;assert.throws(f,p);};
const copy=x=>JSON.parse(JSON.stringify(x)),next=()=>String(serial++).padStart(32,"0");
const names=["Circle","Rectangle","Cloud","Texture","Face","Model"];
let graph=presets.build("sparks",1080),particle=graph.nodes.find(n=>n.type===edits.types.particle);
function set(shape){graph=edits.apply(graph,{type:"setParameters",changes:[{nodeId:particle.id,parameterKey:"15",valueType:3,value:shape}]});particle=graph.nodes.find(n=>n.id===particle.id);}
const shapeField=()=>view.project(graph,null,{height:1080,width:1920,pixelAspect:1}).nodes.find(n=>n.id===particle.id).params.find(p=>p.graphKey==="15");
for(let core=0;core<=4;core++) {
    set(core);const field=shapeField();eq(field.choices,names);eq(field.enumValues,[0,1,2,3,null,4]);eq(field.disabledChoices,[4]);
    eq(field.value,core===4?6:core+1);eq(view.parameterToGraphValue(field,field.value),core);
    eq(codec.fromHex(codec.toHex(graph)).nodes.find(n=>n.id===particle.id).parameters.find(p=>p.key==="15").value,core);
}
set(4);reject(()=>view.parameterToGraphValue(shapeField(),5),/OBJ face-emission/);
for(const choice of [0,7,1.5,NaN])reject(()=>view.parameterToGraphValue(shapeField(),choice),/OBJ face-emission/);
const projected=view.project(graph).nodes.find(n=>n.id===particle.id);ok(!projected.params.some(p=>p.graphKey==="16"));
const entry=presets.decode(presets.encode(graph,"Model cube shape"));
for(const mode of ["add","replace"]) {
    const applied=presets.apply(presets.build("orbit",1080),{type:"applyPreset",presetGraph:entry.graph,mode,applyRenderSettings:true},next);
    ok(applied.nodes.some(n=>n.type===edits.types.particle&&n.parameters.some(p=>p.key==="15"&&p.value===4)));
    eq(applied.nodes.filter(n=>n.type===edits.types.output).length,1);
}
// Execute the real inspector select builder without starting the whole panel.
const panel=fs.readFileSync(path.join(root,"js/panel.js"),"utf8"),start=panel.indexOf("    function popupInput(parameter) {");
ok(start>=0);const end=panel.indexOf("\n    function ",start+1);ok(end>start);
const document={createElement(tag){return {tag,children:[],dataset:{},appendChild(x){this.children.push(x);},addEventListener(){}};}};
const build=vm.runInNewContext("("+panel.slice(start,end).trim()+")",{document,onEdit(){}});
const select=build(shapeField());eq(select.children.map(o=>o.textContent),names);
eq(select.children.map(o=>!!o.disabled),[false,false,false,false,true,false]);eq(select.value,"6");
const source=fs.readFileSync(path.join(root,"jsx/starfield_gateway.jsx"),"utf8").replace("    function readNativeNode(effect, layer) {",
    "    $.global.shapeApi={read:readNativeNode,write:setNodeParameters,validate:validateNodeManifest,toNative:particleShapeToNative,fromNative:particleShapeFromNative};\n    function readNativeNode(effect, layer) {");
const global={},context=vm.createContext({$:{global},app:{}});vm.runInContext(source,context);
const api=global.shapeApi,realm=x=>vm.runInContext("("+JSON.stringify(x)+")",context);
const props=require("./node_property_fixture.js").nodeControls();
for(const p of new Set(Object.values(props))){p.numKeys=0;p.setValue=function(v){writes++;this.value=v;};}
const effect={name:"Particle",matchName:"org.starfieldfx.node.particle",property(name){return props[name]||null;}};
const layer={id:9,width:1920,height:1080,source:{pixelAspect:1},hasVideo:true};
layer.containingComp={numLayers:1,layer(){return layer;}};
const record={id:particle.id,type:particle.type,schemaVersion:particle.schemaVersion,position:{x:100,y:200},outgoing:[],
    parameters:particle.parameters.map(p=>({...p,value:p.type===7?Array.from(p.value):copy(p.value)}))};
const shape=record.parameters.find(p=>p.key==="15"),nativeShape=props.Shape;
for(let core=0;core<=4;core++) {
    shape.value=core;api.write(effect,realm(record),layer);eq(nativeShape.value,core===4?6:core+1);
    eq(copy(api.read(effect,layer)).parameters.find(p=>p.key==="15").value,core);eq(props["Panel Sync Guard"].value,0);
    eq(api.toNative(core),core===4?6:core+1);eq(api.fromNative(core===4?6:core+1),core);
}
nativeShape.value=5;reject(()=>api.read(effect,layer),/Face requires OBJ face emission/);nativeShape.value=6;
for(const invalid of [-1,5,6,1.5,"4",null,NaN,Infinity])reject(()=>api.toNative(invalid),/Invalid Particle Shape/);
for(const invalid of [0,5,7,1.5,NaN,Infinity])reject(()=>api.fromNative(invalid),/Unsupported Particle Shape/);
const before=writes;
for(const [type,value] of [["3",4],[3,5],[3,"4"],[4,4]]) {
    shape.type=type;shape.value=value;reject(()=>api.validate(realm([record])),/Invalid Particle Shape/);
}
eq(writes,before);eq(nativeShape.value,6);eq(props["Panel Sync Guard"].value,0);
console.log(`model_particle_shape_tests: ${checks} checks passed; actual candidate view/select/gateway, fake host controls`);
