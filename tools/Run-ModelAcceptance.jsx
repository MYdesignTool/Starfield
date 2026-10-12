// Explicitly authorized host run only. The generated artifacts/.../Run.jsx
// supplies this configuration; invoking this file alone is report-only.
(function(){
    var host=$.global,config=host.__SFLD_modelAcceptanceConfig;
    if(!config || config.version!==1){$.writeln("Report only: prepare and authorize the Model acceptance wrapper before running.");return;}
    host.__SFLD_modelAcceptanceConfig=null;
    var key="__SFLD_modelAcceptanceRun",modelKey="__SFLD_modelGraphTransactionV1",exportKey="__SFLD_modelAssetBridgeV1";
    if(host[key] && host[key].running)throw new Error("Model acceptance is already running.");
    var pending=host[modelKey];if(pending && pending.state!=="complete" && pending.state!=="failed")throw new Error("An existing Model transaction must finish before acceptance.");
    if(host[exportKey])throw new Error("An existing Model asset export must be released before acceptance.");
    var run={running:true,status:"starting",hostVersion:String(app.version),gatewayBuild:config.gatewayBuild,cases:[],created:[],started:(new Date()).getTime(),index:0,phase:"start",token:"",id:"",task:0},
        comp=null,layer=null,front=null,back=null,previous=app.project.activeItem,selection=[],originalItems={},fixture=null,suppressed=false,assetIndex=0,activeAsset=null;
    host[key]=run;
    function write(){var f=new File(config.result);f.encoding="UTF-8";if(!f.open("w"))throw new Error("Cannot write acceptance result.");try{f.write(JSON.stringify(run));}finally{f.close();}}
    function request(operation,fields){var value={protocol:"org.starfieldfx.panel",version:1,gatewayBuild:config.gatewayBuild,operation:operation,requestId:"acceptance",pinTarget:true,target:{token:run.token},changes:[],transactionId:run.id};
        for(var k in fields)if(Object.prototype.hasOwnProperty.call(fields,k))value[k]=fields[k];
        var entry=host["SFLD_"+operation];if(typeof entry!=="function")throw new Error("Missing host entry: "+operation);
        return JSON.parse(entry(JSON.stringify(value)));}
    function checkReply(response){if(!response || !response.ok)throw new Error(JSON.stringify(response && response.error || response));return response;}
    function own(item){if(originalItems["$"+item.id])throw new Error("Acceptance cannot own an existing project item.");
        for(var i=0;i<run.created.length;i++)if(run.created[i]===item.id)return item;run.created.push(item.id);return item;}
    function ownedItem(id){for(var i=1;i<=app.project.numItems;i++)if(app.project.item(i).id===id)return app.project.item(i);return null;}
    function nativeActive(){var s=host[modelKey];return s && s.id===run.id && (s.state==="executing" || s.state==="applying" || s.state==="receiving");}
    function finish(error){
        if(fixture && fixture.nativeJournal){try{readNativeJournal();if(!error && (!run.nativeJournal || !run.nativeJournal.frames))error="No native render journal activity observed.";
            if(!error && run.nativeJournal.errors.length)error="Native render failures: "+JSON.stringify(run.nativeJournal.errors);
        }catch(journalError){if(!error)error=String(journalError);}}
        run.running=false;run.status=error?"failed":"passed";run.error=error?String(error):"";run.finished=(new Date()).getTime();
        if(run.task)app.cancelTask(run.task);
        if(nativeActive()){run.cleanup="Deferred: native transaction still owns the temporary test composition.";}
        else{try{if(run.id && host[modelKey] && host[modelKey].id===run.id)request("releaseModelGraphTransaction",{});
            if(activeAsset)request("releaseModelAsset",{assetId:activeAsset.id});
            if(run.renderItem){run.renderItem.remove();run.renderItem=null;}
            for(var i=run.created.length-1;i>=0;i--){var item=ownedItem(run.created[i]);if(item)item.remove();}
            if(previous && previous instanceof CompItem){previous.openInViewer();for(var s=0;s<selection.length;s++){try{selection[s].layer.selected=selection[s].selected;}catch(ignored){}}}
            run.cleanup="Temporary test items removed; previous composition and layer selection restored.";
        }catch(cleanupError){run.cleanup=String(cleanupError);run.status="failed";}}
        if(suppressed){app.endSuppressDialogs(false);suppressed=false;}write();}
    function later(){run.task=app.scheduleTask("$.global.__SFLD_modelAcceptanceTick()",250,false);}
    function snapshot(){return checkReply(request("getGraphSnapshot",{})).snapshot;}
    function readNativeJournal(){
        if(!fixture.nativeJournal)return;
        var f=new File(fixture.nativeJournal);if(!f.exists)return;
        f.encoding="UTF-8";if(!f.open("r"))throw new Error("Cannot read native render journal.");
        var text,end;try{if(!f.seek(run.nativeJournalOffset,0))throw new Error("Cannot seek native journal.");text=f.read();end=f.tell();}finally{f.close();}
        // A writer can still be closing its final line. Re-read a partial tail.
        if(text && text.charAt(text.length-1)!=="\n")return;
        run.nativeJournalOffset=end;
        var lines=text.split("\n");for(var n=0;n<lines.length;n++)if(lines[n]) {
            var row=JSON.parse(lines[n]);if(row.nativeBuild!==fixture.nativeBuild)throw new Error("Native journal build differs.");
            run.nativeJournal.frames++;if(row.canceled)run.nativeJournal.canceled++;
            else if(row.result!==0 && run.nativeJournal.errors.length<50)run.nativeJournal.errors.push(row);
        }
    }
    function renderAcceptanceFrame(){
        if(!fixture.renderFrames)return;
        if(app.project.renderQueue.numItems!==0)throw new Error("An existing render queue prevents isolated acceptance.");
        var rq=app.project.renderQueue.items.add(comp);run.renderItem=rq;
        try{rq.timeSpanStart=1;rq.timeSpanDuration=1/comp.frameRate;rq.render=true;
            var output=new File(new File(config.result).parent.fsName+"/case-"+run.index+".avi");
            rq.outputModule(1).file=output;app.project.renderQueue.render();
            if(rq.status!==RQItemStatus.DONE || !output.exists || output.length===0)throw new Error("Acceptance frame did not render completely.");
            readNativeJournal();if(run.nativeJournal.errors.length)throw new Error("Native render failed during deletion lifecycle.");
        }finally{rq.remove();run.renderItem=null;}
    }
    function u32(bytes,offset,value){for(var b=0;b<4;b++)bytes[offset+b]=(value>>>(8*b))&255;}
    function materialize(step){var result=JSON.parse(JSON.stringify(step)),bytes=[];
        for(var i=0;i<result.graphHex.length;i+=2)bytes.push(parseInt(result.graphHex.substr(i,2),16));
        for(var p=0;p<result.texturePatches.length;p++){var patch=result.texturePatches[p];u32(bytes,patch.offset,patch.source==="front"?front.id:back.id);}
        var crc=0xffffffff;for(var offset=32;offset<bytes.length;offset++){crc^=bytes[offset];for(var bit=0;bit<8;bit++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);}u32(bytes,24,(crc^0xffffffff)>>>0);
        result.graphHex="";for(var h=0;h<bytes.length;h++)result.graphHex+=(bytes[h]<16?"0":"")+bytes[h].toString(16);
        for(var n=0;n<result.nodeManifest.length;n++)for(var k=0;k<result.nodeManifest[n].parameters.length;k++){var field=result.nodeManifest[n].parameters[k];
            if(result.nodeManifest[n].type==="org.starfieldfx.nodes.particle" && field.type===3){if(field.key==="31" && field.value===fixture.markerFront)field.value=front.id;if(field.key==="32" && field.value===fixture.markerBack)field.value=back.id;}}
        return result;}
    function verify(step,saved){if(!saved.initialized || saved.nativeNodes.length!==step.nodeManifest.length)throw new Error("Acceptance saved count/receipt differs.");
        var api=host.__SFLD_modelTransactionAPI,observed=[];
        for(var n=0;n<step.nodeManifest.length;n++){var expected=step.nodeManifest[n],actual=null;
            for(var a=0;a<saved.nativeNodes.length;a++)if(saved.nativeNodes[a].id===expected.id)actual=saved.nativeNodes[a];
            if(!actual || actual.type!==expected.type || api.recordSchema(actual)!==api.recordSchema(expected))throw new Error("Acceptance node identity/type/control layout differs: "+expected.id+" / "+expected.type);
            observed.push({id:actual.id,type:actual.type,schemaVersion:actual.schemaVersion});}
        return observed;}
    function exportNext(){var step=run.step;
        if(assetIndex>=step.assets.length){renderAcceptanceFrame();run.cases.push({name:step.name,ok:true,revision:run.saved.revision,nodes:verify(step,run.saved),assetReadbacks:assetIndex,
            elapsedMilliseconds:(new Date()).getTime()-run.stepStarted,prepareMilliseconds:run.queuedAt-run.stepStarted,
            queueAndCommitMilliseconds:run.committedAt-run.queuedAt,readbackMilliseconds:(new Date()).getTime()-run.committedAt});run.index++;run.phase="begin";write();later();return;}
        var asset=step.assets[assetIndex];activeAsset={id:run.id.substr(0,28)+("000"+assetIndex).slice(-4),expected:asset,page:0};
        checkReply(request("beginModelAssetExport",{assetId:activeAsset.id,nodeId:asset.nodeId,baseGraphRevision:run.saved.revision}));run.phase="export";later();}
    host.__SFLD_modelAcceptanceTick=function(){try{
        if(!run.running)return;
        if((new Date()).getTime()-run.started>(fixture.deadlineMilliseconds || 120000)){finish("Acceptance deadline exceeded at case "+run.index+" / "+run.phase);return;}
        if(run.phase==="begin"){
            if(run.index===fixture.steps.length){finish(null);return;}
            run.stepStarted=(new Date()).getTime();run.step=materialize(fixture.steps[run.index]);var before=snapshot();
            run.id=("00000000000000000000000000000000"+(new Date()).getTime().toString(16)+run.index.toString(16)).slice(-32);
            var fields={baseGraphRevision:before.revision,baseRecordStamp:before.recordStamp,graphHex:run.step.graphHex,nodeManifest:run.step.nodeManifest,rendererManifest:run.step.rendererManifest,assets:[]};
            for(var d=0;d<run.step.assets.length;d++){var descriptor=run.step.assets[d];fields.assets.push({nodeId:descriptor.nodeId,source:2,revision:descriptor.revision,bounds:descriptor.bounds,bytes:descriptor.meshHex.length/2});}
            checkReply(request("beginModelGraphTransaction",fields));
            for(var mesh=0;mesh<run.step.assets.length;mesh++){var hex=run.step.assets[mesh].meshHex;for(var page=0;page*65536<hex.length;page++)checkReply(request("writeModelGraphAssetPage",{asset:mesh,page:page,hex:hex.substr(page*65536,65536)}));}
            checkReply(request("queueModelGraphTransaction",{}));run.queuedAt=(new Date()).getTime();run.phase="poll";run.lastState="";write();later();return;
        }
        if(run.phase==="poll"){var reply=checkReply(request("readModelGraphTransaction",{}));
            if(reply.state!==run.lastState){run.lastState=reply.state;write();}
            if(reply.state!=="complete"){later();return;}
            if(!reply.result || !reply.result.ok || !reply.result.committed)throw new Error(JSON.stringify(reply.result));
            if(reply.result.diagnostics && (reply.result.diagnostics.cleanupError || reply.result.diagnostics.undoError))throw new Error("Native cleanup/undo failed: "+JSON.stringify(reply.result.diagnostics));
            run.committedAt=(new Date()).getTime();run.saved=snapshot();verify(run.step,run.saved);checkReply(request("releaseModelGraphTransaction",{}));assetIndex=0;exportNext();return;
        }
        if(run.phase==="export"){var exported=request("readModelAssetPage",{assetId:activeAsset.id,nodeId:activeAsset.expected.nodeId,page:activeAsset.page});
            checkReply(exported);if(exported.state==="queued" || exported.state==="receiving"){later();return;}
            var expectedPages=Math.ceil(activeAsset.expected.meshHex.length/65536);
            if(exported.state!=="ready" || exported.pageCount!==expectedPages || exported.hex!==activeAsset.expected.meshHex.substr(activeAsset.page*65536,65536))throw new Error("Native mesh readback differs for "+activeAsset.expected.nodeId+" / page "+activeAsset.page);
            activeAsset.page++;if(activeAsset.page<expectedPages){later();return;}
            checkReply(request("releaseModelAsset",{assetId:activeAsset.id}));activeAsset=null;assetIndex++;exportNext();return;
        }
        throw new Error("Unknown acceptance phase.");
    }catch(error){finish(error.toString());}};
    try{
        var file=new File(config.fixture);file.encoding="UTF-8";if(!file.open("r"))throw new Error("Missing acceptance fixtures.");try{fixture=JSON.parse(file.read());}finally{file.close();}
        if(fixture.gatewayBuild!==config.gatewayBuild || fixture.version!==1)throw new Error("Acceptance fixture generation differs.");
        if(fixture.renderFrames){if(!fixture.nativeJournal || fixture.nativeBuild!==76)throw new Error("Missing paired native journal contract.");
            if(app.project.renderQueue.numItems!==0)throw new Error("An existing render queue prevents isolated acceptance.");
            var journal=new File(fixture.nativeJournal);run.nativeJournalOffset=journal.exists?journal.length:0;
            run.nativeJournal={frames:0,canceled:0,errors:[]};}
        if(fixture.deadlineMilliseconds!==undefined && (typeof fixture.deadlineMilliseconds!=="number" || !isFinite(fixture.deadlineMilliseconds) || fixture.deadlineMilliseconds<120000 || fixture.deadlineMilliseconds>600000 || Math.floor(fixture.deadlineMilliseconds)!==fixture.deadlineMilliseconds))throw new Error("Invalid bounded acceptance deadline.");
        $.evalFile(new File(config.repo+"/cep_panel/jsx/starfield_gateway.jsx"));$.evalFile(new File(config.repo+"/cep_panel/jsx/model_transaction_transport.jsx"));
        if(host.SFLD_modelTransactionReady()!==config.gatewayBuild)throw new Error("Installed gateway/transport generation differs.");
        if(previous && previous instanceof CompItem)for(var s=1;s<=previous.numLayers;s++)selection.push({layer:previous.layer(s),selected:previous.layer(s).selected});
        for(var oldItem=1;oldItem<=app.project.numItems;oldItem++)originalItems["$"+app.project.item(oldItem).id]=true;
        app.beginSuppressDialogs();suppressed=true;
        var folder=own(app.project.items.addFolder("Starfield Model acceptance "+run.started));
        comp=own(app.project.items.addComp("Starfield Model acceptance",640,360,1,2,24));comp.parentFolder=folder;
        layer=comp.layers.addSolid([0,0,0],"Acceptance particles",640,360,1);own(layer.source);
        if(layer.source.parentFolder && !originalItems["$"+layer.source.parentFolder.id])own(layer.source.parentFolder);
        layer.source.parentFolder=folder;
        front=comp.layers.addSolid([1,0,0],"Front",160,90,1);own(front.source);front.source.parentFolder=folder;front.enabled=false;
        back=comp.layers.addSolid([0,1,0],"Back",160,90,1);own(back.source);back.source.parentFolder=folder;back.enabled=false;
        layer.property("ADBE Effect Parade").addProperty("org.starfieldfx.particle");comp.openInViewer();layer.selected=true;front.selected=false;back.selected=false;
        run.token="p"+app.project.rootFolder.id+"-c"+comp.id+"-l"+layer.id;
        checkReply(request("ensureNodeEffects",{}));run.status="running";run.phase="begin";write();later();
    }catch(error){finish(error.toString());}
}());
