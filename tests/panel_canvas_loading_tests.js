"use strict";
const fs=require("node:fs"),path=require("node:path");
let prelude=fs.readFileSync(path.join(__dirname,"panel_node_warning_tests.js"),"utf8").split("const q=win.reviewProbe,callbacks=[];")[0];
prelude=prelude.replace('window.reviewProbe={state:state,','window.reviewProbe={edit:requestTopologyEdit,refresh:refresh,move:function(id){nodePositions[id].x+=25;commitNodeLayout();},position:function(id){return nodePositions[id];},state:state,');
const setup=String.raw`
const q=win.reviewProbe,original=host.SFLD_getPanelState;
const positions=layout.get(graph),oldOutput=graph.nodes.find(n=>n.type===edits.types.output),oldOutputId=oldOutput.id;
positions[uuid(255)]=positions[oldOutputId];delete positions[oldOutputId];oldOutput.id=uuid(255);
graph.edges.forEach(e=>{if(e.destinationNode===oldOutputId)e.destinationNode=uuid(255);});graph=layout.set(graph,positions);
let fullReads=0,layoutWrites=0,patches=0,releaseFull=null;
host.SFLD_getPanelState=text=>{
    const request=JSON.parse(text);assert.equal(request.canvasOnly,true);
    const result=JSON.parse(original()),s=result.snapshot;
    s.canvasOnly=true;s.canvasStamp="canvas";s.checksum="00000001";delete s.graphHex;delete s.recordStamp;delete s.layerResources;
    for(const record of s.nativeNodes)record.parameters=record.parameters.filter(p=>(record.type===edits.types.particle?["3","5","15"]:["2","31"]).includes(p.key));
    result.revision="canvas-"+revision;return JSON.stringify(result);
};
host.SFLD_setGraphNodeLayout=text=>{
    layoutWrites++;const req=JSON.parse(text),positions=layout.get(graph);
    for(const c of req.positions){assert.deepEqual(c.from,positions[c.id]);positions[c.id]=c.to;}
    graph=layout.set(graph,positions);return response({ok:true,target:{token:"target"}});
};
const transport=win.__adobe_cep__.evalScript;
win.__adobe_cep__.evalScript=function(script,done){
    if(script.includes("return SFLD_getGraphSnapshot")){fullReads++;transport(script,value=>{releaseFull=()=>done(value);});}
    else transport(script,done);
};
win.StarfieldNativeGraph={patch(target,rev,crc,delta,done){patches++;assert.equal(target,"target");}};
q.refresh(false,false);assert.equal(q.state.graph.canvasOnly,true);assert.equal(fullReads,0);
function timer(delay){const entry=Array.from(timers.entries()).find(([,t])=>t.delay===delay);assert.ok(entry,"timer "+delay);timers.delete(entry[0]);entry[1].callback();}
`;
const cases=[String.raw`
q.state.pending=true;q.edit({type:"deleteNodes",nodeIds:[uuid(2)]});assert.equal(fullReads,0,"busy admission must remain closed even for a read-only canvas");q.state.pending=false;
`,String.raw`
const button=elements.chain.children.find(e=>e.getAttribute("data-node-id")===uuid(2)).querySelector(".node-select");
button.fire("click",{});assert.equal(selectedEffects.at(-1),uuid(2));assert.equal(fullReads,0,"native parameter selection needs no full author read");
q.move(uuid(2));timer(600);assert.equal(layoutWrites,1);assert.equal(fullReads,0,"pure layout persists directly from lightweight identities/positions");
q.edit({type:"disconnect",from:uuid(1),to:uuid(2),edgeId:uuid(11)});
q.edit({type:"disconnect",from:uuid(2),to:uuid(255),edgeId:uuid(12)});
assert.equal(fullReads,1,"rapid structural intentions share a single hydration");assert.equal(patches,0,"no mutation is sent from metadata");
releaseFull();assert.ok(!q.state.graph.canvasOnly);assert.ok(q.state.graph.nodes.find(n=>n.id===uuid(2)).parameters.length>10,"complete original parameters survive hydration");
timer(100);assert.equal(patches,1,"queued edits enter the existing native batch after complete snapshot");
`,String.raw`
host.SFLD_getGraphSnapshot=()=>response({ok:false,error:{code:"read_failed",message:"Unavailable"}});
q.edit({type:"deleteNodes",nodeIds:[uuid(2)]});releaseFull();
assert.equal(patches,0);assert.equal(q.state.graph.canvasOnly,true);assert.equal(submitCount,0);assert.match(elements.banner.textContent,/read_failed/);
`,String.raw`
const originalSnapshot=host.SFLD_getGraphSnapshot;
host.SFLD_getGraphSnapshot=()=>{const wrong=JSON.parse(originalSnapshot());wrong.target.token="other";return JSON.stringify(wrong);};
q.edit({type:"deleteNodes",nodeIds:[uuid(2)]});releaseFull();assert.equal(patches,0);assert.equal(q.state.graph.canvasOnly,true);
`,String.raw`
q.edit({type:"deleteNodes",nodeIds:[uuid(2)]});win.fire("pagehide");releaseFull();assert.equal(patches,0);assert.equal(submitCount,0);
`,String.raw`
q.begin();q.edit({type:"disconnect",from:uuid(1),to:uuid(2),edgeId:uuid(11)});
const before=q.position(uuid(2)).x;q.move(uuid(2));releaseFull();
assert.equal(q.state.graph.canvasOnly,true,"hydration cannot rebuild an active gesture");
q.end({type:"pointerup"});timer(50);
assert.equal(q.position(uuid(2)).x,before+25,"local move during hydration survives adoption");
`];
for(const entry of cases)new Function("require","__dirname",prelude+setup+entry)(require,__dirname);
console.log("Actual panel canvas: immediate cards, direct layout/AE selection, one hydration for multiple intents, preserved controls, failure/target/pagehide/gesture gates passed");
