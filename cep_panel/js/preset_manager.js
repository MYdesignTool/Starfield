(function(){
    "use strict";
    var presets=window.StarfieldPresets,codec=window.StarfieldGraphCodec,snapshots=window.StarfieldNativeGraphSnapshot;
    var entries=presets.catalog.slice(),category=null,selected=null,targetToken=null,revision=null,graph=null,busy=false,serial=0;
    var el={};["grid","preview","search","target","breadcrumb","selected-title","description","structure","status","refresh","import","save","home","up","all","render-settings","cancel","replace","add"].forEach(function(id){el[id]=document.getElementById(id);});
    // Keep the page and the reason visible when a dependency did not load.
    var missing=["StarfieldPresets","StarfieldGraphCodec","StarfieldNativeGraphSnapshot","StarfieldGraphTransactions"].filter(function(name){return !window[name];});
    if(missing.length){el.status.className="error";el.status.textContent="Preset interface could not load: "+missing.join(", ")+". Close and reopen Starfield Presets.";return;}
    var cep=window.__adobe_cep__,gatewayBuild="native-presets-46",readyToken="org.starfieldfx.panel/1/"+gatewayBuild;
    function literal(value){return JSON.stringify(value).replace(/\u2028/g,"\\u2028").replace(/\u2029/g,"\\u2029");}
    function status(message,error){el.status.textContent=message;el.status.className=error?"error":"";}
    function pending(value){busy=value;el.add.disabled=el.replace.disabled=value || !selected || !targetToken;el.save.disabled=value || !targetToken;el.refresh.disabled=el.import.disabled=value;el.home.disabled=el.all.disabled=el.search.disabled=value;el.up.disabled=value || (!category && !el.search.value.trim());}
    function host(script,callback){if(!cep || !cep.evalScript){callback(null);return;}cep.evalScript(script,callback);}
    function call(operation,fields,callback){
            var root=cep && cep.getSystemPath?cep.getSystemPath("extension"):null;
            if(!root){callback({ok:false,error:{code:"gateway_missing",message:"The extension path is unavailable."}});return;}
            var request={protocol:"org.starfieldfx.panel",version:1,operation:operation,requestId:"presets-"+(++serial),changes:[],target:targetToken?{token:targetToken}:{},pinTarget:!!targetToken};
            Object.keys(fields||{}).forEach(function(key){request[key]=fields[key];});
            request.gatewayBuild=gatewayBuild;
            // Native file dialogs are user controlled; render/graph calls time out.
            var settled=false,timer=operation.indexOf("PresetFile")>=0?null:setTimeout(function(){if(!settled){settled=true;callback({ok:false,error:{code:"host_timeout",message:"After Effects has not answered. Refresh before retrying."}});}},15000);
            // Loading and invocation share one evalScript turn. Another CEP page
            // cannot replace the global SFLD functions between these two steps.
            var script="(function(){try{$.evalFile(new File("+literal(root.replace(/\\/g,"/")+"/jsx/starfield_gateway.jsx")+"));"+
                "if(SFLD_ready()!=="+literal(readyToken)+")throw new Error('Gateway generation mismatch.');"+
                "return SFLD_"+operation+"("+literal(JSON.stringify(request))+");}catch(e){return JSON.stringify({protocol:'org.starfieldfx.panel',version:1,gatewayBuild:"+literal(gatewayBuild)+",ok:false,error:{code:'gateway_load_failed',message:e.toString()}});}}())";
            host(script,function(raw){if(settled)return;settled=true;if(timer)clearTimeout(timer);try{var response=JSON.parse(raw);if(!response || response.protocol!=="org.starfieldfx.panel" || response.version!==1 || response.gatewayBuild!==gatewayBuild)throw new Error("Unsupported host response or gateway generation.");callback(response);}catch(error){callback({ok:false,error:{code:"bad_response",message:"Invalid reply from After Effects: "+error.message}});}});
    }
    var client=window.StarfieldGraphTransactions.create({call:call,codec:codec,edits:{apply:presets.apply}});
    function failure(response){status(response && response.error?response.error.code+": "+response.error.message:"The host did not return a usable response.",true);pending(false);}
    function refresh(){
        if(busy)return;pending(true);targetToken=null;revision=null;graph=null;status("Finding the selected Starfield effect…");
        call("getState",{},function(response){if(!response.ok){el.target.textContent="Select a layer with Starfield in After Effects.";failure(response);return;}
            targetToken=response.target.token;el.target.textContent=response.target.comp+" / "+response.target.layer;
            call("getGraphSnapshot",{},function(record){if(!record.ok){failure(record);return;}var normalized=snapshots.normalize(record);if(!normalized.ok){failure(normalized);return;}
                revision=normalized.snapshot.revision;if(normalized.snapshot.initialized)graph=codec.fromHex(normalized.snapshot.graphHex);
                status("Target pinned. Choose a preset, then Add or Replace.");pending(false);
            });
        });
    }
    function initialize(callback){
        call("getGraphSnapshot",{},function(record){if(!record.ok){callback(record);return;}
            var finish=function(response){if(!response.ok){callback(response);return;}client.ensureNativeEffects(response.snapshot,targetToken,function(ensured){if(!ensured.ok){callback(ensured);return;}var normalized=snapshots.normalize(ensured);if(normalized.ok){revision=normalized.snapshot.revision;graph=codec.fromHex(normalized.snapshot.graphHex);}callback(normalized);});};
            if(record.snapshot.initialized)finish(record);else call("syncGraphSnapshot",{},finish);
        });
    }
    function apply(mode){
        if(busy || !selected || !targetToken)return;pending(true);status("Applying "+selected.name+"…");
        var choice=selected,settings=el["render-settings"].checked;
        initialize(function(response){if(!response.ok){failure(response);return;}
            client.apply({type:"applyPreset",presetId:choice.graph?null:choice.id,presetGraph:choice.graph,mode:mode,applyRenderSettings:settings},function(committed){
                if(!committed.ok){failure(committed);return;}revision=committed.snapshot.revision;graph=codec.fromHex(committed.snapshot.graphHex);status(choice.name+(mode==="add"?" added.":" replaced the current setup.")+" Undo in After Effects restores the previous graph.");pending(false);
            },targetToken,revision);
        });
    }
    function illustration(canvas,entry){
        var c=canvas.getContext("2d"),w=canvas.width,h=canvas.height;c.fillStyle="#090b0e";c.fillRect(0,0,w,h);
        var seed=entry.id.split("").reduce(function(n,x){return (n*31+x.charCodeAt(0))>>>0;},7);
        function random(){seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296;}
        c.fillStyle=entry.color || "#99badd";
        for(var i=0;i<140;i++){var x=random()*w,y=random()*h,r=.4+random()*1.8;
            if(entry.id==="orbit"){var a=i*.27,rad=4+i*w/360;x=w/2+Math.cos(a)*rad;y=h/2+Math.sin(a)*rad*.6;}
            else if(entry.id==="sparks"){x=w/2+(random()-.5)*w*.5*(1-y/h);}
            c.globalAlpha=.2+random()*.8;if(entry.id==="ribbons"){c.save();c.translate(x,y);c.rotate(random()*6.28);c.fillRect(-4,-1,8,2);c.restore();}else{c.beginPath();c.arc(x,y,r,0,Math.PI*2);c.fill();}
        }c.globalAlpha=1;
    }
    function select(entry){selected=entry;render();el["selected-title"].textContent=entry.name;el.description.textContent=entry.description;
        var previewGraph=entry.graph || presets.build(entry.id,1080);el.structure.textContent=previewGraph.nodes.length+" nodes · "+previewGraph.edges.length+" connections · illustration preview";illustration(el.preview,entry);pending(busy);
    }
    function render(){
        el.grid.textContent="";el.breadcrumb.textContent=category || "Categories";var query=el.search.value.trim().toLowerCase();el.up.disabled=busy || (!category && !query);
        if(!category && !query){presets.categories.slice(1).forEach(function(name){var folder=document.createElement("button");folder.className="card";folder.setAttribute("role","listitem");
            var art=document.createElement("div");art.className="folder-art";art.innerHTML='<svg viewBox="0 0 100 80" aria-hidden="true"><path d="M5 18h32l8 9h48v49H5zM8 10h28l8 8H8z"/></svg>';
            var text=document.createElement("span");text.textContent=name;folder.appendChild(art);folder.appendChild(text);folder.addEventListener("click",function(){category=name;render();});el.grid.appendChild(folder);});return;}
        var found=entries.filter(function(entry){return (!category || category==="All presets" || entry.category===category) && (!query || (entry.name+" "+entry.category+" "+entry.description).toLowerCase().indexOf(query)>=0);});
        found.forEach(function(entry){var card=document.createElement("button");card.className="card"+(selected===entry?" selected":"");card.setAttribute("role","listitem");card.setAttribute("aria-pressed",selected===entry?"true":"false");var canvas=document.createElement("canvas");canvas.width=320;canvas.height=180;illustration(canvas,entry);var label=document.createElement("span");label.textContent=entry.name;card.appendChild(canvas);card.appendChild(label);card.addEventListener("click",function(){select(entry);});el.grid.appendChild(card);});
        if(!found.length){var empty=document.createElement("div");empty.className="empty";empty.textContent=category==="My Presets"?"Import a Starfield preset, or save the current graph.":"No presets match this search.";el.grid.appendChild(empty);}
    }
    function browse(destination){category=destination;selected=null;el.search.value="";el["selected-title"].textContent="Choose a preset";el.description.textContent="Browse a category or search the library.";el.structure.textContent="";el.preview.getContext("2d").clearRect(0,0,el.preview.width,el.preview.height);render();pending(busy);}
    el.search.addEventListener("input",render);el.home.addEventListener("click",function(){browse(null);});el.up.addEventListener("click",function(){browse(null);});el.all.addEventListener("click",function(){browse("All presets");});
    el.refresh.addEventListener("click",refresh);el.add.addEventListener("click",function(){apply("add");});el.replace.addEventListener("click",function(){apply("replace");});
    el.cancel.addEventListener("click",function(){if(busy)return;if(cep && cep.closeExtension)cep.closeExtension();else window.close();});
    el.import.addEventListener("click",function(){if(busy)return;pending(true);call("readPresetFile",{},function(response){if(!response.ok){failure(response);return;}if(response.cancelled){pending(false);return;}try{var entry=presets.decode(response.text);entries.push(entry);category="My Presets";el.search.value="";select(entry);status("Imported "+entry.name+". Choose Add or Replace to apply.");pending(false);}catch(error){failure({error:{code:error.code||"invalid_preset",message:error.message}});}});});
    el.save.addEventListener("click",function(){if(busy || !targetToken)return;var name=window.prompt("Preset name","My Particle Setup");if(!name)return;pending(true);
        initialize(function(response){if(!response.ok){failure(response);return;}var text;try{text=presets.encode(graph,name,"My Presets");}catch(error){failure({error:{code:error.code,message:error.message}});return;}
            call("writePresetFile",{text:text},function(saved){if(!saved.ok){failure(saved);return;}if(saved.cancelled){pending(false);return;}var entry=presets.decode(text);entries.push(entry);category="My Presets";select(entry);status("Saved "+entry.name+" to "+saved.path);pending(false);});
        });
    });
    render();refresh();
}());
