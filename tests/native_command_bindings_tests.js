"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const source=fs.readFileSync(path.join(process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"../web_panel"),"jsx/starfield_gateway.jsx"),"utf8");
const host={},lookups=[],context=vm.createContext({$:{global:host},app:{findMenuCommandId(name){lookups.push(name);return 19;}}});
vm.runInContext(source,context);
assert.equal(host.SFLD_nativeCommandId("Starfield Commit Node Graph"),19,"older hosts retain named-command fallback");
const names=["Starfield Reveal Selected Effect","Starfield Prepare Model Asset Export","Starfield Apply Model Graph Transaction",
    "Starfield Choose Preset File","Starfield Commit Node Graph","Starfield Repair Selected Graph"];
host.__SFLD_nativeCommandsV1={version:1,commands:Object.fromEntries(names.map((name,index)=>[name,7001+index]))};
for(const [index,name] of names.entries())assert.equal(host.SFLD_nativeCommandId(name),7001+index);
assert.equal(host.SFLD_nativeCommandId("unregistered"),0);
for(const value of [0,-1,1.5,NaN,Infinity,"7005",2147483648,null,undefined]) {
    host.__SFLD_nativeCommandsV1.commands[names[4]]=value;
    assert.equal(host.SFLD_nativeCommandId(names[4]),0,"invalid binding cannot fall back to a different menu command");
}
for(const value of [{version:2,commands:{[names[4]]:7005}},{version:1},{commands:{}}]) {
    host.__SFLD_nativeCommandsV1=value;assert.equal(host.SFLD_nativeCommandId(names[4]),0);
}
host.__SFLD_nativeCommandsV1={version:1,commands:{[names[4]]:8105}};
assert.equal(host.SFLD_nativeCommandId(names[4]),8105,"cached gateway reads the current map on every invocation");
assert.deepEqual(lookups,[names[4]],"bound commands never query the visible Window menu");
console.log("Session native command resolution: all six IDs, legacy fallback, refreshed bindings and corrupt/missing binding rejection passed.");
