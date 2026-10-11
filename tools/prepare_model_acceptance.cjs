"use strict";
// Generate production-planner requests for the separately authorized AE run.
const fs=require("fs"),path=require("path"),assert=require("node:assert/strict");
if(process.argv[2]!=="--prepare"){console.log("Report only: add --prepare <isolated panel> <artifact output> to generate AE acceptance requests; no AE process is started.");process.exit(0);}
const root=path.resolve(__dirname,".."),panel=path.resolve(process.argv[3]||""),destination=path.resolve(process.argv[4]||"");
for(const target of [panel,destination])if(!target.startsWith(path.join(root,"artifacts")+path.sep))throw Error("Acceptance inputs/outputs must be under artifacts.");
if(fs.existsSync(path.join(destination,"result.json")))throw Error("Retain the prior acceptance result; choose a new artifact directory.");
const load=name=>require(path.join(panel,"js",name+".js")),edits=load("graph_edits"),presets=load("presets"),layout=load("graph_layout"),codec=load("graph_codec"),assets=load("model_assets");
const build=/var GATEWAY_BUILD = "([^"]+)"/.exec(fs.readFileSync(path.join(panel,"jsx/starfield_gateway.jsx"),"utf8"))[1];
const copy=x=>JSON.parse(JSON.stringify(x,(_,v)=>ArrayBuffer.isView(v)?Array.from(v):v)),uuid=n=>n.toString(16).padStart(32,"0");
let sequence=10000;const next=()=>uuid(++sequence),steps=[];
const markerFront=1270000044,markerBack=1270000066;
const geometry=Buffer.alloc(164);geometry.write("SFMG");geometry.writeUInt16LE(1,4);geometry.writeUInt16LE(32,6);geometry.writeUInt32LE(164,8);geometry.writeUInt32LE(3,16);geometry.writeUInt32LE(1,28);
let bounds=[-2,-3,-4,2,3,4];[[-2,-3,-4],[2,3,4],[2,-3,4]].forEach((v,i)=>v.concat(1).forEach((n,a)=>geometry.writeDoubleLE(n,32+i*32+a*8)));
for(let i=0;i<3;i++){geometry.writeUInt32LE(i,128+i*12);geometry.writeUInt32LE(0xffffffff,132+i*12);geometry.writeUInt32LE(0xffffffff,136+i*12);}
geometry.writeUInt32LE(codec.crc32(geometry.subarray(32)),12);let meshHex=geometry.toString("hex");assert.deepEqual(assets.validateMesh(meshHex).bounds,bounds);
// An optional owned numeric mesh permits the same full transaction/preset/
// duplicate suite to use a real parser output without retaining the user's OBJ.
if(process.argv[5]){const meshPath=path.resolve(process.argv[5]);
    if(!meshPath.startsWith(path.join(root,"artifacts")+path.sep))throw Error("Acceptance mesh must be under artifacts.");
    const mesh=fs.readFileSync(meshPath);if(mesh.length>8*1024*1024)throw Error("Acceptance mesh exceeds the Model budget.");
    meshHex=mesh.toString("hex");bounds=assets.validateMesh(meshHex).bounds;}
function parameter(n,key,value){n.parameters.find(p=>p.key===String(key)).value=value;}
function portableAssets(g){return g.nodes.filter(n=>n.type===edits.types.model && n.parameters.find(p=>p.key==="13").value===1)
    .map(n=>({nodeId:n.id,source:2,revision:17,bounds:bounds.slice(),meshHex}));}
function record(g){const positions=layout.resolve(g);return g.nodes.filter(n=>n.type!==edits.types.output).map(n=>({...copy(n),position:positions[n.id],outgoing:g.edges.filter(e=>e.sourceNode===n.id).map(e=>({id:e.id,target:e.destinationNode}))}));}
const renderNames=["maxParticles","timeRemapEnabled","timeRemapSeconds","previewEnabled","previewChance","acceleration","timeSamplingHz","motionBlur","shutterAngle","shutterPhase","motionBlurType","motionBlurLevels","linearAccuracy","opacityBoost","motionBlurDisregard"];
function save(name,g){assets.validate(g,portableAssets(g));const output=g.nodes.find(n=>n.type===edits.types.output),renderer={id:output.id,position:layout.resolve(g)[output.id]};
    renderNames.forEach((name,i)=>renderer[name]=output.parameters.find(p=>p.key===String(i+1)).value);
    const bytes=Buffer.from(codec.serialize(g)),patches=[];
    for(const [value,source] of [[markerFront,"front"],[markerBack,"back"]]){const pattern=Buffer.alloc(4);pattern.writeUInt32LE(value);
        for(let offset=bytes.indexOf(pattern,32);offset>=0;offset=bytes.indexOf(pattern,offset+4))patches.push({offset,source});}
    const expected=g.nodes.reduce((sum,n)=>sum+n.parameters.filter(p=>p.type===3 && [markerFront,markerBack].includes(p.value)).length,0);assert.equal(patches.length,expected);
    const patched=Buffer.from(bytes);for(const p of patches)patched.writeUInt32LE(p.source==="front"?44:66,p.offset);patched.writeUInt32LE(codec.crc32(patched.subarray(32)),24);codec.parse(patched);
    steps.push({name,graphHex:bytes.toString("hex"),texturePatches:patches,nodeManifest:record(g),rendererManifest:renderer,assets:portableAssets(g)});
}
let graph=presets.build("sparks",360);graph=edits.apply(graph,{type:"addNode",nodeType:"model"},()=>uuid(101));save("default Model Add",graph);
graph=edits.apply(graph,{type:"addNode",nodeType:"emitter"},()=>uuid(102));graph=edits.apply(graph,{type:"addNode",nodeType:"particle"},()=>uuid(103));graph=edits.apply(graph,{type:"addNode",nodeType:"transform"},()=>uuid(104));
for(const [from,to] of [[uuid(102),uuid(103)],[uuid(103),uuid(104)],[uuid(104),uuid(255)]])graph=edits.apply(graph,{type:"connect",from,to},next);
const texture=graph.nodes.find(n=>n.id===uuid(103));parameter(texture,15,3);parameter(texture,31,markerFront);parameter(texture,32,markerBack);save("mixed nodes and existing front/back Texture references",graph);
graph=edits.apply(graph,{type:"duplicateNodes",nodeIds:[uuid(101)],modelAssets:{}},next);save("default Model duplicate",graph);
const model=graph.nodes.find(n=>n.id===uuid(101)),raw=Buffer.alloc(48);bounds.forEach((v,i)=>raw.writeDoubleLE(v,i*8));
for(const [key,value] of [[1,Array.from(Buffer.from(model.id,"hex"))],[2,17],[12,Array.from(raw)],[13,1],[5,[0,45,0]]])parameter(model,key,value);save("restore imported SFMG1 resource and pose",graph);
graph=edits.apply(graph,{type:"duplicateNodes",nodeIds:[uuid(101)],modelAssets:{[uuid(101)]:{revision:17,bounds}}},next);save("imported Model duplicate",graph);
const preset=copy(graph),presetAssets=portableAssets(preset),resources=[{id:markerFront,name:"Front",sourceName:"Front source",texture:true},{id:markerBack,name:"Back",sourceName:"Back source",texture:true}];
const encoded=presets.encode(preset,"Model acceptance","Checks",resources,presetAssets),decoded=presets.decode(encoded);
for(const mode of ["add","replace"]){graph=presets.apply(graph,{type:"applyPreset",presetGraph:decoded.graph,presetModelAssets:decoded.modelAssets,presetResources:decoded.resources,mode,applyRenderSettings:true,layerResources:resources},next);save("resource preset "+mode,graph);}
graph=edits.apply(graph,{type:"deleteNodes",nodeIds:graph.nodes.filter(n=>n.type===edits.types.model).map(n=>n.id)},next);save("delete Model nodes with existing Texture references",graph);
const exportPages=steps.reduce((sum,step)=>sum+step.assets.reduce((pages,a)=>pages+Math.ceil(a.meshHex.length/65536),0),0);
fs.mkdirSync(destination,{recursive:true});const fixtures={version:1,gatewayBuild:build,markerFront,markerBack,steps,
    deadlineMilliseconds:Math.min(600000,120000+exportPages*500)};
fs.writeFileSync(path.join(destination,"fixtures.json"),JSON.stringify(fixtures));
const config={version:1,gatewayBuild:build,repo:root,fixture:path.join(destination,"fixtures.json"),result:path.join(destination,"result.json")};
// ASCII source avoids relying on the host's source-file encoding for Unicode paths.
// The entry log deliberately does not require JSON: a cold ExtendScript engine
// must report a missing JSON runtime before loading the production gateway.
const literal=value=>JSON.stringify(value).replace(/[^\x20-\x7e]/g,c=>'\\u'+c.charCodeAt(0).toString(16).padStart(4,'0'));
const wrapper='(function(){\n'+
    'function quote(s){return "\\\""+String(s).replace(/\\\\/g,"\\\\\\\\").replace(/"/g,"\\\\\\\"").replace(/\\r/g,"\\\\r").replace(/\\n/g,"\\\\n").replace(/\\t/g,"\\\\t")+"\\\"";}\n'+
    'function entry(stage,error){var f=new File('+literal(path.join(destination,'entry.json'))+');f.encoding="UTF-8";if(!f.open("w"))throw new Error("Cannot write acceptance entry log.");try{f.write("{\\\"stage\\\":"+quote(stage)+",\\\"error\\\":"+quote(error)+"}");}finally{f.close();}}\n'+
    'try{entry("entered","");if(typeof JSON==="undefined" || typeof JSON.parse!=="function" || typeof JSON.stringify!=="function")throw new Error("Missing JSON runtime in the AE script engine.");\n'+
    '$.global.__SFLD_modelAcceptanceConfig='+literal(config)+';entry("dispatching","");\n'+
    '$.evalFile(new File('+literal(path.join(root,"tools/Run-ModelAcceptance.jsx"))+'));entry("scheduled","");\n'+
    '}catch(error){entry("failed",String(error)+" (line "+error.line+")");}\n}());\n';
fs.writeFileSync(path.join(destination,"Run.jsx"),wrapper);
console.log(`Prepared ${steps.length} production-planner AE cases at ${destination}; no AE process/project was changed.`);
