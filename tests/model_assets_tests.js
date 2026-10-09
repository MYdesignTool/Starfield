"use strict";
const assert=require("node:assert/strict"),assets=require("../tools/candidates/model_assets.js");
let checks=0;const eq=(a,b)=>{checks++;assert.deepEqual(a,b);},reject=(fn,pattern)=>{checks++;assert.throws(fn,pattern || /Model|model|SFMG1/);};
function crc(bytes){let n=0xffffffff;for(const byte of bytes){n^=byte;for(let i=0;i<8;i++)n=(n>>>1)^((n&1)?0xedb88320:0);}return (n^0xffffffff)>>>0;}
function fix(buffer){buffer.writeUInt32LE(crc(buffer.subarray(32)),12);return buffer.toString("hex");}
function mesh(count=3,attributes=false){const uv=attributes?1:0,normal=uv,b=Buffer.alloc(32+count*32+uv*24+normal*24+36);
    b.writeUInt32LE(0x474d4653,0);b.writeUInt16LE(1,4);b.writeUInt16LE(32,6);b.writeUInt32LE(b.length,8);
    [count,uv,normal,1].forEach((n,i)=>b.writeUInt32LE(n,16+i*4));
    for(let i=0;i<count;i++){b.writeDoubleLE(i===1?1:i>2?100:0,32+i*32);b.writeDoubleLE(i===2?1:0,40+i*32);b.writeDoubleLE(1,56+i*32);}
    if(attributes)b.writeDoubleLE(1,32+count*32+24+16);
    const triangle=32+count*32+uv*24+normal*24;
    for(let c=0;c<3;c++){b.writeUInt32LE(c,triangle+c*12);b.writeUInt32LE(attributes?0:0xffffffff,triangle+c*12+4);b.writeUInt32LE(attributes?0:0xffffffff,triangle+c*12+8);}fix(b);return b;}
const nodeId="0123456789abcdef0123456789abcdef",model="org.starfieldfx.nodes.model",box=[0,0,0,1,1,0];
function graph(source=1){const bounds=Buffer.alloc(48);(source?box:[-.5,-.5,-.5,.5,.5,.5]).forEach((v,i)=>bounds.writeDoubleLE(v,i*8));
    return {nodes:[{id:nodeId,type:model,parameters:[{key:"1",type:7,value:Uint8Array.from(source?Buffer.from(nodeId,"hex"):new Uint8Array(16))},
        {key:"2",type:3,value:source?17:0},{key:"12",type:7,value:Uint8Array.from(bounds)},{key:"13",type:3,value:source}]}]};}
const asset={nodeId,revision:17,bounds:box,meshHex:mesh().toString("hex")};
eq(assets.validateMesh(asset.meshHex),{bytes:164,bounds:box,positions:3,textureCoordinates:0,normals:0,triangles:1});
eq(assets.validateMesh(mesh(4,true).toString("hex")).bounds,box); // Unused far vertex does not affect referenced bounds.
eq(assets.validate(graph(),[asset]),[asset]);eq(assets.validate(graph(0),[asset]),[asset]);eq(assets.validate(graph(0),[]),[]);
reject(()=>assets.validate(graph(),[]),/no portable mesh/);
for(const value of [null,"",asset.meshHex.toUpperCase(),asset.meshHex+"0","zz".repeat(32)])reject(()=>assets.validateMesh(value));
for(const mutation of [b=>b.writeUInt16LE(2,4),b=>b.writeUInt16LE(0,6),b=>b.writeUInt32LE(163,8),b=>b.writeUInt32LE(65537,16),
    b=>b.writeUInt32LE(0,28),b=>b.writeUInt32LE(1,12),b=>b.writeDoubleLE(NaN,32),b=>b.writeDoubleLE(1e9+1,32),
    b=>b.writeDoubleLE(Infinity,56),b=>b.writeUInt32LE(3,128),b=>b.writeUInt32LE(0,132),b=>b.writeUInt32LE(0,136),
    b=>b.writeUInt32LE(0,152)]){
    const b=mesh();mutation(b);if(b.readUInt32LE(12)===crc(mesh().subarray(32)))fix(b);reject(()=>assets.validateMesh(b.toString("hex")));
}
const zeroNormal=mesh(3,true);zeroNormal.writeDoubleLE(0,32+3*32+24+16);reject(()=>assets.validateMesh(fix(zeroNormal)),/zero Model normal/);
for(const mutation of [a=>{a.nodeId="f".repeat(32);},a=>{a.revision=0;},a=>{a.bounds=[0,0,0,1,1,1];},a=>{a.revision=18;}]){
    const a=JSON.parse(JSON.stringify(asset));mutation(a);reject(()=>assets.validate(graph(),[a]));}
reject(()=>assets.validate(graph(),[asset,asset]),/duplicate/);
let g=graph();g.nodes[0].parameters[0].value[0]^=1;reject(()=>assets.validate(g,[asset]),/identity/);
g=graph();g.nodes[0].parameters[2].value=Array.from(g.nodes[0].parameters[2].value);g.nodes[0].parameters[2].value[0]=256;reject(()=>assets.validate(g,[asset]),/bounds byte/);
const huge=mesh(65536).toString("hex");eq(assets.validateMesh(huge).positions,65536);
let now=0,result,calls=[],tasks=[],responseMutator=null,pendingOnce=false,asyncBegin=false,beginCallback;
function snapshot(hex=asset.meshHex){return {revision:11,nativeNodes:[{id:nodeId,type:model,parameters:[{key:"13",type:3,value:0}],
    modelAsset:{revision:17,bounds:box},payload:hex}]};}
function client(hex=asset.meshHex){calls=[];tasks=[];result=null;now=0;
    return assets.create({now:()=>now,idFactory:()=>"a".repeat(32),schedule(fn){tasks.push(fn);},call(operation,fields,callback){
        calls.push({operation,fields});eq(fields.pinTarget,true);eq(fields.target.token,"pinned");
        if(operation==="releaseModelAsset"){callback({ok:true});return;}
        if(operation==="beginModelAssetExport"){
            if(asyncBegin){beginCallback=callback;return;}callback({ok:true});return;
        }
        eq(operation,"readModelAssetPage");let response;
        if(pendingOnce){pendingOnce=false;response={ok:true,state:"receiving",assetId:fields.assetId};}
        else response={ok:true,state:"ready",assetId:fields.assetId,page:fields.page,pageCount:Math.ceil(hex.length/65536),bytes:hex.length/2,
            nodeId,source:1,revision:17,bounds:box,hex:hex.substr(fields.page*65536,65536)};
        if(responseMutator)responseMutator(response);callback(response);
    }});
}
let c=client();c.collect(snapshot(),"pinned",r=>{result=r;});eq(result,{ok:true,assets:[asset]});eq(calls.filter(c=>c.operation==="releaseModelAsset").length,1);
c=client(huge);c.collect(snapshot(huge),"pinned",r=>{result=r;});eq(result.ok,true);eq(result.assets[0].meshHex,huge);eq(calls.filter(c=>c.operation==="readModelAssetPage").length,65);
for(const mutation of [r=>{r.assetId="f".repeat(32);},r=>{r.revision=18;},r=>{r.source=2;},r=>{r.page=1;},r=>{r.bytes=8*1024*1024+1;},
    r=>{r.bounds=[0,0,0,1,1,1];},r=>{r.hex="ab";},r=>{r.pageCount=2;},r=>{r.nodeId="f".repeat(32);},r=>{r.hex="z".repeat(r.hex.length);}]){
    responseMutator=mutation;c=client();c.collect(snapshot(),"pinned",r=>{result=r;});eq(result.ok,false);eq(calls.filter(c=>c.operation==="releaseModelAsset").length,1);
}responseMutator=null;
pendingOnce=true;c=client();c.collect(snapshot(),"pinned",r=>{result=r;});eq(result,null);eq(tasks.length,1);tasks.shift()();eq(result.ok,true);
pendingOnce=true;c=client();let completed=0,handle=c.collect(snapshot(),"pinned",r=>{result=r;completed++;});handle.cancel();eq(result.error.code,"cancelled");tasks.shift()();eq(completed,1);
pendingOnce=true;c=client();c.collect(snapshot(),"pinned",r=>{result=r;});now=60001;tasks.shift()();eq(result.error.code,"model_asset_timeout");
asyncBegin=true;c=client();completed=0;handle=c.collect(snapshot(),"pinned",r=>{result=r;completed++;});handle.cancel();beginCallback({ok:true});eq(completed,1);eq(calls.filter(c=>c.operation==="readModelAssetPage").length,0);asyncBegin=false;
c=client();c.collect({revision:11,nativeNodes:[]},"pinned",r=>{result=r;});eq(result,{ok:true,assets:[]});eq(calls.length,0);
reject(()=>c.collect(null,"pinned",()=>{}),/captured snapshot/);
console.log(`model_assets_tests: ${checks} checks passed; portable SFMG1 validation/export client; no AE qualification`);
