"use strict";
// Prepare only; launching AE has its own ADR0011 permission and exact command.
const fs=require("fs"),path=require("path"),cp=require("child_process");
if(process.argv[2]!=="--prepare"){console.log("Report only: --prepare <isolated panel> <artifact output> <native journal>; no AE starts.");process.exit(0);}
const root=path.resolve(__dirname,".."),panel=path.resolve(process.argv[3]),dest=path.resolve(process.argv[4]),journal=path.resolve(process.argv[5]);
for(const p of [panel,dest,journal])if(!p.startsWith(path.join(root,"artifacts")+path.sep))throw Error("Keep all prepared paths under repository artifacts.");
cp.execFileSync(process.execPath,[path.join(root,"tools/prepare_model_acceptance.cjs"),"--prepare",panel,dest],{stdio:"inherit"});
const edits=require(path.join(panel,"js/graph_edits.js")),presets=require(path.join(panel,"js/presets.js")),codec=require(path.join(panel,"js/graph_codec.js")),layout=require(path.join(panel,"js/graph_layout.js"));
const fixture=JSON.parse(fs.readFileSync(path.join(dest,"fixtures.json"),"utf8"));let serial=20000;const next=()=> (++serial).toString(16).padStart(32,"0");
let full=presets.build("sparks",360);
for(const type of ["transform","model","material","motion"])full=edits.apply(full,{type:"addNode",nodeType:type},next);
const present=new Set(full.nodes.map(n=>n.type));for(const type of ["emitter","particle","force","transform","model","material","motion"])if(!present.has(edits.types[type]))throw Error("Missing native kind "+type);
const output=full.nodes.find(n=>n.type===edits.types.output);output.parameters.find(p=>p.key==="1").value=200;output.parameters.find(p=>p.key==="6").value=1;
const renderNames=["maxParticles","timeRemapEnabled","timeRemapSeconds","previewEnabled","previewChance","acceleration","timeSamplingHz","motionBlur","shutterAngle","shutterPhase","motionBlurType","motionBlurLevels","linearAccuracy","opacityBoost","motionBlurDisregard"];
function save(name,g){g=JSON.parse(JSON.stringify(g,(_,v)=>ArrayBuffer.isView(v)?Array.from(v):v));const positions=layout.resolve(g),out=g.nodes.find(n=>n.type===edits.types.output),renderer={id:out.id,position:positions[out.id]};
 renderNames.forEach((key,i)=>renderer[key]=out.parameters.find(p=>p.key===String(i+1)).value);
 return {name,graphHex:Buffer.from(codec.serialize(g)).toString("hex"),texturePatches:[],assets:[],rendererManifest:renderer,
 nodeManifest:g.nodes.filter(n=>n.type!==edits.types.output).map(n=>({...n,position:positions[n.id],outgoing:g.edges.filter(e=>e.sourceNode===n.id).map(e=>({id:e.id,target:e.destinationNode}))}))};}
fixture.steps=[];
for(const type of ["emitter","particle","force","transform","model","material","motion"]){fixture.steps.push(save("restore all seven / "+type,full));
 const ids=full.nodes.filter(n=>n.type===edits.types[type]).map(n=>n.id);fixture.steps.push(save("delete "+type,edits.apply(full,{type:"deleteNodes",nodeIds:ids},next)));}
fixture.steps.push(save("restore all seven / delete all",full));fixture.steps.push(save("delete all nodes",edits.apply(full,{type:"deleteNodes",nodeIds:full.nodes.filter(n=>n.type!==edits.types.output).map(n=>n.id)},next)));
fixture.nativeJournal=journal;fixture.nativeBuild=76;fixture.renderFrames=true;fixture.deadlineMilliseconds=600000;
fs.writeFileSync(path.join(dest,"fixtures.json"),JSON.stringify(fixture));console.log(`Prepared ${fixture.steps.length} seven-kind deletion/render cases with native journal verification; no AE operation.`);
