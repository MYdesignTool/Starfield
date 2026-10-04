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
        output: "org.starfieldfx.nodes.output"
    };
    var SPECS = {
        emitter: {
            "2": { label: "Particles Per Second", kind: "slider", decimals: 0, step: 1, min: 0, max: 1000000, legacyKey: "birth_rate" },
            "3": { label: "Random Seed", kind: "slider", decimals: 0, step: 1, min: 0, max: 2147483647, legacyKey: "seed" },
            "5": { label: "Type", kind: "popup", decimals: 0, step: 1, min: 1, max: 4, displayOffset: 1,
                  choices: ["Point", "Box", "Sphere", "Disc"], legacyKey: "emitter_shape" },
            "6": { label: "Origin", kind: "point3d", decimals: 3, min: -100, max: 100, legacyKey: "emitter_origin" },
            "7": { label: "Velocity", hidden: true, kind: "point3d", decimals: 2, min: -1000, max: 1000,
                  legacyKeys: ["velocity_x", "velocity_y", "velocity_z"] },
            "8": { label: "Particle Size", hidden: true, kind: "slider", decimals: 2, min: 0, max: 100000, unit: "px", legacyKey: "particle_size" },
            "9": { label: "Opacity", hidden: true, kind: "slider", decimals: 1, step: 1, min: 0, max: 100, scale: 100, unit: "%", legacyKey: "opacity" },
            "10": { label: "Disc Size", kind: "slider", decimals: 2, step: 0.01, min: 0, max: 10, legacyKey: "emitter_size" },
            "11": { label: "Velocity Random", hidden: true, kind: "slider", decimals: 2, min: 0, max: 100, legacyKey: "velocity_spread" },
            "12": { label: "Speed", kind: "slider", decimals: 1, step: 1, min: 0, max: 10000, unit: "px/s" },
            "13": { label: "Speed Amplitude", hidden: true, kind: "slider", decimals: 3, min: 0, max: 100000 },
            "22": { label: "Speed Random", kind: "slider", decimals: 1, step: 1, min: 0, max: 100, unit: "%" },
            "14": { label: "Angle X", kind: "slider", decimals: 1, step: 0.1, min: -32768, max: 32767.99998, unit: "°" },
            "15": { label: "Angle Y", kind: "slider", decimals: 1, step: 0.1, min: -32768, max: 32767.99998, unit: "°" },
            "16": { label: "Angle Z", kind: "slider", decimals: 1, step: 0.1, min: -32768, max: 32767.99998, unit: "°" },
            "17": { label: "Direction", kind: "popup", decimals: 0, step: 1, min: 1, max: 2, displayOffset: 1,
                   choices: ["Directional", "Uniform"] },
            "18": { label: "Direction Span", kind: "slider", decimals: 1, step: 1, min: 0, max: 180 },
            "19": { label: "Size X", kind: "slider", decimals: 0, step: 1, min: 0, max: 100000, unit: "px", legacyKey: "emitter_size_x" },
            "20": { label: "Size Y", kind: "slider", decimals: 0, step: 1, min: 0, max: 100000, unit: "px", legacyKey: "emitter_size_y" },
            "21": { label: "Size Z", kind: "slider", decimals: 0, step: 1, min: 0, max: 100000, unit: "px", legacyKey: "emitter_size_z" },
            "23": { label:"Emitting",kind:"popup",decimals:0,step:1,min:1,max:4,displayOffset:1,choices:["Default","Once","Sequenced","Randomized"] },
            "24": {label:"Emit Chance",kind:"slider",decimals:1,step:1,min:0,max:100,unit:"%"},
            "25": {label:"Emit Life Start",kind:"slider",decimals:1,step:1,min:0,max:100,unit:"%"},
            "26": {label:"Emit Life End",kind:"slider",decimals:1,step:1,min:0,max:100,unit:"%"},
            "27": {label:"Inherit Velocity",kind:"slider",decimals:1,step:1,min:0,max:100,unit:"%"},
            "28": {label:"Inherit Size",kind:"slider",decimals:1,step:1,min:0,max:100,unit:"%"},
            "29": {label:"Inherit Opacity",kind:"slider",decimals:1,step:1,min:0,max:100,unit:"%"},
            "30": {label:"Inherit Color",kind:"slider",decimals:1,step:1,min:0,max:100,unit:"%"},
            "31": {hidden:true},
            "32": {label:"Orient",kind:"point3d",decimals:1,step:.1,min:-32768,max:32767.99998,unit:"°"}
        },
        particle: {
            "1": { label: "Color", kind: "color", decimals: 0, step: 1, min: 0, max: 255, scale: 255, legacyKey: "color_start" },
            "2": { hidden: true },
            "12": {label:"Particle Color",kind:"popup",decimals:0,step:1,min:1,max:4,displayOffset:1,
                choices:["Solid color","Color over life","Random from gradient","Loop from grad"]},
            "13": {hidden:true},
            "3": { label: "Size (Pixels)", kind: "slider", decimals: 1, step: 1, min: 0, max: 100000, unit: "px", legacyKey: "particle_size" },
            "4": { label: "Size Over Life", kind: "slider", decimals: 1, step: 1, min: 0, max: 100, unit: "%", legacyKey: "particle_size_end" },
            "5": { label: "Opacity", kind: "slider", decimals: 1, step: 1, min: 0, max: 100, scale: 100, unit: "%", legacyKey: "opacity" },
            "6": { label: "Opacity Over Life", kind: "slider", decimals: 1, step: 1, min: 0, max: 100, unit: "%", legacyKey: "opacity_end" },
            "9": { label: "Size Random", kind: "slider", decimals: 0, step: 1, min: 0, max: 100, legacyKey: "particle_size_random" },
            "10": { label: "Opacity Random", kind: "slider", decimals: 0, step: 1, min: 0, max: 100, legacyKey: "opacity_random" },
            "11": { label: "Life (Seconds)", kind: "slider", decimals: 1, min: 0, max: 10000, step: 0.1, legacyKey: "particle_lifetime" },
            "14": {label:"Life Random",kind:"slider",decimals:1,step:1,min:0,max:100,unit:"%"},
            "15": {label:"Shape",kind:"popup",min:1,max:3,displayOffset:1,choices:["Circle","Rectangle","Cloud"]},
            "16": {label:"Size Y (Pixels)",kind:"slider",decimals:1,step:1,min:0,max:100000,unit:"px"},
            "17": {label:"Orient To",kind:"popup",min:1,max:3,displayOffset:1,choices:["Nothing","Motion(particle)","Emitter"]},
            "18": {label:"Angle",kind:"point3d",decimals:1,step:.1,min:-32768,max:32767.99998,unit:"°"},
            "19": {label:"Angle Random",kind:"slider",decimals:1,step:1,min:0,max:100,unit:"%"},
            "20": {label:"Speed",kind:"point3d",decimals:1,step:.1,min:-32768,max:32767.99998,unit:"°/s"},
            "21": {label:"Rotation Speed Random",kind:"slider",decimals:1,step:1,min:0,max:100,unit:"%"},
            "22": {label:"Limit To 2D",kind:"popup",min:1,max:2,displayOffset:1,choices:["Off","On"]},
            "23": {label:"Particle Feather",kind:"slider",decimals:1,step:1,min:0,max:100,unit:"%"},
            "24": {label:"Up Axis",kind:"popup",min:1,max:3,displayOffset:1,choices:["X","Y","Z"]},
            "25": {label:"Random Limit",kind:"popup",min:1,max:5,displayOffset:1,choices:["None","All Axis","X","Y","Z"]},
            "26": {label:"Limit Angle",kind:"slider",decimals:1,step:.1,min:-32768,max:32767.99998,unit:"°"},
            "27": {hidden:true},
            "28": {label:"Anchor X (Percent)",kind:"slider",decimals:1,step:.1,min:0,max:100,unit:"%"},
            "29": {label:"Anchor Y (Percent)",kind:"slider",decimals:1,step:.1,min:0,max:100,unit:"%"}
        },
        force: {
            "1": { label: "Gravity", kind: "slider", decimals: 1, step: 1, min: -100000, max: 100000 },
            "2": { label: "Air Density", kind: "slider", decimals: 2, step: 0.01, min: 0, max: 100 },
            "3": { label: "Gravity random", kind: "slider", decimals: 1, step: 1, min: 0, max: 100, unit:"%" },
            "4": { label: "Wind", kind: "point3d", decimals: 1, step: 1, min: -100000, max: 100000 },
            "5": { label: "Spin", kind: "slider", decimals: 1, step: 1, min: 0, max: 100000, unit:"px" },
            "6": { label: "Spin Frequency", kind: "slider", decimals: 2, step: 0.01, min: 0, max: 1000 },
            "7": { label: "Spin resist", kind: "slider", decimals: 1, step: 1, min: 0, max: 100, unit:"%" },
            "8": { label: "Spin Delay (Seconds)", kind: "slider", decimals: 1, step: 0.1, min: 0, max: 10000 },
            "9": { hidden:true }
        },
        output: {
            "1": { label: "Max Particles", kind: "slider", decimals: 0, step: 1, min: 0, max: 2000000, legacyKey: "particle_count" },
            "2": {label:"Time Remapping On / Off",kind:"popup",choices:["Off","On"],values:[0,1]},
            "3": {label:"Time (Seconds)",kind:"slider",decimals:2,step:0.1,min:-1000000,max:1000000},
            "4": {label:"Preview",kind:"popup",choices:["Off","On"],values:[0,1]},
            "5": {label:"Particle chance",kind:"slider",decimals:1,step:1,min:0,max:100,unit:"%"},
            "6": {label:"Acceleration",kind:"popup",choices:["GPU","CPU"],values:[0,1]},
            "7": {label:"Time Sampling",kind:"popup",choices:["30 Hz","60 Hz","120 Hz"],values:[30,60,120]}
        }
    };
    var LABELS = { emitter: "Emitter", particle: "Particle", force: "Force",
                   output: "Output" };

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

    function activeEmitterParameters(graph) {
        if (!graph || Object.prototype.toString.call(graph.nodes) !== "[object Array]" ||
            Object.prototype.toString.call(graph.edges) !== "[object Array]") return null;
        var byId = {};
        var incoming = {};
        var outputIds = [];
        for (var i = 0; i < graph.nodes.length; i++) {
            var node = graph.nodes[i];
            if (!node || typeof node.id !== "string" || !node.id) return null;
            byId["$" + node.id] = node;
            if (kindFor(node.type) === "output") outputIds.push(node.id);
        }
        if (outputIds.length !== 1) return null;
        for (var e = 0; e < graph.edges.length; e++) {
            var edge = graph.edges[e];
            if (!edge || !byId["$" + edge.sourceNode] || !byId["$" + edge.destinationNode]) return null;
            var destinationKey = "$" + edge.destinationNode;
            if (!incoming[destinationKey]) incoming[destinationKey] = [];
            incoming[destinationKey].push(edge.sourceNode);
        }
        var active = {};
        var pending = [outputIds[0]];
        active["$" + outputIds[0]] = true;
        while (pending.length) {
            var current = pending.pop();
            var sources = incoming["$" + current] || [];
            for (var s = 0; s < sources.length; s++) {
                var key = "$" + sources[s];
                if (!active[key]) { active[key] = true; pending.push(sources[s]); }
            }
        }
        var emitters = [];
        var particles = [];
        for (var n = 0; n < graph.nodes.length; n++) {
            if (kindFor(graph.nodes[n].type) === "emitter" && active["$" + graph.nodes[n].id]) {
                var emitting = findParameter(graph.nodes[n], "31");
                if (emitting && Number(emitting.value) === 1) return null; // sampled parent populations require native evaluation
                emitters.push(graph.nodes[n]);
            } else if (kindFor(graph.nodes[n].type) === "particle" && active["$" + graph.nodes[n].id]) {
                particles.push(graph.nodes[n]);
            }
        }
        var output = byId["$" + outputIds[0]];
        var capParameter = findParameter(output, "1");
        if (!capParameter) return null;
        var cap = Number(capParameter.value);
        if (!isFinite(cap) || Math.floor(cap) !== cap || cap < 0 || cap > 2000000) return null;
        particles.sort(function (left, right) { return left.id < right.id ? -1 : left.id > right.id ? 1 : 0; });
        var streams = [];
        for (var emitterIndex = 0; emitterIndex < emitters.length; emitterIndex++) {
            var emitter = emitters[emitterIndex], rateParameter = findParameter(emitter, "2");
            if (!rateParameter) return null;
            var rate = Number(rateParameter.value);
            if (!isFinite(rate) || rate < 0 || rate > 1000000) return null;
            var branchLifetimes = [], lifetime = 0;
            for (var particleIndex = 0; particleIndex < particles.length; particleIndex++) {
                var particle = particles[particleIndex];
                if ((incoming["$" + particle.id] || []).indexOf(emitter.id) < 0) continue;
                var lifeParameter = findParameter(particle, "11");
                if (!lifeParameter) return null;
                var life = Number(lifeParameter.value);
                if (!isFinite(life) || life < 0 || life > 10000) return null;
                branchLifetimes.push(life);
                lifetime = Math.max(lifetime, life);
            }
            if (branchLifetimes.length) streams.push({ emitterId: emitter.id, birthRate: rate,
                lifetimeSeconds: lifetime, branchLifetimes: branchLifetimes });
        }
        return { maxParticles: cap, emitters: streams };
    }

    function countLiveParticles(timeSeconds, birthRate, lifetimeSeconds, populationCap, branchLifetimes) {
        var time = Number(timeSeconds), rate = Number(birthRate), lifetime = Number(lifetimeSeconds);
        var cap = Math.floor(Number(populationCap));
        if (!isFinite(time) || !isFinite(rate) || !isFinite(lifetime) || !isFinite(cap) ||
            time < 0 || rate <= 0 || lifetime <= 0 || cap <= 0) return 0;
        var lastSlot = Math.floor(time * rate);
        if (!isFinite(lastSlot) || lastSlot > 9007199254740992) return 0;
        if (!branchLifetimes || !branchLifetimes.length) branchLifetimes = [lifetime];
        var count = 0;
        for (var branch = 0; branch < branchLifetimes.length; branch++) {
            var branchLife = Number(branchLifetimes[branch]);
            if (!isFinite(branchLife) || branchLife <= 0) continue;
            var firstSlot = Math.max(0, Math.floor((time - branchLife) * rate) + 1);
            var offset = (branch - (firstSlot % branchLifetimes.length) + branchLifetimes.length) % branchLifetimes.length;
            var firstAssigned = firstSlot + offset;
            if (firstAssigned <= lastSlot) count += Math.floor((lastSlot - firstAssigned) / branchLifetimes.length) + 1;
            if (count >= cap) return cap;
        }
        return count;
    }

    function countGraphLiveParticles(timeSeconds, emission) {
        if (!emission) return 0;
        var count = 0;
        for (var i = 0; i < emission.emitters.length; i++) {
            var stream = emission.emitters[i];
            count += countLiveParticles(timeSeconds, stream.birthRate, stream.lifetimeSeconds,
                                        emission.maxParticles, stream.branchLifetimes);
            if (count >= emission.maxParticles) return emission.maxParticles;
        }
        return count;
    }

    function graphValueToDisplay(value, spec) {
        if (spec.kind === "popup") return spec.values ? spec.values.indexOf(Number(value))+1 : Number(value) + (spec.displayOffset || 0);
        if (spec.kind === "color") return value.map(function (channel) { return channel * spec.scale; });
        return Object.prototype.toString.call(value) === "[object Array]" ? value.slice() : Number(value) * (spec.scale || 1);
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
            scrubStep: spec && spec.step ? spec.step : undefined,
            min: spec && typeof spec.min === "number" ? spec.min : undefined,
            max: spec && typeof spec.max === "number" ? spec.max : undefined,
            choices: spec && spec.choices ? spec.choices.slice() : undefined,
            enumValues: spec && spec.values ? spec.values.slice() : undefined,
            unit: spec && spec.unit ? spec.unit : undefined,
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

    function decodeGradient(value) {
        if(value===undefined || value===null) return [{position:0,color:[1,1,1]},{position:1,color:[1,1,1]}];
        if(!(value instanceof Uint8Array) || value.length<68 || value[0]!==1 || value[2]!==0 || value[3]!==0 ||
            value[1]<2 || value[1]>8 || value.length!==4+32*value[1]) fail("invalid_gradient","Malformed Color Gradient.");
        var view=new DataView(value.buffer,value.byteOffset,value.byteLength),stops=[];
        for(var i=0;i<value[1];i++) {
            var position=view.getFloat64(4+32*i,true),color=[];
            if(!isFinite(position) || position<0 || position>1 || (i && position<=stops[i-1].position)) fail("invalid_gradient","Unordered Color Gradient.");
            for(var c=0;c<3;c++) {
                var channel=view.getFloat64(12+32*i+8*c,true);
                if(!isFinite(channel)||channel<0||channel>64) fail("invalid_gradient","Invalid Color Gradient channel.");
                color.push(channel);
            }
            stops.push({position:position,color:color});
        }
        return stops;
    }
    function encodeGradient(stops) {
        if(!stops || stops.length<2 || stops.length>8) fail("invalid_gradient","A gradient requires 2–8 stops.");
        var bytes=new Uint8Array(4+32*stops.length),view=new DataView(bytes.buffer);
        bytes[0]=1;bytes[1]=stops.length;
        for(var i=0;i<stops.length;i++) {
            view.setFloat64(4+32*i,stops[i].position,true);
            for(var c=0;c<3;c++) view.setFloat64(12+32*i+8*c,stops[i].color[c],true);
        }
        decodeGradient(bytes);return bytes;
    }

    function project(graph, layoutOverride, geometry) {
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
            var emitting = findParameter(source,"31");
            var isAuxiliary = kind === "emitter" && emitting && Number(emitting.value) === 1;
            var shape = findParameter(source,"5");
            var node = { id: source.id, kind: kind, type: source.type, label: isAuxiliary ? "Auxiliary" : LABELS[kind],
                         inputPort: kind === "emitter" ? (isAuxiliary ? "2" : null) : "1",
                         outputPort: kind === "output" ? null : (kind === "emitter" ? "1" : "2"),
                         params: [], graphParameters: source.parameters, curves: null,
                         curveParameterKeys: kind === "particle"
                            ? { size: "7", opacity: "8", rotation:"27", sizeStart: "3", sizeEnd: "4", opacityStart: "5", opacityEnd: "6" }
                            : null };
            byId[node.id] = node;
            if(kind==="particle") {
                var colorMode=findParameter(source,"12"),gradient=findParameter(source,"13");
                node.colorMode=colorMode?Number(colorMode.value):0;
                node.gradient=decodeGradient(gradient?gradient.value:null);
            }
            var specs = SPECS[kind];
            for (var p = 0; p < source.parameters.length; p++) {
                var graphParameter = source.parameters[p];
                var spec = specs[graphParameter.key];
                if (kind === "particle") {
                    if (graphParameter.key === "7" || graphParameter.key === "8") continue;
                }
                if (kind === "emitter" && graphParameter.key === "1") continue;
                if (spec && spec.hidden) continue;
                if(kind==="particle" && graphParameter.key==="1" && node.colorMode!==0) continue;
                if (kind === "emitter") {
                    if (Number(graphParameter.key) >= 24 && Number(graphParameter.key) <= 30 && !isAuxiliary) continue;
                    if (["19","20","21"].indexOf(graphParameter.key) >= 0 && (!shape || [1,2].indexOf(Number(shape.value)) < 0)) continue;
                    if (graphParameter.key === "10" && (!shape || Number(shape.value) !== 3)) continue;
                }
                var parameter = viewParameter(node, kind, graphParameter, spec);
                if(kind==="particle" && ["18","20"].indexOf(graphParameter.key)>=0) {
                    [0,1,2].forEach(function(axis) {
                        var component=viewParameter(node,kind,graphParameter,spec);
                        component.kind="slider";component.key+=":"+axis;
                        component.label=spec.label+" "+["X","Y","Z"][axis];
                        component.forceComponent=axis;component.canonicalForce=graphParameter.value.slice();
                        component.displayScale=1;component.value=graphParameter.value[axis];
                        node.params.push(component);
                    });
                    continue;
                }
                if (kind === "force" && (graphParameter.key === "1" || graphParameter.key === "4")) {
                    var forceHeight = geometry && Number(geometry.height) > 0 ? Number(geometry.height) : 1;
                    var forceAspect=geometry && Number(geometry.pixelAspect)>0 ? Number(geometry.pixelAspect) : 1;
                    var axes = graphParameter.key === "1" ? [1] : [0,1,2];
                    axes.forEach(function(axis) {
                        var component = viewParameter(node,kind,graphParameter,spec);
                        component.kind="slider"; component.key+=":"+axis;
                        component.label=graphParameter.key === "1" ? "Gravity" : "Wind "+["X","Y","Z"][axis];
                        component.forceComponent=axis; component.canonicalForce=graphParameter.value.slice();
                        component.displayScale=forceHeight*(axis===0 ? 1/forceAspect : axis===1 ? -1 : 1);
                        component.value=graphParameter.value[axis]*component.displayScale;
                        node.params.push(component);
                    });
                    continue;
                }
                if (kind === "force" && graphParameter.key === "5" && geometry && Number(geometry.height)>0) {
                    parameter.value *= Number(geometry.height); parameter.displayScale=Number(geometry.height);
                }
                if (kind === "emitter" && geometry && Number(geometry.height) > 0 && Number(geometry.width) > 0) {
                    var height = Number(geometry.height), width = Number(geometry.width);
                    var aspect = Number(geometry.pixelAspect);
                    if (!isFinite(aspect) || aspect <= 0) aspect = 1;
                    if (graphParameter.key === "6") {
                        var origin = graphParameter.value;
                        parameter.kind = "point2d"; parameter.label = "Origin XY";
                        parameter.key += ":xy"; parameter.originComponent = "xy";
                        parameter.canonicalOrigin = origin.slice(); parameter.geometry = geometry;
                        parameter.displayDecimals = 0; parameter.scrubStep = 1;
                        parameter.channelMin = [width / 2 - 100 * height / aspect, height / 2 - 100 * height];
                        parameter.channelMax = [width / 2 + 100 * height / aspect, height / 2 + 100 * height];
                        parameter.min = Math.min(parameter.channelMin[0], parameter.channelMin[1]);
                        parameter.max = Math.max(parameter.channelMax[0], parameter.channelMax[1]);
                        parameter.value = [width / 2 + origin[0] * height / aspect, height / 2 - origin[1] * height];
                        node.params.push(parameter);
                        var zParameter = viewParameter(node, kind, graphParameter, spec);
                        zParameter.kind = "slider"; zParameter.label = "Origin Z";
                        zParameter.key += ":z"; zParameter.originComponent = "z";
                        zParameter.canonicalOrigin = origin.slice(); zParameter.geometry = geometry;
                        zParameter.displayDecimals = 0; zParameter.scrubStep = 1;
                        zParameter.min = -100 * height; zParameter.max = 100 * height;
                        zParameter.value = origin[2] * height; zParameter.unit = "px";
                        node.params.push(zParameter);
                        continue;
                    }
                    if (graphParameter.key === "12") {
                        parameter.value = Number(graphParameter.value) * height;
                        parameter.displayScale = height;
                    }
                }
                node.params.push(parameter);
            }
            if (kind === "particle") {
                ["9", "10"].forEach(function (key) {
                    if (!findParameter(source, key)) {
                        node.params.push(viewParameter(node, kind,
                            { key: key, type: 4, value: 0 }, specs[key]));
                    }
                });
            }
            if (kind === "emitter") {
                var emitterOrder = ["5","23","2","6","12","22","14","15","16","17","32","18","19","20","21","10","24","25","26","27","28","29","30","3"];
                node.params.sort(function (a,b) { return emitterOrder.indexOf(a.graphKey)-emitterOrder.indexOf(b.graphKey); });
            }
            if (kind === "particle") {
                var particleOrder = {"15":0,"11":1,"14":2,"3":3,"16":4,"9":5,"5":6,"10":7,
                    "12":8,"1":9,"2":10,"23":11,"24":12,"4":13,"6":14,"17":15,"18":16,"19":17,"25":18,"26":19,"20":20,"21":21,"28":22,"29":23,"22":24};
                node.params.sort(function (left, right) {
                    return (particleOrder[left.graphKey] || 0) - (particleOrder[right.graphKey] || 0);
                });
            }
            if (kind === "force") {
                var forceOrder=["1","3","4","5","6","7","8","2"];
                node.params.sort(function(a,b) { return forceOrder.indexOf(a.graphKey)-forceOrder.indexOf(b.graphKey); });
                var forceCurve=findParameter(source,"9");
                if (forceCurve && forceCurve.type!==7) fail("invalid_curve","Force curve must use opaque bytes");
                node.curveParameterKeys={size:"9"};
                node.curves={size:decodeCurve(forceCurve && forceCurve.value,0,100,100,100)};
            }
            if (node.curveParameterKeys && kind !== "force") {
                var sizeEnd = findParameter(source, "4");
                var opacityEnd = findParameter(source, "6");
                var sizeCurve = findParameter(source, "7");
                var opacityCurve = findParameter(source, "8");
                if ((sizeCurve && sizeCurve.type !== 7) || (opacityCurve && opacityCurve.type !== 7)) {
                    fail("invalid_curve", "over-life curve graph parameters must use the opaque value type");
                }
                node.curves = {
                    size: decodeCurve(sizeCurve && sizeCurve.value, 0, 100,
                                      100, sizeEnd ? sizeEnd.value : 100),
                    opacity: decodeCurve(opacityCurve && opacityCurve.value, 0, 100,
                                         100, opacityEnd ? opacityEnd.value : 100),
                    rotation: decodeCurve(findParameter(source,"27") && findParameter(source,"27").value,-32768,32768,0,0)
                };
            }
            node.position = positions[node.id] || { x: 235, y: 22 + i * 100 };
            nodes.push(node);
        }
        var outputs = nodes.filter(function (node) { return node.kind === "output"; });
        if (outputs.length) {
            var maxParticles = findParameter(graph.nodes.filter(function (node) {
                return node.id === outputs[0].id;
            })[0], "1");
            if (maxParticles) outputs[0].maxParticles = maxParticles.value;
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

    function samePositions(left, right) {
        if (!left || !right || Object.prototype.toString.call(left) !== "[object Object]" ||
            Object.prototype.toString.call(right) !== "[object Object]") return false;
        var leftIds = Object.keys(left);
        var rightIds = Object.keys(right);
        if (leftIds.length !== rightIds.length) return false;
        for (var i = 0; i < leftIds.length; i++) {
            var id = leftIds[i];
            var a = left[id];
            var b = right[id];
            if (!Object.prototype.hasOwnProperty.call(right, id) || !a || !b ||
                a.x !== b.x || a.y !== b.y) return false;
        }
        return true;
    }

    function graphSnapshotChanged(previous, next) {
        return !!next && (!previous || previous.revision !== next.revision ||
                          previous.graphHex !== next.graphHex);
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

    return { types: TYPE, project: project, samePositions: samePositions,
             graphSnapshotChanged: graphSnapshotChanged,
             activeEmitterParameters: activeEmitterParameters,
             countLiveParticles: countLiveParticles, countGraphLiveParticles: countGraphLiveParticles,
             encodeCurve: encodeCurve, decodeCurve: decodeCurve,
             encodeGradient:encodeGradient,decodeGradient:decodeGradient,
             mapLegacyEdit: mapLegacyEdit, parameterToGraphValue: function (parameter, displayValue) {
                 if (typeof parameter.forceComponent === "number") {
                     var force=parameter.canonicalForce.slice();
                     force[parameter.forceComponent]=Number(displayValue)/parameter.displayScale;
                     return force;
                 }
                 if (parameter.originComponent) {
                     var origin = parameter.canonicalOrigin.slice(), geometry = parameter.geometry;
                     var height = Number(geometry.height), aspect = Number(geometry.pixelAspect);
                     if (!isFinite(aspect) || aspect <= 0) aspect = 1;
                     if (parameter.originComponent === "xy") {
                         origin[0] = (Number(displayValue[0]) - Number(geometry.width) / 2) * aspect / height;
                         origin[1] = 0.5 - Number(displayValue[1]) / height;
                     } else origin[2] = Number(displayValue) / height;
                     return origin;
                 }
                 if (parameter.kind === "popup") return parameter.enumValues ? parameter.enumValues[Number(displayValue)-1] : Number(displayValue) - parameter.displayOffset;
                 if (parameter.kind === "color") return displayValue.map(function (channel) { return channel / parameter.displayScale; });
                 return Object.prototype.toString.call(displayValue) === "[object Array]" ? displayValue.slice() : Number(displayValue) / parameter.displayScale;
             } };
}));
