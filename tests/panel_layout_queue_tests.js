"use strict";
const assert=require("node:assert/strict"),path=require("node:path");
const panel=process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"../web_panel");
const queue=require(path.join(panel,"js/graph_layout_queue.js"));
let pending=[],saves=[],failures=[],timers=new Map(),clock=0,saved=0;
const q=queue.create({setTimeout(fn){timers.set(++clock,fn);return clock;},clearTimeout(id){timers.delete(id);},
    save(context,changes,done){saves.push({context,changes});pending.push(done);},saved(){saved++;},failed(e){failures.push(e);}});
function tick(){const batch=Array.from(timers.values());timers.clear();batch.forEach(fn=>fn());}
const context={token:"test",projectScope:"a.aep"};
{
    const scheduled=new Map(),sent=[];let serial=0,finish;
    const own=queue.create({setTimeout(fn){scheduled.set(++serial,fn);return serial;},clearTimeout(id){scheduled.delete(id);},save(ctx,changes,done){sent.push(changes);finish=done;},saved(){},failed(e){throw e;}});
    own.reset(context,{a:{x:0,y:0}});own.defer();assert.equal(own.adopt(context,{a:{x:0,y:0},c:{x:10,y:10}}),true);
    own.enqueue({c:{x:30,y:40}});assert.equal(scheduled.size,0,"membership adoption preserves active gesture pause");own.resume();
    const advance=()=>{const batch=[...scheduled.values()];scheduled.clear();batch.forEach(fn=>fn());};advance();
    assert.deepEqual(sent[0],[{id:"c",from:{x:10,y:10},to:{x:30,y:40}}]);
    assert.equal(own.adopt(context,{c:{x:99,y:99}}),false,"topology adoption never cancels an in-flight layout write");finish({ok:true});
    assert.equal(own.adopt(context,{c:{x:30,y:40}}),true);own.enqueue({c:{x:30,y:40},a:{x:50,y:50}});advance();assert.equal(sent.length,1,"removed membership cannot schedule a stale write");
}
q.reset(context,{a:{x:0,y:0},b:{x:5,y:5}});
for(let i=1;i<=100;i++)q.enqueue({a:{x:i,y:-i},b:{x:5,y:5}});
assert.equal(saves.length,0,"drag bursts do not talk to AE before quiet time");tick();
assert.equal(saves.length,1);assert.deepEqual(saves[0].changes,[{id:"a",from:{x:0,y:0},to:{x:100,y:-100}}]);
q.enqueue({a:{x:0,y:0},b:{x:8,y:9}});tick();assert.equal(saves.length,1,"inflight metadata writes never overlap");
let done=0;q.flush(error=>{assert.equal(error,null);done++;});pending.shift()({ok:true});
assert.equal(saves.length,2);assert.equal(done,0,"structural barrier waits for the latest coordinates too");
assert.deepEqual(saves[1].changes,[{id:"a",from:{x:100,y:-100},to:{x:0,y:0}},{id:"b",from:{x:5,y:5},to:{x:8,y:9}}]);
pending.shift()({ok:true});assert.equal(done,1);assert.equal(q.hasWork(),false);
q.enqueue({a:{x:1,y:1}});q.enqueue({a:{x:0,y:0}});tick();assert.equal(saves.length,2,"drag back to baseline makes no request");
q.enqueue({a:{x:1,y:1}});q.defer();tick();assert.equal(saves.length,2,"a new gesture pauses unsent persistence");
q.enqueue({a:{x:0,y:0}});q.resume();tick();assert.equal(saves.length,2,"canceling a subsequent gesture can still coalesce to baseline");
q.enqueue({a:{x:2,y:3}});tick();q.flush(error=>{assert.equal(error.code,"timeout");done++;});
pending.shift()({ok:false,error:{code:"timeout"}});tick();assert.equal(saves.length,3,"unknown outcomes are never automatically retried");
assert.equal(failures.length,1);assert.equal(done,2);
q.enqueue({a:{x:4,y:5}});tick();const late=pending.shift();
q.reset({token:"new",projectScope:"b.aep"},{a:{x:30,y:40}});late({ok:true});assert.equal(saved,2,"late prior-context ACK cannot mutate new context");
q.enqueue({a:{x:31,y:40}});tick();assert.deepEqual(saves.at(-1).changes,[{id:"a",from:{x:30,y:40},to:{x:31,y:40}}]);
pending.shift()({ok:true});assert.equal(q.hasWork(),false);
{
    const callbacks=[],sent=[],scheduled=new Map();let serial=0;
    const paused=queue.create({setTimeout(fn){scheduled.set(++serial,fn);return serial;},clearTimeout(id){scheduled.delete(id);},
        save(ctx,changes,done){sent.push(changes);callbacks.push(done);},saved(){},failed(e){throw e;}});
    function advance(){const batch=[...scheduled.values()];scheduled.clear();batch.forEach(fn=>fn());}
    paused.reset(context,{a:{x:0,y:0}});paused.enqueue({a:{x:10,y:0}});advance();
    paused.enqueue({a:{x:20,y:0}});paused.defer();callbacks.shift()({ok:true});advance();
    assert.equal(sent.length,1,"inflight ACK cannot restart persistence during a later active gesture");
    paused.enqueue({a:{x:30,y:0}});advance();assert.equal(sent.length,1,"enqueue also respects persistent gesture pause");
    paused.resume();advance();assert.equal(sent.length,2);assert.deepEqual(sent[1],[{id:"a",from:{x:10,y:0},to:{x:30,y:0}}]);
    callbacks.shift()({ok:true});paused.defer();paused.enqueue({a:{x:40,y:0}});
    let flushed=false;paused.flush(e=>{assert.equal(e,null);flushed=true;});assert.equal(sent.length,3,"explicit structural/history barrier can flush a paused queue");
    callbacks.shift()({ok:true});assert.equal(flushed,true);
}
console.log("panel_layout_queue_tests passed: 100-drag merge, serial latest-value flush, return-to-origin, timeout/no retry and stale context");
