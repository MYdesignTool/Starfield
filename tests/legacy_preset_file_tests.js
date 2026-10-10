"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
if(!process.env.STARFIELD_PANEL_ROOT)throw new Error("Set STARFIELD_PANEL_ROOT to the isolated legacy-modal candidate.");
const panel=process.env.STARFIELD_PANEL_ROOT;
const source=fs.readFileSync(path.join(panel,"jsx/starfield_gateway.jsx"),"utf8");
let sideEffects=0,checks=0;
function forbidden(){sideEffects++;throw new Error("Legacy file entry must not perform IO or open a modal.");}
const global={SFLD_readPresetFile:forbidden,SFLD_writePresetFile:forbidden};
function File(){return forbidden();}File.openDialog=File.saveDialog=forbidden;
const context=vm.createContext({$:{global},app:{executeCommand:forbidden},File,confirm:forbidden});
const eq=(a,b)=>{checks++;assert.equal(a,b);};
assert.doesNotMatch(source,/File\.(?:openDialog|saveDialog)|\b(?:alert|confirm|prompt)\s*\(/);checks++;
const request=(operation,fields={})=>JSON.stringify({protocol:"org.starfieldfx.panel",version:1,operation,...fields});
for(let reload=0;reload<2;reload++){
    vm.runInContext(source,context);
    for(const operation of ["readPresetFile","writePresetFile"]){
        for(const fields of [{},{text:'{"format":"org.starfieldfx.preset"}'},{text:'{"format":"invalid"}'},{text:42}]){
            const response=JSON.parse(global["SFLD_"+operation](request(operation,fields)));
            eq(response.ok,false);eq(response.error.code,"preset_file_protocol_changed");
            assert.match(response.error.message,/Close and reopen Starfield Presets/);checks++;
        }
        for(const invalid of ["{",request("getState"),request(operation,{text:"x".repeat(262145)}),
            JSON.stringify({protocol:"other",version:1,operation}),request(operation,{gatewayBuild:"obsolete"})]){
            const response=JSON.parse(global["SFLD_"+operation](invalid));
            eq(response.ok,false);eq(response.error.code,"invalid_request");
        }
    }
}
eq(sideEffects,0);
console.log(`legacy_preset_file_tests: ${checks} checks passed; actual isolated gateway, no modal or file IO`);
