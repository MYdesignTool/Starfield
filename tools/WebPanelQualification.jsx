// Actual AE startup smoke only; this file never reports complete qualification.
// Run only after explicit ADR0011 authorization through the companion launcher.
(function () {
    var root=new File($.fileName).parent.parent;
    var directory=new Folder(root.fsName+"/artifacts/webview2/ae-qualification");
    if(!directory.exists && !directory.create())throw new Error("Cannot create checkout-local test report folder.");
    function encode(value) {
        if(typeof value==="string")return '"'+value.replace(/\\/g,"\\\\").replace(/"/g,'\\"').replace(/\r/g,"\\r").replace(/\n/g,"\\n").replace(/\t/g,"\\t")+'"';
        if(value===null)return "null";
        if(typeof value==="number" || typeof value==="boolean")return String(value);
        var fields=[],key;
        for(key in value)if(Object.prototype.hasOwnProperty.call(value,key))fields.push(encode(key)+":"+encode(value[key]));
        return "{"+fields.join(",")+"}";
    }
    var gatewayFile=new File(root.fsName+"/web_panel/jsx/starfield_gateway.jsx"),gatewayBuild="";
    if(gatewayFile.open("r")){try{var match=/var GATEWAY_BUILD = "([^"]+)"/.exec(gatewayFile.read());if(match)gatewayBuild=match[1];}finally{gatewayFile.close();}}
    var report={kind:"actual-ae-startup-smoke",version:String(app.version),gatewayBuild:gatewayBuild,completeQualification:false,phase:"starting",gatewayObserved:false};
    function save(){var file=new File(directory.fsName+"/startup-smoke.json");file.encoding="UTF-8";
        if(!file.open("w"))throw new Error("Cannot write startup smoke report.");try{file.write(encode(report));}finally{file.close();}}
    try {
        if(String(app.version).split(".")[0]!=="23")throw new Error("Only AE2023 is within the qualification scope.");
        var graph=app.findMenuCommandId("Starfield Particle Controls (WebView2)"),presets=app.findMenuCommandId("Starfield Presets (WebView2)");
        report.graphMenu=graph;report.presetsMenu=presets;
        if(!graph || !presets)throw new Error("The installed native WebView2 menus were not registered.");
        // Never replace an owner's open/recovered project. Use the startup empty
        // project only, and save it solely inside checkout artifacts.
        if(!app.project || app.project.file || app.project.numItems!==0)throw new Error("AE has an existing project; left untouched. Open native panels manually.");
        var comp=app.project.items.addComp("Starfield WebView2 Qualification",640,360,1,3,24);
        var layer=comp.layers.addSolid([0,0,0],"Starfield WebView2 Test",640,360,1);
        layer.property("ADBE Effect Parade").addProperty("org.starfieldfx.particle");
        layer.selected=true;comp.openInViewer();
        app.executeCommand(graph);app.executeCommand(presets);
        report.phase="native-menus-opened-awaiting-ui";save();
        var attempts=0;
        $.global.__SFLD_webviewQualificationPoll=function(){
            attempts++;
            report.gatewayObserved=typeof $.global.SFLD_ready==="function" && $.global.SFLD_ready()==="org.starfieldfx.panel/1/"+gatewayBuild;
            report.nativeEffectCount=layer.property("ADBE Effect Parade").numProperties;
            report.polls=attempts;
            if((report.gatewayObserved && report.nativeEffectCount>1) || attempts>=30){
                report.phase="startup-observation-complete-manual-gates-open";
                try{app.project.save(new File(directory.fsName+"/startup-smoke.aep"));}catch(error){report.saveError=String(error);}
                save();delete $.global.__SFLD_webviewQualificationPoll;
            }else{save();app.scheduleTask("$.global.__SFLD_webviewQualificationPoll()",800,false);}
        };
        app.scheduleTask("$.global.__SFLD_webviewQualificationPoll()",800,false);
    }catch(error){report.phase="aborted";report.error=String(error);save();}
}());
