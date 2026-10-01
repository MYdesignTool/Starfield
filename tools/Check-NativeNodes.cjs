"use strict";
// Run after launching AE with -r tools/native_node_host_bridge.jsx. Each RPC has
// a 20-second deadline; failures stop this check instead of retrying AE operations.
const fs=require("node:fs"),path=require("node:path"),assert=require("node:assert/strict");
const codec=require("../cep_panel/js/graph_codec.js"),edits=require("../cep_panel/js/graph_edits.js");
const transactions=require("../cep_panel/js/graph_transactions.js"),view=require("../cep_panel/js/graph_view.js");
const folder=path.join(__dirname,"../artifacts/host-check");
let serial=0,token;const evidence=[];
const pause=ms=>new Promise(resolve=>setTimeout(resolve,ms));
async function awaitFile(name,test,timeout=20000) {
    const deadline=Date.now()+timeout;
    while(Date.now()<deadline) {
        try {const value=JSON.parse(fs.readFileSync(path.join(folder,name),"utf8").replace(/^\uFEFF/,""));if(test(value)) return value;} catch(_) {}
        await pause(100);
    }
    throw new Error("Timed out waiting for "+name);
}
async function rpc(data) {
    const id=++serial,temp=path.join(folder,"in.tmp"),target=path.join(folder,"in.json");
    fs.writeFileSync(temp,JSON.stringify(Object.assign({serial:id},data)),"utf8");fs.renameSync(temp,target);
    return (await awaitFile("out.json",response=>response.serial===id)).response;
}
async function call(operation,fields) {
    const body=Object.assign({protocol:"org.starfieldfx.panel",version:1,operation,requestId:"host-"+serial,
        target:token?{token}:undefined,pinTarget:false},fields);
    return rpc({operation,body:JSON.stringify(body)});
}
function checked(result,stage) {
    assert.equal(result&&result.ok,true,stage+": "+JSON.stringify(result));evidence.push(stage);return result;
}
const client=transactions.create({codec,edits,call:(op,fields,callback)=>call(op,fields).then(callback)});
const apply=edit=>new Promise(resolve=>client.apply(edit,resolve,token));
const ensure=snapshot=>new Promise(resolve=>client.ensureNativeEffects(snapshot,token,resolve));
(async function() {
    let failure=null,host;
    try {
        host=await awaitFile("ready.json",()=>true,45000);assert.equal(host.ok,true,JSON.stringify(host));
        token=checked(await call("getState"),"discover target").target.token;
        let snapshot=checked(await call("syncGraphSnapshot"),"initialize snapshot").snapshot;
        const boot=checked(await ensure(snapshot),"create native node effects");
        snapshot=boot.snapshot||snapshot;
        let graph=codec.fromHex(snapshot.graphHex);
        const particle=graph.nodes.find(n=>n.type===edits.types.particle),emitter=graph.nodes.find(n=>n.type===edits.types.emitter);
        assert.ok(particle&&emitter);
        const originalSize=particle.parameters.find(p=>p.key==="3").value;
        let copied=checked(await apply({type:"duplicateNodes",nodeIds:[particle.id],offset:{x:150,y:-100}}),"duplicate Particle");
        graph=codec.fromHex(copied.snapshot.graphHex);
        const copy=graph.nodes.find(n=>n.type===edits.types.particle&&n.id!==particle.id);
        assert.ok(copy);
        checked(await apply({type:"setParameters",changes:[{nodeId:copy.id,parameterKey:"3",valueType:4,value:64}]}),"edit copied Particle");
        let inspection=checked(await rpc({action:"inspect"}),"inspect independent effects");
        assert.equal(inspection.effects.find(e=>e.id===particle.id).size,originalSize);
        assert.equal(inspection.effects.find(e=>e.id===copy.id).size,64);
        checked(await apply({type:"moveNodes",positions:{[copy.id]:{x:-270,y:-160}}}),"save signed node position");
        checked(await apply({type:"setParameters",changes:[{nodeId:copy.id,parameterKey:"8",valueType:7,
            value:view.encodeCurve([{age:0,value:100},{age:0.4,value:30},{age:1,value:0}])}]}),"save opacity curve");
        const latest=checked(await call("getGraphSnapshot"),"read saved graph").snapshot;
        const edge=codec.fromHex(latest.graphHex).edges.find(e=>e.sourceNode===emitter.id&&e.destinationNode===copy.id);
        assert.ok(edge);
        checked(await apply({type:"disconnect",edgeId:edge.id}),"disconnect copied branch");
        checked(await apply({type:"connect",from:emitter.id,to:copy.id}),"reconnect copied branch");
        checked(await apply({type:"deleteNodes",nodeIds:[copy.id]}),"delete copied Particle");
        inspection=checked(await rpc({action:"inspect"}),"inspect removed native effect");
        assert.ok(!inspection.effects.some(e=>e.id===copy.id));
        checked(await rpc({action:"setNodeSize",nodeId:particle.id,value:23}),"edit native Effect Controls stream");
        const direct=codec.fromHex(checked(await call("getGraphSnapshot"),"read native edit snapshot").snapshot.graphHex);
        assert.equal(direct.nodes.find(n=>n.id===particle.id).parameters.find(p=>p.key==="3").value,23);
        checked(await rpc({action:"deleteNode",nodeId:particle.id}),"delete native effect directly");
        const before=checked(await call("getGraphSnapshot"),"read before native deletion reconciliation").snapshot;
        const reconciled=checked(await ensure(before),"reconcile native deletion");
        const after=codec.fromHex((reconciled.snapshot||checked(await call("getGraphSnapshot"),"read reconciled snapshot").snapshot).graphHex);
        assert.ok(!after.nodes.some(n=>n.id===particle.id));
    } catch(error) { failure=error.stack||String(error); }
    finally {
        try {checked(await rpc({action:"finish"}),"remove temporary test comp");} catch(error) {failure=failure||String(error);}
        const report={host:host&&host.version,passed:evidence,failure};
        fs.writeFileSync(path.join(folder,"result.json"),JSON.stringify(report,null,2));
        console.log(JSON.stringify(report,null,2));process.exitCode=failure?1:0;
    }
}());
