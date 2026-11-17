"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const code=fs.readFileSync(path.join(__dirname,"../web_panel/js/webview_host.js"),"utf8");
function fixture(options={}) {
    const events={},messages=[],webEvents={};
    const root={chrome:{webview:{postMessage(value){messages.push(value);if(options.fail)throw Error("closed");},addEventListener(name,fn){webEvents[name]=fn;}}},
        __starfieldWebViewConfig:{root:"D:/中文 project/web_panel",nativeGraphDelete:options.nativeDelete===true},addEventListener(name,fn){events[name]=fn;}};
    if(options.timers){let serial=0;root.setTimeout=(fn,delay)=>{if(options.delays)options.delays.push(delay);options.timers.set(++serial,fn);return serial;};root.clearTimeout=id=>options.timers.delete(id);}
    if(options.cep)root.__adobe_cep__=options.cep;
    if(options.noConfig)delete root.__starfieldWebViewConfig;
    if(options.noWeb)delete root.chrome;
    const delivered=[],local=[];
    if(options.graph){root.document={getElementById(){return {};},hasFocus(){return options.focus!==false;},activeElement:{tagName:"DIV"},execCommand(command){local.push(command);}};
        root.CustomEvent=function(type){this.type=type;};root.dispatchEvent=event=>delivered.push(event.type);}
    vm.runInNewContext(code,{window:root});
    return {root,events,messages,delivered,local,native(data){webEvents.message({data});},reply(id,result,channel="org.starfieldfx.webview2/1"){webEvents.message({data:{channel,id,result}});}};
}
const original={evalScript(){}};
{
    const timers=new Map(),delays=[],f=fixture({graph:true,nativeDelete:true,timers,delays}),id="0102030405060708090a0b0c0d0e0f10";let result,calls=0;
    f.root.StarfieldNativeGraph.patch("p0-c88-l111",7,"00010002",{removed:[],rows:[{source:id,expected:[],desired:[{target:id,id}]}]},r=>{result=r;calls++;},"");
    const sent=f.messages.at(-1);assert.equal(timers.size,1);assert.equal(delays[0],30000,"known5–10s native calls must not be misclassified by the old8s script timeout");const expire=[...timers.values()][0];timers.clear();expire();
    assert.equal(result.error.code,"graph_edit_outcome_unknown");f.reply(sent.id,'{"ok":true,"revision":8}');assert.equal(calls,1,"late ACK cannot resume an expired request");
    f.root.StarfieldNativeGraph.diagnostics("p0-c88-l111",7,"00010002","",r=>{result=r;});f.reply(f.messages.at(-1).id,'{"ok":true,"issues":[]}');assert.equal(timers.size,0,"positive ACK clears its deadline");
    f.root.StarfieldNativeGraph.diagnostics("p0-c88-l111",7,"00010002","",r=>{result=r;});f.events.pagehide();assert.equal(timers.size,0,"page teardown releases every native deadline");assert.equal(result.error.code,"panel_closed");
}
{
    const history=fixture({graph:true});let deferred,completed;
    history.root.StarfieldGraphHistory=perform=>{deferred=perform;};
    const before=history.messages.filter(m=>m.kind==="undo").length;
    history.events.keydown({key:"z",ctrlKey:true,target:{tagName:"DIV"},preventDefault(){}});
    assert.equal(history.messages.filter(m=>m.kind==="undo").length,before,"panel history barrier runs before native dispatch");
    deferred(result=>{completed=result;});const sent=history.messages.at(-1);assert.equal(sent.kind,"undo");
    history.reply(sent.id,"ok");assert.equal(completed.ok,true,"existing native126 ACK completes history barrier");
    history.events.keydown({key:"y",ctrlKey:true,target:{tagName:"DIV"},preventDefault(){}});
    deferred(result=>{completed=result;});history.reply(history.messages.at(-1).id,'{"ok":false,"error":{"code":"busy"}}');
    assert.equal(completed.error.code,"busy","native history failures are preserved");
}
{
    const n=fixture({graph:true,nativeDelete:true}),id="0102030405060708090a0b0c0d0e0f10";let result;
    n.root.StarfieldNativeGraph.deleteNodes("p0-c88-l111",7,"00010002",[id],r=>{result=r;},"");
    const sent=n.messages.at(-1);assert.equal(sent.kind,"nativeGraphDelete");assert.equal(sent.script,"p0-c88-l111|7|00010002||"+id);
    n.reply(sent.id,'{"ok":true,"committed":true,"revision":8}');assert.equal(result.revision,8);
    n.root.StarfieldNativeGraph.patch("p0-c88-l111",7,"00010002",{removed:[],rows:[{source:id,expected:[],desired:[{target:id,id:id}]}]},r=>{result=r;},"");
    const patch=n.messages.at(-1);assert.equal(patch.kind,"nativeGraphPatch");assert.equal(patch.script,"p0-c88-l111|7|00010002|||"+id+":>"+id+id);
    n.reply(patch.id,'{"ok":true,"revision":8}');assert.equal(result.revision,8);
    n.root.StarfieldNativeGraph.diagnostics("p0-c88-l111",7,"00010002","",r=>{result=r;});
    const diagnostic=n.messages.at(-1);assert.equal(diagnostic.kind,"nativeGraphDiagnostics");assert.equal(diagnostic.script,"p0-c88-l111|7|00010002||");
    n.reply(diagnostic.id,'{"ok":true,"issues":[]}');assert.equal(result.issues.length,0);
    const before=n.messages.length;
    n.root.StarfieldNativeGraph.deleteNodes("p0-c88-l111",7,"00010002",[id,id],r=>{result=r;},"");
    assert.equal(n.messages.length,before);assert.equal(result.error.code,"invalid_native_delete");
    n.root.StarfieldNativeGraph.deleteNodes("p0-c88-l111",7,"00010002",[id],r=>{result=r;},"");n.reply(n.messages.at(-1).id,"broken");
    assert.equal(result.error.code,"native_delete_outcome_unknown");
    n.native({channel:"org.starfieldfx.webview2/1",event:"nativeGraphEdited",result:{ok:true}});
    assert.equal(n.delivered.at(-1),"starfield:nativeGraphEdited");
    assert.equal(fixture({graph:true}).root.StarfieldNativeGraph,undefined,"old native pairing retains its existing transport");
    assert.equal(fixture({nativeDelete:true}).root.StarfieldNativeGraph,undefined,"preset view has no native node delete entry");
}
assert.equal(fixture({cep:original}).root.__adobe_cep__,original,"existing CEP transport unchanged");
assert.equal(fixture({noConfig:true}).root.__adobe_cep__,undefined);
assert.equal(fixture({noWeb:true}).root.__adobe_cep__,undefined);
const f=fixture(),bridge=f.root.__adobe_cep__;
const focusBridge=fixture();
focusBridge.root.__adobe_cep__.evalScript("select",()=>{},true);
assert.equal(focusBridge.messages[0].kind,"evalScriptKeepGraphFocus");
assert.equal(focusBridge.messages[0].script,"select");
focusBridge.root.__adobe_cep__.evalScript("ordinary",()=>{},"true");
assert.equal(focusBridge.messages[1].kind,"evalScript","only boolean true opts into focus ownership");
assert.equal(bridge.getSystemPath("extension"),"D:/中文 project/web_panel");
assert.equal(bridge.getSystemPath("userData"),"");
const received=[];
bridge.evalScript('JSON.stringify({name:"灯光\\\"",unicode:"中文"})',value=>received.push(value));
bridge.evalScript("second",value=>received.push(value));
assert.equal(f.messages[0].script,'JSON.stringify({name:"灯光\\\"",unicode:"中文"})');
f.reply("wv-1","wrong","untrusted");f.reply("missing","wrong");assert.equal(received.length,0);
f.reply("wv-2",'{"ok":true,"second":true}');f.reply("wv-1",'{"ok":true,"name":"中文"}');
f.reply("wv-1","duplicate");assert.deepEqual(received,['{"ok":true,"second":true}','{"ok":true,"name":"中文"}']);
bridge.requestOpenExtension("third.party");assert.equal(f.messages.length,2);
bridge.requestOpenExtension("org.starfieldfx.panel.presets");assert.equal(f.messages[2].kind,"openPresets");
f.reply("wv-3",'{}');bridge.closeExtension();assert.equal(f.messages[3].kind,"close");f.reply("wv-4",'{}');
let prevented=0;
f.events.keydown({key:"z",ctrlKey:true,target:{tagName:"INPUT"},preventDefault(){prevented++;}});
assert.equal(prevented,0);
f.events.keydown({key:"z",ctrlKey:true,target:{tagName:"DIV"},preventDefault(){prevented++;}});
assert.equal(f.messages[4].kind,"undo");f.reply("wv-5",'{}');
f.events.keydown({key:"z",ctrlKey:true,shiftKey:true,target:{tagName:"DIV"},preventDefault(){prevented++;}});
assert.equal(f.messages[5].kind,"redo");f.reply("wv-6",'{}');
f.events.keydown({key:"y",ctrlKey:true,repeat:true,target:{tagName:"DIV"},preventDefault(){}});
assert.equal(f.messages.length,6,"held shortcut never repeats a project write");
const busy=fixture();let rejected;
for(let i=0;i<64;i++)busy.root.__adobe_cep__.evalScript("x",()=>{});
busy.root.__adobe_cep__.evalScript("overflow",value=>{rejected=JSON.parse(value);});
assert.equal(busy.messages.length,64);assert.equal(rejected.error.code,"webview_unavailable");
busy.reply("wv-1","done");busy.root.__adobe_cep__.evalScript("space freed",()=>{});assert.equal(busy.messages.length,65);
const maximum=fixture();maximum.root.__adobe_cep__.evalScript("x".repeat(1048576),()=>{});assert.equal(maximum.messages.length,1,"nested old request wrapper retains headroom");
const oversized=fixture();oversized.root.__adobe_cep__.evalScript("x".repeat(1048577),value=>{rejected=JSON.parse(value);});assert.equal(oversized.messages.length,0);
const failed=fixture({fail:true});failed.root.__adobe_cep__.evalScript("x",value=>{rejected=JSON.parse(value);});assert.equal(rejected.error.code,"webview_transport_failed");
const versioned=fixture();let nativeError;
versioned.root.__adobe_cep__.evalScript("/* native-presets-99 */ SFLD_getPanelState()",value=>{nativeError=JSON.parse(value);});
versioned.reply("wv-1",'{"ok":false,"error":{"code":"host_timeout","message":"Expired without executing"}}');
assert.equal(nativeError.protocol,"org.starfieldfx.panel");assert.equal(nativeError.version,1);assert.equal(nativeError.gatewayBuild,"native-presets-99");assert.equal(nativeError.error.code,"host_timeout","native failures preserve their typed error instead of masquerading as version mismatches");
let closed;
bridge.evalScript("awaiting",value=>{closed=JSON.parse(value);});f.events.pagehide();assert.equal(closed.error.code,"panel_closed");
bridge.evalScript("after close",value=>{rejected=JSON.parse(value);});assert.equal(rejected.error.code,"webview_unavailable");
for(const html of ["index.html","presets.html"]){const text=fs.readFileSync(path.join(__dirname,"../web_panel",html),"utf8");assert(text.indexOf("js/webview_host.js")<text.indexOf("js/graph_codec.js"));}
{
    const g=fixture({graph:true});let serial=0;
    function packet(extra={}){const parts=g.messages.filter(m=>m.kind==="graphInputState").at(-1).script.split("|");
        return {channel:"org.starfieldfx.webview2/1",event:"graphDelete",page:parts[0],inputEpoch:Number(parts[1]),serial:++serial,backspace:false,expires:Date.now()+250,...extra};}
    const initial=packet();g.native(initial);assert.deepEqual(g.delivered,["starfield:nativeDelete"]);
    g.native(initial);assert.equal(g.delivered.length,1,"duplicate native serial never submits twice");
    let stopped=0;
    g.events.keydown({key:"Delete",target:{tagName:"DIV"},preventDefault(){stopped++;},stopImmediatePropagation(){stopped++;}});
    assert.equal(stopped,2,"late DOM keydown of native-owned key is consumed");
    g.events.keyup({key:"Delete"});
    const before=packet();g.events.pointerdown({});g.native(before);assert.equal(g.delivered.length,1,"input context changes invalidate a delayed notification");
    const beforeCopy=packet();g.events.keydown({key:"d",ctrlKey:true,target:{tagName:"DIV"}});g.native(beforeCopy);
    assert.equal(g.delivered.length,1,"copy/select shortcuts invalidate a pending Delete before they alter selection");
    for(const extra of [{page:"other-page"},{inputEpoch:0},{expires:Date.now()-1},{expires:Date.now()+10000},{serial:NaN},{backspace:"true"},{channel:"foreign"}])g.native(packet(extra));
    assert.equal(g.delivered.length,1,"untrusted/malformed/expired notifications drop");
    g.events.keydown({key:"Delete",target:{tagName:"DIV"}});g.native(packet());assert.equal(g.delivered.length,1,"DOM-owned delete cannot replay via native notification");
    g.events.keyup({key:"Delete"});
    g.root.document.activeElement={tagName:"INPUT"};g.native(packet());assert.deepEqual(g.local,["forwardDelete"]);
    g.events.keyup({key:"Delete"});g.native(packet({backspace:true}));assert.deepEqual(g.local,["forwardDelete","delete"]);
    g.events.keyup({key:"Backspace"});g.root.document.activeElement={tagName:"INPUT",readOnly:true};g.native(packet());assert.equal(g.local.length,2);
    g.events.keyup({key:"Delete"});g.root.document.activeElement={tagName:"SELECT"};g.native(packet());assert.equal(g.delivered.length,1);
    g.events.pagehide();g.native(packet());assert.equal(g.delivered.length,1,"closed document never executes queued input");
}
console.log("WebView2 adapter: existing CEP, Unicode, ordering, duplicate replies, queue bounds, errors, teardown and shortcuts passed.");
