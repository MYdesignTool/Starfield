"use strict";
// Execute the manual report with read-only fake host objects, never actual AE.
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const parser=require("internal/deps/acorn/acorn/dist/acorn");
const source=fs.readFileSync(path.join(__dirname,"../tools/Report-ModelTarget.jsx"),"utf8");
parser.parse(source,{ecmaVersion:3,allowReserved:"never"});let checks=1;
function readonly(value){return new Proxy(value,{set(){throw new Error("Unexpected host write");},deleteProperty(){throw new Error("Unexpected host deletion");}});}
function run(projectId,selection=true,session){
    function CompItem(){}
    const layer=readonly({id:303,name:"Starfield target"}),comp=new CompItem();
    comp.id=202;comp.selectedLayers=selection?[layer]:[];
    const project=projectId===null?null:readonly({rootFolder:readonly({id:projectId}),activeItem:readonly(comp)});
    const messages=[],host=readonly({__SFLD_modelGraphTransactionV1:session,SFLD_modelTransactionHostRequest:function(){},SFLD_modelTransactionHostResult:function(){}});
    vm.runInNewContext(source,{app:readonly({project,version:"23.6",findMenuCommandId:name=>{
        assert.equal(name,"Starfield Apply Model Graph Transaction");checks++;return 55;
    }}),$:readonly({global:host,engineName:"main"}),CompItem,alert:text=>messages.push(text)});
    assert.equal(messages.length,1);checks++;return messages[0];
}
for(const id of [0,101]){const message=run(id);assert.ok(message.includes("token=p"+id+"-c202-l303"));checks++;
    assert.ok(message.includes("AE=23.6"));checks++;}
for(const message of [run(null),run(101,false)]){assert.ok(message.includes("Select the composition"));checks++;}
const idle=run(0);assert.ok(idle.includes("Model state=none"));checks++;
const receiving=run(0,true,readonly({state:"receiving",expires:Date.now()+300000}));
for(const text of ["Model state=receiving","engine=main","Host command=55; Host request entry=function"]){assert.ok(receiving.includes(text));checks++;}
const executing=run(0,true,readonly({state:"executing",expires:Date.now()+300000,message:"The renderer is busy or its guard changed."}));
for(const text of ["Model state=executing","Model message=The renderer is busy or its guard changed.","Result=none; diagnostics=none","Host result entry=function"]){assert.ok(executing.includes(text));checks++;}
console.log(`model_target_report_tests: ${checks} checks passed; ES3/read-only fake host, no AE execution`);
