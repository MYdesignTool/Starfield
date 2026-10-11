"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm");
const expressions=JSON.parse(fs.readFileSync("artifacts/model-transform-animation-expressions.json","utf8"));
const models=expressions.filter(text=>text.includes('fx.param("Node UUID 7").value === 6'));
const transforms=expressions.filter(text=>text.includes('fx.param("Node UUID 7").value === 5'));
assert.equal(models.length,14);assert.ok(transforms.length>12);
let frame=0,expectedCount=0,incompatible=0,checks=2;
function effect(count,first,uuid,values){return {get numProperties(){throw Error("Effect parameter count unavailable");},name:"Same node name",
    param(index){
        if(typeof index === "string") {
            const metadata=/^Node UUID ([0-7])$/.exec(index);
            if(!metadata)throw Error("No named metadata");
            return {name:index,value:Number(metadata[1])===7?uuid:0};
        }
        if(count!==expectedCount){incompatible++;throw Error("cross-kind numeric stream resolution");}
        if(index>=first && index<first+8)return {name:`Node UUID ${index-first}`,value:index===first+7?uuid:0};
        return {name:`Control ${index}`,value:typeof values[index]==="function"?values[index]():values[index]||0};
    }};}
const model=effect(100,86,6,{5:()=>37+frame,6:21,7:9,8:15,9:25,10:35,11:100,12:80,13:120,17:1,18:1});
const transform=effect(90,82,5,{2:[960,540],10:100,11:100,12:100,13:100,14:100});
const otherModel=effect(100,86,999,{5:999});
const unrelated={param(index){if(typeof index==="number")incompatible++;throw Error("unrelated effect lookup");}};
const renderer={param(index){if(typeof index==="number")incompatible++;throw Error("recursive renderer lookup");}};
const thisProperty={propertyGroup:()=>({propertyIndex:1})};
function layer(effects){const result=()=>({numProperties:effects.length});result.effect=index=>effects[index-1];
    result.width=1920;result.height=1080;return result;}
const matrix=[1,0,0,960,0,1,0,540,0,0,1,0];
for(frame of [0,1,2,0])for(const effects of [[renderer,transform,unrelated,otherModel,model],
    [renderer,model,otherModel,unrelated,transform]])for(const text of expressions){
    const isModel=text.includes('fx.param("Node UUID 7").value === 6');expectedCount=isModel?100:90;
    assert.ok(!text.includes("fx.numProperties") && text.includes('fx.param("Node UUID 0").value'));checks++;
    const entry=/Starfield Transform affine entry (\d+)/.exec(text);
    const field=/result = fx.param\((\d+)\).value(?:\[(\d+)\])?/.exec(text);
    let expected;
    if(entry)expected=matrix[Number(entry[1])];
    else {expected=(isModel?model:transform).param(Number(field[1])).value;if(field[2]!==undefined)expected=expected[Number(field[2])];}
    assert.equal(vm.runInNewContext(text,{thisLayer:layer(effects),thisProperty}),expected);checks++;
}
assert.equal(incompatible,0,"actual generated Model/Transform expressions do not open another kind's numeric streams");checks++;
const legacy=models[0].replace(/fx\.param\("Node UUID (\d)"\)/g,(_,n)=>`fx.param(${86+Number(n)})`);expectedCount=100;
vm.runInNewContext(legacy,{thisLayer:layer([renderer,transform,model]),thisProperty});
assert.ok(incompatible>0,"old lookup crosses Model/Transform layouts even when the guard catches the error");checks++;
console.log(`Actual mixed Model/Transform expression execution: ${checks} checks passed (JS host fixture, not AE qualification).`);
