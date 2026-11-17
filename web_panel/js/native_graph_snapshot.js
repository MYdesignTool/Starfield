// Build the portable graph from the ordinary records owned by AE node effects.
// This runs in CEP, never in ExtendScript: no expression or CUSTOM_VALUE access.
(function (root, factory) {
    var common = typeof module === "object" && module.exports;
    var api = factory(common ? require("./graph_codec.js") : root.StarfieldGraphCodec,
        common ? require("./graph_layout.js") : root.StarfieldGraphLayout,
        common ? require("./graph_edits.js") : root.StarfieldGraphEdits);
    if (common) module.exports = api;
    else root.StarfieldNativeGraphSnapshot = api;
}(typeof window !== "undefined" ? window : this, function (codec, layout, edits) {
    "use strict";
    function normalize(response) {
        if (!response || !response.ok || !response.snapshot) return response;
        var snapshot = response.snapshot;
        if(snapshot.canvasOnly===true)return response;
        if (!snapshot.initialized) return response;
        try {
            if (!Array.isArray(snapshot.nativeNodes) || !snapshot.renderer) {
                throw {code:"invalid_native_node_record",message:"The current native control snapshot is missing. Close and reopen the Starfield panels."};
            }
            var graph = {version:1,nodes:[],edges:[],optionalRecords:[]}, positions = {};
            var emitterTargets = {},particleTargets={},modelTargets={},materialTargets={};
            snapshot.nativeNodes.forEach(function (record) {
                if(record.type==="org.starfieldfx.nodes.particle")particleTargets[record.id]=true;
                if(record.type==="org.starfieldfx.nodes.model")modelTargets[record.id]=true;
                if(record.type==="org.starfieldfx.nodes.material")materialTargets[record.id]=true;
                if (record.type === "org.starfieldfx.nodes.emitter") emitterTargets[record.id] = true;
            });
            snapshot.nativeNodes.forEach(function (record) {
                graph.nodes.push({id:record.id,type:record.type,schemaVersion:edits.nativeRecordSchema(record),
                    parameters:record.parameters.map(function (parameter) {
                        return {key:String(parameter.key),type:parameter.type,
                            value:parameter.type === 7 ? new Uint8Array(parameter.value) : parameter.value};
                    })});
                positions[record.id] = record.position;
                record.outgoing.forEach(function (edge) {
                    graph.edges.push({id:edge.id,sourceNode:record.id,
                        sourcePort:record.type === "org.starfieldfx.nodes.emitter" || record.type === "org.starfieldfx.nodes.model" || record.type==="org.starfieldfx.nodes.material" ? "1" : "2",
                        destinationNode:edge.target,destinationPort:materialTargets[edge.target] || modelTargets[edge.target] ? "2" :
                            (record.type === "org.starfieldfx.nodes.model" || record.type === "org.starfieldfx.nodes.material") && particleTargets[edge.target] ? "3" : emitterTargets[edge.target] ? "2" : "1"});
                });
            });
            var output = snapshot.renderer;
            graph.nodes.push({id:output.id,type:"org.starfieldfx.nodes.output",schemaVersion:edits.schemaVersion("output"),
                parameters:[{key:"1",type:3,value:output.maxParticles},
                    {key:"2",type:3,value:output.timeRemapEnabled || 0},{key:"3",type:4,value:output.timeRemapSeconds || 0},
                    {key:"4",type:3,value:output.previewEnabled || 0},{key:"5",type:4,value:typeof output.previewChance === "number" ? output.previewChance : 100},{key:"6",type:3,value:output.acceleration || 0},{key:"7",type:3,value:output.timeSamplingHz || 30},{key:"8",type:3,value:typeof output.motionBlur === "number" ? output.motionBlur : 1},{key:"9",type:4,value:typeof output.shutterAngle === "number" ? output.shutterAngle : 360},{key:"10",type:4,value:typeof output.shutterPhase === "number" ? output.shutterPhase : 0},{key:"11",type:3,value:typeof output.motionBlurType === "number" ? output.motionBlurType : 0},{key:"12",type:4,value:typeof output.motionBlurLevels === "number" ? output.motionBlurLevels : 8},{key:"13",type:4,value:typeof output.linearAccuracy === "number" ? output.linearAccuracy : 70},{key:"14",type:4,value:typeof output.opacityBoost === "number" ? output.opacityBoost : 0},{key:"15",type:3,value:typeof output.motionBlurDisregard === "number" ? output.motionBlurDisregard : 0}]});
            positions[output.id] = output.position;
            graph = layout.set(graph,positions);
            var bytes = codec.serialize(graph);
            var checksum = new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength).getUint32(24,true).toString(16);
            while (checksum.length < 8) checksum = "0" + checksum;
            snapshot.graphHex = codec.toHex(graph);
            snapshot.byteCount = bytes.length;
            snapshot.crc32 = checksum;
            snapshot.checksumMatches = checksum === snapshot.checksum; // diagnostic; decimal JSON may round doubles
            return response;
        } catch (error) {
            return {ok:false,target:response.target,error:{code:error.code || "invalid_native_node_record",
                message:error.message || String(error)}};
        }
    }
    function canvasGraph(snapshot){
        function invalid(){throw {code:"invalid_canvas_snapshot",message:"The canvas metadata is invalid. Refresh to read it again."};}
        if(!snapshot || snapshot.canvasOnly!==true || !snapshot.initialized || !Array.isArray(snapshot.nativeNodes) || snapshot.nativeNodes.length>63 ||
            !snapshot.renderer || !Number.isInteger(snapshot.revision) || snapshot.revision<1 || !/^[0-9a-f]{8}$/.test(snapshot.checksum))invalid();
        var geometry=snapshot.geometry;
        if(!geometry || !Number.isFinite(geometry.width) || geometry.width<=0 || !Number.isFinite(geometry.height) || geometry.height<=0 ||
            !Number.isFinite(geometry.pixelAspect) || geometry.pixelAspect<=0)invalid();
        var graph={version:1,canvasOnly:true,nodes:[],edges:[],optionalRecords:[]},positions={},ids={},edgeIds={};
        var output="000000000000000000000000000000ff";
        function position(id,p){if(!p || !Number.isFinite(p.x) || !Number.isFinite(p.y) || Math.abs(p.x)>1e9 || Math.abs(p.y)>1e9)invalid();positions[id]={x:p.x,y:p.y};}
        snapshot.nativeNodes.forEach(function(record){
            if(!record || !/^[0-9a-f]{32}$/.test(record.id) || /^0{32}$/.test(record.id) || record.id===output || ids[record.id] ||
                Object.keys(edits.types).every(function(kind){return kind==="output" || edits.types[kind]!==record.type;}) ||
                !Number.isInteger(record.schemaVersion) || record.schemaVersion<1 || record.schemaVersion>65535 ||
                !Array.isArray(record.parameters) || !Array.isArray(record.outgoing) || record.outgoing.length>4)invalid();
            record.parameters.forEach(function(p){if(!p || !/^[0-9]+$/.test(p.key) || (p.type!==3 && p.type!==4 && p.type!==5) ||
                (p.type===5?(!Array.isArray(p.value)||p.value.length!==3||!p.value.every(Number.isFinite)):!Number.isFinite(p.value)))invalid();});
            ids[record.id]=record;position(record.id,record.position);
            graph.nodes.push({id:record.id,type:record.type,schemaVersion:record.schemaVersion,parameters:record.parameters});
        });
        ids[output]={type:edits.types.output};
        if(snapshot.renderer.id!==output || !Number.isFinite(snapshot.renderer.maxParticles))invalid();
        graph.nodes.push({id:output,type:edits.types.output,schemaVersion:edits.schemaVersion("output"),parameters:[{key:"1",type:3,value:snapshot.renderer.maxParticles}]});
        position(output,snapshot.renderer.position);
        snapshot.nativeNodes.forEach(function(record){record.outgoing.forEach(function(edge){
            var dest=ids[edge.target],geometry=record.type===edits.types.model || record.type===edits.types.material;
            if(!/^[0-9a-f]{32}$/.test(edge.id) || /^0{32}$/.test(edge.id) || edgeIds[edge.id] || !dest)invalid();edgeIds[edge.id]=true;
            graph.edges.push({id:edge.id,sourceNode:record.id,sourcePort:record.type===edits.types.emitter || geometry?"1":"2",destinationNode:edge.target,
                destinationPort:dest.type===edits.types.model || dest.type===edits.types.material || dest.type===edits.types.emitter?"2":geometry && dest.type===edits.types.particle?"3":"1"});
        });});
        return {graph:graph,positions:positions};
    }
    return {normalize:normalize,canvasGraph:canvasGraph};
}));
