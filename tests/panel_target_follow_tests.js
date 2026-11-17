"use strict";
// Reuse the actual panel fake-DOM prelude, not a second implementation of the
// panel. The test below changes only host responses and fires real UI events.
const fs=require("node:fs"),path=require("node:path");
let prelude=fs.readFileSync(path.join(__dirname,"panel_node_warning_tests.js"),"utf8").split("const q=win.reviewProbe,callbacks=[];")[0];
prelude=prelude.replace('"targetLine","modeLine","revisionLine","resolutionLine",','"targetLine","modeLine","revisionLine","resolutionLine","graphEmpty",');
prelude=prelude.replace('elements.autoRefresh.checked=true;','elements.autoRefresh.checked=false;');
prelude=prelude.replace('window.reviewProbe={state:state,','window.reviewProbe={refresh:refresh,call:call,state:state,');
const cases=String.raw`
const q=win.reviewProbe;
assert.equal(elements.autoRefresh.checked,true,"startup resets the checkbox, even if browser restored it unchecked");
const original=host.SFLD_getPanelState,requests=[];
host.SFLD_getPanelState=text=>{requests.push(JSON.parse(text));return original(text);};
elements.autoRefresh.checked=false;win.fire("focus");
assert.equal(requests.at(-1).pinTarget,true);assert.equal(requests.at(-1).target.token,"target");
assert.equal(q.state.targetToken,"target");
elements.refresh.fire("click");assert.equal(requests.at(-1).pinTarget,false,"explicit Refresh follows selection even while paused");
q.call("selectNodeEffect",{nodeId:uuid(2)},()=>{});assert.equal(selectedEffects.at(-1),uuid(2));
const selectOriginal=host.SFLD_selectNodeEffect;
host.SFLD_selectNodeEffect=text=>{assert.equal(JSON.parse(text).pinTarget,true,"edits target the canvas, not a newly selected layer");return selectOriginal(text);};
q.call("selectNodeEffect",{nodeId:uuid(2)},()=>{});
// A selection response arriving after the checkbox is unchecked must be ignored.
host.SFLD_getPanelState=()=>{const changed=JSON.parse(original());changed.target.token="other";return JSON.stringify(changed);};
elements.autoRefresh.checked=true;holdInspection=true;q.refresh(false,false);
elements.autoRefresh.checked=false;releaseInspection();holdInspection=false;
assert.equal(q.state.targetToken,"target","late selection response cannot switch after the checkbox is unchecked");
const oldScope=q.state.graphSnapshot.projectScope || "";
host.SFLD_getPanelState=()=>{const changed=JSON.parse(original());changed.snapshot.projectScope="D:/late-project.aep";return JSON.stringify(changed);};
elements.autoRefresh.checked=true;holdInspection=true;q.refresh(false,false);
elements.autoRefresh.checked=false;releaseInspection();holdInspection=false;
assert.equal(q.state.graphSnapshot.projectScope || "",oldScope,"a late different project with reused AE IDs is also a paused switch");
const saved=q.state.graphSnapshot;
host.SFLD_getPanelState=()=>response({ok:false,error:{code:"no_target",message:"No selected layer"}});
elements.autoRefresh.checked=true;q.refresh(false,false);
assert.equal(q.state.targetToken,null);assert.equal(q.state.graph,null);assert.equal(q.state.graphSnapshot,null);
assert.equal(q.state.nodes.length,0);assert.ok(!elements.banner.classList.contains("error"));
assert.equal(elements.graphEmpty.hidden,false);assert.equal(elements.graphMinimap.hidden,true);
assert.ok(!Array.from(timers.values()).some(t=>[250,750,1500,3000,5000].includes(t.delay)),"ordinary absence never starts connection-retry banners");
// Paused + no previous view never adopts a selection on focus.
elements.autoRefresh.checked=false;let inspected=0;host.SFLD_getPanelState=()=>{inspected++;return original();};
win.fire("focus");assert.equal(inspected,0);assert.equal(q.state.targetToken,null);
elements.autoRefresh.checked=true;elements.autoRefresh.fire("change");
assert.equal(q.state.targetToken,"target");assert.equal(elements.graphEmpty.hidden,true);assert.equal(elements.graphMinimap.hidden,false);
const previousEpoch=q.state.additionEpoch;
host.SFLD_getPanelState=()=>{const changed=JSON.parse(original());changed.snapshot.projectScope="D:/different.aep";return JSON.stringify(changed);};
q.refresh(false,false);assert.ok(q.state.additionEpoch>previousEpoch,"project scope changes invalidate the old view even when AE target IDs are reused");
// Real failures remain visible rather than being swallowed as an empty state.
host.SFLD_getPanelState=()=>response({ok:false,error:{code:"missing_parameter",message:"A real failure"}});
q.refresh(false,false);assert.ok(elements.banner.classList.contains("error"));
const html=fs.readFileSync(path.join(panelRoot,"index.html"),"utf8"),css=fs.readFileSync(path.join(panelRoot,"css/panel.css"),"utf8");
assert.ok(html.indexOf('id="graphEmpty"')>html.indexOf('id="selectionBox"'),"empty hint is outside the scaled graph canvas");
assert.match(css,/\.graph-empty\s*\{[^}]*inset:\s*0;[^}]*align-items:\s*center;[^}]*justify-content:\s*center;/);
assert.match(css,/\.graph-empty\s*\{[^}]*font-size:\s*22px/);
console.log("Actual panel target follow: default reset, pause/manual/focus, absence, centered hint and explicit target checks passed");
`;
new Function("require","__dirname",prelude+cases)(require,__dirname);
