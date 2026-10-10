"use strict";
// Actual candidate client/ExtendScript transport, Model validators and graph
// planner. Project DOM, SFMW and native executor are simulated here.
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=process.env.STARFIELD_PANEL_ROOT;if(!root)throw new Error("Set STARFIELD_PANEL_ROOT to the isolated Model candidate.");
const gatewayBuild=/var GATEWAY_BUILD = "([^"]+)"/.exec(fs.readFileSync(path.join(root,"jsx/starfield_gateway.jsx"),"utf8"))[1];
const load=n=>require(path.join(root,"js",n+".js"));
const edits=load("graph_edits"),presets=load("presets"),graphTransactions=load("graph_transactions"),snapshots=load("native_graph_snapshot"),codec=load("graph_codec");
const clientAPI=load("model_graph_transactions"),assetAPI=load("model_assets"),copy=x=>JSON.parse(JSON.stringify(x,(_,v)=>ArrayBuffer.isView(v)?Array.from(v):v));
let checks=0;const eq=(a,b,message)=>{checks++;assert.deepEqual(a,b,message);},ok=v=>{checks++;assert.ok(v);};
const uuid=n=>n.toString(16).padStart(32,"0"),modelId=uuid(101),transactionId=uuid(900),bounds=[-2,-3,-4,2,3,4];
let token="p101-c202-l303";
function mesh(triangles=1){const b=Buffer.alloc(32+3*32+36*triangles);b.write("SFMG");b.writeUInt16LE(1,4);b.writeUInt16LE(32,6);b.writeUInt32LE(b.length,8);b.writeUInt32LE(3,16);b.writeUInt32LE(triangles,28);
    [[-2,-3,-4],[2,3,4],[2,-3,4]].forEach((point,p)=>point.concat(1).forEach((v,a)=>b.writeDoubleLE(v,32+p*32+a*8)));
    for(let t=0;t<triangles;t++)for(let i=0;i<3;i++){b.writeUInt32LE(i,128+t*36+i*12);b.writeUInt32LE(0xffffffff,132+t*36+i*12);b.writeUInt32LE(0xffffffff,136+t*36+i*12);}
    let crc=0xffffffff;for(const byte of b.subarray(32)){crc^=byte;for(let bit=0;bit<8;bit++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);}b.writeUInt32LE((crc^0xffffffff)>>>0,12);return b.toString("hex");}
const meshHex=mesh();eq(assetAPI.validateMesh(meshHex).bounds,bounds);
function graph(imported){let g=presets.build("sparks",1080);g=edits.apply(g,{type:"addNode",nodeType:"model"},()=>modelId);
    if(imported){const n=g.nodes.find(n=>n.id===modelId),raw=Buffer.alloc(48);bounds.forEach((v,i)=>raw.writeDoubleLE(v,i*8));
        for(const [key,value] of [[1,Array.from(Buffer.from(modelId,"hex"))],[2,17],[12,Array.from(raw)],[13,1]])n.parameters.find(p=>p.key===String(key)).value=value;}
    return g;}
const layout=load("graph_layout"),imported=graph(true),cube=graph(false);
function manifest(g){const positions=layout.resolve(g);return g.nodes.filter(n=>n.type!==edits.types.output).map(n=>({
    ...copy(n),position:positions[n.id],outgoing:g.edges.filter(e=>e.sourceNode===n.id).map(e=>({id:e.id,target:e.destinationNode})).sort((a,b)=>a.id.localeCompare(b.id))}));}
function render(g){const n=g.nodes.find(n=>n.type===edits.types.output),r={id:n.id,position:layout.resolve(g)[n.id]};
    ["maxParticles","timeRemapEnabled","timeRemapSeconds","previewEnabled","previewChance","acceleration","timeSamplingHz","motionBlur","shutterAngle","shutterPhase","motionBlurType","motionBlurLevels","linearAccuracy","opacityBoost","motionBlurDisregard"].forEach((name,i)=>{
        const p=n.parameters.find(p=>String(p.key)===String(i+1));r[name]=p?p.value:[1000000,0,0,0,100,0,30,1,360,0,0,8,70,0,0][i];});return r;}
const sandbox={$:{global:{}},Date:class extends Date{constructor(...args){super(args.length?args[0]:now);}},
    app:{findMenuCommandId:()=>55,executeCommand:()=>{if(throwQueue)throw new Error("queue command failed");hostRuns++;if(!skipNative)tasks.push(nativeRun);}}};
const context=vm.createContext(sandbox),host=sandbox.$.global,realm=x=>vm.runInContext("("+JSON.stringify(x)+")",context);
vm.runInContext(fs.readFileSync(path.join(root,"jsx/starfield_gateway.jsx"),"utf8"),context);
const api=host.__SFLD_modelTransactionAPI;
let nodes=[],renderer={},revision=11,guard=0,stamp="stamp",tasks=[],hostRuns=0,prepares=0,commits=0,restores=0,writes=0,bytesRead=0,
    failedPrepare=false,failedCommit=false,badSaved=false,cleanupError=0,undoError=0,loseQueueAck=false,loseResult=false,now=1000,sequence=2000,
    throwQueue=false,skipNative=false,stopAfterClaim=false,loseRequestAck=false;
const nativeMeshes={};
function snapshot(){const current=copy(nodes);for(const n of current)if(n.type===edits.types.model){const asset=nativeMeshes[n.id],raw=n.parameters.find(p=>p.key==="12").value;
    n.modelAsset=asset?{revision:asset.revision,bounds:asset.bounds}: {revision:n.parameters.find(p=>p.key==="2").value,bounds:Array.from({length:6},(_,i)=>Buffer.from(raw).readDoubleLE(i*8))};}
    return realm({initialized:true,revision,checksum:"00000000",nativeNodes:current,renderer:copy(renderer),recordStamp:stamp,geometry:{width:1920,height:1080,pixelAspect:1}});}
api.resolve=request=>request.target && request.target.token===token?{token,target:{layer:{time:0}},properties:{guard:{get value(){return guard;}},revision:{get value(){return revision;}}}}:{error:{code:"stale_target",message:"Wrong target"}};
api.snapshot=()=>snapshot();api.validateTransform=()=>{};
api.ensure=(layer,desired,writeExisting,previous,defer)=>{writes++;if(defer){prepares++;if(failedPrepare)throw new Error("prepare failure");
        nodes=copy(desired).map(n=>{if(n.type===edits.types.model){const old=previous.find(p=>p.id===n.id);return old?{...copy(old),position:n.position,outgoing:n.outgoing}:manifest(cube).find(p=>p.type===edits.types.model) && {...copy(manifest(cube).find(p=>p.type===edits.types.model)),id:n.id,position:n.position,outgoing:n.outgoing};}return n;});
    }else{commits++;if(failedCommit)throw new Error("commit failure");for(const n of desired){if(n.type===edits.types.model && n.parameters.find(p=>p.key==="13").value===1){
        ok(nativeMeshes[n.id]);eq(nativeMeshes[n.id].revision,n.parameters.find(p=>p.key==="2").value);}}nodes=copy(desired);}};
api.remove=()=>{};api.writeRenderer=(resolved,r)=>{renderer=copy(r);};api.nonce=()=>66;
api.trigger=()=>{revision++;if(badSaved)nodes[0].position.x+=10;return realm({ok:true,nonce:66});};
function nativeRun(){const line=host.SFLD_modelTransactionHostRequest();if(line==="0")return;
    eq(host.__SFLD_modelGraphTransactionV1.state,"queued");
    if(loseRequestAck){loseRequestAck=false;tasks.push(nativeRun);return;}
    eq(guard,0);const parts=line.split("|"),id=parts[0],count=Number(parts[4]),before={nodes:copy(nodes),renderer:copy(renderer),revision};
    eq(host.SFLD_modelTransactionHostClaim(id),"1");if(stopAfterClaim)return;
    eq(parts.slice(1,4),/^p([0-9]+)-c([0-9]+)-l([0-9]+)$/.exec(token).slice(1));eq(host.SFLD_modelTransactionHostContinue(id),"1");
    const assets=[];
    for(let i=0;i<count;i++){const fields=host.SFLD_modelTransactionHostAsset(id,i).split("|"),length=Number(fields[3]),pages=[];
        for(let page=0;page<Math.ceil(length/32768);page++)pages.push(host.SFLD_modelTransactionHostPage(id,i,page));
        const hex=pages.join("");eq(hex.length/2,length);bytesRead+=length;assets.push({nodeId:fields[0],source:Number(fields[1]),revision:Number(fields[2]),bounds:fields.slice(4).map(Number),meshHex:hex});}
    // Native preflight validates all geometry before backup/prepare, as the
    // separately tested actual C++ executor does.
    try{for(const a of assets)if(a.revision)eq(assetAPI.validateMesh(a.meshHex).bounds,a.bounds);}catch(error){host.SFLD_modelTransactionHostResult(id,"0|0|512|0|0|0|0|5|0");return;}
    eq(host.SFLD_modelTransactionHostBegin(id),"1");eq(host.__SFLD_modelGraphTransactionV1.state,"executing");
    guard=1;let success=host.SFLD_modelTransactionHostPrepare(id)==="1";
    if(success){for(const a of assets){nativeMeshes[a.nodeId]=copy(a);const n=nodes.find(n=>n.id===a.nodeId),raw=Buffer.alloc(48);a.bounds.forEach((v,i)=>raw.writeDoubleLE(v,i*8));
        for(const [key,value] of [[1,a.source===2?Array.from(Buffer.from(a.nodeId,"hex")):Array(16).fill(0)],[2,a.source===2?a.revision:0],
            [12,a.source===2?Array.from(raw):manifest(cube).find(n=>n.type===edits.types.model).parameters.find(p=>p.key==="12").value],[13,a.source-1]])n.parameters.find(p=>p.key===String(key)).value=value;}
        success=host.SFLD_modelTransactionHostCommit(id)==="1";}
    if(!success){nodes=before.nodes;renderer=before.renderer;revision=before.revision;restores++;}
    guard=0;if(!loseResult)eq(host.SFLD_modelTransactionHostResult(id,[success?1:0,success?7:5,success?0:512,0,0,cleanupError,undoError,0,-1].join("|")),"1");
}
function reset(g=cube){nodes=manifest(g);renderer=render(g);revision=11;guard=0;stamp="stamp";tasks=[];hostRuns=prepares=commits=restores=writes=bytesRead=0;
    failedPrepare=failedCommit=badSaved=loseQueueAck=loseResult=throwQueue=skipNative=stopAfterClaim=loseRequestAck=false;cleanupError=undoError=0;now=1000;host.__SFLD_modelGraphTransactionV1=null;
    for(const key of Object.keys(nativeMeshes))delete nativeMeshes[key];vm.runInContext(fs.readFileSync(path.join(root,"jsx/model_transaction_transport.jsx"),"utf8"),context);}
function request(operation,fields={}){return JSON.stringify({protocol:"org.starfieldfx.panel",version:1,gatewayBuild:gatewayBuild,operation,requestId:"test",pinTarget:true,target:{token},transactionId,changes:[],...fields});}
function call(operation,fields,callback){
    if(operation==="beginModelAssetExport" || operation==="releaseModelAsset"){callback({ok:true,assetId:fields.assetId});return;}
    if(operation==="readModelAssetPage"){const n=nodes.find(n=>n.id===modelId),asset=nativeMeshes[modelId] || meshAsset;
        callback({ok:true,state:"ready",assetId:fields.assetId,nodeId:modelId,page:0,pageCount:1,bytes:asset.meshHex.length/2,source:n.parameters.find(p=>p.key==="13").value+1,revision:asset.revision,bounds:asset.bounds,hex:asset.meshHex});return;}
    const raw=host["SFLD_"+operation](request(operation,fields)),r=JSON.parse(raw);
    if(operation==="queueModelGraphTransaction" && loseQueueAck)callback({ok:false,error:{code:"host_timeout",message:"lost"}});else callback(r);}
function drain(){let visits=0;while(tasks.length){ok(++visits<1000);tasks.shift()();}}
function plan(g=imported){return {target:{token},baseGraphRevision:11,baseRecordStamp:"stamp",graphHex:codec.toHex(g),nodeManifest:manifest(g),rendererManifest:render(g)};}
const meshAsset={nodeId:modelId,source:2,revision:17,bounds,meshHex};
function client(){return clientAPI.create({call,schedule:fn=>tasks.push(fn),now:()=>now,idFactory:()=>transactionId});}
reset();let result;client().apply(plan(),[meshAsset],r=>{result=r;});drain();eq(result.ok,true,JSON.stringify(result));eq(result.committed,true);eq([hostRuns,prepares,commits,restores],[1,1,1,0]);eq(bytesRead,meshHex.length/2);
eq(host.__SFLD_modelGraphTransactionV1,null);eq(nativeMeshes[modelId].meshHex,meshHex);
// Exercise the production defaults rather than supplying an already-valid ID
// factory and an alternate Model client. Only the asynchronous timer is queued
// locally; real graph planning, client validation and JSX callbacks still run.
token="p0-c1-l29";reset(presets.build("sparks",1080));const originalTimer=global.setTimeout;
try{global.setTimeout=fn=>tasks.push(fn);
    graphTransactions.create({codec,edits,call}).apply({type:"addNode",nodeType:"model"},r=>{result=r;},token,11,
        {ok:true,target:{token},snapshot:copy(snapshot())});drain();
    eq(result.ok,true,JSON.stringify(result));eq(hostRuns,1);eq(nodes.filter(n=>n.type===edits.types.model).length,1);eq(bytesRead,0);
}finally{global.setTimeout=originalTimer;}
reset();client().apply(plan(),[meshAsset],r=>{result=r;});drain();eq(result.ok,true,JSON.stringify(result));eq(hostRuns,1);eq(bytesRead,meshHex.length/2);
token="p101-c202-l303";
// The generic identity/assets error must identify which boundary rejected the
// request without issuing a host call or revealing mesh payloads.
for(const badToken of [undefined,null,"p-1-c202-l303","p00-c202-l303","p101-c0-l303","p101-c202-l0","pinned"]){
    reset();client().apply({...plan(cube),target:{token:badToken}},[],r=>{result=r;});drain();
    eq(result.error.code,"invalid_model_transaction");ok(result.error.message.includes("Invalid Model target identity"));
    ok(result.error.message.includes("token="+String(badToken)));eq(hostRuns,0);eq(writes,0);
}
for(const badId of [undefined,0,"0".repeat(32),uuid(255),"A".repeat(32)]){
    reset();clientAPI.create({call,idFactory:()=>badId}).apply(plan(cube),[],r=>{result=r;});
    ok(result.error.message.includes("Invalid Model transaction ID"));eq(hostRuns,0);eq(writes,0);
}
for(const badAssets of [null,{},Array(64).fill(meshAsset)]){
    reset();client().apply(plan(cube),badAssets,r=>{result=r;});
    ok(result.error.message.includes("Invalid Model transaction asset list"));eq(hostRuns,0);eq(writes,0);
}
reset();const many=mesh(8192);client().apply(plan(),[{...meshAsset,meshHex:many}],r=>{result=r;});drain();eq(result.ok,true,JSON.stringify(result));eq(bytesRead,many.length/2);eq(nativeMeshes[modelId].meshHex,many);
reset();client().apply(plan(cube),[{...meshAsset,source:1}],r=>{result=r;});drain();eq(result.ok,true);eq(nativeMeshes[modelId].revision,17);
eq(nodes.find(n=>n.id===modelId).parameters.find(p=>p.key==="2").value,0);eq(nodes.find(n=>n.id===modelId).parameters.find(p=>p.key==="13").value,0);
for(const key of ["failedPrepare","failedCommit","badSaved"]){reset();if(key==="failedPrepare")failedPrepare=true;if(key==="failedCommit")failedCommit=true;if(key==="badSaved")badSaved=true;
    client().apply(plan(),[meshAsset],r=>{result=r;});drain();eq(result.ok,false);eq(result.committed,false);eq(restores,1);eq(nodes,manifest(cube));eq(revision,11);}
reset();cleanupError=516;undoError=512;client().apply(plan(),[meshAsset],r=>{result=r;});drain();eq(result.ok,true);eq(result.committed,true);eq(result.diagnostics.cleanupError,516);eq(result.diagnostics.undoError,512);eq(hostRuns,1);
reset();loseQueueAck=true;client().apply(plan(),[meshAsset],r=>{result=r;});drain();eq(result.ok,true);eq(hostRuns,1);
reset();loseRequestAck=true;client().apply(plan(cube),[],r=>{result=r;});drain();eq(result.ok,true);eq(hostRuns,1);eq(prepares,1);
reset();throwQueue=true;client().apply(plan(cube),[],r=>{result=r;});drain();eq(result.ok,false);eq(writes,0);eq(hostRuns,0);eq(host.__SFLD_modelGraphTransactionV1,null);
for(const phase of ["queued","receiving"]){reset();skipNative=phase==="queued";stopAfterClaim=phase==="receiving";
    client().apply(plan(cube),[],r=>{result=r;});for(let step=0;step<6&&tasks.length;step++)tasks.shift()();
    eq(host.__SFLD_modelGraphTransactionV1.state,phase);now+=15001;drain();eq(result.ok,false);
    eq(result.error.code,phase==="queued"?"model_host_not_started":"model_host_transfer_stalled");
    ok(result.error.message.includes("state="+phase));eq(writes,0);eq(host.__SFLD_modelGraphTransactionV1,null);
    skipNative=stopAfterClaim=false;client().apply(plan(cube),[],r=>{result=r;});drain();eq(result.ok,true);eq(prepares,1);
}
// Reloading a page can safely recover a stalled pre-mutation job, but never
// replace an applying job whose native rollback/publication may be in progress.
reset();call("beginModelGraphTransaction",{...plan(cube),assets:[]},r=>eq(r.ok,true));
now+=15001;call("beginModelGraphTransaction",{...plan(cube),assets:[]},r=>eq(r.ok,true));eq(writes,0);
const active=host.__SFLD_modelGraphTransactionV1;now+=5*60*1000+1;
for(const phase of ["executing","applying"]){active.state=phase;
    call("beginModelGraphTransaction",{...plan(cube),assets:[]},r=>{eq(r.error.code,"model_transaction_busy");ok(r.error.message.includes("state="+phase));});
    eq(host.__SFLD_modelGraphTransactionV1,active);eq(writes,0);
    call("releaseModelGraphTransaction",{},r=>eq(r.ok,true));eq(host.__SFLD_modelGraphTransactionV1,active);eq(active.cancelled,true);}
active.message="The renderer is busy or its guard changed.";active.notificationError="Invalid native Model result encoding.";
call("beginModelGraphTransaction",{...plan(cube),assets:[]},r=>{eq(r.error.code,"model_transaction_busy");ok(r.error.message.includes(active.message));ok(r.error.message.includes(active.notificationError));});
call("readModelGraphTransaction",{},r=>{eq(r.message,active.message);eq(r.notificationError,active.notificationError);eq(r.state,"applying");});
reset();const edit={type:"applyPreset",presetGraph:imported,presetModelAssets:[{nodeId:modelId,revision:17,bounds,meshHex}],mode:"add"};
const updated=presets.apply(cube,edit,()=>uuid(++sequence));ok(edit.modelAssetsToRestore[0].nodeId!==modelId);eq(edit.modelAssetsToRestore[0].source,2);
const tx=graphTransactions.create({codec,edits:{apply:presets.apply},call,modelTransactions:client(),idFactory:()=>uuid(++sequence)});
tx.apply({type:"applyPreset",presetGraph:imported,presetModelAssets:[{nodeId:modelId,revision:17,bounds,meshHex}],mode:"replace"},r=>{result=r;},token,11,{ok:true,target:{token},snapshot:copy(snapshot())});
drain();eq(result.ok,true,JSON.stringify(result));eq(hostRuns,1);ok(Object.keys(nativeMeshes)[0]!==modelId);
reset();tx.apply({type:"applyPreset",presetGraph:imported,presetModelAssets:[{nodeId:modelId,revision:17,bounds,meshHex}],mode:"add"},r=>{result=r;},token,11,{ok:true,target:{token},snapshot:copy(snapshot())});
drain();eq(result.ok,true,JSON.stringify(result));eq(nodes.filter(n=>n.type===edits.types.model).length,2);eq(hostRuns,1);
for(const sourceGraph of [imported,cube]){reset(sourceGraph);nativeMeshes[modelId]=copy({...meshAsset,source:sourceGraph===cube?1:2});
    const duplicateClient=graphTransactions.create({codec,edits,call,modelTransactions:client(),idFactory:()=>uuid(++sequence)});
    duplicateClient.apply({type:"duplicateNodes",nodeIds:[modelId]},r=>{result=r;},token,11,{ok:true,target:{token},snapshot:copy(snapshot())});
    drain();eq(result.ok,true,JSON.stringify(result));eq(nodes.filter(n=>n.type===edits.types.model).length,2);eq(Object.keys(nativeMeshes).length,2);eq(hostRuns,1);
    const copiedAsset=nativeMeshes[Object.keys(nativeMeshes).find(id=>id!==modelId)];eq(copiedAsset.meshHex,meshHex);eq(copiedAsset.source,sourceGraph===cube?1:2);}
reset();const duplicateEdit={type:"duplicateNodes",nodeIds:[modelId],modelAssets:{[modelId]:{revision:17,bounds}}};const duplicated=edits.apply(imported,duplicateEdit,()=>uuid(++sequence));
eq(duplicateEdit.modelResourceCopies.length,1);const copied=duplicated.nodes.find(n=>n.id===duplicateEdit.modelResourceCopies[0].nodeId);
eq(copied.parameters.find(p=>p.key==="1").value,Array.from(Buffer.from(copied.id,"hex")));codec.toHex(duplicated);
reset();let begin=JSON.parse(host.SFLD_beginModelGraphTransaction(request("beginModelGraphTransaction",{...plan(),assets:[{...meshAsset,bytes:meshHex.length/2}]})));eq(begin.ok,true);eq(writes,0);
eq(JSON.parse(host.SFLD_beginModelGraphTransaction(request("beginModelGraphTransaction",{...plan(),assets:[]}))).ok,false);
for(const bad of [{asset:1,page:0,hex:meshHex},{asset:0,page:1,hex:meshHex},{asset:0,page:0,hex:"ab"},{asset:0,page:0,hex:meshHex.toUpperCase()}]){
    reset();call("beginModelGraphTransaction",{...plan(),assets:[{...meshAsset,bytes:meshHex.length/2}]},r=>eq(r.ok,true));
    call("writeModelGraphAssetPage",bad,r=>eq(r.ok,false));eq(writes,0);eq(hostRuns,0);}
for(const mutate of [p=>p.assets[0].bytes=8388609,p=>p.assets[0].revision=2147483648,p=>p.assets[0].source=1,p=>p.assets.push(p.assets[0]),
    p=>p.assets[0].bounds=[0,0,0,0,0,0],p=>p.target={token:"p101-c202-l304"}]){
    reset();const p={...plan(),assets:[{...meshAsset,bytes:meshHex.length/2}]};mutate(p);call("beginModelGraphTransaction",p,r=>eq(r.ok,false));eq(writes,0);eq(hostRuns,0);}
reset();client().apply(plan(),[{...meshAsset,meshHex:meshHex.slice(0,-2)+"00"}],r=>{result=r;});drain();eq(result.ok,false);eq(hostRuns,0);eq(writes,0);
reset();const cancel=client().apply(plan(),[meshAsset],r=>{result=r;});cancel.cancel();drain();eq(result.ok,false);eq(hostRuns,0);eq(writes,0);
reset();client().apply(plan(),[meshAsset],r=>{result=r;});stamp="changed";drain();eq(result.ok,false);eq(hostRuns,0);eq(writes,0);
reset();loseResult=true;client().apply(plan(),[meshAsset],r=>{result=r;});
for(let step=0;step<12 && tasks.length;step++)tasks.shift()();now+=5*60*1000+1;drain();eq(result.committed,true);eq(result.diagnostics.notificationPending,true);eq(hostRuns,1);
reset();loseResult=true;failedPrepare=true;client().apply(plan(),[meshAsset],r=>{result=r;});
for(let step=0;step<12 && tasks.length;step++)tasks.shift()();now+=5*60*1000+1;drain();
eq(result.error.code,"model_transaction_outcome_unknown");ok(result.error.message.includes("state=applying"));ok(result.error.message.includes("prepare failure"));eq(hostRuns,1);
// Re-evaluating the helper preserves only plain staged state and no DOM refs.
reset();let lateBegin;
const delayed=clientAPI.create({schedule:fn=>tasks.push(fn),now:()=>now,idFactory:()=>transactionId,call(operation,fields,callback){
    if(operation==="beginModelGraphTransaction")call(operation,fields,r=>{lateBegin=()=>callback(r);});else call(operation,fields,callback);}});
const lateCancel=delayed.apply(plan(),[meshAsset],r=>{result=r;});lateCancel.cancel();ok(host.__SFLD_modelGraphTransactionV1);
lateBegin();eq(host.__SFLD_modelGraphTransactionV1,null);eq(hostRuns,0);eq(writes,0);
reset();call("beginModelGraphTransaction",{...plan(),assets:[]},r=>eq(r.ok,true));const saved=host.__SFLD_modelGraphTransactionV1;
vm.runInContext(fs.readFileSync(path.join(root,"jsx/model_transaction_transport.jsx"),"utf8"),context);eq(host.__SFLD_modelGraphTransactionV1,saved);eq(writes,0);
// AEGP and CEP independently evaluated scripts can have different Array
// prototypes. A primitive result string must cross that boundary unchanged.
saved.state="executing";
const otherContext=vm.createContext({resultEntry:host.SFLD_modelTransactionHostResult,id:transactionId});
eq(vm.runInContext('resultEntry(id,"0|2|512|0|0|0|0|0|-1")',otherContext),"1");
eq(saved.state,"complete");eq(saved.result.error.code,"graph_commit_failed");eq(saved.diagnostics.stage,2);
eq(vm.runInContext('resultEntry(id,"0|2|512|0|0|0|0|0|-1")',otherContext),"1");
eq(saved.state,"complete");eq(writes,0);
eq(host.SFLD_modelTransactionHostResult(uuid(999),"0|2|512|0|0|0|0|0|-1"),"2");eq(host.__SFLD_modelGraphTransactionV1,saved);
eq(host.SFLD_modelTransactionHostResult(transactionId,"0|3|516|0|0|0|0|0|-1"),"0");eq(saved.diagnostics.stage,2);
for(const invalid of [[0,2,512,0,0,0,0,0,-1],null,"0|2|512", "0|2|2147483648|0|0|0|0|0|-1",
    "2|2|512|0|0|0|0|0|-1","0|8|512|0|0|0|0|0|-1","0|2|512|0|0|0|0|9|-1","0|2|512|0|0|0|0|0|63"]){
    eq(host.SFLD_modelTransactionHostResult(transactionId,invalid),"0");eq(saved.state,"complete");eq(saved.diagnostics.stage,2);
}
console.log(`model_graph_transport_tests: ${checks} checks passed; actual candidate protocol/planner, simulated DOM/native writes; AE qualification open`);
