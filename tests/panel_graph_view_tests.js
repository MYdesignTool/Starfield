"use strict";

var assert = require("assert");
var edits = require("../cep_panel/js/graph_edits.js");
var view = require("../cep_panel/js/graph_view.js");
var codec = require("../cep_panel/js/graph_codec.js");

function uuid(n) { return ("00000000000000000000000000000000" + n.toString(16)).slice(-32); }
function graph() {
    var points = [{ age: 0, value: 10 }, { age: 0.4, value: 24 }, { age: 1, value: 2 }];
    return {
        version: 1,
        nodes: [
            { id: uuid(1), type: edits.types.emitter, schemaVersion: 1, parameters: [
                { key: "1", type: 3, value: 6400 }, { key: "2", type: 4, value: 60 },
                { key: "5", type: 3, value: 2 }, { key: "6", type: 5, value: [1920, 1080, 1080] }
            ] },
            { id: uuid(2), type: edits.types.particle, schemaVersion: 1, parameters: [
                { key: "1", type: 5, value: [1, 0.5, 0] }, { key: "2", type: 5, value: [0, 0.25, 1] },
                { key: "3", type: 4, value: 10 }, { key: "4", type: 4, value: 2 },
                { key: "5", type: 4, value: 1 }, { key: "6", type: 4, value: 0 }
            ] },
            { id: uuid(3), type: edits.types.output, schemaVersion: 1, parameters: [] }
        ],
        edges: [
            { id: uuid(11), sourceNode: uuid(1), sourcePort: "1", destinationNode: uuid(2), destinationPort: "1" },
            { id: uuid(12), sourceNode: uuid(2), sourcePort: "2", destinationNode: uuid(3), destinationPort: "1" }
        ], optionalRecords: []
    };
}

var source = graph();
var customCurve = view.encodeCurve([{ age: 0, value: 10 }, { age: 0.4, value: 24 }, { age: 1, value: 2 }]);
source.nodes[1].parameters.push({ key: "7", type: 7, value: customCurve });
source = codec.fromHex(codec.toHex(source));
var projected = view.project(source);
var particle = projected.nodes.filter(function (node) { return node.kind === "particle"; })[0];
var output = projected.nodes.filter(function (node) { return node.kind === "output"; })[0];
assert.strictEqual(particle.label, "Particle");
assert.strictEqual(particle.curves.size.custom, true);
assert.deepStrictEqual(particle.curves.size.points, [
    { age: 0, value: 10 }, { age: 0.4, value: 24 }, { age: 1, value: 2 }
]);
assert.strictEqual(output.maxParticles, 6400);
assert.strictEqual(output.params[0].graphNodeId, source.nodes[0].id,
                   "Output Max Particles control edits the emitter-owned value");

var emitter = projected.nodes.filter(function (node) { return node.kind === "emitter"; })[0];
var type = emitter.params.filter(function (parameter) { return parameter.graphKey === "5"; })[0];
assert.strictEqual(type.value, 3);
assert.strictEqual(view.parameterToGraphValue(type, 3), 2, "popup labels map to zero-based graph enums");
var color = particle.params.filter(function (parameter) { return parameter.graphKey === "1"; })[0];
assert.deepStrictEqual(view.parameterToGraphValue(color, [255, 127.5, 0]), [1, 0.5, 0]);

var wrongCurveType = graph();
wrongCurveType.nodes[1].parameters.push({ key: "7", type: 4, value: 0 });
assert.throws(function () { view.project(wrongCurveType); }, /opaque value type/i);

var oldAliasEdit = view.mapLegacyEdit(graph(), { type: "deleteNodes", nodeIds: ["particle"] });
assert.strictEqual(oldAliasEdit.nodeIds[0], uuid(2), "legacy canvas node IDs map to stable graph UUIDs");
console.log("Panel graph view checks passed.");
