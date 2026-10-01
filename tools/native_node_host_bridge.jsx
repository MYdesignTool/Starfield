// Minimal, bounded AE 2023 integration bridge for Check-NativeNodes.cjs.
// Uses only the authorized test project and artifacts/host-check. No preferences.
(function () {
    var root = File($.fileName).parent.parent.fsName;
    var folder = new Folder(root + "/artifacts/host-check");
    function quote(value) { return '"' + String(value).replace(/\\/g,"\\\\").replace(/"/g,'\\"').replace(/\r/g,"\\r").replace(/\n/g,"\\n") + '"'; }
    function write(name, text) {
        var file = new File(folder.fsName + "/" + name); file.encoding = "UTF-8";
        if (!file.open("w")) throw new Error("Cannot write host-check file: " + name);
        file.write(text); file.close();
    }
    var state = { lastSerial: 0, started: (new Date()).getTime(), comp: null, task: null };
    function cleanup() {
        if (state.task !== null) app.cancelTask(state.task);
        if (state.comp) { state.comp.remove(); state.comp = null; }
    }
    function nodeId(effect) {
        var id = "";
        for (var i=0;i<8;i++) {
            var part = Number(effect.property("Node UUID " + i).value).toString(16);
            while (part.length<4) part="0"+part;
            id += part;
        }
        return id;
    }
    function inspect() {
        var parade = state.layer.property("ADBE Effect Parade"), records = [];
        for(var i=1;i<=parade.numProperties;i++) {
            var effect=parade.property(i);
            if(effect.matchName.indexOf("org.starfieldfx.node.")!==0) continue;
            var size=effect.property("Size");
            records.push('{"id":'+quote(nodeId(effect))+',"matchName":'+quote(effect.matchName)+
                ',"size":'+(size?Number(size.value):'null')+'}');
        }
        return '{"ok":true,"effects":['+records.join(',')+'],"maxParticles":'+
            Number(parade.property(1).property(27).value)+'}';
    }
    state.tick = function () {
        if ((new Date()).getTime()-state.started>180000) { cleanup(); return; }
        var file=new File(folder.fsName+"/in.json");
        if(!file.exists || !file.open("r")) return;
        file.encoding="UTF-8"; var input=file.read();file.close();
        // Only locally generated test input is read; this is not a product API.
        var request=eval('('+input+')');
        if(!request || request.serial<=state.lastSerial) return;
        state.lastSerial=request.serial;
        var response;
        try {
            if(request.action==="finish") { cleanup(); response='{"ok":true,"cleaned":true}'; }
            else if(request.action==="inspect") response=inspect();
            else if(request.action==="deleteNode" || request.action==="setNodeSize") {
                var parade=state.layer.property("ADBE Effect Parade"),found=false;
                for(var i=parade.numProperties;i>=1;i--) {
                    var effect=parade.property(i);
                    if(effect.matchName.indexOf("org.starfieldfx.node.")!==0 || nodeId(effect)!==request.nodeId) continue;
                    if(request.action==="deleteNode") effect.remove();
                    else effect.property("Size").setValue(request.value);
                    found=true;break;
                }
                response='{"ok":'+found+'}';
            } else {
                var allowed={getState:1,getGraphSnapshot:1,syncGraphSnapshot:1,ensureNodeEffects:1,submitGraph:1};
                if(!allowed[request.operation]) throw new Error("Unsupported host-check operation.");
                response=$.global["SFLD_"+request.operation](request.body);
            }
        } catch(error) { response='{"ok":false,"error":{"code":"host_check_error","message":'+quote(error.toString())+'}}'; }
        write("out.json",'{"serial":'+request.serial+',"response":'+response+'}');
    };
    try {
        write("stage.txt", "script_started");
        // Do not consume the finish request left by a previous failed run.
        write("in.json", '{"serial":0}');
        // Development parameter schemas are intentionally not migrated. Use a
        // clean temporary composition instead of loading an obsolete effect schema.
        if (app.project.file && app.project.file.fsName.toLowerCase() !==
            new File("D:/Project/Code/test/testproject.aep").fsName.toLowerCase()) {
            throw new Error("Refusing to replace a project outside the authorized test scope.");
        }
        app.newProject();
        write("stage.txt", "clean_project_created");
        state.comp=app.project.items.addComp("Starfield native node check",640,360,1,2,24);
        state.layer=state.comp.layers.addSolid([0,0,0],"Native node check",640,360,1,2);
        write("stage.txt", "before_open_empty_comp");
        state.comp.openInViewer();
        write("stage.txt", "empty_comp_opened");
        state.comp.time=1;state.layer.selected=true;
        write("stage.txt", "before_add_renderer");
        state.layer.property("ADBE Effect Parade").addProperty("org.starfieldfx.particle");
        write("stage.txt", "renderer_added");
        $.evalFile(new File(root+"/cep_panel/jsx/starfield_gateway.jsx"));
        write("stage.txt", "gateway_loaded");
        $.global.SFLD_HostCheck=state;
        write("ready.json",'{"ok":true,"version":'+quote(app.version)+'}');
        state.task=app.scheduleTask("$.global.SFLD_HostCheck.tick()",150,true);
    } catch(error) {
        cleanup();write("ready.json",'{"ok":false,"error":'+quote(error.toString())+'}');
    }
}());
