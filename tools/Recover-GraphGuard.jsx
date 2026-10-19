// Explicit, owner-run M3-20 recovery. Never called by the panel or at startup.
// Scoped to the frozen host report; never treats an arbitrary busy guard as idle.
// Creates a disabled full-layer backup and rebuilds from unchanged node controls.
// One AE undo group. No project save, process, registry, preference or cache action.
(function () {
    var root=(new File($.fileName)).parent.parent.fsName,
        folder=root+"/artifacts/m3-20-guard-recovery-20261017/",
        result={version:1,ok:false,phase:"preflight",host:String(app.version),at:(new Date()).getTime()},
        baseline=null,host=$.global,groupOpen=false,originalWritten=false,published=false;
    function requireValue(condition,message){if(!condition)throw new Error(message);}
    function load(file){requireValue(file.open("r"),"Cannot open frozen recovery baseline.");
        try{return JSON.parse(file.read());}finally{file.close();}}
    function target(){
        var project=app.project,token=baseline.selectedTarget,parts=/^p([0-9]+)-c([0-9]+)-l([0-9]+)$/.exec(token);
        requireValue(project && parts && String(project.rootFolder.id)===parts[1],"Project identity changed.");
        var comp=null,layer=null,i;
        for(i=1;i<=project.numItems;i++){var item=project.item(i);if(item instanceof CompItem && String(item.id)===parts[2]){comp=item;break;}}
        requireValue(comp && comp.name===baseline.selectedNames.comp,"Composition identity or name changed.");
        for(i=1;i<=comp.numLayers;i++){var candidate=comp.layer(i);if(String(candidate.id)===parts[3]){layer=candidate;break;}}
        requireValue(layer && layer.name===baseline.selectedNames.layer,"Layer identity or name changed.");
        return layer;
    }
    function renderer(layer){
        var parade=layer.property("ADBE Effect Parade"),found=0,count=0;
        requireValue(parade && parade.numProperties<=4224,"Invalid effect inventory.");
        for(var i=1;i<=parade.numProperties;i++){var effect=parade.property(i);
            if(effect.matchName==="org.starfieldfx.particle"){found=i;count++;}}
        requireValue(count===1,"Recovery requires exactly one renderer on this layer.");
        return layer.property("ADBE Effect Parade").property(found);
    }
    function read(name){var p=renderer(target()).property(name);requireValue(p,"Missing "+name);
        var value=Number(p.valueAtTime(0,true));requireValue(isFinite(value),"Invalid "+name);return value;}
    function write(name,value){var p=renderer(target()).property(name);requireValue(p,"Missing "+name);p.setValue(value);
        requireValue(read(name)===value,"Write did not persist: "+name);}
    function snapshot(){
        var response=JSON.parse(host.SFLD_getGraphSnapshot(JSON.stringify({protocol:"org.starfieldfx.panel",version:1,
            operation:"getGraphSnapshot",requestId:"manual-guard-recovery",pinTarget:true,target:{token:baseline.selectedTarget}})));
        requireValue(response.ok && response.snapshot && response.snapshot.initialized===true,"Canonical snapshot unavailable.");
        var current=response.snapshot;
        requireValue(JSON.stringify(current.nativeNodes)===JSON.stringify(baseline.snapshot.nativeNodes) &&
            JSON.stringify(current.renderer)===JSON.stringify(baseline.snapshot.renderer),"Node controls or topology changed since the frozen report.");
        var parade=target().property("ADBE Effect Parade");
        for(var i=1;i<=parade.numProperties;i++){var effect=parade.property(i);
            if(String(effect.matchName).indexOf("org.starfieldfx.node.")===0){var p=effect.property("Panel Sync Guard");
                requireValue(p && Number(p.valueAtTime(0,true))===0,"A node is still guarded; recovery refused.");}}
        return current;
    }
    try {
        baseline=load(new File(folder+"baseline.json"));
        requireValue(baseline.version===2 && baseline.selectedTarget==="p0-c1-l70" &&
            baseline.snapshot && baseline.snapshot.ok && baseline.snapshot.initialized &&
            baseline.beforeSnapshot && baseline.beforeSnapshot[0].guard===1 &&
            baseline.beforeSnapshot[0].commit===790333 && baseline.beforeSnapshot[0].receipt===-790333 &&
            baseline.beforeSnapshot[0].revision===44,"Unexpected recovery baseline.");
        requireValue(typeof host.SFLD_ready==="function" && host.SFLD_ready()==="org.starfieldfx.panel/1/native-presets-99" &&
            typeof host.SFLD_getGraphSnapshot==="function","Open the installed CEP99 panel before recovery.");
        var session=host.__SFLD_modelGraphTransactionV1;
        requireValue(!session || session.state==="complete" || session.state==="failed" || session.state==="cancelled",
            "A Model/graph session may still be active; recovery refused.");
        var expected=baseline.beforeSnapshot[0];result.target=baseline.selectedTarget;result.names=baseline.selectedNames;
        requireValue(read("Panel Graph Sync Guard")===1 && read("Commit Graph Edit")===expected.commit &&
            read("Graph Edit Receipt")===expected.receipt && read("Graph Revision")===expected.revision &&
            read("Graph Checksum High")===expected.checksumHigh && read("Graph Checksum Low")===expected.checksumLow,
            "Saved transaction state changed; no recovery writes are permitted.");
        snapshot();
        var command=app.findMenuCommandId("Starfield Commit Node Graph");requireValue(command>0,"Native graph command unavailable.");
        var backupName="Starfield recovery backup - "+baseline.selectedNames.layer,owner=target(),comp=owner.containingComp;
        for(var n=1;n<=comp.numLayers;n++)requireValue(comp.layer(n).name!==backupName,"Recovery backup already exists; do not repeat recovery.");
        app.beginUndoGroup("Starfield: recover verified failed graph");groupOpen=true;
        result.phase="backup";
        var backup=owner.duplicate();result.backupLayerId=backup.id;result.backupLayerName=backupName;
        backup.name=backupName;backup.enabled=false;backup.selected=false;
        // The copy is retained as a disabled backup, excluded from native requests.
        renderer(backup).property("Panel Graph Sync Guard").setValue(2);
        requireValue(Number(renderer(backup).property("Panel Graph Sync Guard").valueAtTime(0,true))===2,"Backup guard did not persist.");
        // Duplication may invalidate AE wrappers; reacquire the original by identity.
        snapshot();requireValue(read("Panel Graph Sync Guard")===1 && read("Commit Graph Edit")===expected.commit &&
            read("Graph Edit Receipt")===expected.receipt && read("Graph Revision")===expected.revision,"Original changed during backup.");
        result.phase="publish";var nonce=expected.commit+1;result.requestedNonce=nonce;
        originalWritten=true;write("Commit Graph Edit",nonce);app.executeCommand(command);
        result.savedReceipt=read("Graph Edit Receipt");result.savedRevision=read("Graph Revision");
        requireValue(result.savedReceipt===nonce && result.savedRevision>expected.revision,
            "Native recovery publication failed (requested "+nonce+", saved "+result.savedReceipt+"). Backup and original guard retained.");
        published=true;result.phase="verify controls";snapshot();
        requireValue(read("Panel Graph Sync Guard")===1,"Guard changed during recovery publication.");
        result.phase="release";write("Panel Graph Sync Guard",0);
        result.ok=true;
    } catch(error) {
        result.error=String(error);
        if(originalWritten){try{write("Panel Graph Sync Guard",1);}catch(guardError){result.guardError=String(guardError);}}
    } finally {
        if(groupOpen){try{app.endUndoGroup();}catch(undoError){result.ok=false;result.undoError=String(undoError);}}
    }
    result.published=published;
    if(baseline){try{result.finalGuard=read("Panel Graph Sync Guard");
        if(result.ok && result.finalGuard!==0){result.ok=false;result.error="Guard changed after undo closure.";}}
        catch(readError){result.ok=false;result.readError=String(readError);}}
    if(result.ok)result.phase="complete";
    var text=JSON.stringify(result,null,2),saved=false;
    try{var output=new File(folder+"recovery-result.json");output.encoding="UTF-8";
        if(output.open("w")){try{saved=output.write(text);}finally{output.close();}}}catch(saveError){}
    var message=result.ok?"Recovery published and verified. Disabled full-layer backup retained.\nRefresh the Starfield panel. Undo once to revert this recovery.":
        "Recovery did not complete. No automatic retry.\n"+(result.error || result.undoError || result.readError || "See report.");
    if(saved)alert(message+"\nReport saved: "+folder+"recovery-result.json");else prompt(message+"\nCopy this report:",text);
}());
