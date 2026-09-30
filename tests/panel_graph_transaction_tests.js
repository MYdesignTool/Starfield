"use strict";

var assert = require("assert");
var codec = require("../cep_panel/js/graph_codec.js");
var edits = require("../cep_panel/js/graph_edits.js");
var transactions = require("../cep_panel/js/graph_transactions.js");

function uuid(n) { return ("00000000000000000000000000000000" + n.toString(16)).slice(-32); }
function graph() {
    return {
        version: 1,
        nodes: [
            { id: uuid(1), type: edits.types.emitter, schemaVersion: 3, parameters: [
                { key: "2", type: 4, value: 30 },
                { key: "3", type: 3, value: 1 },
                { key: "5", type: 3, value: 1 }, { key: "6", type: 5, value: [0, 0, 0] },
                { key: "7", type: 5, value: [0, 0.3, 0] }, { key: "8", type: 4, value: 10 },
                { key: "9", type: 4, value: 1 }, { key: "10", type: 4, value: 0.05 },
                { key: "11", type: 4, value: 0.15 }, { key: "19", type: 4, value: 100 },
                { key: "20", type: 4, value: 120 }, { key: "21", type: 4, value: 140 }
            ] },
            { id: uuid(2), type: edits.types.particle, schemaVersion: 2, parameters: [
                { key: "1", type: 5, value: [1, 1, 1] }, { key: "2", type: 5, value: [1, 1, 1] },
                { key: "3", type: 4, value: 10 }, { key: "4", type: 4, value: 10 },
                { key: "5", type: 4, value: 1 }, { key: "6", type: 4, value: 1 },
                { key: "11", type: 4, value: 2 }
            ] },
            { id: uuid(3), type: edits.types.output, schemaVersion: 2, parameters: [
                { key: "1", type: 3, value: 1000 }
            ] }
        ],
        edges: [
            { id: uuid(11), sourceNode: uuid(1), sourcePort: "1", destinationNode: uuid(2), destinationPort: "1" },
            { id: uuid(12), sourceNode: uuid(2), sourcePort: "2", destinationNode: uuid(3), destinationPort: "1" }
        ], optionalRecords: []
    };
}

function createHarness(initialGraph, options) {
    options = options || {};
    var snapshot = { initialized: true, revision: 8, graphHex: codec.toHex(initialGraph) };
    var calls = [];
    var client = transactions.create({
        codec: codec,
        edits: edits,
        maxBytes: options.maxBytes,
        idFactory: function () { return uuid(200 + calls.length); },
        call: function (operation, extra, callback) {
            calls.push({ operation: operation, extra: extra });
            if (operation === "getGraphSnapshot") {
                callback(options.getSnapshot ? options.getSnapshot(snapshot) :
                         { ok: true, snapshot: snapshot, target: { token: "target-1" } });
                return;
            }
            if (operation === "submitGraph") {
                if (options.submit) { callback(options.submit(extra, snapshot)); return; }
                snapshot = { initialized: true, revision: snapshot.revision + 1, graphHex: extra.graphHex };
                callback({ ok: true, snapshot: snapshot, target: { token: "target-1" } });
                return;
            }
            if (operation === "ensureNodeEffects") {
                callback({ ok: true, operation: operation, graphRevision: extra.baseGraphRevision,
                           target: { token: "target-1" },
                           missingNodeIds: options.missingNodeIds || [] });
                return;
            }
            callback({ ok: false, error: { code: "unexpected_operation", message: operation } });
        }
    });
    return { client: client, calls: calls, snapshot: function () { return snapshot; } };
}

var source = graph();
var harness = createHarness(source);
var reply;
harness.client.apply({ type: "addNode", nodeType: "force" }, function (response) { reply = response; });
assert.strictEqual(reply.ok, true);
assert.strictEqual(harness.calls.length, 2);
assert.strictEqual(harness.calls[0].operation, "getGraphSnapshot");
assert.strictEqual(harness.calls[1].operation, "submitGraph");
assert.strictEqual(harness.calls[1].extra.baseGraphRevision, 8);
assert.deepStrictEqual(harness.calls[1].extra.baseNodeManifest.map(function (node) { return node.type; }),
                       [edits.types.emitter, edits.types.particle],
                       "the main renderer's logical Output terminal must not become a separate AE effect");
assert.deepStrictEqual(harness.calls[1].extra.nodeManifest.map(function (node) { return node.type; }),
                       [edits.types.emitter, edits.types.particle, edits.types.force]);
assert.deepStrictEqual(harness.calls[1].extra.baseNodeManifest[0].parameters.filter(function (parameter) {
    return parameter.key === "19" || parameter.key === "20" || parameter.key === "21";
}).map(function (parameter) { return [parameter.key, parameter.value]; }),
[["19", 100], ["20", 120], ["21", 140]],
"direct-pixel emitter dimensions must be included in the per-node AE manifest");
assert.strictEqual(reply.snapshot.revision, 9);
assert.strictEqual(codec.fromHex(reply.graphHex).nodes.length, 4);

var ensureHarness = createHarness(source);
var ensureReply;
ensureHarness.client.ensureNativeEffects(ensureHarness.snapshot(), "target-1", function (response) {
    ensureReply = response;
});
assert.strictEqual(ensureReply.ok, true);
assert.strictEqual(ensureHarness.calls.length, 1);
assert.strictEqual(ensureHarness.calls[0].operation, "ensureNodeEffects");
assert.strictEqual(ensureHarness.calls[0].extra.nodeManifest.length, 2,
                   "startup reconciliation creates editable native nodes only; Output is the main effect");

var deletedNodeId = uuid(2);
var externalDeleteHarness = createHarness(source, { missingNodeIds: [deletedNodeId] });
var externalDeleteReply;
externalDeleteHarness.client.ensureNativeEffects(externalDeleteHarness.snapshot(), "target-1", function (response) {
    externalDeleteReply = response;
});
assert.strictEqual(externalDeleteReply.ok, true);
assert.strictEqual(externalDeleteReply.operation, "reconcileNativeNodeDeletion");
assert.deepStrictEqual(externalDeleteReply.removedNodeIds, [deletedNodeId]);
assert.strictEqual(externalDeleteReply.snapshot.revision, 9,
                   "external Effect Parade deletion commits one new saved graph revision");
var externallyPrunedGraph = codec.fromHex(externalDeleteReply.snapshot.graphHex);
assert.deepStrictEqual(externallyPrunedGraph.nodes.map(function (node) { return node.id; }), [uuid(1), uuid(3)]);
assert.strictEqual(externallyPrunedGraph.edges.length, 0,
                   "external node deletion also removes its incident graph edges");
assert.strictEqual(externalDeleteHarness.calls[1].operation, "getGraphSnapshot");
assert.strictEqual(externalDeleteHarness.calls[2].operation, "submitGraph");
assert.deepStrictEqual(externalDeleteHarness.calls[2].extra.target, { token: "target-1" },
                       "the deletion reconciliation stays pinned to the inspected AE effect");

var uninitialized = createHarness(source, { getSnapshot: function () {
    return { ok: true, snapshot: { initialized: false }, target: { token: "target-1" } };
} });
uninitialized.client.apply({ type: "addNode", nodeType: "force" }, function (response) { reply = response; });
assert.strictEqual(reply.error.code, "graph_snapshot_uninitialized");
assert.strictEqual(uninitialized.calls.length, 1, "an edit must not silently initialize persisted graph state");

var wrongReceipt = createHarness(source, { submit: function (extra, base) {
    return { ok: true, snapshot: { initialized: true, revision: base.revision + 2, graphHex: extra.graphHex } };
} });
wrongReceipt.client.apply({ type: "addNode", nodeType: "force" }, function (response) { reply = response; });
assert.strictEqual(reply.error.code, "graph_snapshot_unconfirmed");

var oversized = createHarness(source, { maxBytes: 32 });
oversized.client.apply({ type: "addNode", nodeType: "force" }, function (response) { reply = response; });
assert.strictEqual(reply.error.code, "size_limit_exceeded");
assert.strictEqual(oversized.calls.length, 1, "oversized transaction must never reach the host commit call");

console.log("Panel graph transaction checks passed.");
