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
                { key: "4", type: 4, value: 2 },
                { key: "5", type: 3, value: 2 }, { key: "6", type: 5, value: [1920, 1080, 1080] },
                { key: "10", type: 4, value: 0.05 },
                { key: "19", type: 4, value: 1920 }, { key: "20", type: 4, value: 1080 },
                { key: "21", type: 4, value: 720 }
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
var edgeEditSnapshot = { revision: 9, graphHex: "updated-edge-graph" };
assert.strictEqual(view.graphSnapshotChanged({ revision: 8, graphHex: "original-graph" }, edgeEditSnapshot), true,
                   "an edge-only commit changes the graph snapshot even when every node position stays fixed");
assert.strictEqual(view.graphSnapshotChanged(edgeEditSnapshot,
    { revision: 9, graphHex: "updated-edge-graph" }), false,
"an unchanged snapshot should not trigger an unnecessary graph redraw");
assert.strictEqual(view.samePositions(projected.positions, projected.positions), true,
                   "matching canonical layouts should not force a redraw");
var draggedPositions = JSON.parse(JSON.stringify(projected.positions));
draggedPositions[uuid(2)].x += 80;
assert.strictEqual(view.samePositions(draggedPositions, projected.positions), false,
                   "a rejected drag must detect the transient layout and redraw from the saved graph");
var particle = projected.nodes.filter(function (node) { return node.kind === "particle"; })[0];
var output = projected.nodes.filter(function (node) { return node.kind === "output"; })[0];
assert.strictEqual(particle.label, "Particle");
assert.strictEqual(particle.params[0].graphKey, "11",
                   "Particle lifetime appears before appearance controls in the inspector");
assert.strictEqual(particle.params[0].value, 2,
                   "an older Particle graph projects the legacy emitter lifetime onto Particle");
assert.strictEqual(particle.params.filter(function (parameter) { return parameter.graphKey === "3"; })[0].unit, "px");
assert.strictEqual(particle.params.filter(function (parameter) { return parameter.graphKey === "4"; })[0].unit, "px");
assert.deepStrictEqual(particle.params.filter(function (parameter) {
    return parameter.graphKey === "9" || parameter.graphKey === "10";
}).map(function (parameter) { return [parameter.label, parameter.value, parameter.max]; }), [
    ["Size Random", 0, 100], ["Opacity Random", 0, 100]
], "legacy graphs project optional variation controls with zero defaults");
assert.strictEqual(particle.curves.size.custom, true);
assert.deepStrictEqual(particle.curves.size.points, [
    { age: 0, value: 10 }, { age: 0.4, value: 24 }, { age: 1, value: 2 }
]);
assert.strictEqual(output.maxParticles, 6400);
assert.strictEqual(output.params[0].graphNodeId, source.nodes[0].id,
                   "Output Max Particles control edits the emitter-owned value");
assert.deepStrictEqual(view.activeEmitterParameters(source), {
    emitterId: source.nodes[0].id, maxParticles: 6400, birthRate: 60, lifetimeSeconds: 2,
    branchLifetimes: [2]
});
assert.strictEqual(view.countLiveParticles(2.5, 60, 2, 6400), 120,
                   "graph frame status counts the emitter’s live slots at AE comp time");
assert.strictEqual(view.countLiveParticles(2.5, 60, 2, 10), 10,
                   "graph frame status applies the active emitter’s global cap");

var independentLifetimes = codec.fromHex(codec.toHex(graph()));
var longerParticle = JSON.parse(JSON.stringify(independentLifetimes.nodes[1]));
longerParticle.id = uuid(4);
longerParticle.parameters = longerParticle.parameters.filter(function (parameter) {
    return parameter.key !== "7";
});
longerParticle.parameters.push({ key: "11", type: 4, value: 3 });
independentLifetimes.nodes[1].parameters.push({ key: "11", type: 4, value: 1 });
independentLifetimes.nodes.push(longerParticle);
independentLifetimes.edges.push(
    { id: uuid(13), sourceNode: uuid(1), sourcePort: "1", destinationNode: uuid(4), destinationPort: "1" },
    { id: uuid(14), sourceNode: uuid(4), sourcePort: "2", destinationNode: uuid(3), destinationPort: "1" }
);
independentLifetimes = codec.fromHex(codec.toHex(independentLifetimes));
var lifetimeSummary = view.activeEmitterParameters(independentLifetimes);
assert.deepStrictEqual(lifetimeSummary.branchLifetimes, [1, 3],
                       "active Particle nodes report their own lifetimes in stable ID order");
assert.strictEqual(lifetimeSummary.lifetimeSeconds, 3,
                   "the frame-count window uses the longest active branch lifetime");
assert.strictEqual(view.countLiveParticles(2.5, 2, 3, 100, lifetimeSummary.branchLifetimes), 4,
                   "live count expires each modulo-assigned Particle branch independently");

var withParkedEmitter = codec.fromHex(codec.toHex(source));
withParkedEmitter.nodes.push({ id: uuid(10), type: edits.types.emitter, schemaVersion: 1,
    parameters: [{ key: "1", type: 3, value: 1 }, { key: "2", type: 4, value: 1 },
                 { key: "4", type: 4, value: 1 }] });
assert.strictEqual(view.activeEmitterParameters(withParkedEmitter).emitterId, source.nodes[0].id,
                   "a disconnected emitter does not replace the emitter feeding Output");
var parkedEmitterOutput = view.project(withParkedEmitter).nodes.filter(function (node) {
    return node.kind === "output";
})[0];
assert.strictEqual(parkedEmitterOutput.maxParticles, 6400,
                   "Output keeps its cap control bound to the emitter in its active ancestry");
assert.strictEqual(parkedEmitterOutput.params[0].graphNodeId, source.nodes[0].id);

var withSecondActiveEmitter = codec.fromHex(codec.toHex(source));
withSecondActiveEmitter.nodes.push({ id: uuid(10), type: edits.types.emitter, schemaVersion: 1,
    parameters: [{ key: "1", type: 3, value: 100 }, { key: "2", type: 4, value: 20 },
                 { key: "4", type: 4, value: 3 }] });
withSecondActiveEmitter.nodes.push({ id: uuid(11), type: edits.types.particle, schemaVersion: 1,
    parameters: [{ key: "1", type: 5, value: [1, 1, 1] }, { key: "2", type: 5, value: [1, 1, 1] },
                 { key: "3", type: 4, value: 10 }, { key: "4", type: 4, value: 10 },
                 { key: "5", type: 4, value: 1 }, { key: "6", type: 4, value: 1 }] });
withSecondActiveEmitter.edges.push(
    { id: uuid(20), sourceNode: uuid(10), sourcePort: "1", destinationNode: uuid(11), destinationPort: "1" },
    { id: uuid(21), sourceNode: uuid(11), sourcePort: "2", destinationNode: uuid(3), destinationPort: "1" });
assert.strictEqual(view.activeEmitterParameters(withSecondActiveEmitter), null,
                   "frame count fails closed when multiple active emitters are unsupported");

var emitter = projected.nodes.filter(function (node) { return node.kind === "emitter"; })[0];
var type = emitter.params.filter(function (parameter) { return parameter.graphKey === "5"; })[0];
assert.strictEqual(type.value, 3);
assert.strictEqual(view.parameterToGraphValue(type, 3), 2, "popup labels map to zero-based graph enums");
assert.strictEqual(emitter.params.filter(function (parameter) { return parameter.graphKey === "10"; })[0].label,
                   "Disc Size");
assert.deepStrictEqual(["19", "20", "21"].map(function (key) {
    var parameter = emitter.params.filter(function (entry) { return entry.graphKey === key; })[0];
    return [parameter.value, parameter.max, parameter.unit];
}), [[1920, 100000, "px"], [1080, 100000, "px"], [720, 100000, "px"]],
"emitter axis dimensions are direct pixel values in the graph inspector");
var color = particle.params.filter(function (parameter) { return parameter.graphKey === "1"; })[0];
assert.deepStrictEqual(view.parameterToGraphValue(color, [255, 127.5, 0]), [1, 0.5, 0]);

var wrongCurveType = graph();
wrongCurveType.nodes[1].parameters.push({ key: "7", type: 4, value: 0 });
assert.throws(function () { view.project(wrongCurveType); }, /opaque value type/i);

var variedGraph = codec.fromHex(codec.toHex(graph()));
var variedParticle = variedGraph.nodes.filter(function (node) { return node.type === edits.types.particle; })[0];
variedParticle.parameters.push({ key: "9", type: 4, value: 65 }, { key: "10", type: 4, value: 30 });
variedGraph = codec.fromHex(codec.toHex(variedGraph));
variedParticle = view.project(variedGraph).nodes.filter(function (node) { return node.kind === "particle"; })[0];
assert.strictEqual(variedParticle.params.filter(function (parameter) { return parameter.graphKey === "9"; })[0].value, 65);
assert.strictEqual(variedParticle.params.filter(function (parameter) { return parameter.graphKey === "10"; })[0].value, 30);

var oldAliasEdit = view.mapLegacyEdit(graph(), { type: "deleteNodes", nodeIds: ["particle"] });
assert.strictEqual(oldAliasEdit.nodeIds[0], uuid(2), "legacy canvas node IDs map to stable graph UUIDs");
console.log("Panel graph view checks passed.");
