"use strict";
const assert = require("node:assert/strict"), fs = require("node:fs"), vm = require("node:vm"), path = require("node:path");
const root = path.resolve(__dirname, "..");
const expressions = JSON.parse(fs.readFileSync(path.join(root, "artifacts/native-animation-expressions.json"), "utf8"));
assert.equal(expressions.length, 79);
function node(firstUuid, uuid, values) {
    const properties = {};
    for(let n = 0; n < 8; n++) properties[firstUuid+n] = {name:`Node UUID ${n}`,value:n === 7 ? uuid : 0};
    Object.keys(values).forEach(key => properties[key] = {get value(){return typeof values[key]==="function" ? values[key]() : values[key];}});
    const effect = {param(index){
        if(typeof index === "string") {
            const entry=Object.values(properties).find(p=>p.name===index);
            if(!entry)throw Error("No named metadata: "+index);
            return entry;
        }
        if(!properties[index]) return {value:Number(index)};
        return properties[index];
    }};
    Object.defineProperty(effect,"name",{value:"Same display name"}); return effect;
}
let frameTime=0;
const thisProperty={propertyGroup(level){assert.equal(level,1);return {propertyIndex:0};}};
const emitter = node(102, 1, {1:1,4:()=>[250+frameTime*100,500]}), particle = node(510,2,{10:1,11:[0.3,0.5,0.7,1]});
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
        const source = expression.includes('fx.param("Node UUID 7").value === 1') ? emitter : expression.includes('fx.param("Node UUID 7").value === 2') ? particle : force;
        let expected = source.param(Number(match[1])).value;
        if(match[2] !== undefined) expected = expected[Number(match[2])];
        assert.equal(vm.runInNewContext(expression,context),expected,"UUID binding survives effect order and same names");
    }
}
const originExpression=expressions.find(expr=>expr.includes('fx.param("Node UUID 7").value === 1') && expr.includes("result = fx.param(4).value[0]"));
const initialExpression=expressions.find(expr=>expr.includes('fx.param("Node UUID 7").value === 1') && expr.includes("result = fx.param(1).value;"));
assert.ok(initialExpression,"actual first Emitter Type binding (renderer stream 98) is covered");
for(const countMode of [undefined,0,999,"throw"]){
    const initial=node(102,1,{1:1});let countReads=0;
    if(countMode!==undefined)Object.defineProperty(initial,"numProperties",{get(){countReads++;if(countMode==="throw")throw Error("Effect count unavailable");return countMode;}});
    const initialLayer=()=>({numProperties:2});initialLayer.effect=index=>index===1?{}:initial;
    const ownProperty={propertyGroup:()=>({propertyIndex:1})};
    assert.equal(vm.runInNewContext(initialExpression,{thisLayer:initialLayer,thisProperty:ownProperty}),1);
    assert.equal(countReads,0,"new layer binding does not depend on Effect parameter count");
    // Reproduce native66's extra gate using the generated current expression.
    const native66=initialExpression.replace('try { match = ','try { match = fx.numProperties === 110 && ')
        .replace(/fx\.param\("Node UUID (\d)"\)/g,(_,n)=>`fx.param(${102+Number(n)})`);
    assert.equal(vm.runInNewContext(native66,{thisLayer:initialLayer,thisProperty:ownProperty}),-1099511627776,
        "native66 gate loses an otherwise valid first source in this host fixture");
}
// Every UUID word must agree before opening a numeric source parameter.
for(let word=0;word<8;word++){
    const impostor=node(102,1,{1:99});impostor.param(`Node UUID ${word}`).value++;
    const impostorParam=impostor.param;let numericReads=0;
    impostor.param=index=>{if(typeof index==="number")numericReads++;return impostorParam(index);};
    const rejected=()=>({numProperties:1});rejected.effect=()=>impostor;
    assert.equal(vm.runInNewContext(initialExpression,{thisLayer:rejected,thisProperty}),-1099511627776);
    assert.equal(numericReads,0,"a partial UUID cannot resolve a source stream");
}
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
const renderer={param(index){if(typeof index==="string")throw Error("Renderer has no node UUID");return {name:`Node Input ${index-98}`,
    get value(){unrelatedReads++;throw new Error("recursive renderer binding evaluated");}};}};
const unrelated={param(index){if(typeof index==="string")throw Error("No named UUID");return {name:"Animated parameter",get value(){unrelatedReads++;return 0;}};}};
for(const effects of [[renderer,emitter,particle,force,renderer,unrelated],
    [unrelated,renderer,force,particle,duplicate,emitter]]) {
    const layer=()=>({numProperties:effects.length});layer.effect=index=>effects[index-1];
    for(const expression of expressions) {
        assert.ok(expression.includes('fx.param("Node UUID 0").value'),"identity lookup uses constant metadata names");
        assert.ok(!expression.includes("fx.numProperties"),"Effect count is not a layout contract");
        assert.notEqual(vm.runInNewContext(expression,{thisLayer:layer,thisProperty}),-1099511627776);
    }
}
assert.equal(unrelatedReads,0,"UUID lookup never evaluates renderer or unrelated animated values");
// Keep a regression demonstrating what the previous generated expressions did.
const legacy=originExpression.replace(/fx\.param\("Node UUID (\d)"\)/g,(_,n)=>`fx.param(${102+Number(n)})`);
const oldLayer=()=>({numProperties:2});oldLayer.effect=index=>index===1?renderer:emitter;
vm.runInNewContext(legacy,{thisLayer:oldLayer,thisProperty});
assert.ok(unrelatedReads>0,"legacy lookup reads renderer animation aliases while searching");
let incompatibleLookups=0;
const foreignLayouts=[90,100,110,2000].map(count=>({numProperties:count,
    param(index){if(typeof index==="string")throw Error("No matching UUID");incompatibleLookups++;throw new Error("cross-kind stream identity mismatch");}}));
const mixed=()=>({numProperties:foreignLayouts.length+1});mixed.effect=index=>foreignLayouts.concat(emitter)[index-1];
assert.equal(vm.runInNewContext(originExpression,{thisLayer:mixed,thisProperty}),emitter.param(4).value[0]);
assert.equal(incompatibleLookups,0,"mixed Model/Transform/renderer layouts are rejected before resolving numeric streams");
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
