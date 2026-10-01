"use strict";

// Independent-effect transaction model, including AE indexed-group invalidation.
// This exercises the gateway/coordinator, not the native compiler or AE host.
const assert = require("node:assert/strict");
const fs = require("node:fs"), path = require("node:path"), vm = require("node:vm");
const codec = require("../cep_panel/js/graph_codec.js"), layout = require("../cep_panel/js/graph_layout.js");
const edits = require("../cep_panel/js/graph_edits.js"), transactions = require("../cep_panel/js/graph_transactions.js");
const view = require("../cep_panel/js/graph_view.js");
const source = fs.readFileSync(path.join(__dirname, "../cep_panel/jsx/starfield_gateway.jsx"), "utf8");
const uuid = n => n.toString(16).padStart(32, "0"), outputId = uuid(255);
let epoch = 0, rejectNext = false, revision = 0, commits = 0, roundWireNumbers = false, beforeSubmit = null;
let rejectNextAdd = null;
const undo = { begins: 0, ends: 0 }, clone = value => Array.isArray(value) ? value.slice() : value;
const renderer = { matchName: "org.starfieldfx.particle", name: "Starfield Particle", properties: {} };
const control = (name, value) => ({ name, value: clone(value) });
function nodeControls() {
    const values = { "Type":1, "Particles Per Second":100, "Random Seed":1, "Particle Size":10,
        "Opacity":1, "Origin":[1920,1080,1080], "Velocity X":0, "Velocity Y":0.3, "Velocity Z":0,
        "Disc Size":0.05, "Speed Random":0.15, "Size X":100, "Size Y":100, "Size Z":100,
        "Emission Speed":0, "Emission Speed Random":0, "Emission Angle X":0, "Emission Angle Y":0,
        "Emission Angle Z":0, "Direction":1, "Direction Span":60, "Color Start":[1,1,1,1],
        "Color End":[1,1,1,1], "Size":10, "Size Over Life":100, "Opacity Over Life":100,
        "Size Random":0, "Opacity Random":0, "Lifetime":2, "Size Curve Count":0, "Opacity Curve Count":0,
        "Gravity":[0,0,0], "Linear Drag":0, "Panel Sync Guard":0,
        "Node Layout X":0, "Node Layout Y":0, "Outgoing Connection Count":0 };
    for(let i=0;i<8;i++) values["Node UUID "+i]=0;
    for(let s=0;s<4;s++) for(let i=0;i<8;i++) {
        values["Connection "+s+" Target UUID "+i]=0; values["Connection "+s+" Edge UUID "+i]=0;
    }
    for(const label of ["Size","Opacity"]) for(let i=0;i<8;i++) {
        values[label+" Curve "+i+" Age"]=0; values[label+" Curve "+i+" Value"]=100;
    }
    return Object.fromEntries(Object.entries(values).map(([name,value])=>[name,control(name,value)]));
}
for(const [index,name,value] of [[27,"Max Particles",1000],[39,"Layout Output X",180],
    [40,"Layout Output Y",526],[41,"Graph Revision",0],[42,"Panel Graph Sync Guard",0],
    [43,"Commit Graph Edit",0],[44,"Graph Edit Receipt",0],[89,"Node Effects Ready",0],[90,"Graph Checksum High",0],[91,"Graph Checksum Low",0]]) {
    const p=control(name,value); p.propertyIndex=index; renderer.properties[index]=renderer.properties[name]=p;
}
const items=[renderer];
function assertFresh(captured) { if(captured!==epoch) throw new Error("Invalid indexed-group reference"); }
function wrapProperty(p,captured) {
    return {name:p.name,propertyIndex:p.propertyIndex,canSetExpression:false,
        get value(){assertFresh(captured);return clone(p.value);},
        get expression(){throw new Error("AEGP_CanVaryOverTime: expressions are forbidden on node records");},
        set expression(_){throw new Error("Expression requests are unsupported");},
        setValue(v){assertFresh(captured);p.value=clone(v);if(p===renderer.properties[43]) compile(v);} };
}
function wrapEffect(raw) {
    const captured=epoch;
    return {matchName:raw.matchName,get name(){assertFresh(captured);return raw.name;},
        set name(v){assertFresh(captured);raw.name=v;},
        property(key){assertFresh(captured);const p=raw.properties[key];return p?wrapProperty(p,captured):null;},
        remove(){assertFresh(captured);items.splice(items.indexOf(raw),1);epoch++;} };
}
function parade() {
    return {get numProperties(){return items.length;},property(i){return items[i-1]?wrapEffect(items[i-1]):null;},
        canAddProperty(matchName){return matchName !== "org.starfieldfx.node.particle" || !rejectNextAdd;},
        addProperty(matchName){if(matchName===rejectNextAdd) throw new Error("Host refused scripted effect creation");
            items.push({matchName,name:"",properties:nodeControls()});epoch++;return wrapEffect(items[items.length-1]);} };
}
const layer={id:29,name:"Particle Layer",selected:true,width:3840,height:2160,source:{pixelAspect:1},
    property(name){return name==="ADBE Effect Parade"?parade():null;} };
function CompItem() {}
const comp=new CompItem();Object.assign(comp,{id:17,name:"Test Comp",numLayers:1,layer:()=>layer});
const exported={};
vm.runInNewContext("JSON.stringify = wireStringify;\n"+source,{CompItem,wireStringify:value=>JSON.stringify(value,
    (_,item)=>roundWireNumbers&&typeof item==="number"?Number(item.toPrecision(12)):item),
    $:{global:exported},app:{project:{activeItem:comp,rootFolder:{id:5}},
    beginUndoGroup(){undo.begins++;},endUndoGroup(){undo.ends++;}}});
const readUuid=(c,prefix)=>Array.from({length:8},(_,i)=>c[prefix+i].value.toString(16).padStart(4,"0")).join("");
function graphFromEffects() {
    const graph={version:1,nodes:[],edges:[],optionalRecords:[]},positions={},p=(key,type,value)=>({key:String(key),type,value});
    for(const effect of items.filter(effect=>effect!==renderer)) {
        const c=effect.properties,v=name=>clone(c[name].value),id=readUuid(c,"Node UUID ");
        const type="org.starfieldfx.nodes."+effect.matchName.split(".").pop();
        let parameters;
        if(type===edits.types.emitter) {
            const origin=v("Origin");
            parameters=[p(2,4,v("Particles Per Second")),p(3,3,v("Random Seed")),p(5,3,v("Type")-1),
                p(6,5,[(origin[0]-1920)/2160,(1080-origin[1])/2160,(origin[2]-1080)/2160]),
                p(7,5,[v("Velocity X"),v("Velocity Y"),v("Velocity Z")]),p(8,4,v("Particle Size")),
                p(9,4,v("Opacity")),p(10,4,v("Disc Size")),p(11,4,v("Speed Random")),
                p(12,4,v("Emission Speed")),p(13,4,v("Emission Speed Random")),p(14,4,v("Emission Angle X")),
                p(15,4,v("Emission Angle Y")),p(16,4,v("Emission Angle Z")),p(17,3,v("Direction")-1),
                p(18,4,v("Direction Span")),p(19,4,v("Size X")),p(20,4,v("Size Y")),p(21,4,v("Size Z"))];
        } else if(type===edits.types.force) parameters=[p(1,5,v("Gravity")),p(2,4,v("Linear Drag"))];
        else {
            parameters=[p(1,5,v("Color Start").slice(0,3)),p(2,5,v("Color End").slice(0,3)),p(3,4,v("Size")),
                p(4,4,v("Size Over Life")),p(5,4,v("Opacity")),p(6,4,v("Opacity Over Life")),
                p(9,4,v("Size Random")),p(10,4,v("Opacity Random"))];
            if(type===edits.types.particle) parameters.push(p(11,4,v("Lifetime")));
            for(const [key,label] of [[7,"Size"],[8,"Opacity"]]) if(v(label+" Curve Count")) {
                const points=Array.from({length:v(label+" Curve Count")},(_,i)=>({age:v(label+" Curve "+i+" Age"),value:v(label+" Curve "+i+" Value")}));
                parameters.push(p(key,7,view.encodeCurve(points)));
            }
        }
        graph.nodes.push({id,type,schemaVersion:type===edits.types.emitter?3:type===edits.types.particle?2:1,parameters});
        positions[id]={x:v("Node Layout X"),y:v("Node Layout Y")};
        for(let s=0;s<v("Outgoing Connection Count");s++) graph.edges.push({id:readUuid(c,"Connection "+s+" Edge UUID "),
            sourceNode:id,sourcePort:type===edits.types.emitter?"1":"2",
            destinationNode:readUuid(c,"Connection "+s+" Target UUID "),destinationPort:"1"});
    }
    graph.nodes.push({id:outputId,type:edits.types.output,schemaVersion:2,parameters:[p(1,3,renderer.properties[27].value)]});
    positions[outputId]={x:renderer.properties[39].value,y:renderer.properties[40].value};
    return layout.set(graph,positions);
}
function publish(graph) {
    const bytes=codec.serialize(graph),crc=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength).getUint32(24,true);
    renderer.properties[41].value=revision;
    renderer.properties[90].value=crc>>>16;renderer.properties[91].value=crc&65535;
}
function compile(nonce) {
    commits++;if(rejectNext){rejectNext=false;renderer.properties[44].value=-nonce;return;}
    revision++;publish(graphFromEffects());renderer.properties[44].value=nonce;
}
function invoke(operation,fields={}) {
    return JSON.parse(exported["SFLD_"+operation](JSON.stringify(Object.assign({protocol:"org.starfieldfx.panel",version:1,
        requestId:operation,operation,target:{token:"p5-c17-l29"},pinTarget:false},fields))));
}
let next=10;
const snapshots=require("../cep_panel/js/native_graph_snapshot.js");
assert.equal(invoke("getGraphSnapshot").snapshot.initialized,false);
const boot=snapshots.normalize(invoke("syncGraphSnapshot"));
assert.equal(boot.ok,true,JSON.stringify(boot));assert.equal(boot.snapshot.initialized,true);
assert.equal(boot.snapshot.checksumMatches,true,"ordinary record codec must match the compiled payload CRC");
const initial=codec.fromHex(boot.snapshot.graphHex);
const emitterId=initial.nodes.find(n=>n.type===edits.types.emitter).id,particleId=initial.nodes.find(n=>n.type===edits.types.particle).id;
const client=transactions.create({codec,edits,idFactory:()=>uuid(next++),call:(op,fields,cb)=>{
    if(op==="submitGraph"&&beforeSubmit){const change=beforeSubmit;beforeSubmit=null;change();}
    cb(invoke(op,fields));
}});
const snapshot=()=>snapshots.normalize(invoke("getGraphSnapshot")).snapshot;
function apply(edit){let result;client.apply(edit,r=>result=r,"p5-c17-l29");return result;}
let ensured;client.ensureNativeEffects(snapshot(),"p5-c17-l29",r=>ensured=r);
assert.equal(ensured.ok,true,JSON.stringify(ensured));assert.equal(items.length,3);
assert.equal(renderer.properties[89].value,1);assert.deepEqual(Array.from(items[1].properties.Origin.value),[1920,1080,1080]);
assert.equal(renderer.properties[42].value,0);
// Exercise a host's decimal JSON rounding across the JSX/CEP boundary.
const emitterEffect=items.find(e=>e.matchName==="org.starfieldfx.node.emitter");
emitterEffect.properties["Velocity Y"].value=0.30000001192092896;
roundWireNumbers=true;publish(graphFromEffects());
const rounded=snapshot();
assert.equal(rounded.checksumMatches,false,"decimal projection need not match native payload CRC");
const unchangedCommits=commits;
client.ensureNativeEffects(rounded,"p5-c17-l29",r=>ensured=r);
assert.equal(ensured.ok,true,"read-only inspection must accept current native records");
assert.equal(commits,unchangedCommits,"refresh must not recompile an approximate projection");
assert.equal(apply({type:"moveNodes",positions:{[emitterId]:{x:-161.125,y:-80}}}).ok,true);
beforeSubmit=()=>{emitterEffect.properties["Particles Per Second"].value=123;};
const staleCount=items.length;
assert.equal(apply({type:"addNode",nodeType:"force",position:{x:0,y:0}}).error.code,"stale_graph");
assert.equal(items.length,staleCount,"a real intervening edit rejects before effect creation");
roundWireNumbers=false;publish(graphFromEffects());
const duplicate=apply({type:"duplicateNodes",nodeIds:[particleId],offset:{x:160,y:0}});
assert.equal(duplicate.ok,true,JSON.stringify(duplicate));assert.equal(items.length,4);
const copiedId=codec.fromHex(snapshot().graphHex).nodes.find(n=>n.type===edits.types.particle&&n.id!==particleId).id;
assert.equal(apply({type:"setParameters",changes:[{nodeId:copiedId,parameterKey:"3",valueType:4,value:64}]}).ok,true);
assert.equal(items[2].properties.Size.value,10);assert.equal(items[3].properties.Size.value,64);
assert.equal(apply({type:"moveNodes",positions:{[outputId]:{x:-300,y:-200}}}).ok,true);
assert.equal(renderer.properties[39].value,-300);assert.equal(renderer.properties[40].value,-200);
assert.equal(apply({type:"setParameters",changes:[{nodeId:outputId,parameterKey:"1",valueType:3,value:8000}]}).ok,true);
assert.equal(renderer.properties[27].value,8000);
rejectNext=true;assert.equal(apply({type:"deleteNodes",nodeIds:[copiedId]}).ok,false);
assert.equal(items.length,4,"rejected edit restores the removed effect");
assert.equal(codec.fromHex(snapshot().graphHex).nodes.length,4,"rollback also recompiles the render snapshot");
assert.equal(renderer.properties[42].value,0);
assert.equal(apply({type:"deleteNodes",nodeIds:[copiedId]}).ok,true);assert.equal(items.length,3);
const opacity=view.encodeCurve([{age:0,value:100},{age:0.2,value:37.5},{age:1,value:0}]);
assert.equal(apply({type:"setParameters",changes:[{nodeId:particleId,parameterKey:"8",valueType:7,value:opacity}]}).ok,true);
assert.equal(snapshot().checksumMatches,true,"manual ExtendScript Float64 encoding matches the native curve codec");
assert.equal(apply({type:"setParameters",changes:[{nodeId:particleId,parameterKey:"7",valueType:7,
    value:view.encodeCurve([{age:0,value:10},{age:0.8,value:90},{age:1,value:100}])}]}).ok,true);
assert.deepEqual(Array.from(codec.fromHex(snapshot().graphHex).nodes.find(n=>n.id===particleId).parameters.find(p=>p.key==="8").value),
    Array.from(opacity),"editing size preserves opacity knots");
const forceAdd=apply({type:"addNode",nodeType:"force",position:{x:-400,y:-600}});
assert.equal(forceAdd.ok,true,JSON.stringify(forceAdd));
const forceId=codec.fromHex(snapshot().graphHex).nodes.find(n=>n.type===edits.types.force).id;
const particleOutput=codec.fromHex(snapshot().graphHex).edges.find(e=>e.sourceNode===particleId&&e.destinationNode===outputId);
assert.equal(apply({type:"insertNode",nodeId:forceId,edgeId:particleOutput.id,position:{x:-400,y:-600}}).ok,true);
let currentGraph=codec.fromHex(snapshot().graphHex);
assert.ok(currentGraph.edges.some(e=>e.sourceNode===particleId&&e.destinationNode===forceId));
assert.ok(currentGraph.edges.some(e=>e.sourceNode===forceId&&e.destinationNode===outputId));
let forceOutput=currentGraph.edges.find(e=>e.sourceNode===forceId&&e.destinationNode===outputId);
assert.equal(apply({type:"disconnect",edgeId:forceOutput.id}).ok,true);
assert.equal(apply({type:"connect",from:forceId,to:outputId}).ok,true);
// AE reordering must not change graph identity, topology or independently saved values.
const beforeReorder=snapshot().graphHex;
items.reverse();epoch++;
assert.equal(snapshot().graphHex,beforeReorder);
const nativeCopy=JSON.parse(JSON.stringify(items.find(e=>e.matchName==="org.starfieldfx.node.particle")));
items.push(nativeCopy);epoch++;
assert.equal(snapshot().initialized,false,"AE-level Ctrl+D needs new node and edge identities");
assert.equal(invoke("syncGraphSnapshot").ok,true);
assert.equal(snapshot().checksumMatches,true);
currentGraph=codec.fromHex(snapshot().graphHex);
assert.equal(currentGraph.nodes.filter(n=>n.type===edits.types.particle).length,2);
assert.equal(new Set(currentGraph.nodes.map(n=>n.id)).size,currentGraph.nodes.length);
// Direct deletion follows the real Effect Parade and trims incoming links.
const directlyDeleted=items.find(e=>e.matchName==="org.starfieldfx.node.force");
items.splice(items.indexOf(directlyDeleted),1);epoch++;
assert.equal(snapshot().initialized,false);
assert.equal(invoke("syncGraphSnapshot").ok,true);
assert.ok(!codec.fromHex(snapshot().graphHex).nodes.some(n=>n.id===forceId));
assert.ok(!codec.fromHex(snapshot().graphHex).edges.some(e=>e.destinationNode===forceId));
const all=codec.fromHex(snapshot().graphHex).nodes.filter(n=>n.type!==edits.types.output).map(n=>n.id);
assert.equal(apply({type:"deleteNodes",nodeIds:all}).ok,true,"all editable nodes can be removed");
assert.equal(items.length,1);assert.equal(codec.fromHex(snapshot().graphHex).nodes.length,1);
assert.equal(invoke("syncGraphSnapshot").ok,true);
assert.equal(items.length,1,"an intentionally empty graph must not recreate initial nodes");
// A partial fresh initialization with only an unassigned emitter still creates
// Particle and the initial wiring. It is distinct from intentional ready=1 deletion.
renderer.properties[89].value=0;renderer.properties[41].value=0;revision=0;
items.push({matchName:"org.starfieldfx.node.emitter",name:"Starfield Emitter",properties:nodeControls()});epoch++;
const recovered=snapshots.normalize(invoke("syncGraphSnapshot"));
assert.equal(recovered.ok,true,JSON.stringify(recovered));
const recoveredGraph=codec.fromHex(recovered.snapshot.graphHex);
assert.equal(items.length,3);
assert.equal(recoveredGraph.nodes.filter(n=>n.type===edits.types.emitter).length,1);
assert.equal(recoveredGraph.nodes.filter(n=>n.type===edits.types.particle).length,1);
assert.equal(recoveredGraph.edges.length,2);
const beforeFailedAdd=snapshot().graphHex, beforeFailedAddCount=items.length;
rejectNextAdd="org.starfieldfx.node.particle";
const refusedParticle=apply({type:"addNode",nodeType:"particle",position:{x:20,y:20}});
assert.equal(refusedParticle.ok,false);
assert.match(refusedParticle.error.message,/Particle \(org\.starfieldfx\.node\.particle\), create effect:.*Host refused/);
assert.match(refusedParticle.error.message,/canAddProperty=false/);
assert.equal(items.length,beforeFailedAddCount,"failed Particle creation must not change the Effect Parade");
assert.equal(snapshot().graphHex,beforeFailedAdd,"failed creation preserves existing nodes and connections");
assert.equal(renderer.properties[42].value,0);
rejectNextAdd=null;
assert.equal(undo.begins,undo.ends);assert.ok(commits>=8);
console.log("Native node gateway checks passed: bootstrap, add, copy, native Ctrl+D, independent values/curves, signed layout, insert/connect/disconnect, AE reorder/deletion, Output, numeric receipts without expressions, rollback and delete all.");
