"use strict";
const assert = require("node:assert/strict"), fs = require("node:fs"), vm = require("node:vm"), path = require("node:path");
const root = path.resolve(__dirname, "..");
const expressions = JSON.parse(fs.readFileSync(path.join(root, "artifacts/native-animation-expressions.json"), "utf8"));
assert.equal(expressions.length, 74);
function node(firstUuid, uuid, values) {
    const properties = {};
    for(let n = 0; n < 8; n++) properties[firstUuid+n] = {name:`Node UUID ${n}`,value:n === 7 ? uuid : 0};
    Object.keys(values).forEach(key => properties[key] = {get value(){return typeof values[key]==="function" ? values[key]() : values[key];}});
    const effect = {param(index){if(!properties[index]) return {value:Number(index)};return properties[index];}};
    Object.defineProperty(effect,"name",{value:"Same display name"}); return effect;
}
let frameTime=0;
const thisProperty={propertyGroup(level){assert.equal(level,1);return {propertyIndex:0};}};
const emitter = node(102, 1, {4:()=>[250+frameTime*100,500]}), particle = node(510,2,{10:1,11:[0.3,0.5,0.7,1]});
const force = node(208,4,{}), duplicate = node(102,777,{4:[999,999]});
for(frameTime of [0,0.5,1,0]) for(const effects of [[emitter,particle,force],[force,duplicate,particle,emitter]]) {
    const parade = {numProperties:effects.length};
    const layer = name => {assert.equal(name,"ADBE Effect Parade");return parade;};
    layer.effect = index => effects[index-1];
    const context = {thisLayer:layer,thisProperty};
    assert.throws(()=>vm.runInNewContext('var group=thisLayer("ADBE Effect Parade"); group(1);',context),
        /not a function/, "host PropertyGroup objects are not fake JS functions");
    for(const expression of expressions) {
        const match = /result = fx.param\((\d+)\).value(?:\[(\d+)\])?/.exec(expression);
        const source = expression.includes("fx.param(109).value === 1") ? emitter : expression.includes("fx.param(517).value === 2") ? particle : force;
        let expected = source.param(Number(match[1])).value;
        if(match[2] !== undefined) expected = expected[Number(match[2])];
        assert.equal(vm.runInNewContext(expression,context),expected,"UUID binding survives effect order and same names");
    }
}
const originExpression=expressions.find(expr=>expr.includes("fx.param(109).value === 1") && expr.includes("result = fx.param(4).value[0]"));
const missingLayer=()=>({numProperties:0});missingLayer.effect=()=>{throw new Error("missing effect");};
assert.equal(vm.runInNewContext(originExpression,{thisLayer:missingLayer,thisProperty}),-1099511627776,"missing UUID cannot turn particle settings into zero");
const failingEmitter=node(102,1,{4:()=>{throw new Error("Origin XY evaluation failed");}});
const failingLayer=()=>({numProperties:1});failingLayer.effect=()=>failingEmitter;
assert.throws(()=>vm.runInNewContext(originExpression,{thisLayer:failingLayer,thisProperty}),/Origin XY evaluation failed/,
    "source-property failures are not caught as unrelated effects");
// Renderer aliases overlap the native UUID indices (102..109, 510..517).
// Looking at their numeric values while searching creates expression-to-
// expression dependencies, including a dependency on the current alias itself.
let unrelatedReads=0;
const renderer={param(index){return {name:`Node Input ${index-98}`,
    get value(){unrelatedReads++;throw new Error("recursive renderer binding evaluated");}};}};
const unrelated={param(){return {name:"Animated parameter",get value(){unrelatedReads++;return 0;}};}};
for(const effects of [[renderer,emitter,particle,force,renderer,unrelated],
    [unrelated,renderer,force,particle,duplicate,emitter]]) {
    const layer=()=>({numProperties:effects.length});layer.effect=index=>effects[index-1];
    for(const expression of expressions) {
        assert.ok(expression.includes('.name === "Node UUID 0" &&'),"name gate precedes UUID numeric reads");
        assert.notEqual(vm.runInNewContext(expression,{thisLayer:layer,thisProperty}),-1099511627776);
    }
}
assert.equal(unrelatedReads,0,"UUID lookup never evaluates renderer or unrelated animated values");
// Keep a regression demonstrating what the previous generated expressions did.
const legacy=originExpression.replace(/fx\.param\(\d+\)\.name === "Node UUID 0" && /,"");
const oldLayer=()=>({numProperties:2});oldLayer.effect=index=>index===1?renderer:emitter;
vm.runInNewContext(legacy,{thisLayer:oldLayer,thisProperty});
assert.ok(unrelatedReads>0,"legacy lookup reads renderer animation aliases while searching");
const ownLayer=()=>({numProperties:2});
ownLayer.effect=index=>{assert.notEqual(index,1,"own renderer is skipped before any property lookup");return emitter;};
assert.equal(vm.runInNewContext(originExpression,{thisLayer:ownLayer,
    thisProperty:{propertyGroup:()=>({propertyIndex:1})}}),emitter.param(4).value[0]);
// Exercise the actual gateway setter without exposing test entry points in production.
const gateway = fs.readFileSync(path.join(root,"cep_panel/jsx/starfield_gateway.jsx"),"utf8")
    .replace("    function setNodeControl(effect, control, value) {","    $.global.testSetNodeControl = setNodeControl;\n    function setNodeControl(effect, control, value) {");
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
