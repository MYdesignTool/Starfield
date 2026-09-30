"use strict";

// Focused fake-host coverage for creating/removing project-owned node AEX instances.
// This checks the CEP transaction logic; it does not replace AE 2023 qualification.
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");

const source = fs.readFileSync(path.join(__dirname, "..", "cep_panel", "jsx", "starfield_gateway.jsx"), "utf8");

function crc32Hex(hex) {
    let crc = 0xffffffff;
    for (let i = 0; i < hex.length; i += 2) {
        crc ^= parseInt(hex.slice(i, i + 2), 16);
        for (let bit = 0; bit < 8; bit++) crc = (crc >>> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
    }
    return ((crc ^ 0xffffffff) >>> 0).toString(16).padStart(8, "0");
}

function snapshotExpression(revision, graphHex) {
    return "/*SFLDSNAP1:" + revision + ":" + (graphHex.length / 2) + ":" +
        crc32Hex(graphHex) + ":" + graphHex + "*/0";
}

function scalar(name, initial) {
    return {
        name,
        value: Array.isArray(initial) ? initial.slice() : initial,
        setValue(value) { this.value = Array.isArray(value) ? value.slice() : value; }
    };
}

function nodeControls() {
    const values = {
        "Type": 1, "Particles Per Second": 0, "Random Seed": 0, "Particle Size": 10,
        "Opacity": 1, "Origin": [0, 0, 0], "Velocity X": 0, "Velocity Y": 0,
        "Velocity Z": 0, "Disc Size": 0.05, "Speed Random": 0.15,
        "Size X": 100, "Size Y": 100, "Size Z": 100, "Emission Speed": 0,
        "Emission Speed Random": 0, "Emission Angle X": 0, "Emission Angle Y": 0,
        "Emission Angle Z": 0, "Direction": 1, "Direction Span": 60,
        "Color Start": [255, 255, 255, 1], "Color End": [255, 255, 255, 1],
        "Size": 10, "Size Over Life": 100, "Opacity Over Life": 100,
        "Size Random": 0, "Opacity Random": 0, "Lifetime": 2,
        "Size Curve Count": 0, "Opacity Curve Count": 0,
        "Gravity": [0, 0, 0], "Linear Drag": 0
    };
    for (let i = 0; i < 8; i++) values["Node UUID " + i] = 0;
    values["Panel Sync Guard"] = 0;
    return Object.keys(values).reduce((result, name) => {
        result[name] = scalar(name, values[name]);
        return result;
    }, {});
}

const graphHex = "01".repeat(32);
const changedGraphHex = "02".repeat(32);
const finalGraphHex = "03".repeat(32);
const nodeIds = ["11223344556677889900aabbccddeeff", "ffeeddccbbaa00998877665544332211"];
let activeGraphRevision = 4;

const rendererProperties = {};
const nodeEffectsReady = scalar("Node Effects Ready", 0);
const snapshot = scalar("Graph Snapshot", null);
snapshot.expression = snapshotExpression(4, graphHex);
const mailbox = scalar("Graph Edit Request", "");
mailbox.expression = "";
mailbox.expressionEnabled = true;
mailbox.canSetExpression = true;
const receipt = scalar("Graph Edit Receipt", 0);
const commit = scalar("Commit Graph Edit", 0);
commit.setValue = function (nonce) {
    this.value = nonce;
    const match = /^\/\*SFLDTXN1:([0-9]+):([0-9]+):([0-9]+):([0-9a-f]{8}):([0-9a-f]+)\*\/0$/.exec(mailbox.expression);
    if (!match || Number(match[1]) !== nonce || Number(match[2]) !== activeGraphRevision ||
        Number(match[3]) !== match[5].length / 2 || crc32Hex(match[5]) !== match[4]) {
        receipt.value = -nonce;
        return;
    }
    activeGraphRevision++;
    snapshot.expression = snapshotExpression(activeGraphRevision, match[5]);
    receipt.value = nonce;
};
Object.assign(rendererProperties, {
    "Node Effects Ready": nodeEffectsReady,
    "Graph Snapshot": snapshot,
    "Graph Edit Request": mailbox,
    "Commit Graph Edit": commit,
    "Graph Edit Receipt": receipt
});

const renderer = {
    matchName: "org.starfieldfx.particle",
    property(name) { return rendererProperties[name] || null; }
};

const paradeItems = [renderer];
const parade = {
    get numProperties() { return paradeItems.length; },
    property(index) { return paradeItems[index - 1] || null; },
    addProperty(matchName) {
        const effect = {
            matchName,
            name: "",
            properties: nodeControls(),
            property(name) { return this.properties[name] || null; },
            remove() {
                const index = paradeItems.indexOf(this);
                if (index >= 0) paradeItems.splice(index, 1);
            }
        };
        paradeItems.push(effect);
        return effect;
    }
};

const layer = { id: 29, name: "Particle Layer", selected: true,
    property(name) { return name === "ADBE Effect Parade" ? parade : null; } };
function CompItem() {}
const comp = new CompItem();
comp.id = 17;
comp.name = "Test Comp";
comp.numLayers = 1;
comp.layer = index => index === 1 ? layer : null;
const undo = { begins: 0, ends: 0 };
const exported = {};
const app = {
    project: { activeItem: comp, rootFolder: { id: 5 } },
    beginUndoGroup() { undo.begins++; },
    endUndoGroup() { undo.ends++; }
};
vm.runInNewContext(source, { app, CompItem, $: { global: exported } }, { filename: "starfield_gateway.jsx" });

function emitterNode(id, rate, dimensions) {
    return { id, type: "org.starfieldfx.nodes.emitter", schemaVersion: 3, parameters: [
        { key: "2", type: 4, value: rate }, { key: "3", type: 3, value: 5 },
        { key: "5", type: 3, value: 1 }, { key: "6", type: 5, value: [1920, 1080, 1080] },
        { key: "7", type: 5, value: [1, 2, 3] }, { key: "8", type: 4, value: 18 },
        { key: "9", type: 4, value: 0.75 }, { key: "10", type: 4, value: 0.05 },
        { key: "11", type: 4, value: 0.2 }, { key: "19", type: 4, value: dimensions[0] },
        { key: "20", type: 4, value: dimensions[1] }, { key: "21", type: 4, value: dimensions[2] }
    ] };
}

function invoke(operation, fields) {
    return JSON.parse(exported["SFLD_" + operation](JSON.stringify(Object.assign({
        protocol: "org.starfieldfx.panel", version: 1, requestId: operation,
        operation, target: { token: "p5-c17-l29" }, pinTarget: false
    }, fields))));
}

const nodes = [emitterNode(nodeIds[0], 24, [320, 180, 90]),
               emitterNode(nodeIds[1], 48, [640, 360, 180])];
const ensured = invoke("ensureNodeEffects", {
    baseGraphRevision: 4, graphHex, nodeManifest: nodes
});
assert.equal(ensured.ok, true, ensured.error && ensured.error.message);
assert.equal(ensured.count, 2);
assert.equal(paradeItems.length, 3, "two Emitter effects are added beside the one main renderer");
assert.deepEqual(paradeItems.slice(1).map(effect => effect.matchName), [
    "org.starfieldfx.node.emitter", "org.starfieldfx.node.emitter"
]);
assert.equal(paradeItems[1].property("Particles Per Second").value, 24);
assert.equal(paradeItems[2].property("Particles Per Second").value, 48);
assert.equal(nodeEffectsReady.value, 1, "successful bootstrap is persisted on the main effect");
assert.deepEqual(["Size X", "Size Y", "Size Z"].map(name => paradeItems[1].property(name).value), [320, 180, 90]);
assert.deepEqual(["Size X", "Size Y", "Size Z"].map(name => paradeItems[2].property(name).value), [640, 360, 180]);
assert.deepEqual(paradeItems.slice(1).map(effect => effect.name), [
    "Emitter " + nodeIds[0].slice(0, 6), "Emitter " + nodeIds[1].slice(0, 6)
]);
assert.deepEqual(paradeItems.slice(1).map(effect =>
    Array.from({ length: 8 }, (_, index) => effect.property("Node UUID " + index).value)
), [
    [0x1122, 0x3344, 0x5566, 0x7788, 0x9900, 0xaabb, 0xccdd, 0xeeff],
    [0xffee, 0xddcc, 0xbbaa, 0x0099, 0x8877, 0x6655, 0x4433, 0x2211]
]);
assert.equal(undo.begins, 1);
assert.equal(undo.ends, 1);
const unchanged = invoke("ensureNodeEffects", { baseGraphRevision: 4, graphHex, nodeManifest: nodes });
assert.equal(unchanged.ok, true);
assert.equal(undo.begins, 1, "automatic refresh does not add empty undo groups once node effects exist");

// AE-level Ctrl+D copies hidden identity streams verbatim. A graph-owned node
// lookup must reject that ambiguous state rather than silently choosing one.
const copiedEmitter = parade.addProperty(paradeItems[1].matchName);
copiedEmitter.name = paradeItems[1].name;
for (const [name, property] of Object.entries(paradeItems[1].properties)) {
    copiedEmitter.property(name).setValue(property.value);
}
const ambiguous = invoke("ensureNodeEffects", {
    baseGraphRevision: 4, graphHex, nodeManifest: nodes
});
assert.equal(ambiguous.ok, false);
assert.match(ambiguous.error.message, /share node identity/i,
    "duplicate native effect identities are reported instead of binding the first match");

const paradeCountBeforeUnsupportedCarrier = paradeItems.length;
mailbox.canSetExpression = false;
const unsupportedCarrier = invoke("submitGraph", {
    baseGraphRevision: 4,
    baseNodeManifest: nodes,
    nodeManifest: [nodes[1]],
    graphHex: changedGraphHex
});
assert.equal(unsupportedCarrier.ok, false);
assert.equal(unsupportedCarrier.error.code, "graph_carrier_unsupported");
assert.equal(paradeItems.length, paradeCountBeforeUnsupportedCarrier,
    "an unsupported graph mailbox is rejected before adding or removing node effects");
assert.equal(undo.begins, 1, "mailbox capability rejection does not open an undo group");
mailbox.canSetExpression = true;

const committed = invoke("submitGraph", {
    baseGraphRevision: 4,
    baseNodeManifest: nodes,
    nodeManifest: [nodes[1]],
    graphHex: changedGraphHex
});
assert.equal(committed.ok, true, committed.error && committed.error.message);
assert.equal(committed.snapshot.revision, 5);
assert.equal(committed.snapshot.graphHex, changedGraphHex);
assert.equal(paradeItems.length, 2, "deleting a graph node removes all AE effects carrying its identity");
assert.equal(paradeItems[1].property("Particles Per Second").value, 48);
assert.equal(undo.begins, 2, "only the graph transaction opens an undo group");
assert.equal(undo.ends, 2);

paradeItems[1].remove();
const externallyDeleted = invoke("ensureNodeEffects", {
    baseGraphRevision: 5, graphHex: changedGraphHex, nodeManifest: [nodes[1]]
});
assert.equal(externallyDeleted.ok, true);
assert.deepEqual(externallyDeleted.missingNodeIds, [nodeIds[1]],
    "after bootstrap, a missing Effect Parade module is reported as a graph deletion");
assert.equal(paradeItems.length, 1, "refresh does not silently recreate a manually deleted node effect");

// The project-owned node manifest can materialize distinct native module types,
// preserve their typed values, and remove exactly the node omitted by a later edit.
const particleId = "00112233445566778899aabbccddeeff";
const secondParticleId = "102132435465768798a9bacbdcedfe0f";
const forceId = "ffeeddccbbaa00998877665544330001";
const secondForceId = "0011ffeeddccbbaa9988776655443322";
const particle = { id: particleId, type: "org.starfieldfx.nodes.particle", schemaVersion: 2, parameters: [
    { key: "1", type: 5, value: [0.25, 0.5, 0.75] }, { key: "2", type: 5, value: [1, 1, 1] },
    { key: "3", type: 4, value: 32 }, { key: "4", type: 4, value: 75 },
    { key: "5", type: 4, value: 0.8 }, { key: "6", type: 4, value: 60 },
    { key: "9", type: 4, value: 15 }, { key: "10", type: 4, value: 25 },
    { key: "11", type: 4, value: 4.5 }
] };
const secondParticle = { id: secondParticleId, type: particle.type, schemaVersion: particle.schemaVersion,
    parameters: particle.parameters.map(parameter => ({ key: parameter.key, type: parameter.type,
        value: Array.isArray(parameter.value) ? parameter.value.slice() : parameter.value })) };
secondParticle.parameters.find(parameter => parameter.key === "3").value = 64;
secondParticle.parameters.find(parameter => parameter.key === "11").value = 2.5;
const force = { id: forceId, type: "org.starfieldfx.nodes.force", schemaVersion: 1, parameters: [
    { key: "1", type: 5, value: [0, -2, 0] }, { key: "2", type: 4, value: 0.25 }
] };
const secondForce = { id: secondForceId, type: force.type, schemaVersion: force.schemaVersion, parameters: [
    { key: "1", type: 5, value: [1, 0, 0] }, { key: "2", type: 4, value: 0.5 }
] };
const invalidParticle = { id: particleId, type: particle.type, schemaVersion: particle.schemaVersion,
    parameters: [{ key: "999", type: 4, value: 1 }] };
const failedParticleAdd = invoke("submitGraph", {
    baseGraphRevision: 5, baseNodeManifest: [nodes[1]], graphHex: finalGraphHex,
    nodeManifest: [nodes[1], invalidParticle]
});
assert.equal(failedParticleAdd.ok, false, "unsupported node controls reject a partial effect creation");
assert.equal(paradeItems.length, 2, "a failed node initialization removes its partially created effect");

const mixedNodes = [nodes[1], particle, secondParticle, force, secondForce];
const mixed = invoke("submitGraph", {
    baseGraphRevision: 5, baseNodeManifest: [nodes[1]], graphHex: finalGraphHex,
    nodeManifest: mixedNodes
});
assert.equal(mixed.ok, true, mixed.error && mixed.error.message);
assert.equal(mixed.snapshot.revision, 6);
assert.deepEqual(paradeItems.slice(1).map(effect => effect.matchName), [
    "org.starfieldfx.node.emitter", "org.starfieldfx.node.particle",
    "org.starfieldfx.node.particle", "org.starfieldfx.node.force", "org.starfieldfx.node.force"
]);
assert.deepEqual(Array.from(paradeItems[2].property("Color Start").value), [0.25, 0.5, 0.75, 1]);
assert.equal(paradeItems[2].property("Size").value, 32);
assert.equal(paradeItems[2].property("Lifetime").value, 4.5);
assert.equal(paradeItems[3].property("Size").value, 64);
assert.equal(paradeItems[3].property("Lifetime").value, 2.5);
assert.equal(paradeItems[3].property("Node UUID 0").value, 0x1021);
assert.deepEqual(Array.from(paradeItems[4].property("Gravity").value), [0, -2, 0]);
assert.equal(paradeItems[4].property("Linear Drag").value, 0.25);
assert.deepEqual(Array.from(paradeItems[5].property("Gravity").value), [1, 0, 0]);
assert.equal(paradeItems[5].property("Linear Drag").value, 0.5);
assert.equal(paradeItems[5].property("Node UUID 0").value, 0x0011);
assert.equal(paradeItems[2].property("Node UUID 0").value, 0x0011,
    "the same identity can be retried after a failed partial creation");

const outputRejected = invoke("ensureNodeEffects", {
    baseGraphRevision: 6, graphHex: finalGraphHex,
    nodeManifest: mixedNodes.concat([{ id: "00000000000000000000000000000104",
        type: "org.starfieldfx.nodes.output", schemaVersion: 1, parameters: [] }])
});
assert.equal(outputRejected.ok, false, "Output stays virtual and is not materialized as an AEX");
assert.equal(paradeItems.length, 6);

const removeForce = invoke("submitGraph", {
    baseGraphRevision: 6, baseNodeManifest: mixedNodes,
    nodeManifest: [nodes[1], particle, secondParticle, secondForce], graphHex: changedGraphHex
});
assert.equal(removeForce.ok, true, removeForce.error && removeForce.error.message);
assert.equal(removeForce.snapshot.revision, 7);
assert.equal(removeForce.snapshot.graphHex, changedGraphHex);
assert.deepEqual(paradeItems.slice(1).map(effect => effect.matchName), [
    "org.starfieldfx.node.emitter", "org.starfieldfx.node.particle", "org.starfieldfx.node.particle",
    "org.starfieldfx.node.force"
]);
assert.equal(paradeItems[2].property("Size").value, 32, "Particle values survive an unrelated Force deletion");
assert.equal(paradeItems[3].property("Size").value, 64, "the second Particle retains its independent value");
assert.deepEqual(Array.from(paradeItems[4].property("Gravity").value), [1, 0, 0],
    "deleting one Force preserves the other Force's independent vector");
assert.equal(paradeItems[4].property("Linear Drag").value, 0.5);
assert.equal(paradeItems[4].property("Node UUID 0").value, 0x0011);
assert.equal(undo.begins, 5);
assert.equal(undo.ends, 5);

console.log("Native node gateway checks passed (Emitter, two independent Particle and two Force instances, failed-add cleanup and retry, emitter dimensions, duplicate-ID rejection, Output exclusion, graph commit and selective deletion).");
