// Cosmetic metadata only. No graph publication or render receipt is invented.
(function(root,factory){
    if(typeof module==="object" && module.exports)module.exports=factory();
    else root.StarfieldGraphLayoutQueue=factory();
}(this,function(){
    "use strict";
    function copy(p){return {x:Number(p.x),y:Number(p.y)};}
    function same(a,b){return a && b && Math.abs(a.x-b.x)<0.001 && Math.abs(a.y-b.y)<0.001;}
    function create(options){
        var baseline={},wanted={},context=null,timer=null,busy=false,paused=false,generation=0,waiters=[];
        function stop(){if(timer!==null)options.clearTimeout(timer);timer=null;}
        function hasWork(){return busy || Object.keys(wanted).length>0;}
        function settle(error){var list=waiters;waiters=[];list.forEach(function(fn){fn(error);});}
        function send(forced){
            stop();if(busy || (paused && forced!==true))return;
            var changes=[];
            Object.keys(wanted).forEach(function(id){
                if(!same(baseline[id],wanted[id]))changes.push({id:id,from:copy(baseline[id]),to:copy(wanted[id])});
            });
            wanted={};
            if(!changes.length){settle(null);return;}
            var epoch=generation,sentContext=context;busy=true;
            options.save(sentContext,changes,function(result){
                if(epoch!==generation)return;
                busy=false;
                if(!result || !result.ok){
                    wanted={};var error=result && result.error || {code:"layout_save_failed",message:"Node positions could not be saved. Refresh before continuing."};
                    settle(error);options.failed(error);return;
                }
                changes.forEach(function(change){baseline[change.id]=copy(change.to);});
                options.saved(result);
                if(Object.keys(wanted).length && waiters.length)send(true);
                else if(Object.keys(wanted).length && !paused)timer=options.setTimeout(send,options.delay || 600);
                else settle(null);
            });
        }
        return {
            reset:function(next,positions){
                stop();generation++;busy=false;paused=false;wanted={};baseline={};context=next;
                Object.keys(positions || {}).forEach(function(id){baseline[id]=copy(positions[id]);});
                settle({code:"layout_context_changed",message:"The graph target changed before its layout was saved."});
            },
            adopt:function(next,positions){
                // Our own topology receipt may add/remove members. Preserve
                // gesture pause and queued positions; never cancel a host write.
                if(busy || !context || !next || context.token!==next.token || context.projectScope!==next.projectScope)return false;
                stop();var updated={};Object.keys(positions || {}).forEach(function(id){updated[id]=copy(positions[id]);});baseline=updated;
                Object.keys(wanted).forEach(function(id){if(!baseline[id] || same(baseline[id],wanted[id]))delete wanted[id];});
                if(Object.keys(wanted).length && !paused)timer=options.setTimeout(send,options.delay || 600);
                return true;
            },
            enqueue:function(positions){
                if(!context)return;
                Object.keys(positions).forEach(function(id){if(baseline[id])wanted[id]=copy(positions[id]);});
                // Keep only changed positions; a drag back to its origin is free.
                if(!busy)Object.keys(wanted).forEach(function(id){if(same(baseline[id],wanted[id]))delete wanted[id];});
                stop();if(Object.keys(wanted).length && !paused)timer=options.setTimeout(send,options.delay || 600);
            },
            flush:function(callback){waiters.push(callback);send(true);},
            defer:function(){paused=true;stop();},
            resume:function(){paused=false;if(!busy && Object.keys(wanted).length){stop();timer=options.setTimeout(send,options.delay || 600);}},
            hasWork:hasWork
        };
    }
    return {create:create};
}));
