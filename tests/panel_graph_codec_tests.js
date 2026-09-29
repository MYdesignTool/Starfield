"use strict";

const assert = require("node:assert/strict");
const codec = require("../cep_panel/js/graph_codec.js");

function id(tail) { return "0000000000000000000000000000000" + tail; }

function sampleGraph() {
    return {
        version: 1,
        nodes: [
            { id: id("1"), type: "org.starfieldfx.nodes.emitter", schemaVersion: 1, parameters: [
                { key: "18446744073709551615", type: 4, value: -12.5 },
                { key: "2", type: 1, value: true },
                { key: "3", type: 2, value: -2147483648 },
                { key: "4", type: 3, value: 4294967295 },
                { key: "5", type: 5, value: [1.25, -2.5, 3.75] },
                { key: "6", type: 6, value: "粒子 ✨ café" },
                { key: "7", type: 7, value: new Uint8Array([0, 1, 127, 255]) }
            ] },
            { id: id("2"), type: "org.starfieldfx.nodes.output", schemaVersion: 1, parameters: [] }
        ],
        edges: [{ id: id("3"), sourceNode: id("1"), sourcePort: "18446744073709551615",
                  destinationNode: id("2"), destinationPort: "1" }],
        optionalRecords: []
    };
}

function testCanonicalRoundTripAndAllValueKinds() {
    const graph = sampleGraph();
    const encoded = codec.serialize(graph);
    const decoded = codec.parse(encoded);
    assert.equal(decoded.nodes.length, 2);
    assert.equal(decoded.edges.length, 1);
    const params = decoded.nodes[0].parameters;
    assert.equal(params.find(parameter => parameter.key === "2").value, true);
    assert.equal(params.find(parameter => parameter.key === "3").value, -2147483648);
    assert.equal(params.find(parameter => parameter.key === "4").value, 4294967295);
    assert.equal(params.find(parameter => parameter.key === "18446744073709551615").value, -12.5);
    assert.deepEqual(params.find(parameter => parameter.key === "5").value, [1.25, -2.5, 3.75]);
    assert.equal(params.find(parameter => parameter.key === "6").value, "粒子 ✨ café");
    assert.deepEqual(Array.from(params.find(parameter => parameter.key === "7").value), [0, 1, 127, 255]);
    assert.equal(codec.toHex(decoded), codec.bytesToHex(encoded));

    const reordered = sampleGraph();
    reordered.nodes.reverse();
    reordered.nodes[1].parameters.reverse();
    reordered.edges.reverse();
    assert.equal(codec.toHex(reordered), codec.bytesToHex(encoded), "serialization must be canonical");
}

function testCrcAndBounds() {
    const ascii = new Uint8Array(Array.from("123456789", character => character.charCodeAt(0)));
    assert.equal(codec.crc32(ascii), 0xcbf43926);

    const encoded = codec.serialize(sampleGraph());
    const corrupted = new Uint8Array(encoded);
    corrupted[corrupted.length - 1] ^= 1;
    assert.throws(() => codec.parse(corrupted), error => error.code === "checksum_mismatch");
    assert.throws(() => codec.parse(encoded.subarray(0, 20)), error => error.code === "invalid_header");
    assert.throws(() => codec.fromHex("0".repeat(codec.limits.bytes * 2 + 2)),
                  error => error.code === "size_limit_exceeded");

    const overLimitCount = new Uint8Array(encoded);
    new DataView(overLimitCount.buffer).setUint32(16, codec.limits.nodes + 1, true);
    assert.throws(() => codec.parse(overLimitCount), error => error.code === "size_limit_exceeded");
}

function testOptionalRecordPreservationAndStructuralRejects() {
    const graph = sampleGraph();
    graph.optionalRecords = ["018001000a000000aabb"];
    const encoded = codec.serialize(graph);
    const decoded = codec.parse(encoded);
    assert.deepEqual(decoded.optionalRecords, graph.optionalRecords);
    assert.equal(codec.toHex(decoded), codec.bytesToHex(encoded));

    const duplicate = sampleGraph();
    duplicate.nodes[1].id = duplicate.nodes[0].id;
    assert.throws(() => codec.serialize(duplicate), error => error.code === "invalid_graph");
    const dangling = sampleGraph();
    dangling.edges[0].destinationNode = id("9");
    assert.throws(() => codec.serialize(dangling), error => error.code === "invalid_graph");

    const tooManyOptionalRecords = sampleGraph();
    tooManyOptionalRecords.optionalRecords = Array(codec.limits.optionalRecords + 1).fill("018001000a000000aabb");
    assert.throws(() => codec.serialize(tooManyOptionalRecords),
                  error => error.code === "size_limit_exceeded");
}

testCanonicalRoundTripAndAllValueKinds();
testCrcAndBounds();
testOptionalRecordPreservationAndStructuralRejects();
console.log("Panel graph codec checks passed.");
