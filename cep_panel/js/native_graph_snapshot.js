// Build the portable graph from the ordinary records owned by AE node effects.
// This runs in CEP, never in ExtendScript: no expression or CUSTOM_VALUE access.
(function (root, factory) {
    var common = typeof module === "object" && module.exports;
    var api = factory(common ? require("./graph_codec.js") : root.StarfieldGraphCodec,
        common ? require("./graph_layout.js") : root.StarfieldGraphLayout);
    if (common) module.exports = api;
    else root.StarfieldNativeGraphSnapshot = api;
}(typeof window !== "undefined" ? window : this, function (codec, layout) {
    "use strict";
    function normalize(response) {
        if (!response || !response.ok || !response.snapshot ||
            !Array.isArray(response.snapshot.nativeNodes)) return response;
        var snapshot = response.snapshot;
        if (!snapshot.initialized) return response;
        try {
            var graph = {version:1,nodes:[],edges:[],optionalRecords:[]}, positions = {};
            var emitterTargets = {};
            snapshot.nativeNodes.forEach(function (record) {
                if (record.type === "org.starfieldfx.nodes.emitter") emitterTargets[record.id] = true;
            });
            snapshot.nativeNodes.forEach(function (record) {
                graph.nodes.push({id:record.id,type:record.type,schemaVersion:record.schemaVersion,
                    parameters:record.parameters.map(function (parameter) {
                        return {key:String(parameter.key),type:parameter.type,
                            value:parameter.type === 7 ? new Uint8Array(parameter.value) : parameter.value};
                    })});
                positions[record.id] = record.position;
                record.outgoing.forEach(function (edge) {
                    graph.edges.push({id:edge.id,sourceNode:record.id,
                        sourcePort:record.type === "org.starfieldfx.nodes.emitter" ? "1" : "2",
                        destinationNode:edge.target,destinationPort:emitterTargets[edge.target] ? "2" : "1"});
                });
            });
            var output = snapshot.renderer;
            graph.nodes.push({id:output.id,type:"org.starfieldfx.nodes.output",schemaVersion:4,
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
            return {ok:false,error:{code:error.code || "invalid_native_node_record",
                message:error.message || String(error)}};
        }
    }
    return {normalize:normalize};
}));
