"use strict";
// Safety/fixture checks only; never launches or qualifies a real AE host.
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=path.resolve(__dirname,".."),codec=require("../web_panel/js/graph_codec.js");
const fixture=JSON.parse(fs.readFileSync(process.argv[2],"utf8"));
const source=fs.readFileSync(path.join(root,"tools/Run-PreparationQualification.jsx"),"utf8");
let checks=0;
function check(ok,message){assert.ok(ok,message);checks++;}
for(const row of fixture.cases){const graph=codec.fromHex(row.graphHex);check(graph.nodes.length===row.nodeCount,"generated graph count/CRC");check(row.graphHex.length<=48*1024,"bounded graph payload");}
function hostTest(existing,version="23.fake",prior=false){
    const files=new Map(),events=[],scheduled=[],created=[],submissions=[];let serial=0,activeRender=null;
    if(prior)files.set(fixture.result,JSON.stringify({status:"prior-evidence"}));
    function File(name){this.fsName=String(name);this.parent={fsName:path.dirname(this.fsName)};}
    Object.defineProperty(File.prototype,"exists",{get(){return files.has(this.fsName);}});
    Object.defineProperty(File.prototype,"length",{get(){return (files.get(this.fsName)||"").length;}});
    File.prototype.open=function(){this.text="";return true;};File.prototype.write=function(s){this.text+=s;};File.prototype.close=function(){files.set(this.fsName,this.text);};
    const outputID=fixture.cases[0].rendererManifest.id;
    const app={version,findMenuCommandId(){return 1;},executeCommand(){events.push("panel");},scheduleTask(code){scheduled.push({id:++serial,code});return serial;},cancelTask(){},project:{file:null,numItems:existing?1:0,bitsPerChannel:8,save(file){events.push("save");this.file=file;},renderQueue:{numItems:0,items:{add(){app.project.renderQueue.numItems++;const item={status:1,outputModule(){return item.module;},module:{file:null},remove(){app.project.renderQueue.numItems--;}};activeRender=item;return item;}},render(){activeRender.status=2;files.set(activeRender.module.file.fsName,"fake frame");events.push("render");}},items:{addComp(name){events.push("comp");const comp={id:created.length,name,layers:{addSolid(){return {selected:false,property(){return {addProperty(){}};}};}},openInViewer(){}};created.push(comp);return comp;}}}};
    let caseIndex=0;
    const host={__SFLD_preparationQualification:fixture,__SFLD_nativeCommandsV1:{version:1,commands:{}},SFLD_getState(){return JSON.stringify({ok:true,target:{token:"fixture"}});},SFLD_syncGraphSnapshot(){
        const row=fixture.cases[caseIndex],nativeNodes=Object.keys(row.initial).map((type,i)=>({type,id:(1000+caseIndex*100+i).toString(16).padStart(32,"f")}));
        return JSON.stringify({ok:true,snapshot:{initialized:true,revision:1,recordStamp:"test",nativeNodes,renderer:{id:outputID}}});},SFLD_submitGraph(text){
        const request=JSON.parse(text),graph=codec.fromHex(request.graphHex);submissions.push(request);
        for(const native of request.baseNodeManifest){check(graph.nodes.some(n=>n.id===native.id&&n.type===native.type),"preserve each runtime bootstrap UUID in graph");check(request.nodeManifest.some(n=>n.id===native.id&&n.type===native.type),"preserve runtime UUID in manifest");}
        for(const edge of graph.edges)check(graph.nodes.some(n=>n.id===edge.sourceNode)&&graph.nodes.some(n=>n.id===edge.destinationNode),"remapped graph retains actual endpoints");
        caseIndex++;return JSON.stringify({ok:true});}};
    const context=vm.createContext({$: {global:host,writeln(){}},File,app,RQItemStatus:{DONE:2},JSON,Date});vm.runInContext(source,context);
    for(let n=0;scheduled.length&&n<100;n++)vm.runInContext(scheduled.shift().code,context);
    check(scheduled.length===0,"bounded scheduling terminates");
    return {events,submissions,result:JSON.parse(files.get(fixture.result))};
}
const refused=hostTest(true);check(refused.result.status==="failed","existing unsaved project is refused");check(refused.events.length===0,"refused project never opens panel/creates/saves/renders");
const wrong=hostTest(false,"24.fake");check(wrong.result.status==="failed"&&wrong.events.length===0,"unsupported host is refused before project mutation");
const prior=hostTest(false,"23.fake",true);check(prior.result.status==="prior-evidence"&&prior.events.length===0,"prior result is retained without any host action or overwrite");
const success=hostTest(false);check(success.result.status==="completed-measurements","fake host reaches measurements completion");check(success.submissions.length===5,"five independently remapped submissions");check(success.events.filter(e=>e==="render").length===15,"three distinct frames per graph");check(success.events.filter(e=>e==="save").length===1,"only owned test project saved once");
console.log(`Preparation qualification safety/CRC: ${checks} checks passed. No real AE process or host performance evidence.`);
