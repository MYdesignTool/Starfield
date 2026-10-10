"use strict";
// Real gateway materialization/author callbacks with an indexed-group DOM.
// Mesh writes, native graph receipts and outer backup/undo are simulated.
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=process.env.STARFIELD_PANEL_ROOT;if(!root)throw new Error("Set STARFIELD_PANEL_ROOT to the isolated Model candidate.");
const edits=require(path.join(root,"js/graph_edits.js")),codec=require(path.join(root,"js/graph_codec.js")),layout=require(path.join(root,"js/graph_layout.js"));
const {mainControls,nodeControls}=require("./node_property_fixture.js"),uuid=n=>n.toString(16).padStart(32,"0"),token="p101-c202-l303";
const copy=x=>JSON.parse(JSON.stringify(x,(_,v)=>ArrayBuffer.isView(v)?Array.from(v):v));
let checks=0,epoch=0,arbReads=0,commands=0,undo=0,rejectCommit=false,commits=0;
const eq=(a,b,m)=>{checks++;assert.deepEqual(a,b,m);},ok=v=>{checks++;assert.ok(v);};
const renderer={matchName:"org.starfieldfx.particle",name:"Starfield",properties:mainControls()},items=[renderer];
function fresh(captured){if(captured!==epoch)throw new Error("Invalid indexed group reference");}
function property(p,captured){return {name:p.name,matchName:p.matchName,propertyIndex:p.propertyIndex,numKeys:0,canSetExpression:false,
    get value(){fresh(captured);return copy(p.value);},setValue(value){fresh(captured);p.value=copy(value);
        if(p===renderer.properties["Commit Graph Edit"]){commits++;const resolved=api.resolve({pinTarget:true,target:{token}});
            resolved.properties.receipt.setValue(rejectCommit?-value:value);if(!rejectCommit)resolved.properties.revision.setValue(Number(resolved.properties.revision.value)+1);}}};}
function wrap(raw){const captured=epoch,ordered=Array.from(new Set(Object.values(raw.properties)));
    return {matchName:raw.matchName,get name(){fresh(captured);return raw.name;},set name(v){fresh(captured);raw.name=v;},
        get numProperties(){fresh(captured);return ordered.length;},property(key){fresh(captured);
            if(key==="Model Mesh" || key==="org.starfieldfx.node.model-1503"){arbReads++;throw new Error("CUSTOM_VALUE forbidden");}
            const p=raw.properties[key] || (typeof key==="number"?ordered[key-1]:null);return p?property(p,captured):null;},
        remove(){fresh(captured);items.splice(items.indexOf(raw),1);epoch++;}};}
function modelControls(){const properties=nodeControls();
    function add(name,disk,value){const p={name,matchName:"org.starfieldfx.node.model-"+disk,value};properties[name]=properties[p.matchName]=p;}
    add("Source",1501,1);add("Mesh Revision",1504,0);
    for(const [start,name,value] of [[5,"Offset",0],[8,"Angle",0],[11,"Scale",100]])for(let a=0;a<3;a++)add(name+" "+"XYZ"[a],1500+start+a,value);
    for(const [i,name] of ["Flip X","Flip Y","Flip Z","Center","Normalize"].entries())add(name,1514+i,0);
    for(const [i,name] of ["Mesh Min X","Mesh Min Y","Mesh Min Z","Mesh Max X","Mesh Max Y","Mesh Max Z"].entries())add(name,1519+i,i<3?-.5:.5);
    return properties;}
function parade(){return {get numProperties(){return items.length;},property(i){return items[i-1]?wrap(items[i-1]):null;},
    addProperty(matchName){items.push({matchName,name:"",properties:modelControls()});epoch++;return wrap(items[items.length-1]);}};}
const layer={id:303,name:"Particles",selected:true,width:1920,height:1080,time:0,source:{pixelAspect:1},property:name=>name==="ADBE Effect Parade"?parade():null};
function CompItem(){}const comp=new CompItem();Object.assign(comp,{id:202,name:"Comp",time:0,numLayers:1,layer:()=>layer});layer.containingComp=comp;
const host={},context=vm.createContext({CompItem,$:{global:host},app:{project:{activeItem:comp,rootFolder:{id:101},numItems:1,item:()=>comp},
    beginUndoGroup(){undo++;},endUndoGroup(){undo++;},findMenuCommandId:()=>55,executeCommand(){commands++;}}});
const realm=x=>vm.runInContext("("+JSON.stringify(x)+")",context);
vm.runInContext(fs.readFileSync(path.join(root,"jsx/starfield_gateway.jsx"),"utf8"),context);
const api=host.__SFLD_modelTransactionAPI;vm.runInContext(fs.readFileSync(path.join(root,"jsx/model_transaction_transport.jsx"),"utf8"),context);
let resolved=api.resolve({pinTarget:true,target:{token}});ok(!resolved.error,JSON.stringify(resolved.error));
resolved.properties.nodeEffectsReady.setValue(1);resolved.properties.revision.setValue(11);
let g=edits.apply({version:1,nodes:[],edges:[],optionalRecords:[]},{type:"addNode",nodeType:"model"},()=>uuid(101));
g=edits.apply(g,{type:"addNode",nodeType:"output"},()=>uuid(255));g=layout.set(g,{[uuid(101)]:{x:100,y:50},[uuid(255)]:{x:400,y:250}});
const record=n=>({id:n.id,type:n.type,schemaVersion:n.schemaVersion,parameters:copy(n.parameters),position:layout.resolve(g)[uuid(101)],outgoing:[]});
api.ensure(layer,realm([record(g.nodes[0])]),true,realm([]),false);eq(arbReads,0);
const bounds=[-2,-3,-4,2,3,4],raw=Buffer.alloc(48);bounds.forEach((v,i)=>raw.writeDoubleLE(v,i*8));
let transaction=500;
function request(operation,fields={}){return JSON.stringify({protocol:"org.starfieldfx.panel",version:1,gatewayBuild:"native-presets-61",operation,requestId:"test",pinTarget:true,
    target:{token},transactionId:uuid(transaction),changes:[],...fields});}
function begin(replace){resolved=api.resolve({pinTarget:true,target:{token}});const before=copy(api.snapshot(resolved));
    const desired=copy(before.nativeNodes[0]);desired.id=uuid(++transaction);desired.position={x:160,y:90};
    for(const [key,value] of [[1,Array.from(Buffer.from(desired.id,"hex"))],[2,17],[12,Array.from(raw)],[13,1],[5,[0,45,0]]])desired.parameters.find(p=>p.key===String(key)).value=value;
    const nodes=replace?[desired]:before.nativeNodes.concat([desired]);
    const r=JSON.parse(host.SFLD_beginModelGraphTransaction(request("beginModelGraphTransaction",{baseGraphRevision:before.revision,baseRecordStamp:before.recordStamp,
        graphHex:codec.toHex(g),nodeManifest:nodes,rendererManifest:before.renderer,assets:[{nodeId:desired.id,source:2,revision:17,bounds,bytes:32}]})));
    eq(r.ok,true,JSON.stringify(r));return {desired,before,id:uuid(transaction)};}
function stage(pair){const {id,desired}=pair;eq(JSON.parse(host.SFLD_writeModelGraphAssetPage(request("writeModelGraphAssetPage",{asset:0,page:0,hex:"ab".repeat(32)}))).ok,true);
    eq(JSON.parse(host.SFLD_queueModelGraphTransaction(request("queueModelGraphTransaction"))).ok,true);eq(commands,1);
    ok(host.SFLD_modelTransactionHostRequest().startsWith(id+"|101|202|303|1|"));
    // Simulate the native complete backup: a disabled guard2 renderer and node
    // remain in the parade while the live renderer is held at guard1.
    const backupMain={...renderer,properties:copy(renderer.properties)},backupNode={...items[1],properties:copy(items[1].properties)};
    for(const p of new Set(Object.values(backupMain.properties)))if(p.name==="Panel Graph Sync Guard")p.value=2;
    for(const p of new Set(Object.values(backupNode.properties)))if(p.name==="Panel Sync Guard")p.value=2;
    for(let i=0;i<8;i++)backupNode.properties["Node UUID "+i].value=parseInt(uuid(700).substr(i*4,4),16);
    items.push(backupMain,backupNode);epoch++;
    resolved=api.resolve({pinTarget:true,target:{token}});resolved.properties.guard.setValue(1);
    eq(host.SFLD_modelTransactionHostPrepare(id),"1");
    let current=copy(api.snapshot(api.resolve({pinTarget:true,target:{token}})));const model=current.nativeNodes.find(n=>n.id===desired.id);
    eq(model.parameters.find(p=>p.key==="13").value,0);eq(model.modelAsset.revision,0);eq(arbReads,0);
    const effect=items.find(e=>e.matchName==="org.starfieldfx.node.model" && e.properties["Panel Sync Guard"].value!==2 &&
        Array.from({length:8},(_,i)=>e.properties["Node UUID "+i].value.toString(16).padStart(4,"0")).join("")===desired.id);
    effect.properties.Source.value=2;effect.properties["Mesh Revision"].value=17;
    for(const [i,name] of ["Mesh Min X","Mesh Min Y","Mesh Min Z","Mesh Max X","Mesh Max Y","Mesh Max Z"].entries())effect.properties[name].value=bounds[i];
    return {effect,backupMain,backupNode};}
let pair=begin(true),copies=stage(pair);eq(host.SFLD_modelTransactionHostCommit(pair.id),"1");eq(commits,1);eq(arbReads,0);eq(undo,0);
eq(copies.effect.properties["Angle Y"].value,45);eq(copies.effect.properties["Panel Sync Guard"].value,0);
eq(copy(api.snapshot(api.resolve({pinTarget:true,target:{token}}))).nativeNodes.length,1);
eq(host.SFLD_modelTransactionHostResult(pair.id,realm([1,7,0,0,0,0,0,0,-1])),"1");
eq(JSON.parse(host.SFLD_readModelGraphTransaction(request("readModelGraphTransaction"))).result.committed,true);
eq(JSON.parse(host.SFLD_releaseModelGraphTransaction(request("releaseModelGraphTransaction"))).ok,true);
items.splice(items.indexOf(copies.backupMain),1);items.splice(items.indexOf(copies.backupNode),1);epoch++;
api.resolve({pinTarget:true,target:{token}}).properties.guard.setValue(0);commands=0;
pair=begin(false);copies=stage(pair);rejectCommit=true;eq(host.SFLD_modelTransactionHostCommit(pair.id),"0");eq(undo,0);eq(arbReads,0);
eq(host.SFLD_modelTransactionHostResult(pair.id,realm([0,5,512,0,0,0,0,0,-1])),"1");
eq(JSON.parse(host.SFLD_readModelGraphTransaction(request("readModelGraphTransaction"))).result.ok,false);
console.log(`model_graph_gateway_dom_tests: ${checks} checks passed; actual gateway callbacks with stale indexed refs/guard2; native writes simulated, no AE qualification`);
