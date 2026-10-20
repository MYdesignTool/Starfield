/* Regression for Ae-ToolKit's previously fixed pointer reflection discontinuities. */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const code=fs.readFileSync(path.join(__dirname,'../docs/site/glass.js'),'utf8');
function fixture(reduced=false){
  let now=0,serial=0;const frames=new Map(),events={},docEvents={},mediaEvents=[],properties=new Map();
  const media={matches:reduced,addEventListener:(name,fn)=>mediaEvents.push(fn)};
  const card={style:{setProperty:(k,v)=>properties.set(k,v),removeProperty:k=>properties.delete(k)},
    classList:{contains:()=>false},getBoundingClientRect:()=>({left:0,top:0,width:400,height:250}),addEventListener:(name,fn)=>events[name]=fn};
  const document={hidden:false,querySelectorAll:()=>[card],addEventListener:(name,fn)=>docEvents[name]=fn,
    createElement:()=>{throw Error('unsupported optics must stay on CSS fallback');}};
  const CSS={supports:()=>false};
  const window={CSS,matchMedia:q=>q.includes('pointer:fine')?{matches:true}:media};
  vm.runInNewContext(code,{window,document,CSS,performance:{now:()=>now},requestAnimationFrame:fn=>{frames.set(++serial,fn);return serial;},cancelAnimationFrame:id=>frames.delete(id),Math,Map});
  function step(ms=16){now+=ms;const batch=[...frames.values()];frames.clear();for(const fn of batch)fn(now);}
  return {events,document,properties,frames,step,number:k=>parseFloat(properties.get(k)),reduce:b=>{media.matches=b;for(const fn of mediaEvents)fn();},hide:()=>{document.hidden=true;docEvents.visibilitychange();}};
}
const f=fixture();
f.events.pointerenter({clientX:400,clientY:200,pointerType:'mouse'});
assert.equal(f.frames.size,1);assert(!f.properties.has('--mx'),'entry does not jump synchronously');
f.step();assert(f.number('--mx')>30&&f.number('--mx')<85,'entry eases from current position');
const before=f.number('--mx');
for(let i=0;i<20;i++)f.events.pointermove({clientX:0,clientY:0,pointerType:'mouse'});
assert.equal(f.frames.size,1,'pointer burst schedules one paint');assert.equal(f.number('--mx'),before);
f.step();assert(f.number('--mx')>15&&f.number('--mx')<before,'quick direction reversal eases');
const atLeave=f.number('--mx'),alphaAtLeave=f.number('--sheen-opacity');
f.events.pointerleave();assert.equal(f.number('--mx'),atLeave);assert.equal(f.number('--sheen-opacity'),alphaAtLeave,'leave retains current brightness for the fade');
f.step();assert(f.number('--sheen-opacity')<alphaAtLeave);
f.events.pointerenter({clientX:250,clientY:150,pointerType:'mouse'});assert(f.number('--sheen-opacity')>0,'re-entry continues rather than restarting');
f.events.pointerleave();for(let i=0;i<240&&f.frames.size;i++)f.step();
assert.equal(f.frames.size,0,'idle reflection stops rendering');assert(!f.properties.has('--mx'));assert(!f.properties.has('--sheen-opacity'));
f.events.pointermove({clientX:200,clientY:80,pointerType:'touch'});assert.equal(f.frames.size,0,'touch scrolling never wakes reflection');
f.events.pointerenter({clientX:250,clientY:150,pointerType:'mouse'});f.step();f.reduce(true);
assert.equal(f.frames.size,0);assert(!f.properties.has('--mx'),'reduced motion clears live reflection');
f.reduce(false);f.events.pointerenter({clientX:250,clientY:150,pointerType:'mouse'});f.step();f.hide();assert.equal(f.frames.size,0);
const quiet=fixture(true);quiet.events.pointermove({clientX:200,clientY:100,pointerType:'mouse'});assert.equal(quiet.frames.size,0);
console.log('Glass regression checks passed: smooth enter/reversal/leave/re-entry, one paint per frame, idle stop, no touch capture, reduced motion, hidden page and CSS optics fallback.');
