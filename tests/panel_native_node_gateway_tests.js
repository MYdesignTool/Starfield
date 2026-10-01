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
let epoch = 0, rejectNext = false, revision = 1, commits = 0;
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
    [40,"Layout Output Y",526],[41,"Graph Snapshot",0],[42,"Panel Graph Sync Guard",0],
    [43,"Commit Graph Edit",0],[44,"Graph Edit Receipt",0],[89,"Node Effects Ready",0]]) {
    const p=control(name,value); p.propertyIndex=index; renderer.properties[index]=renderer.properties[name]=p;
}
const items=[renderer];
function assertFresh(captured) { if(captured!==epoch) throw new Error("Invalid indexed-group reference"); }
function wrapProperty(p,captured) {
    return {name:p.name,propertyIndex:p.propertyIndex,canSetExpression:false,
        get value(){assertFresh(captured);return clone(p.value);},
        get expression(){assertFresh(captured);return p.expression||"";},
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
        addProperty(matchName){items.push({matchName,name:"",properties:nodeControls()});epoch++;return wrapEffect(items[items.length-1]);} };
}
const layer={id:29,name:"Particle Layer",selected:true,width:3840,height:2160,source:{pixelAspect:1},
    property(name){return name==="ADBE Effect Parade"?parade():null;} };
function CompItem() {}
const comp=new CompItem();Object.assign(comp,{id:17,name:"Test Comp",numLayers:1,layer:()=>layer});
const exported={};
vm.runInNewContext(source,{CompItem,$:{global:exported},app:{project:{activeItem:comp,rootFolder:{id:5}},
    beginUndoGroup(){undo.begins++;},endUndoGroup(){undo.ends++;}}});
const readUuid=(c,prefix)=>Array.from({length:8},(_,i)=>c[prefix+i].value.toString(16).padStart(4,"0")).join("");
function graphFromEffects() {
    const graph={version:1,nodes:[],edges:[],optionalRecords:[]},positions={},p=(key,type,value)=>({key:String(key),type,value});
    for(const effect of items.slice(1)) {
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
    const bytes=codec.serialize(graph),hex=codec.toHex(graph),crc=codec.crc32(bytes).toString(16).padStart(8,"0");
    renderer.properties[41].expression="/*SFLDSNAP1:"+revision+":"+bytes.length+":"+crc+":"+hex+"*/0";
}
function compile(nonce) {
    commits++;if(rejectNext){rejectNext=false;renderer.properties[44].value=-nonce;return;}
    revision++;publish(graphFromEffects());renderer.properties[44].value=nonce;
}
function invoke(operation,fields={}) {
    return JSON.parse(exported["SFLD_"+operation](JSON.stringify(Object.assign({protocol:"org.starfieldfx.panel",version:1,
        requestId:operation,operation,target:{token:"p5-c17-l29"},pinTarget:false},fields))));
}
let initial=graphFromEffects(),next=10;
initial=edits.apply(initial,{type:"addNode",nodeType:"emitter",position:{x:-160,y:-80}},()=>uuid(next++));
initial=edits.apply(initial,{type:"addNode",nodeType:"particle",position:{x:200,y:120}},()=>uuid(next++));
const emitterId=initial.nodes.find(n=>n.type===edits.types.emitter).id,particleId=initial.nodes.find(n=>n.type===edits.types.particle).id;
initial=edits.apply(initial,{type:"connect",from:emitterId,to:particleId},()=>uuid(next++));
initial=edits.apply(initial,{type:"connect",from:particleId,to:outputId},()=>uuid(next++));publish(initial);
const client=transactions.create({codec,edits,idFactory:()=>uuid(next++),call:(op,fields,cb)=>cb(invoke(op,fields))});
const snapshot=()=>invoke("getGraphSnapshot").snapshot;
function apply(edit){let result;client.apply(edit,r=>result=r,"p5-c17-l29");return result;}
let ensured;client.ensureNativeEffects(snapshot(),"p5-c17-l29",r=>ensured=r);
assert.equal(ensured.ok,true,JSON.stringify(ensured));assert.equal(items.length,3);
assert.equal(renderer.properties[89].value,1);assert.deepEqual(Array.from(items[1].properties.Origin.value),[1920,1080,1080]);
assert.equal(renderer.properties[42].value,0);
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
const all=codec.fromHex(snapshot().graphHex).nodes.filter(n=>n.type!==edits.types.output).map(n=>n.id);
assert.equal(apply({type:"deleteNodes",nodeIds:all}).ok,true,"all editable nodes can be removed");
assert.equal(items.length,1);assert.equal(codec.fromHex(snapshot().graphHex).nodes.length,1);
assert.equal(undo.begins,undo.ends);assert.ok(commits>=8);
console.log("Native node gateway checks passed: bootstrap, copy, independent values, signed layout, Output controls, numeric commit without expressions, stale references, rollback and delete all.");
