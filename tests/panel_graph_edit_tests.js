"use strict";

var assert = require("assert");
var edits = require("../cep_panel/js/graph_edits.js");
var codec = require("../cep_panel/js/graph_codec.js");
var layout = require("../cep_panel/js/graph_layout.js");

function uuid(n) { return ("00000000000000000000000000000000" + n.toString(16)).slice(-32); }
function graph() {
    var emitter = uuid(1), particle = uuid(2), force = uuid(3), output = uuid(4);
    return {
        version: 1,
        nodes: [
            { id: emitter, type: edits.types.emitter, schemaVersion: 3, parameters: [
                { key: "2", type: 4, value: 500 }, { key: "6", type: 5, value: [1, 2, 3] }
            ] },
            { id: particle, type: edits.types.particle, schemaVersion: 2, parameters: [
                { key: "1", type: 5, value: [1, 1, 1] }, { key: "2", type: 5, value: [1, 1, 1] },
                { key: "3", type: 4, value: 10 }, { key: "4", type: 4, value: 10 },
                { key: "5", type: 4, value: 1 }, { key: "6", type: 4, value: 1 },
                { key: "11", type: 4, value: 2 }
            ] },
            { id: force, type: edits.types.force, schemaVersion: 1, parameters: [] },
            { id: output, type: edits.types.output, schemaVersion: 2, parameters: [
                { key: "1", type: 3, value: 1000 }
            ] }
        ],
        edges: [
            { id: uuid(11), sourceNode: emitter, sourcePort: "1", destinationNode: particle, destinationPort: "1" },
            { id: uuid(12), sourceNode: particle, sourcePort: "2", destinationNode: force, destinationPort: "1" },
            { id: uuid(13), sourceNode: force, sourcePort: "2", destinationNode: output, destinationPort: "1" }
        ],
        optionalRecords: ["0280020008000000"]
    };
}

var nextId = 100;
function idFactory() { return uuid(nextId++); }
function edgeTo(value, nodeId) { return value.edges.filter(function (edge) { return edge.destinationNode === nodeId; }); }

var original = graph();
var newParticle = edits.apply(original, { type: "addNode", nodeType: "particle" }, idFactory).nodes[4];
assert.deepStrictEqual(newParticle.parameters.filter(function (parameter) {
    return parameter.key === "9" || parameter.key === "10";
}).map(function (parameter) { return parameter.value; }), [0, 0],
"new Particle nodes include zero-default random variation controls");
assert.strictEqual(newParticle.schemaVersion, 2, "new Particle nodes use the current node schema");
assert.strictEqual(newParticle.parameters.filter(function (parameter) {
    return parameter.key === "11";
})[0].value, 2, "new Particle nodes own their lifetime");
assert.deepStrictEqual(newParticle.parameters.filter(function (parameter) {
    return parameter.key === "4" || parameter.key === "6";
}).map(function (parameter) { return parameter.value; }), [100, 100],
"new Particle nodes start with neutral 100% Size and Opacity curves");
var particleId = original.nodes[1].id;
var forceId = original.nodes[2].id;
var outputId = original.nodes[3].id;
var updatedOutput = edits.apply(original, { type: "setParameters", changes: [
    { nodeId: outputId, parameterKey: "1", valueType: 3, value: 2500 }
] }, idFactory);
assert.strictEqual(updatedOutput.nodes[3].parameters[0].value, 2500,
                   "Output Max Particles is edited on the main renderer's logical terminal");
var addedForce = edits.apply(original, { type: "addNode", nodeType: "force" }, idFactory);
assert.strictEqual(addedForce.nodes.length, 5);
assert.strictEqual(original.nodes.length, 4, "edit planner must not mutate its input");
assert.strictEqual(addedForce.optionalRecords[0], original.optionalRecords[0]);
var addedLayout = layout.get(addedForce);
assert.deepStrictEqual(addedLayout[original.nodes[0].id], { x: 235, y: 22 });
assert.deepStrictEqual(addedLayout[addedForce.nodes[4].id], { x: 235, y: 422 },
                       "new nodes get a deterministic project position");
assert.strictEqual(codec.fromHex(codec.toHex(addedForce)).nodes.length, 5,
                   "planned graph edits must round-trip through the schema-1 codec");

var placedForce = edits.apply(original, {
    type: "addNode", nodeType: "force", position: { x: -400, y: 88 }
}, idFactory);
assert.deepStrictEqual(layout.get(placedForce)[placedForce.nodes[4].id], { x: -400, y: 88 });

var parallel = edits.apply(original, { type: "connect", from: particleId, to: outputId }, idFactory);
assert.strictEqual(edgeTo(parallel, outputId).length, 2, "Output input allows parallel streams");
var forceFanIn = edits.apply(original, { type: "addNode", nodeType: "force" }, idFactory);
var secondForce = forceFanIn.nodes[4].id;
forceFanIn = edits.apply(forceFanIn, { type: "connect", from: secondForce, to: forceId }, idFactory);
assert.strictEqual(edgeTo(forceFanIn, forceId).length, 2, "Force input supports parallel fields");

var reconnect = edits.apply(original, { type: "connect", from: original.nodes[0].id, to: particleId }, idFactory);
assert.strictEqual(edgeTo(reconnect, particleId).length, 1, "Particle input reconnect replaces the old source edge");

var inserted = edits.apply(original, { type: "insertNode", from: particleId, to: forceId, nodeType: "force" }, idFactory);
var insertedId = inserted.nodes[4].id;
assert.ok(inserted.edges.some(function (edge) { return edge.sourceNode === particleId && edge.destinationNode === insertedId; }));
assert.ok(inserted.edges.some(function (edge) { return edge.sourceNode === insertedId && edge.destinationNode === forceId; }));
assert.strictEqual(inserted.edges.length, 4);
assert.deepStrictEqual(layout.get(inserted)[insertedId], { x: 235, y: 172 },
                       "a spliced node is placed between its endpoints");

var dropPosition = { x: 462, y: 188 };
var movedGroupPosition = { x: -360, y: 158 };
var spliceEdge = [particleId, forceId];
spliceEdge.id = uuid(12);
var spliceEdit = edits.createInsertEdit(addedForce.nodes[4].id,
    spliceEdge, dropPosition,
    { [particleId]: movedGroupPosition });
var reused = edits.apply(addedForce, spliceEdit, idFactory);
assert.strictEqual(reused.nodes.length, 5, "wire insertion reuses the dragged node");
assert.ok(reused.edges.some(function (edge) {
    return edge.sourceNode === particleId && edge.destinationNode === addedForce.nodes[4].id;
}));
assert.ok(reused.edges.some(function (edge) {
    return edge.sourceNode === addedForce.nodes[4].id && edge.destinationNode === forceId;
}));
assert.deepStrictEqual(layout.get(reused)[addedForce.nodes[4].id], dropPosition,
                       "wire insertion retains the dragged node's actual drop position");
assert.deepStrictEqual(layout.get(reused)[particleId], movedGroupPosition,
                       "wire insertion commits the rest of a moved selection in the same graph edit");
assert.deepStrictEqual(layout.get(codec.fromHex(codec.toHex(reused)))[addedForce.nodes[4].id], dropPosition,
                       "the inserted node's drop position survives graph serialization");

var disconnected = edits.apply(original, { type: "disconnect", edgeId: uuid(12) }, idFactory);
assert.strictEqual(disconnected.edges.length, 2);

var curveBytes = new Uint8Array([1, 2, 0, 0]);
var curveBuffer = new ArrayBuffer(32);
var curveView = new DataView(curveBuffer);
curveView.setFloat64(0, 0, true); curveView.setFloat64(8, 8, true);
curveView.setFloat64(16, 1, true); curveView.setFloat64(24, 2, true);
var completeCurve = new Uint8Array(36);
completeCurve.set(curveBytes, 0); completeCurve.set(new Uint8Array(curveBuffer), 4);
var particleParameters = edits.apply(original, { type: "setParameters", changes: [
    { nodeId: particleId, parameterKey: "3", valueType: 4, value: 8 },
    { nodeId: particleId, parameterKey: "7", valueType: 7, value: completeCurve }
] }, idFactory);
var editedParticle = particleParameters.nodes.filter(function (node) { return node.id === particleId; })[0];
assert.strictEqual(editedParticle.parameters.filter(function (parameter) { return parameter.key === "3"; })[0].value, 8);
assert.deepStrictEqual(Array.prototype.slice.call(
    editedParticle.parameters.filter(function (parameter) { return parameter.key === "7"; })[0].value),
    Array.prototype.slice.call(completeCurve), "optional curve payload is added byte-for-byte");
particleParameters = edits.apply(particleParameters, { type: "setParameters", changes: [
    { nodeId: particleId, parameterKey: "7", remove: true }
] }, idFactory);
editedParticle = particleParameters.nodes.filter(function (node) { return node.id === particleId; })[0];
assert.strictEqual(editedParticle.parameters.some(function (parameter) { return parameter.key === "7"; }), false,
                   "linear curve mode removes the optional curve parameter");

var duplicated = edits.apply(original, { type: "duplicateNodes", nodeIds: [particleId, forceId] }, idFactory);
assert.strictEqual(duplicated.nodes.length, 6);
assert.strictEqual(duplicated.edges.length, 4, "duplicate copies only internal edges");
assert.strictEqual(duplicated.edges.filter(function (edge) { return edge.sourceNode === particleId; }).length, 1);
var duplicateLayout = layout.get(duplicated);
assert.deepStrictEqual(duplicateLayout[duplicated.nodes[4].id], { x: 263, y: 150 });
assert.deepStrictEqual(duplicateLayout[duplicated.nodes[5].id], { x: 263, y: 250 });

var moved = edits.apply(original, {
    type: "moveNodes", positions: { [forceId]: { x: -260, y: 610 } }
}, idFactory);
assert.deepStrictEqual(layout.get(moved)[forceId], { x: -260, y: 610 });
var forceMovedAboveParticle = edits.apply(original, {
    type: "moveNodes", positions: { [forceId]: { x: 180, y: -420 } }
}, idFactory);
assert.deepStrictEqual(layout.get(forceMovedAboveParticle)[forceId], { x: 180, y: -420 },
                       "visual node placement can cross the graph flow direction without changing connections");
assert.throws(function () {
    edits.apply(original, { type: "moveNodes", positions: { [uuid(90)]: { x: 0, y: 0 } } }, idFactory);
}, /absent from the graph/i);

var deleted = edits.apply(original, { type: "deleteNodes", nodeIds: [forceId] }, idFactory);
assert.strictEqual(deleted.nodes.length, 3);
assert.strictEqual(deleted.edges.length, 1);
assert.strictEqual(Object.prototype.hasOwnProperty.call(layout.get(deleted), forceId), false,
                   "deleting a node removes its saved position");

assert.throws(function () {
    edits.apply(original, { type: "connect", from: original.nodes[0].id, to: forceId }, idFactory);
}, /Emitter must connect directly to a Particle node/i);
assert.throws(function () {
    edits.apply(original, { type: "deleteNodes", nodeIds: [outputId] }, idFactory);
}, /required Output node/i);
assert.throws(function () {
    edits.apply(original, { type: "connect", from: forceId, to: particleId }, idFactory);
}, /Particle node input accepts an Emitter directly/i);

console.log("Panel graph edit planner checks passed.");
