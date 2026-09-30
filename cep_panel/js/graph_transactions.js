// CEP coordinator for one bounded graph edit. Host mutation stays behind the
// ExtendScript gateway and its supervised AE callback; this module never writes a
// CUSTOM_VALUE property or changes the render source directly.
(function (root, factory) {
    var api = factory();
    if (typeof module === "object" && module.exports) module.exports = api;
    else root.StarfieldGraphTransactions = api;
}(typeof window !== "undefined" ? window : this, function () {
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
            throw { code: "size_limit_exceeded", message: "The graph exceeds the AE expression-carrier limit of " + maxBytes + " bytes." };
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
        for (var i = 0; i < graph.nodes.length; i++) {
            var node = graph.nodes[i];
            // Output is the graph-facing view of the owning Starfield Particle
            // renderer, not an independently materialized AE node effect.
            if (node.type === "org.starfieldfx.nodes.output") continue;
            result.push({ id: node.id, type: node.type, schemaVersion: node.schemaVersion,
                parameters: node.parameters.map(function (parameter) {
                    return { key: String(parameter.key), type: parameter.type, value: jsonValue(parameter.value) };
                }) });
        }
        return result;
    }

    function create(options) {
        options = options || {};
        if (typeof options.call !== "function" || !options.codec || !options.edits) {
            throw new Error("graph transaction client needs call, codec, and edit planner dependencies");
        }
        var call = options.call;
        var codec = options.codec;
        var edits = options.edits;
        var maxBytes = options.maxBytes || DEFAULT_MAX_BYTES;
        var idFactory = options.idFactory;

        function apply(edit, callback, targetToken) {
            if (typeof callback !== "function") throw new Error("graph transaction callback is required");
            var pinnedTarget = targetToken ? { target: { token: targetToken } } : null;
            call("getGraphSnapshot", pinnedTarget, function (response) {
                if (!response || response.ok !== true) { callback(response || failure("bad_response", "No snapshot response.")); return; }
                var base = response.snapshot;
                var graph;
                var graphHex;
                try {
                    graph = validateSnapshot(base, codec, maxBytes);
                    var updated = edits.apply(graph, edit, idFactory);
                    graphHex = codec.toHex(updated);
                    if (graphHex.length / 2 > maxBytes) {
                        callback(failure("size_limit_exceeded", "The edited graph exceeds the AE expression-carrier limit of " + maxBytes + " bytes."));
                        return;
                    }
                } catch (error) {
                    callback(failure(error && error.code ? error.code : "invalid_graph",
                                     error && error.message ? error.message : String(error)));
                    return;
                }
                var transaction = { baseGraphRevision: base.revision, graphHex: graphHex,
                    baseNodeManifest: nativeNodeManifest(graph), nodeManifest: nativeNodeManifest(updated) };
                if (targetToken) transaction.target = { token: targetToken };
                call("submitGraph", transaction, function (committed) {
                    if (!committed || committed.ok !== true) { callback(committed || failure("bad_response", "No graph commit response.")); return; }
                    var saved = committed.snapshot;
                    try {
                        if (!saved || saved.initialized !== true || saved.revision !== base.revision + 1 ||
                            saved.graphHex !== graphHex) {
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
                               snapshot: saved, graphHex: graphHex });
                });
            });
        }

        function ensureNativeEffects(snapshot, targetToken, callback) {
            if (typeof callback !== "function") throw new Error("node-effect callback is required");
            var graph;
            try {
                graph = validateSnapshot(snapshot, codec, maxBytes);
            } catch (error) {
                callback(failure(error && error.code ? error.code : "invalid_graph_snapshot",
                                 error && error.message ? error.message : String(error)));
                return;
            }
            call("ensureNodeEffects", { target: { token: targetToken },
                baseGraphRevision: snapshot.revision, graphHex: snapshot.graphHex,
                nodeManifest: nativeNodeManifest(graph) }, function (ensured) {
                    if (!ensured || ensured.ok !== true ||
                        Object.prototype.toString.call(ensured.missingNodeIds) !== "[object Array]" ||
                        ensured.missingNodeIds.length === 0) {
                        callback(ensured);
                        return;
                    }
                    apply({ type: "deleteNodes", nodeIds: ensured.missingNodeIds }, function (reconciled) {
                        if (!reconciled || reconciled.ok !== true) { callback(reconciled); return; }
                        callback({ ok: true, operation: "reconcileNativeNodeDeletion",
                            target: reconciled.target, snapshot: reconciled.snapshot,
                            removedNodeIds: ensured.missingNodeIds.slice() });
                    }, targetToken);
                });
        }

        return { apply: apply, ensureNativeEffects: ensureNativeEffects };
    }

    return { create: create, defaultMaxBytes: DEFAULT_MAX_BYTES };
}));
