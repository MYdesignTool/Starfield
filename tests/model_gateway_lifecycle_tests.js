"use strict";
// Execute the actual two page call() bodies and their generated host scripts.
// evalFile/CEP are bounded substitutes; no AE or filesystem mutation occurs.
const fs=require("node:fs"),path=require("node:path"),vm=require("node:vm"),assert=require("node:assert/strict");
const root=process.env.STARFIELD_PANEL_ROOT;if(!root)throw new Error("Set STARFIELD_PANEL_ROOT.");
const build=/var GATEWAY_BUILD = "([^"]+)"/.exec(fs.readFileSync(path.join(root,"jsx/starfield_gateway.jsx"),"utf8"))[1];
let checks=0;function eq(a,b){checks++;assert.deepEqual(a,b);}
for(const page of ["panel","preset_manager"]){
 const source=fs.readFileSync(path.join(root,"js",page+".js"),"utf8"),start=source.indexOf("function call(");
 const end=source.indexOf(page==="panel"?"\n    function showError":"\n    var client",start);
 assert.ok(start>=0 && end>start);
 let gateways=0,transports=0,serial=0;
 const engine={JSON,File:function(p){this.path=p;}};
 const reply=request=>JSON.stringify({protocol:"org.starfieldfx.panel",version:1,gatewayBuild:build,ok:true,
     operation:request.operation,bound:engine.__SFLD_modelTransactionAPI.serial});
 function installGateway(){gateways++;engine.__SFLD_modelTransactionAPI={gatewayBuild:build,serial:++serial};
     engine.SFLD_ready=()=>"org.starfieldfx.panel/1/"+build;}
 function installTransport(){transports++;const bound=engine.__SFLD_modelTransactionAPI;
     engine.SFLD_modelTransactionReady=()=>bound===engine.__SFLD_modelTransactionAPI?bound.gatewayBuild:"";
     for(const operation of ["beginModelGraphTransaction","readModelGraphTransaction"])
         engine["SFLD_"+operation]=text=>{const result=JSON.parse(reply(JSON.parse(text)));result.bound=bound.serial;return JSON.stringify(result);};}
 installGateway();installTransport();gateways=transports=0;
 // Existing entry points alone cannot certify a current transport/gateway pair.
 delete engine.SFLD_modelTransactionReady;
 engine.$={evalFile(file){if(file.path.endsWith("starfield_gateway.jsx"))installGateway();
     else if(file.path.endsWith("model_transaction_transport.jsx"))installTransport();else throw new Error("Unexpected file");}};
 const hostContext=vm.createContext(engine),execute=(script,callback)=>callback(vm.runInContext(script,hostContext));
 const timers={setTimeout:()=>1,clearTimeout:()=>{}};
 const scope=vm.createContext({JSON,Object,window:timers,setTimeout:timers.setTimeout,clearTimeout:timers.clearTimeout,
     GATEWAY_BUILD:build,GATEWAY_READY_TOKEN:"org.starfieldfx.panel/1/"+build,gatewayBuild:build,readyToken:"org.starfieldfx.panel/1/"+build,
     REQUEST_TIMEOUT_MS:15000,requestId:()=>"test",serial:0,state:{targetToken:"p0-c1-l29",pinnedTargetToken:null,revision:11},
     targetToken:"p0-c1-l29",extensionRoot:()=>"candidate",cep:{getSystemPath:()=>"candidate"},
     quote:JSON.stringify,literal:JSON.stringify,snippet:String,evalScript:execute,host:execute});
 vm.runInContext(source.slice(start,end),scope);
 function call(operation){let response;scope.call(operation,{},r=>response=r);eq(response.ok,true);
     eq(response.bound,engine.__SFLD_modelTransactionAPI.serial);return response;}
 call("beginModelGraphTransaction");eq([gateways,transports],[0,1]);
 call("readModelGraphTransaction");eq([gateways,transports],[0,1]);
 installGateway();call("readModelGraphTransaction");eq([gateways,transports],[1,2]);
 call("readModelGraphTransaction");eq([gateways,transports],[1,2]);
 engine.SFLD_ready=()=>"org.starfieldfx.panel/1/old";
 call("readModelGraphTransaction");eq([gateways,transports],[2,3]);
 call("beginModelGraphTransaction");eq([gateways,transports],[2,3]);
}
console.log(`model_gateway_lifecycle_tests: ${checks} checks passed; real page call bodies/generated scripts, simulated CEP/evalFile; AE qualification open`);
