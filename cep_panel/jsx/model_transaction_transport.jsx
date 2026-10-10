// Candidate-only explicit graph/Model session. All saved data is plain and
// bounded. Native Host owns the only undo group, full backup and asset writes.
(function(){
    var host=typeof $!=="undefined" && $.global?$.global:this,api=host.__SFLD_modelTransactionAPI;
    if(!api)throw new Error("Load the paired Model gateway before its transaction transport.");
    var KEY="__SFLD_modelGraphTransactionV1",MODEL="org.starfieldfx.nodes.model",OUTPUT="000000000000000000000000000000ff";
    var PAGE=65536,MAX_MESH=8*1024*1024,MAX_TOTAL=64*1024*1024;
    function integer(value,min,max){return typeof value==="number" && isFinite(value) && Math.floor(value)===value && value>=min && value<=max;}
    function uuid(value){return typeof value==="string" && /^[0-9a-f]{32}$/.test(value) && !/^0{32}$/.test(value) && value!==OUTPUT;}
    function now(){return (new Date()).getTime();}
    function plain(value){return JSON.parse(JSON.stringify(value));}
    function failure(session,message){session.message=String(message);session.state="failed";session.assets=[];}
    function session(id){var value=host[KEY];if(!value || value.version!==1 || value.id!==id)return null;
        if(now()>value.expires && value.state!=="complete" && value.state!=="published" && value.state!=="applying")failure(value,"Model graph transaction expired.");
        return value;}
    function target(s,guard,base){
        if(s.cancelled || now()>s.expires)throw new Error("Model graph transaction cancelled or expired.");
        var resolved=api.resolve(s.request);if(resolved.error)throw new Error(resolved.error.message);
        if(Number(resolved.properties.guard.value)!==guard)throw new Error("The renderer is busy or its guard changed.");
        if(typeof resolved.target.layer.time==="number" && resolved.target.layer.time!==s.time)throw new Error("The target time changed during Model transfer.");
        if(base){var snapshot=api.snapshot(resolved);
            if(!snapshot.initialized || snapshot.revision<s.request.baseGraphRevision || snapshot.recordStamp!==s.request.baseRecordStamp)
                throw new Error("The graph changed during Model transfer; reload before editing.");}
        return resolved;
    }
    function parameter(node,key){for(var i=0;i<node.parameters.length;i++)if(String(node.parameters[i].key)===String(key))return node.parameters[i];return null;}
    function close(a,b,color){
        if(a instanceof Array && b instanceof Array){if(a.length!==b.length)return false;for(var i=0;i<a.length;i++)if(!close(a[i],b[i],color))return false;return true;}
        if(typeof a==="number" && typeof b==="number")return isFinite(a) && isFinite(b) && Math.abs(a-b)<=(color?1/255+1e-7:1e-6*Math.max(1,Math.abs(a)));
        return a===b;
    }
    function valueMatches(node,expected,actual){
        if(!actual || expected.type!==actual.type)return false;
        var key=String(expected.key),curve=node.type==="org.starfieldfx.nodes.particle" && (key==="7" || key==="8" || key==="27" || key==="13") ||
            node.type==="org.starfieldfx.nodes.force" && key==="9";
        if(expected.type===7 && curve){var a=expected.value,b=actual.value;
            if(!(a instanceof Array) || !(b instanceof Array) || a.length!==b.length || a.length<4)return false;
            for(var h=0;h<4;h++)if(a[h]!==b[h])return false;
            for(var offset=4;offset<a.length;offset+=8)if(!close(api.float64(a,offset),api.float64(b,offset),key==="13" && (offset-4)%32!==0))return false;
            return true;}
        return close(expected.value,actual.value,node.type==="org.starfieldfx.nodes.particle" && (key==="1" || key==="2"));
    }
    function records(expected,actual,ignoreModelAsset){
        if(expected.length!==actual.length)throw new Error("Saved node count differs.");var map={};
        for(var i=0;i<actual.length;i++){if(map[actual[i].id])throw new Error("Duplicate saved node identity.");map[actual[i].id]=actual[i];}
        for(var n=0;n<expected.length;n++){var node=expected[n],saved=map[node.id];
            if(!saved || saved.type!==node.type || saved.schemaVersion!==node.schemaVersion || !close(node.position.x,saved.position.x) || !close(node.position.y,saved.position.y) ||
                JSON.stringify(node.outgoing)!==JSON.stringify(saved.outgoing))throw new Error("Saved node identity, layout or connection differs.");
            for(var p=0;p<node.parameters.length;p++){var field=node.parameters[p];
                if(ignoreModelAsset && node.type===MODEL && /^(1|2|12|13)$/.test(String(field.key)))continue;
                if(node.type==="org.starfieldfx.nodes.transform" && String(field.key)==="7")continue;
                if(!valueMatches(node,field,parameter(saved,field.key)))throw new Error("Saved parameter differs: "+node.id+" / "+field.key);}
        }
    }
    function renderer(expected,actual){for(var key in expected)if(Object.prototype.hasOwnProperty.call(expected,key) && !close(expected[key],actual[key])){
        if(key==="position" && close(expected.position.x,actual.position.x) && close(expected.position.y,actual.position.y))continue;
        throw new Error("Saved renderer field differs: "+key);}}
    function validateAssets(nodes,assets){
        if(!(assets instanceof Array) || assets.length>63)throw new Error("Invalid Model asset count.");var map={},seen={},total=0,copy=[];
        for(var n=0;n<nodes.length;n++)map[nodes[n].id]=nodes[n];
        for(var i=0;i<assets.length;i++){var a=assets[i],node=map[a.nodeId];
            if(!uuid(a.nodeId) || !node || node.type!==MODEL || seen[a.nodeId] || !integer(a.source,1,2) || !integer(a.revision,0,2147483647) ||
                !integer(a.bytes,0,MAX_MESH) || (a.revision===0?a.bytes!==0:a.bytes<32) || a.bytes>MAX_TOTAL-total ||
                !(a.bounds instanceof Array) || a.bounds.length!==6)throw new Error("Invalid Model asset descriptor.");
            for(var axis=0;axis<6;axis++)if(typeof a.bounds[axis]!=="number" || !isFinite(a.bounds[axis]) || Math.abs(a.bounds[axis])>1e9 ||
                (axis>=3 && a.bounds[axis]<a.bounds[axis-3]))throw new Error("Invalid Model asset bounds.");
            var model=api.modelRecord(node);
            if(a.source!==parameter(node,13).value+1 || (model.imported && (model.revision!==a.revision || !close(model.bounds,a.bounds))))
                throw new Error("Model asset does not match its author record.");
            if(a.revision===0 && !close(a.bounds,[-.5,-.5,-.5,.5,.5,.5]))throw new Error("Invalid default Cube bounds.");
            seen[a.nodeId]=true;total+=a.bytes;copy.push({nodeId:a.nodeId,source:a.source,revision:a.revision,bytes:a.bytes,bounds:a.bounds.slice(),pages:[]});
        }return copy;
    }
    function parse(text,operation){var request=api.parse(text);if(!request || request.operation!==operation || request.pinTarget!==true ||
        !request.target || typeof request.target.token!=="string" || !uuid(request.transactionId))throw new Error("Invalid pinned Model transaction request.");return request;}
    function matching(text,operation){var request=parse(text,operation),s=session(request.transactionId);
        if(!s || s.request.target.token!==request.target.token)throw new Error("Model transaction is missing or belongs to another target.");return {s:s,request:request};}
    host.SFLD_beginModelGraphTransaction=function(text){try{
        var request=parse(text,"beginModelGraphTransaction"),old=host[KEY];
        if(old && old.version===1 && old.state!=="complete" && old.state!=="failed" && now()<=old.expires)return api.fail("model_transaction_busy","Another Model graph transaction is running.");
        if(!integer(request.baseGraphRevision,1,16777215) || typeof request.baseRecordStamp!=="string" || request.baseRecordStamp.length>256*1024 ||
            typeof request.graphHex!=="string" || request.graphHex.length<64 || request.graphHex.length>api.maxGraphBytes*2 || request.graphHex.length%2 ||
            !/^[0-9a-f]+$/.test(request.graphHex))throw new Error("Invalid Model graph planning receipt.");
        var nodes=api.validateNodes(request.nodeManifest);if(nodes.length>63)throw new Error("Too many graph nodes.");
        api.validateRenderer(request.rendererManifest);
        var resolved=api.resolve(request);if(resolved.error)return api.fail(resolved.error.code,resolved.error.message);
        api.validateTransform(nodes,resolved.target.layer);
        var s={version:1,id:request.transactionId,request:plain(request),assets:validateAssets(nodes,request.assets || []),state:"uploading",
            expires:now()+5*60*1000,time:resolved.target.layer.time,cancelled:false,uploadAsset:0,uploadPage:0};
        target(s,0,true);
        var command=app.findMenuCommandId("Starfield Apply Model Graph Transaction");
        if(!command)return api.fail("model_transaction_unavailable","Model graph edits require the paired Starfield Host build.");
        host[KEY]=s;return api.reply({ok:true,transactionId:s.id,state:s.state});
    }catch(error){return api.fail("invalid_model_transaction",error.toString());}};
    host.SFLD_writeModelGraphAssetPage=function(text){try{
        var pair=matching(text,"writeModelGraphAssetPage"),s=pair.s,r=pair.request;
        if(s.state!=="uploading")throw new Error("Model upload is not active.");
        target(s,0,true);
        while(s.uploadAsset<s.assets.length && s.assets[s.uploadAsset].bytes===0)++s.uploadAsset;
        var a=s.assets[s.uploadAsset],expected=a?Math.min(PAGE,a.bytes*2-s.uploadPage*PAGE):0;
        if(!a || r.asset!==s.uploadAsset || r.page!==s.uploadPage || typeof r.hex!=="string" || r.hex.length!==expected || expected<=0 || !/^[0-9a-f]+$/.test(r.hex))
            throw new Error("Invalid or out of order Model upload page.");
        a.pages.push(r.hex);++s.uploadPage;
        if(s.uploadPage===Math.ceil(a.bytes*2/PAGE)){++s.uploadAsset;s.uploadPage=0;}
        return api.reply({ok:true,transactionId:s.id,asset:r.asset,page:r.page});
    }catch(error){if(typeof s!=="undefined" && s && s.state==="uploading")failure(s,error.toString());return api.fail("model_transaction_upload_failed",error.toString());}};
    host.SFLD_queueModelGraphTransaction=function(text){try{
        var pair=matching(text,"queueModelGraphTransaction"),s=pair.s;
        if(s.state!=="uploading")throw new Error("Model graph request is already queued or finished.");target(s,0,true);
        for(var i=0;i<s.assets.length;i++)if(s.assets[i].pages.length!==Math.ceil(s.assets[i].bytes*2/PAGE))throw new Error("Incomplete Model upload.");
        var command=app.findMenuCommandId("Starfield Apply Model Graph Transaction");if(!command)throw new Error("Paired Model Host command is unavailable.");
        s.state="queued";app.executeCommand(command);return api.reply({ok:true,transactionId:s.id,state:s.state});
    }catch(error){if(typeof s!=="undefined" && s && s.state==="uploading")failure(s,error.toString());return api.fail("model_transaction_queue_failed",error.toString());}};
    host.SFLD_modelTransactionHostRequest=function(){var s=host[KEY];if(!s || !session(s.id) || s.state!=="queued")return "0";
        try{target(s,0,true);var parts=/^p([0-9]+)-c([0-9]+)-l([0-9]+)$/.exec(s.request.target.token);
            if(!parts)throw new Error("Invalid Model target identity.");var ids=[s.request.rendererManifest.id];for(var n=0;n<s.request.nodeManifest.length;n++)ids.push(s.request.nodeManifest[n].id);
            s.state="receiving";return [s.id,parts[1],parts[2],parts[3],s.assets.length,ids.join(",")].join("|");
        }catch(error){failure(s,error.toString());return "0";}};
    host.SFLD_modelTransactionHostContinue=function(id){var s=session(id);if(!s || s.state!=="receiving")return "0";
        try{target(s,0,true);return "1";}catch(error){failure(s,error.toString());return "0";}};
    host.SFLD_modelTransactionHostAsset=function(id,index){var s=session(id);if(!s || s.state!=="receiving" || !integer(index,0,s.assets.length-1))return "0";
        var a=s.assets[index];return [a.nodeId,a.source,a.revision,a.bytes].concat(a.bounds).join("|");};
    host.SFLD_modelTransactionHostPage=function(id,index,page){var s=session(id);if(!s || s.state!=="receiving" || !integer(index,0,s.assets.length-1))return "0";
        var a=s.assets[index];return integer(page,0,a.pages.length-1)?a.pages[page]:"0";};
    host.SFLD_modelTransactionHostPrepare=function(id){var s=session(id);if(!s || s.state!=="receiving")return "0";
        try{var resolved=target(s,1,true);s.beforeRevision=Number(resolved.properties.revision.value);s.state="applying";
            var previous=api.snapshot(resolved).nativeNodes;
            api.ensure(resolved.target.layer,s.request.nodeManifest,true,previous,true);
            api.remove(resolved.target.layer,api.removed(previous,s.request.nodeManifest));
            var fresh=target(s,1,false),prepared=api.snapshot(fresh);
            s.preparedNodes=plain(prepared.nativeNodes);s.preparedRenderer=plain(prepared.renderer);return "1";
        }catch(error){s.message=error.toString();return "0";}};
    host.SFLD_modelTransactionHostCommit=function(id){var s=session(id);if(!s || s.state!=="applying")return "0";
        try{var resolved=target(s,1,false),before=api.snapshot(resolved);
            if(before.revision!==s.beforeRevision)throw new Error("Graph revision changed while applying Model assets.");
            records(s.preparedNodes,before.nativeNodes,true);renderer(s.preparedRenderer,before.renderer);
            api.ensure(resolved.target.layer,s.request.nodeManifest,true,before.nativeNodes,false);
            var fresh=target(s,1,false);api.writeRenderer(fresh,s.request.rendererManifest);
            var nonce=api.nonce(fresh.properties.commit,fresh.properties.receipt),receipt=api.trigger(fresh,"Starfield: apply Model graph",s.request.requestId,nonce,false);
            if(!receipt.ok)throw new Error(receipt.error && receipt.error.message || "Native graph commit failed.");
            var updated=api.snapshot(fresh);
            if(!updated.initialized || updated.revision<=s.beforeRevision)throw new Error("Native graph acknowledgement is not current.");
            records(s.request.nodeManifest,updated.nativeNodes,false);renderer(s.request.rendererManifest,updated.renderer);
            s.result={ok:true,committed:true,operation:"submitGraph",target:{token:fresh.token},snapshot:updated,nonce:receipt.nonce};
            s.state="published";s.assets=[];return "1";
        }catch(error){s.message=error.toString();return "0";}};
    host.SFLD_modelTransactionHostResult=function(id,numbers){var s=session(id);if(!s || !(numbers instanceof Array) || numbers.length!==9)return "0";
        for(var i=0;i<numbers.length;i++)if(!integer(numbers[i],-2147483648,2147483647))return "0";
        var committed=numbers[0]===1;
        if(committed && (!s.result || !s.result.committed))return "0";
        s.diagnostics={stage:numbers[1],error:numbers[2],rollbackError:numbers[3],assetRollbackError:numbers[4],cleanupError:numbers[5],undoError:numbers[6],assetError:numbers[7],assetIndex:numbers[8]};
        if(!committed)s.result={ok:false,committed:false,error:{code:"graph_commit_failed",message:(s.message || "Native Model graph transaction failed.")+" (stage "+numbers[1]+", error "+numbers[2]+", rollback "+numbers[3]+", asset rollback "+numbers[4]+")"}};
        s.result.diagnostics=s.diagnostics;s.state="complete";s.assets=[];return "1";};
    host.SFLD_readModelGraphTransaction=function(text){try{var s=matching(text,"readModelGraphTransaction").s;
        if(s.state==="complete" || s.state==="published")return api.reply({ok:true,transactionId:s.id,state:s.state,result:s.result});
        if(s.state==="failed")return api.fail("model_transaction_failed",s.message);
        return api.reply({ok:true,transactionId:s.id,state:s.state});
    }catch(error){return api.fail("model_transaction_missing",error.toString());}};
    host.SFLD_releaseModelGraphTransaction=function(text){try{var s=matching(text,"releaseModelGraphTransaction").s;
        if(s.state==="applying")s.cancelled=true;else host[KEY]=null;return api.reply({ok:true});
    }catch(error){return api.fail("model_transaction_missing",error.toString());}};
}());
