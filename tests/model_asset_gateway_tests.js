"use strict";
// Actual isolated gateway/ordinary DOM, no CUSTOM_VALUE or AE project writes.
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
if(!process.env.STARFIELD_PANEL_ROOT)throw new Error("Set STARFIELD_PANEL_ROOT to the isolated Model candidate.");
const source=fs.readFileSync(path.join(process.env.STARFIELD_PANEL_ROOT,"jsx/starfield_gateway.jsx"),"utf8");
let checks=0,now=1000,commands=0,arbReads=0,writes=0;
const eq=(a,b)=>{checks++;assert.deepEqual(a,b);},ok=v=>{checks++;assert.ok(v);};
const nodeId="fedcba98765432100123456789abcdef",assetId="0123456789abcdef0123456789abcdef",token="p101-c202-l303";
const properties={};
function add(name,disk,value){const p={name,matchName:"org.starfieldfx.node.model-"+disk,value,numKeys:0,
    setValue(){writes++;throw new Error("Transport must not write the project");}};properties[name]=properties[p.matchName]=p;}
add("Source",1501,1);add("Mesh Revision",1504,17);
for(const [start,label,value] of [[5,"Offset",0],[8,"Angle",0],[11,"Scale",100]])for(let axis=0;axis<3;axis++)add(label+" "+"XYZ"[axis],1500+start+axis,value);
for(const [index,name] of ["Flip X","Flip Y","Flip Z","Center","Normalize"].entries())add(name,1514+index,0);
const bounds=[-2,-3,-4,2,3,4],boundNames=["Mesh Min X","Mesh Min Y","Mesh Min Z","Mesh Max X","Mesh Max Y","Mesh Max Z"];
for(let axis=0;axis<6;axis++)add(boundNames[axis],1519+axis,bounds[axis]);
add("Panel Sync Guard",4094,0);add("Node Layout X",4019,100);add("Node Layout Y",4020,100);add("Outgoing Connection Count",4021,0);
for(let i=0;i<8;i++)add("Node UUID "+i,4100+i,parseInt(nodeId.substr(i*4,4),16));
const model={name:"Model",matchName:"org.starfieldfx.node.model",property(key){
    if(key==="Model Mesh" || key==="org.starfieldfx.node.model-1503"){arbReads++;throw new Error("CUSTOM_VALUE");}return properties[key] || null;}};
const carrierNames=["Graph Revision","Panel Graph Sync Guard","Commit Graph Edit","Graph Edit Receipt","Node Effects Ready","Graph Checksum High","Graph Checksum Low"];
const carrierIndices=[39,40,41,42,87,88,89];
const carriers={};carrierNames.forEach((name,i)=>{carriers[name]=carriers[carrierIndices[i]]={name,propertyIndex:carrierIndices[i],value:name==="Graph Revision"?11:name==="Node Effects Ready"?1:0};});
const renderer={name:"Starfield",matchName:"org.starfieldfx.particle",property(key){return carriers[key] || null;}};
const backupModel={name:"Model backup",matchName:model.matchName,property(key){return key==="Panel Sync Guard"?{value:2}:model.property(key);}};
const backupRenderer={name:"Renderer backup",matchName:renderer.matchName,property(key){return key==="Panel Graph Sync Guard"?{value:2}:renderer.property(key);}};
const parade={list:[renderer,model],get numProperties(){return this.list.length;},property(i){return this.list[i-1];}};
const layer={id:303,width:1920,height:1080,source:{pixelAspect:1},property(name){return name==="ADBE Effect Parade"?parade:null;}};
function CompItem(){}const comp=new CompItem();Object.assign(comp,{id:202,numLayers:1,layer(i){return i===1?layer:null;}});
const app={project:{rootFolder:{id:101},numItems:1,item(i){return i===1?comp:null;}},
    findMenuCommandId(name){eq(name,"Starfield Prepare Model Asset Export");return 9;},executeCommand(id){eq(id,9);commands++;}};
const host={},context=vm.createContext({$:{global:host},app,CompItem,Date:function(){this.getTime=()=>now;}});
const realm=x=>vm.runInContext("("+JSON.stringify(x)+")",context);
const load=()=>vm.runInContext(source,context);load();
function request(operation,extra){return JSON.stringify(Object.assign({protocol:"org.starfieldfx.panel",version:1,gatewayBuild:"native-presets-62",
    operation,pinTarget:true,target:{token},assetId,nodeId,baseGraphRevision:11,page:0},extra));}
const call=(op,extra)=>JSON.parse(host["SFLD_"+op](request(op,extra)));
const session=()=>host.__SFLD_modelAssetBridgeV1;
function reset(){host.__SFLD_modelAssetBridgeV1=null;now=1000;commands=0;app.project.rootFolder.id=101;parade.list=[renderer,model];
    properties.Source.value=1;properties["Mesh Revision"].value=17;properties["Panel Sync Guard"].value=0;
    carriers["Graph Revision"].value=11;carriers["Panel Graph Sync Guard"].value=0;
    boundNames.forEach((name,i)=>{properties[name].value=bounds[i];});}
function begin(){const response=call("beginModelAssetExport");checks++;assert.equal(response.ok,true,JSON.stringify(response));eq(commands,1);eq(session().state,"queued");}
function receive(bytes){eq(host.SFLD_modelAssetHostRequest(),assetId+"|101|202|303|"+nodeId+"|1|17");
    eq(host.SFLD_modelAssetHostBegin(assetId,bytes,realm(bounds)),"1");eq(host.SFLD_modelAssetHostContinue(assetId),"1");}
function finish(bytes){for(let i=0;i<Math.ceil(bytes*2/65536);i++)eq(host.SFLD_modelAssetHostChunk(assetId,i,"ab".repeat(Math.min(32768,bytes-i*32768))),"1");
    eq(host.SFLD_modelAssetHostFinish(assetId),"1");}
reset();eq(host.SFLD_modelAssetHostRequest(),"0");begin();receive(70001);
eq(call("readModelAssetPage").state,"receiving");finish(70001);
let page=call("readModelAssetPage");eq(page.ok,true);eq(page.pageCount,3);eq(page.bytes,70001);eq(page.bounds,bounds);eq(page.source,1);eq(page.revision,17);eq(page.hex.length,65536);
eq(call("readModelAssetPage",{page:2}).hex.length,(70001-65536)*2);
eq(call("readModelAssetPage",{page:3}).error.code,"invalid_request");
load();eq(call("readModelAssetPage").hex,page.hex); // Reload preserves plain session.
eq(call("readModelAssetPage",{target:{token:"p101-c202-l999"}}).error.code,"stale_target");
eq(call("releaseModelAsset",{assetId:"f".repeat(32)}).ok,true);ok(session());
eq(call("releaseModelAsset").ok,true);eq(session(),null);eq(host.SFLD_modelAssetHostContinue(assetId),"0");
for(const extra of [{pinTarget:false},{assetId:"quote'"},{nodeId:"x".repeat(32)},{baseGraphRevision:1.5},{baseGraphRevision:0},
    {baseGraphRevision:16777216},{baseGraphRevision:"11"}]){reset();eq(call("beginModelAssetExport",extra).ok,false);eq(commands,0);eq(session(),null);}
reset();begin();eq(call("beginModelAssetExport").error.code,"model_asset_busy");eq(commands,1);
for(const mutate of [()=>{carriers["Graph Revision"].value=12;},()=>{carriers["Panel Graph Sync Guard"].value=1;},()=>{properties.Source.value=2;},
    ()=>{properties["Mesh Revision"].value=18;},()=>{properties["Mesh Min X"].value=-2.0000000001;},()=>{properties["Panel Sync Guard"].value=1;},
    ()=>{app.project.rootFolder.id=102;},()=>{parade.list=[renderer,model,model];},()=>{parade.list=[renderer];}]){
    reset();begin();receive(32);mutate();eq(host.SFLD_modelAssetHostContinue(assetId),"0");eq(session().state,"failed");eq(session().pages.length,0);
    eq(call("readModelAssetPage").ok,false);}
for(const [bytes,box] of [[0,bounds],[8*1024*1024+1,bounds],[1.5,bounds],["1",bounds],[32,[0,0,0,0,0,0]]]){
    reset();begin();eq(host.SFLD_modelAssetHostBegin(assetId,bytes,realm(box)),"0");eq(session().state,"failed");}
for(const [index,hex] of [[1,"ab".repeat(32)],[.5,"ab".repeat(32)],[0,"ab"],[0,"zz".repeat(32)],[0,"AB".repeat(32)],[0,32]]){
    reset();begin();receive(32);eq(host.SFLD_modelAssetHostChunk(assetId,index,hex),"0");eq(session().pages.length,0);}
reset();begin();receive(70001);eq(host.SFLD_modelAssetHostChunk(assetId,0,"ab".repeat(32768)),"1");eq(host.SFLD_modelAssetHostFinish(assetId),"0");eq(session().pages.length,0);
reset();begin();receive(32);now+=60001;eq(call("readModelAssetPage").error.code,"model_asset_timeout");eq(session().pages.length,0);
reset();begin();eq(host.SFLD_modelAssetHostFail(assetId,8,516),"1");eq(call("readModelAssetPage").ok,false);eq(session().pages.length,0);
reset();begin();receive(8*1024*1024);finish(8*1024*1024);eq(call("readModelAssetPage",{page:255}).hex.length,65536);eq(session().pages.length,256);
reset();properties.Source.value=2;eq(call("beginModelAssetExport").ok,true);eq(host.SFLD_modelAssetHostRequest(),assetId+"|101|202|303|"+nodeId+"|2|17");
// Both native lookup and the pinned renderer inventory must ignore guard2.
reset();parade.list=[backupRenderer,backupModel,renderer,model];begin();receive(32);finish(32);
eq(call("readModelAssetPage").ok,true);eq(call("releaseModelAsset").ok,true);
eq(writes,0);eq(arbReads,0);
// Real client -> actual isolated gateway -> private native entry point shapes.
// Encode the same SFMG1 numeric triangle used by Core's format contract.
const assets=require("../tools/candidates/model_assets.js"),numeric=Buffer.alloc(164);
numeric.writeUInt32LE(0x474d4653,0);numeric.writeUInt16LE(1,4);numeric.writeUInt16LE(32,6);numeric.writeUInt32LE(164,8);
numeric.writeUInt32LE(3,16);numeric.writeUInt32LE(1,28);
[[-2,-3,-4],[2,-3,4],[0,3,0]].forEach((p,i)=>{p.concat(1).forEach((v,a)=>numeric.writeDoubleLE(v,32+i*32+a*8));});
for(let i=0;i<3;i++){numeric.writeUInt32LE(i,128+i*12);numeric.writeUInt32LE(0xffffffff,132+i*12);numeric.writeUInt32LE(0xffffffff,136+i*12);}
let crc=0xffffffff;for(const byte of numeric.subarray(32)){crc^=byte;for(let i=0;i<8;i++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);}numeric.writeUInt32LE((crc^0xffffffff)>>>0,12);
reset();let collected=null,clientTasks=[];
const client=assets.create({idFactory:()=>assetId,now:()=>now,call(operation,fields,callback){callback(call(operation,fields));},
    schedule(fn){clientTasks.push(function(){eq(host.SFLD_modelAssetHostRequest(),assetId+"|101|202|303|"+nodeId+"|1|17");
        eq(host.SFLD_modelAssetHostBegin(assetId,numeric.length,realm(bounds)),"1");
        eq(host.SFLD_modelAssetHostChunk(assetId,0,numeric.toString("hex")),"1");eq(host.SFLD_modelAssetHostFinish(assetId),"1");fn();});}});
client.collect({revision:11,nativeNodes:[{id:nodeId,type:"org.starfieldfx.nodes.model",parameters:[{key:"13",type:3,value:0}],modelAsset:{revision:17,bounds}}]},token,r=>{collected=r;});
eq(collected,null);eq(clientTasks.length,1);clientTasks.shift()();eq(collected.ok,true);eq(collected.assets[0],{nodeId,revision:17,bounds,meshHex:numeric.toString("hex")});
eq(session(),null);eq(writes,0);eq(arbReads,0);
console.log(`model_asset_gateway_tests: ${checks} checks passed; isolated gateway; no AE qualification`);
