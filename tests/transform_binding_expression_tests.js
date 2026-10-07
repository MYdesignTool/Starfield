"use strict";
const assert = require("assert");
const fs = require("fs");
const vm = require("vm");
const expressions = JSON.parse(fs.readFileSync("artifacts/transform-matrix-expressions.json", "utf8"));
assert.equal(expressions.length, 12);
let checks = 1;
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
    return expressions.map(text => vm.runInNewContext(text + "\nresult;", {
        result:-1,time:2.5,thisLayer:owner,thisComp:{layer(i){assert.equal(i,7);return source;}},
        fx:{param(i){assert.equal(i,1);return {value: source ? 7 : 0};}}
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
        fx:{param(){return {value:source};}}});
    assert(Math.abs(value-run(owner,source)[i])<1e-10);checks++;
}
assert.throws(()=>vm.runInNewContext(expressions[0],{time:2.5,thisLayer:owner,
    thisComp:{layer(){throw Error("missing layer");}},fx:{param(){return {value:7};}}}),/missing layer/);checks++;
console.log(`Transform expressions: ${checks} checks passed.`);
