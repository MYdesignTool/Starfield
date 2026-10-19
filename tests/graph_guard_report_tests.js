"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const source=fs.readFileSync(path.join(__dirname,"../tools/Report-GraphGuard.jsx"),"utf8");
require("internal/deps/acorn/acorn/dist/acorn").parse(source,{ecmaVersion:3});
let checks=1;
function readonly(value){return new Proxy(value,{set(){throw Error("Host write forbidden");},deleteProperty(){throw Error("Host delete forbidden");}});}
function run({selected=true,save=true,snapshot=true,throwGuard=false}={}){
    let written="",opened=0,closed=0,read=0;const alerts=[],prompts=[];
    const main=readonly({name:"Starfield",matchName:"org.starfieldfx.particle",property(name){return readonly({valueAtTime(t,pre){
        assert.equal(t,0);assert.equal(pre,true);checks+=2;
        if(name==="Panel Graph Sync Guard"){if(throwGuard)throw Error("unreadable guard");return 1;}
        if(name==="Commit Graph Edit" || name==="Graph Edit Receipt")return 123;
        return 2;
    }});}});
    const layer=readonly({id:70,name:"Old layer",property(name){assert.equal(name,"ADBE Effect Parade");checks++;return readonly({numProperties:1,property:()=>main});}});
    function CompItem(){}const comp=readonly(Object.assign(new CompItem(),{id:1,name:"Comp1",selectedLayers:selected?[layer]:[],numLayers:1,layer:()=>layer}));
    const host=readonly({SFLD_ready:()=>"org.starfieldfx.panel/1/native-presets-98",
        __SFLD_modelGraphTransactionV1:readonly({id:"abc",state:"complete",diagnostics:readonly({cleanupError:512}),
            request:readonly({target:readonly({token:"p0-c1-l70"})}),result:readonly({ok:true,committed:true})}),
        SFLD_getGraphSnapshot:snapshot?wire=>{const req=JSON.parse(wire);assert.equal(req.operation,"getGraphSnapshot");assert.equal(req.target.token,"p0-c1-l70");checks+=2;read++;
            return JSON.stringify({ok:true,gatewayBuild:"native-presets-98",snapshot:{initialized:true,revision:4,checksum:"123",repairNeeded:false,nativeNodes:[],renderer:{id:"output"}}});}:undefined});
    function File(name){this.fsName=name;this.parent={parent:{fsName:"D:/checkout"}};this.open=mode=>{
        assert.equal(name,"D:/checkout/artifacts/m3-20-guard-owner-20261017/host-report.json");assert.equal(mode,"w");checks+=2;opened++;return save;};
        this.write=text=>{written=text;return true;};this.close=()=>closed++;}
    vm.runInNewContext(source,{app:readonly({version:"23.5x52",project:readonly({activeItem:comp,rootFolder:readonly({id:0}),numItems:1,item:()=>comp})}),
        $:readonly({global:host,engineName:"main",fileName:"D:/checkout/tools/Report-GraphGuard.jsx"}),CompItem,File,
        alert:text=>alerts.push(text),prompt:(title,text)=>prompts.push({title,text})});
    if(!selected){assert.equal(opened,0);assert.equal(read,0);assert.equal(alerts.length,1);checks+=3;return;}
    const report=JSON.parse(save?written:prompts[0].text);
    assert.equal(report.selectedTarget,"p0-c1-l70");assert.deepEqual(report.beforeSnapshot,report.afterSnapshot);
    assert.equal(report.version,2);assert.equal(report.selectedNames.layer,"Old layer");assert.equal(report.projectRenderers[0].comp,"Comp1");checks+=3;
    if(snapshot){assert.deepEqual(report.snapshot.nativeNodes,[]);assert.equal(report.snapshot.renderer.id,"output");assert.equal(report.snapshot.repairNeeded,false);checks+=3;}
    assert.equal(report.projectRenderers.length,1);assert.equal(report.projectRenderers[0].target,"p0-c1-l70");checks+=2;
    assert.equal(report.modelSession.diagnostics.cleanupError,512);assert.equal(read,snapshot?1:0);checks+=4;
    if(throwGuard)assert.match(report.beforeSnapshot[0].guard.error,/unreadable/);
    else assert.equal(report.beforeSnapshot[0].guard,1,"saved guard retained");
    if(save){assert.equal(closed,1);assert.equal(alerts.length,1);}else{assert.equal(closed,0);assert.equal(prompts.length,1);}checks+=3;
}
run();run({selected:false});run({save:false});run({snapshot:false});run({throwGuard:true});
console.log(`graph_guard_report_tests: ${checks} checks passed; read-only host fixtures, report output restricted to checkout artifacts`);
