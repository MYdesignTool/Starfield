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
        "Emission Angle Z": 0, "Direction": 1, "Direction Span": 60
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
const nodeIds = ["11223344556677889900aabbccddeeff", "ffeeddccbbaa00998877665544332211"];

const rendererProperties = {};
const snapshot = scalar("Graph Snapshot", null);
snapshot.expression = snapshotExpression(4, graphHex);
const mailbox = scalar("Graph Edit Request", "");
mailbox.expression = "";
mailbox.expressionEnabled = true;
const receipt = scalar("Graph Edit Receipt", 0);
const commit = scalar("Commit Graph Edit", 0);
commit.setValue = function (nonce) {
    this.value = nonce;
    const match = /^\/\*SFLDTXN1:([0-9]+):([0-9]+):([0-9]+):([0-9a-f]{8}):([0-9a-f]+)\*\/0$/.exec(mailbox.expression);
    if (!match || Number(match[1]) !== nonce || Number(match[2]) !== 4 ||
        Number(match[3]) !== match[5].length / 2 || crc32Hex(match[5]) !== match[4]) {
        receipt.value = -nonce;
        return;
    }
    snapshot.expression = snapshotExpression(5, match[5]);
    receipt.value = nonce;
};
Object.assign(rendererProperties, {
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
assert.equal(undo.begins, 3, "the rejected duplicate reconciliation still closes one undo group");
assert.equal(undo.ends, 3);

console.log("Native node gateway checks passed (Emitter create, independent values, dimensions, duplicate-ID rejection, duplicate cleanup, graph commit).");
