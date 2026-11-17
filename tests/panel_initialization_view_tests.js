"use strict";
// Execute the real panel and DOM renderer with a delayed first initialization.
const fs=require("node:fs"),path=require("node:path");
let prelude=fs.readFileSync(path.join(__dirname,"panel_node_warning_tests.js"),"utf8").split("const q=win.reviewProbe,callbacks=[];")[0];
prelude=prelude.replace('window.reviewProbe={state:state,','window.reviewProbe={refresh:refresh,state:state,');
const setup=String.raw`
const q=win.reviewProbe,readyState=host.SFLD_getPanelState;
const legacyNodes=["emitter","particle","force","output"].map(id=>({id,label:id,params:[]}));
const legacyEdges=legacyNodes.slice(1).map((node,i)=>({from:legacyNodes[i].id,to:node.id}));
const legacy=()=>response({ok:true,target:{token:"new-effect",comp:"Comp",layer:"New layer"},
    controlSource:"AE Controls",revision:"legacy-before-sync",resolution:"name",layoutPersistence:true,
    nodes:legacyNodes,edges:legacyEdges,layout:{},snapshot:{initialized:false},
    frameStatus:{ok:true,targetToken:"new-effect",available:true,liveParticleCount:999}});
host.SFLD_getPanelState=legacy;
let syncs=0,releaseSync;
host.SFLD_syncGraphSnapshot=text=>{
    const request=JSON.parse(text);assert.equal(request.target.token,"new-effect");syncs++;
    return response({...receipt(),operation:"syncGraphSnapshot",target:{token:"new-effect"}});
};
const transport=win.__adobe_cep__.evalScript;
win.__adobe_cep__.evalScript=(script,done)=>{
    if(script.includes("return SFLD_syncGraphSnapshot"))transport(script,value=>{releaseSync=()=>done(value);});
    else transport(script,done);
};
const displayed=[];const append=elements.chain.appendChild.bind(elements.chain);
elements.chain.appendChild=child=>{displayed.push(child.getAttribute("data-node-id"));append(child);};
q.refresh(false,false);
assert.equal(syncs,1);assert.equal(q.state.targetToken,"target","wait for confirmation before changing the view");
assert.ok(!displayed.includes("force"));
`;
const cases=[String.raw`
releaseSync();
assert.equal(q.state.targetToken,"new-effect");
assert.equal(q.state.controlSource,"Node Graph","the first confirmed paint uses the post-initialization mode");
assert.equal(q.state.topologyReady,true);assert.equal(q.state.graphMode,true);
assert.equal(q.state.nodes.length,3);assert.equal(q.state.edges.length,2);
assert.deepEqual(displayed,[uuid(1),uuid(2),uuid(3)],"paint only the confirmed Emitter/Particle/Output, with no legacy Force frame");
assert.equal(elements.modeLine.textContent,"Mode: Node Graph");
assert.equal(elements.revisionLine.textContent,"Revision: graph-1-records-1 / Graph 1");
assert.match(elements.resolutionLine.textContent,/Lookup: native.*Layout: AE graph/);
assert.notEqual(q.state.liveParticleCount,999,"pre-initialization frame data cannot be applied to the new graph");
assert.equal(calls.filter(c=>c==="getGraphSnapshot").length,0,"adopt the confirmed receipt without an extra full read");
const firstPositions=q.state.nodes.map(n=>[n.id,n.position.x,n.position.y]);
assert.deepEqual(firstPositions,[[uuid(1),-250,-150],[uuid(2),-250,40],[uuid(3),-250,240]]);
host.SFLD_getPanelState=()=>{const r=JSON.parse(readyState());r.target.token="new-effect";return JSON.stringify(r);};
q.refresh(false,false);assert.equal(syncs,1);
assert.deepEqual(q.state.nodes.map(n=>[n.id,n.position.x,n.position.y]),firstPositions,"the next read does not replace topology or move nodes");
assert.ok(!displayed.some(id=>legacyNodes.some(n=>n.id===id)));
`,String.raw`
elements.autoRefresh.checked=false;releaseSync();
assert.equal(q.state.targetToken,"target","pausing during initialization still blocks a late target switch");
assert.equal(displayed.length,0);
`,String.raw`
// An already initialized graph in deliberate AE Controls mode is not a new bootstrap.
releaseSync();displayed.length=0;
host.SFLD_getPanelState=()=>{const r=JSON.parse(legacy());r.snapshot=receipt().snapshot;return JSON.stringify(r);};
q.refresh(false,false);assert.equal(syncs,1);assert.equal(q.state.controlSource,"AE Controls");
assert.deepEqual(q.state.nodes.map(n=>n.id),legacyNodes.map(n=>n.id));
`];
for(const entry of cases)new Function("require","__dirname",prelude+setup+entry)(require,__dirname);
console.log("First initialization: canonical first paint, no legacy Force/positions/status, no extra read, stable refresh and paused/legacy modes passed");
