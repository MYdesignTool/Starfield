"use strict";

var assert = require("assert");
var codec = require("../cep_panel/js/graph_codec.js");
var edits = require("../cep_panel/js/graph_edits.js");
var transactions = require("../cep_panel/js/graph_transactions.js");
var fixture = require("./native_snapshot_fixture.js");

function uuid(n) { return ("00000000000000000000000000000000" + n.toString(16)).slice(-32); }
function graph() {
    var result = {version:1,nodes:[],edges:[
        {id:uuid(11),sourceNode:uuid(1),sourcePort:"1",destinationNode:uuid(2),destinationPort:"1"},
        {id:uuid(12),sourceNode:uuid(2),sourcePort:"2",destinationNode:uuid(3),destinationPort:"1"}
    ],optionalRecords:[]};
    result.nodes = [fixture.node("emitter", uuid(1)), fixture.node("particle", uuid(2)), fixture.node("output", uuid(3))];
    [100, 120, 140].forEach(function (value, i) {
        result.nodes[0].parameters.find(function (p) { return p.key === String(19 + i); }).value = value;
    });
    return require("../cep_panel/js/graph_layout.js").set(result, {
        [uuid(1)]:{x:0,y:0},[uuid(2)]:{x:0,y:0},[uuid(3)]:{x:0,y:0}
    });
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
                callback(fixture.receipt(options.getSnapshot ? options.getSnapshot(snapshot) :
                         { ok: true, snapshot: snapshot, target: { token: "target-1" } }));
                return;
            }
            if (operation === "submitGraph") {
                if (options.submit) { callback(fixture.receipt(options.submit(extra, snapshot))); return; }
                snapshot = { initialized: true, revision: snapshot.revision + 1, graphHex: extra.graphHex };
                callback(fixture.receipt({ ok: true, snapshot: snapshot, target: { token: "target-1" } }));
                return;
            }
            if (operation === "ensureNodeEffects") {
                callback({ ok: true, operation: operation,
                           graphRevision: options.ensureGraphRevision === undefined ?
                               extra.baseGraphRevision : options.ensureGraphRevision,
                           target: { token: "target-1" },
                           missingNodeIds: options.missingNodeIds || [] });
                return;
            }
            callback({ ok: false, error: { code: "unexpected_operation", message: operation } });
        }
    });
    return { client: client, calls: calls, snapshot: function () { return fixture.receipt({ok:true,snapshot:snapshot}).snapshot; },
             replaceSnapshot: function (value) { snapshot = value; },
             setMissingNodeIds: function (value) { options.missingNodeIds = value; } };
}

var source = graph();
var preparedHarness = createHarness(source);
var prepared = fixture.receipt({ok:true,target:{token:"target-1"},snapshot:preparedHarness.snapshot()});
var preparedReply;
preparedHarness.client.apply({type:"addNode",nodeType:"force"},function(r){preparedReply=r;},"target-1",8,prepared);
assert.strictEqual(preparedReply.ok,true);
assert.deepStrictEqual(preparedHarness.calls.map(function(c){return c.operation;}),["submitGraph"],
    "an immediate planning receipt removes the redundant client snapshot call");
var wrongTargetHarness=createHarness(source);
wrongTargetHarness.client.apply({type:"addNode",nodeType:"force"},function(r){preparedReply=r;},"different",8,prepared);
assert.strictEqual(preparedReply.error.code,"stale_target");
assert.strictEqual(wrongTargetHarness.calls.length,0,"a wrong-target prepared receipt never reaches the host");
wrongTargetHarness.client.apply({type:"addNode",nodeType:"force"},function(r){preparedReply=r;},"target-1",9,prepared);
assert.strictEqual(preparedReply.error.code,"stale_graph");
assert.strictEqual(wrongTargetHarness.calls.length,0,"an obsolete planning revision never reaches the host");
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

var duplicateWireHarness = createHarness(source);
var duplicateWireReply;
duplicateWireHarness.client.apply({ type: "connect", from: uuid(2), to: uuid(3) }, function (response) {
    duplicateWireReply = response;
});
assert.strictEqual(duplicateWireReply.ok, true);
assert.strictEqual(duplicateWireReply.noOp, true, "an existing wire is acknowledged as a no-op");
assert.strictEqual(duplicateWireHarness.calls.length, 1,
                   "an identical connection must not call submitGraph or create an AE undo record");
assert.strictEqual(duplicateWireHarness.snapshot().revision, 8,
                   "an identical connection does not advance the project graph revision");
assert.strictEqual(duplicateWireHarness.snapshot().graphHex, codec.toHex(source));

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

// Ordinary native-record deletion/re-key tests live in panel_native_node_gateway_tests.
// A receipt must confirm the renderer compiled the records, not just acknowledge edits.
var wrongChecksum = createHarness(source, { submit: function (extra, base) {
    return { ok: true, snapshot: { initialized: true, revision: base.revision + 1,
        graphHex: extra.graphHex, checksumMatches: false } };
} });
wrongChecksum.client.apply({ type: "addNode", nodeType: "force" }, function (response) { reply = response; });
assert.strictEqual(reply.ok, true, "a decimal projection CRC is diagnostic, not an authoring gate");

var uninitialized = createHarness(source, { getSnapshot: function () {
    return { ok: true, snapshot: { initialized: false }, target: { token: "target-1" } };
} });
uninitialized.client.apply({ type: "addNode", nodeType: "force" }, function (response) { reply = response; });
assert.strictEqual(reply.error.code, "graph_snapshot_uninitialized");
assert.strictEqual(uninitialized.calls.length, 1, "an edit must not silently initialize persisted graph state");

var wrongReceipt = createHarness(source, { submit: function (extra, base) {
    return { ok: true, snapshot: { initialized: true, revision: base.revision, graphHex: extra.graphHex } };
} });
wrongReceipt.client.apply({ type: "addNode", nodeType: "force" }, function (response) { reply = response; });
assert.strictEqual(reply.error.code, "graph_snapshot_unconfirmed");

var oversized = createHarness(source, { maxBytes: 32 });
oversized.client.apply({ type: "addNode", nodeType: "force" }, function (response) { reply = response; });
assert.strictEqual(reply.error.code, "size_limit_exceeded");
assert.strictEqual(oversized.calls.length, 1, "oversized transaction must never reach the host commit call");

var malformedBootstrapReply;
var malformedBootstrapClient = transactions.create({
    codec: codec,
    edits: edits,
    call: function (operation, extra, callback) {
        callback({ ok: true, missingNodeIds: [],
            snapshot: { initialized: true, revision: 9, graphHex: "00" } });
    }
});
assert.doesNotThrow(function () {
    malformedBootstrapClient.ensureNativeEffects(
        { initialized: true, revision: 8, graphHex: codec.toHex(source) },
        "target-1", function (response) { malformedBootstrapReply = response; });
}, "a malformed native bootstrap acknowledgement must reach the callback as an error");
assert.strictEqual(malformedBootstrapReply.ok, false);
assert.ok(malformedBootstrapReply.error);

console.log("Panel graph transaction checks passed.");
