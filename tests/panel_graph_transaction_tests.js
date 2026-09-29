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
            { id: uuid(1), type: edits.types.emitter, schemaVersion: 1, parameters: [
                { key: "1", type: 3, value: 1000 }, { key: "2", type: 4, value: 30 },
                { key: "3", type: 3, value: 1 }, { key: "4", type: 4, value: 2 },
                { key: "5", type: 3, value: 1 }, { key: "6", type: 5, value: [0, 0, 0] },
                { key: "7", type: 5, value: [0, 0.3, 0] }, { key: "8", type: 4, value: 10 },
                { key: "9", type: 4, value: 1 }, { key: "10", type: 4, value: 0.05 },
                { key: "11", type: 4, value: 0.15 }
            ] },
            { id: uuid(2), type: edits.types.particle, schemaVersion: 1, parameters: [
                { key: "1", type: 5, value: [1, 1, 1] }, { key: "2", type: 5, value: [1, 1, 1] },
                { key: "3", type: 4, value: 10 }, { key: "4", type: 4, value: 10 },
                { key: "5", type: 4, value: 1 }, { key: "6", type: 4, value: 1 }
            ] },
            { id: uuid(3), type: edits.types.output, schemaVersion: 1, parameters: [] }
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
assert.strictEqual(reply.snapshot.revision, 9);
assert.strictEqual(codec.fromHex(reply.graphHex).nodes.length, 4);

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
