/* Explicit AE2023 smoke test for the frozen native132 MFR qualification build.
 * Never run against an existing project. Output and project are confined to
 * artifacts/mfr-01-20261116/host-native132. This is not full feature qualification.
 * MFR configuration is scoped to this script; no preference/registry writes.
 */
(function () {
    var root=File($.fileName).parent.parent;
    $.evalFile(File(root.fsName+"/web_panel/jsx/starfield_gateway.jsx"));
    var codec=$.global.__SFLD_modelTransactionAPI.json;
    var parent=Folder(root.fsName+"/artifacts/mfr-01-20261116/host-native132");
    var run=Folder(parent.fsName+"/run-"+(new Date().getTime()));
    var report={nativeBuild:132,scope:"AE2023 smoke only; resource-family and full precision acceptance still required",passed:false,cases:[],outputs:[],host:String(app.version)};
    var owned=false;
    function writeReport(){var file=File(run.fsName+"/report.json");file.encoding="UTF-8";if(!file.open("w"))throw new Error("Cannot write qualification report.");file.write(codec.stringify(report));file.close();}
    function find(group,match){
        var child=null;try{child=group.property(match);}catch(ignored){}
        if(child && child.matchName===match)return child;
        for(var i=1;i<=group.numProperties;i++){child=group.property(i);if(child && child.matchName===match)return child;
            if(child && child.numProperties){var nested=find(child,match);if(nested)return nested;}}
        return null;
    }
    function parameter(effect,id){var suffix=String(id);while(suffix.length<4)suffix="0"+suffix;
        var p=find(effect,effect.matchName+"-"+suffix)||find(effect,effect.matchName+"-"+id);
        if(!p)throw new Error("Missing exact disk parameter "+id);return p;}
    function template(om){for(var i=0;i<om.templates.length;i++)if(/png/i.test(om.templates[i]))return om.templates[i];
        throw new Error("No PNG output template. No templates or preferences will be created.");}
    function build(kind){
        var comp=app.project.items.addComp("MFR132-"+kind,960,540,1,4,24);
        var layer=comp.layers.addSolid([0,0,0],"Starfield qualification",960,540,1,4);
        var effects=layer.property("ADBE Effect Parade");
        if(kind==="legacy-nodes"){
            var types=["emitter","particle","force","transform","model","material","motion","turbulence"];
            for(var i=0;i<types.length;i++)layer.property("ADBE Effect Parade").addProperty("org.starfieldfx.node."+types[i]);
        }
        // Reacquire indexed groups after addProperty; AE may invalidate them.
        layer.property("ADBE Effect Parade").addProperty("org.starfieldfx.particle");
        var fx=layer.property("ADBE Effect Parade").property(layer.property("ADBE Effect Parade").numProperties);
        if(kind==="legacy-nodes"){
            parameter(fx,29).setValue(1);
            parameter(fx,3).setValueAtTime(0,2000);parameter(fx,3).setValueAtTime(3,9000);
            parameter(fx,12).setValue(3);parameter(fx,13).setValue(11);
        }else{
            layer.threeDLayer=true;
            var camera=comp.layers.addCamera("Qualification camera",[480,270]);
            camera.property("ADBE Transform Group").property("ADBE Position").setValueAtTime(0,[480,270,-1000]);
            camera.property("ADBE Transform Group").property("ADBE Position").setValueAtTime(4,[550,280,-900]);
        }
        for(var layerIndex=1;layerIndex<=comp.numLayers;layerIndex++)comp.layer(layerIndex).selected=false;
        return comp;
    }
    try {
        if(String(app.version).split(".")[0]!=="23")throw new Error("This qualification is restricted to AE2023.");
        if(!app.project || app.project.file || app.project.numItems!==0)throw new Error("Refusing to touch an existing/nonempty project.");
        if(typeof app.setMultiFrameRenderingConfig!=="function")throw new Error("MFR scripting API unavailable.");
        if(!parent.exists && !parent.create())throw new Error("Cannot create qualification directory.");
        if(!run.create())throw new Error("Cannot create unique qualification run.");
        owned=true;
        var comps=[build("factory-camera"),build("legacy-nodes")];
        comps[0].openInViewer();
        var originalGpu=app.project.gpuAccelType;
        var gpuTypes=app.availableGPUAccelTypes||[],canCuda=false;
        for(var g=0;g<gpuTypes.length;g++)if(gpuTypes[g]===GpuAccelType.CUDA)canCuda=true;
        report.cudaAvailable=canCuda;
        // 8/16/32 project depths exercise host selectors, but PNG output is an
        // 8-bit smoke comparison. Never report this as full F32/HDR acceptance.
        for(var d=0;d<3;d++)for(var c=0;c<comps.length;c++)for(var backend=0;backend<(canCuda?2:1);backend++){
            var depth=[8,16,32][d];app.project.bitsPerChannel=depth;
            app.project.gpuAccelType=backend?GpuAccelType.CUDA:GpuAccelType.SOFTWARE;
            var fx=comps[c].layer("Starfield qualification").property("ADBE Effect Parade");fx=fx.property(fx.numProperties);
            parameter(fx,1611).setValue(backend?1:2);
            var caseName=depth+"bpc-"+c+"-"+(backend?"cuda":"cpu");
            for(var parallel=0;parallel<2;parallel++){
                while(app.project.renderQueue.numItems)app.project.renderQueue.item(1).remove();
                var directory=Folder(run.fsName+"/"+caseName+"-"+(parallel?"parallel":"serial"));if(!directory.create())throw new Error("Cannot create output directory.");
                var item=app.project.renderQueue.items.add(comps[c]);item.timeSpanStart=1;item.timeSpanDuration=2;
                var om=item.outputModule(1),chosen=template(om);om.applyTemplate(chosen);
                item.outputModule(1).file=File(directory.fsName+"/frame_[#####].png");
                app.setMultiFrameRenderingConfig(parallel===1,100);
                report.outputs.push({name:caseName,parallel:parallel===1,directory:directory.fsName,outputTemplate:chosen});writeReport();
                app.project.renderQueue.render();
                if(item.status!==RQItemStatus.DONE)throw new Error("Render did not finish: "+caseName);
            }
            report.cases.push(caseName);writeReport();
        }
        app.project.gpuAccelType=originalGpu;
        app.project.save(File(run.fsName+"/qualification.aep"));report.passed=true;writeReport();
    }catch(error){report.error=String(error);try{if(run.exists)writeReport();}catch(ignoredReport){}}
    finally {
        if(owned){app.project.close(CloseOptions.DO_NOT_SAVE_CHANGES);
            // Let the scoped MFR setting revert when this script returns before
            // requesting normal application shutdown. Never terminate a process.
            app.scheduleTask("app.quit();",1000,false);}
    }
})();
