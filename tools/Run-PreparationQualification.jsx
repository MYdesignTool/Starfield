// Invoke only through an explicitly authorized prepared wrapper. No existing
// project is opened, closed or saved. Results/test project stay in artifacts.
(function(){
    var host=$.global,c=host.__SFLD_preparationQualification;host.__SFLD_preparationQualification=null;
    if(!c || c.version!==1){$.writeln("Report only: prepare and authorize the preparation qualification wrapper.");return;}
    if(typeof JSON==="undefined" || !JSON.parse || !JSON.stringify)throw new Error("AE JSON runtime unavailable.");
    if(new File(c.result).exists || new File(c.project).exists){$.writeln("Prior qualification evidence exists; prepare a fresh directory.");return;}
    if(typeof host.__SFLD_preparationQualificationTick==="function")throw new Error("A preparation qualification is already active.");
    var run={version:1,status:"starting",hostVersion:String(app.version),started:(new Date()).getTime(),cases:[],pendingManualGates:["original project repair/Undo/save-reopen","continuous interactive editing","native preparation-only timing","Stardust parity"]};
    var token="",comp=null,layer=null,index=0,task=0,polls=0,ownedProject=null;
    function write(){var f=new File(c.result);f.encoding="UTF-8";if(!f.open("w"))throw new Error("Cannot write qualification result.");try{f.write(JSON.stringify(run));}finally{f.close();}}
    function reply(operation,fields){var q={protocol:"org.starfieldfx.panel",version:1,gatewayBuild:c.gatewayBuild,operation:operation,requestId:"prep-"+index,pinTarget:!!token};
        if(token)q.target={token:token};for(var k in fields)if(Object.prototype.hasOwnProperty.call(fields,k))q[k]=fields[k];
        var fn=host["SFLD_"+operation];if(typeof fn!=="function")throw new Error("Missing gateway operation "+operation);
        var r=JSON.parse(fn(JSON.stringify(q)));if(!r || !r.ok)throw new Error(JSON.stringify(r));return r;}
    function finish(error){if(task)app.cancelTask(task);run.status=error?"failed":"completed-measurements";run.error=error?String(error):"";run.finished=(new Date()).getTime();
        // Leave the test project and AE open for inspection. Never quit AE.
        try{if(ownedProject && app.project===ownedProject && !app.project.file)app.project.save(new File(c.project));}catch(saveError){run.saveError=String(saveError);}
        write();delete host.__SFLD_preparationQualificationTick;delete host.__SFLD_preparationQualificationFrames;}
    function crcHex(hex){var crc=0xffffffff;for(var i=64;i<hex.length;i+=2){crc^=parseInt(hex.substr(i,2),16);for(var b=0;b<8;b++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);}
        crc=(crc^0xffffffff)>>>0;var text="";for(var n=0;n<4;n++){var part=((crc>>>(n*8))&255).toString(16);text+=(part.length<2?"0":"")+part;}return hex.substr(0,48)+text+hex.substr(56);}
    function adapt(value,map){if(typeof value==="string")return map[value]||value;
        if(value===null || typeof value!=="object")return value;var out=Object.prototype.toString.call(value)==="[object Array]"?[]:{};
        for(var k in value)if(Object.prototype.hasOwnProperty.call(value,k))out[k]=adapt(value[k],map);return out;}
    function renderFrame(row,frame){if(app.project.renderQueue.numItems!==0)throw new Error("Unexpected existing render queue item.");
        var item=app.project.renderQueue.items.add(comp),output=new File(new File(c.result).parent.fsName+"/"+row.name+"-"+frame+".avi");
        try{item.timeSpanStart=frame/24;item.timeSpanDuration=1/24;item.render=true;item.outputModule(1).file=output;
            var start=(new Date()).getTime();app.project.renderQueue.render();var elapsed=(new Date()).getTime()-start;
            if(item.status!==RQItemStatus.DONE || !output.exists || output.length===0)throw new Error("Single-frame render failed: "+row.name+"/"+frame);
            row.frames.push({frame:frame,wallMilliseconds:elapsed,output:output.fsName,bytes:output.length});write();
        }finally{item.remove();}}
    function next(){try{
        if((new Date()).getTime()-run.started>600000)throw new Error("Qualification deadline reached; no automatic retry.");
        if(!host.__SFLD_nativeCommandsV1 || typeof host.SFLD_getState!=="function"){
            if(++polls>60)throw new Error("Native panel did not publish session bindings; no command IDs were guessed.");
            task=app.scheduleTask("$.global.__SFLD_preparationQualificationTick()",500,false);return;}
        if(index>=c.cases.length){finish(null);return;}
        var fixture=c.cases[index],row={name:fixture.name,nodeCount:fixture.nodeCount,targetParticles:fixture.population,frames:[]};run.cases.push(row);
        comp=app.project.items.addComp("Preparation "+fixture.name,320,180,1,4,24);
        layer=comp.layers.addSolid([0,0,0],"Starfield preparation fixture",320,180,1,4);
        layer.property("ADBE Effect Parade").addProperty("org.starfieldfx.particle");comp.time=2;comp.openInViewer();layer.selected=true;token="";
        token=reply("getState",{}).target.token;var snapshot=reply("syncGraphSnapshot",{}).snapshot;
        if(!snapshot || !snapshot.initialized)throw new Error("Cannot initialize fresh qualification graph.");
        var map={};for(var n=0;n<snapshot.nativeNodes.length;n++){var nativeNode=snapshot.nativeNodes[n],source=fixture.initial[nativeNode.type];
            if(!source || map[source])throw new Error("Unexpected bootstrap source inventory.");map[source]=nativeNode.id;}
        if(snapshot.renderer.id!==fixture.rendererManifest.id)throw new Error("Unexpected Output identity.");
        var hex=fixture.graphHex;for(var old in map)if(Object.prototype.hasOwnProperty.call(map,old))hex=hex.split(old).join(map[old]);hex=crcHex(hex);
        var start=(new Date()).getTime();
        reply("submitGraph",{baseGraphRevision:snapshot.revision,baseRecordStamp:snapshot.recordStamp,baseNodeManifest:snapshot.nativeNodes,baseRendererManifest:snapshot.renderer,
            graphHex:hex,nodeManifest:adapt(fixture.nodeManifest,map),rendererManifest:fixture.rendererManifest});
        row.submitWallMilliseconds=(new Date()).getTime()-start;write();
        // Let UI metadata capture finish between explicit submit and frame work.
        task=app.scheduleTask("$.global.__SFLD_preparationQualificationFrames()",1000,false);
    }catch(error){finish(error);}}
    host.__SFLD_preparationQualificationTick=next;
    host.__SFLD_preparationQualificationFrames=function(){try{var row=run.cases[run.cases.length-1];
        for(var f=48;f<=50;f++)renderFrame(row,f);index++;task=app.scheduleTask("$.global.__SFLD_preparationQualificationTick()",250,false);
    }catch(error){finish(error);}};
    try{
        if(String(app.version).split(".")[0]!=="23")throw new Error("Only AE2023 is in scope.");
        if(!app.project || app.project.file || app.project.numItems!==0 || app.project.renderQueue.numItems!==0)throw new Error("Refusing to touch an existing or recovered project.");
        ownedProject=app.project;run.bitsPerChannel=app.project.bitsPerChannel;
        var command=app.findMenuCommandId("Starfield Particle Controls (WebView2)");if(!command)throw new Error("Native graph panel menu unavailable.");
        app.executeCommand(command);run.status="waiting-for-native-panel";write();task=app.scheduleTask("$.global.__SFLD_preparationQualificationTick()",500,false);
    }catch(error){finish(error);}
}());
