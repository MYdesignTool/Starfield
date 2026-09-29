// Convert canonical graph records into the panel's parameter-inspector view model.
(function (root, factory) {
    var layout = typeof module === "object" && module.exports
        ? require("./graph_layout.js") : root.StarfieldGraphLayout;
    var api = factory(layout);
    if (typeof module === "object" && module.exports) module.exports = api;
    else root.StarfieldGraphView = api;
}(typeof window !== "undefined" ? window : this, function (layout) {
    "use strict";

    var TYPE = {
        emitter: "org.starfieldfx.nodes.emitter",
        particle: "org.starfieldfx.nodes.particle",
        force: "org.starfieldfx.nodes.force",
        appearance: "org.starfieldfx.nodes.appearance",
        output: "org.starfieldfx.nodes.output"
    };
    var SPECS = {
        emitter: {
            "1": { label: "Max Particles", kind: "slider", decimals: 0, min: 0, max: 2000000, legacyKey: "particle_count" },
            "2": { label: "Particles Per Second", kind: "slider", decimals: 0, min: 0, max: 1000000, legacyKey: "birth_rate" },
            "3": { label: "Random Seed", kind: "slider", decimals: 0, min: 0, max: 2147483647, legacyKey: "seed" },
            "4": { label: "Lifetime", kind: "slider", decimals: 3, min: 0, max: 1000000, legacyKey: "particle_lifetime" },
            "5": { label: "Type", kind: "popup", decimals: 0, min: 1, max: 4, displayOffset: 1,
                  choices: ["Point", "Box", "Sphere", "Disc"], legacyKey: "emitter_shape" },
            "6": { label: "Origin", kind: "point3d", decimals: 3, min: -100, max: 100, legacyKey: "emitter_origin" },
            "7": { label: "Velocity", kind: "point3d", decimals: 2, min: -1000, max: 1000,
                  legacyKeys: ["velocity_x", "velocity_y", "velocity_z"] },
            "8": { label: "Particle Size", kind: "slider", decimals: 2, min: 0, max: 100000, legacyKey: "particle_size" },
            "9": { label: "Opacity", kind: "slider", decimals: 3, min: 0, max: 1, legacyKey: "opacity" },
            "10": { label: "Emitter Size", kind: "slider", decimals: 3, min: 0, max: 10, legacyKey: "emitter_size" },
            "11": { label: "Speed Random", kind: "slider", decimals: 2, min: 0, max: 100, legacyKey: "velocity_spread" },
            "12": { label: "Emission Speed", kind: "slider", decimals: 2, min: 0, max: 1000 },
            "13": { label: "Emission Speed Random", kind: "slider", decimals: 2, min: 0, max: 1000 },
            "14": { label: "Emission Angle X", kind: "slider", decimals: 1, min: -100000, max: 100000 },
            "15": { label: "Emission Angle Y", kind: "slider", decimals: 1, min: -100000, max: 100000 },
            "16": { label: "Emission Angle Z", kind: "slider", decimals: 1, min: -100000, max: 100000 },
            "17": { label: "Direction Mode", kind: "popup", decimals: 0, min: 1, max: 2, displayOffset: 1,
                   choices: ["Directional", "Uniform"] },
            "18": { label: "Direction Span", kind: "slider", decimals: 1, min: 0, max: 180 },
            "19": { label: "Size X", kind: "slider", decimals: 0, min: 0, max: 1000, legacyKey: "emitter_size_x" },
            "20": { label: "Size Y", kind: "slider", decimals: 0, min: 0, max: 1000, legacyKey: "emitter_size_y" },
            "21": { label: "Size Z", kind: "slider", decimals: 0, min: 0, max: 1000, legacyKey: "emitter_size_z" }
        },
        particle: {
            "1": { label: "Color Start", kind: "color", decimals: 0, min: 0, max: 255, scale: 255, legacyKey: "color_start" },
            "2": { label: "Color End", kind: "color", decimals: 0, min: 0, max: 255, scale: 255, legacyKey: "color_end" },
            "3": { label: "Size", kind: "slider", decimals: 2, min: 0, max: 100000, legacyKey: "particle_size" },
            "4": { label: "Size Over Life", kind: "slider", decimals: 2, min: 0, max: 100000, legacyKey: "particle_size_end" },
            "5": { label: "Opacity", kind: "slider", decimals: 3, min: 0, max: 1, legacyKey: "opacity" },
            "6": { label: "Opacity Over Life", kind: "slider", decimals: 3, min: 0, max: 1, legacyKey: "opacity_end" }
        },
        appearance: {
            "1": { label: "Color Start", kind: "color", decimals: 0, min: 0, max: 255, scale: 255 },
            "2": { label: "Color End", kind: "color", decimals: 0, min: 0, max: 255, scale: 255 },
            "3": { label: "Size", kind: "slider", decimals: 2, min: 0, max: 100000 },
            "4": { label: "Size Over Life", kind: "slider", decimals: 2, min: 0, max: 100000 },
            "5": { label: "Opacity", kind: "slider", decimals: 3, min: 0, max: 1 },
            "6": { label: "Opacity Over Life", kind: "slider", decimals: 3, min: 0, max: 1 }
        },
        force: {
            "1": { label: "Gravity", kind: "point3d", decimals: 2, min: -1000, max: 1000,
                  legacyKeys: ["gravity_x", "gravity_y", "gravity_z"] },
            "2": { label: "Linear Drag", kind: "slider", decimals: 3, min: 0, max: 100, legacyKey: "linear_drag" }
        },
        output: {}
    };
    var LABELS = { emitter: "Emitter", particle: "Particle", force: "Force",
                   appearance: "Appearance", output: "Output" };

    function fail(code, message) {
        var error = new Error(message);
        error.code = code;
        throw error;
    }

    function kindFor(type) {
        for (var kind in TYPE) if (Object.prototype.hasOwnProperty.call(TYPE, kind) && TYPE[kind] === type) return kind;
        return null;
    }

    function findParameter(node, key) {
        for (var i = 0; i < node.parameters.length; i++) {
            if (node.parameters[i].key === key) return node.parameters[i];
        }
        return null;
    }

    function graphValueToDisplay(value, spec) {
        if (spec.kind === "popup") return Number(value) + (spec.displayOffset || 0);
        if (spec.kind === "color") return value.map(function (channel) { return channel * spec.scale; });
        return Object.prototype.toString.call(value) === "[object Array]" ? value.slice() : value;
    }

    function viewParameter(node, kind, graphParameter, spec) {
        var parameter = {
            key: node.id + ":" + graphParameter.key,
            graphKey: graphParameter.key,
            graphType: graphParameter.type,
            graphNodeId: node.id,
            label: spec ? spec.label : "Parameter " + graphParameter.key,
            kind: spec ? spec.kind : (graphParameter.type === 5 ? "point3d" : "slider"),
            value: spec ? graphValueToDisplay(graphParameter.value, spec) : graphParameter.value,
            displayDecimals: spec ? spec.decimals : 3,
            min: spec && typeof spec.min === "number" ? spec.min : undefined,
            max: spec && typeof spec.max === "number" ? spec.max : undefined,
            choices: spec && spec.choices ? spec.choices.slice() : undefined,
            legacyKey: spec ? spec.legacyKey : undefined,
            legacyKeys: spec && spec.legacyKeys ? spec.legacyKeys.slice() : undefined,
            displayScale: spec && spec.scale ? spec.scale : 1,
            displayOffset: spec && spec.displayOffset ? spec.displayOffset : 0,
            readonly: !spec
        };
        return parameter;
    }

    function decodeCurve(value, minimum, maximum, fallbackStart, fallbackEnd) {
        var points = [{ age: 0, value: fallbackStart }, { age: 1, value: fallbackEnd }];
        if (value === null || value === undefined) return { custom: false, points: points };
        if (!(value instanceof Uint8Array) || value.length < 36) {
            fail("invalid_curve", "an active over-life curve has an invalid payload length or value type");
        }
        var view = new DataView(value.buffer, value.byteOffset, value.byteLength);
        var count = value[1];
        if (value[0] !== 1 || count < 2 || count > 8 || value[2] !== 0 || value[3] !== 0 ||
            value.length !== 4 + count * 16) fail("invalid_curve", "the graph contains a malformed over-life curve");
        points = [];
        for (var i = 0; i < count; i++) {
            var age = view.getFloat64(4 + i * 16, true);
            var ordinate = view.getFloat64(12 + i * 16, true);
            if (!isFinite(age) || !isFinite(ordinate) || age < 0 || age > 1 ||
                ordinate < minimum || ordinate > maximum || (i && age <= points[i - 1].age)) {
                fail("invalid_curve", "the graph contains an unordered or out-of-range over-life curve");
            }
            points.push({ age: age, value: ordinate });
        }
        if (points[0].age !== 0 || points[points.length - 1].age !== 1) {
            fail("invalid_curve", "over-life curves must have endpoints at 0 and 100 percent life");
        }
        return { custom: true, points: points };
    }

    function encodeCurve(points) {
        if (!points || points.length < 2 || points.length > 8 || points[0].age !== 0 ||
            points[points.length - 1].age !== 1) fail("invalid_curve", "a curve needs fixed 0 and 100 percent endpoints");
        var bytes = new Uint8Array(4 + points.length * 16);
        bytes[0] = 1;
        bytes[1] = points.length;
        var view = new DataView(bytes.buffer);
        for (var i = 0; i < points.length; i++) {
            if (!isFinite(points[i].age) || !isFinite(points[i].value) ||
                points[i].age < 0 || points[i].age > 1 || (i && points[i].age <= points[i - 1].age)) {
                fail("invalid_curve", "curve points must be finite and ordered by life percentage");
            }
            view.setFloat64(4 + i * 16, points[i].age, true);
            view.setFloat64(12 + i * 16, points[i].value, true);
        }
        return bytes;
    }

    function project(graph, layoutOverride) {
        if (!graph || Object.prototype.toString.call(graph.nodes) !== "[object Array]" ||
            Object.prototype.toString.call(graph.edges) !== "[object Array]") {
            fail("invalid_graph", "the decoded graph has no node and edge arrays");
        }
        var nodes = [];
        var positions = layoutOverride || layout.resolve(graph);
        var byId = {};
        for (var i = 0; i < graph.nodes.length; i++) {
            var source = graph.nodes[i];
            var kind = kindFor(source.type);
            if (!kind) fail("unknown_node_type", "the panel cannot display node type " + source.type);
            var node = { id: source.id, kind: kind, type: source.type, label: LABELS[kind],
                         inputPort: kind === "emitter" ? null : "1",
                         outputPort: kind === "output" ? null : (kind === "emitter" ? "1" : "2"),
                         params: [], graphParameters: source.parameters, curves: null,
                         curveParameterKeys: kind === "particle" || kind === "appearance"
                            ? { size: "7", opacity: "8", sizeStart: "3", sizeEnd: "4", opacityStart: "5", opacityEnd: "6" }
                            : null };
            byId[node.id] = node;
            var specs = SPECS[kind];
            for (var p = 0; p < source.parameters.length; p++) {
                var graphParameter = source.parameters[p];
                var spec = specs[graphParameter.key];
                if (kind === "particle" || kind === "appearance") {
                    if (graphParameter.key === "7" || graphParameter.key === "8") continue;
                }
                if (kind === "emitter" && graphParameter.key === "1") continue;
                node.params.push(viewParameter(node, kind, graphParameter, spec));
            }
            if (node.curveParameterKeys) {
                var sizeStart = findParameter(source, "3");
                var sizeEnd = findParameter(source, "4");
                var opacityStart = findParameter(source, "5");
                var opacityEnd = findParameter(source, "6");
                var sizeCurve = findParameter(source, "7");
                var opacityCurve = findParameter(source, "8");
                if ((sizeCurve && sizeCurve.type !== 7) || (opacityCurve && opacityCurve.type !== 7)) {
                    fail("invalid_curve", "over-life curve graph parameters must use the opaque value type");
                }
                node.curves = {
                    size: decodeCurve(sizeCurve && sizeCurve.value, 0, 100000,
                                      sizeStart ? sizeStart.value : 10, sizeEnd ? sizeEnd.value : 10),
                    opacity: decodeCurve(opacityCurve && opacityCurve.value, 0, 1,
                                         opacityStart ? opacityStart.value : 1, opacityEnd ? opacityEnd.value : 1)
                };
            }
            node.position = positions[node.id] || { x: 235, y: 22 + i * 100 };
            nodes.push(node);
        }
        var emitters = nodes.filter(function (node) { return node.kind === "emitter"; });
        var outputs = nodes.filter(function (node) { return node.kind === "output"; });
        if (emitters.length && outputs.length) {
            var max = findParameter(graph.nodes.filter(function (node) { return node.id === emitters[0].id; })[0], "1");
            if (max) {
                outputs[0].params.push(viewParameter(emitters[0], "emitter", max, SPECS.emitter["1"]));
                outputs[0].maxParticles = max.value;
            }
        }
        var edges = graph.edges.map(function (edge) {
            var pair = [edge.sourceNode, edge.destinationNode];
            pair.id = edge.id;
            pair.outputPort = edge.sourcePort;
            pair.inputPort = edge.destinationPort;
            return pair;
        });
        return { graph: graph, nodes: nodes, edges: edges, positions: positions };
    }

    function mapLegacyEdit(graph, edit) {
        var projected = project(graph);
        var firstByKind = {};
        for (var i = 0; i < projected.nodes.length; i++) {
            if (!firstByKind[projected.nodes[i].kind]) firstByKind[projected.nodes[i].kind] = projected.nodes[i].id;
        }
        var copy = {};
        for (var key in edit) if (Object.prototype.hasOwnProperty.call(edit, key)) copy[key] = edit[key];
        function mapId(id) {
            return firstByKind[id] || id;
        }
        if (copy.from) copy.from = mapId(copy.from);
        if (copy.to) copy.to = mapId(copy.to);
        if (copy.nodeId) copy.nodeId = mapId(copy.nodeId);
        if (copy.nodeIds) copy.nodeIds = copy.nodeIds.map(mapId);
        return copy;
    }

    return { types: TYPE, project: project, encodeCurve: encodeCurve, decodeCurve: decodeCurve,
             mapLegacyEdit: mapLegacyEdit, parameterToGraphValue: function (parameter, displayValue) {
                 if (parameter.kind === "popup") return Number(displayValue) - parameter.displayOffset;
                 if (parameter.kind === "color") return displayValue.map(function (channel) { return channel / parameter.displayScale; });
                 return Object.prototype.toString.call(displayValue) === "[object Array]" ? displayValue.slice() : displayValue;
             } };
}));
