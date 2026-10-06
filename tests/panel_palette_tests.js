"use strict";
// Actual panel + palette + graph edit/transaction modules, no AE process.
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const panelRoot=process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"..","cep_panel");
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
    "inspectorTitle","inspectorMeta","inspectorBody","inspector","inspectorDrag","closeInspector","targetLine","modeLine","revisionLine","resolutionLine",
    "refresh","autoRefresh","targetLock","graphContextMenu","disconnectContextAction","nodePalette","nodePaletteItems","nodePaletteToggle"];
const elements=Object.fromEntries(ids.map(id=>[id,new Element(id)])),win=new Element("window"),doc=new Element("document"),canvas=new Element("canvas");
doc.body=new Element("body");doc.hidden=false;doc.getElementById=id=>elements[id];doc.querySelector=()=>canvas;doc.createElement=tag=>new Element("created",tag);
doc.createElementNS=(namespace,tag)=>doc.createElement(tag);doc.elementFromPoint=()=>canvas;
elements.graphScroll.appendChild(elements.graphViewport);elements.graphViewport.appendChild(canvas);canvas.appendChild(elements.graphEdges);
canvas.appendChild(elements.chain);elements.graphEdges.appendChild(elements.edgePaths);elements.graphViewport.appendChild(elements.graphMinimap);
elements.workspace.appendChild(elements.graphScroll);elements.workspace.appendChild(elements.inspector);elements.inspector.hidden=true;
elements.graphContextMenu.hidden=true;elements.autoRefresh.checked=true;
const entries={};for(const kind of ["emitter","auxiliary","particle","force"]){const el=new Element(kind,"button");el.setAttribute("data-node-type",kind);elements.nodePaletteItems.appendChild(el);entries[kind]=el;}
let revision=1,pulseCalls=0,submitCount=0,holdSubmit=false,releaseSubmit=null,denyNext=false,now=1000;
let holdInspection=false,releaseInspection=null;
const calls=[],timers=new Map();let timerId=0;
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
host.SFLD_submitGraph=text=>{
    calls.push("submitGraph");submitCount++;const request=JSON.parse(text);
    assert.equal(request.target.token,"target");assert.equal(request.baseGraphRevision,revision);assert.equal(request.baseRecordStamp,"records-"+revision);
    if(denyNext){denyNext=false;return response({ok:false,error:{code:"stale_graph",message:"Graph changed before commit"}});}
    graph=codec.fromHex(request.graphHex);revision++;return response(receipt());
};
win.__adobe_cep__={getSystemPath:()=>"C:/test/cep_panel",evalScript(script,callback){
    const value=vm.runInNewContext(script,host);
    if(holdSubmit && script.includes("return SFLD_submitGraph")){releaseSubmit=()=>callback(value);}
    else if(holdInspection && script.includes("return SFLD_getPanelState")){releaseInspection=()=>callback(value);}
    else callback(value);
}};
win.localStorage={getItem:()=>null};win.setTimeout=(callback,delay)=>{timers.set(++timerId,{callback,delay});return timerId;};
win.clearTimeout=id=>timers.delete(id);win.requestAnimationFrame=()=>1;win.innerWidth=1000;win.innerHeight=720;
for(const [name,file] of Object.entries({StarfieldGraphCodec:"graph_codec",StarfieldGraphLayout:"graph_layout",StarfieldGraphEdits:"graph_edits",
    StarfieldGraphTransactions:"graph_transactions",StarfieldGraphView:"graph_view",StarfieldNativeGraphSnapshot:"native_graph_snapshot"}))
    win[name]=require(path.join(panelRoot,"js",file+".js"));
class Clock extends Date{static now(){return now;}}
const context={window:win,document:doc,Date:Clock,Math,JSON,String,Number,isFinite};
vm.runInNewContext(fs.readFileSync(path.join(panelRoot,"js","node_palette.js"),"utf8"),context);
vm.runInNewContext(fs.readFileSync(path.join(panelRoot,"js","panel.js"),"utf8"),context);
assert.equal(elements.targetLine.textContent,"Comp / Particles");assert.ok(!entries.particle.disabled,"ready graph enables palette");
function poll(){const timer=Array.from(timers).find(([,value])=>value.delay===500||value.delay===2000);assert.ok(timer);timers.delete(timer[0]);now+=timer[1].delay;timer[1].callback();}
function begin(kind){elements.nodePaletteItems.fire("pointerdown",{target:entries[kind],pointerId:1,button:0,buttons:1,clientX:18,clientY:200});}
function move(){win.fire("pointermove",{pointerId:1,buttons:1,clientX:300,clientY:320});}
function drop(){win.fire("pointerup",{pointerId:1,button:0,buttons:0,clientX:300,clientY:320});}
function expectedPosition(){const source=graph.nodes.find(node=>node.type===edits.types.emitter),card=elements.chain.children.find(el=>el.getAttribute("data-node-id")===source.id);
    const position=layout.resolve(graph)[source.id],offset={x:parseFloat(card.style.left)-position.x,y:parseFloat(card.style.top)-position.y};
    const match=/translate\(([-\d.]+)px,\s*([-\d.]+)px\) scale\(([-\d.]+)\)/.exec(canvas.style.transform);assert.ok(match,canvas.style.transform);
    const x=(300-36-Number(match[1]))/Number(match[3])-offset.x-55,y=(320-150-Number(match[2]))/Number(match[3])-offset.y-27;
    return {x,y};}
for(const kind of ["emitter","auxiliary","particle","force"]) {
    const before=graph.nodes.length,prior=new Set(graph.nodes.map(node=>node.id)),position=expectedPosition(),count=submitCount,pulses=pulseCalls;
    begin(kind);for(let i=0;i<20;i++)move();poll();assert.equal(pulseCalls,pulses,"palette drag pauses idle host polling");
    assert.equal(submitCount,count,"no transaction during movement");drop();assert.equal(submitCount,count+1);
    assert.equal(graph.nodes.length,before+1);assert.equal(graph.nodes.filter(node=>node.type===edits.types.output).length,1);
    const added=graph.nodes.find(node=>!prior.has(node.id)),actual=layout.resolve(graph)[added.id];
    assert.ok(Math.abs(actual.x-position.x)<1e-7);assert.ok(Math.abs(actual.y-position.y)<1e-7);
    assert.equal(added.type,edits.types[kind==="auxiliary"?"emitter":kind]);
    assert.ok(!entries.particle.disabled,"acknowledgement reenables palette");
    if(kind==="particle")elements.graphScroll.fire("wheel",{clientX:300,clientY:320,deltaY:-160});
}
begin("particle");move();const beforeCancel=submitCount;doc.fire("keydown",{key:"Escape"});drop();assert.equal(submitCount,beforeCancel);
poll();assert.ok(pulseCalls>0,"polling resumes after palette cancellation");
holdSubmit=true;begin("force");move();drop();assert.equal(submitCount,beforeCancel+1);assert.ok(entries.particle.disabled,"pending commit disables palette");
begin("particle");move();drop();assert.equal(submitCount,beforeCancel+1,"pending mutation prevents another drop");
holdSubmit=false;releaseSubmit();assert.ok(!entries.particle.disabled);
denyNext=true;const oldHex=codec.toHex(graph);begin("particle");move();drop();assert.equal(codec.toHex(graph),oldHex);
assert.ok(!entries.particle.disabled,"rejected transaction leaves palette usable");
assert.match(elements.banner.textContent,/stale_graph/);
// A read already in flight must not make the ready palette feel unresponsive.
// The existing interaction/epoch guards discard its reply during/after a drop.
for(const during of [true,false]) {
    holdInspection=true;elements.refresh.fire("click");
    const delayedInspection=releaseInspection;
    const before=submitCount;begin("particle");move();assert.equal(submitCount,before);
    if(during){holdInspection=false;delayedInspection();}
    holdInspection=false;
    drop();assert.equal(submitCount,before+1,"ready palette can drop while an inspection is pending");
    const submittedHex=codec.toHex(graph),renderedCount=elements.chain.children.length;
    if(!during)delayedInspection();
    assert.equal(codec.toHex(graph),submittedHex);assert.equal(elements.chain.children.length,renderedCount,"old inspection cannot replace committed view");
    assert.ok(!entries.particle.disabled);
}
assert.ok(calls.filter(x=>x==="submitGraph").length>=6,"real panel routes palette drops through actual graph transactions");
console.log("panel_palette_tests passed: real panel/transaction drops, pan/zoom/offset, one Output, poll suspension, pending writes, rejected edits and in-flight inspection guards");
