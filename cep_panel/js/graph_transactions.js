// CEP coordinator for one bounded graph edit. Host mutation stays behind the
// ExtendScript gateway and its supervised AE callback; this module never writes a
// CUSTOM_VALUE property or changes the render source directly.
(function (root, factory) {
    var layout = typeof module === "object" && module.exports
        ? require("./graph_layout.js") : root.StarfieldGraphLayout;
    var snapshots = typeof module === "object" && module.exports
        ? require("./native_graph_snapshot.js") : root.StarfieldNativeGraphSnapshot;
    var api = factory(layout, snapshots);
    if (typeof module === "object" && module.exports) module.exports = api;
    else root.StarfieldGraphTransactions = api;
}(typeof window !== "undefined" ? window : this, function (layout, snapshots) {
    "use strict";

    var DEFAULT_MAX_BYTES = 24 * 1024;

    function failure(code, message) {
        return { ok: false, error: { code: code, message: message } };
    }

    function validateSnapshot(snapshot, codec, maxBytes) {
        if (!snapshot || snapshot.initialized !== true || typeof snapshot.graphHex !== "string") {
            throw { code: "graph_snapshot_uninitialized", message: "Capture current controls into Node Graph mode before editing topology." };
        }
        if (typeof snapshot.revision !== "number" || !isFinite(snapshot.revision) ||
            Math.floor(snapshot.revision) !== snapshot.revision || snapshot.revision < 1) {
            throw { code: "invalid_graph_snapshot", message: "The host returned an invalid graph revision." };
        }
        if (snapshot.graphHex.length > maxBytes * 2) {
            throw { code: "size_limit_exceeded", message: "The compiled graph exceeds the AE project limit of " + maxBytes + " bytes." };
        }
        return codec.fromHex(snapshot.graphHex);
    }

    function jsonValue(value) {
        if (Object.prototype.toString.call(value) === "[object Array]" ||
            Object.prototype.toString.call(value) === "[object Uint8Array]") {
            var result = [];
            for (var i = 0; i < value.length; i++) result.push(jsonValue(value[i]));
            return result;
        }
        return value;
    }

    function nativeNodeManifest(graph) {
        var result = [];
        var positions = layout.resolve(graph);
        var byId = Object.create(null);
        for (var i = 0; i < graph.nodes.length; i++) {
            var node = graph.nodes[i];
            // Output is the graph-facing view of the owning Starfield Particle
            // renderer, not an independently materialized AE node effect.
            if (node.type === "org.starfieldfx.nodes.output") continue;
            var nativeNode = { id: node.id, type: node.type, schemaVersion: node.schemaVersion,
                position: positions[node.id], outgoing: [],
                parameters: node.parameters.map(function (parameter) {
                    return { key: String(parameter.key), type: parameter.type, value: jsonValue(parameter.value) };
                }) };
            byId["$" + node.id] = nativeNode;
            result.push(nativeNode);
        }
        for (var edgeIndex = 0; edgeIndex < graph.edges.length; edgeIndex++) {
            var edge = graph.edges[edgeIndex];
            var source = byId["$" + edge.sourceNode];
            if (source) source.outgoing.push({ id: edge.id, target: edge.destinationNode });
        }
        for (var nodeIndex = 0; nodeIndex < result.length; nodeIndex++) {
            result[nodeIndex].outgoing.sort(function (left, right) {
                return left.id < right.id ? -1 : left.id > right.id ? 1 : 0;
            });
        }
        return result;
    }

    function rendererManifest(graph) {
        var positions = layout.resolve(graph);
        var outputs = graph.nodes.filter(function (node) { return node.type === "org.starfieldfx.nodes.output"; });
        if (outputs.length !== 1) throw new Error("The graph must contain one Output owned by its renderer.");
        var node = outputs[0];
        return { id: node.id, position: positions[node.id], maxParticles: node.parameters[0].value };
    }

    // AE may round float controls and colors. Missing optional numeric parameters
    // compile to registry defaults. Validate the saved structure and values rather
    // than rejecting an otherwise identical graph solely for its byte encoding.
    function sameCompiledGraph(expected, actual) {
        if (expected.nodes.length !== actual.nodes.length || expected.edges.length !== actual.edges.length) return false;
        var defaults = { "9": 0, "10": 0 };
        function near(a, b, color) {
            if (typeof a === "number" && typeof b === "number") {
                return isFinite(a) && isFinite(b) && Math.abs(a - b) <=
                    (color ? 1 / 255 + 1e-7 : 1e-6 * Math.max(1, Math.abs(a)));
            }
            if (a && b && typeof a.length === "number" && a.length === b.length) {
                for (var k = 0; k < a.length; k++) if (!near(a[k], b[k], color)) return false;
                return true;
            }
            return a === b;
        }
        var actualNodes = {};
        actual.nodes.forEach(function (node) { actualNodes[node.id] = node; });
        var ep = layout.resolve(expected), ap = layout.resolve(actual);
        for (var n = 0; n < expected.nodes.length; n++) {
            var node = expected.nodes[n], saved = actualNodes[node.id];
            if (!saved || saved.type !== node.type || saved.schemaVersion !== node.schemaVersion ||
                !near(ep[node.id].x, ap[node.id].x) || !near(ep[node.id].y, ap[node.id].y)) return false;
            var params = {}, savedParams = {};
            node.parameters.forEach(function (p) { params[p.key] = p; });
            saved.parameters.forEach(function (p) { savedParams[p.key] = p; });
            var keys = Object.keys(params).concat(Object.keys(savedParams));
            for (var p = 0; p < keys.length; p++) {
                var key = keys[p], a = params[key], b = savedParams[key];
                if (!a || !b) {
                    var value = a || b;
                    var fallback = (node.type === "org.starfieldfx.nodes.particle" || node.type === "org.starfieldfx.nodes.appearance") ?
                        defaults[key] : node.type === "org.starfieldfx.nodes.emitter" ?
                        ({"12":0,"13":0,"14":0,"15":0,"16":0,"17":0,"18":60,"22":0,"23":0,"24":100,"25":0,"26":100,"27":0,"28":0,"29":0,"30":0})[key] : undefined;
                    if (fallback === undefined || value.type !== (key === "17" || key === "23" ? 3 : 4) || value.value !== fallback) return false;
                } else if (a.type === 7 && (key === "7" || key === "8")) {
                    var av = a.value, bv = b.value;
                    if (b.type !== 7 || av.length !== bv.length || av.length < 36 || av[0] !== bv[0] || av[1] !== bv[1]) return false;
                    var ad = new DataView(new Uint8Array(av).buffer), bd = new DataView(new Uint8Array(bv).buffer);
                    for (var offset = 4; offset < av.length; offset += 8) {
                        if (!near(ad.getFloat64(offset, true), bd.getFloat64(offset, true))) return false;
                    }
                } else if (a.type !== b.type || !near(a.value, b.value,
                    (node.type === "org.starfieldfx.nodes.particle" || node.type === "org.starfieldfx.nodes.appearance") && (key === "1" || key === "2"))) return false;
            }
        }
        var edges = {};
        actual.edges.forEach(function (edge) { edges[edge.id] = edge; });
        for (var e = 0; e < expected.edges.length; e++) {
            var edge = expected.edges[e], other = edges[edge.id];
            if (!other || edge.sourceNode !== other.sourceNode || edge.destinationNode !== other.destinationNode ||
                edge.sourcePort !== other.sourcePort || edge.destinationPort !== other.destinationPort) return false;
        }
        return true;
    }

    function create(options) {
        options = options || {};
        if (typeof options.call !== "function" || !options.codec || !options.edits) {
            throw new Error("graph transaction client needs call, codec, and edit planner dependencies");
        }
        var call = function (operation, fields, callback) {
            options.call(operation, fields, function (response) {
                callback(snapshots.normalize(response));
            });
        };
        var codec = options.codec;
        var edits = options.edits;
        var maxBytes = options.maxBytes || DEFAULT_MAX_BYTES;
        var idFactory = options.idFactory;
        function apply(edit, callback, targetToken, expectedRevision) {
            if (typeof callback !== "function") throw new Error("graph transaction callback is required");
            var pinnedTarget = targetToken ? { target: { token: targetToken } } : null;
            call("getGraphSnapshot", pinnedTarget, function (response) {
                if (!response || response.ok !== true) { callback(response || failure("bad_response", "No snapshot response.")); return; }
                var base = response.snapshot;
                if (typeof expectedRevision === "number" && base.revision !== expectedRevision) {
                    callback(failure("stale_graph", "The project graph changed after node-effect inspection; refresh before reconciling."));
                    return;
                }
                var graph;
                var graphHex;
                try {
                    graph = validateSnapshot(base, codec, maxBytes);
                    var authoredEdit = {};
                    Object.keys(edit).forEach(function (key) { authoredEdit[key] = edit[key]; });
                    authoredEdit.layerHeightPixels = base.geometry ? Number(base.geometry.height) : 1;
                    var updated = edits.apply(graph, authoredEdit, idFactory);
                    graphHex = codec.toHex(updated);
                    if (graphHex.length / 2 > maxBytes) {
                        callback(failure("size_limit_exceeded", "The edited graph exceeds the AE project limit of " + maxBytes + " bytes."));
                        return;
                    }
                } catch (error) {
                    callback(failure(error && error.code ? error.code : "invalid_graph",
                                     error && error.message ? error.message : String(error)));
                    return;
                }
                // Reconnecting an already-present wire is idempotent. Avoid an AE
                // graph revision and empty undo record for this one no-op gesture.
                // Other operations may carry native-effect reconciliation side effects.
                if (edit.type === "connect" && graphHex === String(base.graphHex).toLowerCase()) {
                    callback({ ok: true, operation: "submitGraph", target: response.target,
                               snapshot: base, graphHex: graphHex, noOp: true });
                    return;
                }
                var transaction = { baseGraphRevision: base.revision, baseRecordStamp:base.recordStamp, graphHex: graphHex,
                    baseNodeManifest: nativeNodeManifest(graph), nodeManifest: nativeNodeManifest(updated),
                    baseRendererManifest: rendererManifest(graph), rendererManifest: rendererManifest(updated) };
                if (targetToken) transaction.target = { token: targetToken };
                call("submitGraph", transaction, function (committed) {
                    if (!committed || committed.ok !== true) { callback(committed || failure("bad_response", "No graph commit response.")); return; }
                    var saved = committed.snapshot;
                    try {
                        if (!saved || saved.initialized !== true || saved.revision <= base.revision ||
                            !sameCompiledGraph(updated, validateSnapshot(saved, codec, maxBytes))) {
                            callback(failure("graph_snapshot_unconfirmed", "The host acknowledgement did not match the submitted graph."));
                            return;
                        }
                        validateSnapshot(saved, codec, maxBytes);
                    } catch (error) {
                        callback(failure(error && error.code ? error.code : "invalid_graph_snapshot",
                                         error && error.message ? error.message : String(error)));
                        return;
                    }
                    callback({ ok: true, operation: "submitGraph", target: committed.target,
                               snapshot: saved, graphHex: saved.graphHex });
                });
            });
        }

        function ensureNativeEffects(snapshot, targetToken, callback) {
            if (typeof callback !== "function") throw new Error("node-effect callback is required");
            var graph;
            try {
                var normalized = snapshots.normalize({ok:true,snapshot:snapshot});
                if (!normalized.ok) throw normalized.error;
                graph = validateSnapshot(snapshot, codec, maxBytes);
            } catch (error) {
                callback(failure(error && error.code ? error.code : "invalid_graph_snapshot",
                                 error && error.message ? error.message : String(error)));
                return;
            }
            call("ensureNodeEffects", { target: { token: targetToken },
                baseGraphRevision: snapshot.revision, graphHex: snapshot.graphHex,
                nodeManifest: nativeNodeManifest(graph), rendererManifest: rendererManifest(graph) }, function (ensured) {
                    try {
                        if (ensured && ensured.ok && ensured.snapshot) validateSnapshot(ensured.snapshot, codec, maxBytes);
                    } catch (error) {
                        callback(failure(error && error.code ? error.code : "invalid_graph_snapshot",
                            error && error.message ? error.message : String(error)));
                        return;
                    }
                    callback(ensured);
                });
        }

        return { apply: apply, ensureNativeEffects: ensureNativeEffects };
    }

    return { create: create, defaultMaxBytes: DEFAULT_MAX_BYTES };
}));
