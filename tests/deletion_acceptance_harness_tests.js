"use strict";
// Execute the actual new acceptance functions, with mocked File/render queue.
const fs=require("fs"),path=require("path"),vm=require("vm"),assert=require("node:assert/strict");
const parser=require("internal/deps/acorn/acorn/dist/acorn"),root=path.resolve(__dirname,"..");
const text=fs.readFileSync(path.join(root,"tools/Run-ModelAcceptance.jsx"),"utf8");
const ast=parser.parse(text,{ecmaVersion:3,allowReserved:"never"});
const functions=ast.body[0].expression.callee.body.body.filter(n=>n.type==="FunctionDeclaration");
const code=["readNativeJournal","renderAcceptanceFrame"].map(name=>{const n=functions.find(n=>n.id.name===name);assert.ok(n);return text.slice(n.start,n.end);}).join("\n");
let checks=0;function check(v,message){checks++;assert.ok(v,message);}
function fixture(mode="pass") {
 let journal="",removed=0,created=0,rendered=0,items=mode==="busy"?[{owner:true}]:[],output=null;
 const run={index:0,nativeJournalOffset:0,nativeJournal:{frames:0,canceled:0,errors:[]}};
 function File(name){this.name=name;this.position=0;this.parent={fsName:"/owned"};}
 Object.defineProperty(File.prototype,"exists",{get(){return this.name==="journal" || Boolean(this.done);}});
 Object.defineProperty(File.prototype,"length",{get(){return this.name==="journal"?journal.length:this.done?99:0;}});
 File.prototype.open=()=>true;File.prototype.close=()=>{};File.prototype.seek=function(pos){this.position=pos;return true;};
 File.prototype.read=function(){const result=journal.slice(this.position);this.position=journal.length;return result;};File.prototype.tell=function(){return this.position;};
 const queue={get numItems(){return items.length;},items:{add(){created++;const item={status:0,outputModule:()=>({set file(v){output=v;}}),remove(){removed++;items.splice(items.indexOf(item),1);}};items.push(item);return item;}},
 render(){rendered++;if(mode!=="empty")journal+=JSON.stringify({nativeBuild:mode==="old-build"?75:76,result:mode==="native-error"?516:0,canceled:false})+"\n";
  items[0].status=mode==="render-error"?2:1;output.done=mode!=="render-error";}};
 const context=vm.createContext({File,app:{project:{renderQueue:queue}},run,fixture:{renderFrames:true,nativeJournal:"journal",nativeBuild:76},config:{result:"result"},comp:{frameRate:24},RQItemStatus:{DONE:1}});
 vm.runInContext(code,context);
 return {context,run,append(value){journal+=value;},stats:()=>({removed,created,rendered,items:items.length})};
}
{
 const f=fixture();vm.runInContext("renderAcceptanceFrame()",f.context);check(f.run.nativeJournal.frames===1,"actual journal tracks rendered activity");
 check(JSON.stringify(f.stats())===JSON.stringify({removed:1,created:1,rendered:1,items:0}),"only owned render item created and removed");
 f.append(JSON.stringify({nativeBuild:76,result:1,canceled:true})+"\n");vm.runInContext("readNativeJournal()",f.context);
 check(f.run.nativeJournal.frames===2 && f.run.nativeJournal.canceled===1 && !f.run.nativeJournal.errors.length,"explicit cancel is separate from native failure");
 vm.runInContext("readNativeJournal()",f.context);check(f.run.nativeJournal.frames===2,"journal offsets do not replay rows");
 f.append('{"nativeBuild":76,"result":0');vm.runInContext("readNativeJournal()",f.context);check(f.run.nativeJournal.frames===2,"partial tail is retained");
 f.append(',"canceled":false}\n');vm.runInContext("readNativeJournal()",f.context);check(f.run.nativeJournal.frames===3,"complete tail read once");
}
for(const mode of ["native-error","old-build","render-error","busy"]) {
 const f=fixture(mode);assert.throws(()=>vm.runInContext("renderAcceptanceFrame()",f.context));checks++;
 check(f.stats().items===(mode==="busy"?1:0),"failure cleanup preserves owner queue");
 if(mode==="busy")check(f.stats().created===0 && f.stats().rendered===0,"owner queue rejects before mutation");
 if(mode==="native-error")check(f.run.nativeJournal.errors.length===1,"suppressed UI cannot hide native error from results");
}
const f=fixture("empty");vm.runInContext("renderAcceptanceFrame()",f.context);check(f.run.nativeJournal.frames===0,"missing activity cannot qualify host (finish rejects it)");
console.log(`deletion_acceptance_harness_tests: ${checks} checks passed; actual ES3 functions, native log/error/cancel/tail, render failure and queue ownership; no AE run`);
