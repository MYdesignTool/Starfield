"use strict";
// Actual panel + palette + graph edit/transaction modules, no AE process.
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const panelRoot=process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"..","web_panel");
const codec=require(path.join(panelRoot,"js","graph_codec.js")),layout=require(path.join(panelRoot,"js","graph_layout.js"));
const edits=require(path.join(panelRoot,"js","graph_edits.js"));
const fixture=require("./native_snapshot_fixture.js");
const uuid=n=>n.toString(16).padStart(32,"0");
let graph=layout.set({version:1,nodes:[fixture.node("emitter",uuid(1)),fixture.node("particle",uuid(2)),fixture.node("output",uuid(3))],
    edges:[{id:uuid(11),sourceNode:uuid(1),sourcePort:"1",destinationNode:uuid(2),destinationPort:"1"},
           {id:uuid(12),sourceNode:uuid(2),sourcePort:"2",destinationNode:uuid(3),destinationPort:"1"}],optionalRecords:[]},
    {[uuid(1)]:{x:-250,y:-150},[uuid(2)]:{x:-250,y:40},[uuid(3)]:{x:-250,y:240}});
class Element {
    constructor(id,tag="div"){this.id=id;this.tagName=tag.toUpperCase();this.children=[];this.listeners={};this.attributes={};this.style={};this.hidden=false;
        this.clientWidth=860;this.clientHeight=500;this.offsetWidth=110;this.offsetHeight=54;this._class="";this.classes=new Set();
        this.classList={add:x=>this.classes.add(x),remove:x=>this.classes.delete(x),contains:x=>this.classes.has(x),
            toggle:(x,on)=>{if(on===undefined)on=!this.classes.has(x);on?this.classes.add(x):this.classes.delete(x);}};}
    get className(){return Array.from(this.classes).join(" ");}set className(value){this.classes=new Set(value.split(/\s+/).filter(Boolean));}
    set innerHTML(value){this.html=value;this.children=[];}get innerHTML(){return this.html||"";}
    set textContent(value){this.text=value;this.children=[];}get textContent(){return this.text||"";}
    addEventListener(type,callback){(this.listeners[type]||(this.listeners[type]=[])).push(callback);}
    fire(type,event={}){event.target=event.target||this;event.currentTarget=this;event.preventDefault=()=>{};event.stopPropagation=()=>{};
        for(const fn of this.listeners[type]||[])fn(event);}
    setAttribute(key,value){if(key==="class")this.className=String(value);else this.attributes[key]=String(value);}
    getAttribute(key){return key==="class"?this.className:this.attributes[key];}
    appendChild(child){this.children.push(child);child.parentNode=this;}removeChild(child){this.children=this.children.filter(x=>x!==child);child.parentNode=null;}
    contains(child){return child===this||this.children.some(x=>x.contains(child));}
    matches(selector){if(selector[0]===".")return this.classes.has(selector.slice(1));const match=/^\[([^=\]]+)(?:="([^"]+)")?\]$/.exec(selector);
        return !!(match&&this.attributes[match[1]]!==undefined&&(match[2]===undefined||this.attributes[match[1]]===match[2]));}
    querySelectorAll(selector){const result=[];for(const child of this.children){if(child.matches(selector))result.push(child);result.push(...child.querySelectorAll(selector));}return result;}
    querySelector(selector){return this.querySelectorAll(selector)[0]||null;}
    getBoundingClientRect(){return {left:36,top:150,right:896,bottom:650,width:860,height:500};}
    getContext(){return null;}focus(){}setPointerCapture(){}releasePointerCapture(){}
}
const ids=["banner","chain","workspace","graphScroll","graphViewport","graphMinimap","graphEdges","edgePaths","selectionBox","zoomReadout",
    "targetLine","modeLine","revisionLine","resolutionLine",
    "refresh","autoRefresh","targetLock","graphContextMenu","disconnectContextAction","nodePalette","nodePaletteItems","nodePaletteToggle"];
const elements=Object.fromEntries(ids.map(id=>[id,new Element(id)])),win=new Element("window"),doc=new Element("document"),canvas=new Element("canvas");
doc.body=new Element("body");doc.hidden=false;doc.getElementById=id=>elements[id];doc.querySelector=()=>canvas;doc.createElement=tag=>new Element("created",tag);
doc.createElementNS=(namespace,tag)=>doc.createElement(tag);doc.elementFromPoint=()=>canvas;
elements.graphScroll.appendChild(elements.graphViewport);elements.graphViewport.appendChild(canvas);canvas.appendChild(elements.graphEdges);
canvas.appendChild(elements.chain);elements.graphEdges.appendChild(elements.edgePaths);elements.graphViewport.appendChild(elements.graphMinimap);
elements.workspace.appendChild(elements.graphScroll);
elements.graphContextMenu.hidden=true;elements.autoRefresh.checked=true;
const entries={};for(const kind of ["emitter","auxiliary","particle","force"]){const el=new Element(kind,"button");el.setAttribute("data-node-type",kind);elements.nodePaletteItems.appendChild(el);entries[kind]=el;}
let revision=1,pulseCalls=0,submitCount=0,holdSubmit=false,releaseSubmit=null,denyNext=false,now=1000;
let holdInspection=false,releaseInspection=null;
let holdLayoutReply=false,releaseLayoutReply=null;
const calls=[],timers=new Map();let timerId=0;
const selectedEffects=[];let holdSelection=false,releaseSelection=null;
const probe={};vm.runInNewContext(fs.readFileSync(path.join(panelRoot,"jsx","starfield_gateway.jsx"),"utf8"),{$:{global:probe}});
const ready=probe.SFLD_ready(),generation=ready.split("/").pop();
function response(payload){return JSON.stringify({protocol:"org.starfieldfx.panel",version:1,gatewayBuild:generation,...payload});}
function receipt(){return fixture.receipt({ok:true,target:{token:"target"},snapshot:{initialized:true,revision,recordStamp:"records-"+revision,
    graphHex:codec.toHex(graph),geometry:{width:1920,height:1080,pixelAspect:1}}});}
const host={SFLD_ready:()=>ready,File:function(value){this.path=value;},$:{evalFile(){host.SFLD_ready=()=>ready;}}};
host.SFLD_getPanelState=()=>{calls.push("getPanelState");return response({ok:true,target:{token:"target",comp:"Comp",layer:"Particles"},
    revision:"r"+revision,controlSource:"Node Graph",resolution:"name",snapshot:receipt().snapshot});};
host.SFLD_getGraphSnapshot=()=>{calls.push("getGraphSnapshot");return response(receipt());};
host.SFLD_getPanelPulse=()=>{pulseCalls++;return response({ok:true,stamp:"pulse"+revision,frameStatus:{available:false,targetToken:"target"}});};
host.SFLD_selectNodeEffect=text=>{const request=JSON.parse(text);calls.push("selectNodeEffect");
    assert.equal(request.target.token,"target");selectedEffects.push(request.nodeId);
    return response({ok:true,operation:"selectNodeEffect",nodeId:request.nodeId});};
host.SFLD_submitGraph=text=>{
    calls.push("submitGraph");submitCount++;const request=JSON.parse(text);
    assert.equal(request.target.token,"target");assert.equal(request.baseGraphRevision,revision);assert.equal(request.baseRecordStamp,"records-"+revision);
    if(denyNext){denyNext=false;return response({ok:false,error:{code:"stale_graph",message:"Graph changed before commit"}});}
    graph=codec.fromHex(request.graphHex);revision++;return response(receipt());
};
win.__adobe_cep__={getSystemPath:()=>"C:/test/web_panel",evalScript(script,callback){
    const value=vm.runInNewContext(script,host);
    if(holdSelection && script.includes("return SFLD_selectNodeEffect")){releaseSelection=()=>callback(value);}
    else if(holdSubmit && script.includes("return SFLD_submitGraph")){releaseSubmit=()=>callback(value);}
    else if(holdInspection && script.includes("return SFLD_getPanelState")){releaseInspection=()=>callback(value);}
    else if(holdLayoutReply && script.includes("return SFLD_setGraphNodeLayout")){releaseLayoutReply=()=>callback(response({ok:false,error:{code:"layout_unknown",message:"Lost coordinate acknowledgement"}}));}
    else callback(value);
}};
win.localStorage={getItem:()=>null};win.setTimeout=(callback,delay)=>{timers.set(++timerId,{callback,delay});return timerId;};
win.clearTimeout=id=>timers.delete(id);win.requestAnimationFrame=()=>1;win.innerWidth=1000;win.innerHeight=720;
for(const [name,file] of Object.entries({StarfieldGraphLayoutQueue:"graph_layout_queue",StarfieldGraphCodec:"graph_codec",StarfieldGraphLayout:"graph_layout",StarfieldGraphEdits:"graph_edits",
    StarfieldGraphTransactions:"graph_transactions",StarfieldGraphView:"graph_view",StarfieldNativeGraphSnapshot:"native_graph_snapshot"}))
    win[name]=require(path.join(panelRoot,"js",file+".js"));
class Clock extends Date{static now(){return now;}}
const context={window:win,document:doc,Date:Clock,Math,JSON,String,Number,isFinite};
vm.runInNewContext(fs.readFileSync(path.join(panelRoot,"js","node_palette.js"),"utf8"),context);
vm.runInNewContext(fs.readFileSync(process.env.STARFIELD_PANEL_SELECTION_SOURCE || path.join(panelRoot,"js","panel.js"),"utf8"),context);
assert.equal(elements.targetLine.textContent,"Comp / Particles");assert.ok(!entries.particle.disabled,"ready graph enables palette");
// The real panel runs without an inspector DOM or editor helper scripts.
const panelHtml=fs.readFileSync(path.join(panelRoot,"index.html"),"utf8");
assert.ok(!panelHtml.includes('id="inspector"'));
for(const editor of ["editor_presets","curve_editor","gradient_editor"])
    assert.ok(!panelHtml.includes('js/'+editor+'.js'),"graph panel does not load retired editors");
function select(id,event={}){const card=elements.chain.children.find(el=>el.getAttribute("data-node-id")===id);
    const button=card.querySelector(".node-select");assert.match(button.title,/AE Effect Controls/);button.fire("click",event);}
function edit(id,event={}){const card=elements.chain.children.find(el=>el.getAttribute("data-node-id")===id);
    card.querySelector(".node-select").fire("click",event);}
const beforeSelection=codec.toHex(graph),selectionCalls=calls.length;
for(const node of graph.nodes)select(node.id);
assert.deepEqual(selectedEffects,graph.nodes.map(node=>node.id),"single click immediately edits native nodes and Output");
assert.deepEqual(calls.slice(selectionCalls),graph.nodes.map(()=>"selectNodeEffect"),"single clicks only issue selection requests");
select(uuid(1),{ctrlKey:true});select(uuid(2),{shiftKey:true});assert.equal(selectedEffects.length,3,"multi-selection remains graph-local");
for(const node of graph.nodes){select(node.id,{detail:1});select(node.id,{detail:2});
    elements.chain.children.find(el=>el.getAttribute("data-node-id")===node.id).querySelector(".node-select").fire("dblclick",{});}
assert.deepEqual(selectedEffects.slice(3),[uuid(1),uuid(2),uuid(3)],"complete two-click/double-click sequence edits exactly once, including Output");
for(const event of [{ctrlKey:true},{shiftKey:true},{metaKey:true},{altKey:true},{repeat:true}])edit(uuid(1),event);
assert.equal(selectedEffects.length,6,"modified/repeated gestures do not open AE controls");
holdSelection=true;edit(uuid(1));edit(uuid(2));edit(uuid(3));
assert.equal(selectedEffects.length,7,"rapid clicks do not overlap host selection calls");
holdSelection=false;releaseSelection();
assert.deepEqual(selectedEffects.slice(6),[uuid(1),uuid(3)],"queued selection coalesces to the last clicked node");
holdSelection=true;const doubleQueuedStart=selectedEffects.length;edit(uuid(1));select(uuid(2),{detail:1});select(uuid(2),{detail:2});
elements.chain.children.find(el=>el.getAttribute("data-node-id")===uuid(2)).querySelector(".node-select").fire("dblclick",{});
holdSelection=false;releaseSelection();
assert.deepEqual(selectedEffects.slice(doubleQueuedStart),[uuid(1),uuid(2)],"double-click preserves its first click's queued edit while another host selection is in flight");
assert.equal(codec.toHex(graph),beforeSelection,"selection never edits parameters or topology");
holdSelection=true;const staleEditStart=selectedEffects.length;edit(uuid(1));edit(uuid(2));select(uuid(3),{ctrlKey:true});
holdSelection=false;releaseSelection();
assert.equal(selectedEffects.length,staleEditStart+1,"later modifier selection cancels a queued old AE edit intent");
assert.equal(elements.chain.children.find(el=>el.getAttribute("data-node-id")===uuid(3)).querySelector(".node-select").getAttribute("aria-pressed"),"true","late edit acknowledgement preserves current graph selection");
const enterButton=elements.chain.children.find(el=>el.getAttribute("data-node-id")===uuid(2)).querySelector(".node-select");
enterButton.fire("keydown",{key:"Enter"});assert.equal(selectedEffects.at(-1),uuid(2),"Enter explicitly edits the keyboard-focused node");
const afterEnter=selectedEffects.length;enterButton.fire("keydown",{key:"Enter",repeat:true});assert.equal(selectedEffects.length,afterEnter,"held Enter does not repeat host edits");
assert.equal(submitCount,0);assert.equal(revision,1);
function poll(){const timer=Array.from(timers).find(([,value])=>value.callback.name==="pollPanelState");assert.ok(timer);timers.delete(timer[0]);now+=timer[1].delay;timer[1].callback();}
function begin(kind){elements.nodePaletteItems.fire("pointerdown",{target:entries[kind],pointerId:1,button:0,buttons:1,clientX:18,clientY:200});}
function move(){win.fire("pointermove",{pointerId:1,buttons:1,clientX:300,clientY:320});}
function drop(){win.fire("pointerup",{pointerId:1,button:0,buttons:0,clientX:300,clientY:320});}
function submitAfterPaint(){const scheduled=Array.from(timers).find(([,t])=>t.delay===100);
    assert.ok(scheduled,"host write is scheduled after local projection");timers.delete(scheduled[0]);scheduled[1].callback();}
function expectedPosition(){const source=graph.nodes.find(node=>node.type===edits.types.emitter),card=elements.chain.children.find(el=>el.getAttribute("data-node-id")===source.id);
    const position=layout.resolve(graph)[source.id],offset={x:parseFloat(card.style.left)-position.x,y:parseFloat(card.style.top)-position.y};
    const match=/translate\(([-\d.]+)px,\s*([-\d.]+)px\) scale\(([-\d.]+)\)/.exec(canvas.style.transform);assert.ok(match,canvas.style.transform);
    const x=(300-36-Number(match[1]))/Number(match[3])-offset.x-55,y=(320-150-Number(match[2]))/Number(match[3])-offset.y-27;
    return {x,y};}
for(const kind of ["emitter","auxiliary","particle","force"]) {
    const before=graph.nodes.length,prior=new Set(graph.nodes.map(node=>node.id)),position=expectedPosition(),count=submitCount,pulses=pulseCalls;
    begin(kind);for(let i=0;i<20;i++)move();poll();assert.equal(pulseCalls,pulses,"palette drag pauses idle host polling");
    assert.equal(submitCount,count,"no transaction during movement");drop();
    assert.equal(elements.chain.children.length,before+1,"local addition is visible before host submission");
    assert.equal(submitCount,count,"painting precedes host submission");submitAfterPaint();assert.equal(submitCount,count+1);
    assert.equal(graph.nodes.length,before+1);assert.equal(graph.nodes.filter(node=>node.type===edits.types.output).length,1);
    const added=graph.nodes.find(node=>!prior.has(node.id)),actual=layout.resolve(graph)[added.id];
    assert.ok(Math.abs(actual.x-position.x)<1e-7);assert.ok(Math.abs(actual.y-position.y)<1e-7);
    assert.equal(added.type,edits.types[kind==="auxiliary"?"emitter":kind]);
    assert.ok(!entries.particle.disabled,"acknowledgement reenables palette");
    if(kind==="particle")elements.graphScroll.fire("wheel",{clientX:300,clientY:320,deltaY:-160});
}
begin("particle");move();const beforeCancel=submitCount;doc.fire("keydown",{key:"Escape"});drop();assert.equal(submitCount,beforeCancel);
poll();assert.ok(pulseCalls>0,"polling resumes after palette cancellation");
const burstStart=graph.nodes.length,burstReads=calls.filter(x=>x==="getPanelState").length;
holdSubmit=true;begin("force");move();drop();submitAfterPaint();assert.equal(submitCount,beforeCancel+1);
assert.ok(!entries.particle.disabled,"pending addition keeps the palette available");
for(let i=0;i<9;i++){begin("particle");move();drop();}
assert.equal(elements.chain.children.length,burstStart+10,"later nodes appear while the first acknowledgement is held");
assert.equal(submitCount,beforeCancel+1,"later additions do not overlap the current native write");
begin("force");move();holdSubmit=false;releaseSubmit();
drop();assert.equal(elements.chain.children.length,burstStart+11,"our receipt does not interrupt an ongoing palette drag");
const firstCard=elements.chain.children[0],previewIds=elements.chain.children.map(el=>el.getAttribute("data-node-id"));
submitAfterPaint();assert.equal(submitCount,beforeCancel+2,"ten waiting additions share the second guarded commit");
assert.equal(graph.nodes.length,burstStart+11);assert.deepEqual(new Set(graph.nodes.map(n=>n.id)),new Set(previewIds));
assert.equal(elements.chain.children[0],firstCard,"matching saved receipt does not rebuild the confirmed canvas");
assert.equal(calls.filter(x=>x==="getPanelState").length,burstReads,"successful additions do not trigger repeated full panel reads");
assert.ok(!entries.particle.disabled);
denyNext=true;const oldHex=codec.toHex(graph);begin("particle");move();drop();submitAfterPaint();assert.equal(codec.toHex(graph),oldHex);
assert.ok(!entries.particle.disabled,"rejected transaction leaves palette usable");
assert.match(elements.banner.textContent,/SF-T107/);
assert.match(elements.banner.children.at(-1).children.at(-1).textContent,/stale_graph/);
// A read already in flight must not make the ready palette feel unresponsive.
// The existing interaction/epoch guards discard its reply during/after a drop.
for(const during of [true,false]) {
    holdInspection=true;elements.refresh.fire("click");
    const delayedInspection=releaseInspection;
    const before=submitCount;begin("particle");move();assert.equal(submitCount,before);
    if(during){holdInspection=false;delayedInspection();}
    holdInspection=false;
    drop();submitAfterPaint();assert.equal(submitCount,before+1,"ready palette can drop while an inspection is pending");
    const submittedHex=codec.toHex(graph),renderedCount=elements.chain.children.length;
    if(!during)delayedInspection();
    assert.equal(codec.toHex(graph),submittedHex);assert.equal(elements.chain.children.length,renderedCount,"old inspection cannot replace committed view");
    assert.ok(!entries.particle.disabled);
}
assert.ok(calls.filter(x=>x==="submitGraph").length>=6,"real panel routes palette drops through actual graph transactions");
// Native Clear notification must enter the same real full-transaction routing
// as DOM Delete. Stop at the transport boundary; no native author writes here.
{
    const model=require(path.join(panelRoot,"js/model_graph_transactions.js")),originalCreate=model.create;
    const force=graph.nodes.find(node=>node.type===edits.types.force),before=codec.toHex(graph),ordinary=submitCount;
    let full=0,complete;
    model.create=()=>({apply(transaction,restore,callback){full++;complete=callback;
        assert.ok(transaction.baseNodeManifest.some(n=>n.id===force.id));
        assert.ok(!transaction.nodeManifest.some(n=>n.id===force.id),"Force removal is planned before the full native executor");}});
    try {
        select(force.id);doc.activeElement=new Element("text","input");win.fire("starfield:nativeDelete");
        assert.equal(full,0,"browser text input never becomes graph deletion");
        doc.activeElement=null;win.fire("starfield:nativeDelete");submitAfterPaint();
        assert.equal(full,1,"native notification enters full graph transaction");assert.equal(submitCount,ordinary,"removal never uses ordinary submitGraph");
        win.fire("starfield:nativeDelete");assert.equal(full,1,"pending deletion ignores a second notification");
        complete({ok:false,error:{code:"probe_rejected",message:"Transport intentionally stopped"}});
        assert.equal(codec.toHex(graph),before,"rejected transport leaves authoritative graph unchanged");
        select(uuid(3));win.fire("starfield:nativeDelete");assert.equal(full,1,"Output-only selection cannot be deleted");
    } finally {model.create=originalCreate;}
}
{
    const force=graph.nodes.find(node=>node.type===edits.types.force),ordinary=submitCount;
    let editsSent=0,finish;
    win.StarfieldNativeGraph={deleteNodes(target,base,checksum,ids,callback){editsSent++;finish=callback;
        assert.equal(target,"target");assert.equal(base,revision);assert.deepEqual(Array.from(ids),[force.id]);}};
    select(force.id);doc.activeElement=null;win.fire("starfield:nativeDelete");
    assert.equal(editsSent,0,"native deletion lets Chromium paint pending feedback before host work");
    assert.match(elements.banner.textContent,/Deleting/);submitAfterPaint();
    assert.equal(editsSent,1,"paired native panel sends one bounded delta");assert.equal(submitCount,ordinary,"native delta bypasses JSX submitGraph");
    win.fire("starfield:nativeDelete");assert.equal(editsSent,1,"pending native delete is not repeated");
    const before=codec.toHex(graph);finish({ok:false,committed:false,error:{code:"native_probe",message:"stopped before write"}});
    assert.equal(codec.toHex(graph),before);assert.match(elements.banner.textContent,/native_probe/);
    select(force.id);win.fire("starfield:nativeDelete");submitAfterPaint();assert.equal(editsSent,2);
    graph=edits.apply(graph,{type:"deleteNodes",nodeIds:[force.id]});revision++;
    const reads=calls.filter(c=>c==="getGraphSnapshot").length;
    finish({ok:true,committed:true,revision});assert.equal(calls.filter(c=>c==="getGraphSnapshot").length,reads+1,"one authoritative read after native publication");
    assert.ok(!elements.chain.children.some(c=>c.getAttribute("data-node-id")===force.id));
    delete win.StarfieldNativeGraph;
}
// Ordinary signed-coordinate dragging must stay outside graph publication.
{
    let layoutSaves=0;
    host.SFLD_setGraphNodeLayout=text=>{
        const request=JSON.parse(text);calls.push("setGraphNodeLayout");layoutSaves++;
        const positions=layout.resolve(graph);
        for(const change of request.positions){assert.deepEqual(change.from,positions[change.id]);positions[change.id]=change.to;}
        graph=layout.set(graph,positions);return response({ok:true,target:{token:"target"}});
    };
    const id=uuid(1),old=layout.resolve(graph)[id],oldSubmits=submitCount,oldReads=calls.filter(x=>x==="getPanelState").length;
    const button=elements.chain.children.find(c=>c.getAttribute("data-node-id")===id).querySelector(".node-select");
    button.fire("pointerdown",{button:0,pointerId:70,clientX:100,clientY:100});
    for(let i=1;i<=100;i++)win.fire("pointermove",{pointerId:70,clientX:100+i*5,clientY:100-i*4});
    assert.equal(layoutSaves,0);assert.equal(submitCount,oldSubmits,"drag movement never publishes");
    win.fire("pointerup",{pointerId:70,clientX:600,clientY:-300});
    assert.equal(layoutSaves,0,"drop is local until the quiet save");
    assert.ok(!entries.particle.disabled,"cosmetic save does not lock palette editing");
    const quiet=Array.from(timers).find(([,t])=>t.delay===600);assert.ok(quiet);timers.delete(quiet[0]);quiet[1].callback();
    assert.equal(layoutSaves,1);assert.equal(submitCount,oldSubmits,"coordinate persistence does not compile graph");
    assert.equal(calls.filter(x=>x==="getPanelState").length,oldReads,"layout ACK does not request a full state");
    const moved=layout.resolve(graph)[id];assert.ok(moved.x>old.x && moved.y<old.y,"final signed position is persisted");
    // First subsequent manifest edit refreshes only once to include current positions.
    const snapshots=calls.filter(x=>x==="getGraphSnapshot").length;
    begin("particle");move();drop();submitAfterPaint();
    assert.equal(calls.filter(x=>x==="getGraphSnapshot").length,snapshots+1);
    assert.deepEqual(layout.resolve(graph)[id],moved,"structural edit preserves the newly saved layout");
    const beforeHistoryGraph=graph,historyCalls=calls.length;
    const nextButton=elements.chain.children.find(c=>c.getAttribute("data-node-id")===id).querySelector(".node-select");
    nextButton.fire("pointerdown",{button:0,pointerId:71,clientX:100,clientY:100});
    win.fire("pointermove",{pointerId:71,clientX:800,clientY:-400});
    win.fire("pointerup",{pointerId:71,clientX:800,clientY:-400});
    assert.equal(layoutSaves,1,"second move is still pending when Undo is pressed");
    win.StarfieldGraphHistory(done=>{calls.push("undo");graph=beforeHistoryGraph;done({ok:true});});
    assert.deepEqual(calls.slice(historyCalls),["setGraphNodeLayout","undo","getPanelState"],"Undo saves the move first, dispatches once and refreshes once after ACK");
    assert.equal(layoutSaves,2);assert.deepEqual(layout.resolve(graph)[id],moved,"Undo restores the prior saved position");
    const timersAfterHistory=Array.from(timers.values()).filter(t=>t.delay===600);assert.equal(timersAfterHistory.length,0,"Undo leaves no delayed move to reapply");
    const afterUndoButton=elements.chain.children.find(c=>c.getAttribute("data-node-id")===id).querySelector(".node-select");
    afterUndoButton.fire("pointerdown",{button:0,pointerId:72,clientX:100,clientY:100});
    win.fire("pointermove",{pointerId:72,clientX:300,clientY:300});
    win.fire("pointerup",{pointerId:72,clientX:300,clientY:300});
    win.StarfieldGraphHistory(done=>{calls.push("undo");graph=beforeHistoryGraph;done({ok:true});});
    assert.equal(layoutSaves,3,"drag after Undo uses restored host coordinates as its CAS baseline");
    const saveLayout=host.SFLD_setGraphNodeLayout;
    host.SFLD_setGraphNodeLayout=()=>{calls.push("setGraphNodeLayout");return response({ok:false,error:{code:"layout_rejected",message:"Stopped before write"}});};
    const failedButton=elements.chain.children.find(c=>c.getAttribute("data-node-id")===id).querySelector(".node-select");
    failedButton.fire("pointerdown",{button:0,pointerId:73,clientX:100,clientY:100});
    win.fire("pointermove",{pointerId:73,clientX:1000,clientY:-900});
    win.fire("pointerup",{pointerId:73,clientX:1000,clientY:-900});
    const failedTimer=Array.from(timers).find(([,t])=>t.delay===600);assert.ok(failedTimer);timers.delete(failedTimer[0]);failedTimer[1].callback();
    assert.match(elements.banner.textContent,/layout_rejected/);
    host.SFLD_setGraphNodeLayout=saveLayout;
    const retryButton=elements.chain.children.find(c=>c.getAttribute("data-node-id")===id).querySelector(".node-select");
    retryButton.fire("pointerdown",{button:0,pointerId:74,clientX:100,clientY:100});
    win.fire("pointermove",{pointerId:74,clientX:350,clientY:350});
    win.fire("pointerup",{pointerId:74,clientX:350,clientY:350});
    win.StarfieldGraphHistory(done=>{graph=beforeHistoryGraph;done({ok:true});});
    assert.equal(layoutSaves,4,"after rejected save, next user drag starts from the refreshed host baseline");
    const counts=()=>calls.filter(x=>x==="setGraphNodeLayout").length;
    function dragStart(pointer,x,y) {
        const el=elements.chain.children.find(c=>c.getAttribute("data-node-id")===id).querySelector(".node-select");
        el.fire("pointerdown",{button:0,pointerId:pointer,clientX:100,clientY:100});win.fire("pointermove",{pointerId:pointer,clientX:x,clientY:y});
    }
    holdLayoutReply=true;dragStart(75,500,500);win.fire("pointerup",{pointerId:75,clientX:500,clientY:500});
    const heldTimer=Array.from(timers).find(([,t])=>t.delay===600);timers.delete(heldTimer[0]);heldTimer[1].callback();
    const beforeUnknown=counts(),unknownReads=calls.filter(x=>x==="getPanelState").length;
    dragStart(76,900,900);holdLayoutReply=false;releaseLayoutReply();
    assert.equal(calls.filter(x=>x==="getPanelState").length,unknownReads,"failed older ACK does not start a read during a newer gesture");
    win.fire("pointerup",{pointerId:76,clientX:900,clientY:900});
    assert.equal(counts(),beforeUnknown,"gesture already active before an unknown result cannot resubmit from the unknown baseline");
    assert.equal(calls.filter(x=>x==="getPanelState").length,unknownReads+1,"gesture end restores host state through one read");
    assert.equal(Array.from(timers.values()).filter(t=>t.delay===600).length,0,"unknown result leaves no automatic coordinate retry");
}
console.log("panel_palette_tests passed: actual panel drag/metadata barrier, batched additions, single-click editing and native deletion");
