// Explicit, bounded file transport; never used by panel polling.
(function(root,factory){var api=factory();if(typeof module==="object" && module.exports)module.exports=api;
    else root.StarfieldPresetFiles=api;}(typeof window!=="undefined"?window:this,function(){
    "use strict";
    var PAGE=32768,MAX_TEXT=128*1024*1024+256*1024;
    function integer(n,min,max){return typeof n==="number" && isFinite(n) && Math.floor(n)===n && n>=min && n<=max;}
    function validId(id){return typeof id==="string" && /^[0-9a-f]{32}$/.test(id) && !/^0{32}$/.test(id);}
    function create(options){
        if(!options || typeof options.call!=="function")throw new Error("Preset files need a gateway call.");
        var schedule=options.schedule || function(fn){setTimeout(fn,0);},now=options.now || Date.now;
        var idFactory=options.idFactory || function(){var id="";for(var i=0;i<32;i++)id+=Math.floor(Math.random()*16).toString(16);return id;};
        function transfer(text,callback){
            if(typeof callback!=="function" || (text!==null && (typeof text!=="string" || !text.length || text.length>MAX_TEXT)))throw new Error("Invalid preset file transfer.");
            var id=idFactory(),done=false,started=false,deadline=now()+300000,page=0,offset=0,parts=[],total=null,path=null;
            if(!validId(id))throw new Error("Invalid preset file transfer identity.");
            function release(response){if(done)return;done=true;parts=[];
                options.call("releasePresetFile",{transferId:id},function(){callback(response);});}
            function error(response){release(response && response.error?response:{ok:false,error:{code:"preset_file_transfer_failed",message:"Invalid preset file response."}});}
            function call(op,fields,next){
                if(done)return;if(started && now()>deadline){error({ok:false,error:{code:"preset_file_timeout",message:"Preset file transfer timed out."}});return;}
                fields.transferId=id;
                options.call(op,fields,function(response){if(done)return;
                    if(!response || !response.ok){error(response);return;}if(response.cancelled){release(response);return;}
                    if(response.transferId!==id){error(null);return;}deadline=now()+300000;next(response);});
            }
            function next(){
                if(text!==null){
                    if(offset===text.length){call("finishPresetFileWrite",{},function(response){release(response);});return;}
                    var end=Math.min(text.length,offset+PAGE);
                    // Keep a UTF-16 surrogate pair in one UTF-8 write page.
                    if(end<text.length && text.charCodeAt(end-1)>=0xd800 && text.charCodeAt(end-1)<=0xdbff)end--;
                    var chunk=text.slice(offset,end);
                    call("writePresetFilePage",{page:page,text:chunk},function(response){
                        if(response.page!==page || response.received!==end){error(null);return;}page++;offset=end;schedule(next);});
                }else{
                    call("readPresetFilePage",{page:page},function(response){
                        if(response.page!==page || response.characters!==total || response.path!==path ||
                            typeof response.text!=="string" || !response.text.length || response.text.length>PAGE || offset+response.text.length>total){error(null);return;}
                        parts.push(response.text);offset+=response.text.length;page++;
                        if(response.complete!== (offset===total)){error(null);return;}
                        if(response.complete){var loaded=parts.join("");release({ok:true,text:loaded,path:path});return;}schedule(next);
                    });
                }
            }
            call(text===null?"beginPresetFileRead":"beginPresetFileWrite",text===null?{}:{characters:text.length},function(response){
                started=true;
                if(text===null){if(!integer(response.characters,1,MAX_TEXT) || typeof response.path!=="string"){error(null);return;}total=response.characters;path=response.path;}
                next();
            });
            return {cancel:function(){release({ok:false,error:{code:"cancelled",message:"Preset file transfer cancelled."}});}};
        }
        return {save:function(text,callback){return transfer(text,callback);},load:function(callback){return transfer(null,callback);}};
    }
    return {create:create,maxTextCharacters:MAX_TEXT,pageCharacters:PAGE};
}));
