"use strict";
const assert=require("node:assert/strict"),path=require("node:path");
const api=require(path.resolve(process.argv[2] || "tools/candidates/model_graph_transactions.js"));
let checks=0;
function check(value,message){++checks;assert(value,message);}
function scenario(mode){
    let now=0,queues=0,reads=0,releases=0,completions=0,result,completed=false,aborted=false;
    const tasks=[],delays=[];
    const client=api.create({now:()=>now,idFactory:()=>"1".repeat(32),schedule:(fn,delay)=>{tasks.push(fn);delays.push(delay);},
        call:(operation,fields,callback)=>{
            if(operation==="beginModelGraphTransaction"){callback({ok:true,transactionId:fields.transactionId});return;}
            if(operation==="queueModelGraphTransaction"){++queues;callback({ok:false,error:{code:"host_timeout"}});return;}
            if(operation==="readModelGraphTransaction"){
                ++reads;
                if(mode==="permanent"){callback({ok:false,error:{code:"model_transaction_missing",message:"missing"}});return;}
                if(mode==="deadline" || reads<=3){now+=8000;callback({ok:false,error:{code:"host_timeout",message:"8 seconds"}});return;}
                completed=true;callback({ok:true,transactionId:fields.transactionId,state:"complete",result:{ok:true,committed:true}});return;
            }
            if(operation==="releaseModelGraphTransaction"){++releases;if(!completed)aborted=true;callback({ok:true});return;}
            throw Error(operation);
        }});
    const pending=client.apply({target:{token:"p0-c1-l29"}},[],response=>{++completions;result=response;});
    let loops=0;while(tasks.length){check(++loops<100,"retry is bounded");tasks.shift()();}
    check(queues===1,"a timed-out queue request must not repeat the native edit");
    check(releases===1 && completions===1,"release and final callback are delivered once");
    pending.cancel();check(releases===1 && completions===1,"late cancellation cannot repeat completion");
    if(mode==="recover"){
        check(result && result.ok && result.committed,"recover the committed receipt after slow transport");
        check(!aborted && reads===4,"do not cancel the applying native command on an 8s read timeout");
        check(delays.filter(delay=>delay===1000).length===3,"retry status queries with backoff");
    }else if(mode==="deadline"){
        check(result.error.code==="model_transaction_outcome_unknown","overall deadline reports an unknown outcome");
        check(now>300000 && aborted,"only the existing overall deadline requests cancellation");
    }else{
        check(result.error.code==="model_transaction_missing" && reads===1,"permanent errors retain their existing failure path");
    }
}
for(const mode of ["recover","deadline","permanent"])scenario(mode);
console.log(`model_graph_timeout_tests: ${checks} checks passed; actual client, delayed transport, single native edit`);
