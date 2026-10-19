// Project-owned node-card positions stored as an optional schema-1 graph record.
(function (root, factory) {
    var codec = typeof module === "object" && module.exports
        ? require("./graph_codec.js") : root.StarfieldGraphCodec;
    var api = factory(codec);
    if (typeof module === "object" && module.exports) module.exports = api;
    else root.StarfieldGraphLayout = api;
}(typeof window !== "undefined" ? window : this, function (codec) {
    "use strict";

    var RECORD_KIND = 0x8001;
    var RECORD_VERSION = 1;
    var HEADER_BYTES = 12;
    var ENTRY_BYTES = 32;
    var MAX_NODES = 4096;
    var MAX_COORDINATE = 1000000000;

    function fail(code, message) {
        var result = new Error(message);
        result.code = code;
        throw result;
    }

    function readU16(bytes, offset) {
        return bytes[offset] | (bytes[offset + 1] << 8);
    }

    function readU32(bytes, offset) {
        return (bytes[offset] | (bytes[offset + 1] << 8) |
                (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
    }

    function writeU16(bytes, offset, value) {
        bytes[offset] = value & 0xff;
        bytes[offset + 1] = (value >>> 8) & 0xff;
    }

    function writeU32(bytes, offset, value) {
        bytes[offset] = value & 0xff;
        bytes[offset + 1] = (value >>> 8) & 0xff;
        bytes[offset + 2] = (value >>> 16) & 0xff;
        bytes[offset + 3] = (value >>> 24) & 0xff;
    }

    function isLayoutRecord(hex) {
        if (typeof hex !== "string" || hex.length < 16 || hex.length % 2 !== 0) return false;
        var bytes = codec.hexToBytes(hex);
        return bytes.length >= 8 && readU16(bytes, 0) === RECORD_KIND;
    }

    function graphNodeMap(graph) {
        var map = {};
        for (var i = 0; i < graph.nodes.length; i++) map["$" + graph.nodes[i].id] = graph.nodes[i];
        return map;
    }

    function validatePosition(position) {
        if (!position || typeof position.x !== "number" || typeof position.y !== "number" ||
            !isFinite(position.x) || !isFinite(position.y) ||
            Math.abs(position.x) > MAX_COORDINATE || Math.abs(position.y) > MAX_COORDINATE) {
            fail("invalid_layout", "node positions must be finite coordinates within the signed layout range");
        }
    }

    function parseRecord(hex, nodesById) {
        var bytes = codec.hexToBytes(hex);
        if (bytes.length < HEADER_BYTES || readU16(bytes, 0) !== RECORD_KIND ||
            readU16(bytes, 2) !== RECORD_VERSION || readU32(bytes, 4) !== bytes.length) {
            fail("invalid_layout", "node layout record header is invalid");
        }
        var count = readU32(bytes, 8);
        if (count > MAX_NODES || HEADER_BYTES + count * ENTRY_BYTES !== bytes.length) {
            fail("invalid_layout", "node layout record has an invalid entry count or length");
        }
        var map = {};
        var view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
        var offset = HEADER_BYTES;
        for (var i = 0; i < count; i++) {
            var id = codec.bytesToHex(bytes.subarray(offset, offset + 16));
            if (!nodesById["$" + id] || Object.prototype.hasOwnProperty.call(map, id)) {
                fail("invalid_layout", "node layout record contains an unknown or duplicate node ID");
            }
            var position = { x: view.getFloat64(offset + 16, true),
                             y: view.getFloat64(offset + 24, true) };
            validatePosition(position);
            map[id] = position;
            offset += ENTRY_BYTES;
        }
        return map;
    }

    function get(graph) {
        if (!graph || Object.prototype.toString.call(graph.nodes) !== "[object Array]") {
            fail("invalid_graph", "node layout requires a graph with a node array");
        }
        var records = graph.optionalRecords || [];
        var found = null;
        for (var i = 0; i < records.length; i++) {
            if (!isLayoutRecord(records[i])) continue;
            if (found !== null) fail("invalid_layout", "graph contains more than one node layout record");
            found = records[i];
        }
        return found === null ? null : parseRecord(found, graphNodeMap(graph));
    }

    function encode(graph, positions) {
        if (!graph || Object.prototype.toString.call(graph.nodes) !== "[object Array]" ||
            Object.prototype.toString.call(positions) !== "[object Object]" || graph.nodes.length > MAX_NODES) {
            fail("invalid_layout", "complete node positions are required for the graph");
        }
        var nodesById = graphNodeMap(graph);
        var ids = Object.keys(positions).sort();
        if (ids.length !== graph.nodes.length) {
            fail("invalid_layout", "project layout must include every graph node exactly once");
        }
        for (var i = 0; i < graph.nodes.length; i++) {
            if (!Object.prototype.hasOwnProperty.call(positions, graph.nodes[i].id)) {
                fail("invalid_layout", "project layout is missing a node position");
            }
        }
        var bytes = new Uint8Array(HEADER_BYTES + ids.length * ENTRY_BYTES);
        var view = new DataView(bytes.buffer);
        writeU16(bytes, 0, RECORD_KIND);
        writeU16(bytes, 2, RECORD_VERSION);
        writeU32(bytes, 4, bytes.length);
        writeU32(bytes, 8, ids.length);
        var offset = HEADER_BYTES;
        for (var j = 0; j < ids.length; j++) {
            var id = ids[j];
            if (!nodesById["$" + id]) fail("invalid_layout", "project layout contains a node absent from the graph");
            var position = positions[id];
            validatePosition(position);
            var idBytes = codec.hexToBytes(id);
            bytes.set(idBytes, offset);
            view.setFloat64(offset + 16, position.x, true);
            view.setFloat64(offset + 24, position.y, true);
            offset += ENTRY_BYTES;
        }
        return codec.bytesToHex(bytes);
    }

    function set(graph, positions) {
        var updated = {
            version: graph.version,
            nodes: graph.nodes,
            edges: graph.edges,
            optionalRecords: (graph.optionalRecords || []).filter(function (record) {
                return !isLayoutRecord(record);
            })
        };
        updated.optionalRecords.push(encode(graph, positions));
        return updated;
    }

    function derive(graph) {
        var indegree = {};
        var ranks = {};
        var touched = {};
        var outgoing = {};
        var nodesById = graphNodeMap(graph);
        for (var i = 0; i < graph.nodes.length; i++) {
            var id = graph.nodes[i].id;
            indegree["$" + id] = 0;
            ranks["$" + id] = 0;
            outgoing["$" + id] = [];
        }
        for (var e = 0; e < graph.edges.length; e++) {
            var edge = graph.edges[e];
            if (!nodesById["$" + edge.sourceNode] || !nodesById["$" + edge.destinationNode]) continue;
            outgoing["$" + edge.sourceNode].push(edge.destinationNode);
            indegree["$" + edge.destinationNode] += 1;
            touched["$" + edge.sourceNode] = true;
            touched["$" + edge.destinationNode] = true;
        }
        var ready = [];
        var degree = {};
        for (var n = 0; n < graph.nodes.length; n++) {
            var nodeId = graph.nodes[n].id;
            degree["$" + nodeId] = indegree["$" + nodeId];
            if (!degree["$" + nodeId]) ready.push(nodeId);
        }
        ready.sort();
        var visited = 0;
        while (ready.length) {
            var current = ready.shift();
            visited += 1;
            var children = outgoing["$" + current];
            for (var c = 0; c < children.length; c++) {
                var child = children[c];
                ranks["$" + child] = Math.max(ranks["$" + child], ranks["$" + current] + 1);
                degree["$" + child] -= 1;
                if (degree["$" + child] === 0) {
                    ready.push(child);
                    ready.sort();
                }
            }
        }
        var maxRank = 0;
        for (var ranked in ranks) if (Object.prototype.hasOwnProperty.call(ranks, ranked)) maxRank = Math.max(maxRank, ranks[ranked]);
        var rows = {};
        for (var r = 0; r < graph.nodes.length; r++) {
            var graphNode = graph.nodes[r];
            var row = touched["$" + graphNode.id] ? ranks["$" + graphNode.id] : maxRank + 1;
            (rows["$" + row] || (rows["$" + row] = [])).push(graphNode);
        }
        var positions = {};
        for (var rowKey in rows) {
            if (!Object.prototype.hasOwnProperty.call(rows, rowKey)) continue;
            rows[rowKey].sort(function (left, right) {
                if (left.type !== right.type) return left.type < right.type ? -1 : 1;
                return left.id < right.id ? -1 : left.id > right.id ? 1 : 0;
            });
            var rank = Number(rowKey.slice(1));
            for (var column = 0; column < rows[rowKey].length; column++) {
                positions[rows[rowKey][column].id] = {
                    x: 235 + (column - (rows[rowKey].length - 1) / 2) * 150,
                    y: 22 + rank * 100
                };
            }
        }
        return positions;
    }

    function resolve(graph) {
        var saved = get(graph);
        var result = derive(graph);
        if (saved) {
            for (var id in saved) if (Object.prototype.hasOwnProperty.call(saved, id)) result[id] = saved[id];
        }
        return result;
    }

    return {
        recordKind: RECORD_KIND,
        recordVersion: RECORD_VERSION,
        coordinateLimit: MAX_COORDINATE,
        get: get,
        set: set,
        derive: derive,
        resolve: resolve
    };
}));
