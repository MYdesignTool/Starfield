"use strict";
// Actual panel + palette + graph edit/transaction modules, no AE process.
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const panelRoot=path.resolve(process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"../web_panel"));
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
    remove(){if(this.parentNode)this.parentNode.removeChild(this);}
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
let panel=fs.readFileSync(path.join(panelRoot,"js","panel.js"),"utf8");
panel=panel.replace(/    refresh\(true, true\);\s*\}\)\(\);\s*$/,'    window.reviewProbe={state:state,diagnostics:requestNodeDiagnostics,render:function(){render(state);},setGraph:function(g){var v=window.StarfieldGraphView.project(g);state.graph=g;state.nodes=v.nodes;state.edges=v.edges;nodePositions=v.positions;render(state);},enqueue:queueTopologyEdit,end:endNodeDrag,begin:function(){dragState={moved:false,copy:false,origins:[]};},draft:function(){return queuedDraft;}}; refresh(true, true);\r\n})();');
win.StarfieldEditQueue=require(path.join(panelRoot,"js/edit_queue.js"));
vm.runInNewContext(panel,context);
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


const q=win.reviewProbe,callbacks=[];
win.StarfieldNativeGraph={diagnostics(target,rev,crc,scope,done){callbacks.push({rev,crc,done});}};
const draft=codec.fromHex(q.state.graphSnapshot.graphHex),oldOutput=draft.nodes.find(n=>n.type===edits.types.output);
draft.edges.forEach(e=>{if(e.destinationNode===oldOutput.id)e.destinationNode=uuid(255);});oldOutput.id=uuid(255);draft.optionalRecords=[];
draft.nodes.push(fixture.node("emitter",uuid(4)),fixture.node("particle",uuid(5)));
draft.edges.push({id:uuid(13),sourceNode:uuid(4),sourcePort:"1",destinationNode:uuid(5),destinationPort:"1"},{id:uuid(14),sourceNode:uuid(5),sourcePort:"2",destinationNode:uuid(255),destinationPort:"1"});
q.setGraph(draft);q.state.graphSnapshot.projectScope="";q.state.graphSnapshot.checksum="00000001";
q.diagnostics(q.state.graphSnapshot);
const issue={edge:uuid(12),source:uuid(2),destination:uuid(255),code:"SF-G103",message:"cannot merge Motion chains <script>"};
function reply(entry,issues){entry.done({ok:true,revision:entry.rev,checksum:parseInt(entry.crc,16),issues});}
reply(callbacks[0],[issue]);
function card(id){return elements.chain.children.find(e=>e.getAttribute("data-node-id")===id);}
function marker(id){return card(id).querySelector(".node-warning");}
assert.equal(marker(uuid(2)).textContent,"!");assert.ok(marker(uuid(2)).classList.contains("error"));
assert.ok(card(uuid(2)).classList.contains("has-node-error"));
assert.match(marker(uuid(2)).getAttribute("aria-label"),/SF-G103/);
assert.ok(!marker(uuid(2)).title,"custom tooltip replaces browser title");
for(const id of [1,255,4,5])assert.equal(marker(uuid(id)),null,"ancestors, Output and unrelated branches do not inherit the source error");
function popup(){return doc.body.querySelector(".node-diagnostic-tooltip");}
marker(uuid(2)).fire("mouseenter");assert.equal(popup().getAttribute("role"),"tooltip");
assert.equal(popup().children[0].children[0].textContent,"Error · SF-G103");
assert.match(popup().children[0].children[1].textContent,/<script>/,"diagnostic is text, never injected markup");
assert.equal(popup().querySelectorAll("script").length,0);
assert.ok(parseInt(popup().style.left)>=8 && parseInt(popup().style.top)>=8);
marker(uuid(2)).fire("mouseleave");assert.ok(popup(),"brief gap lets pointer reach the popup");
popup().fire("mouseenter");assert.ok(!Array.from(timers.values()).some(t=>t.delay===150));
popup().fire("mouseleave");Array.from(timers.values()).find(t=>t.delay===150).callback();assert.equal(popup(),null);
marker(uuid(2)).fire("focus");assert.ok(popup(),"keyboard focus shows the same popup");
marker(uuid(2)).fire("keydown",{key:"Escape"});assert.equal(popup(),null);
marker(uuid(2)).fire("mouseenter");win.fire("scroll");assert.equal(popup(),null);
marker(uuid(2)).fire("mouseenter");win.fire("keydown",{key:"Escape"});assert.equal(popup(),null,"Escape dismisses a pointer tooltip without keyboard focus");
q.render();assert.match(marker(uuid(2)).getAttribute("aria-label"),/SF-G103/,"error survives ordinary repaint");
q.state.graphSnapshot.checksum="00000002";q.state.additionEpoch++;q.diagnostics(q.state.graphSnapshot);
reply(callbacks[0],[{...issue,message:"old branch"}]);assert.equal(marker(uuid(2)),null,"late old-CRC/epoch response cannot repaint warnings after Undo branch");
reply(callbacks[1],[{...issue,message:"current branch"}]);assert.match(marker(uuid(2)).getAttribute("aria-label"),/current branch/);
q.state.graphSnapshot.checksum="00000003";q.diagnostics(q.state.graphSnapshot);callbacks[2].done({ok:false});
assert.match(elements.resolutionLine.textContent,/Warnings unavailable/,"unavailable read is not presented as no issues");
// Exact owner topology: Emitter -> Particle -> Auxiliary -> Output. Only Aux
// owns the rejected final edge; no graph write/diagnostic read on hover or paint.
const aux=fixture.node("emitter",uuid(6));aux.parameters=aux.parameters.filter(p=>p.key!=="31");aux.parameters.push({key:"31",type:3,value:1});
draft.nodes.push(aux);draft.edges.find(e=>e.id===uuid(12)).destinationNode=aux.id;
draft.edges.push({id:uuid(15),sourceNode:aux.id,sourcePort:"1",destinationNode:uuid(255),destinationPort:"1"});
q.setGraph(draft);q.state.graphSnapshot.checksum="00000004";q.diagnostics(q.state.graphSnapshot);
reply(callbacks[3],[{edge:uuid(15),source:aux.id,destination:uuid(255),code:"SF-G101",message:"Auxiliary cannot directly output"}]);
assert.ok(marker(aux.id).classList.contains("error"));
for(const id of [1,2,255,4,5])assert.equal(marker(uuid(id)),null);
const particle=draft.nodes.find(n=>n.id===uuid(2));particle.parameters=particle.parameters.filter(p=>p.key!=="15");particle.parameters.push({key:"15",type:3,value:4});
const reads=callbacks.length,writes=submitCount;q.setGraph(draft);
assert.ok(marker(particle.id).classList.contains("warning"));assert.ok(!card(particle.id).classList.contains("has-node-error"));
marker(particle.id).fire("mouseenter");assert.equal(popup().children[0].children[0].textContent,"Warning · SF-W101");
assert.match(popup().children[0].children[1].textContent,/default cube/);
q.render();assert.ok(popup(),"unchanged diagnostics survive ordinary repaint without flicker");assert.equal(callbacks.length,reads);assert.equal(submitCount,writes);
const model={id:uuid(7),type:edits.types.model,schemaVersion:1,parameters:[]};draft.nodes.push(model);
draft.edges.push({id:uuid(16),sourceNode:model.id,sourcePort:"1",destinationNode:particle.id,destinationPort:"3"});q.setGraph(draft);
assert.equal(marker(particle.id),null,"valid Model input removes fallback warning immediately");assert.equal(popup(),null,"resolved issue closes the old popup");
draft.edges=draft.edges.filter(e=>e.sourceNode!==aux.id);q.setGraph(draft);assert.equal(marker(aux.id),null);assert.ok(!card(aux.id).classList.contains("has-node-error"));
console.log("Actual panel diagnostics: local severity, red outline, cube warning, dark popup, safe text, no host calls, stale receipt and unavailable status passed");
