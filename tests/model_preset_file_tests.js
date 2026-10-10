"use strict";
// Real isolated preset codec/client/gateway. File IO is entirely in memory.
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
if(!process.env.STARFIELD_PANEL_ROOT)throw new Error("Set STARFIELD_PANEL_ROOT to the isolated Model candidate.");
const root=process.env.STARFIELD_PANEL_ROOT,load=relative=>fs.readFileSync(path.join(root,relative),"utf8");
const presets=require(path.join(root,"js/presets.js")),codec=require(path.join(root,"js/graph_codec.js")),
    edits=require(path.join(root,"js/graph_edits.js")),files=require(path.join(root,"js/preset_files.js"));
let checks=0;const eq=(a,b)=>{checks++;assert.deepEqual(a,b);},ok=v=>{checks++;assert.ok(v);},reject=fn=>{checks++;assert.throws(fn);};
const modelId="abcdefabcdefabcdefabcdefabcdefab",transferId="12341234123412341234123412341234";
function mesh(positions=3){const b=Buffer.alloc(32+32*positions+36);b.writeUInt32LE(0x474d4653);b.writeUInt16LE(1,4);b.writeUInt16LE(32,6);b.writeUInt32LE(b.length,8);b.writeUInt32LE(positions,16);b.writeUInt32LE(1,28);
    [[-2,-3,-4],[2,-3,4],[0,3,0]].forEach((p,i)=>p.concat(1).forEach((v,a)=>b.writeDoubleLE(v,32+i*32+a*8)));
    for(let i=3;i<positions;i++)b.writeDoubleLE(1,32+i*32+24);
    for(let i=0;i<3;i++){b.writeUInt32LE(i,32+32*positions+i*12);b.writeUInt32LE(0xffffffff,36+32*positions+i*12);b.writeUInt32LE(0xffffffff,40+32*positions+i*12);}
    let crc=0xffffffff;for(const byte of b.subarray(32)){crc^=byte;for(let bit=0;bit<8;bit++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);}b.writeUInt32LE((crc^0xffffffff)>>>0,12);return b.toString("hex");}
const bounds=[-2,-3,-4,2,3,4];
function graph(imported=true){let g=presets.build("sparks",1080);g=edits.apply(g,{type:"addNode",nodeType:"model"},()=>modelId);
    const n=g.nodes.find(n=>n.id===modelId);
    if(imported){const raw=Buffer.alloc(48);bounds.forEach((v,i)=>raw.writeDoubleLE(v,i*8));
        for(const [key,value] of [[1,Array.from(Buffer.from(modelId,"hex"))],[2,17],[12,Array.from(raw)],[13,1]])n.parameters.find(p=>p.key===String(key)).value=value;}
    return g;}
const asset={nodeId:modelId,revision:17,bounds,meshHex:mesh()},imported=graph();
let encoded=presets.encode(imported,"测试模型 ✨","My Presets",[],[asset]),entry=presets.decode(encoded);
eq(JSON.parse(encoded).version,3);eq(entry.modelAssets,[asset]);eq(codec.toHex(entry.graph),codec.toHex(presets.authoring(imported)));
reject(()=>presets.encode(imported,"Missing","My Presets",[],[]));
reject(()=>presets.decode(JSON.stringify({...JSON.parse(encoded),modelAssets:[]})));
reject(()=>presets.decode(JSON.stringify({...JSON.parse(encoded),modelAssets:[{...asset,revision:18}]})));
reject(()=>presets.decode(JSON.stringify({...JSON.parse(encoded),modelAssets:[{...asset,nodeId:"1".repeat(32)}]})));
reject(()=>presets.decode(JSON.stringify({...JSON.parse(encoded),modelAssets:[{...asset,meshHex:asset.meshHex.slice(0,-2)+"00"}]})));
const restoredEdit={type:"applyPreset",presetGraph:entry.graph,presetModelAssets:entry.modelAssets,mode:"replace"};
const restoredGraph=presets.apply(presets.build("sparks",1080),restoredEdit,edits.randomId),restoredId=restoredEdit.modelAssetsToRestore[0].nodeId;
ok(restoredId!==modelId);eq(restoredEdit.modelAssetsToRestore[0],{...asset,nodeId:restoredId,source:2});
eq(restoredGraph.nodes.find(n=>n.id===restoredId).parameters.find(p=>p.key==="1").value,Array.from(Buffer.from(restoredId,"hex")));
const parked=presets.decode(presets.encode(graph(false),"Parked","My Presets",[],[asset]));eq(parked.modelAssets,[asset]);
eq(parked.graph.nodes.find(n=>n.id===modelId).parameters.find(p=>p.key==="13").value,0);
const legacy=presets.encode(presets.build("sparks",1080),"Legacy","My Presets",[]);
eq(JSON.parse(legacy).version,1);eq(presets.decode(legacy).modelAssets,[]);
const textured=presets.build("sparks",1080),particle=textured.nodes.find(n=>n.type===edits.types.particle);
particle.parameters.find(p=>p.key==="31").value=44;
const textureText=presets.encode(textured,"Texture","My Presets",[{id:44,texture:true,name:"纹理",sourceName:"源"}]);
eq(JSON.parse(textureText).version,2);eq(presets.decode(textureText).resources[0].layerName,"纹理");
const largeAsset={...asset,meshHex:mesh(10000)},large=presets.encode(imported,"Large","My Presets",[],[largeAsset]);
ok(Buffer.byteLength(large)>256*1024);eq(presets.decode(large).modelAssets,[largeAsset]);

let disk={},now=1000,operations=[],openDialog="D:/user/import.sfldpreset",saveDialog="D:/user/输出.sfldpreset",confirmed=true,fault={};
function File(name){this.fsName=name;this.error="";this.opened=false;}
Object.defineProperties(File.prototype,{name:{get(){return this.fsName.slice(this.fsName.lastIndexOf("/")+1);}},exists:{get(){return this.fsName in disk;}},length:{get(){return Buffer.byteLength(disk[this.fsName] || "");}}});
File.openDialog=File.saveDialog=()=>{throw new Error("Script modal chooser must not run.");};
File.prototype.open=function(mode){operations.push(["open",this.fsName,mode]);if(fault.open){this.error="open failed";return false;}this.opened=true;this.mode=mode;if(mode==="w")disk[this.fsName]="";return true;};
File.prototype.read=function(){if(fault.read){this.error="read failed";return "";}return disk[this.fsName];};
File.prototype.write=function(text){disk[this.fsName]=fault.silentWrite?"corrupt":text;if(fault.write){disk[this.fsName]="partial";this.error="write failed";return false;}return true;};
File.prototype.close=function(){operations.push(["close",this.fsName]);this.opened=false;if(fault.close){this.error="close failed";return false;}return true;};
File.prototype.rename=function(name){operations.push(["rename",this.fsName,name]);
    if(fault.renameOld && this.fsName===saveDialog || fault.publish && this.fsName.endsWith(".tmp") || fault.rollback && this.fsName.endsWith(".bak"))return false;
    const dest=this.fsName.slice(0,this.fsName.lastIndexOf("/")+1)+name;if(dest in disk)return false;disk[dest]=disk[this.fsName];delete disk[this.fsName];this.fsName=dest;return true;};
File.prototype.remove=function(){operations.push(["remove",this.fsName]);if(fault.removeBackupThrows && this.fsName.endsWith(".bak"))throw new Error("cleanup failed");
    if(fault.removeBackup && this.fsName.endsWith(".bak") || fault.removeTemporary && this.fsName.endsWith(".tmp"))return false;delete disk[this.fsName];return true;};
let queuedCommands=0;
const app={findMenuCommandId:name=>name==="Starfield Choose Preset File"?77:0,executeCommand:id=>{eq(id,77);queuedCommands++;}};
const host={},context=vm.createContext({$:{global:host},File,app,confirm:()=>{throw new Error("Script confirm must not run.");},Date:function(){this.getTime=()=>now;}});
function reload(){vm.runInContext(load("jsx/starfield_gateway.jsx"),context);vm.runInContext(load("jsx/preset_file_transport.jsx"),context);}
reload();
function rawCall(op,extra,target=host){return JSON.parse(target["SFLD_"+op](JSON.stringify({protocol:"org.starfieldfx.panel",version:1,gatewayBuild:"native-presets-61",operation:op,transferId,...extra})));}
function hexPath(path){return Array.from({length:path.length},(_,i)=>path.charCodeAt(i).toString(16).padStart(4,"0")).join("");}
function nativeModal(target=host){const descriptor=target.SFLD_presetFileHostRequest();if(descriptor==="0")return;
    const id=descriptor.slice(0,32);ok(/^[a-f0-9]{32}$/.test(id));const save=descriptor.endsWith("|2");let path=save?saveDialog:openDialog;
    eq(target.SFLD_presetFileHostBegin(id),"1");
    if(save && path && !/\.sfldpreset$/i.test(path))path+=".sfldpreset";
    const cancelled=!path || save && path in disk && !confirmed;
    eq(target.SFLD_presetFileHostValidate(id),"1");eq(target.SFLD_presetFileHostComplete(id,cancelled?"":hexPath(path),cancelled?1:0),"1");}
function call(op,extra){let response=rawCall(op,extra);if(response.state==="queued" && (op==="beginPresetFileRead" || op==="finishPresetFileWrite")){
    nativeModal();response=rawCall("readPresetFileModal");if(op==="finishPresetFileWrite")rawCall("releasePresetFile");}return response;}
function reset(){disk={};now=1000;operations=[];fault={};queuedCommands=0;openDialog="D:/user/import.sfldpreset";saveDialog="D:/user/输出.sfldpreset";confirmed=true;host.__SFLD_presetFileSessionV1=null;}
const temporary=()=>saveDialog+".starfield-"+transferId+".tmp",backup=()=>saveDialog+".starfield-"+transferId+".bak";
function upload(text){eq(call("beginPresetFileWrite",{characters:text.length}).ok,true);
    for(let page=0,start=0;start<text.length;page++){let end=Math.min(text.length,start+32768);if(end<text.length && text.charCodeAt(end-1)>=0xd800 && text.charCodeAt(end-1)<=0xdbff)end--;
        eq(call("writePresetFilePage",{page,text:text.slice(start,end)}).received,end);start=end;}return call("finishPresetFileWrite");}
reset();eq(upload(large).ok,true);eq(disk[saveDialog],large);eq(Object.keys(disk),[saveDialog]);eq(host.__SFLD_presetFileSessionV1,null);
reset();disk[saveDialog]="previous";eq(upload(encoded).ok,true);eq(disk[saveDialog],encoded);eq(Object.keys(disk),[saveDialog]);
for(const fail of ["open","write","silentWrite","read","close","renameOld","publish"]){reset();disk[saveDialog]="previous";fault[fail]=true;
    eq(upload(encoded).ok,false);eq(disk[saveDialog],"previous");eq(disk[backup()],undefined);eq(disk[temporary()],undefined);eq(host.__SFLD_presetFileSessionV1,null);}
reset();disk[saveDialog]="previous";fault.publish=fault.rollback=true;let response=upload(encoded);
eq(response.ok,false);eq(response.context.retainedBackup,backup());eq(disk[backup()],"previous");eq(disk[saveDialog],undefined);
reset();disk[saveDialog]="previous";fault.removeBackup=true;response=upload(encoded);eq(response.ok,true);eq(response.retainedBackup,backup());eq(disk[backup()],"previous");
reset();disk[saveDialog]="previous";fault.removeBackupThrows=true;response=upload(encoded);eq(response.ok,true);eq(response.retainedBackup,backup());eq(disk[saveDialog],encoded);
reset();disk[saveDialog]="previous";fault.publish=fault.removeTemporary=true;response=upload(encoded);eq(response.ok,false);eq(response.context.retainedTemporary,temporary());eq(disk[saveDialog],"previous");eq(disk[temporary()],encoded);
for(const collision of [temporary,backup]){reset();disk[collision()]="someone else";eq(upload(encoded).ok,false);eq(disk[collision()],"someone else");eq(disk[saveDialog],undefined);}
reset();saveDialog=null;eq(upload(encoded).cancelled,true);eq(operations.length,0);
reset();disk[saveDialog]="previous";confirmed=false;eq(upload(encoded).cancelled,true);eq(disk[saveDialog],"previous");eq(operations.length,0);
reset();eq(call("beginPresetFileWrite",{characters:0}).ok,false);eq(call("beginPresetFileWrite",{characters:files.maxTextCharacters+1}).ok,false);
eq(call("beginPresetFileWrite",{characters:encoded.length}).ok,true);eq(call("writePresetFilePage",{page:1,text:"x"}).ok,false);
eq(call("writePresetFilePage",{page:0,text:"x".repeat(32769)}).ok,false);eq(call("finishPresetFileWrite").ok,false);eq(operations.length,0);
reload();eq(call("writePresetFilePage",{page:0,text:encoded}).ok,true);eq(call("finishPresetFileWrite").ok,true);
reset();eq(call("beginPresetFileWrite",{characters:1}).ok,true);now+=300001;eq(call("writePresetFilePage",{page:0,text:"x"}).ok,false);eq(host.__SFLD_presetFileSessionV1,null);
reset();openDialog=null;eq(call("beginPresetFileRead").cancelled,true);eq(operations.length,0);
reset();disk[openDialog]=large;response=call("beginPresetFileRead");eq(response.characters,large.length);eq(call("readPresetFilePage",{page:1}).ok,false);
let pages=[];for(let page=0;pages.join("").length<large.length;page++){response=call("readPresetFilePage",{page});eq(response.ok,true);ok(response.text.length<=32768);pages.push(response.text);reload();}
eq(pages.join(""),large);eq(call("readPresetFilePage",{page:pages.length}).ok,false);eq(call("releasePresetFile").ok,true);eq(host.__SFLD_presetFileSessionV1,null);

// Real client -> gateway with one small request per page and yields between calls.
reset();disk[openDialog]=large;let tasks=[],completed=[];
const client=files.create({idFactory:()=>transferId,now:()=>now,schedule:fn=>tasks.push(fn),call(op,fields,callback){
    ok(JSON.stringify(fields).length<262144);if(op==="readPresetFileModal")nativeModal();callback(rawCall(op,fields));}});
function drain(){let steps=0;while(tasks.length){if(++steps>1000)throw new Error("Fixture queue did not settle: "+JSON.stringify(host.__SFLD_presetFileSessionV1, (k,v)=>k==="text"?"<payload>":v));tasks.shift()();}}
client.load(r=>completed.push(r));drain();eq(completed.length,1);eq(completed[0].text,large);eq(host.__SFLD_presetFileSessionV1,null);
completed=[];client.save(large,r=>completed.push(r));drain();eq(completed.length,1);eq(completed[0].ok,true);eq(disk[saveDialog],large);
completed=[];const cancelled=client.save(large,r=>completed.push(r));ok(tasks.length>0);cancelled.cancel();drain();eq(completed.length,1);eq(completed[0].error.code,"cancelled");eq(host.__SFLD_presetFileSessionV1,null);
reset();completed=[];client.save(large,r=>completed.push(r));now+=300001;drain();eq(completed.length,1);eq(completed[0].error.code,"preset_file_timeout");eq(host.__SFLD_presetFileSessionV1,null);
// A surrogate pair at a page boundary survives save/read unchanged.
reset();const unicode=JSON.stringify({format:"org.starfieldfx.preset",version:3,name:"Unicode",graphHex:JSON.parse(encoded).graphHex,resources:[],modelAssets:[asset],padding:"x".repeat(32000)+"😀".repeat(2000)});
completed=[];client.save(unicode,r=>completed.push(r));drain();eq(completed[0].ok,true);eq(disk[saveDialog],unicode);
disk[openDialog]=unicode;completed=[];client.load(r=>completed.push(r));drain();eq(completed[0].text,unicode);
eq(operations.filter(o=>o[0]==="open" && o[2]==="w" && !o[1].endsWith(".tmp")).length,0);

// Native modal protocol has no script-owned window or IO before completion.
reset();disk[openDialog]=encoded;response=rawCall("beginPresetFileRead");eq(response.state,"queued");eq(queuedCommands,1);eq(operations.length,0);
eq(rawCall("beginPresetFileRead").ok,false);eq(host.__SFLD_presetFileSessionV1.stage,"queued");eq(rawCall("readPresetFilePage",{page:0}).ok,false);
eq(host.SFLD_presetFileHostRequest(),transferId+"|1");eq(host.SFLD_presetFileHostRequest(),transferId+"|1");eq(host.SFLD_presetFileHostBegin(transferId),"1");
eq(host.SFLD_presetFileHostBegin(transferId),"0");eq(rawCall("readPresetFileModal").state,"choosing");eq(operations.length,0);
eq(host.SFLD_presetFileHostComplete(transferId,hexPath(openDialog),0),"1");response=rawCall("readPresetFileModal");eq(response.characters,encoded.length);
const completedOps=operations.length;eq(host.SFLD_presetFileHostComplete(transferId,hexPath(openDialog),0),"0");eq(rawCall("readPresetFileModal"),response);eq(operations.length,completedOps);
rawCall("releasePresetFile");eq(host.SFLD_presetFileHostValidate(transferId),"0");
for(const abort of ["release","expire","replace"]){reset();eq(rawCall("beginPresetFileWrite",{characters:encoded.length}).ok,true);
    rawCall("writePresetFilePage",{page:0,text:encoded});eq(rawCall("finishPresetFileWrite").state,"queued");eq(host.SFLD_presetFileHostBegin(transferId),"1");
    if(abort==="release")rawCall("releasePresetFile");else if(abort==="expire")now+=300001;else host.__SFLD_presetFileSessionV1.id="1".repeat(32);
    eq(host.SFLD_presetFileHostValidate(transferId),"0");eq(host.SFLD_presetFileHostComplete(transferId,hexPath(saveDialog),0),"0");eq(operations.length,0);eq(disk[saveDialog],undefined);}
for(const path of ["relative", "D:/nul\0.sfldpreset", "D:/bad\ud800.sfldpreset", "D:/bad\udc00.sfldpreset", "D:/other.json"]){reset();
    rawCall("beginPresetFileWrite",{characters:encoded.length});rawCall("writePresetFilePage",{page:0,text:encoded});rawCall("finishPresetFileWrite");host.SFLD_presetFileHostBegin(transferId);
    eq(host.SFLD_presetFileHostComplete(transferId,hexPath(path),0),"1");eq(rawCall("readPresetFileModal").ok,false);eq(operations.length,0);}
reset();rawCall("beginPresetFileRead");host.SFLD_presetFileHostBegin(transferId);eq(host.SFLD_presetFileHostComplete(transferId,"",2),"1");eq(rawCall("readPresetFileModal").error.code,"preset_file_error");eq(operations.length,0);
// An ambiguous queue acknowledgement is recovered by reading, never replayed.
for(const mode of ["save","load"]){reset();disk[openDialog]=encoded;tasks=[];completed=[];let queues=0;
    const uncertain=files.create({idFactory:()=>transferId,now:()=>now,schedule:fn=>tasks.push(fn),call(op,fields,callback){
        const result=rawCall(op,fields);if(op==="beginPresetFileRead" || op==="finishPresetFileWrite"){queues++;callback({ok:false,error:{code:"host_timeout",message:"lost ack"}});return;}
        if(op==="readPresetFileModal"){nativeModal();callback(rawCall(op,fields));return;}callback(result);}});
    if(mode==="save")uncertain.save(encoded,r=>completed.push(r));else uncertain.load(r=>completed.push(r));drain();eq(queues,1);eq(completed.length,1);eq(completed[0].ok,true);
    eq(operations.filter(o=>o[0]==="open" && o[2]==="w").length,mode==="save"?1:0);}
reset();tasks=[];completed=[];const delays=[];
const pending=files.create({idFactory:()=>transferId,now:()=>now,schedule:(fn,delay)=>{tasks.push(fn);delays.push(delay);},call(op,fields,callback){callback(rawCall(op,fields));}});
pending.load(r=>completed.push(r));eq(queuedCommands,1);eq(tasks.length,1);eq(delays[0],50);now+=300001;drain();eq(completed[0].error.code,"preset_file_timeout");eq(operations.length,0);eq(host.__SFLD_presetFileSessionV1,null);
// Cancel/release wins over a late queue acknowledgement, with one user result.
reset();completed=[];let deferred=null,releases=0;
const late=files.create({idFactory:()=>transferId,call(op,fields,callback){if(op==="beginPresetFileRead"){deferred=()=>callback(rawCall(op,fields));return;}
    if(op==="releasePresetFile")releases++;callback(rawCall(op,fields));}});
const abandoned=late.load(r=>completed.push(r));abandoned.cancel();eq(completed.length,1);deferred();eq(releases,2);eq(host.__SFLD_presetFileSessionV1,null);eq(operations.length,0);

// Execute the actual Save Current/Import event handlers and generated evalScript.
const snapshots=require(path.join(root,"js/native_graph_snapshot.js")),transactions=require(path.join(root,"js/graph_transactions.js")),
    assets=require(path.join(root,"js/model_assets.js")),fixture=require("./native_snapshot_fixture.js");
class Element{constructor(){this.children=[];this.listeners={};this.value="";this.checked=true;}
    set textContent(v){this.text=v;this.children=[];}get textContent(){return this.text;}
    addEventListener(type,callback){this.listeners[type]=callback;}appendChild(child){this.children.push(child);}setAttribute(){}
    getContext(){return new Proxy({},{get:()=>()=>{}});}}
function ui(capturedGraph,meshFailure=false){
    reset();tasks=[];const elements={},calls=[],timers=new Map();let timer=0,loads=0;
    const browserHost={File,Date,app,$:{global:null}};browserHost.$.global=browserHost;
    const hostContext=vm.createContext(browserHost);
    browserHost.$.evalFile=function(file){loads++;vm.runInContext(load(file.fsName.split("cep_panel/")[1]),hostContext);};
    vm.runInContext(load("jsx/starfield_gateway.jsx"),hostContext);
    function reply(payload){return JSON.stringify({protocol:"org.starfieldfx.panel",version:1,gatewayBuild:"native-presets-61",...payload});}
    const captured=fixture.receipt({ok:true,target:{token:"target"},snapshot:{initialized:true,revision:11,checksum:"00000000",recordStamp:"stamp",
        graphHex:codec.toHex(capturedGraph),geometry:{width:1920,height:1080,pixelAspect:1},layerResources:[]}});
    const model=captured.snapshot.nativeNodes.find(n=>n.id===modelId);if(model)model.modelAsset={revision:17,bounds};
    browserHost.SFLD_getState=()=>{calls.push("getState");return reply({ok:true,target:{token:"target",comp:"Comp",layer:"Particles"}});};
    browserHost.SFLD_getGraphSnapshot=()=>{calls.push("getGraphSnapshot");return reply(captured);};
    browserHost.SFLD_beginModelAssetExport=text=>{const r=JSON.parse(text);calls.push(r.operation);eq(r.target.token,"target");eq(r.baseGraphRevision,11);
        return reply(meshFailure?{ok:false,error:{code:"stale_author",message:"Changed"}}:{ok:true,assetId:r.assetId});};
    browserHost.SFLD_readModelAssetPage=text=>{const r=JSON.parse(text);calls.push(r.operation);
        return reply({ok:true,assetId:r.assetId,state:"ready",page:0,pageCount:1,nodeId:modelId,source:model.parameters.find(p=>p.key==="13").value+1,
            revision:17,bounds,bytes:asset.meshHex.length/2,hex:asset.meshHex});};
    browserHost.SFLD_releaseModelAsset=()=>{calls.push("releaseModelAsset");return reply({ok:true});};
    const window={prompt:()=>"UI mesh",StarfieldPresets:presets,StarfieldGraphCodec:codec,StarfieldGraphTransactions:transactions,StarfieldNativeGraphSnapshot:snapshots,
        StarfieldModelAssets:assets,StarfieldPresetFiles:{create:options=>files.create({...options,schedule:fn=>tasks.push(fn)})},close(){},
        __adobe_cep__:{getSystemPath:()=>"C:/test/cep_panel",evalScript(script,callback){if(script.includes("return SFLD_readPresetFileModal("))nativeModal(browserHost);callback(vm.runInContext(script,hostContext));}}};
    const uiContext=vm.createContext({window,document:{getElementById:id=>elements[id] || (elements[id]=new Element()),createElement:()=>new Element()},
        prompt:()=>"UI mesh",setTimeout(fn,delay){timers.set(++timer,{fn,delay});return timer;},clearTimeout:id=>timers.delete(id),console,Date,JSON,Math});
    vm.runInContext(load("js/preset_manager.js"),uiContext);
    eq(calls,["getState","getGraphSnapshot"]);eq(loads,0);eq(timers.size,0);
    elements.save.listeners.click();drain();eq(timers.size,0);
    if(meshFailure){eq(elements.status.className,"error");eq(disk[saveDialog],undefined);eq(loads,0);eq(operations.length,0);return;}
    eq(elements.status.className,"");ok(elements.status.textContent.startsWith("Saved UI mesh"));eq(loads,1);
    const saved=presets.decode(disk[saveDialog]);eq(saved.modelAssets,model?[asset]:[]);
    if(model)eq(calls.slice(2),["getGraphSnapshot","beginModelAssetExport","readModelAssetPage","releaseModelAsset"]);
    else eq(calls.slice(2),["getGraphSnapshot"]);
    disk[openDialog]=disk[saveDialog];elements.import.listeners.click();drain();eq(elements.status.className,"");eq(timers.size,0);eq(loads,1);
    ok(elements.status.textContent.startsWith("Imported UI mesh"));eq(elements.add.disabled,false);
}
ui(imported);ui(graph(false));ui(presets.build("sparks",1080));ui(imported,true);
// Run the unchanged legacy preset checks against the isolated dependencies.
const legacyRequire=require("node:module").createRequire(path.join(__dirname,"preset_tests.js"));
function candidateRequire(name){return name.startsWith("../cep_panel/")?require(path.join(root,name.slice("../cep_panel/".length))):legacyRequire(name);}
candidateRequire.resolve=name=>name.startsWith("../cep_panel/")?path.join(root,name.slice("../cep_panel/".length)):legacyRequire.resolve(name);
vm.runInNewContext(fs.readFileSync(path.join(__dirname,"preset_tests.js"),"utf8"),{require:candidateRequire,console},{filename:"preset_tests.js (isolated dependencies)"});
console.log(`model_preset_file_tests: ${checks} checks passed; real isolated codec/client/gateway; no AE or disk IO`);
