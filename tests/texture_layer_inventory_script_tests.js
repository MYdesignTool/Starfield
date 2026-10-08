"use strict";
const fs=require("node:fs"),path=require("node:path"),vm=require("node:vm"),assert=require("node:assert/strict");
const script=fs.readFileSync(path.join(__dirname,"../artifacts/texture-selector-tests/generated-inventory.jsx"),"utf8");
let checks=0;const equal=(a,b)=>{checks++;assert.deepEqual(a,b);},reject=(f,re)=>{checks++;assert.throws(f,re);};
class CompItem {constructor(id,layers=[]){this.id=id;this.layers=layers;this.numLayers=layers.length;}layer(i){return this.layers[i-1];}}
const precomp=new CompItem(45),footage={id:50};
const layers=[{id:77,name:"Renderer",source:footage,hasVideo:true},
    {id:101,name:"Comp 2 中文 & | 😀",source:precomp,hasVideo:false},
    {id:102,name:"Clip",source:footage,hasVideo:true},
    {id:103,name:"Audio",source:footage,hasVideo:false},
    {id:104,name:"Null",source:footage,hasVideo:true,nullLayer:true},
    {id:105,name:"Camera",hasVideo:false}];
const comp=new CompItem(44,layers),wrong=new CompItem(99,[{id:201,name:"Wrong",source:footage,hasVideo:true}]);
let items=[wrong,footage,comp,precomp];
const project={get numItems(){return items.length;},item(i){return items[i-1];},activeItem:wrong};
const context={app:{project},CompItem};
function read(){return vm.runInNewContext(script,context).trimEnd().split("\n").slice(1).map(line=>{
    const [id,hex]=line.split("|");let name="";for(let i=0;i<hex.length;i+=4)name+=String.fromCharCode(parseInt(hex.slice(i,i+4),16));
    return {id:Number(id),name};
});}
equal(read(),[{id:101,name:"Comp 2 中文 & | 😀"},{id:102,name:"Clip"}]);
project.activeItem=null;equal(read().map(x=>x.id),[101,102]);
layers[1].name="x".repeat(511)+"😀rest";equal(read()[0].name,"x".repeat(511));
layers[1].name="x".repeat(510)+"😀rest";equal(read()[0].name,"x".repeat(510)+"😀");
items=[wrong,footage];reject(read,/owner composition unavailable/);
items=[{id:44}];reject(read,/not a composition/);
items=[comp];comp.numLayers=4097;reject(read,/owner composition unavailable/);comp.numLayers=layers.length;
context.app.project=null;reject(read,/project inventory unavailable/);context.app.project=project;
items=new Array(65537);reject(read,/project inventory unavailable/);
equal(script.includes("setValue"),false);equal(script.includes("addNull"),false);equal(script.includes("executeCommand"),false);
console.log(`texture_layer_inventory_script_tests: ${checks} checks passed; actual AE menu remains a host gate`);
