"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path").posix;
const source=fs.readFileSync(require("node:path").join(__dirname,"../tools/mfr_qualification.jsx"),"utf8");
require("internal/deps/acorn/acorn/dist/acorn").parse(source,{ecmaVersion:3,allowReserved:"never"});
function execute(options={}){
    const files=new Map(),directories=new Set(["D:/repo"]),actions=[],items=[],comps=[];
    function File(name){const value={fsName:name,encoding:"",open(){return true;},write(text){files.set(name,text);},close(){}};
        Object.defineProperty(value,"parent",{get:()=>Folder(path.dirname(name))});return value;}
    function Folder(name){const value={fsName:name,create(){directories.add(name);return true;}};
        Object.defineProperty(value,"exists",{get:()=>directories.has(name)});Object.defineProperty(value,"parent",{get:()=>Folder(path.dirname(name))});return value;}
    function property(match){return {matchName:match,setValue(value){actions.push(["value",match,value]);},setValueAtTime(time,value){actions.push(["animated",match,time,value]);}};}
    function effect(match){const props=Object.fromEntries([3,12,13,29,1611].map(id=>{const name=match+"-"+String(id).padStart(4,"0");return [name,property(name)];}));return {matchName:match,numProperties:0,property:name=>props[name]||null};}
    function comp(name){const layers=[];
        function layer(name){const effects=[],group={numProperties:0,addProperty(match){effects.push(effect(match));group.numProperties=effects.length;return effects.at(-1);},property(index){return effects[index-1];}};
            return {name,selected:true,property(match){if(match==="ADBE Effect Parade")return group;return {property:key=>property(key)};}};}
        const value={name,layers:{addSolid(color,name){const result=layer(name);layers.unshift(result);return result;},addCamera(name){const result=layer(name);layers.unshift(result);return result;}},layer(key){return typeof key==="number"?layers[key-1]:layers.find(item=>item.name===key);},openInViewer(){actions.push(["viewer"]);}};
        Object.defineProperty(value,"numLayers",{get:()=>layers.length});comps.push(value);return value;
    }
    const queue={items:{add(comp){const om={templates:options.noTemplate?["Lossless"]:["PNG Sequence"],applyTemplate(name){actions.push(["template",name]);}};
        const item={status:"QUEUED",outputModule(){return om;},remove(){items.splice(items.indexOf(item),1);}};items.push(item);return item;}},item:index=>items[index-1],render(){actions.push(["render"]);for(const item of items)item.status=options.renderFailure?"FAILED":"DONE";}};
    Object.defineProperty(queue,"numItems",{get:()=>items.length});
    const project={file:options.existing?"owner.aep":null,numItems:options.existing?1:0,items:{addComp:comp},renderQueue:queue,gpuAccelType:"software",save(file){actions.push(["save",file.fsName]);},close(){actions.push(["close"]);}};
    const app={version:options.version||"23.6",project,availableGPUAccelTypes:options.cuda?["cuda"]:null,setMultiFrameRenderingConfig(...args){actions.push(["mfr",...args]);},scheduleTask(...args){actions.push(["schedule",...args]);}};
    vm.runInNewContext(source,{app,File,Folder,JSON:undefined,$:{fileName:"D:/repo/tools/mfr_qualification.jsx",global:{__SFLD_modelTransactionAPI:{json:JSON}},evalFile(){}},GpuAccelType:{CUDA:"cuda",SOFTWARE:"software"},RQItemStatus:{DONE:"DONE"},CloseOptions:{DO_NOT_SAVE_CHANGES:0}});
    const report=[...files.entries()].find(([name])=>name.endsWith("/report.json"));
    return {actions,comps,report:report?JSON.parse(report[1]):null};
}
for(const cuda of [false,true]){
    const result=execute({cuda}),renders=cuda?24:12;
    assert.equal(result.report.passed,true);assert.equal(result.report.cases.length,cuda?12:6);assert.equal(result.report.outputs.length,renders);
    assert.equal(result.actions.filter(row=>row[0]==="render").length,renders);
    const settings=result.actions.filter(row=>row[0]==="mfr");assert.equal(settings.length,renders);
    settings.forEach((row,index)=>assert.deepEqual(row,["mfr",index%2===1,100]));
    assert.equal(result.actions.filter(row=>row[0]==="save").length,1);assert.deepEqual(result.actions.at(-2),["close"]);
    assert.deepEqual(result.actions.at(-1),["schedule","app.quit();",1000,false]);
    assert.ok(result.actions.some(row=>row[0]==="value" && row[1].endsWith("-0029") && row[2]===1),"legacy control uses disk29, not registration index28");
}
for(const options of [{existing:true},{version:"24.0"}]){const result=execute(options);assert.equal(result.report,null);assert.equal(result.actions.length,0);}
for(const options of [{noTemplate:true},{renderFailure:true}]){const result=execute(options);assert.equal(result.report.passed,false);assert.ok(result.report.error);assert.ok(result.actions.some(row=>row[0]==="close"));assert.equal(result.actions.filter(row=>row[0]==="save").length,0);}
console.log("MFR host harness: real ES3 script, absent global JSON, CPU/CUDA 8/16/32 case matrix, scoped MFR, existing-project refusal, failed output/render cleanup passed. No AE execution.");
