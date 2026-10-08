"use strict";
const assert = require("assert");
const fs = require("fs");
const vm = require("vm");
const bodies = JSON.parse(fs.readFileSync("artifacts/transform-matrix-expressions.json", "utf8"));
const expressions = bodies.selected;
assert.equal(expressions.length, 12);
assert.equal(bodies.none.length, 12);
let checks = 2;
function layer(m, anchor, twoD) {
    function map(v, point) {
        const out = [0,0,0];
        for(let r=0;r<3;r++) {
            out[r] = point ? m[r*4+3] : 0;
            for(let c=0;c<3;c++) out[r] += m[r*4+c] * (v[c] || 0);
        }
        return twoD ? out.slice(0,2) : out;
    }
    return {width:1920,height:1080,anchorPoint:{value:anchor},
        toWorld(v,t){assert.equal(t,2.5);return map(v,true);},
        toWorldVec(v,t){assert.equal(t,2.5);return map(v,false);}};
}
function run(owner, source) {
    return (source ? expressions : bodies.none).map(text => vm.runInNewContext(text + "\nresult;", {
        result:-1,time:2.5,thisLayer:owner,thisComp:{layer(){throw Error("layer indices are not expression resources");}},
        fx:{param(i){assert.equal(i,1);return source || 0;}}
    },{timeout:1000}));
}
function compare(actual, expected) {
    for(let i=0;i<12;i++) {assert(Math.abs(actual[i]-expected[i])<1e-10,`${i}: ${actual[i]} vs ${expected[i]}`);checks++;}
}
const identity=[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1];
const owner=layer([2,0,0,20,0,-4,0,40,0,0,.5,60,0,0,0,1],[0,0,0]);
const source=layer([-2,.3,0,100,0,3,.2,200,.4,0,1,300,0,0,0,1],[50,50,10]);
compare(run(owner,source),[-1,.15,0,-2.5,0,-.75,-.05,-78,.8,0,2,540]);
compare(run(owner,null),[1,0,0,960,0,1,0,540,0,0,1,0]);
compare(run(layer(identity,[],true),layer([1,0,0,910,0,1,0,490,0,0,1,0,0,0,0,1],[50,50],true)),[1,0,0,960,0,1,0,540,0,0,1,0]);
compare(run(layer(identity,[]),layer([0,0,0,20,0,0,0,40,0,0,0,60,0,0,0,1],[50,50,0])),[0,0,0,20,0,0,0,40,0,0,0,60]);
assert.throws(()=>run(layer([0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1],[]),source),/singular/);checks++;
// Layer-object values are supported without relying on mutable layer indices.
for(let i=0;i<12;i++) {
    const value=vm.runInNewContext(expressions[i]+"\nresult;",{result:0,time:2.5,thisLayer:owner,
        fx:{param(){return source;}}});
    assert(Math.abs(value-run(owner,source)[i])<1e-10);checks++;
}
assert.throws(()=>vm.runInNewContext(expressions[0],{time:2.5,thisLayer:owner,
    fx:{param(){throw Error("missing layer");}}}),/missing layer/);checks++;
// None can throw on property access in AE. UI compilation already validated
// the constant resource; identity aliases must not access its empty selector.
let selectorReads=0;
for(const text of bodies.none) {
    const context={thisLayer:owner,fx:{param(){selectorReads++;throw Error("empty layer selector");}}};
    assert(Number.isFinite(vm.runInNewContext(text+"\nresult;",context)));checks++;
}
assert.equal(selectorReads,0);checks++;
for(const text of expressions) {
    assert.throws(()=>vm.runInNewContext(text+"\nresult;",{time:2.5,thisLayer:owner,
        fx:{param(){return 0;}}}),/inherited layer unavailable/);checks++;
}

// Execute the complete expressions emitted by the actual native transaction,
// including the UUID gate, effect search and renderer skip, not just the body.
const noneBindings=JSON.parse(fs.readFileSync("artifacts/transform-none-animation-expressions.json","utf8"));
const selectedBindings=JSON.parse(fs.readFileSync("artifacts/transform-selected-animation-expressions.json","utf8"));
assert.equal(noneBindings.length,12);assert.equal(selectedBindings.length,12);checks+=2;
function effect(uuid, sourceValue) {
    return {param(index) {
        if(index>=82 && index<=89)return {name:`Node UUID ${index-82}`,value:index===89?uuid:0};
        assert.equal(index,1);
        selectorReads++;
        if(sourceValue === undefined)throw Error("empty layer selector");
        return sourceValue;
    }};
}
const unrelated={param(){return {name:"Other parameter",get value(){throw Error("unrelated numeric read");}};}};
function complete(bindings, geometry, effects, source) {
    const host=()=>({numProperties:effects.length});Object.assign(host,geometry);
    host.effect=index=>effects[index-1];
    return bindings.map(({expression})=>vm.runInNewContext(expression,{thisLayer:host,time:2.5,
        thisProperty:{propertyGroup(){return {propertyIndex:1};}},
        thisComp:{layer(index){assert.equal(index,7);return source;}}},{timeout:1000}));
}
selectorReads=0;
const noneEffect=effect(5),duplicate=effect(777,source);
for(const effects of [[unrelated,noneEffect,duplicate],[unrelated,duplicate,unrelated,noneEffect]]) {
    compare(complete(noneBindings,owner,effects),[1,0,0,960,0,1,0,540,0,0,1,0]);
}
assert.equal(selectorReads,0);checks++;
const proxy=Object.create(source);
Object.defineProperty(proxy,"value",{get(){throw Error("Layer object is not a numeric property");}});
for(const sourceValue of [source,proxy]) for(const effects of [
    [unrelated,effect(5,sourceValue),duplicate],[unrelated,duplicate,unrelated,effect(5,sourceValue)]]) {
    compare(complete(selectedBindings,owner,effects,source),[-1,.15,0,-2.5,0,-.75,-.05,-78,.8,0,2,540]);
}
assert.deepEqual(complete(noneBindings,owner,[unrelated,duplicate]),Array(12).fill(-1099511627776));checks++;
assert.throws(()=>complete(selectedBindings,owner,[unrelated,effect(5)]),/empty layer selector/);checks++;
// Reproduce the old unsafe read against the same host-shaped None fixture.
const oldNone=noneBindings.map(({expression})=>({expression:expression.replace(
    "var identity =", "var selected = fx.param(1).value;\nvar identity =")}));
assert.throws(()=>complete(oldNone,owner,[unrelated,noneEffect]),/empty layer selector/);checks++;
const oldSelected=selectedBindings.map(({expression})=>({expression:expression.replace(
    "var src = fx.param(1);", "var src = fx.param(1).value;")}));
assert.throws(()=>complete(oldSelected,owner,[unrelated,effect(5,proxy)]),/Layer object is not a numeric property/);checks++;
console.log(`Transform expressions: ${checks} checks passed.`);
