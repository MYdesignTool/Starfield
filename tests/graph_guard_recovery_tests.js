"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const source=fs.readFileSync(path.join(__dirname,"../tools/Recover-GraphGuard.jsx"),"utf8");
require("internal/deps/acorn/acorn/dist/acorn").parse(source,{ecmaVersion:3});
let checks=1;
const clone=v=>JSON.parse(JSON.stringify(v));
const baseline={version:2,selectedTarget:"p0-c1-l70",selectedNames:{comp:"Comp 1",layer:"Medium Blue Solid 4"},
    beforeSnapshot:[{guard:1,commit:790333,receipt:-790333,revision:44,checksumHigh:10732,checksumLow:58207}],
    snapshot:{ok:true,initialized:true,revision:44,nativeNodes:[{id:"node",type:"motion",position:{x:1,y:2}}],renderer:{id:"output"}}};
function run(options={}){
    const trace=[],alerts=[],writes=[];let result,undo=0,calls=0,selected=true;
    const names={"Panel Graph Sync Guard":1,"Commit Graph Edit":790333,"Graph Edit Receipt":-790333,
        "Graph Revision":44,"Graph Checksum High":10732,"Graph Checksum Low":58207};
    Object.assign(names,options.values || {});
    const original={id:70,name:options.foreignName?"Different layer":"Medium Blue Solid 4",enabled:true,selected:true,values:names,
        nodeGuard:options.nodeGuard || 0,snapshot:clone(baseline.snapshot)};
    if(options.changedNodes)original.snapshot.nativeNodes[0].position.x++;
    function CompItem(){}const comp=Object.assign(new CompItem(),{id:1,name:"Comp 1",layer:i=>layers[i-1]});
    const layers=[original];Object.defineProperty(comp,"numLayers",{get:()=>layers.length});
    function wire(layer){
        layer.containingComp=comp;
        layer.property=name=>{
            assert.equal(name,"ADBE Effect Parade");checks++;
            return {numProperties:2,property:i=>({matchName:i===1?"org.starfieldfx.particle":"org.starfieldfx.node.motion",
                property:key=>({valueAtTime(t,pre){assert.equal(t,0);assert.equal(pre,true);checks+=2;
                    return i===1?layer.values[key]:layer.nodeGuard;},
                    setValue(value){assert.equal(i,1);checks++;writes.push({id:layer.id,key,value});trace.push(key+":"+value);
                        if(layer===original && key==="Panel Graph Sync Guard" && value===0 && options.discardClear)return;
                        layer.values[key]=value;
                        if(layer!==original && options.backupGuardError)throw Error("backup guard failure");
                        if(layer===original && key==="Commit Graph Edit" && options.commitSetterError)throw Error("partial nonce write");
                    }})})};
        };
        layer.duplicate=()=>{
            trace.push("duplicate");assert.equal(undo,1);checks++;
            const backup={id:71,name:layer.name,enabled:layer.enabled,selected:false,values:clone(layer.values),nodeGuard:layer.nodeGuard,snapshot:clone(layer.snapshot)};
            wire(backup);layers.push(backup);
            if(options.changedDuringBackup)original.values["Graph Revision"]++;
            return backup;
        };
    }
    wire(original);
    if(options.existingBackup)layers.push({name:"Starfield recovery backup - Medium Blue Solid 4"});
    const project={rootFolder:{id:0},activeItem:comp,numItems:1,item:()=>comp};
    const app={version:"23.5x52",project,
        beginUndoGroup(label){assert.equal(label,"Starfield: recover verified failed graph");checks++;undo++;trace.push("begin");},
        endUndoGroup(){undo--;trace.push("end");if(options.endRelock)original.values["Panel Graph Sync Guard"]=1;if(options.endError)throw Error("undo close error");},
        findMenuCommandId(name){assert.equal(name,"Starfield Commit Node Graph");checks++;return options.noCommand?0:91;},
        executeCommand(id){assert.equal(id,91);assert.equal(undo,1);assert.equal(layers[1].enabled,false);
            assert.equal(layers[1].values["Panel Graph Sync Guard"],2);checks+=4;trace.push("command");calls++;
            if(options.commandError)throw Error("native command failure");
            original.values["Graph Edit Receipt"]=options.rejected?-790334:790334;
            if(!options.noRevision)original.values["Graph Revision"]++;
            if(options.changedAfterPublish)original.snapshot.nativeNodes[0].position.x++;
        }};
    const host={SFLD_ready:()=>options.wrongGateway?"old":"org.starfieldfx.panel/1/native-presets-99",
        __SFLD_modelGraphTransactionV1:options.activeSession?{state:"committing"}:null,
        SFLD_getGraphSnapshot:wire=>{const req=JSON.parse(wire);assert.equal(req.target.token,"p0-c1-l70");assert.equal(req.pinTarget,true);checks+=2;
            return JSON.stringify({ok:!options.snapshotFailure,snapshot:original.snapshot});}};
    function File(name){this.parent={parent:{fsName:"D:/checkout"}};this.fsName=name;
        this.open=mode=>{assert.ok((mode==="r" && name==="D:/checkout/artifacts/m3-20-guard-recovery-20261017/baseline.json") ||
            (mode==="w" && name==="D:/checkout/artifacts/m3-20-guard-recovery-20261017/recovery-result.json"));checks++;return true;};
        this.read=()=>JSON.stringify(baseline);this.close=()=>{};this.write=text=>{result=JSON.parse(text);return true;};}
    vm.runInNewContext(source,{app,$:{global:host,fileName:"D:/checkout/tools/Recover-GraphGuard.jsx"},File,CompItem,
        alert:text=>alerts.push(text),prompt(){throw Error("Unexpected save failure");}});
    assert.equal(undo,0);assert.equal(alerts.length,1);checks+=2;
    return {result,layers,writes,trace,calls,original};
}
{
    const r=run();assert.equal(r.result.ok,true);assert.equal(r.result.finalGuard,0);assert.equal(r.result.published,true);
    assert.deepEqual(r.original.snapshot,baseline.snapshot);assert.equal(r.layers.length,2);assert.equal(r.layers[1].values["Commit Graph Edit"],790333);
    assert.equal(r.layers[1].values["Graph Edit Receipt"],-790333);assert.deepEqual(r.layers[1].snapshot,baseline.snapshot);
    assert.deepEqual(r.trace,["begin","duplicate","Panel Graph Sync Guard:2","Commit Graph Edit:790334","command","Panel Graph Sync Guard:0","end"]);checks+=9;
}
for(const options of [{foreignName:true},{activeSession:true},{wrongGateway:true},{noCommand:true},{changedNodes:true},{nodeGuard:1},
    {nodeGuard:2},{nodeGuard:3},{nodeGuard:4},{values:{"Panel Graph Sync Guard":0}},{values:{"Graph Edit Receipt":0}},
    {values:{"Commit Graph Edit":790334}},{values:{"Graph Revision":45}},{values:{"Graph Checksum Low":0}},
    {existingBackup:true},{snapshotFailure:true}]){
    const r=run(options);assert.equal(r.result.ok,false);assert.equal(r.writes.length,0);assert.equal(r.calls,0);checks+=3;
}
for(const options of [{rejected:true},{noRevision:true},{commandError:true},{commitSetterError:true},{changedAfterPublish:true},{discardClear:true}]){
    const r=run(options);assert.equal(r.result.ok,false);assert.equal(r.result.finalGuard,1);assert.equal(r.layers.length,2);
    assert.equal(r.layers[1].values["Graph Edit Receipt"],-790333);assert.equal(r.layers[1].enabled,false);checks+=5;
}
for(const options of [{backupGuardError:true},{changedDuringBackup:true}]){
    const r=run(options);assert.equal(r.result.ok,false);assert.equal(r.calls,0);assert.equal(r.writes.filter(w=>w.id===70).length,0);checks+=3;
}
for(const options of [{endRelock:true},{endError:true}]){const r=run(options);assert.equal(r.result.ok,false);assert.equal(r.result.published,true);checks+=2;}
console.log(`graph_guard_recovery_tests: ${checks} checks passed; actual ES3 recovery, scoped refusal, backup, publish/verify/release, failure retention and undo closure; no AE execution`);
