"use strict";
// Real gateway materialization/author callbacks with an indexed-group DOM.
// Mesh writes, native graph receipts and outer backup/undo are simulated.
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=process.env.STARFIELD_PANEL_ROOT;if(!root)throw new Error("Set STARFIELD_PANEL_ROOT to the isolated Model candidate.");
const gatewayBuild=/var GATEWAY_BUILD = "([^"]+)"/.exec(fs.readFileSync(path.join(root,"jsx/starfield_gateway.jsx"),"utf8"))[1];
const edits=require(path.join(root,"js/graph_edits.js")),codec=require(path.join(root,"js/graph_codec.js")),layout=require(path.join(root,"js/graph_layout.js"));
const {mainControls,nodeControls}=require("./node_property_fixture.js"),uuid=n=>n.toString(16).padStart(32,"0"),token="p0-c202-l303";
const copy=x=>JSON.parse(JSON.stringify(x,(_,v)=>ArrayBuffer.isView(v)?Array.from(v):v));
const stableEffects=process.env.STARFIELD_DOM_STABLE_EFFECTS==="1";
const sharedEffects=process.env.STARFIELD_DOM_SHARED_EFFECTS==="1",sharedWrappers=new Map();
const fullGraph=process.env.STARFIELD_DOM_FULL_GRAPH==="1";
const reorderedConnections=process.env.STARFIELD_DOM_REORDER_CONNECTIONS==="1";
const staleGatewayAPI=process.env.STARFIELD_DOM_STALE_GATEWAY_API==="1";
const legacyParticleLabel=process.env.STARFIELD_DOM_LEGACY_PARTICLE_LABEL==="1";
const legacyKind=process.env.STARFIELD_DOM_LEGACY_KIND || (legacyParticleLabel?"particle":"");
const catalogDir=process.env.STARFIELD_NODE_CATALOG_DIR;
const invalidateCommit=process.env.STARFIELD_DOM_INVALIDATE_COMMIT==="1";
const reloadOnCommit=process.env.STARFIELD_DOM_RELOAD_ON_COMMIT==="1";
const hasMotion=process.env.STARFIELD_DOM_MOTION==="1";
const deferredMotion=process.env.STARFIELD_DOM_DEFERRED_MOTION==="1",pendingMotionDeliveries=[];
const staleGuard=process.env.STARFIELD_DOM_STALE_GUARD==="1";
let guardGeneration=0,ignoreGuardClear=false,ignoreGuardAcquire=false,restoreGuardOnUndo=false;
let guardDisplay=null,guardSamplerFailure=false;
let checks=0,epoch=0,arbReads=0,commands=0,undo=0,rejectCommit=false,commits=0,staleReads=0;
const deletionRender=process.env.STARFIELD_DOM_DELETE_RENDER==="1";
let publishedBindings=[],missingRenderBindings=0,physicalRemovals=0;
const rawUuid=e=>Array.from({length:8},(_,i)=>e.properties["Node UUID "+i].value.toString(16).padStart(4,"0")).join("");
function renderAtRemoval(){if(deletionRender)for(const id of publishedBindings)
    if(!items.some(e=>e.matchName!==renderer.matchName&&rawUuid(e)===id))missingRenderBindings++;}
const eq=(a,b,m)=>{checks++;assert.deepEqual(a,b,m);},ok=v=>{checks++;assert.ok(v);};
const renderer={matchName:"org.starfieldfx.particle",name:"Starfield",properties:mainControls()},items=[renderer];
function fresh(captured){if(captured!==epoch){staleReads++;throw new ReferenceError("Object is invalid");}}
function property(p,captured){const guardCaptured=guardGeneration;return {name:p.name,matchName:p.matchName,propertyIndex:p.propertyIndex,numKeys:0,canSetExpression:false,
    valueAtTime(time,preExpression){fresh(captured);assert.equal(time,0);assert.equal(preExpression,true);
        if(guardSamplerFailure && p.name==="Panel Graph Sync Guard")throw new Error("Guard sampler unavailable");return copy(p.value);},
    get value(){fresh(captured);if(p.name==="Panel Graph Sync Guard" && guardDisplay!==null)return guardDisplay;
        if(staleGuard && p.name==="Panel Graph Sync Guard" && guardCaptured!==guardGeneration)return 1;return copy(p.value);},setValue(value){fresh(captured);
        if(ignoreGuardClear && p.name==="Panel Graph Sync Guard" && value===0)return;
        if(ignoreGuardAcquire && p.name==="Panel Graph Sync Guard" && value===1)return;
        if(p.minimum!==undefined)assert.ok(Number.isInteger(value) && value>=p.minimum && value<=p.maximum,
            p.name+": popup value "+value+" outside "+p.minimum+".."+p.maximum);
        if(p.color)assert.ok(Array.isArray(value) && value.length===4 && value.every(v=>Number.isFinite(v) && v>=0 && v<=1),
            p.name+": AE color requires four unit-interval components");
        if(deferredMotion && p.name==="Motion Type")pendingMotionDeliveries.push({property:p,previous:copy(p.value)});
        p.value=copy(value);}};}
function wrapInstance(raw){
    // Reacquiring an effect may reuse its wrapper identity while earlier child
    // Property handles remain invalid. Exercise both wrapper policies rather
    // than relying on JavaScript object identity to validate an AE handle.
    if(stableEffects && raw.wrapper){raw.wrapperEpoch=epoch;return raw.wrapper;}
    const originalEpoch=epoch,ordered=Array.from(new Set(Object.values(raw.properties)));
    if(catalogDir)ordered.sort((a,b)=>a.propertyIndex-b.propertyIndex);
    raw.wrapperEpoch=epoch;
    const captured=()=>stableEffects?raw.wrapperEpoch:originalEpoch;
    const wrapped={get selected(){return !!raw.selected;},set selected(v){raw.selected=v;},
        get matchName(){fresh(captured());return raw.matchName;},get name(){fresh(captured());return raw.name;},set name(v){fresh(captured());raw.name=v;},
        get numProperties(){fresh(captured());return ordered.length;},property(key){fresh(captured());
            if(key==="Model Mesh" || key==="org.starfieldfx.node.model-1503"){arbReads++;throw new Error("CUSTOM_VALUE forbidden");}
            const p=raw.properties[key] || (typeof key==="number"?ordered[key-1]:null);return p?property(p,captured()):null;},
        remove(){fresh(captured());renderAtRemoval();items.splice(items.indexOf(raw),1);epoch++;physicalRemovals++;renderAtRemoval();}};
    if(stableEffects)raw.wrapper=wrapped;
    return wrapped;}
function wrap(raw){
    if(raw!==renderer)guardGeneration++;
    const current=wrapInstance(raw);if(!sharedEffects)return current;
    let slot=sharedWrappers.get(raw.matchName);
    if(!slot){slot={current};slot.proxy=new Proxy({}, {
        get(_target,key){const value=slot.current[key];return typeof value==="function"?value.bind(slot.current):value;},
        set(_target,key,value){slot.current[key]=value;return true;}
    });sharedWrappers.set(raw.matchName,slot);}
    slot.current=current;return slot.proxy;
}
function modelControls(){const properties=nodeControls();
    function add(name,disk,value){const p={name,matchName:"org.starfieldfx.node.model-"+disk,value};properties[name]=properties[p.matchName]=p;}
    add("Source",1501,1);add("Mesh Revision",1504,0);
    for(const [start,name,value] of [[5,"Offset",0],[8,"Angle",0],[11,"Scale",100]])for(let a=0;a<3;a++)add(name+" "+"XYZ"[a],1500+start+a,value);
    for(const [i,name] of ["Flip X","Flip Y","Flip Z","Center","Normalize"].entries())add(name,1514+i,0);
    for(const [i,name] of ["Mesh Min X","Mesh Min Y","Mesh Min Z","Mesh Max X","Mesh Max Y","Mesh Max Z"].entries())add(name,1519+i,i<3?-.5:.5);
    return properties;}
function registeredControls(matchName){if(!catalogDir)return modelControls();
    const kind=matchName.substr(matchName.lastIndexOf(".")+1),raw=fs.readFileSync(path.join(catalogDir,"m3-17-full-audit-catalog-"+kind+".log")),
        log=raw.toString(raw[0]===255 && raw[1]===254?"utf16le":"utf8").replace(/^\uFEFF/,"");
    const line=log.split(/\r?\n/).find(line=>line.startsWith("NODE_CONTROL_CATALOG:"));assert.ok(line,"Missing actual registration catalog for "+kind);
    const catalog=JSON.parse(line.substr("NODE_CONTROL_CATALOG:".length)),properties={};assert.equal(catalog.kind,kind);
    for(const c of catalog.controls){const p={name:c.name,matchName:matchName+"-"+c.disk,propertyIndex:c.index,value:c.value,
        minimum:c.minimum,maximum:c.maximum,color:c.type===5};
        properties[c.name]=properties[p.matchName]=properties[matchName+"-"+String(c.disk).padStart(4,"0")]=p;}
    return properties;}
function parade(){return {get numProperties(){return items.length;},property(i){return items[i-1]?wrap(items[i-1]):null;},
    addProperty(matchName){const properties=registeredControls(matchName);
        if(deferredMotion && matchName==="org.starfieldfx.node.motion")properties["Motion Type"].value=2;
        items.push({matchName,name:"",properties});epoch++;return wrap(items[items.length-1]);}};}
const layer={id:303,name:"Particles",selected:true,width:1920,height:1080,time:0,source:{pixelAspect:1},property:name=>name==="ADBE Effect Parade"?parade():null};
Object.defineProperty(layer,"selectedProperties",{get:()=>items.filter(e=>e.selected).map(wrap)});
const sourceTransform={property:name=>name==="ADBE Anchor Point"?{value:[0,0,0]}:null};
const front={id:44,name:"Front",index:2,width:320,height:180,source:{pixelAspect:1},hasVideo:true,property:()=>sourceTransform},
    back={id:66,name:"Back",index:3,width:320,height:180,source:{pixelAspect:1},hasVideo:true,property:()=>sourceTransform};
function CompItem(){}const comp=new CompItem();Object.assign(comp,{id:202,name:"Comp",time:0,numLayers:3,layer:i=>[layer,front,back][i-1]});
Object.defineProperty(comp,"selectedLayers",{get:()=>[layer]});
for(const l of [layer,front,back])l.containingComp=comp;layer.index=1;
const host={},context=vm.createContext({CompItem,$:{global:host},app:{project:{activeItem:comp,rootFolder:{id:0},numItems:1,item:()=>comp},
    beginUndoGroup(){undo++;},endUndoGroup(){undo++;if(restoreGuardOnUndo){restoreGuardOnUndo=false;renderer.properties["Panel Graph Sync Guard"].value=1;}},findMenuCommandId:name=>name==="Starfield Commit Node Graph"?99:55,
    executeCommand(id){commands++;if(id!==99)return;
        if(!rejectCommit)for(const effect of items)if(effect.matchName==="org.starfieldfx.node.motion") {
            const guard=effect.properties["Panel Sync Guard"],mode=effect.properties["Motion Type"];
            if(guard && guard.value>=4 && guard.value<=6 && Number.isInteger(guard.value)) {
                mode.value=guard.value===5?1:guard.value===4?2:3;guard.value=0;
            }
        }
        // Model a delayed supervised delivery from a prior script popup setter.
        // This is an adversarial lifetime/order fixture, not proof of AE's cause.
        if(!rejectCommit)while(pendingMotionDeliveries.length){const delivery=pendingMotionDeliveries.shift();delivery.property.value=delivery.previous;}
        commits++;const resolved=api.resolve({pinTarget:true,target:{token}}),value=Number(resolved.properties.commit.value);
        resolved.properties.receipt.setValue(rejectCommit?-value:value);if(!rejectCommit)resolved.properties.revision.setValue(Number(resolved.properties.revision.value)+1);
        if(deletionRender&&!rejectCommit)publishedBindings=copy(api.snapshot(resolved)).nativeNodes.map(n=>n.id);
        if(invalidateCommit)epoch++;
        if(reloadOnCommit){vm.runInContext(gatewaySource,context);api=host.__SFLD_modelTransactionAPI;}}}});
const realm=x=>vm.runInContext("("+JSON.stringify(x)+")",context);
const dialectSource=source=>process.env.STARFIELD_EXTENDSCRIPT_CONDITIONAL_MODEL==="1"?
    require("./extendscript_conditionals.cjs").modelLeftAssociation(source):source;
const gatewaySource=dialectSource(fs.readFileSync(path.join(root,"jsx/starfield_gateway.jsx"),"utf8"));
// The session engine survives CEP reloads. A transport may still close over a
// previous gateway even after SFLD_ready and the public API have been replaced.
const oldGatewaySource=gatewaySource.replace('schemaVersion: nativeRecords.version(type)',
    'schemaVersion: type === "org.starfieldfx.nodes.'+(legacyKind || "particle")+'" ? '+((legacyKind==="force")?1:3)+' : nativeRecords.version(type)');
if(staleGatewayAPI || legacyKind)assert.notEqual(oldGatewaySource,gatewaySource);
vm.runInContext(staleGatewayAPI || legacyKind?oldGatewaySource:gatewaySource,context);
vm.runInContext(dialectSource(fs.readFileSync(path.join(root,"jsx/model_transaction_transport.jsx"),"utf8")),context);
if(staleGatewayAPI)vm.runInContext(gatewaySource,context);
let api=host.__SFLD_modelTransactionAPI;
if(staleGatewayAPI && typeof host.SFLD_modelTransactionReady==="function")eq(host.SFLD_modelTransactionReady(),"");
let resolved=api.resolve({pinTarget:true,target:{token}});ok(!resolved.error,JSON.stringify(resolved.error));
resolved.properties.nodeEffectsReady.setValue(1);resolved.properties.revision.setValue(11);
let g=fullGraph?require(path.join(root,"js/presets.js")).build("sparks",1080):{version:1,nodes:[],edges:[],optionalRecords:[]};
if(fullGraph){
    g=edits.apply(g,{type:"addNode",nodeType:"particle"},()=>uuid(102));
    g=edits.apply(g,{type:"connect",from:g.nodes.find(n=>n.type===edits.types.emitter).id,to:uuid(102)},()=>uuid(103));
    g=edits.apply(g,{type:"connect",from:uuid(102),to:g.nodes.find(n=>n.type===edits.types.output).id},()=>uuid(104));
    g=edits.apply(g,{type:"addNode",nodeType:"emitter"},()=>uuid(105));
    g=edits.apply(g,{type:"connect",from:uuid(105),to:uuid(102)},()=>uuid(106));
    g=edits.apply(g,{type:"addNode",nodeType:"transform"},()=>uuid(107));
    for(const n of g.nodes.filter(n=>n.type===edits.types.particle))for(const [key,value] of [[31,44],[32,66]])n.parameters.find(p=>p.key===String(key)).value=value;
}
g=edits.apply(g,{type:"addNode",nodeType:"model",position:{x:100.123456789,y:50.234567891}},()=>uuid(101));
if(!fullGraph)g=edits.apply(g,{type:"addNode",nodeType:"output"},()=>uuid(255));
if(process.env.STARFIELD_DOM_MATERIAL==="1"){
    g=edits.apply(g,{type:"addNode",nodeType:"material"},()=>uuid(108));
    g=edits.apply(g,{type:"connect",from:uuid(101),to:uuid(108)},()=>uuid(109));
    g=edits.apply(g,{type:"connect",from:uuid(108),to:g.nodes.find(n=>n.type===edits.types.output).id},()=>uuid(110));
    // Exercise the actual new-node default before changing to independent color.
}
if(hasMotion)g=edits.apply(g,{type:"addNode",nodeType:"motion"},()=>uuid(111));
const record=n=>({id:n.id,type:n.type,schemaVersion:n.schemaVersion,parameters:copy(n.parameters),position:layout.resolve(g)[n.id],
    outgoing:g.edges.filter(e=>e.sourceNode===n.id).map(e=>({id:e.id,target:e.destinationNode})).sort((a,b)=>a.id.localeCompare(b.id))});
api.ensure(layer,realm(g.nodes.filter(n=>n.type!==edits.types.output && n.type!==edits.types.material).map(n=>{
    const r=record(n);r.outgoing=r.outgoing.filter(e=>e.target!==uuid(108));return r;
})),true,realm([]),false);eq(arbReads,0);
// This fixture starts from an already initialized project. Complete the initial
// fake native author phase before taking its base receipt; pending author intent
// is not an initialized saved Motion record. Later scripted setters still queue
// the adversarial deferred deliveries exercised at real command boundaries.
for(const effect of items)if(effect.matchName==="org.starfieldfx.node.motion"){
    const guard=effect.properties["Panel Sync Guard"];
    if(guard.value>=4 && guard.value<=6){effect.properties["Motion Type"].value=guard.value===5?1:guard.value===4?2:3;guard.value=0;}
}
pendingMotionDeliveries.length=0;
const bounds=[-2,-3,-4,2,3,4],raw=Buffer.alloc(48);bounds.forEach((v,i)=>raw.writeDoubleLE(v,i*8));
let transaction=500;
function request(operation,fields={}){return JSON.stringify({protocol:"org.starfieldfx.panel",version:1,gatewayBuild:gatewayBuild,operation,requestId:"test",pinTarget:true,
    target:{token},transactionId:uuid(transaction),changes:[],...fields});}
function begin(replace){resolved=api.resolve({pinTarget:true,target:{token}});const before=copy(api.snapshot(resolved));
    if(deletionRender)publishedBindings=before.nativeNodes.map(n=>n.id);
    const desired=copy(before.nativeNodes.find(n=>n.type===edits.types.model)),previousModelId=desired.id;desired.id=uuid(++transaction);desired.position={x:160.456789123,y:90.567891234};
    if(!replace)desired.outgoing=[];
    for(const [key,value] of [[1,Array.from(Buffer.from(desired.id,"hex"))],[2,17],[12,Array.from(raw)],[13,1],[5,[0,45,0]]])desired.parameters.find(p=>p.key===String(key)).value=value;
    const nodes=replace?before.nativeNodes.filter(n=>n.type!==edits.types.model).concat([desired]):before.nativeNodes.concat([desired]);
    if(process.env.STARFIELD_DOM_MATERIAL==="1"){
        if(replace)desired.outgoing=record(g.nodes.find(n=>n.type===edits.types.model)).outgoing;
        let material=nodes.find(n=>n.type===edits.types.material);
        if(!material){material=record(g.nodes.find(n=>n.type===edits.types.material));nodes.push(material);}
        if(!replace)for(const [key,value] of [[1,1],[2,[.2,.4,.8]],[3,.75],[4,.3],[5,.4],[6,0]])
            material.parameters.find(p=>p.key===String(key)).value=value;
    }
    if(replace)for(const n of nodes)for(const e of n.outgoing)if(e.target===previousModelId)e.target=desired.id;
    if(hasMotion){const motion=nodes.find(n=>n.type===edits.types.motion);
        for(const [key,value] of [[13,2],[14,[-30,45,-60]],[15,[.2,-.3,.4]],[4,6*Math.PI],[17,300],[5,25]])motion.parameters.find(p=>p.key===String(key)).value=value;
        motion.parameters=motion.parameters.filter(p=>p.key!=="6");motion.parameters.push({key:"6",type:7,value:Array.from(require(path.join(root,"js/graph_view.js")).encodeCurve([{age:0,value:25},{age:1,value:75}]))});}
    for(const node of nodes)node.schemaVersion=edits.nativeRecordSchema(node);
    if(reorderedConnections)for(const node of nodes)node.outgoing=node.outgoing.slice().reverse().map(edge=>({target:edge.target,id:edge.id}));
    const r=JSON.parse(host.SFLD_beginModelGraphTransaction(request("beginModelGraphTransaction",{baseGraphRevision:before.revision,baseRecordStamp:before.recordStamp,
        graphHex:codec.toHex(g),nodeManifest:nodes,rendererManifest:before.renderer,assets:[{nodeId:desired.id,source:2,revision:17,bounds,bytes:32}]})));
    eq(r.ok,true,JSON.stringify(r));return {desired,before,nodes,id:uuid(transaction)};}
function stage(pair){const {id,desired}=pair;eq(JSON.parse(host.SFLD_writeModelGraphAssetPage(request("writeModelGraphAssetPage",{asset:0,page:0,hex:"ab".repeat(32)}))).ok,true);
    eq(JSON.parse(host.SFLD_queueModelGraphTransaction(request("queueModelGraphTransaction"))).ok,true);eq(commands,1);
    ok(host.SFLD_modelTransactionHostRequest().startsWith(id+"|0|202|303|1|"));
    eq(host.__SFLD_modelGraphTransactionV1.state,"queued");eq(host.SFLD_modelTransactionHostClaim(id),"1");
    eq(host.SFLD_modelTransactionHostBegin(id),"1");
    // Simulate the native complete backup: a disabled guard2 renderer and node
    // remain in the parade while the live renderer is held at guard1.
    const backupMain={matchName:renderer.matchName,name:renderer.name,properties:copy(renderer.properties)},
        backupNode={matchName:items[1].matchName,name:items[1].name,properties:copy(items[1].properties)};
    for(const p of new Set(Object.values(backupMain.properties)))if(p.name==="Panel Graph Sync Guard")p.value=2;
    for(const p of new Set(Object.values(backupNode.properties)))if(p.name==="Panel Sync Guard")p.value=2;
    for(let i=0;i<8;i++)backupNode.properties["Node UUID "+i].value=parseInt(uuid(700).substr(i*4,4),16);
    items.push(backupMain,backupNode);epoch++;
    resolved=api.resolve({pinTarget:true,target:{token}});resolved.properties.guard.setValue(1);
    eq(host.SFLD_modelTransactionHostPrepare(id),"1",host.__SFLD_modelGraphTransactionV1.message);
    if(process.env.STARFIELD_DOM_MATERIAL==="1"){
        const material=items.find(e=>e.matchName==="org.starfieldfx.node.material");
        const fields=pair.nodes.find(n=>n.type===edits.types.material).parameters,field=k=>fields.find(p=>p.key===String(k)).value;
        eq(material.properties["Color Source"].value,field(1)+1);
        eq(material.properties.Color.value,[...field(2),1]);
        for(const [key,name] of [[3,"Roughness"],[4,"Metallic"],[5,"Opacity"],[6,"AE Lights"]])
            eq(material.properties[name].value,field(key)*(key===6?1:100));
    }
    if(typeof host.SFLD_modelTransactionReady==="function")eq(host.SFLD_modelTransactionReady(),gatewayBuild);
    if(hasMotion){const motion=items.find(e=>e.matchName==="org.starfieldfx.node.motion"),field=k=>pair.nodes.find(n=>n.type===edits.types.motion).parameters.find(p=>p.key===String(k)).value;
        if(!deferredMotion)eq(motion.properties["Motion Type"].value,1);
        ok([0,5].includes(motion.properties["Panel Sync Guard"].value),"prepared Circle is already saved or retains explicit native intent");
        eq(motion.properties["Origin Type"].value,3);
        eq(motion.properties["Speed"].value,300);eq(motion.properties["Speed Random"].value,25);
        eq(motion.properties["Angle X"].value,30);eq(motion.properties["Angle Y"].value,45);eq(motion.properties["Angle Z"].value,60);
        eq(motion.properties["Offset X"].value,216);eq(motion.properties["Offset Y"].value,324);eq(motion.properties["Offset Z"].value,432);
        eq(motion.properties["Motion Over Life"].value,2);eq(motion.properties["Motion Over Life Curve 1 Value"].value,75);
        const read=copy(api.snapshot(api.resolve({pinTarget:true,target:{token}}))).nativeNodes.find(n=>n.type===edits.types.motion);
        for(const p of read.parameters)eq(p.value,field(p.key));}
    let current=copy(api.snapshot(api.resolve({pinTarget:true,target:{token}})));const model=current.nativeNodes.find(n=>n.id===desired.id);
    eq(model.parameters.find(p=>p.key==="13").value,0);eq(model.modelAsset.revision,0);eq(arbReads,0);
    const effect=items.find(e=>e.matchName==="org.starfieldfx.node.model" && e.properties["Panel Sync Guard"].value!==2 &&
        Array.from({length:8},(_,i)=>e.properties["Node UUID "+i].value.toString(16).padStart(4,"0")).join("")===desired.id);
    effect.properties.Source.value=2;effect.properties["Mesh Revision"].value=17;
    for(const [i,name] of ["Mesh Min X","Mesh Min Y","Mesh Min Z","Mesh Max X","Mesh Max Y","Mesh Max Z"].entries())effect.properties[name].value=bounds[i];
    return {effect,backupMain,backupNode};}
let pair=begin(true),copies=stage(pair);eq(host.SFLD_modelTransactionHostCommit(pair.id),"1",host.__SFLD_modelGraphTransactionV1.message);eq(commits,1);eq(arbReads,0);eq(undo,0);
eq(copies.effect.properties["Angle Y"].value,45);eq(copies.effect.properties["Panel Sync Guard"].value,0);
eq(copy(api.snapshot(api.resolve({pinTarget:true,target:{token}}))).nativeNodes.length,pair.nodes.length);
eq(host.SFLD_modelTransactionHostResult(pair.id,"1|7|0|0|0|0|0|0|-1"),"1");
eq(JSON.parse(host.SFLD_readModelGraphTransaction(request("readModelGraphTransaction"))).result.committed,true);
eq(JSON.parse(host.SFLD_releaseModelGraphTransaction(request("releaseModelGraphTransaction"))).ok,true);
items.splice(items.indexOf(copies.backupMain),1);items.splice(items.indexOf(copies.backupNode),1);epoch++;
api.resolve({pinTarget:true,target:{token}}).properties.guard.setValue(0);commands=0;
if(sharedEffects){
    // Different same-kind effects share a host wrapper, while their children
    // still belong to the particular instance acquired when property() ran.
    // Distinct Source values expose both cached-child contamination and the
    // effect reference retained while enumerating later siblings.
    const obj=copies.effect,cube={matchName:obj.matchName,name:"Independent Cube",properties:structuredClone(obj.properties)};
    const cubeId=uuid(880);cube.properties.Source.value=1;cube.properties["Mesh Revision"].value=0;
    for(let i=0;i<8;i++)cube.properties["Node UUID "+i].value=parseInt(cubeId.substr(i*4,4),16);
    items.push(cube);epoch++;
    const records=copy(api.snapshot(api.resolve({pinTarget:true,target:{token}}))).nativeNodes;
    eq(records.find(n=>n.id===rawUuid(obj)).parameters.find(p=>p.key==="13").value,1,"OBJ source remains on its own UUID");
    eq(records.find(n=>n.id===cubeId).parameters.find(p=>p.key==="13").value,0,"Cube source remains on its own UUID");
    // Selection needs only the common immutable record, so also exercise the
    // Turbulence route without inventing a second parameter registration catalog.
    const turbulence={matchName:"org.starfieldfx.node.turbulence",name:"Turbulence selection fixture",properties:nodeControls()};
    for(let i=0;i<8;i++)turbulence.properties["Node UUID "+i].value=parseInt(uuid(881).substr(i*4,4),16);
    items.push(turbulence);epoch++;
    const before=items.map(e=>copy(e.properties));
    for(const target of [...items.filter(e=>e!==renderer),renderer]){
        const id=target===renderer?uuid(255):rawUuid(target);
        const result=JSON.parse(host.SFLD_selectNodeEffect(request("selectNodeEffect",{nodeId:id})));
        eq(result.ok,true,JSON.stringify(result));eq(items.filter(e=>e.selected),[target],"selection follows complete UUID for "+target.matchName);
    }
    eq(items.map(e=>copy(e.properties)),before,"navigation never changes authored controls");
    items.splice(items.indexOf(cube),1);items.splice(items.indexOf(turbulence),1);epoch++;commands=0;
}
if(process.env.STARFIELD_DOM_MATERIAL==="1"){
    const rawMaterial=items.find(e=>e.matchName==="org.starfieldfx.node.material"),beforeValues=copy(rawMaterial.properties);
    // Exercise the public transaction boundary; private control readers/writers
    // are deliberately not exported by the production gateway.
    const readNodes=()=>copy(api.snapshot(api.resolve({pinTarget:true,target:{token}}))).nativeNodes;
    const records=readNodes();
    for(const value of [2,4294967295]){
        const bad=copy(records);bad.find(n=>n.type===edits.types.material).parameters.find(p=>p.key==="1").value=value;
        checks++;assert.throws(()=>api.ensure(layer,realm(bad),true,realm(records),false),/material switch 1/);
        eq(copy(rawMaterial.properties),beforeValues);
    }
    rawMaterial.properties["Color Source"].value=0;
    checks++;assert.throws(readNodes,/Invalid native Material Color Source: 0/);
    rawMaterial.properties["Color Source"].value=1;
    const numeric=copy(records);
    for(const field of numeric.find(n=>n.type===edits.types.material).parameters)field.key=Number(field.key);
    api.ensure(layer,realm(numeric),true,realm([]),false);eq(rawMaterial.properties["Color Source"].value,1);
    const coordinator=require(path.join(root,"js/graph_transactions.js")),operations=[];
    const client=coordinator.create({codec,edits,idFactory:()=>uuid(++transaction),call(operation,fields,callback){
        operations.push(operation);
        assert.equal(operation,"submitGraph","Material edits must not copy the effect parade or enter an asset transaction.");
        callback(JSON.parse(host["SFLD_"+operation](request(operation,fields))));
    }});
    let response;const count=items.length;
    const prepared=()=>({ok:true,target:{token},snapshot:copy(api.snapshot(api.resolve({pinTarget:true,target:{token}})))});
    for(const value of [1,0]) {
        const before=prepared();
        client.apply({type:"setParameters",changes:[{nodeId:uuid(108),parameterKey:"1",valueType:3,value}]},r=>{response=r;},token,before.snapshot.revision,before);
        eq(response.ok,true,JSON.stringify(response));eq(rawMaterial.properties["Color Source"].value,value+1);
        eq(items.length,count);eq(host.__SFLD_modelGraphTransactionV1,null);
    }
    const before=prepared();
    client.apply({type:"setParameters",changes:[{nodeId:uuid(108),parameterKey:"2",valueType:5,value:[.4,.6,.2]}]},r=>{response=r;},token,before.snapshot.revision,before);
    eq(response.ok,true,JSON.stringify(response));eq(rawMaterial.properties.Color.value,[.4,.6,.2,1]);
    eq(operations,["submitGraph","submitGraph","submitGraph"]);eq(items.length,count);
    // The subsequent full-asset failure fixture counts only its own commands.
    commands=0;
}
if(hasMotion) {
    const priorUndo=undo;
    const tx=require(path.join(root,"js/graph_transactions.js")),normalizer=require(path.join(root,"js/native_graph_snapshot.js"));
    const motionId=uuid(111),newMotionId=uuid(900);
    const client=tx.create({codec,edits,idFactory:()=>newMotionId,call(operation,fields,callback){
        eq(operation,"submitGraph");callback(JSON.parse(host["SFLD_"+operation](request(operation,fields))));
    }});
    const prepared=()=>normalizer.normalize({ok:true,target:{token},snapshot:copy(api.snapshot(api.resolve({pinTarget:true,target:{token}})))});
    let response;
    const additionClient=tx.create({codec,edits,idFactory:()=>uuid(904),call(operation,fields,callback){
        eq(operation,"submitGraph");callback(JSON.parse(host["SFLD_"+operation](request(operation,fields))));
    }});
    const beforeAddition=prepared();
    additionClient.apply({type:"addNode",nodeType:"motion"},r=>{response=r;},token,beforeAddition.snapshot.revision,beforeAddition);
    eq(response.ok,true,JSON.stringify(response));
    eq(items.find(e=>e.matchName==="org.starfieldfx.node.motion" && rawUuid(e)===uuid(904)).properties["Motion Type"].value,1);
    if(process.env.STARFIELD_DOM_NEGATIVE_ONLY==="1")process.exit(0);
    const cubeClient=tx.create({codec,edits,idFactory:()=>uuid(902),call(operation,fields,callback){
        eq(operation,"submitGraph");callback(JSON.parse(host["SFLD_"+operation](request(operation,fields))));
    }});
    const beforeCube=prepared();
    cubeClient.apply({type:"addNode",nodeType:"model"},r=>{response=r;},token,beforeCube.snapshot.revision,beforeCube);
    eq(response.ok,true,JSON.stringify(response));
    eq(items.find(e=>e.matchName==="org.starfieldfx.node.model" && rawUuid(e)===uuid(902)).properties.Source.value,1);
    eq(arbReads,0,"ordinary Cube add does not read arbitrary mesh bytes");
    const beforeCubeDuplicate=prepared();
    const duplicateClient=tx.create({codec,edits,idFactory:()=>uuid(901),call(operation,fields,callback){
        eq(operation,"submitGraph");callback(JSON.parse(host["SFLD_"+operation](request(operation,fields))));
    }});
    duplicateClient.apply({type:"duplicateNodes",nodeIds:[uuid(902)],offsetX:28,offsetY:28},r=>{response=r;},token,beforeCubeDuplicate.snapshot.revision,beforeCubeDuplicate);
    eq(response.ok,true,JSON.stringify(response));eq(arbReads,0,"default Cube duplication does not export assets");
    for(const [mode,popup] of [[2,3],[1,1],[0,2],[1,1]]) {
        const before=prepared();
        client.apply({type:"setParameters",changes:[{nodeId:motionId,parameterKey:"1",valueType:3,value:mode}]},r=>{response=r;},token,before.snapshot.revision,before);
        eq(response.ok,true,JSON.stringify(response));eq(items.find(e=>e.matchName==="org.starfieldfx.node.motion").properties["Motion Type"].value,popup);
    }
    // Inject a post-publication mode drift after the writer's immediate checks.
    // The real gateway must fail inside its transaction and remove the new node.
    const before=prepared(),beforeCount=items.length,execute=context.app.executeCommand;
    let flip=true;
    context.app.executeCommand=function(id){execute.call(this,id);if(id===99 && flip){
        flip=false;items.filter(e=>e.matchName==="org.starfieldfx.node.motion").at(-1).properties["Motion Type"].value=3;
    }};
    client.apply({type:"addNode",nodeType:"motion"},r=>{response=r;},token,before.snapshot.revision,before);
    context.app.executeCommand=execute;
    eq(response.ok,false);eq(response.error.code,"graph_commit_failed");ok(response.error.message.includes("Published Motion Type differs"));
    const motionEvidence=JSON.parse(response.error.message.split(" Motion readback: ")[1]);
    eq(motionEvidence.node,newMotionId,"failure evidence identifies the added node before rollback");
    eq(motionEvidence.named,3);eq(motionEvidence.disk,3,"failure evidence retains actual native drift before node removal");
    eq(motionEvidence.snapshot,{reader:"valueAtTime(0,true)",raw:3,core:2},"snapshot evidence preserves the original sampled value and mapping");
    ok(/^native-presets-/.test(motionEvidence.gateway));
    eq(items.length,beforeCount,"failed Motion publication removes partially initialized node");
    eq(prepared().snapshot.graphHex,before.snapshot.graphHex,"failed Motion publication restores previous graph");
    eq(prepared().snapshot.nativeNodes.some(n=>n.id===newMotionId),false);
    // Diagnostics are subordinate to rollback even if their serializer fails.
    vm.runInContext("var originalStringify=JSON.stringify; JSON.stringify=function(value){if(value && value.gateway && value.node)throw new Error('diagnostic serializer failed');return originalStringify(value);};",context);
    flip=true;
    context.app.executeCommand=function(id){execute.call(this,id);if(id===99 && flip){
        flip=false;items.filter(e=>e.matchName==="org.starfieldfx.node.motion").at(-1).properties["Motion Type"].value=3;
    }};
    client.apply({type:"addNode",nodeType:"motion"},r=>{response=r;},token,before.snapshot.revision,before);
    context.app.executeCommand=execute;
    vm.runInContext("JSON.stringify=originalStringify;",context);
    eq(response.ok,false);eq(response.error.code,"graph_commit_failed");
    ok(response.error.message.includes("Published Motion Type differs"));
    ok(response.error.message.includes("Motion readback unavailable"));
    eq(items.length,beforeCount,"diagnostic failure still removes the partial node");
    eq(prepared().snapshot.graphHex,before.snapshot.graphHex,"diagnostic failure still restores the previous graph");
    if(process.env.STARFIELD_DOM_GUARD_SAMPLING==="1") {
        const guard=renderer.properties["Panel Graph Sync Guard"];
        const move=(x,base)=>client.apply({type:"moveNodes",positions:{[motionId]:{x,y:75}}},r=>{response=r;},token,base.snapshot.revision,base);
        let base=prepared();guardDisplay=1;move(521,base);
        eq(response.ok,true,"idle saved guard permits edit despite ordinary getter reporting busy");eq(guard.value,0);
        // This also exercises add/materialize/commit/release with a nonzero ordinary getter.
        base=prepared();const beforeAdd=items.length;
        client.apply({type:"addNode",nodeType:"motion"},r=>{response=r;},token,base.snapshot.revision,base);
        eq(response.ok,true);eq(items.length,beforeAdd+1);eq(guard.value,0);
        guardDisplay=0;guard.value=1;base=prepared();const heldCommands=commands,heldGraph=base.snapshot.graphHex;
        move(522,base);eq(response.ok,false);eq(response.error.code,"graph_transaction_busy");
        const evidence=JSON.parse(response.error.message.split(" Guard readback: ")[1]);
        eq(evidence.reader,"valueAtTime(0,true)");eq(evidence.saved,1);eq(evidence.indexed,0);eq(evidence.named,0);
        eq(evidence.indexedZero,1);eq(evidence.namedZero,1);eq(evidence.propertyIndex,40);
        eq(commands,heldCommands);eq(guard.value,1);eq(prepared().snapshot.graphHex,heldGraph);
        // Broken diagnostics cannot turn a held guard into permission to mutate.
        vm.runInContext("var guardStringify=JSON.stringify;JSON.stringify=function(value){if(value && value.reader && value.saved===1)throw new Error('guard evidence failed');return guardStringify(value);};",context);
        move(523,base);eq(response.error.code,"graph_transaction_busy");ok(response.error.message.includes("Guard readback unavailable"));
        vm.runInContext("JSON.stringify=guardStringify;",context);
        eq(commands,heldCommands);eq(guard.value,1);
        guard.value=0;guardDisplay=null;base=prepared();guardSamplerFailure=true;move(524,base);guardSamplerFailure=false;
        eq(response.ok,false);ok(response.error.message.includes("Guard sampler unavailable"));eq(commands,heldCommands);eq(guard.value,0);
        // Real host state was not changed by a repair/reset: these are fixture-only assignments.
    }
    if(process.env.STARFIELD_DOM_GUARD_FAILURES==="1") {
        const move=(x,base)=>client.apply({type:"moveNodes",positions:{[motionId]:{x,y:75}}},r=>{response=r;},token,base.snapshot.revision,base);
        const guard=renderer.properties["Panel Graph Sync Guard"];
        let base=prepared();const beforeAcquire=commands,beforeAcquireGraph=base.snapshot.graphHex;
        ignoreGuardAcquire=true;move(540,base);ignoreGuardAcquire=false;
        eq(response.ok,false);ok(response.error.message.includes("acquisition was not saved"));
        eq(commands,beforeAcquire,"failed acquisition does not authorize a rollback command");
        eq(prepared().snapshot.graphHex,beforeAcquireGraph);eq(guard.value,0);
        base=prepared();ignoreGuardClear=true;move(550,base);ignoreGuardClear=false;
        eq(response.ok,false);eq(response.error.code,"graph_guard_release_failed");
        ok(response.error.message.includes("release was not saved"));eq(guard.value,1);
        const locked=prepared(),lockedCount=commands;move(560,locked);
        eq(response.error.code,"graph_transaction_busy");eq(guard.value,1);eq(commands,lockedCount,"unknown held guard never runs another native command");
        guard.value=0; // Fake-host reset only; production must not clear unknown ownership.
        base=prepared();restoreGuardOnUndo=true;move(570,base);
        eq(response.ok,false);eq(response.error.code,"graph_guard_release_failed");
        ok(response.error.message.includes("after undo closure"));eq(guard.value,1);
        guard.value=0;base=prepared();move(580,base);eq(response.ok,true);eq(guard.value,0);
        // A busy bootstrap is not an owner and cannot mutate an existing guard.
        guard.value=1;const beforeBusy=copy(renderer.properties),commandCount=commands;
        const busy=JSON.parse(host.SFLD_syncGraphSnapshot(request("syncGraphSnapshot")));
        eq(busy.ok,false);eq(copy(renderer.properties),beforeBusy);eq(commands,commandCount);eq(guard.value,1);
        guard.value=0;
    }
    commands=0;undo=priorUndo; // The subsequent fixture counts its own transaction.
}
const batchBaseline=items.map(e=>({matchName:e.matchName,name:e.name,properties:copy(e.properties)}));
pair=begin(false);copies=stage(pair);rejectCommit=true;eq(host.SFLD_modelTransactionHostCommit(pair.id),"0");eq(undo,process.env.STARFIELD_DOM_MATERIAL==="1"?6:0);eq(arbReads,0);
eq(host.SFLD_modelTransactionHostResult(pair.id,"0|5|512|0|0|0|0|0|-1"),"1");
eq(JSON.parse(host.SFLD_readModelGraphTransaction(request("readModelGraphTransaction"))).result.ok,false);
eq(staleReads,0,"No cached invalid Property may be used after native backup/add/remove.");
if(deletionRender){ok(physicalRemovals>0);eq(missingRenderBindings,0,"No render-at-removal may resolve a deleted published source.");}
if(fullGraph) {
    // Restore the fixture's simulated outer backup, then delete several kinds
    // in one real transport/gateway commit against invalidating indexed groups.
    eq(JSON.parse(host.SFLD_releaseModelGraphTransaction(request("releaseModelGraphTransaction"))).ok,true);
    renderer.properties=batchBaseline[0].properties;delete renderer.wrapper;
    items.splice(0,items.length,renderer,...batchBaseline.slice(1));epoch++;rejectCommit=false;commands=0;
    const before=copy(api.snapshot(api.resolve({pinTarget:true,target:{token}})));
    const ids=before.nativeNodes.filter(n=>[edits.types.particle,edits.types.force,edits.types.transform,edits.types.motion].includes(n.type)).map(n=>n.id);
    ok(ids.length>=3);
    const desired=before.nativeNodes.filter(n=>!ids.includes(n.id));desired.forEach(n=>{n.outgoing=n.outgoing.filter(e=>!ids.includes(e.target));});
    transaction++;const id=uuid(transaction);
    eq(JSON.parse(host.SFLD_beginModelGraphTransaction(request("beginModelGraphTransaction",{baseGraphRevision:before.revision,
        baseRecordStamp:before.recordStamp,graphHex:codec.toHex(g),nodeManifest:desired,rendererManifest:before.renderer,assets:[]}))).ok,true);
    eq(JSON.parse(host.SFLD_queueModelGraphTransaction(request("queueModelGraphTransaction"))).ok,true);
    ok(host.SFLD_modelTransactionHostRequest().startsWith(id+"|"));eq(host.SFLD_modelTransactionHostClaim(id),"1");
    eq(host.SFLD_modelTransactionHostBegin(id),"1");api.resolve({pinTarget:true,target:{token}}).properties.guard.setValue(1);
    eq(host.SFLD_modelTransactionHostPrepare(id),"1",host.__SFLD_modelGraphTransactionV1.message);
    eq(host.SFLD_modelTransactionHostCommit(id),"1",host.__SFLD_modelGraphTransactionV1.message);
    const after=copy(api.snapshot(api.resolve({pinTarget:true,target:{token}})));
    eq(after.nativeNodes.map(n=>n.id).sort(),desired.map(n=>n.id).sort(),"batch removal leaves exactly the surviving identities");
    eq(after.nativeNodes.flatMap(n=>n.outgoing).some(e=>ids.includes(e.target)),false,"batch removal leaves no dangling edges");
    eq(staleReads,0,"multi-node removal never reuses an invalidated child Property");
    eq(missingRenderBindings,0,"batch physical removals follow verified publication");
}
console.log(`model_graph_gateway_dom_tests: ${checks} checks passed; effect wrapper identity ${stableEffects?"reused":"fresh"}; ${fullGraph?"Emitter/Particle/Model connections":"Model only"}, fractional layout; native writes simulated, no AE qualification`);
