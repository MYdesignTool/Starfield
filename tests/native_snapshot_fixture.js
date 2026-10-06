"use strict";

// Current ordinary-control receipts for client tests. This is a protocol fixture,
// not an AE compiler: the gateway host model exercises actual property writes.
const codec = require("../cep_panel/js/graph_codec.js");
const layout = require("../cep_panel/js/graph_layout.js");
const edits = require("../cep_panel/js/graph_edits.js");
const fields = ["maxParticles", "timeRemapEnabled", "timeRemapSeconds", "previewEnabled",
    "previewChance", "acceleration", "timeSamplingHz", "motionBlur", "shutterAngle",
    "shutterPhase", "motionBlurType", "motionBlurLevels", "linearAccuracy", "opacityBoost",
    "motionBlurDisregard"];
const defaults = [1000000, 0, 0, 0, 100, 0, 30, 1, 360, 0, 0, 8, 70, 0, 0];

function node(kind, id) {
    if (kind === "output") return {id, type: edits.types.output, schemaVersion: edits.schemaVersion(kind),
        parameters: defaults.map((value, i) => ({key: String(i + 1), type: [2, 4, 8, 9, 11, 12, 13].includes(i) ? 4 : 3, value}))};
    return edits.apply({version: 1, nodes: [], edges: [], optionalRecords: []},
        {type: "addNode", nodeType: kind}, () => id).nodes[0];
}

function receipt(response) {
    if (!response.ok || !response.snapshot || !response.snapshot.initialized) return response;
    const snapshot = response.snapshot, graph = codec.fromHex(snapshot.graphHex);
    const positions = layout.get(graph) || {};
    snapshot.nativeNodes = graph.nodes.filter(n => n.type !== edits.types.output).map(n => ({
        ...n, parameters: n.parameters.map(p => ({...p, value: p.type === 7 ? Array.from(p.value) : p.value})),
        position: positions[n.id] || {x: 0, y: 0},
        outgoing: graph.edges.filter(e => e.sourceNode === n.id).map(e => ({id: e.id, target: e.destinationNode}))
    }));
    const output = graph.nodes.find(n => n.type === edits.types.output);
    snapshot.renderer = {id: output.id, position: positions[output.id] || {x: 0, y: 0}};
    fields.forEach((name, i) => {
        const p = output.parameters.find(p => p.key === String(i + 1));
        snapshot.renderer[name] = p ? p.value : defaults[i];
    });
    return response;
}

module.exports = {node, receipt};
