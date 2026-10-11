"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),vm=require("node:vm"),path=require("node:path");
const root=path.resolve(__dirname,"..");
const expressions=JSON.parse(fs.readFileSync(path.join(root,"artifacts/native-animation-expressions.json"),"utf8"));
assert.equal(expressions.length,79);
let frameTime=0,lookups=0;
const names={1:"org.starfieldfx.node.emitter",2:"org.starfieldfx.node.particle",4:"org.starfieldfx.node.force"};
function node(first,uuid,values,name=names[uuid]) {
    const properties={};for(let i=0;i<8;i++)properties['Node UUID '+i]={value:i===7?uuid:0};
    for(const key of Object.keys(values))properties[key]={get value(){return typeof values[key]==='function'?values[key]():values[key];}};
    return {name,param(index){if(typeof index==='string')return properties[index];return properties[index]||{value:0};}};
}
const emitter=node(102,1,{1:1,4:()=>[250+frameTime*100,500]}),particle=node(510,2,{10:1,11:[.3,.5,.7,1]}),force=node(208,4,{});
const duplicate=node(102,777,{4:[999,999]},'__Starfield transaction backup');
const unrelated={name:'Unrelated',param(){throw Error('cross-kind stream resolution');}};
const renderer={name:'org.starfieldfx.particle',param(){throw Error('recursive renderer alias');}};
function host(effects) {
    const layer=()=>{throw Error('expression must not enumerate effects');};
    layer.effect=name=>{assert.equal(typeof name,'string');lookups++;const found=effects.find(f=>f.name===name);if(!found)throw Error('missing effect');return found;};return layer;
}
function identity(expression){return Number(/fx.param\("Node UUID 7"\).value === (\d+)/.exec(expression)[1]);}
for(frameTime of [0,.5,1,0])for(const effects of [[renderer,emitter,particle,force,unrelated,duplicate],[unrelated,duplicate,force,particle,emitter,renderer]])for(const expression of expressions) {
    const source={1:emitter,2:particle,4:force}[identity(expression)];const field=/result = fx.param\((\d+)\).value(?:\[(\d+)\])?/.exec(expression);
    let expected=source.param(Number(field[1])).value;if(field[2]!==undefined)expected=expected[Number(field[2])];
    const before=lookups;assert.equal(vm.runInNewContext(expression,{thisLayer:host(effects)}),expected);assert.equal(lookups,before+1);
    assert.ok(!expression.includes('for (var n')&&!expression.includes('thisLayer.effect(n)'));
}
const initialExpression=expressions.find(e=>identity(e)===1&&e.includes('result = fx.param(1).value;'));
const originExpression=expressions.find(e=>identity(e)===1&&e.includes('result = fx.param(4).value[0]'));
assert.ok(initialExpression,'actual renderer stream 98 is covered');
for(const count of [undefined,0,999,'throw']) {
    const initial=node(102,1,{1:1});Object.defineProperty(initial,'numProperties',{get(){throw Error('Effect count must never be consulted');}});
    assert.equal(vm.runInNewContext(initialExpression,{thisLayer:host([renderer,initial])}),1);
}
for(let word=0;word<8;word++) {
    const wrong=node(102,1,{1:99});wrong.param('Node UUID '+word).value++;
    const param=wrong.param;let numeric=0;wrong.param=i=>{if(typeof i==='number')numeric++;return param(i);};
    assert.equal(vm.runInNewContext(initialExpression,{thisLayer:host([wrong])}),-1099511627776);assert.equal(numeric,0);
}
assert.equal(vm.runInNewContext(originExpression,{thisLayer:host([renderer,unrelated])}),-1099511627776);
const failing=node(102,1,{4:()=>{throw Error('Origin XY evaluation failed');}});
assert.throws(()=>vm.runInNewContext(originExpression,{thisLayer:host([failing])}),/Origin XY evaluation failed/);
// Host-type caching is a hypothesis, not an AE emulator. This adversarial
// fixture rejects a call site visiting more than one effect layout, even if JS
// catches its exception; the actual new expression opens only its target.
let mismatches=0,pinned;
const cachingLayer=host([particle,emitter]);const direct=cachingLayer.effect;
cachingLayer.effect=key=>{const fx=direct(key);if(pinned&&pinned!==fx.name){mismatches++;throw Error('stream matchName mismatch');}pinned=fx.name;return fx;};
assert.equal(vm.runInNewContext(originExpression,{thisLayer:cachingLayer}),emitter.param(4).value[0]);assert.equal(mismatches,0);
const escaped=JSON.parse(fs.readFileSync(path.join(root,'artifacts/native-escaped-source-expression.json'),'utf8'));
assert.equal(vm.runInNewContext(escaped.expression,{thisLayer:host([node(102,1,{1:1},escaped.name)])}),1);
assert.ok(!escaped.expression.includes(escaped.name),'UTF-16 name is entirely escaped');
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
