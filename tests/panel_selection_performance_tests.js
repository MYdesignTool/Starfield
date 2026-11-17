"use strict";
const fs=require("node:fs"),path=require("node:path");
let prelude=fs.readFileSync(path.join(__dirname,"panel_node_warning_tests.js"),"utf8").split("const q=win.reviewProbe,callbacks=[];")[0];
prelude=prelude.replace('window.reviewProbe={state:state,','window.reviewProbe={refresh:refresh,state:state,');
const cases=String.raw`
const q=win.reviewProbe,original=host.SFLD_getPanelState;
let chosen="target",light=0,status=0,full=0,diagnostics=0,inFlight=0,maxInFlight=0;
const transport=win.__adobe_cep__.evalScript;
win.__adobe_cep__.evalScript=function(script,callback){
    maxInFlight=Math.max(maxInFlight,++inFlight);
    transport(script,function(result){inFlight--;callback(result);});
};
host.SFLD_getPanelState=text=>{full++;const result=JSON.parse(original(text));result.target.token=chosen;result.snapshot.panelStamp="steady-"+chosen;return JSON.stringify(result);};
host.SFLD_getPanelPulse=text=>{
    const req=JSON.parse(text);
    if(req.selectionOnly){light++;return response({ok:true,selectionOnly:true,target:chosen?{token:chosen}:null,projectScope:"",selectionStamp:JSON.stringify(["",chosen])});}
    status++;return response({ok:true,target:{token:chosen},projectScope:"",stamp:"steady-"+chosen,frameStatus:{targetToken:chosen,available:false}});
};
win.StarfieldNativeGraph={diagnostics(target,rev,crc,scope,done){diagnostics++;done({ok:true,revision:rev,checksum:parseInt(crc,16),issues:[]});}};
function fireNamed(name){const entry=Array.from(timers.entries()).find(([,timer])=>timer.callback.name===name);assert.ok(entry,"timer "+name);timers.delete(entry[0]);now+=entry[1].delay;entry[1].callback();}
function poll(){fireNamed("pollPanelState");}
poll();assert.equal(light,1);assert.equal(full,0);assert.equal(status,0,"fast probe does not ask for full status");
// Three quick choices collapse into a single full read of the final target.
for(const token of ["B","C","D"]){chosen=token;poll();assert.equal(full,0);assert.equal(diagnostics,0);}
poll();assert.equal(full,1);assert.equal(q.state.targetToken,"D");assert.equal(diagnostics,0,"native diagnostics are deferred until quiet");
chosen="E";poll();assert.equal(full,1,"first observation does not load a transient graph");
assert.ok(!Array.from(timers.values()).some(t=>t.callback.name==="attempt"),"a new selection cancels old deferred diagnostics");
chosen="D";poll();assert.equal(full,1,"returning to the current canvas needs no full read");
// Focus notifications schedule one probe; they do not synchronously read graphs.
win.fire("focus");win.fire("focus");assert.equal(full,1);
assert.equal(Array.from(timers.values()).filter(t=>t.callback.name==="pollPanelState").length,1);
poll();assert.equal(full,1);
// A slow full read never causes a second host call or a 2s discovery backoff.
chosen="E";poll();holdInspection=true;poll();assert.equal(full,2);assert.equal(inFlight,1);
const before=light;poll();assert.equal(light,before);assert.ok(Array.from(timers.values()).some(t=>t.callback.name==="pollPanelState" && t.delay===200));
holdInspection=false;releaseInspection();assert.equal(q.state.targetToken,"E");assert.equal(inFlight,0);
fireNamed("attempt");assert.equal(diagnostics,1,"only the settled canvas requests diagnostics");
assert.equal(maxInFlight,1,"selection/status/full reads never overlap");
elements.autoRefresh.checked=false;const paused=light;poll();assert.equal(light,paused);
elements.autoRefresh.checked=true;doc.hidden=true;poll();assert.equal(light,paused);doc.hidden=false;
// Once settled, full status is sampled at the slow cadence, not each probe.
now+=2100;poll();assert.equal(status,1);const priorStatus=status;poll();assert.equal(status,priorStatus);
chosen=null;poll();poll();assert.equal(q.state.targetToken,null);assert.equal(diagnostics,1);assert.equal(full,2,"empty selection needs no full graph read");
console.log(JSON.stringify({checks:"selection coalescing/focus/slow read/quiet diagnostics/pause/empty/status",light,status,full,diagnostics,maxInFlight}));
`;
new Function("require","__dirname",prelude+cases)(require,__dirname);
