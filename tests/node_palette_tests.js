"use strict";
const assert=require("assert"),fs=require("fs"),path=require("path"),vm=require("vm");
const panelRoot=process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"..","cep_panel");
class Element {
    constructor(name) { this.name=name;this.children=[];this.listeners={};this.attributes={};this.style={};this.classes=new Set();
        this.classList={add:x=>this.classes.add(x),remove:x=>this.classes.delete(x),toggle:(x,on)=>on?this.classes.add(x):this.classes.delete(x)}; }
    addEventListener(type,callback){(this.listeners[type]||(this.listeners[type]=[])).push(callback);}
    fire(type,event={}){event.target=event.target||this;event.currentTarget=this;event.preventDefault=()=>event.prevented=true;event.stopPropagation=()=>event.stopped=true;
        for(const fn of this.listeners[type]||[])fn(event);return event;}
    setAttribute(key,value){this.attributes[key]=String(value);}
    getAttribute(key){return this.attributes[key];}
    appendChild(child){this.children.push(child);child.parentNode=this;}
    removeChild(child){this.children=this.children.filter(x=>x!==child);child.parentNode=null;}
    contains(child){return child===this||this.children.some(x=>x.contains(child));}
    querySelectorAll(){return this.children.filter(x=>x.getAttribute("data-node-type"));}
    getBoundingClientRect(){return {left:40,top:80,right:540,bottom:400};}
    setPointerCapture(id){this.captured=id;}
    releasePointerCapture(id){if(this.captured===id)this.captured=null;}
}
const source=fs.readFileSync(path.join(panelRoot,"js","node_palette.js"),"utf8");
function fixture() {
    const win=new Element("window"),doc=new Element("document"),body=new Element("body");doc.body=body;doc.createElement=name=>new Element(name);
    const root=new Element("palette"),items=new Element("items"),toggle=new Element("toggle"),viewport=new Element("viewport");
    root.appendChild(items);root.appendChild(toggle);body.appendChild(root);body.appendChild(viewport);
    const nodes={};for(const kind of ["emitter","auxiliary","particle","force","transform"]){const button=new Element(kind);button.setAttribute("data-node-type",kind);items.appendChild(button);nodes[kind]=button;}
    const canvas=new Element("canvas");viewport.appendChild(canvas);let surface=canvas,guard="target1/revision1",resizeCount=0,prepareCount=0;
    doc.elementFromPoint=()=>surface;
    const calls=[];vm.runInNewContext(source,{window:win,document:doc,isFinite});
    const palette=win.StarfieldNodePalette.create({root,items,toggle,viewport,document:doc,window:win,guard:()=>guard,
        prepare:()=>prepareCount++,zoom:()=>.75,resize:()=>resizeCount++,isBlocked:el=>el.blocked,
        add:(...args)=>{calls.push(args);}});
    return {win,doc,body,root,items,toggle,viewport,nodes,canvas,palette,calls,setGuard:x=>guard=x,setSurface:x=>surface=x,
        resizeCount:()=>resizeCount,prepareCount:()=>prepareCount};
}
function pointer(f,kind="particle",id=1){return f.items.fire("pointerdown",{target:f.nodes[kind],pointerId:id,button:0,buttons:1,isPrimary:true,clientX:20,clientY:100});}
function move(f,x=200,y=180,id=1){return f.win.fire("pointermove",{pointerId:id,buttons:1,clientX:x,clientY:y});}
function drop(f,x=200,y=180,id=1){return f.win.fire("pointerup",{pointerId:id,button:0,buttons:0,clientX:x,clientY:y});}

const initial=fixture();assert.strictEqual(initial.items.hidden,false);assert.strictEqual(initial.toggle.getAttribute("aria-expanded"),"true");
assert.strictEqual(initial.toggle.textContent,"‹");assert.ok(!initial.root.classes.has("collapsed"));
initial.toggle.fire("click");assert.strictEqual(initial.items.hidden,true);assert.strictEqual(initial.toggle.getAttribute("aria-expanded"),"false");
assert.strictEqual(initial.toggle.textContent,"›");pointer(initial);assert.ok(!initial.palette.isDragging());
initial.toggle.fire("click");assert.strictEqual(initial.items.hidden,false);assert.strictEqual(initial.resizeCount(),3);

for(const kind of ["emitter","auxiliary","particle","force","transform"]) {
    const f=fixture();pointer(f,kind);assert.ok(f.palette.isDragging());assert.strictEqual(f.nodes[kind].captured,1);
    for(let i=0;i<120;i++)move(f,170+i*.1,190);
    assert.strictEqual(f.calls.length,0,"pointer movement must not write the graph");
    assert.ok(f.viewport.classes.has("palette-drop-target"));
    const preview=f.body.children.find(x=>x.name==="div");assert.strictEqual(preview.style.transform,"scale(0.75)");
    drop(f,240,200);assert.deepStrictEqual(f.calls,[[kind,240,200]]);assert.ok(!f.palette.isDragging());
    assert.strictEqual(f.nodes[kind].captured,null);assert.strictEqual(f.body.children.length,2);assert.ok(!f.viewport.classes.has("palette-drop-target"));
    drop(f);assert.strictEqual(f.calls.length,1,"duplicate pointerup must not add twice");
}

for(const finish of [
    f=>drop(f,20,100), f=>drop(f,540,200), f=>drop(f,200,400), f=>drop(f,NaN,200),
    f=>f.win.fire("pointercancel",{pointerId:1}), f=>f.items.fire("lostpointercapture",{pointerId:1}),
    f=>f.win.fire("blur"), f=>f.doc.fire("keydown",{key:"Escape"}),
    f=>{f.doc.hidden=true;f.doc.fire("visibilitychange");}, f=>f.toggle.fire("click"),
    f=>{f.setGuard(null);drop(f);}, f=>{f.setGuard("target2/revision1");drop(f);},
    f=>{f.setGuard("target1/revision2");move(f);}, f=>{move(f);f.setSurface(new Element("inspector"));drop(f);},
    f=>{const minimap=new Element("minimap");minimap.blocked=true;f.viewport.appendChild(minimap);f.setSurface(minimap);drop(f);},
    f=>f.win.fire("pointermove",{pointerId:1,buttons:0,clientX:200,clientY:180})
]) {
    const f=fixture();pointer(f);move(f);finish(f);assert.strictEqual(f.calls.length,0,"cancel/outside drop must not write");
    assert.ok(!f.palette.isDragging());assert.strictEqual(f.body.children.length,2);
}
const click=fixture();pointer(click);drop(click,20,100);assert.strictEqual(click.calls.length,0,"click is not drag-to-add");
const unready=fixture();unready.setGuard(null);unready.palette.refresh();assert.ok(unready.nodes.particle.disabled);pointer(unready);assert.ok(!unready.palette.isDragging());
unready.setGuard("ready");unready.palette.refresh();assert.ok(!unready.nodes.particle.disabled);
const alternate=fixture();alternate.items.fire("pointerdown",{target:alternate.nodes.particle,pointerId:1,button:2,isPrimary:true});assert.ok(!alternate.palette.isDragging());
alternate.items.fire("pointerdown",{target:alternate.nodes.particle,pointerId:1,button:0,isPrimary:false});assert.ok(!alternate.palette.isDragging());
pointer(alternate);move(alternate,200,180,2);drop(alternate,200,180,2);assert.ok(alternate.palette.isDragging());assert.strictEqual(alternate.calls.length,0);
alternate.win.fire("pointercancel",{pointerId:2});assert.ok(alternate.palette.isDragging());drop(alternate,200,180);assert.ok(!alternate.palette.isDragging());
const unknown=fixture(),output=new Element("output");output.setAttribute("data-node-type","output");unknown.items.appendChild(output);
unknown.items.fire("pointerdown",{target:output,pointerId:1,button:0,buttons:1});assert.ok(!unknown.palette.isDragging(),"Output cannot be added");

const html=fs.readFileSync(path.join(panelRoot,"index.html"),"utf8");
const paletteMarkup=html.slice(html.indexOf('<aside id="nodePalette"'),html.indexOf('<div id="graphScroll"'));
assert.deepStrictEqual(Array.from(paletteMarkup.matchAll(/data-node-type="([^"]+)"/g),x=>x[1]),["emitter","auxiliary","particle","force","transform"]);
assert.ok(html.indexOf("js/node_palette.js")<html.indexOf("js/panel.js"));
assert.match(html,/aria-controls="nodePaletteItems" aria-expanded="true"/);
console.log("node_palette_tests passed: five kinds, default expansion, collapse, drop, cancellation, target guards and no movement writes");
