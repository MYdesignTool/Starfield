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
reject(()=>presets.apply(presets.build("sparks",1080),{type:"applyPreset",presetGraph:entry.graph,presetModelAssets:entry.modelAssets,mode:"replace"},edits.randomId));
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
File.openDialog=()=>openDialog?new File(openDialog):null;File.saveDialog=()=>saveDialog?new File(saveDialog):null;
File.prototype.open=function(mode){operations.push(["open",this.fsName,mode]);if(fault.open){this.error="open failed";return false;}this.opened=true;this.mode=mode;if(mode==="w")disk[this.fsName]="";return true;};
File.prototype.read=function(){if(fault.read){this.error="read failed";return "";}return disk[this.fsName];};
File.prototype.write=function(text){disk[this.fsName]=fault.silentWrite?"corrupt":text;if(fault.write){disk[this.fsName]="partial";this.error="write failed";return false;}return true;};
File.prototype.close=function(){operations.push(["close",this.fsName]);this.opened=false;if(fault.close){this.error="close failed";return false;}return true;};
File.prototype.rename=function(name){operations.push(["rename",this.fsName,name]);
    if(fault.renameOld && this.fsName===saveDialog || fault.publish && this.fsName.endsWith(".tmp") || fault.rollback && this.fsName.endsWith(".bak"))return false;
    const dest=this.fsName.slice(0,this.fsName.lastIndexOf("/")+1)+name;if(dest in disk)return false;disk[dest]=disk[this.fsName];delete disk[this.fsName];this.fsName=dest;return true;};
File.prototype.remove=function(){operations.push(["remove",this.fsName]);if(fault.removeBackupThrows && this.fsName.endsWith(".bak"))throw new Error("cleanup failed");
    if(fault.removeBackup && this.fsName.endsWith(".bak") || fault.removeTemporary && this.fsName.endsWith(".tmp"))return false;delete disk[this.fsName];return true;};
const host={},context=vm.createContext({$:{global:host},File,confirm:()=>confirmed,Date:function(){this.getTime=()=>now;}});
function reload(){vm.runInContext(load("jsx/starfield_gateway.jsx"),context);vm.runInContext(load("jsx/preset_file_transport.jsx"),context);}
reload();
function call(op,extra){return JSON.parse(host["SFLD_"+op](JSON.stringify({protocol:"org.starfieldfx.panel",version:1,gatewayBuild:"native-presets-61",operation:op,transferId,...extra})));}
function reset(){disk={};now=1000;operations=[];fault={};openDialog="D:/user/import.sfldpreset";saveDialog="D:/user/输出.sfldpreset";confirmed=true;host.__SFLD_presetFileSessionV1=null;}
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
    ok(JSON.stringify(fields).length<262144);callback(call(op,fields));}});
function drain(){while(tasks.length)tasks.shift()();}
client.load(r=>completed.push(r));drain();eq(completed.length,1);eq(completed[0].text,large);eq(host.__SFLD_presetFileSessionV1,null);
completed=[];client.save(large,r=>completed.push(r));drain();eq(completed.length,1);eq(completed[0].ok,true);eq(disk[saveDialog],large);
completed=[];const cancelled=client.save(large,r=>completed.push(r));ok(tasks.length>0);cancelled.cancel();drain();eq(completed.length,1);eq(completed[0].error.code,"cancelled");eq(host.__SFLD_presetFileSessionV1,null);
reset();completed=[];client.save(large,r=>completed.push(r));now+=300001;drain();eq(completed.length,1);eq(completed[0].error.code,"preset_file_timeout");eq(host.__SFLD_presetFileSessionV1,null);
// A surrogate pair at a page boundary survives save/read unchanged.
reset();const unicode=JSON.stringify({format:"org.starfieldfx.preset",version:3,name:"Unicode",graphHex:JSON.parse(encoded).graphHex,resources:[],modelAssets:[asset],padding:"x".repeat(32000)+"😀".repeat(2000)});
completed=[];client.save(unicode,r=>completed.push(r));drain();eq(completed[0].ok,true);eq(disk[saveDialog],unicode);
disk[openDialog]=unicode;completed=[];client.load(r=>completed.push(r));drain();eq(completed[0].text,unicode);
eq(operations.filter(o=>o[0]==="open" && o[2]==="w" && !o[1].endsWith(".tmp")).length,0);

// Execute the actual Save Current/Import event handlers and generated evalScript.
const snapshots=require(path.join(root,"js/native_graph_snapshot.js")),transactions=require(path.join(root,"js/graph_transactions.js")),
    assets=require(path.join(root,"js/model_assets.js")),fixture=require("./native_snapshot_fixture.js");
class Element{constructor(){this.children=[];this.listeners={};this.value="";this.checked=true;}
    set textContent(v){this.text=v;this.children=[];}get textContent(){return this.text;}
    addEventListener(type,callback){this.listeners[type]=callback;}appendChild(child){this.children.push(child);}setAttribute(){}
    getContext(){return new Proxy({},{get:()=>()=>{}});}}
function ui(capturedGraph,meshFailure=false){
    reset();tasks=[];const elements={},calls=[],timers=new Map();let timer=0,loads=0;
    const browserHost={File,Date,$:{global:null}};browserHost.$.global=browserHost;
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
        __adobe_cep__:{getSystemPath:()=>"C:/test/cep_panel",evalScript(script,callback){callback(vm.runInContext(script,hostContext));}}};
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
