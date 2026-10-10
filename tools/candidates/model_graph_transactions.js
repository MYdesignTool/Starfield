(function(root,factory){var assets=typeof module==="object" && module.exports?require("./model_assets.js"):root.StarfieldModelAssets;
    var api=factory(assets);if(typeof module==="object" && module.exports)module.exports=api;else root.StarfieldModelGraphTransactions=api;
}(typeof window!=="undefined"?window:this,function(assets){
    "use strict";
    function failure(code,message){return {ok:false,error:{code:code,message:message}};}
    function create(options){
        options=options || {};if(typeof options.call!=="function")throw new Error("Model graph transaction needs a gateway call.");
        var schedule=options.schedule || function(fn,delay){return setTimeout(fn,delay);},clock=options.now || Date.now;
        var idFactory=options.idFactory || function(){var id="";for(var i=0;i<32;i++)id+=Math.floor(Math.random()*16).toString(16);return id;};
        function apply(transaction,meshes,callback){
            if(typeof callback!=="function")throw new Error("Model transaction callback is required.");
            var id=idFactory(),token=transaction && transaction.target && transaction.target.token,descriptors=[],finished=false,started=false,queued=false,
                index=0,page=0,deadline=clock()+5*60*1000,published=null;
            function fields(extra){var value={transactionId:id,pinTarget:true,target:{token:token}};
                Object.keys(extra || {}).forEach(function(key){value[key]=extra[key];});return value;}
            function end(response){if(finished)return;finished=true;
                if(!started){callback(response);return;}
                options.call("releaseModelGraphTransaction",fields(),function(){callback(response);});}
            function error(response){end(response && response.error?response:failure("model_transaction_failed","No Model transaction response."));}
            try{
                if(typeof id!=="string" || !/^[0-9a-f]{32}$/.test(id) || /^0{32}$/.test(id) || id==="000000000000000000000000000000ff")
                    throw new Error("Invalid Model transaction ID (kind="+typeof id+", value="+String(id)+").");
                if(typeof token!=="string" || !/^p(0|[1-9][0-9]*)-c[1-9][0-9]*-l[1-9][0-9]*$/.test(token))
                    throw new Error("Invalid Model target identity (kind="+typeof token+", token="+String(token)+").");
                if(!Array.isArray(meshes) || meshes.length>63)
                    throw new Error("Invalid Model transaction asset list (kind="+typeof meshes+", count="+
                        (Array.isArray(meshes)?meshes.length:"not an array")+").");
                var total=0,seen={};
                meshes.forEach(function(mesh){
                    if(!mesh || typeof mesh.nodeId!=="string" || !/^[0-9a-f]{32}$/.test(mesh.nodeId) || seen[mesh.nodeId] ||
                        (mesh.source!==1 && mesh.source!==2) || typeof mesh.revision!=="number" || Math.floor(mesh.revision)!==mesh.revision ||
                        mesh.revision<1 || mesh.revision>2147483647 || typeof mesh.meshHex!=="string" || mesh.meshHex.length/2>assets.maxTotalBytes-total)
                        throw new Error("Invalid Model restore asset.");
                    var geometry=assets.validateMesh(mesh.meshHex);total+=geometry.bytes;
                    if(JSON.stringify(geometry.bounds)!==JSON.stringify(mesh.bounds))throw new Error("Model restore bounds do not match its mesh.");
                    seen[mesh.nodeId]=true;descriptors.push({nodeId:mesh.nodeId,source:mesh.source,revision:mesh.revision,bytes:geometry.bytes,bounds:mesh.bounds.slice()});
                });
            }catch(exception){end(failure("invalid_model_transaction",exception.message || String(exception)));return {cancel:function(){}};}
            var begin=fields(transaction);begin.transactionId=id;begin.pinTarget=true;begin.assets=descriptors;delete begin.baseNodeManifest;
            options.call("beginModelGraphTransaction",begin,function(response){
                if(finished){if(response && response.ok && response.transactionId===id)options.call("releaseModelGraphTransaction",fields(),function(){});return;}
                if(!response || !response.ok || response.transactionId!==id){error(response);return;}
                started=true;schedule(upload,0);
            });
            function expired(){if(clock()<=deadline)return false;
                if(published){published.diagnostics=published.diagnostics || {};published.diagnostics.notificationPending=true;}
                end(published || failure(queued?"model_transaction_outcome_unknown":"model_transaction_timeout",
                    queued?"Model transaction did not return its final result. Refresh the graph before retrying.":"Model transaction upload timed out."));return true;}
            function upload(){if(finished || expired())return;
                if(index===meshes.length){
                    // A lost queue acknowledgement cannot be treated as a safe
                    // retry: the Host may already own this mutation.
                    queued=true;options.call("queueModelGraphTransaction",fields(),function(){if(!finished)schedule(read,0);});return;}
                var hex=meshes[index].meshHex.substr(page*65536,65536),expectedAsset=index,expectedPage=page;
                options.call("writeModelGraphAssetPage",fields({asset:index,page:page,hex:hex}),function(response){
                    if(finished)return;if(!response || !response.ok || response.transactionId!==id || response.asset!==expectedAsset || response.page!==expectedPage){error(response);return;}
                    ++page;if(page*65536>=meshes[index].meshHex.length){++index;page=0;}schedule(upload,0);
                });
            }
            function read(){if(finished || expired())return;
                options.call("readModelGraphTransaction",fields(),function(response){
                    if(finished)return;if(!response || !response.ok || response.transactionId!==id){
                        error(response && response.error?response:failure("model_transaction_outcome_unknown","No final Model graph response. Refresh before retrying."));return;}
                    if(response.state==="complete"){end(response.result || failure("model_transaction_outcome_unknown","The completed transaction has no result."));return;}
                    if(response.state==="published"){
                        if(!response.result || response.result.committed!==true){error(failure("model_transaction_outcome_unknown","Invalid published Model result."));return;}
                        published=response.result;
                    }
                    schedule(read,50);
                });
            }
            return {cancel:function(){end(published || failure(queued?"model_transaction_cancel_pending":"cancelled",
                queued?"Cancellation requested; refresh the graph to confirm the outcome.":"Model graph transaction cancelled."));}};
        }
        return {apply:apply};
    }
    return {create:create};
}));
