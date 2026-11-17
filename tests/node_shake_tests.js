"use strict";
const assert=require("node:assert/strict"),shake=require("../web_panel/js/node_shake.js");
function replay(points){const detector=shake.create();detector.reset(0,0,0);let triggers=0;
    for(const [x,y,t] of points)triggers+=Number(detector.sample(x,y,t));return {detector,triggers};}
for(const axis of [0,1])for(const sign of [-1,1]){
    const points=[20,-20,20,-20,20].map((value,i)=>axis?[0,sign*value,50+i*80]:[sign*value,0,50+i*80]);
    const result=replay(points);assert.equal(result.triggers,1);assert.equal(result.detector.triggered(),true);
    assert.equal(result.detector.sample(100,100,500),false,"one detach intent per gesture");
}
assert.equal(replay([20,-20,20,-20].map((x,i)=>[x,0,i*80+30])).triggers,0,"three reversals are insufficient");
assert.equal(replay([20,-20,20,-20,20].map((x,i)=>[x,0,i*250+30])).triggers,0,"slow movement is ordinary dragging");
assert.equal(replay(Array.from({length:100},(_,i)=>[(i%2?1:-1)*5,0,i*4])).triggers,0,"jitter does not detach");
assert.equal(replay(Array.from({length:100},(_,i)=>[i,Math.sin(i)*3,i*4])).triggers,0,"a steady diagonal drag does not detach");
let detector=shake.create();detector.reset(0,0,0);detector.sample(20,0,30);detector.sample(-20,0,100);
detector.reset(0,0,200);assert.equal(detector.triggered(),false,"new/cancelled drag clears old intent");
for(const t of [NaN,Infinity,-1])assert.equal(detector.sample(10,0,t),false);
console.log("node shake: horizontal/vertical, both directions, timing, jitter, latch and cancellation passed");
