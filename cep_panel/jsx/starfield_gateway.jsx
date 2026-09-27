// ExtendScript gateway for the Starfield node editor panel (ADR 0009 protocol v1).
//
// Rules this file must keep:
//   - Only the public AE scripting DOM is used. No sockets, no helper processes,
//     no direct writes to the effect's arbitrary-data graph parameter.
//   - Every request is parsed, version-checked and fully validated before any
//     mutation; a failure rejects the whole change set.
//   - Values written here are the effect's ordinary supervised parameters. The
//     effect turns them into its canonical graph during PF_Cmd_USER_CHANGED_PARAM
//     (Node Graph mode) or renders them directly (AE Controls mode).
//
// Host qualification status: see docs/compatibility-matrix.md. Name-based property
// resolution and scripted supervision are AE 2023 gates, not yet verified.

(function () {
    var PROTOCOL = "org.starfieldfx.panel";
    var VERSION = 1;
    var MATCH_NAME = "org.starfieldfx.particle";
    var MAX_CHANGES = 32;
    var MAX_REQUEST_BYTES = 65536;

    // One row per bound effect parameter. `index` is the registered parameter index
    // (schema/parameters.json); `name` is the Effect Controls label used for
    // resolution. `min`/`max` mirror the manifest bounds so the panel cannot send a
    // value the effect would have to clamp.
    var BINDINGS = [
        { key: "particle_count", index: 27, name: "Max Particles", kind: "slider", min: 0, max: 2000000 },
        { key: "birth_rate", index: 3, name: "Particles Per Second", kind: "slider", min: 0, max: 1000000 },
        { key: "seed", index: 28, name: "Random Seed", kind: "slider", min: 0, max: 2147483647 },
        { key: "particle_lifetime", index: 12, name: "Lifetime", kind: "slider", min: 0, max: 1000000 },
        { key: "emitter_shape", index: 2, name: "Type", kind: "popup" },
        { key: "emitter_origin", index: 4, name: "Origin", kind: "point3d" },
        { key: "velocity_x", index: 6, name: "Velocity X", kind: "slider", min: -1000, max: 1000 },
        { key: "velocity_y", index: 7, name: "Velocity Y", kind: "slider", min: -1000, max: 1000 },
        { key: "velocity_z", index: 8, name: "Velocity Z", kind: "slider", min: -1000, max: 1000 },
        { key: "particle_size", index: 13, name: "Size", kind: "slider", min: 0, max: 100000 },
        { key: "opacity", index: 15, name: "Opacity", kind: "slider", min: 0, max: 1 },
        { key: "emitter_size", index: 5, name: "Emitter Size", kind: "slider", min: 0, max: 10 },
        { key: "velocity_spread", index: 9, name: "Speed Random", kind: "slider", min: 0, max: 100 },
        { key: "gravity_x", index: 21, name: "Gravity X", kind: "slider", min: -1000, max: 1000 },
        { key: "gravity_y", index: 22, name: "Gravity Y", kind: "slider", min: -1000, max: 1000 },
        { key: "gravity_z", index: 23, name: "Gravity Z", kind: "slider", min: -1000, max: 1000 },
        { key: "linear_drag", index: 24, name: "Linear Drag", kind: "slider", min: 0, max: 100 },
        { key: "color_start", index: 17, name: "Color Start", kind: "color" },
        { key: "color_end", index: 18, name: "Color End", kind: "color" },
        { key: "particle_size_end", index: 14, name: "Size Over Life", kind: "slider", min: 0, max: 100000 },
        { key: "opacity_end", index: 16, name: "Opacity Over Life", kind: "slider", min: 0, max: 1 }
    ];

    // The Alpha chain is fixed in protocol v1: display order, not a hidden graph.
    var CHAIN = [
        { id: "emitter", label: "Emitter", keys: ["particle_count", "birth_rate", "seed", "particle_lifetime",
                                                  "emitter_shape", "emitter_origin", "velocity_x", "velocity_y",
                                                  "velocity_z", "emitter_size", "velocity_spread"] },
        { id: "force", label: "Force", keys: ["gravity_x", "gravity_y", "gravity_z", "linear_drag"] },
        { id: "appearance", label: "Appearance", keys: ["particle_size", "particle_size_end", "opacity",
                                                        "opacity_end", "color_start", "color_end"] },
        { id: "output", label: "Output", keys: [] }
    ];
    var EDGES = [["emitter", "force"], ["force", "appearance"], ["appearance", "output"]];

    function bindingFor(key) {
        for (var i = 0; i < BINDINGS.length; i++) {
            if (BINDINGS[i].key === key) return BINDINGS[i];
        }
        return null;
    }

    function reply(payload) {
        payload.protocol = PROTOCOL;
        payload.version = VERSION;
        return JSON.stringify(payload);
    }

    function fail(code, message, extra) {
        var payload = { ok: false, error: { code: code, message: String(message) } };
        if (extra) payload.context = extra;
        return reply(payload);
    }

    function parseRequest(text) {
        if (typeof text !== "string" || text.length === 0 || text.length > MAX_REQUEST_BYTES) return null;
        var request = null;
        try { request = JSON.parse(text); } catch (error) { return null; }
        if (!request || request.protocol !== PROTOCOL || request.version !== VERSION) return null;
        if (typeof request.operation !== "string") return null;
        return request;
    }

    // Resolves the single Starfield effect the panel may touch. Ambiguity is an
    // error, never a guess: exactly one selected layer with exactly one instance.
    function findTarget() {
        if (!app.project) return { error: { code: "no_project", message: "No project is open." } };
        var comp = app.project.activeItem;
        if (!comp || !(comp instanceof CompItem)) {
            return { error: { code: "no_active_comp", message: "Open a composition and select its layer." } };
        }
        var layer = null;
        var count = 0;
        for (var i = 1; i <= comp.numLayers; i++) {
            var candidate = comp.layer(i);
            if (!candidate.selected) continue;
            if (candidate.matchName !== "ADBE Text Layer" && effectCount(candidate) > 0) {
                layer = candidate;
                count++;
            }
        }
        if (count === 0) return { error: { code: "no_target", message: "Select a layer carrying Starfield Particle." } };
        if (count > 1) return { error: { code: "ambiguous_target", message: "Select exactly one layer." } };
        var effect = findEffect(layer);
        if (!effect) return { error: { code: "no_effect", message: "The selected layer has no Starfield Particle effect." } };
        return { comp: comp, layer: layer, effect: effect };
    }

    function effectCount(layer) {
        if (!layer) return 0;
        var parade = layer.property("ADBE Effect Parade");
        if (!parade) return 0;
        var total = 0;
        for (var i = 1; i <= parade.numProperties; i++) {
            if (parade.property(i) && parade.property(i).matchName === MATCH_NAME) total++;
        }
        return total;
    }

    function findEffect(layer) {
        var parade = layer.property("ADBE Effect Parade");
        for (var i = 1; i <= parade.numProperties; i++) {
            var effect = parade.property(i);
            if (effect && effect.matchName === MATCH_NAME) return effect;
        }
        return null;
    }

    // Resolves a parameter by its registered name, and records whether the fallback
    // index path was used. AE 2023 name resolution is a qualification gate; the
    // resolution string is reported so the host pass can confirm which path ran.
    function resolveProperty(effect, binding, report) {
        var property = effect.property(binding.name);
        if (property) return property;
        property = effect.property(binding.index);
        if (property && property.name === binding.name) {
            if (report && report.resolution === "name") report.resolution = "index";
            return property;
        }
        return null;
    }

    function readValue(property, binding) {
        var value = property.value;
        if (binding.kind === "point3d") return [Number(value[0]), Number(value[1]), Number(value[2])];
        if (binding.kind === "color") {
            // Scripting colors are 0..1 floats; the panel shows 0..255.
            return [Math.round(Number(value[0]) * 255), Math.round(Number(value[1]) * 255),
                    Math.round(Number(value[2]) * 255)];
        }
        return Number(value);
    }

    function validateValue(binding, value) {
        if (binding.kind === "point3d") {
            if (!(value instanceof Array) || value.length !== 3) return false;
            for (var i = 0; i < 3; i++) if (typeof value[i] !== "number" || !isFinite(value[i])) return false;
            return true;
        }
        if (binding.kind === "color") {
            if (!(value instanceof Array) || value.length !== 3) return false;
            for (var c = 0; c < 3; c++) {
                if (typeof value[c] !== "number" || !isFinite(value[c]) || value[c] < 0 || value[c] > 255) return false;
            }
            return true;
        }
        if (typeof value !== "number" || !isFinite(value)) return false;
        if (binding.kind === "popup") return value >= 1 && value <= 4;
        return value >= binding.min && value <= binding.max;
    }

    function currentRevision(effect, report) {
        // Cheap opaque token over every bound value: identifies the edit base without
        // exposing host pointers. Not a hash of the arbitrary graph.
        var text = "";
        for (var i = 0; i < BINDINGS.length; i++) {
            var property = resolveProperty(effect, BINDINGS[i], report);
            if (!property) return null;
            text += BINDINGS[i].key + "=" + String(readValue(property, BINDINGS[i])) + ";";
        }
        var hash = 5381;
        for (var c = 0; c < text.length; c++) hash = ((hash * 33) ^ text.charCodeAt(c)) & 0xffffffff;
        return (hash >>> 0).toString(16);
    }

    function controlSource(effect) {
        var property = effect.property("Control Source");
        if (!property) return "unknown";
        return Number(property.value) === 2 ? "Node Graph" : "AE Controls";
    }

    function getState(request) {
        var target = findTarget();
        if (target.error) return fail(target.error.code, target.error.message);
        var report = { resolution: "name" };
        var revision = currentRevision(target.effect, report);
        if (revision === null) return fail("missing_parameter", "A bound effect parameter could not be resolved; the effect build and panel bindings disagree.");
        var nodes = [];
        for (var i = 0; i < CHAIN.length; i++) {
            var node = { id: CHAIN[i].id, label: CHAIN[i].label, params: [] };
            for (var k = 0; k < CHAIN[i].keys.length; k++) {
                var binding = bindingFor(CHAIN[i].keys[k]);
                var property = resolveProperty(target.effect, binding, report);
                if (!property) return fail("missing_parameter", "Missing parameter: " + binding.name);
                node.params.push({ key: binding.key, label: binding.name, kind: binding.kind,
                                   value: readValue(property, binding) });
            }
            nodes.push(node);
        }
        return reply({ ok: true, operation: "getState", requestId: request.requestId || "",
                       target: { comp: target.comp.name, layer: target.layer.name,
                                 effectIndex: target.effect.propertyIndex ? target.effect.propertyIndex : 0 },
                       controlSource: controlSource(target.effect), resolution: report.resolution,
                       revision: revision, nodes: nodes, edges: EDGES });
    }

    function setParameters(request) {
        var target = findTarget();
        if (target.error) return fail(target.error.code, target.error.message);
        if (!(request.changes instanceof Array) || request.changes.length === 0 || request.changes.length > MAX_CHANGES) {
            return fail("invalid_request", "changes must contain 1 to " + MAX_CHANGES + " entries.");
        }
        var report = { resolution: "name" };
        var revision = currentRevision(target.effect, report);
        if (revision === null) return fail("missing_parameter", "A bound effect parameter could not be resolved.");
        if (request.baseRevision && request.baseRevision !== revision) {
            return fail("stale_state", "The effect changed since the panel loaded it; reload before editing.");
        }

        // Validate the entire change set before any mutation.
        var planned = [];
        var seen = {};
        for (var i = 0; i < request.changes.length; i++) {
            var change = request.changes[i];
            if (!change || typeof change.key !== "string") return fail("invalid_request", "Each change needs a key.");
            if (seen[change.key]) return fail("invalid_request", "Duplicate change for " + change.key + ".");
            seen[change.key] = true;
            var binding = bindingFor(change.key);
            if (!binding) return fail("unknown_binding", "Unknown parameter binding: " + change.key);
            if (!validateValue(binding, change.value)) {
                return fail("invalid_value", "Rejected value for " + binding.name + ".");
            }
            var property = resolveProperty(target.effect, binding, report);
            if (!property) return fail("missing_parameter", "Missing parameter: " + binding.name);
            if (property.dimensions === 1 && property.numKeys > 0 && property.isTimeVarying) {
                // Animated controls are owned by AE keyframes; the panel only writes
                // constants so it cannot silently destroy animation.
                return fail("animated_parameter", binding.name + " is animated; edit it in the timeline.");
            }
            planned.push({ binding: binding, property: property, value: change.value });
        }

        app.beginUndoGroup("Starfield: set parameters");
        var failed = null;
        for (var p = 0; p < planned.length; p++) {
            try {
                var value = planned[p].value;
                if (planned[p].binding.kind === "color") {
                    planned[p].property.setValue([value[0] / 255, value[1] / 255, value[2] / 255, 1]);
                } else if (planned[p].binding.kind === "point3d") {
                    planned[p].property.setValue([value[0], value[1], value[2]]);
                } else {
                    planned[p].property.setValue(value);
                }
            } catch (error) {
                failed = { binding: planned[p].binding, message: error.toString() };
                break;
            }
        }
        app.endUndoGroup();
        if (failed) return fail("host_write_failed", failed.message, { key: failed.binding.key });

        var updated = getState({ requestId: request.requestId });
        return updated;
    }

    // Public entry points. Both take and return JSON strings, so the panel never
    // depends on ExtendScript object marshalling.
    function SFLD_getState(requestJson) {
        var request = parseRequest(requestJson);
        if (!request) return fail("invalid_request", "Unsupported or malformed request envelope.");
        try {
            if (request.operation !== "getState") return fail("unknown_operation", request.operation);
            return getState(request);
        } catch (error) {
            return fail("host_error", error.toString());
        }
    }

    function SFLD_setParameters(requestJson) {
        var request = parseRequest(requestJson);
        if (!request) return fail("invalid_request", "Unsupported or malformed request envelope.");
        try {
            if (request.operation !== "setParameters") return fail("unknown_operation", request.operation);
            return setParameters(request);
        } catch (error) {
            return fail("host_error", error.toString());
        }
    }
})();
