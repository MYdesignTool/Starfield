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
                        destinationNode:edge.target,destinationPort:"1"});
                });
            });
            var output = snapshot.renderer;
            graph.nodes.push({id:output.id,type:"org.starfieldfx.nodes.output",schemaVersion:2,
                parameters:[{key:"1",type:3,value:output.maxParticles}]});
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
