// Loaded only for an explicit preset file operation, after starfield_gateway.jsx.
(function(){
    var host=$.global,api=host.__SFLD_presetFileAPI;
    if(!api)throw new Error("Preset file gateway is unavailable.");
    var KEY="__SFLD_presetFileSessionV1",PAGE=32768,MAX_TEXT=128*1024*1024+256*1024,MAX_BYTES=128*1024*1024+1024*1024;
    function integer(n,min,max){return typeof n==="number" && isFinite(n) && Math.floor(n)===n && n>=min && n<=max;}
    function request(text,operation){var r=api.parse(text);return r && r.operation===operation &&
        typeof r.transferId==="string" && /^[0-9a-f]{32}$/.test(r.transferId) && !/^0{32}$/.test(r.transferId)?r:null;}
    function bad(message){return api.fail("invalid_request",message || "Invalid preset file transfer request.");}
    function session(r,kind){var s=host[KEY];if(!s || s.id!==r.transferId || s.kind!==kind)throw new Error("Preset file transfer is not current.");
        if(new Date().getTime()>s.expires){host[KEY]=null;throw new Error("Preset file transfer expired.");}s.expires=new Date().getTime()+300000;return s;}
    function available(){var s=host[KEY];if(s && new Date().getTime()<=s.expires)throw new Error("Another preset file transfer is active.");host[KEY]=null;}
    function fileError(error){return api.fail("preset_file_error",error.toString());}
    function active(id){var s=host[KEY];return s && s.id===id && new Date().getTime()<=s.expires?s:null;}
    function queued(s){s.stage="queued";s.expires=new Date().getTime()+300000;
        var command=app.findMenuCommandId("Starfield Choose Preset File");
        if(!command)throw new Error("The paired Starfield Host file chooser is unavailable.");
        app.executeCommand(command);return api.reply({ok:true,transferId:s.id,state:"queued"});}
    function retain(s,response){s.result=response;s.stage="ready";s.expires=new Date().getTime()+300000;return "1";}
    function pathFromHex(hex){if(typeof hex!=="string" || !hex.length || hex.length>32767*4 || hex.length%4 || !/^[0-9a-f]+$/.test(hex))throw new Error("Invalid preset file path.");
        var path="",i,c;for(i=0;i<hex.length;i+=4){c=parseInt(hex.substr(i,4),16);if(c<32)throw new Error("Invalid preset file path.");path+=String.fromCharCode(c);}
        if(!/^[a-z]:[\\\/]/i.test(path) && path.substr(0,2)!=="\\\\")throw new Error("Preset file path must be absolute.");
        for(i=0;i<path.length;i++){c=path.charCodeAt(i);if(c>=0xd800 && c<=0xdbff){if(++i===path.length || path.charCodeAt(i)<0xdc00 || path.charCodeAt(i)>0xdfff)throw new Error("Invalid preset path encoding.");}
            else if(c>=0xdc00 && c<=0xdfff)throw new Error("Invalid preset path encoding.");}return path;}
    function validate(text){
        if(!text || text.length>MAX_TEXT)throw new Error("Preset file exceeds the supported size.");
        var data=JSON.parse(text);
        if(!data || data.format!=="org.starfieldfx.preset" || (data.version!==1 && data.version!==2 && data.version!==3) ||
            typeof data.name!=="string" || !data.name || data.name.length>120 || typeof data.graphHex!=="string" ||
            !data.graphHex || data.graphHex.length>api.maxGraphBytes*2 || data.graphHex.length%2 || !/^[0-9a-f]+$/i.test(data.graphHex))throw new Error("Unsupported preset data.");
        if(data.version!==3 && text.length>256*1024)throw new Error("Legacy preset metadata exceeds the file limit.");
        if(data.version===3 && (Object.prototype.toString.call(data.modelAssets)!=="[object Array]" || data.modelAssets.length>256))throw new Error("Invalid portable Model asset inventory.");
        return data;
    }
    function read(file){var opened=false;try{if(file.length>MAX_BYTES)throw new Error("Preset file exceeds the supported size.");
        file.encoding="UTF-8";if(!file.open("r"))throw new Error(file.error || "Unable to read preset.");opened=true;
        var text=file.read();if(file.error)throw new Error(file.error);return text;
    }finally{if(opened && !file.close())throw new Error(file.error || "Unable to close preset.");}}
    host.SFLD_beginPresetFileRead=function(text){var r=request(text,"beginPresetFileRead");if(!r)return bad();
        try{available();var s={id:r.transferId,kind:"read",expires:0};host[KEY]=s;return queued(s);}
        catch(error){if(s && host[KEY]===s)host[KEY]=null;return fileError(error);}};
    host.SFLD_readPresetFilePage=function(text){var r=request(text,"readPresetFilePage");if(!r || !integer(r.page,0,Math.ceil(MAX_TEXT/(PAGE-1))))return bad();
        try{var s=session(r,"read");if(s.stage!=="ready" || !s.text || r.page!==s.page || s.offset===s.characters)return bad("Preset page is out of sequence.");
            var end=Math.min(s.characters,s.offset+PAGE);if(end<s.characters && s.text.charCodeAt(end-1)>=0xd800 && s.text.charCodeAt(end-1)<=0xdbff)end--;
            var part=s.text.substring(s.offset,end);s.offset=end;s.page++;
            return api.reply({ok:true,transferId:s.id,page:r.page,text:part,characters:s.characters,path:s.path,complete:end===s.characters});
        }catch(error){return fileError(error);}};
    host.SFLD_beginPresetFileWrite=function(text){var r=request(text,"beginPresetFileWrite");if(!r || !integer(r.characters,1,MAX_TEXT))return bad();
        try{available();host[KEY]={id:r.transferId,kind:"write",stage:"uploading",characters:r.characters,pages:[],received:0,page:0,expires:new Date().getTime()+300000};
            return api.reply({ok:true,transferId:r.transferId});}catch(error){return fileError(error);}};
    host.SFLD_writePresetFilePage=function(text){var r=request(text,"writePresetFilePage");
        if(!r || !integer(r.page,0,Math.ceil(MAX_TEXT/(PAGE-1))) || typeof r.text!=="string" || !r.text.length || r.text.length>PAGE)return bad();
        try{var s=session(r,"write");if(s.stage!=="uploading" || r.page!==s.page || r.text.length+s.received>s.characters)return bad("Preset page is out of sequence or too long.");
            s.pages.push(r.text);s.received+=r.text.length;s.page++;
            return api.reply({ok:true,transferId:s.id,page:r.page,received:s.received});}catch(error){return fileError(error);}};
    host.SFLD_finishPresetFileWrite=function(text){var r=request(text,"finishPresetFileWrite");if(!r)return bad();
        try{var s=session(r,"write");if(s.stage!=="uploading")return bad("Preset chooser is already queued.");
            if(s.received!==s.characters)return bad("Preset file upload is incomplete.");s.text=s.pages.join("");validate(s.text);s.pages=[];return queued(s);
        }catch(error){if(host[KEY] && host[KEY].id===r.transferId)host[KEY]=null;return fileError(error);}};
    host.SFLD_readPresetFileModal=function(text){var r=request(text,"readPresetFileModal");if(!r)return bad();var s=active(r.transferId);
        if(!s)return fileError(new Error("Preset file transfer is not current or expired."));
        return s.stage==="ready"?s.result:api.reply({ok:true,transferId:s.id,state:s.stage});};
    host.SFLD_presetFileHostRequest=function(){var s=host[KEY];if(!s || !active(s.id) || s.stage!=="queued")return "0";
        return s.id+"|"+(s.kind==="read"?"1":"2");};
    host.SFLD_presetFileHostBegin=function(id){var s=active(id);if(!s || s.stage!=="queued")return "0";s.stage="choosing";return "1";};
    host.SFLD_presetFileHostValidate=function(id){var s=active(id);return s && s.stage==="choosing"?"1":"0";};
    host.SFLD_presetFileHostComplete=function(id,hex,choice){var s=active(id);if(!s || s.stage!=="choosing")return "0";
        if(choice===1)return retain(s,api.reply({ok:true,transferId:id,cancelled:true}));
        if(choice!==0)return retain(s,fileError(new Error("Native preset file chooser failed.")));
        try{var path=pathFromHex(hex);if(s.kind==="read"){var file=new File(path),content=read(file);validate(content);
            s.text=content;s.characters=content.length;s.path=file.fsName;s.page=0;s.offset=0;
            return retain(s,api.reply({ok:true,transferId:id,characters:s.characters,path:s.path}));}
            if(!/\.sfldpreset$/i.test(path))throw new Error("Invalid preset destination suffix.");
            return retain(s,writeSelected(s,path));
        }catch(error){return retain(s,fileError(error));}};
    function writeSelected(s,path){
        var temporary=null,backup=null,destination=null,opened=false,created=false,moved=false,published=false,originalName="";
        try{
            var content=s.text;destination=new File(path);originalName=destination.name;
            temporary=new File(path+".starfield-"+s.id+".tmp");backup=new File(path+".starfield-"+s.id+".bak");
            if(temporary.exists || backup.exists)throw new Error("Preset transaction path already exists.");
            temporary.encoding="UTF-8";if(!temporary.open("w"))throw new Error(temporary.error || "Unable to prepare preset.");opened=true;created=true;
            if(!temporary.write(content))throw new Error(temporary.error || "Unable to write preset.");
            if(!temporary.close())throw new Error(temporary.error || "Unable to close preset.");opened=false;
            if(read(temporary)!==content)throw new Error("Preset file verification failed.");
            if(destination.exists){if(!destination.rename(backup.name))throw new Error(destination.error || "Unable to retain previous preset.");moved=true;}
            if(!temporary.rename(originalName))throw new Error(temporary.error || "Unable to publish preset.");published=true;
            var retained=null;
            if(moved)try{if(!backup.remove())retained=backup.fsName;}catch(cleanupError){retained=backup.fsName;}
            return api.reply({ok:true,transferId:s.id,path:path,retainedBackup:retained});
        }catch(error){
            var rollback=null,staging=null;
            if(opened && temporary)try{if(temporary.close())opened=false;}catch(closeError){}
            if(moved && !published && backup)try{if(!backup.rename(originalName))rollback=backup.fsName;}catch(restoreError){rollback=backup.fsName;}
            if(temporary && !published && created && temporary.exists)try{if(!temporary.remove())staging=temporary.fsName;}catch(removeError){staging=temporary.fsName;}
            return api.fail("preset_file_error",error.toString()+(rollback?" Previous preset remains at "+rollback:"")+
                (staging?" Unpublished file remains at "+staging:""),rollback || staging?{retainedBackup:rollback,retainedTemporary:staging}:null);
        }finally{
            if(opened && temporary)try{temporary.close();}catch(closeError){}
        }
    }
    host.SFLD_releasePresetFile=function(text){var r=request(text,"releasePresetFile");if(!r)return bad();
        if(host[KEY] && host[KEY].id===r.transferId)host[KEY]=null;return api.reply({ok:true,transferId:r.transferId});};
})();
