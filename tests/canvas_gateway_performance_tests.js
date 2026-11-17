"use strict";
const fs=require("node:fs"),path=require("node:path");
const prelude=fs.readFileSync(path.join(__dirname,"panel_native_node_gateway_tests.js"),"utf8").split("const readUuid=")[0];
const cases=String.raw`
renderer.properties["Control Source"].value=2;renderer.properties["Graph Revision"].value=7;renderer.properties["Node Effects Ready"].value=1;
for(let n=1;n<=8;n++){
    const props=nodeControls();const id=uuid(n);
    for(let i=0;i<8;i++)props["Node UUID "+i].value=parseInt(id.slice(i*4,i*4+4),16);
    props["Node Layout X"].value=n*120;props["Node Layout Y"].value=40;
    for(const label of ["Size","Opacity","Rotation"]){
        props[label+" Curve Count"].value=64;
        for(let i=0;i<64;i++)props[label+" Curve "+i+" Age"].value=i/63;
        // This older generic fixture packs extended points after slot8. Mirror
        // the current registered disk IDs for the full 64-point comparison.
        const base={Size:[3000,3100],Opacity:[3200,3300],Rotation:[3400,3500]}[label];
        for(let i=8;i<64;i++)for(let axis=0;axis<2;axis++){
            const name=label+" Curve "+i+(axis?" Value":" Age"),id=base[axis]+i;
            const p={name,matchName:"org.starfieldfx.node.particle-"+String(id).padStart(4,"0"),value:axis?(label==="Rotation"?0:100):i/63};
            props[name]=props[p.matchName]=props["org.starfieldfx.node.particle-"+id]=p;
        }
    }
    items.push({name:"Particle "+n,matchName:"org.starfieldfx.node.particle",properties:props});
}
function read(canvasOnly){return JSON.parse(exported.SFLD_getPanelState(JSON.stringify({protocol:"org.starfieldfx.panel",version:1,operation:"getPanelState",canvasOnly})));}
nodePropertyReads=0;const thin=read(true),thinReads=nodePropertyReads;
assert.equal(thin.ok,true,JSON.stringify(thin.error));assert.equal(thin.snapshot.canvasOnly,true);assert.equal(thin.snapshot.nativeNodes.length,8);
assert.ok(thin.snapshot.nativeNodes.every(n=>n.parameters.length===3));assert.equal(thin.snapshot.layerResources,undefined);assert.equal(thin.snapshot.recordStamp,undefined);
nodePropertyReads=0;const full=read(false),fullReads=nodePropertyReads;
assert.equal(full.ok,true,JSON.stringify(full.error));assert.ok(fullReads>thinReads*10,"curves are not read merely to display a canvas");
const snapshots=require(path.join(panelRoot,"js/native_graph_snapshot.js"));
const normalized=snapshots.normalize(thin);assert.equal(normalized.snapshot.graphHex,undefined,"thin metadata is not serialized into a parameter-default graph");
const display=snapshots.canvasGraph(thin.snapshot);assert.equal(display.graph.nodes.length,9);
assert.throws(()=>codec.toHex(display.graph),e=>e.code==="canvas_snapshot_read_only");
assert.throws(()=>edits.apply(display.graph,{type:"deleteNodes",nodeIds:[uuid(1)]}),e=>e.code==="canvas_snapshot_read_only");
assert.equal(layout.set(display.graph,display.positions).canvasOnly,true,"layout helpers preserve the read-only marker");
const queue=require(path.join(panelRoot,"js/edit_queue.js")).create({codec,edits,schedule(){},isCurrent(){return true;}});
const blocked=queue.enqueue({type:"deleteNodes",nodeIds:[uuid(1)]},thin,1);assert.equal(blocked.ok,false);assert.equal(blocked.error.code,"graph_snapshot_unconfirmed");
const bad=JSON.parse(JSON.stringify(thin.snapshot));bad.nativeNodes[0].outgoing.push({id:uuid(99),target:uuid(1234)});
assert.throws(()=>snapshots.canvasGraph(bad),e=>e.code==="invalid_canvas_snapshot");
assert.equal(propertyWrites,0);assert.equal(undo.begins,0);assert.equal(commits,0);
console.log(JSON.stringify({nodes:8,thinReads,fullReads,reduction:(fullReads/thinReads).toFixed(1),writes:propertyWrites,checks:"actual JSX curves/read-only codec/layout/edit guards"}));
`;
new Function("require","__dirname",prelude+cases)(require,__dirname);
