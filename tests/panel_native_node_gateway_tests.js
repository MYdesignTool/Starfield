"use strict";

// Independent-effect transaction model, including AE indexed-group invalidation.
// This exercises the gateway/coordinator, not the native compiler or AE host.
const assert = require("node:assert/strict");
const fs = require("node:fs"), path = require("node:path"), vm = require("node:vm");
const panelRoot=process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"../cep_panel");
const codec = require(path.join(panelRoot,"js/graph_codec.js")), layout = require(path.join(panelRoot,"js/graph_layout.js"));
const edits = require(path.join(panelRoot,"js/graph_edits.js")), transactions = require(path.join(panelRoot,"js/graph_transactions.js"));
const view = require(path.join(panelRoot,"js/graph_view.js"));
const source = fs.readFileSync(path.join(panelRoot,"jsx/starfield_gateway.jsx"), "utf8");
const uuid = n => n.toString(16).padStart(32, "0"), outputId = uuid(255);
let epoch = 0, rejectNext = false, revision = 0, commits = 0, roundWireNumbers = false, beforeSubmit = null;
let rejectNextAdd = null;
let nodePropertyReads = 0;
let numericNodeVisits = 0, hideDirectCurveIds = false;
const undo = { begins: 0, ends: 0 }, clone = value => Array.isArray(value) ? value.slice() : value;
const renderer = { matchName: "org.starfieldfx.particle", name: "Starfield Particle", properties: {} };
const {mainControls,nodeControls}=require("./node_property_fixture.js");
renderer.properties=mainControls();
renderer.properties["Max Particles"].value=1000;
const items=[renderer];
function assertFresh(captured) { if(captured!==epoch) throw new Error("Invalid indexed-group reference"); }
function wrapProperty(p,captured,isNode) {
    return {name:p.name,matchName:p.matchName,propertyIndex:p.propertyIndex,canSetExpression:false,
        get value(){assertFresh(captured);if(isNode)nodePropertyReads++;return clone(p.value);},
        get expression(){throw new Error("AEGP_CanVaryOverTime: expressions are forbidden on node records");},
        set expression(_){throw new Error("Expression requests are unsupported");},
        setValue(v){assertFresh(captured);p.value=clone(v);if(p===renderer.properties["Commit Graph Edit"]) compile(v);} };
}
function wrapEffect(raw) {
    const captured=epoch;
    const ordered=Array.from(new Set(Object.values(raw.properties)));
    return {matchName:raw.matchName,get numProperties(){assertFresh(captured);return ordered.length;},get name(){assertFresh(captured);return raw.name;},
        set name(v){assertFresh(captured);raw.name=v;},
        property(key){assertFresh(captured);
            if(hideDirectCurveIds && raw.matchName==="org.starfieldfx.node.particle" &&
                typeof key==="string" && /-(0?(700|800|960)|361[012])$/.test(key))return null;
            if(raw!==renderer && typeof key==="number")numericNodeVisits++;
            const p=raw.properties[key] || (typeof key==="number"?ordered[key-1]:null);
            return p?wrapProperty(p,captured,raw!==renderer):null;},
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
const comp=new CompItem();Object.assign(comp,{id:17,name:"Test Comp",time:0,numLayers:1,layer:()=>layer});
const exported={};
vm.runInNewContext("JSON.stringify = wireStringify;\n"+source,{CompItem,wireStringify:value=>JSON.stringify(value,
    (_,item)=>roundWireNumbers&&typeof item==="number"?Number(item.toPrecision(12)):item),
    $:{global:exported},app:{project:{activeItem:comp,rootFolder:{id:5},numItems:1,item:()=>comp},
    beginUndoGroup(){undo.begins++;},endUndoGroup(){undo.ends++;}}});
const readUuid=(c,prefix)=>Array.from({length:8},(_,i)=>c[prefix+i].value.toString(16).padStart(4,"0")).join("");
function graphFromEffects() {
    const graph={version:1,nodes:[],edges:[],optionalRecords:[]},positions={};
    const fixture=require("./native_snapshot_fixture.js");
    for(const effect of items.filter(effect=>effect!==renderer)) {
        const c=effect.properties,v=name=>clone(c[name].value),id=readUuid(c,"Node UUID ");
        const kind=effect.matchName.split(".").pop(),node=edits.apply({version:1,nodes:[],edges:[],optionalRecords:[]},
            {type:"addNode",nodeType:kind},()=>id).nodes[0];
        const set=(key,value)=>node.parameters.find(p=>p.key===String(key)).value=value;
        const scalar=entries=>entries.forEach(([key,name,scale=1,offset=0])=>set(key,v(name)*scale+offset));
        if(kind==="emitter") {
            scalar([[2,"Particles Per Second"],[3,"Random Seed"],[5,"Type",1,-1],[8,"Particle Size"],
                [9,"Opacity",.01],[10,"Disc Size"],[11,"Velocity Random"],
                [14,"Angle X"],[15,"Angle Y"],[16,"Angle Z"],[17,"Direction",1,-1],[18,"Direction Span"],
                [19,"Size X"],[20,"Size Y"],[21,"Size Z"],[22,"Speed Random"],[23,"Emitting",1,-1],
                [24,"Emit Chance"],[25,"Emit Life Start"],[26,"Emit Life End"],[27,"Inherit Velocity"],
                [28,"Inherit Size"],[29,"Inherit Opacity"],[30,"Inherit Color"],[31,"Auxiliary Source"]]);
            set(12,v("Speed")/2160);
            const xy=v("Origin XY");set(6,[(xy[0]-1920)/2160,(1080-xy[1])/2160,v("Origin Z")/2160]);
            set(7,[v("Velocity X"),v("Velocity Y"),v("Velocity Z")]);set(32,[v("Orient X"),v("Orient Y"),v("Orient Z")]);
        } else if(kind==="force") {
            set(1,[0,-v("Gravity")/2160,0]);set(4,[v("Wind X")/2160,-v("Wind Y")/2160,v("Wind Z")/2160]);
            set(5,v("Spin")/2160);
            scalar([[2,"Air Density"],[3,"Gravity random"],[6,"Spin Frequency"],[7,"Spin resist"],[8,"Spin Delay (Seconds)"]]);
        } else {
            if(v("Cloud Style Enabled"))scalar([[37,"Circles"],[38,"Aspect"],[39,"Density"]]);
            else node.parameters=node.parameters.filter(p=>Number(p.key)<37);
            set(1,v("Color").slice(0,3));set(18,[v("Angle X"),v("Angle Y"),v("Angle Z")]);set(20,[v("Speed X"),v("Speed Y"),v("Speed Z")]);
            scalar([[3,"Size (Pixels)"],[4,"Size Over Life"],[5,"Opacity",.01],[6,"Opacity Over Life"],
                [9,"Size Random"],[10,"Opacity Random"],[11,"Life (Seconds)"],[12,"Particle Color",1,-1],
                [14,"Life Random"],[15,"Shape",1,-1],[16,"Size Y (Pixels)"],[17,"Orient To",1,-1],
                [19,"Angle Random"],[21,"Rotation Speed Random"],[22,"Limit To 2D"],[23,"Particle Feather"],
                [24,"Up Axis",1,-1],[25,"Random Limit",1,-1],[26,"Limit Angle"],[28,"Anchor X (Percent)"],[29,"Anchor Y (Percent)"]]);
            const stops=Array.from({length:v("Color Gradient")},(_,i)=>({position:v("Color Gradient "+i+" Position")/100,color:v("Color Gradient "+i+" Color").slice(0,3)}));
            stops.interpolation=v("Color Gradient Interpolation");set(13,view.encodeGradient(stops));
        }
        for(const [key,label] of kind==="particle"?[[7,"Size"],[8,"Opacity"],[27,"Rotation"]]:kind==="force"?[[9,"Wind and Spin"]]:[]) {
            node.parameters=node.parameters.filter(p=>p.key!==String(key));
            if(v(label+" Curve Count")) {
                const points=Array.from({length:v(label+" Curve Count")},(_,i)=>({age:v(label+" Curve "+i+" Age"),value:v(label+" Curve "+i+" Value")}));
                points.interpolation=v(label+" Curve Interpolation");node.parameters.push({key:String(key),type:7,value:view.encodeCurve(points)});
            }
        }
        graph.nodes.push(node);positions[id]={x:v("Node Layout X"),y:v("Node Layout Y")};
        for(let slot=0;slot<v("Outgoing Connection Count");slot++) graph.edges.push({id:readUuid(c,"Connection "+slot+" Edge UUID "),
            sourceNode:id,sourcePort:kind==="emitter"?"1":"2",destinationNode:readUuid(c,"Connection "+slot+" Target UUID "),destinationPort:"1"});
    }
    const output=fixture.node("output",outputId),v=name=>renderer.properties[name].value;
    const values=[v("Max Particles"),v("On / Off"),v("Time (Seconds)"),v("Preview"),v("Particle chance"),
        v("Acceleration")-1,[30,60,120][v("Time Sampling")-1],...Array.from({length:8},(_,i)=>v("org.starfieldfx.particle-"+(1641+i)) - ([0,3,7].includes(i)?1:0))];
    output.parameters.forEach((p,i)=>p.value=values[i]);graph.nodes.push(output);
    positions[outputId]={x:v("Layout Output X"),y:v("Layout Output Y")};return layout.set(graph,positions);
}
function publish(graph) {
    const bytes=codec.serialize(graph),crc=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength).getUint32(24,true);
    renderer.properties["Graph Revision"].value=revision;
    renderer.properties["Graph Checksum High"].value=crc>>>16;renderer.properties["Graph Checksum Low"].value=crc&65535;
}
function compile(nonce) {
    commits++;if(rejectNext){rejectNext=false;renderer.properties["Graph Edit Receipt"].value=-nonce;return;}
    revision++;publish(graphFromEffects());renderer.properties["Graph Edit Receipt"].value=nonce;
}
function invoke(operation,fields={}) {
    return JSON.parse(exported["SFLD_"+operation](JSON.stringify(Object.assign({protocol:"org.starfieldfx.panel",version:1,
        requestId:operation,operation,target:{token:"p5-c17-l29"},pinTarget:false},fields))));
}
let next=10;
const snapshots=require(path.join(panelRoot,"js/native_graph_snapshot.js"));
assert.equal(invoke("getGraphSnapshot").snapshot.initialized,false);
const boot=snapshots.normalize(invoke("syncGraphSnapshot"));
assert.equal(boot.ok,true,JSON.stringify(boot));assert.equal(boot.snapshot.initialized,true);
assert.equal(boot.snapshot.checksumMatches,true,"ordinary record codec must match the compiled payload CRC");
const initial=codec.fromHex(boot.snapshot.graphHex);
renderer.properties["Control Source"].value=2;
const readsBeforePulse=nodePropertyReads,commitsBeforePulse=commits;
const pulse=invoke("getPanelPulse");
assert.equal(pulse.ok,true,JSON.stringify(pulse));
assert.equal(nodePropertyReads,readsBeforePulse,"lightweight pulse does not read sibling node values");
assert.equal(commits,commitsBeforePulse,"background pulse never compiles or mutates a graph");
const combined=invoke("getPanelState");
assert.equal(combined.ok,true,JSON.stringify(combined));
assert.equal(combined.snapshot.nativeNodes.length,2,"one combined request returns the current native graph");
assert.equal(commits,commitsBeforePulse,"combined full inspection is read-only");
hideDirectCurveIds=true;
numericNodeVisits=0;
const fallback=invoke("getGraphSnapshot");
assert.equal(fallback.ok,true,JSON.stringify(fallback));
const particleRaw=items.find(e=>e.matchName==="org.starfieldfx.node.particle");
assert.equal(numericNodeVisits,new Set(Object.values(particleRaw.properties)).size,
    "all unresolved curve fields share one bounded property traversal per effect/request");
hideDirectCurveIds=false;
const emitterId=initial.nodes.find(n=>n.type===edits.types.emitter).id,particleId=initial.nodes.find(n=>n.type===edits.types.particle).id;
const presets=require(path.join(panelRoot,"js/presets.js"));
const transactionEdits=Object.assign({},edits,{apply:(graph,edit,nextId)=>edit.type==="applyPreset"?
    presets.apply(graph,edit,nextId):edits.apply(graph,edit,nextId)});
const client=transactions.create({codec,edits:transactionEdits,idFactory:()=>uuid(next++),call:(op,fields,cb)=>{
    if(op==="submitGraph"&&beforeSubmit){const change=beforeSubmit;beforeSubmit=null;change();}
    cb(invoke(op,fields));
}});
const snapshot=()=>snapshots.normalize(invoke("getGraphSnapshot")).snapshot;
function wireGraphHex(graph) {
    // JSON transports -0 as 0; CRC remains diagnostic, while all graph fields
    // must still match exactly after this one transport normalization.
    for(const node of graph.nodes)for(const p of node.parameters) {
        if(typeof p.value==="number" && p.value===0)p.value=0;
        if(Array.isArray(p.value))p.value=p.value.map(v=>v===0?0:v);
    }
    return codec.toHex(graph);
}
function apply(edit){let result;client.apply(edit,r=>result=r,"p5-c17-l29");return result;}
let ensured;client.ensureNativeEffects(snapshot(),"p5-c17-l29",r=>ensured=r);
assert.equal(ensured.ok,true,JSON.stringify(ensured));assert.equal(items.length,3);
assert.equal(renderer.properties["Node Effects Ready"].value,1);assert.deepEqual(Array.from(items[1].properties["Origin XY"].value),[1920,1080]);
assert.equal(renderer.properties["Panel Graph Sync Guard"].value,0);
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
const preparedNative=invoke("getGraphSnapshot");
let stalePrepared;
client.apply({type:"addNode",nodeType:"force",position:{x:0,y:0}},r=>stalePrepared=r,
    "p5-c17-l29",preparedNative.snapshot.revision,preparedNative);
assert.equal(stalePrepared.error.code,"stale_graph","host rechecks records even when planning reused a receipt");
assert.equal(items.length,staleCount,"a real intervening edit rejects before effect creation");
roundWireNumbers=false;publish(graphFromEffects());
const duplicate=apply({type:"duplicateNodes",nodeIds:[particleId],offset:{x:160,y:0}});
assert.equal(duplicate.ok,true,JSON.stringify(duplicate));assert.equal(items.length,4);
const copiedId=codec.fromHex(snapshot().graphHex).nodes.find(n=>n.type===edits.types.particle&&n.id!==particleId).id;
assert.equal(apply({type:"setParameters",changes:[{nodeId:copiedId,parameterKey:"3",valueType:4,value:64}]}).ok,true);
assert.equal(items[2].properties["Size (Pixels)"].value,10);assert.equal(items[3].properties["Size (Pixels)"].value,64);
assert.equal(apply({type:"moveNodes",positions:{[outputId]:{x:-300,y:-200}}}).ok,true);
assert.equal(renderer.properties["Layout Output X"].value,-300);assert.equal(renderer.properties["Layout Output Y"].value,-200);
assert.equal(apply({type:"setParameters",changes:[{nodeId:outputId,parameterKey:"1",valueType:3,value:8000}]}).ok,true);
assert.equal(renderer.properties["Max Particles"].value,8000);
rejectNext=true;assert.equal(apply({type:"deleteNodes",nodeIds:[copiedId]}).ok,false);
assert.equal(items.length,4,"rejected edit restores the removed effect");
assert.equal(codec.fromHex(snapshot().graphHex).nodes.length,4,"rollback also recompiles the render snapshot");
assert.equal(renderer.properties["Panel Graph Sync Guard"].value,0);
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
const extraEmitters=[];
for(let i=0;i<2;i++) {
    const previousIds=codec.fromHex(snapshot().graphHex).nodes.map(n=>n.id);
    assert.equal(apply({type:"addNode",nodeType:"emitter",position:{x:600,y:i*100}}).ok,true);
    extraEmitters.push(codec.fromHex(snapshot().graphHex).nodes.find(n=>!previousIds.includes(n.id)).id);
}
const multiEmitterReads=nodePropertyReads,multiEmitterCommits=commits;
assert.equal(invoke("getPanelPulse").ok,true);
assert.equal(nodePropertyReads,multiEmitterReads,"multiple emitters add no sibling value reads to the pulse");
assert.equal(commits,multiEmitterCommits,"multiple-emitter idle inspection does not recompile");
assert.equal(apply({type:"deleteNodes",nodeIds:extraEmitters}).ok,true);
// AE reordering must not change graph identity, topology or independently saved values.
const beforeReorder=snapshot().graphHex;
items.reverse();epoch++;
assert.equal(snapshot().graphHex,beforeReorder);
const original=items.find(e=>e.matchName==="org.starfieldfx.node.particle"), copiedProperties=new Map();
const nativeCopy={matchName:original.matchName,name:original.name,properties:{}};
for(const [key,p] of Object.entries(original.properties)) {
    if(!copiedProperties.has(p))copiedProperties.set(p,{...p,value:clone(p.value)});
    nativeCopy.properties[key]=copiedProperties.get(p);
}
items.push(nativeCopy);epoch++;
assert.equal(snapshot().initialized,false,"AE-level Ctrl+D needs new node and edge identities");
assert.equal(invoke("syncGraphSnapshot").ok,true);
assert.equal(snapshot().graphHex,wireGraphHex(graphFromEffects()));
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
renderer.properties["Node Effects Ready"].value=0;renderer.properties["Graph Revision"].value=0;revision=0;
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
assert.equal(renderer.properties["Panel Graph Sync Guard"].value,0);
rejectNextAdd=null;
const cloudId=codec.fromHex(snapshot().graphHex).nodes.find(n=>n.type===edits.types.particle).id;
const cloudChanges=[[15,2,3],[37,34,3],[38,230,4],[39,1000,4]].map(([key,value,valueType])=>
    ({nodeId:cloudId,parameterKey:String(key),valueType,value}));
assert.equal(apply({type:"setParameters",changes:cloudChanges}).ok,true,"Cloud values commit through the complete indexed-effect transaction");
const cloudSource=codec.fromHex(snapshot().graphHex),cloudBeforeFailure=snapshot().graphHex;
const cloudValues=g=>g.nodes.filter(n=>n.type===edits.types.particle).map(n=>[37,38,39].map(k=>n.parameters.find(p=>p.key===String(k))?.value));
assert.deepEqual(cloudValues(cloudSource),[[34,230,1000]]);
rejectNext=true;
assert.equal(apply({type:"setParameters",changes:[{nodeId:cloudId,parameterKey:"39",valueType:4,value:0}]}).ok,false);
assert.equal(snapshot().graphHex,cloudBeforeFailure,"failed Cloud graph commit restores exact values and activation");
assert.deepEqual(cloudValues(codec.fromHex(snapshot().graphHex)),[[34,230,1000]]);
for(const mode of ["add","replace"]) {
    const result=apply({type:"applyPreset",presetGraph:cloudSource,mode,applyRenderSettings:true});
    assert.equal(result.ok,true,"Cloud preset "+mode+" commits with current native node schema: "+JSON.stringify(result));
    const applied=codec.fromHex(snapshot().graphHex);
    assert.equal(applied.nodes.filter(n=>n.type===edits.types.output).length,1);
    assert.ok(cloudValues(applied).every(v=>v[0]===34 && v[1]===230 && v[2]===1000));
    assert.equal(cloudValues(applied).length,mode==="add"?2:1);
}
assert.equal(undo.begins,undo.ends);assert.ok(commits>=8);
console.log("Native node gateway checks passed: bootstrap, add, copy, native Ctrl+D, independent values/curves, signed layout, insert/connect/disconnect, AE reorder/deletion, Output, numeric receipts without expressions, rollback and delete all.");
