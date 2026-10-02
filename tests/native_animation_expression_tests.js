"use strict";
const assert = require("node:assert/strict"), fs = require("node:fs"), vm = require("node:vm"), path = require("node:path");
const root = path.resolve(__dirname, "..");
const expressions = JSON.parse(fs.readFileSync(path.join(root, "artifacts/native-animation-expressions.json"), "utf8"));
assert.equal(expressions.length, 54);
function node(firstUuid, uuid, values) {
    const properties = {};
    for(let n = 0; n < 8; n++) properties[firstUuid+n] = {value:n === 7 ? uuid : 0};
    Object.keys(values).forEach(key => properties[key] = {get value(){return typeof values[key]==="function" ? values[key]() : values[key];}});
    const effect = {param(index){if(!properties[index]) return {value:Number(index)};return properties[index];}};
    Object.defineProperty(effect,"name",{value:"Same display name"}); return effect;
}
let frameTime=0;
const emitter = node(98, 1, {4:()=>[250+frameTime*100,500]}), particle = node(111,2,{6:[0.2,0.4,0.6,1],7:[0.3,0.5,0.7,1]});
const force = node(95,4,{}), duplicate = node(98,777,{4:[999,999]});
for(frameTime of [0,0.5,1,0]) for(const effects of [[emitter,particle,force],[force,duplicate,particle,emitter]]) {
    const parade = {numProperties:effects.length};
    const layer = name => {assert.equal(name,"ADBE Effect Parade");return parade;};
    layer.effect = index => effects[index-1];
    const context = {thisLayer:layer};
    assert.throws(()=>vm.runInNewContext('var group=thisLayer("ADBE Effect Parade"); group(1);',context),
        /not a function/, "host PropertyGroup objects are not fake JS functions");
    for(const expression of expressions) {
        const match = /result = fx.param\((\d+)\).value(?:\[(\d+)\])?/.exec(expression);
        const source = expression.includes("fx.param(105).value === 1") ? emitter : expression.includes("fx.param(118).value === 2") ? particle : force;
        let expected = source.param(Number(match[1])).value;
        if(match[2] !== undefined) expected = expected[Number(match[2])];
        assert.equal(vm.runInNewContext(expression,context),expected,"UUID binding survives effect order and same names");
    }
}
const originExpression=expressions.find(expr=>expr.includes("fx.param(105).value === 1") && expr.includes("result = fx.param(4).value[0]"));
const missingLayer=()=>({numProperties:0});missingLayer.effect=()=>{throw new Error("missing effect");};
assert.equal(vm.runInNewContext(originExpression,{thisLayer:missingLayer}),-1099511627776,"missing UUID cannot turn particle settings into zero");
const failingEmitter=node(98,1,{4:()=>{throw new Error("Origin XY evaluation failed");}});
const failingLayer=()=>({numProperties:1});failingLayer.effect=()=>failingEmitter;
assert.throws(()=>vm.runInNewContext(originExpression,{thisLayer:failingLayer}),/Origin XY evaluation failed/,
    "source-property failures are not caught as unrelated effects");
// Exercise the actual gateway setter without exposing test entry points in production.
const gateway = fs.readFileSync(path.join(root,"cep_panel/jsx/starfield_gateway.jsx"),"utf8")
    .replace("    function setNodeControl(effect, name, value) {","    $.global.testSetNodeControl = setNodeControl;\n    function setNodeControl(effect, name, value) {");
const global = {};
vm.runInNewContext(gateway, {$:{global}, app:{}});
const keyframes = [[0,10],[2,20]]; let staticWrites=0;
const property = {name:"Size",value:15,numKeys:2,canSetExpression:true,expressionEnabled:false,
    setValue(){staticWrites++;},setValueAtTime(time,value){keyframes.push([time,value]);}};
const effect = {matchName:"org.starfieldfx.node.particle",property:()=>property,propertyGroup:()=>({containingComp:{time:1}})};
global.testSetNodeControl(effect,"Size",17);
assert.deepEqual(keyframes,[[0,10],[2,20],[1,17]]);
assert.equal(staticWrites,0);
property.expressionEnabled=true;
assert.throws(()=>global.testSetNodeControl(effect,"Size",18),/driven by an expression/);
assert.equal(keyframes.length,3);
const metadata={name:"Node UUID 0",value:0,canSetExpression:false,numKeys:0,
    get expressionEnabled(){throw new Error("constant streams cannot expose expression state");},setValue(v){this.value=v;}};
effect.property=()=>metadata;
global.testSetNodeControl(effect,"Node UUID 0",12);
assert.equal(metadata.value,12);
console.log("Native animation expressions and CEP keyframe preservation checks passed.");
