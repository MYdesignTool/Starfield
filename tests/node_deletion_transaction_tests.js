"use strict";
const assert=require("node:assert/strict"),path=require("node:path");
const root=process.env.STARFIELD_PANEL_ROOT;if(!root)throw new Error("Set isolated STARFIELD_PANEL_ROOT");
const load=n=>require(path.join(root,"js",n+".js")),codec=load("graph_codec"),edits=load("graph_edits"),presets=load("presets"),transactions=load("graph_transactions"),layout=load("graph_layout");
function capture(graph){const positions=layout.resolve(graph),output=graph.nodes.find(n=>n.type===edits.types.output),renderer={id:output.id,position:positions[output.id]};
    ["maxParticles","timeRemapEnabled","timeRemapSeconds","previewEnabled","previewChance","acceleration","timeSamplingHz","motionBlur","shutterAngle","shutterPhase","motionBlurType","motionBlurLevels","linearAccuracy","opacityBoost","motionBlurDisregard"].forEach((name,i)=>{const p=output.parameters.find(p=>String(p.key)===String(i+1));renderer[name]=p?p.value:[1000000,0,0,0,100,0,30,1,360,0,0,8,70,0,0][i];});
    return {nativeNodes:graph.nodes.filter(n=>n.type!==edits.types.output).map(n=>({...n,schemaVersion:edits.nativeRecordSchema(n),position:positions[n.id],outgoing:graph.edges.filter(e=>e.sourceNode===n.id).map(e=>({id:e.id,target:e.destinationNode}))})),renderer};}
let checks=0,sequence=1000;const uuid=()=> (++sequence).toString(16).padStart(32,"0");
for(const kind of ["emitter","particle","force","transform","model","material","motion"]){
    let graph=edits.apply(presets.build("sparks",1080),{type:"addNode",nodeType:kind},uuid);
    const added=graph.nodes[graph.nodes.length-1],calls=[];
    const snapshot={...capture(graph),initialized:true,revision:10,recordStamp:"base",graphHex:codec.toHex(graph),geometry:{width:1920,height:1080,pixelAspect:1}};
    let answer,received;
    const client=transactions.create({codec,edits,idFactory:uuid,
        call(operation,fields,callback){calls.push(operation);throw new Error("Deletion used ordinary/asset-export call: "+operation);},
        modelTransactions:{apply(transaction,meshes,callback){received=transaction;assert.equal(meshes.length,0);checks++;
            callback({ok:true,committed:true,target:{token:"p0-c2-l3"},snapshot:{...snapshot,nativeNodes:transaction.nodeManifest,renderer:transaction.rendererManifest,revision:11,graphHex:transaction.graphHex}});}}
    });
    client.apply({type:"deleteNodes",nodeIds:[added.id]},result=>answer=result,"p0-c2-l3",10,{ok:true,target:{token:"p0-c2-l3"},snapshot});
    assert.equal(answer && answer.ok,true,JSON.stringify(answer));checks++;
    assert.ok(received.baseNodeManifest.some(n=>n.id===added.id));checks++;
    assert.ok(!received.nodeManifest.some(n=>n.id===added.id));checks++;
    assert.deepEqual(calls,[]);checks++;
}
console.log(`node_deletion_transaction_tests: ${checks} checks passed; all seven kinds route through full backup, no AE qualification`);
