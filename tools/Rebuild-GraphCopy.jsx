// Explicit owner-run M3-20 recovery, never called by the panel.
// Creates a DISABLED copy; original and prior backup are never written.
// Replaces only the copy's renderer, preserving the duplicated native effects.
// Requires static Output controls. Fresh renderer resets its derived/private data
// and effect-level compositing options; original remains available for comparison.
// One undo group; no save, process, preference, registry, or cache operations.
(function () {
    var folder=(new File($.fileName)).parent.parent.fsName+"/artifacts/m3-20-guard-recovery-20261017/",
        host=$.global,baseline=null,source=null,copyId=null,groupOpen=false,
        result={version:1,ok:false,phase:"preflight",host:String(app.version),at:(new Date()).getTime()},
        copyName="Starfield rebuilt - Medium Blue Solid 4",mainMatch="org.starfieldfx.particle",
        controls=["Max Particles","Layout Output X","Layout Output Y",mainMatch+"-0921","Time (Seconds)",
            "Preview","Particle chance","Time Sampling","Acceleration",
            mainMatch+"-1641",mainMatch+"-1642",mainMatch+"-1643",mainMatch+"-1644",
            mainMatch+"-1645",mainMatch+"-1646",mainMatch+"-1647",mainMatch+"-1648"],
        values=[],sourceState=null,sourceEnabled=null,rendererName=null,rendererEnabled=null,rendererIndex=0;
    function need(condition,message){if(!condition)throw new Error(message);}
    function equal(a,b){return JSON.stringify(a)===JSON.stringify(b);}
    function load(name){var f=new File(folder+name);need(f.open("r"),"Missing "+name);try{return JSON.parse(f.read());}finally{f.close();}}
    function ownerComp(){for(var c=1;c<=app.project.numItems;c++){var comp=app.project.item(c);
        if(comp instanceof CompItem && comp.id===1 && comp.name==="Comp 1")return comp;}
        throw new Error("Recovery composition disappeared.");}
    function layer(id){var comp=ownerComp();for(var i=1;i<=comp.numLayers;i++)if(comp.layer(i).id===id)return comp.layer(i);
        throw new Error("Recovery layer disappeared.");}
    function renderer(target){var parade=target.property("ADBE Effect Parade"),found=0,count=0;
        need(parade && parade.numProperties<=4224,"Invalid effect inventory.");
        for(var i=1;i<=parade.numProperties;i++)if(parade.property(i).matchName===mainMatch){found=i;count++;}
        need(count===1,"Expected exactly one renderer.");return target.property("ADBE Effect Parade").property(found);}
    function read(target,name){var p=renderer(target).property(name);need(p,"Missing "+name);
        var value=Number(p.valueAtTime(0,true));need(isFinite(value),"Nonfinite "+name);return value;}
    function write(name,value){var target=layer(copyId);renderer(target).property(name).setValue(value);
        need(read(layer(copyId),name)===value,"Value did not persist: "+name);}
    function token(target){return "p"+app.project.rootFolder.id+"-c"+target.containingComp.id+"-l"+target.id;}
    function snapshot(target){var response=JSON.parse(host.SFLD_getGraphSnapshot(JSON.stringify({protocol:"org.starfieldfx.panel",version:1,
        operation:"getGraphSnapshot",requestId:"manual-rebuild-copy",pinTarget:true,target:{token:token(target)}})));
        need(response.ok && response.snapshot && response.snapshot.initialized===true,"Canonical snapshot unavailable.");
        need(equal(response.snapshot.nativeNodes,baseline.snapshot.nativeNodes) && equal(response.snapshot.renderer,baseline.snapshot.renderer),
            "Authored nodes or Output controls differ from the frozen report.");return response.snapshot;}
    function state(target){return [read(target,"Panel Graph Sync Guard"),read(target,"Commit Graph Edit"),read(target,"Graph Edit Receipt"),
        read(target,"Graph Revision"),read(target,"Graph Checksum High"),read(target,"Graph Checksum Low")];}
    function staticValue(target,name){var p=renderer(target).property(name);need(p,"Missing Output control "+name);
        need(p.numKeys===0 && (!p.canSetExpression || (!p.expressionEnabled && !p.expression)),
            "Animated/expression Output control requires a separate recovery: "+name);
        return read(target,name);}
    function checkSource(){source=layer(70);need(source.name===baseline.selectedNames.layer && source.enabled===sourceEnabled && equal(state(source),sourceState),
        "Original changed during recovery.");snapshot(source);
        need(renderer(source).name===rendererName && renderer(source).enabled===rendererEnabled && renderer(source).propertyIndex===rendererIndex,
            "Original renderer identity changed.");
        for(var i=0;i<controls.length;i++)need(staticValue(source,controls[i])===values[i],"Original Output control changed.");}
    try {
        baseline=load("baseline.json");
        need(baseline.version===2 && baseline.selectedTarget==="p0-c1-l70" && baseline.selectedNames.comp==="Comp 1" &&
            baseline.selectedNames.layer==="Medium Blue Solid 4" && baseline.snapshot && baseline.snapshot.checksum==="29ece35f",
            "Unexpected frozen baseline.");
        need(app.project && app.project.rootFolder.id===0,"Unexpected project.");
        for(var c=1;c<=app.project.numItems;c++){var comp=app.project.item(c);if(!(comp instanceof CompItem) || comp.id!==1 || comp.name!=="Comp 1")continue;
            for(var l=1;l<=comp.numLayers;l++)if(comp.layer(l).id===70 && comp.layer(l).name==="Medium Blue Solid 4")source=comp.layer(l);}
        need(source,"Frozen source layer unavailable.");
        need(typeof host.SFLD_ready==="function" && host.SFLD_ready()==="org.starfieldfx.panel/1/native-presets-99" &&
            typeof host.SFLD_getGraphSnapshot==="function","Open the installed CEP99 panel first.");
        var session=host.__SFLD_modelGraphTransactionV1;
        need(!session || session.state==="complete" || session.state==="failed" || session.state==="cancelled","A graph/Model session may be active.");
        sourceState=state(source);sourceEnabled=source.enabled;
        need(sourceState[0]===1 && (sourceState[1]===790333 || sourceState[1]===790334) && sourceState[2]===-sourceState[1] &&
            sourceState[3]===44 && sourceState[4]===10732 && sourceState[5]===58207,"Source is not the diagnosed terminal failure.");
        snapshot(source);
        var parade=source.property("ADBE Effect Parade");
        for(var e=1;e<=parade.numProperties;e++){var fx=parade.property(e);
            if(String(fx.matchName).indexOf("org.starfieldfx.node.")===0)
                need(Number(fx.property("Panel Sync Guard").valueAtTime(0,true))===0,"A source node is guarded.");}
        rendererName=String(renderer(source).name);rendererEnabled=renderer(source).enabled;rendererIndex=renderer(source).propertyIndex;
        for(var i=0;i<controls.length;i++)values.push(staticValue(source,controls[i]));
        var command=app.findMenuCommandId("Starfield Commit Node Graph");need(command>0,"Native graph command unavailable.");
        for(var n=1;n<=source.containingComp.numLayers;n++)need(source.containingComp.layer(n).name!==copyName,"Rebuilt copy already exists; do not repeat.");
        // Another unresolved pending renderer would make the native command ambiguous.
        for(c=1;c<=app.project.numItems;c++){comp=app.project.item(c);if(!(comp instanceof CompItem))continue;
            for(l=1;l<=comp.numLayers;l++){var candidate=comp.layer(l),effects=candidate.property("ADBE Effect Parade");
                for(e=1;effects && e<=effects.numProperties;e++)if(effects.property(e).matchName===mainMatch){
                    var other=effects.property(e),guard=Number(other.property("Panel Graph Sync Guard").valueAtTime(0,true));
                    need(guard===0 || guard===1 || guard===2,"Unknown renderer guard.");
                    if(guard===1){var nonce=Number(other.property("Commit Graph Edit").valueAtTime(0,true)),receipt=Number(other.property("Graph Edit Receipt").valueAtTime(0,true));
                        need(nonce>=1 && nonce<=1000000 && Math.floor(nonce)===nonce && (receipt===nonce || receipt===-nonce),"Another renderer has a pending command.");}
                }}}
        checkSource();app.beginUndoGroup("Starfield: rebuild disabled graph copy");groupOpen=true;result.phase="duplicate";
        var copy=source.duplicate();copyId=copy.id;result.copyTarget=token(copy);copy.name=copyName;copy.enabled=false;copy.selected=false;
        need(layer(copyId).enabled===false,"Copy could not be disabled.");
        write("Panel Graph Sync Guard",2);checkSource();
        result.phase="replace copy renderer";renderer(layer(copyId)).remove();
        layer(copyId).property("ADBE Effect Parade").addProperty(mainMatch);
        write("Panel Graph Sync Guard",1);
        renderer(layer(copyId)).moveTo(rendererIndex);
        renderer(layer(copyId)).name=rendererName;renderer(layer(copyId)).enabled=rendererEnabled;
        need(renderer(layer(copyId)).propertyIndex===rendererIndex,"Renderer order did not persist.");
        for(i=0;i<controls.length;i++)write(controls[i],values[i]);
        write("Node Effects Ready",1);
        need(read(layer(copyId),"Graph Revision")===0 && read(layer(copyId),"Graph Edit Receipt")===0,"Replacement renderer is not fresh.");
        result.phase="publish";write("Commit Graph Edit",790335);app.executeCommand(command);
        need(read(layer(copyId),"Graph Edit Receipt")===790335 && read(layer(copyId),"Graph Revision")>0,"New graph was not published.");
        snapshot(layer(copyId));checkSource();
        result.phase="release copy";write("Panel Graph Sync Guard",0);result.ok=true;
    } catch(error) {result.error=String(error);}
    if(!result.ok && copyId!==null){try{write("Panel Graph Sync Guard",2);}catch(guardError){result.guardError=String(guardError);}}
    if(groupOpen){try{app.endUndoGroup();}catch(undoError){result.ok=false;result.undoError=String(undoError);}}
    if(result.ok){try{checkSource();need(layer(copyId).enabled===false && read(layer(copyId),"Panel Graph Sync Guard")===0,
        "Copy state changed after undo closure.");snapshot(layer(copyId));result.phase="complete";}catch(finalError){result.ok=false;result.error=String(finalError);}}
    var text=JSON.stringify(result,null,2),saved=false;
    try{var output=new File(folder+"rebuild-copy-result.json");output.encoding="UTF-8";
        if(output.open("w")){try{saved=output.write(text);}finally{output.close();}}}catch(saveError){}
    var message=result.ok?"Disabled rebuilt copy is ready for inspection. Original unchanged.\nSelect '"+copyName+"' and refresh the panel.\nUndo once immediately to remove this recovery copy.":
        "Rebuild did not complete. Do not retry. Original was not written.\n"+(result.error || result.undoError || "See report.")+"\nUndo once immediately if a copy was created.";
    if(saved)alert(message+"\nReport: "+folder+"rebuild-copy-result.json");else prompt(message+"\nCopy this report:",text);
}());
