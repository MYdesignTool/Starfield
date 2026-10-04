// Independently authored procedural presets. Only portable authoring data survives export.
(function(root,factory){
    var node=typeof module==="object" && module.exports;
    var api=factory(node?require("./graph_edits.js"):root.StarfieldGraphEdits,
        node?require("./graph_layout.js"):root.StarfieldGraphLayout,
        node?require("./graph_codec.js"):root.StarfieldGraphCodec,
        node?require("./graph_view.js"):root.StarfieldGraphView);
    if(node)module.exports=api;else root.StarfieldPresets=api;
}(typeof window!=="undefined"?window:this,function(edits,layout,codec,view){
    "use strict";
    var OUTPUT="000000000000000000000000000000ff",MAX_BYTES=24*1024;
    var categories=["All presets","Backgrounds","Effects","Graphic Elements","Physical","Nodes","My Presets"];
    var catalog=[
        {id:"starlight",name:"Starlight Field",category:"Backgrounds",description:"A layered field of small blue stars.",color:"#8dbce9"},
        {id:"sparks",name:"Warm Sparks",category:"Effects",description:"Rising amber sparks with gravity and fading color.",color:"#efa34c"},
        {id:"orbit",name:"Orbital Drift",category:"Effects",description:"A gentle spiral shaped by Wind and Spin.",color:"#74cdd5"},
        {id:"ribbons",name:"Ribbon Confetti",category:"Graphic Elements",description:"Rotating rectangles with offset lifetime colors.",color:"#d09bdb"},
        {id:"snow",name:"Soft Snow",category:"Physical",description:"Wide emission, gentle gravity and feathered circles.",color:"#c7ddea"},
        {id:"trails",name:"Secondary Trails",category:"Nodes",description:"Particles emit a second stream through Auxiliary.",color:"#9adbaf"}
    ];
    function fail(message){var e=new Error(message);e.code="invalid_preset";throw e;}
    function clone(graph){return codec.fromHex(codec.toHex(graph));}
    function parameter(node,key,value,type){var found=node.parameters.filter(function(p){return p.key===String(key);})[0];if(found)found.value=value;else node.parameters.push({key:String(key),type:type||4,value:value});}
    function gradient(node,colors){parameter(node,12,1,3);parameter(node,13,view.encodeGradient(colors.map(function(color,i){return {position:i/(colors.length-1),color:color};})),7);}
    function build(id,height){
        height=Number(height)||1080;var entry=catalog.filter(function(p){return p.id===id;})[0];if(!entry)fail("Unknown built-in preset.");
        var graph={version:1,nodes:[],edges:[],optionalRecords:[]},count=1;
        function next(){var s=(count++).toString(16);return new Array(33-s.length).join("0")+s;}
        ["emitter","particle","force","output"].forEach(function(kind){graph=edits.apply(graph,{type:"addNode",nodeType:kind,layerHeightPixels:height},kind==="output"?function(){return OUTPUT;}:next);});
        var emitter=graph.nodes[0],particle=graph.nodes[1],force=graph.nodes[2],output=graph.nodes[3];
        graph=edits.apply(graph,{type:"connect",from:emitter.id,to:particle.id,fromPort:"1",toPort:"1"},next);
        graph=edits.apply(graph,{type:"connect",from:particle.id,to:force.id,fromPort:"2",toPort:"1"},next);
        graph=edits.apply(graph,{type:"connect",from:force.id,to:output.id,fromPort:"2",toPort:"1"},next);
        emitter=graph.nodes[0];particle=graph.nodes[1];force=graph.nodes[2];output=graph.nodes[3];
        parameter(output,1,1000000,3);parameter(emitter,17,1,3);
        parameter(particle,7,view.encodeCurve([{age:0,value:100},{age:1,value:0}]),7);
        parameter(particle,8,view.encodeCurve([{age:0,value:0},{age:.1,value:100},{age:.8,value:100},{age:1,value:0}]),7);
        if(id==="starlight") {
            parameter(emitter,2,2000);parameter(emitter,5,1,3);parameter(emitter,19,1000);parameter(emitter,20,600);parameter(emitter,21,300);parameter(emitter,12,20/height);
            parameter(particle,11,4);parameter(particle,3,2);parameter(particle,14,40);parameter(particle,9,60);gradient(particle,[[.25,.45,.9],[.85,.95,1]]);
        } else if(id==="sparks") {
            parameter(emitter,2,1200);parameter(emitter,6,[0,-.3,0],5);parameter(emitter,17,0,3);parameter(emitter,18,22);parameter(emitter,12,320/height);parameter(emitter,22,45);
            parameter(particle,11,2.5);parameter(particle,14,35);parameter(particle,3,4);parameter(particle,9,60);parameter(force,1,[0,-130/height,0],5);
            gradient(particle,[[1,.95,.55],[1,.35,.03],[.45,.03,.01]]);
        } else if(id==="orbit") {
            parameter(emitter,2,600);parameter(emitter,5,3,3);parameter(emitter,10,.08);parameter(emitter,12,35/height);
            parameter(particle,11,5);parameter(particle,3,3);parameter(force,5,200/height);parameter(force,6,.4);parameter(force,7,20);
            gradient(particle,[[.15,.65,.85],[.9,.6,.25]]);
        } else if(id==="ribbons") {
            parameter(emitter,2,500);parameter(emitter,12,170/height);parameter(particle,15,1,3);parameter(particle,3,12);parameter(particle,16,3);
            parameter(particle,19,100);parameter(particle,20,[15,30,120],5);parameter(particle,21,50);parameter(particle,11,3);parameter(particle,12,2,3);
            parameter(particle,13,view.encodeGradient([{position:0,color:[1,.3,.45]},{position:.5,color:[.2,.6,1]},{position:1,color:[.9,.75,.3]}]),7);
        } else if(id==="snow") {
            parameter(emitter,2,400);parameter(emitter,5,1,3);parameter(emitter,19,1200);parameter(emitter,20,5);parameter(emitter,21,50);parameter(emitter,6,[0,.35,0],5);parameter(emitter,12,25/height);
            parameter(particle,11,6);parameter(particle,3,5);parameter(particle,9,60);parameter(particle,23,70);parameter(force,1,[0,-25/height,0],5);parameter(force,4,[10/height,0,0],5);
        } else if(id==="trails") {
            parameter(emitter,2,30);parameter(emitter,12,160/height);parameter(particle,11,3);parameter(particle,3,5);
            graph=edits.apply(graph,{type:"addNode",nodeType:"auxiliary",layerHeightPixels:height},next);graph=edits.apply(graph,{type:"addNode",nodeType:"particle",layerHeightPixels:height},next);
            var auxiliary=graph.nodes[4],child=graph.nodes[5];parameter(auxiliary,2,12);parameter(auxiliary,12,0);parameter(auxiliary,27,70);parameter(child,11,.7);parameter(child,3,2);gradient(child,[[.3,.9,.5],[.15,.45,.9]]);
            graph=edits.apply(graph,{type:"connect",from:particle.id,to:auxiliary.id,fromPort:"2",toPort:"2"},next);
            graph=edits.apply(graph,{type:"connect",from:auxiliary.id,to:child.id,fromPort:"1",toPort:"1"},next);
            graph=edits.apply(graph,{type:"connect",from:child.id,to:output.id,fromPort:"2",toPort:"1"},next);
        }
        var positions={};graph.nodes.forEach(function(n,i){positions[n.id]={x:80+(i%4)*180,y:70+Math.floor(i/4)*180};});
        return layout.set(graph,positions);
    }
    function authoring(graph,context){
        var clean=clone(graph),positions=layout.resolve(clean);clean.optionalRecords=[];
        clean=layout.set(clean,positions);validate(clean,context);return clean;
    }
    function validate(graph,context){
        if(codec.toHex(graph).length/2>MAX_BYTES)fail("Preset exceeds the 24 KiB project limit.");
        if(graph.nodes.length>64 || graph.edges.length>256)fail("Preset has too many nodes or connections.");
        var outputs=graph.nodes.filter(function(n){return n.type===edits.types.output;});if(outputs.length!==1 || outputs[0].id!==OUTPUT)fail("Preset needs one renderer Output.");
        if(typeof edits.schemaVersion!=="function")fail("The node library did not update. Close and reopen Starfield Presets.");
        graph.nodes.forEach(function(n){
            var kind=Object.keys(edits.types).filter(function(k){return edits.types[k]===n.type;})[0];
            if(!kind)fail((context||"Preset")+": unsupported node type "+n.type+".");
            var expected=edits.schemaVersion(kind);
            if(n.schemaVersion!==expected)fail((context||"Preset")+": "+kind+" has unsupported node schema "+n.schemaVersion+"; expected "+expected+".");
            if(graph.edges.filter(function(e){return e.sourceNode===n.id;}).length>4)fail("A node can have at most four outgoing connections.");
        });
        // Check every edge and cycle using the same authoring planner as the canvas.
        var check=clone(graph);check.edges=[];graph.edges.forEach(function(e){check=edits.apply(check,{type:"connect",from:e.sourceNode,to:e.destinationNode,fromPort:e.sourcePort,toPort:e.destinationPort},function(){return e.id;});});
        return true;
    }
    function apply(base,edit,idFactory){
        if(edit.type!=="applyPreset")return edits.apply(base,edit,idFactory);
        var preset=authoring(edit.presetGraph || build(edit.presetId,edit.layerHeightPixels),"Selected preset"),current=clone(base);
        var oldOutput=current.nodes.filter(function(n){return n.type===edits.types.output;})[0];if(!oldOutput)fail("The current graph has no Output.");
        var positions=layout.resolve(current),sourcePositions=layout.resolve(preset),occupied={};
        current.nodes.concat(preset.nodes).forEach(function(n){occupied[n.id]=true;});current.edges.concat(preset.edges).forEach(function(e){occupied[e.id]=true;});
        function next(){for(var attempt=0;attempt<128;attempt++){var id=(idFactory||edits.randomId)();if(typeof id==="string" && /^[0-9a-f]{32}$/.test(id) && !/^0{32}$/.test(id) && id!==OUTPUT && !occupied[id]){occupied[id]=true;return id;}}fail("Could not allocate a new preset identity.");}
        var map={},incomingOutput=preset.nodes.filter(function(n){return n.type===edits.types.output;})[0];map[incomingOutput.id]=oldOutput.id;
        if(edit.mode==="replace") {current={version:1,nodes:[oldOutput],edges:[],optionalRecords:[]};positions={};positions[oldOutput.id]=sourcePositions[incomingOutput.id];}
        else if(edit.mode!=="add")fail("Choose Add or Replace.");
        if(edit.applyRenderSettings)oldOutput.parameters=incomingOutput.parameters;
        var offset=edit.mode==="add"?Math.max.apply(null,[0].concat(Object.keys(positions).map(function(id){return positions[id].y;})))+180:0;
        preset.nodes.forEach(function(n){if(n.id===incomingOutput.id)return;var id=next();map[n.id]=id;current.nodes.push({id:id,type:n.type,schemaVersion:n.schemaVersion,parameters:n.parameters});positions[id]={x:sourcePositions[n.id].x,y:sourcePositions[n.id].y+offset};});
        preset.edges.forEach(function(e){current.edges.push({id:next(),sourceNode:map[e.sourceNode],sourcePort:e.sourcePort,destinationNode:map[e.destinationNode],destinationPort:e.destinationPort});});
        current=layout.set(current,positions);validate(current,"Updated project");return current;
    }
    function encode(graph,name,category){if(typeof name!=="string" || !name.trim() || name.length>120)fail("Enter a preset name up to 120 characters.");return JSON.stringify({format:"org.starfieldfx.preset",version:1,name:name.trim(),category:category||"My Presets",graphHex:codec.toHex(authoring(graph))},null,2);}
    function decode(text){if(typeof text!=="string" || text.length>MAX_BYTES*2+2048)fail("Preset file is too large.");var data=JSON.parse(text);if(!data || data.format!=="org.starfieldfx.preset" || data.version!==1 || typeof data.name!=="string" || !data.name.trim() || data.name.length>120 || typeof data.graphHex!=="string" || data.graphHex.length>MAX_BYTES*2)fail("This is not a supported Starfield preset.");var graph=authoring(codec.fromHex(data.graphHex));return {id:"imported-"+edits.randomId(),name:data.name,category:"My Presets",description:"Imported Starfield preset",color:"#91bde0",graph:graph};}
    return {catalog:catalog,categories:categories,build:build,apply:apply,encode:encode,decode:decode,authoring:authoring,validate:validate};
}));
