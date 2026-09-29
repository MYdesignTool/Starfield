"use strict";

const assert = require("node:assert/strict");
const codec = require("../cep_panel/js/graph_codec.js");
const layout = require("../cep_panel/js/graph_layout.js");

function id(tail) { return "0000000000000000000000000000000" + tail; }

function graphWithChain() {
    return {
        version: 1,
        nodes: [
            { id: id("1"), type: "org.starfieldfx.nodes.emitter", schemaVersion: 1, parameters: [] },
            { id: id("2"), type: "org.starfieldfx.nodes.particle", schemaVersion: 1, parameters: [] },
            { id: id("3"), type: "org.starfieldfx.nodes.force", schemaVersion: 1, parameters: [] },
            { id: id("4"), type: "org.starfieldfx.nodes.output", schemaVersion: 1, parameters: [] },
            { id: id("5"), type: "org.starfieldfx.nodes.force", schemaVersion: 1, parameters: [] }
        ],
        edges: [
            { id: id("6"), sourceNode: id("1"), sourcePort: "1", destinationNode: id("2"), destinationPort: "1" },
            { id: id("7"), sourceNode: id("2"), sourcePort: "2", destinationNode: id("3"), destinationPort: "1" },
            { id: id("8"), sourceNode: id("3"), sourcePort: "2", destinationNode: id("4"), destinationPort: "1" }
        ],
        optionalRecords: ["0280020008000000"]
    };
}

function testDerivedTopDownPositions() {
    const graph = graphWithChain();
    const positions = layout.derive(graph);
    assert.deepEqual(positions[id("1")], { x: 235, y: 22 });
    assert.deepEqual(positions[id("2")], { x: 235, y: 122 });
    assert.deepEqual(positions[id("3")], { x: 235, y: 222 });
    assert.deepEqual(positions[id("4")], { x: 235, y: 322 });
    assert.equal(positions[id("5")].y, 422, "isolated nodes are placed below the connected flow");
}

function testProjectRecordRoundTripAndOpaquePreservation() {
    const graph = graphWithChain();
    const positions = {
        [id("1")]: { x: -480, y: 12.5 },
        [id("2")]: { x: 30, y: 120 },
        [id("3")]: { x: 210, y: 280 },
        [id("4")]: { x: 30, y: 400 },
        [id("5")]: { x: 800, y: 500 }
    };
    const updated = layout.set(graph, positions);
    assert.equal(updated.optionalRecords.length, 2);
    assert.ok(updated.optionalRecords.includes("0280020008000000"));
    const decoded = codec.fromHex(codec.toHex(updated));
    assert.deepEqual(layout.get(decoded), positions);
    assert.deepEqual(layout.resolve(decoded), positions);
    assert.equal(codec.toHex(decoded), codec.toHex(updated));
}

function testLayoutRejectsInvalidMapsAndDuplicateRecords() {
    const graph = graphWithChain();
    const complete = layout.derive(graph);
    const missing = Object.assign({}, complete);
    delete missing[id("5")];
    assert.throws(() => layout.set(graph, missing), error => error.code === "invalid_layout");

    const outside = Object.assign({}, complete, { [id("1")]: { x: layout.coordinateLimit + 1, y: 0 } });
    assert.throws(() => layout.set(graph, outside), error => error.code === "invalid_layout");

    const withLayout = layout.set(graph, complete);
    withLayout.optionalRecords.push(withLayout.optionalRecords[1]);
    assert.throws(() => layout.get(withLayout), error => error.code === "invalid_layout");
}

testDerivedTopDownPositions();
testProjectRecordRoundTripAndOpaquePreservation();
testLayoutRejectsInvalidMapsAndDuplicateRecords();
console.log("Panel graph layout checks passed.");
