// Manual diagnostic for the already-running AE host. Project access is read-only.
// Writes only artifacts/m3-20-guard-owner-20261017/host-report.json in this checkout.
// No setValue, executeCommand, undo, selection, preference, or process changes.
(function () {
    var report={version:2,host:String(app.version),engine:String($.engineName),at:(new Date()).getTime()},
        host=$.global,project=app.project,comp=project?project.activeItem:null;
    function attempt(read){try{return read();}catch(error){return {error:String(error)};}}
    function number(effect,name){return attempt(function(){var p=effect.property(name);
        if(!p)throw new Error("Missing "+name);return Number(p.valueAtTime(0,true));});}
    function inventory(layer){
        var result=[],parade=layer.property("ADBE Effect Parade");
        for(var i=1;parade && i<=parade.numProperties && i<=4224;i++) {
            var effect=parade.property(i),match=String(effect.matchName);
            if(match!=="org.starfieldfx.particle" && match.indexOf("org.starfieldfx.node.")!==0)continue;
            var renderer=match==="org.starfieldfx.particle",entry={index:i,matchName:match,
                guard:number(effect,renderer?"Panel Graph Sync Guard":"Panel Sync Guard")};
            if(renderer){
                entry.revision=number(effect,"Graph Revision");entry.commit=number(effect,"Commit Graph Edit");
                entry.receipt=number(effect,"Graph Edit Receipt");entry.ready=number(effect,"Node Effects Ready");
                entry.checksumHigh=number(effect,"Graph Checksum High");entry.checksumLow=number(effect,"Graph Checksum Low");
            } else {
                entry.uuid=[];for(var part=0;part<8;part++)entry.uuid.push(number(effect,"Node UUID "+part));
            }
            result.push(entry);
        }
        return result;
    }
    function sessionSummary(session){
        if(!session)return null;
        var out={},keys=["version","id","state","message","notificationError","nativeOutcome","cancelled","expires"];
        for(var i=0;i<keys.length;i++)if(typeof session[keys[i]]!=="undefined")out[keys[i]]=session[keys[i]];
        if(session.request)out.target=session.request.target;
        if(session.diagnostics)out.diagnostics=session.diagnostics;
        if(session.result)out.result={ok:session.result.ok,committed:session.result.committed,error:session.result.error,diagnostics:session.result.diagnostics};
        return out;
    }
    report.gateway=attempt(function(){return typeof host.SFLD_ready==="function"?String(host.SFLD_ready()):"unavailable";});
    report.modelSession=attempt(function(){return sessionSummary(host.__SFLD_modelGraphTransactionV1);});
    var layers=comp instanceof CompItem?comp.selectedLayers:[];
    if(!project || !project.rootFolder || layers.length!==1){
        alert("Select the composition and exactly one Starfield layer, then run Report-GraphGuard.jsx again.");return;
    }
    var layer=layers[0],token="p"+project.rootFolder.id+"-c"+comp.id+"-l"+layer.id;
    report.selectedTarget=token;
    report.selectedNames={comp:String(comp.name),layer:String(layer.name)};
    report.projectRenderers=attempt(function(){
        var renderers=[],visited=0;
        for(var c=1;c<=project.numItems;c++){
            var item=project.item(c);if(!(item instanceof CompItem))continue;
            for(var l=1;l<=item.numLayers;l++){
                var candidate=item.layer(l),parade=candidate.property("ADBE Effect Parade");
                for(var e=1;parade && e<=parade.numProperties;e++){
                    if(++visited>65536)throw new Error("Project inventory bound exceeded");
                    var effect=parade.property(e);if(effect.matchName!=="org.starfieldfx.particle")continue;
                    if(renderers.length>=256)throw new Error("Renderer inventory bound exceeded");
                    renderers.push({target:"p"+project.rootFolder.id+"-c"+item.id+"-l"+candidate.id,index:e,
                        comp:String(item.name),layer:String(candidate.name),effect:String(effect.name),
                        guard:number(effect,"Panel Graph Sync Guard"),commit:number(effect,"Commit Graph Edit"),
                        receipt:number(effect,"Graph Edit Receipt"),ready:number(effect,"Node Effects Ready")});
                }
            }
        }
        return renderers;
    });
    report.beforeSnapshot=attempt(function(){return inventory(layer);});
    // Only the installed read-only snapshot entry, never bootstrap or a commit.
    report.snapshot=attempt(function(){
        if(typeof host.SFLD_getGraphSnapshot!=="function")return {available:false};
        var response=JSON.parse(host.SFLD_getGraphSnapshot(JSON.stringify({protocol:"org.starfieldfx.panel",version:1,
            operation:"getGraphSnapshot",requestId:"manual-guard-report",pinTarget:true,target:{token:token}})));
        var snapshot=response.snapshot;
        return {ok:response.ok,error:response.error,gateway:response.gatewayBuild,
            initialized:snapshot?snapshot.initialized:null,revision:snapshot?snapshot.revision:null,
            checksum:snapshot?snapshot.checksum:null,nodes:snapshot?snapshot.nativeNodes.length:null,
            repairNeeded:snapshot?snapshot.repairNeeded:null,
            nativeNodes:snapshot?snapshot.nativeNodes:null,renderer:snapshot?snapshot.renderer:null};
    });
    report.afterSnapshot=attempt(function(){return inventory(layer);});
    var text=JSON.stringify(report,null,2),saved=false,path="";
    try {
        var root=(new File($.fileName)).parent.parent;
        var output=new File(root.fsName+"/artifacts/m3-20-guard-owner-20261017/host-report.json");
        path=output.fsName;output.encoding="UTF-8";
        if(output.open("w")){try{saved=output.write(text);}finally{output.close();}}
    } catch(error) {saved=false;}
    if(saved)alert("Starfield guard report saved. Project unchanged.\n"+path+"\nReply: report saved");
    else prompt("Report could not be saved. Copy this read-only report; do not change AE security settings.",text);
}());
