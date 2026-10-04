// Pure schema-1 graph edit planner. It preserves canonical graph data and optional
// records; the native registry remains authoritative for semantic validation.
(function (root, factory) {
    var layout = typeof module === "object" && module.exports
        ? require("./graph_layout.js") : root.StarfieldGraphLayout;
    var api = factory(layout);
    if (typeof module === "object" && module.exports) module.exports = api;
    else root.StarfieldGraphEdits = api;
}(typeof window !== "undefined" ? window : this, function (layout) {
    "use strict";

    var TYPES = {
        emitter: "org.starfieldfx.nodes.emitter",
        particle: "org.starfieldfx.nodes.particle",
        force: "org.starfieldfx.nodes.force",
        output: "org.starfieldfx.nodes.output"
    };
    var SCHEMA_VERSIONS = { emitter: 6, particle: 5, force: 2, output: 4 };
    var PORTS = {
        "org.starfieldfx.nodes.emitter": { input: "2", output: "1" },
        "org.starfieldfx.nodes.particle": { input: "1", output: "2" },
        "org.starfieldfx.nodes.force": { input: "1", output: "2" },
        "org.starfieldfx.nodes.output": { input: "1" }
    };
    var DEFAULTS = {
        emitter: [
            { key: "2", type: 4, value: 100 },
            { key: "3", type: 3, value: 1000 },
            { key: "5", type: 3, value: 0 }, { key: "6", type: 5, value: [0, 0, 0] },
            { key: "7", type: 5, value: [0, 0, 0] }, { key: "8", type: 4, value: 10 },
            { key: "9", type: 4, value: 1 }, { key: "10", type: 4, value: 0.05 },
            { key: "11", type: 4, value: 0 }, { key: "14", type: 4, value: 0 },
            { key: "12", type: 4, value: 100 },
            { key: "15", type: 4, value: 0 }, { key: "16", type: 4, value: 0 },
            { key: "17", type: 3, value: 1 }, { key: "18", type: 4, value: 60 },
            { key: "19", type: 4, value: 100 }, { key: "20", type: 4, value: 100 },
            { key: "21", type: 4, value: 100 }, { key: "22", type: 4, value: 0 },
            {key:"23",type:3,value:0},{key:"31",type:3,value:0}, {key:"24",type:4,value:100},
            {key:"25",type:4,value:0}, {key:"26",type:4,value:100},
            {key:"27",type:4,value:0}, {key:"28",type:4,value:0},
            {key:"29",type:4,value:0}, {key:"30",type:4,value:0}
        ],
        particle: [
            { key: "12", type: 3, value: 0 },
            { key: "13", type: 7, value: defaultGradient() },
            { key: "1", type: 5, value: [1, 1, 1] },
            { key: "3", type: 4, value: 10 }, { key: "4", type: 4, value: 100 },
            { key: "5", type: 4, value: 1 }, { key: "6", type: 4, value: 100 },
            { key: "11", type: 4, value: 2 },
            { key: "9", type: 4, value: 0 }, { key: "10", type: 4, value: 0 },
            { key: "14", type: 4, value: 0 }, { key: "15", type: 3, value: 0 },
            { key: "16", type: 4, value: 10 }, { key: "17", type: 3, value: 0 },
            { key: "18", type: 5, value: [0,0,0] }, { key: "19", type: 4, value: 0 },
            { key: "20", type: 5, value: [0,0,0] }, { key: "21", type: 4, value: 0 },
            { key: "22", type: 3, value: 1 }, { key: "23", type: 4, value: 0 },
            { key: "24", type: 3, value: 2 }
        ],
        force: [
            { key: "1", type: 5, value: [0, 0, 0] }, { key: "2", type: 4, value: 0 },
            { key: "3", type: 4, value: 0 }, { key: "4", type: 5, value: [0,0,0] },
            { key: "5", type: 4, value: 0 }, { key: "6", type: 4, value: 0 },
            { key: "7", type: 4, value: 0 }, { key: "8", type: 4, value: 0 }
        ],
        output: [{ key: "1", type: 3, value: 1000000 }, {key:"2",type:3,value:0},
            {key:"3",type:4,value:0}, {key:"4",type:3,value:0}, {key:"5",type:4,value:100},{key:"6",type:3,value:0},{key:"7",type:3,value:30}]
    };

    function defaultGradient() {
        var bytes=new Uint8Array(68),view=new DataView(bytes.buffer);
        bytes[0]=1;bytes[1]=2;
        for(var i=0;i<2;i++) {
            view.setFloat64(4+32*i,i,true);
            for(var channel=0;channel<3;channel++) view.setFloat64(12+32*i+8*channel,1,true);
        }
        return bytes;
    }

    function fail(code, message) {
        var result = new Error(message);
        result.code = code;
        throw result;
    }

    function copyValue(value) {
        if (Object.prototype.toString.call(value) === "[object Array]") {
            return value.map(copyValue);
        }
        if (value instanceof Uint8Array) return new Uint8Array(value);
        return value;
    }

    function copyGraph(graph) {
        if (!graph || Object.prototype.toString.call(graph.nodes) !== "[object Array]" ||
            Object.prototype.toString.call(graph.edges) !== "[object Array]") {
            fail("invalid_graph", "graph must contain node and edge arrays");
        }
        return {
            version: graph.version,
            nodes: graph.nodes.map(function (node) {
                return { id: node.id, type: node.type, schemaVersion: node.schemaVersion,
                         parameters: node.parameters.map(function (parameter) {
                             return { key: parameter.key, type: parameter.type, value: copyValue(parameter.value) };
                         }) };
            }),
            edges: graph.edges.map(function (edge) {
                return { id: edge.id, sourceNode: edge.sourceNode, sourcePort: edge.sourcePort,
                         destinationNode: edge.destinationNode, destinationPort: edge.destinationPort };
            }),
            optionalRecords: (graph.optionalRecords || []).slice()
        };
    }

    function makeId(factory, graph) {
        var occupied = {};
        for (var n = 0; n < graph.nodes.length; n++) occupied["$" + graph.nodes[n].id] = true;
        for (var e = 0; e < graph.edges.length; e++) occupied["$" + graph.edges[e].id] = true;
        for (var attempt = 0; attempt < 32; attempt++) {
            var value = factory();
            if (typeof value !== "string" || !/^[0-9a-fA-F]{32}$/.test(value) || /^0{32}$/.test(value)) {
                fail("invalid_id_factory", "ID factory must return a non-zero 128-bit UUID in hexadecimal");
            }
            value = value.toLowerCase();
            if (!occupied["$" + value]) return value;
        }
        fail("id_collision", "unable to allocate a unique graph UUID");
    }

    function randomId() {
        var bytes = new Uint8Array(16);
        var cryptoObject = typeof window !== "undefined" ? window.crypto : null;
        if (cryptoObject && typeof cryptoObject.getRandomValues === "function") cryptoObject.getRandomValues(bytes);
        else {
            for (var i = 0; i < bytes.length; i++) bytes[i] = Math.floor(Math.random() * 256);
        }
        bytes[6] = (bytes[6] & 0x0f) | 0x40;
        bytes[8] = (bytes[8] & 0x3f) | 0x80;
        var result = "";
        for (var b = 0; b < bytes.length; b++) result += (bytes[b] < 16 ? "0" : "") + bytes[b].toString(16);
        return result;
    }

    function nodeById(graph, id) {
        for (var i = 0; i < graph.nodes.length; i++) if (graph.nodes[i].id === id) return graph.nodes[i];
        return null;
    }

    function makeNode(kind, id) {
        var auxiliary = kind === "auxiliary";
        if (auxiliary) kind = "emitter";
        if (!Object.prototype.hasOwnProperty.call(TYPES, kind)) fail("unknown_node_type", "unsupported built-in node type");
        var result = { id: id, type: TYPES[kind], schemaVersion: SCHEMA_VERSIONS[kind],
                 parameters: DEFAULTS[kind].map(function (parameter) {
                     return { key: parameter.key, type: parameter.type, value: copyValue(parameter.value) };
                 }) };
        if (auxiliary) result.parameters.filter(function (p) { return p.key === "31"; })[0].value = 1;
        return result;
    }

    function makeEdge(graph, factory, sourceNode, destinationNode, sourcePort, destinationPort) {
        return { id: makeId(factory, graph), sourceNode: sourceNode, sourcePort: sourcePort,
                 destinationNode: destinationNode, destinationPort: destinationPort };
    }

    function validateConnection(graph, sourceId, destinationId, sourcePort, destinationPort) {
        var source = nodeById(graph, sourceId);
        var destination = nodeById(graph, destinationId);
        if (!source || !destination) fail("missing_node", "connection endpoint does not exist");
        if (sourceId === destinationId) fail("cycle", "a node cannot connect to itself");
        var sourceSchema = PORTS[source.type];
        var destinationSchema = PORTS[destination.type];
        if (!sourceSchema || !destinationSchema || sourceSchema.output !== String(sourcePort) ||
            destinationSchema.input !== String(destinationPort)) {
            fail("invalid_port", "connection must use a compatible built-in output and input port");
        }
        if (source.type === TYPES.emitter && destination.type !== TYPES.particle) {
            fail("emitter_requires_particle", "an emitter must connect directly to a Particle node");
        }
        if (destination.type === TYPES.particle && source.type !== TYPES.emitter) {
            fail("particle_requires_emitter", "a Particle node input accepts an Emitter directly");
        }
        if (destination.type === TYPES.emitter) {
            var mode = destination.parameters.filter(function (p) { return p.key === "31"; })[0];
            if (!mode || Number(mode.value) !== 1) fail("auxiliary_required", "Create an Auxiliary source before connecting a parent stream.");
        }
        // Reject a cycle before producing a request. The native validator still checks
        // the complete graph, including node-specific stage rules and resource limits.
        var adjacency = {};
        for (var i = 0; i < graph.edges.length; i++) {
            var edge = graph.edges[i];
            (adjacency["$" + edge.sourceNode] || (adjacency["$" + edge.sourceNode] = [])).push(edge.destinationNode);
        }
        var pending = [destinationId];
        var visited = {};
        while (pending.length) {
            var current = pending.pop();
            if (current === sourceId) fail("cycle", "connection would create a cycle");
            if (visited["$" + current]) continue;
            visited["$" + current] = true;
            var next = adjacency["$" + current] || [];
            for (var j = 0; j < next.length; j++) pending.push(next[j]);
        }
    }

    function addNode(graph, edit, idFactory, positions) {
        var id = makeId(idFactory, graph);
        var node = makeNode(edit.nodeType, id);
        if (node.type === TYPES.emitter) {
            var height = Number(edit.layerHeightPixels) || 1;
            if (!(height > 0) || !isFinite(height)) fail("invalid_geometry", "Emitter layer height must be positive.");
            node.parameters.filter(function (p) { return p.key === "12"; })[0].value = 100 / height;
        }
        graph.nodes.push(node);
        var fallback = layout.derive(graph)[id];
        positions[id] = edit.position || fallback;
        return id;
    }

    function connect(graph, edit, idFactory) {
        var source = nodeById(graph, edit.from);
        var destination = nodeById(graph, edit.to);
        if (!source || !destination) fail("missing_node", "connection endpoint does not exist");
        var sourceSchema = PORTS[source.type];
        var destinationSchema = PORTS[destination.type];
        if (!sourceSchema || !destinationSchema) fail("unknown_node_type", "connection uses a node type not known to this editor");
        var sourcePort = edit.outputPort === undefined ? sourceSchema.output : String(edit.outputPort);
        var destinationPort = edit.inputPort === undefined ? destinationSchema.input : String(edit.inputPort);
        validateConnection(graph, edit.from, edit.to, sourcePort, destinationPort);
        for (var i = 0; i < graph.edges.length; i++) {
            var existing = graph.edges[i];
            if (existing.sourceNode === edit.from && existing.sourcePort === sourcePort &&
                existing.destinationNode === edit.to && existing.destinationPort === destinationPort) {
                return true;
            }
        }
        graph.edges.push(makeEdge(graph, idFactory, edit.from, edit.to, sourcePort, destinationPort));
        return false;
    }

    function disconnect(graph, edit) {
        var found = -1;
        for (var i = 0; i < graph.edges.length; i++) {
            var edge = graph.edges[i];
            var match = edit.edgeId ? edge.id === edit.edgeId : edge.sourceNode === edit.from && edge.destinationNode === edit.to;
            if (!match) continue;
            if (found >= 0) fail("ambiguous_edge", "more than one edge matches; specify the stable edge ID");
            found = i;
        }
        if (found < 0) fail("missing_edge", "edge does not exist");
        graph.edges.splice(found, 1);
    }

    function insertNode(graph, edit, idFactory, positions) {
        var edgeIndex = -1;
        for (var i = 0; i < graph.edges.length; i++) {
            var candidate = graph.edges[i];
            if (edit.edgeId ? candidate.id === edit.edgeId : candidate.sourceNode === edit.from && candidate.destinationNode === edit.to) {
                if (edgeIndex >= 0) fail("ambiguous_edge", "specify the stable edge ID to insert into a parallel connection");
                edgeIndex = i;
            }
        }
        if (edgeIndex < 0) fail("missing_edge", "edge to splice does not exist");
        var previous = graph.edges[edgeIndex];
        if (!nodeById(graph, previous.sourceNode) || !nodeById(graph, previous.destinationNode)) {
            fail("missing_node", "edge to splice refers to a node absent from the graph");
        }
        var position = edit.position || {
            x: (positions[previous.sourceNode].x + positions[previous.destinationNode].x) / 2,
            y: (positions[previous.sourceNode].y + positions[previous.destinationNode].y) / 2
        };
        var nodeId = edit.nodeId || null;
        if (nodeId && (!nodeById(graph, nodeId) || nodeId === previous.sourceNode || nodeId === previous.destinationNode)) {
            fail("missing_node", "the node to insert must exist and differ from both edge endpoints");
        }
        graph.edges.splice(edgeIndex, 1);
        if (!nodeId) nodeId = addNode(graph, { nodeType: edit.nodeType, position: position,
            layerHeightPixels: edit.layerHeightPixels }, idFactory, positions);
        else positions[nodeId] = position;
        if (edit.layout !== undefined) moveNodes(graph, { positions: edit.layout }, positions);
        connect(graph, { from: previous.sourceNode, to: nodeId,
                         outputPort: previous.sourcePort }, idFactory);
        connect(graph, { from: nodeId, to: previous.destinationNode,
                         inputPort: previous.destinationPort }, idFactory);
        return nodeId;
    }

    function createInsertEdit(nodeId, edge, position, layoutPositions) {
        if (typeof nodeId !== "string" || !nodeId || !edge || Object.prototype.toString.call(edge) !== "[object Array]" ||
            edge.length !== 2 || !position || !isFinite(Number(position.x)) || !isFinite(Number(position.y))) {
            fail("invalid_edit", "insert edit requires a node, an edge, and a finite drop position");
        }
        var edit = { type: "insertNode", nodeId: nodeId, edgeId: edge.id || undefined,
                     from: edge[0], to: edge[1],
                     position: { x: Number(position.x), y: Number(position.y) } };
        if (layoutPositions !== undefined) {
            if (!layoutPositions || Object.prototype.toString.call(layoutPositions) !== "[object Object]") {
                fail("invalid_edit", "insert edit layout must be a node ID position map");
            }
            if (Object.keys(layoutPositions).length) edit.layout = layoutPositions;
        }
        return edit;
    }

    function deleteNodes(graph, edit, positions) {
        if (Object.prototype.toString.call(edit.nodeIds) !== "[object Array]" || !edit.nodeIds.length) {
            fail("invalid_edit", "deleteNodes requires at least one node ID");
        }
        var ids = {};
        for (var i = 0; i < edit.nodeIds.length; i++) {
            if (!nodeById(graph, edit.nodeIds[i])) fail("missing_node", "cannot delete a node absent from the graph");
            ids["$" + edit.nodeIds[i]] = true;
        }
        for (var n = 0; n < graph.nodes.length; n++) {
            if (ids["$" + graph.nodes[n].id] && graph.nodes[n].type === TYPES.output) {
                fail("required_output", "the required Output node cannot be deleted");
            }
        }
        graph.nodes = graph.nodes.filter(function (node) { return !ids["$" + node.id]; });
        graph.edges = graph.edges.filter(function (edge) {
            return !ids["$" + edge.sourceNode] && !ids["$" + edge.destinationNode];
        });
        for (var id in ids) {
            if (Object.prototype.hasOwnProperty.call(ids, id)) delete positions[id.slice(1)];
        }
    }

    function duplicateNodes(graph, edit, idFactory, positions) {
        if (Object.prototype.toString.call(edit.nodeIds) !== "[object Array]" || !edit.nodeIds.length) {
            fail("invalid_edit", "duplicateNodes requires at least one node ID");
        }
        var selected = {};
        var ids = {};
        for (var i = 0; i < edit.nodeIds.length; i++) {
            if (!nodeById(graph, edit.nodeIds[i])) fail("missing_node", "cannot duplicate a node absent from the graph");
            selected["$" + edit.nodeIds[i]] = true;
        }
        var originals = graph.nodes.filter(function (node) { return selected["$" + node.id]; });
        for (var n = 0; n < originals.length; n++) {
            if (originals[n].type === TYPES.output) fail("duplicate_output", "Output is unique and cannot be duplicated");
            var copy = { id: makeId(idFactory, graph), type: originals[n].type,
                         schemaVersion: originals[n].schemaVersion,
                         parameters: originals[n].parameters.map(function (parameter) {
                             return { key: parameter.key, type: parameter.type, value: copyValue(parameter.value) };
                         }) };
            graph.nodes.push(copy);
            ids["$" + originals[n].id] = copy.id;
            var originalPosition = positions[originals[n].id];
            var offset = edit.offset || { x: 28, y: 28 };
            positions[copy.id] = { x: originalPosition.x + offset.x, y: originalPosition.y + offset.y };
        }
        var originalEdges = graph.edges.slice();
        for (var e = 0; e < originalEdges.length; e++) {
            var edge = originalEdges[e];
            var sourceSelected = selected["$" + edge.sourceNode] === true;
            var destinationSelected = selected["$" + edge.destinationNode] === true;
            if (!sourceSelected && !destinationSelected) continue;
            var source = nodeById(graph, edge.sourceNode);
            var destination = nodeById(graph, edge.destinationNode);

            if (!source || !destination) continue;
            var copiedSource = sourceSelected ? ids["$" + edge.sourceNode] : edge.sourceNode;
            var copiedDestination = destinationSelected ? ids["$" + edge.destinationNode] : edge.destinationNode;
            graph.edges.push(makeEdge(graph, idFactory, copiedSource, copiedDestination,
                                      edge.sourcePort, edge.destinationPort));
        }
        return ids;
    }

    function moveNodes(graph, edit, positions) {
        if (!edit.positions || Object.prototype.toString.call(edit.positions) !== "[object Object]") {
            fail("invalid_edit", "moveNodes requires a node ID to position map");
        }
        if (!Object.keys(edit.positions).length) fail("invalid_edit", "moveNodes requires at least one position");
        for (var id in edit.positions) {
            if (!Object.prototype.hasOwnProperty.call(edit.positions, id)) continue;
            if (!nodeById(graph, id)) fail("missing_node", "cannot move a node that is absent from the graph");
            positions[id] = edit.positions[id];
        }
    }

    function setParameters(graph, edit) {
        if (Object.prototype.toString.call(edit.changes) !== "[object Array]" ||
            !edit.changes.length || edit.changes.length > 512) {
            fail("invalid_edit", "setParameters requires between 1 and 512 parameter changes");
        }
        for (var i = 0; i < edit.changes.length; i++) {
            var change = edit.changes[i];
            if (!change || typeof change.nodeId !== "string" || typeof change.parameterKey !== "string" ||
                !/^\d+$/.test(change.parameterKey)) {
                fail("invalid_edit", "parameter changes need a node ID and decimal graph parameter key");
            }
            var node = nodeById(graph, change.nodeId);
            if (!node) fail("missing_node", "cannot edit a node absent from the graph");
            var found = -1;
            for (var p = 0; p < node.parameters.length; p++) {
                if (node.parameters[p].key === change.parameterKey) { found = p; break; }
            }
            if (change.remove === true) {
                if (found < 0) continue;
                if ((node.type !== TYPES.particle) ||
                    (change.parameterKey !== "7" && change.parameterKey !== "8")) {
                    fail("invalid_parameter", "only optional over-life curve parameters can be removed");
                }
                node.parameters.splice(found, 1);
                continue;
            }
            var value = copyValue(change.value);
            if (found < 0) {
                if ((node.type !== TYPES.particle) ||
                    (change.parameterKey !== "7" && change.parameterKey !== "8" && !(node.type===TYPES.particle && change.parameterKey==="13")) ||
                    change.valueType !== 7 || !(value instanceof Uint8Array)) {
                    fail("missing_parameter", "the requested graph parameter is not present");
                }
                node.parameters.push({ key: change.parameterKey, type: 7, value: value });
                continue;
            }
            if (change.valueType !== node.parameters[found].type) {
                fail("parameter_type_mismatch", "graph parameter edits must preserve the value type");
            }
            node.parameters[found].value = value;
        }
        // Changing the source back to Default removes its parent stream in the
        // same authored transaction, including the saved source-side records.
        graph.edges = graph.edges.filter(function (edge) {
            var target = nodeById(graph, edge.destinationNode);
            if (!target || target.type !== TYPES.emitter) return true;
            var mode = target.parameters.filter(function (p) { return p.key === "31"; })[0];
            return mode && Number(mode.value) === 1;
        });
    }

    function apply(inputGraph, edit, idFactory) {
        if (!edit || typeof edit.type !== "string") fail("invalid_edit", "edit type is required");
        var graph = copyGraph(inputGraph);
        var positions = layout.resolve(graph);
        idFactory = idFactory || randomId;
        if (edit.type === "addNode") addNode(graph, edit, idFactory, positions);
        else if (edit.type === "connect") {
            if (connect(graph, edit, idFactory)) return graph;
        }
        else if (edit.type === "disconnect") disconnect(graph, edit);
        else if (edit.type === "insertNode") insertNode(graph, edit, idFactory, positions);
        else if (edit.type === "deleteNodes") deleteNodes(graph, edit, positions);
        else if (edit.type === "duplicateNodes") duplicateNodes(graph, edit, idFactory, positions);
        else if (edit.type === "moveNodes") moveNodes(graph, edit, positions);
        else if (edit.type === "setParameters") setParameters(graph, edit);
        else fail("unsupported_edit", "edit operation is not supported");
        return layout.set(graph, positions);
    }

    function canConnect(graph, sourceId, destinationId, sourcePort, destinationPort) {
        if (!graph) return false;
        try {
            validateConnection(graph, sourceId, destinationId, sourcePort, destinationPort);
            return true;
        } catch (invalidConnection) { return false; }
    }

    return { types: TYPES, ports: PORTS, apply: apply, createInsertEdit: createInsertEdit,
             randomId: randomId, canConnect: canConnect };
}));
