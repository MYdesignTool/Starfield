"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const source=fs.readFileSync(path.join(__dirname,"../tools/Rebuild-GraphCopy.jsx"),"utf8");
require("internal/deps/acorn/acorn/dist/acorn").parse(source,{ecmaVersion:3});
const clone=v=>JSON.parse(JSON.stringify(v)),main="org.starfieldfx.particle";
const controls=["Max Particles","Layout Output X","Layout Output Y",main+"-0921","Time (Seconds)","Preview","Particle chance","Time Sampling","Acceleration",
    ...Array.from({length:8},(_,i)=>main+"-"+(1641+i))];
const authored=Object.fromEntries(controls.map((x,i)=>[x,i+1]));
const baseline={version:2,selectedTarget:"p0-c1-l70",selectedNames:{comp:"Comp 1",layer:"Medium Blue Solid 4"},
    snapshot:{initialized:true,checksum:"29ece35f",nativeNodes:[{id:"emitter",type:"emitter"},{id:"particle",type:"particle"},{id:"motion",type:"motion"}],renderer:authored}};
let checks=1;
function run(opt={}){
    let result,calls=0,undo=0,generation=0;const mutations=[],alerts=[];
    function CompItem(){}const comp=new CompItem();Object.assign(comp,{id:1,name:"Comp 1",layer:i=>layers[i-1]});
    const original={id:70,name:opt.foreignName?"Changed":"Medium Blue Solid 4",enabled:true,selected:true,containingComp:comp};
    const originalValues={...authored,"Panel Graph Sync Guard":1,"Commit Graph Edit":790334,"Graph Edit Receipt":-790334,"Graph Revision":44,
        "Graph Checksum High":10732,"Graph Checksum Low":58207,"Node Effects Ready":1,...opt.values};
    const originalNodeData=clone(baseline.snapshot.nativeNodes);
    original.effects=[{match:main,name:"Custom main",enabled:true,values:originalValues,legacy:true},
        ...originalNodeData.map(n=>({match:"org.starfieldfx.node."+n.type,name:n.type,enabled:true,values:{"Panel Sync Guard":opt.nodeGuard||0},
            // Actual host duplicate owns this data; script must never rewrite it.
            author:{id:n.id,keys:[{time:0,value:7},{time:3,value:42}],expression:"value + 1"}}))];
    const frozen=clone(original);delete frozen.containingComp;
    const layers=[original];Object.defineProperty(comp,"numLayers",{get:()=>layers.length});
    if(opt.existingCopy)layers.push({id:72,name:"Starfield rebuilt - Medium Blue Solid 4",containingComp:comp,effects:[]});
    if(opt.pending)layers.push({id:73,name:"Other",containingComp:comp,effects:[{match:main,name:"Other main",enabled:true,
        values:{"Panel Graph Sync Guard":1,"Commit Graph Edit":12,"Graph Edit Receipt":0}}]});
    function mutate(target,action){assert.notEqual(target,original,"Original must never be written");assert.equal(undo,1);checks+=2;mutations.push(action);}
    function wire(target){
        target.property=name=>{
            assert.equal(name,"ADBE Effect Parade");checks++;
            return {get numProperties(){return target.effects.length;},property(index){const effect=target.effects[index-1],gen=generation;
                function fresh(){assert.equal(gen,generation,"Effect wrapper must be reacquired after a parade edit");checks++;}
                return {get matchName(){fresh();return effect.match;},get name(){fresh();return effect.name;},set name(v){fresh();mutate(target,"name");effect.name=v;},
                    get propertyIndex(){fresh();return target.effects.indexOf(effect)+1;},get enabled(){fresh();return effect.enabled;},set enabled(v){fresh();mutate(target,"enabled");effect.enabled=v;},
                    property(key){fresh();if(opt.missingControl && key===controls[0])return null;
                        return {numKeys:target===original&&key===controls[0]&&opt.animated?2:0,
                            canSetExpression:true,expressionEnabled:!!(target===original&&key===controls[0]&&opt.expression),
                            expression:target===original&&key===controls[0]&&opt.expression?"value+1":"",
                            valueAtTime(t,pre){fresh();assert.equal(t,0);assert.equal(pre,true);checks+=2;return effect.values[key];},
                            setValue(v){fresh();mutate(target,key);
                                if(opt.discardClear&&key==="Panel Graph Sync Guard"&&v===0)return;
                                effect.values[key]=v;
                            }};},
                    remove(){fresh();mutate(target,"remove");target.effects.splice(target.effects.indexOf(effect),1);generation++;if(opt.removeThrows)throw Error("partial remove");},
                    moveTo(index){fresh();mutate(target,"move");target.effects.splice(target.effects.indexOf(effect),1);target.effects.splice(index-1,0,effect);generation++;}
                };},addProperty(match){mutate(target,"add");assert.equal(match,main);checks++;
                    if(opt.addThrows)throw Error("add failed");
                    target.effects.push({match:main,name:"Starfield",enabled:true,legacy:false,values:{"Panel Graph Sync Guard":0,"Graph Revision":0,"Graph Edit Receipt":0,"Commit Graph Edit":0}});generation++;
                }};
        };
        target.duplicate=()=>{assert.equal(target,original);assert.equal(undo,1);checks+=2;
            const copy={id:74,name:target.name,enabled:target.enabled,selected:false,containingComp:comp,effects:clone(target.effects)};wire(copy);layers.push(copy);generation++;
            if(opt.changedDuringCopy)originalValues["Graph Revision"]++;
            return copy;
        };
    }
    layers.forEach(wire);
    const app={version:"23.5x52",project:{rootFolder:{id:0},numItems:1,item:()=>comp},
        beginUndoGroup(label){assert.equal(label,"Starfield: rebuild disabled graph copy");checks++;undo++;},endUndoGroup(){undo--;if(opt.undoError)throw Error("undo close");},
        findMenuCommandId:()=>opt.noCommand?0:91,executeCommand(id){assert.equal(id,91);assert.equal(undo,1);checks+=2;calls++;
            const copy=layers.find(l=>l.id===74),fx=copy.effects.find(e=>e.match===main);
            assert.equal(copy.enabled,false);assert.equal(fx.legacy,false,"Broken old graph must not be republished");
            assert.equal(fx.values["Panel Graph Sync Guard"],1);assert.equal(fx.values["Commit Graph Edit"],790335);
            assert.equal(copy.effects[0],fx);assert.equal(fx.name,"Custom main");checks+=6;
            if(opt.commandError)throw Error("native failure");
            fx.values["Graph Edit Receipt"]=opt.rejected?-790335:790335;fx.values["Graph Revision"]=opt.noRevision?0:1;
        }};
    const host={SFLD_ready:()=>opt.wrongGateway?"old":"org.starfieldfx.panel/1/native-presets-99",
        __SFLD_modelGraphTransactionV1:opt.session?{state:"committing"}:null,
        SFLD_getGraphSnapshot:wire=>{const req=JSON.parse(wire);assert.equal(req.operation,"getGraphSnapshot");assert.equal(req.pinTarget,true);checks+=2;
            const target=req.target.token==="p0-c1-l70"?original:layers.find(l=>"p0-c1-l"+l.id===req.target.token);assert.ok(target);checks++;
            const record=clone(baseline.snapshot);if(opt.changedNodes || (opt.changedCopy && target!==original))record.nativeNodes.pop();
            return JSON.stringify({ok:!opt.snapshotError,snapshot:record});}};
    function File(name){this.parent={parent:{fsName:"D:/checkout"}};this.open=mode=>{assert.ok((mode==="r"&&name.endsWith("/baseline.json")) ||
        (mode==="w"&&name==="D:/checkout/artifacts/m3-20-guard-recovery-20261017/rebuild-copy-result.json"));checks++;return true;};
        this.read=()=>JSON.stringify(baseline);this.close=()=>{};this.write=text=>{result=JSON.parse(text);return true;};}
    vm.runInNewContext(source,{app,$:{global:host,fileName:"D:/checkout/tools/Rebuild-GraphCopy.jsx"},File,CompItem,
        alert:message=>alerts.push(message),prompt(){throw Error("Unexpected report failure");}});
    const after={...original};delete after.containingComp;delete after.property;delete after.duplicate;
    if(!opt.changedDuringCopy){assert.deepEqual(after,frozen,"Original, native animation and rejected graph are untouched");checks++;}
    assert.equal(undo,0);assert.equal(alerts.length,1);checks+=2;
    const copy=layers.find(l=>l.id===74);
    if(copy){assert.equal(copy.enabled,false);assert.deepEqual(copy.effects.filter(e=>e.match!==main),original.effects.filter(e=>e.match!==main),"Duplicated native animation remains exact");checks+=2;}
    return {result,calls,mutations,copy};
}
{
    const r=run();assert.equal(r.result.ok,true);assert.equal(r.result.phase,"complete");assert.equal(r.calls,1);
    assert.equal(r.copy.effects[0].values["Panel Graph Sync Guard"],0);
    for(const key of controls){assert.equal(r.copy.effects[0].values[key],authored[key]);checks++;}checks+=4;
}
for(const opt of [{foreignName:true},{existingCopy:true},{pending:true},{session:true},{wrongGateway:true},{animated:true},{expression:true},{missingControl:true},
    {noCommand:true},{changedNodes:true},{snapshotError:true},{nodeGuard:1},{nodeGuard:2},{nodeGuard:3},{nodeGuard:4},
    {values:{"Panel Graph Sync Guard":0}},{values:{"Graph Edit Receipt":0}},{values:{"Graph Checksum Low":0}},{values:{"Graph Revision":45}}]){
    const r=run(opt);assert.equal(r.result.ok,false);assert.equal(r.mutations.length,0);assert.equal(r.calls,0);assert.equal(r.copy,undefined);checks+=4;
}
for(const opt of [{changedDuringCopy:true},{removeThrows:true},{addThrows:true},{commandError:true},{rejected:true},{noRevision:true},{changedCopy:true},{discardClear:true},{undoError:true}]){
    const r=run(opt);assert.equal(r.result.ok,false);assert.ok(r.copy);assert.equal(r.copy.enabled,false);checks+=3;
}
console.log(`graph_copy_rebuild_tests: ${checks} checks passed; actual ES3 tool, original write prohibition, stale references, disabled copy, static Output, native animation preservation and failure injection; no AE execution`);
