"use strict";
// Preparation only. Running AE needs the separate ADR0011 authorization.
const fs=require("node:fs"),path=require("node:path"),crypto=require("node:crypto");
const root=path.resolve(__dirname,".."),artifacts=path.join(root,"artifacts");
if(process.argv[2]!=="--prepare"){console.log("Report only: --prepare <new artifacts directory> generates an AE2023 preparation test; no AE process starts.");process.exit(0);}
const dest=path.resolve(process.argv[3]||"");
if(!dest.startsWith(artifacts+path.sep) || fs.existsSync(dest))throw Error("Choose a new directory inside checkout artifacts.");
const codec=require("../web_panel/js/graph_codec.js"),edits=require("../web_panel/js/graph_edits.js"),presets=require("../web_panel/js/presets.js"),layout=require("../web_panel/js/graph_layout.js");
const uuid=()=>crypto.randomUUID().replaceAll("-",""),clone=x=>JSON.parse(JSON.stringify(x,(_,v)=>ArrayBuffer.isView(v)?Array.from(v):v));
const renderNames=["maxParticles","timeRemapEnabled","timeRemapSeconds","previewEnabled","previewChance","acceleration","timeSamplingHz","motionBlur","shutterAngle","shutterPhase","motionBlurType","motionBlurLevels","linearAccuracy","opacityBoost","motionBlurDisregard"];
function parameter(node,key,value){const p=node.parameters.find(p=>p.key===String(key));if(!p)throw Error("Missing fixture parameter "+key);p.value=value;}
const cases=[];
for(const [name,population,depth,complex] of [["simple-30k",30000,0,false],["force-4-30k",30000,4,false],["force-12-30k",30000,12,false],["force-24-50k",50000,24,false],["complex-6-30k",30000,6,true]]){
    let graph=presets.build("sparks",180),positions=layout.resolve(graph),mapping={};
    graph.nodes.forEach(n=>{if(n.type!==edits.types.output)mapping[n.id]=uuid();});
    const mapped={};graph.nodes.forEach(n=>{const old=n.id;n.id=mapping[old]||old;mapped[n.id]=positions[old];});
    graph.edges.forEach(e=>{e.id=uuid();e.sourceNode=mapping[e.sourceNode]||e.sourceNode;e.destinationNode=mapping[e.destinationNode]||e.destinationNode;});
    graph=layout.set(graph,mapped);
    let emitter=graph.nodes.find(n=>n.type===edits.types.emitter),particle=graph.nodes.find(n=>n.type===edits.types.particle);
    parameter(emitter,2,population/2);parameter(emitter,17,0);parameter(particle,11,2);parameter(particle,14,0);
    const output=graph.nodes.find(n=>n.type===edits.types.output);parameter(output,1,100000);parameter(output,8,0);
    const initial={};for(const n of graph.nodes)if(n.type!==edits.types.output)initial[n.type]=n.id;
    for(let i=0;i<depth;i++){
        const type=complex?(i%2?"turbulence":"transform"):"force",id=uuid();
        graph=edits.apply(graph,{type:"addNode",nodeType:type},()=>id);
        const edge=graph.edges.find(e=>e.destinationNode===output.id);
        const previous=edge.sourceNode;graph=edits.apply(graph,{type:"disconnect",edgeId:edge.id},uuid);
        graph=edits.apply(graph,{type:"connect",from:previous,to:id},uuid);
        graph=edits.apply(graph,{type:"connect",from:id,to:output.id},uuid);
    }
    const bytes=Buffer.from(codec.serialize(graph));if(bytes.length>24*1024)throw Error("Fixture exceeds graph budget: "+name);
    codec.parse(bytes);positions=layout.resolve(graph);
    const nodes=graph.nodes.filter(n=>n.type!==edits.types.output).map(n=>({id:n.id,type:n.type,schemaVersion:n.schemaVersion,position:positions[n.id],parameters:clone(n.parameters.filter(p=>n.type!==edits.types.transform||p.key!=="7")),outgoing:graph.edges.filter(e=>e.sourceNode===n.id).map(e=>({id:e.id,target:e.destinationNode})).sort((a,b)=>a.id.localeCompare(b.id))}));
    const renderer={id:output.id,position:positions[output.id]};renderNames.forEach((key,i)=>renderer[key]=output.parameters.find(p=>p.key===String(i+1)).value);
    cases.push({name,population,initial,nodeCount:graph.nodes.length,graphHex:bytes.toString("hex"),nodeManifest:nodes,rendererManifest:renderer});
}
const gateway=fs.readFileSync(path.join(root,"web_panel/jsx/starfield_gateway.jsx"),"utf8");
const config={version:1,root,result:path.join(dest,"result.json"),project:path.join(dest,"qualification.aep"),gatewayBuild:/var GATEWAY_BUILD = "([^"]+)"/.exec(gateway)[1],cases};
const literal=value=>JSON.stringify(value).replace(/[^\x20-\x7e]/g,c=>"\\u"+c.charCodeAt(0).toString(16).padStart(4,"0"));
const wrapper="$.global.__SFLD_preparationQualification="+literal(config)+";\n$.evalFile(new File("+literal(path.join(root,"tools/Run-PreparationQualification.jsx"))+"));\n";
fs.mkdirSync(dest,{recursive:true});fs.writeFileSync(path.join(dest,"fixtures.json"),JSON.stringify(config,null,2));fs.writeFileSync(path.join(dest,"Run.jsx"),wrapper);
console.log(`Prepared ${cases.length} bounded AE cases at ${dest}; no AE operation. Render queue timings include encoding/host overhead and are not native preparation-only timings.`);
