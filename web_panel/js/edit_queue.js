(function(root,factory){var api=factory();if(typeof module==="object" && module.exports)module.exports=api;else root.StarfieldEditQueue=api;}(typeof window!=="undefined"?window:this,function(){
    "use strict";
    function failure(code,message){return {ok:false,error:{code:code,message:message}};}
    function links(graph,id){return graph.edges.filter(function(e){return e.sourceNode===id;}).map(function(e){return {target:e.destinationNode,id:e.id};}).sort(function(a,b){return a.id<b.id?-1:a.id>b.id?1:0;});}
    function delta(base,graph,previous){
        var old={},next={},removed=[],rows=[];
        previous.nodes.forEach(function(n){old[n.id]=n;});graph.nodes.forEach(function(n){next[n.id]=n;});
        for(var id in next)if(!old[id] || old[id].type!==next[id].type || old[id].schemaVersion!==next[id].schemaVersion || JSON.stringify(old[id].parameters)!==JSON.stringify(next[id].parameters))return null;
        for(var key in old)if(!next[key])removed.push(key);
        (base.nativeNodes || []).forEach(function(n){if(!next[n.id])return;var wanted=links(graph,n.id),expected=n.outgoing.map(function(e){return {target:e.target,id:e.id};});
            if(JSON.stringify(wanted)!==JSON.stringify(expected.slice().sort(function(a,b){return a.id<b.id?-1:a.id>b.id?1:0;})))rows.push({source:n.id,expected:expected,desired:wanted});});
        return {removed:removed,rows:rows};
    }
    function create(options){
        var current=null,serial=0,max=64;
        function active(s){return current===s && (!options.isCurrent || options.isCurrent(s.token,s.scope,s.epoch));}
        function pending(s){return s.waiting.length+(s.batch?s.batch.items.length:0);}
        function apply(graph,item,geometry){var at=0;
            // Defaults use the receipt being replayed, as in the addition-only
            // queue. A later ACK may have a different layer-height unit context.
            item.edit.layerHeightPixels=geometry?Number(geometry.height):1;
            if(item.edit.type==="duplicateNodes")delete item.edit.modelResourceCopies;return options.edits.apply(graph,item.edit,function(){if(at<item.ids.length)return item.ids[at++];var id=options.edits.randomId();item.ids.push(id);at++;return id;});}
        function preview(s){var graph=options.codec.fromHex(s.confirmed.snapshot.graphHex);(s.batch?s.batch.items:[]).concat(s.waiting).forEach(function(item){graph=apply(graph,item,s.confirmed.snapshot.geometry);});return graph;}
        function publish(s){if(active(s) && options.onView)options.onView(preview(s),s.confirmed.snapshot,{pending:pending(s)>0,count:pending(s),targetToken:s.token,epoch:s.epoch});}
        function stop(s,result){if(!active(s))return;var dropped=pending(s);current=null;s.waiting=[];s.batch=null;options.onError(result,{targetToken:s.token,cancelled:dropped});}
        function resourceCopy(graph,item){
            if(item.edit.type!=="duplicateNodes")return false;
            return graph.nodes.some(function(node){
                if(node.type!==options.edits.types.model || item.edit.nodeIds.indexOf(node.id)<0)return false;
                return node.parameters.some(function(p){return String(p.key)==="1" && p.value && typeof p.value.some==="function" && p.value.some(function(byte){return byte!==0;});});
            });
        }
        function pump(s){if(!active(s) || s.batch || s.scheduled || !s.waiting.length)return;s.scheduled=true;
            options.schedule(function(){s.scheduled=false;if(!active(s) || s.batch || !s.waiting.length)return;
                var count=0,base=s.confirmed,graph=options.codec.fromHex(base.snapshot.graphHex);
                try{while(count<s.waiting.length && count<32){
                    var item=s.waiting[count],resource=resourceCopy(graph,item);
                    if(resource && count)break;
                    graph=apply(graph,item,base.snapshot.geometry);count++;
                    // Only an imported mesh copy needs a confirmed source asset
                    // before the following copy. Ordinary copies batch freely.
                    if(resource)break;
                }}catch(e){stop(s,failure(e.code || "invalid_edit",e.message || String(e)));return;}
                var batch={items:s.waiting.splice(0,count),base:base};s.batch=batch;
                var settled=false;
                function done(result){if(settled)return;settled=true;if(!active(s) || s.batch!==batch)return;
                    if(!result || !result.ok || !result.snapshot || !result.snapshot.initialized || !result.target || result.target.token!==s.token || (result.snapshot.projectScope || "")!==s.scope ||
                        (result.noOp?result.snapshot.revision!==batch.base.snapshot.revision:result.snapshot.revision<=batch.base.snapshot.revision)){stop(s,result && !result.ok?result:failure("graph_snapshot_unconfirmed","The edit receipt is not current. Refresh before continuing."));return;}
                    s.confirmed=result;s.batch=null;
                    try{publish(s);}catch(e){stop(s,failure(e.code || "stale_graph",e.message || String(e)));return;}
                    if(!s.waiting.length){current=null;if(options.onSettled)options.onSettled(result);}else pump(s);
                }
                try{options.submit(batch.base,graph,done,batch.items.map(function(item){return item.edit;}));}catch(e){done(failure("graph_edit_outcome_unknown",String(e)));}
            });
        }
        function enqueue(edit,initial,epoch){
            var s=current,created=false;
            try{
                if(initial && initial.snapshot && initial.snapshot.canvasOnly)return failure("graph_snapshot_unconfirmed","Load complete controls before editing this canvas.");
                if(!s){if(!initial || !initial.ok || !initial.target || !initial.snapshot || !initial.snapshot.initialized) return failure("graph_snapshot_unconfirmed","Refresh the graph before editing.");
                    s={id:++serial,token:initial.target.token,scope:initial.snapshot.projectScope || "",epoch:epoch,confirmed:initial,waiting:[],batch:null,scheduled:false};created=true;}
                if(!active(s) && !created)return failure("stale_target","The graph target changed.");
                if(pending(s)>=max)return failure("edit_queue_full","The editor is catching up. Wait for pending edits to finish.");
                var item={edit:JSON.parse(JSON.stringify(edit)),ids:[]};item.edit.layerHeightPixels=s.confirmed.snapshot.geometry?Number(s.confirmed.snapshot.geometry.height):1;
                var graph=preview(s);apply(graph,item,s.confirmed.snapshot.geometry); // reject before accepting/painting an intent
                s.waiting.push(item);current=s;publish(s);pump(s);return {ok:true};
            }catch(e){return failure(e.code || "invalid_edit",e.message || String(e));}
        }
        return {enqueue:enqueue,busy:function(){return !!current;},canAccept:function(){return !current || pending(current)<max;},cancel:function(){current=null;serial++;}};
    }
    return {create:create,delta:delta};
}));
