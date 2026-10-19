// Optional asset/backup transport is loaded when an operation requires it.
(function(root,factory){
    if(typeof module==="object" && module.exports)module.exports={create:factory};
    else root.StarfieldOptionalModules=factory(root);
}(typeof window!=="undefined"?window:this,function(root){
    "use strict";
    var waiting=null,generation=null;
    function modules(){return {ok:true,assets:root.StarfieldModelAssets,transactions:root.StarfieldModelGraphTransactions};}
    function available(name){var value=root[name];return value && typeof value.create==="function";}
    function loadModel(version,callback){
        if(typeof callback!=="function")throw new Error("Optional module callback is required.");
        if(!/^\d+$/.test(String(version))){callback({ok:false,error:{code:"module_generation_invalid",message:"Invalid optional module generation."}});return;}
        if(waiting){
            if(generation!==String(version)){callback({ok:false,error:{code:"module_generation_mismatch",message:"Optional module generation changed."}});return;}
            waiting.push(callback);return;
        }
        if(available("StarfieldModelAssets") && available("StarfieldModelGraphTransactions")){callback(modules());return;}
        waiting=[callback];generation=String(version);
        var scripts=[["model_assets","StarfieldModelAssets"],["model_graph_transactions","StarfieldModelGraphTransactions"]],index=0;
        function finish(response){var listeners=waiting;waiting=null;generation=null;
            listeners.forEach(function(listener){listener(response);});}
        function next(){
            if(index===scripts.length){finish(modules());return;}
            var entry=scripts[index++];if(available(entry[1])){next();return;}
            var script=root.document.createElement("script"),settled=false,timer;
            function complete(error){
                if(settled)return;settled=true;root.clearTimeout(timer);script.onload=script.onerror=null;
                if(error || !available(entry[1])){
                    if(script.parentNode)script.parentNode.removeChild(script);
                    finish({ok:false,error:{code:"optional_module_load_failed",message:"Could not load "+entry[0]+". Reopen the panel."}});
                }else next();
            }
            script.src="js/"+entry[0]+".js?v="+generation;script.async=false;
            script.onload=function(){complete(false);};script.onerror=function(){complete(true);};
            timer=root.setTimeout(function(){complete(true);},8000);
            try{root.document.head.appendChild(script);}catch(error){complete(true);}
        }
        next();
    }
    return {loadModel:loadModel};
}));
