"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"../web_panel");
const gateway=fs.readFileSync(path.join(root,"jsx/starfield_gateway.jsx"),"utf8");
let checks=0;
for(const globalJSON of [undefined,{parse(){throw Error("foreign parser");},stringify(){throw Error("foreign serializer");}}]){
    const owner={},context=vm.createContext({JSON:globalJSON,$:{global:owner},app:{project:null}});
    vm.runInContext(gateway,context);
    const codec=owner.__SFLD_modelTransactionAPI.json;
    assert.equal(owner.__SFLD_presetFileAPI.json,codec);checks++;
    const response=JSON.parse(owner.SFLD_getPanelState(JSON.stringify({protocol:"org.starfieldfx.panel",version:1,
        operation:"getPanelState",requestId:"startup",target:{},changes:[]})));
    assert.equal(response.ok,false);assert.equal(response.error.code,"no_project");checks+=2;
    assert.equal(context.JSON,globalJSON,"gateway never creates or changes global JSON");checks++;
    const values=[null,true,false,0,-0,1e308,-1e-200,"中文 😀 \"\\\b\f\n\r\t\u0000\u2028\u2029",[],{},
        {array:[undefined,null,NaN,Infinity],missing:undefined,text:"safe",constructor:"data"}];
    let seed=4129;const random=()=>{seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296;};
    for(let i=0;i<1000;i++)values.push({n:(random()-.5)*Math.pow(10,Math.floor(random()*600)-300),
        text:String.fromCharCode(Math.floor(random()*65536))+"\"\\\n",list:[i,random()>.5,null],nested:{value:i}});
    for(const value of values){
        const expected=JSON.parse(JSON.stringify(value)),encoded=codec.stringify(value);
        assert.deepEqual(JSON.parse(encoded),expected);
        assert.deepEqual(JSON.parse(Buffer.from(encoded,"utf8").toString("utf8")),expected,"Unicode also survives the native UTF-8 transport");
        assert.deepEqual(JSON.parse(JSON.stringify(codec.parse(JSON.stringify(value)))),expected);checks+=3;
    }
    for(const invalid of [""," ","undefined","NaN","Infinity","01","-01","1.","1e","+1",".1","[1,]","[,1]",
        '{"a":1,}',"{a:1}",'{"a" 1}',"true false",'"\u0000"','"\\x20"','"\\u12xy"','"unfinished',
        '{"__proto__":{"polluted":true}}','({"pwned":1})','1; this.pwned=1']){
        assert.throws(()=>codec.parse(invalid),undefined,invalid);checks++;
    }
    assert.equal(context.pwned,undefined);assert.equal({}.polluted,undefined);checks+=2;
    assert.equal(codec.parse("1e999"),Infinity);assert.ok(Object.is(codec.parse("-0"),-0));checks+=2;
    assert.throws(()=>codec.parse("[".repeat(66)+"0"+"]".repeat(66)));checks++;
    const cyclic={};cyclic.loop=cyclic;assert.throws(()=>codec.stringify(cyclic));checks++;
    const large="a".repeat(1024*1024)+"\n中文";
    assert.equal(codec.parse(codec.stringify(large)),large);checks++;
}
// Run each real browser call() and the generated AE script in a JSON-free engine.
for(const page of ["panel","preset_manager"]){
    const source=fs.readFileSync(path.join(root,"js/"+page+".js"),"utf8"),start=source.indexOf("function call(");
    const end=source.indexOf(page==="panel"?"\n    function showError":"\n    var client",start);
    const engine={JSON:undefined,app:{project:null},File:function(file){this.path=file;},$:{global:null}};
    engine.$.global=engine;const context=vm.createContext(engine);let loads=0;
    engine.$.evalFile=()=>{loads++;vm.runInContext(gateway,context);};
    const build=/var GATEWAY_BUILD = "([^"]+)"/.exec(gateway)[1],timers={setTimeout:()=>1,clearTimeout:()=>{}};
    const scope=vm.createContext({JSON,Object,window:timers,setTimeout:timers.setTimeout,clearTimeout:timers.clearTimeout,
        GATEWAY_BUILD:build,GATEWAY_READY_TOKEN:"org.starfieldfx.panel/1/"+build,gatewayBuild:build,readyToken:"org.starfieldfx.panel/1/"+build,
        REQUEST_TIMEOUT_MS:15000,requestId:()=>"test",serial:0,state:{targetToken:null,pinnedTargetToken:null,revision:null},elements:{autoRefresh:{checked:true}},
        targetToken:null,extensionRoot:()=>"candidate",cep:{getSystemPath:()=>"candidate"},
        quote:JSON.stringify,literal:JSON.stringify,snippet:String,evalScript:(script,done)=>done(vm.runInContext(script,context)),host:(script,done)=>done(vm.runInContext(script,context))});
    if(page==="preset_manager")vm.runInContext(source.slice(source.indexOf("function gatewayLoadFailure"),start),scope);
    vm.runInContext(source.slice(start,end),scope);
    for(let i=0;i<2;i++){let response;scope.call("getPanelState",{},result=>response=result);
        assert.equal(response.error.code,"no_project");assert.equal(engine.JSON,undefined);checks+=2;}
    assert.equal(loads,1,"cached gateway also works without global JSON");checks++;
    engine.SFLD_ready=()=>"stale";
    const message='load failed "\\\n中文\u0000\u2028\u2029';engine.$.evalFile=()=>{throw Error(message);};
    let failed;scope.call("getPanelState",{},result=>failed=result);
    assert.equal(failed.error.code,"gateway_load_failed");assert.equal(failed.error.message,"Error: "+message);checks+=2;
    assert.equal(engine.JSON,undefined);checks++;
}
console.log(`Clean-engine JSON: ${checks} checks passed; real gateway/two page scripts, preserved loader errors, plain-data round trips, strict parsing and no global JSON mutation. No AE host qualification.`);
