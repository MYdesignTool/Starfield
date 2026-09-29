// ExtendScript gateway for the Starfield Particle Controls panel (ADR 0009 protocol v1).
//
// Rules this file must keep:
//   - Only the public AE scripting DOM is used. No sockets, no helper processes,
//     no direct writes to the effect's arbitrary-data graph parameter. Graph edits
//     stage a bounded expression mailbox and use the effect's supervised callback.
//   - Every request is parsed, version-checked and fully validated before any
//     mutation; a failure rejects the whole change set.
//   - Render values are ordinary supervised parameters. Node positions use separate
//     hidden, non-animated ordinary streams owned by the effect instance; they never
//     enter the renderer's graph or rely on CEP-local persistence.
//
// Host qualification status: see docs/compatibility-matrix.md. The graph carrier
// entry points are source-level only; expression scripting, callback acknowledgement,
// undo and persistence remain AE 2023 gates.

(function () {
    var PROTOCOL = "org.starfieldfx.panel";
    var VERSION = 1;
    var GATEWAY_BUILD = "output-particle-status-1";
    var MATCH_NAME = "org.starfieldfx.particle";
    var MAX_CHANGES = 40;
    var MAX_REQUEST_BYTES = 65536;
    var MAX_GRAPH_BYTES = 24 * 1024;
    var MAX_GRAPH_REVISION = 4294967295;
    var MAX_GRAPH_NONCE = 1000000;
    var graphNonceCounter = 0;
    var GRAPH_CARRIERS = {
        snapshot: { index: 41, name: "Graph Snapshot" },
        request: { index: 42, name: "Graph Edit Request" },
        commit: { index: 43, name: "Commit Graph Edit" },
        receipt: { index: 44, name: "Graph Edit Receipt" }
    };

    // One row per bound effect parameter. `index` is the registered parameter index
    // (schema/parameters.json); `name` is the Effect Controls label used for
    // resolution. `min`/`max` mirror the manifest bounds. `displayDecimals` sets
    // panel edit precision; `choices` supplies popup labels. Host validation remains authoritative.
    var BINDINGS = [
        { key: "particle_count", index: 27, name: "Max Particles", kind: "slider", min: 0, max: 2000000, displayDecimals: 0 },
        { key: "birth_rate", index: 3, name: "Particles Per Second", kind: "slider", min: 0, max: 1000000, displayDecimals: 0 },
        { key: "seed", index: 28, name: "Random Seed", kind: "slider", min: 0, max: 2147483647, displayDecimals: 0 },
        { key: "particle_lifetime", index: 12, name: "Lifetime", kind: "slider", min: 0, max: 1000000, displayDecimals: 3 },
        { key: "emitter_shape", index: 2, name: "Type", kind: "popup", min: 1, max: 4, displayDecimals: 0,
          choices: ["Point", "Box", "Sphere", "Disc"] },
        { key: "emitter_origin", index: 4, name: "Origin", kind: "point3d", displayDecimals: 0 },
        { key: "velocity_x", index: 6, name: "Velocity X", kind: "slider", min: -1000, max: 1000, displayDecimals: 2 },
        { key: "velocity_y", index: 7, name: "Velocity Y", kind: "slider", min: -1000, max: 1000, displayDecimals: 2 },
        { key: "velocity_z", index: 8, name: "Velocity Z", kind: "slider", min: -1000, max: 1000, displayDecimals: 2 },
        { key: "particle_size", index: 13, name: "Size", kind: "slider", min: 0, max: 100000, displayDecimals: 2 },
        { key: "opacity", index: 15, name: "Opacity", kind: "slider", min: 0, max: 1, displayDecimals: 3 },
        { key: "emitter_size", index: 5, name: "Emitter Size", kind: "slider", min: 0, max: 10, displayDecimals: 3 },
        { key: "emitter_size_x", index: 81, name: "Size X", kind: "slider", min: 0, max: 1000, displayDecimals: 0 },
        { key: "emitter_size_y", index: 82, name: "Size Y", kind: "slider", min: 0, max: 1000, displayDecimals: 0 },
        { key: "emitter_size_z", index: 83, name: "Size Z", kind: "slider", min: 0, max: 1000, displayDecimals: 0 },
        { key: "velocity_spread", index: 9, name: "Speed Random", kind: "slider", min: 0, max: 100, displayDecimals: 2 },
        { key: "gravity_x", index: 21, name: "Gravity X", kind: "slider", min: -1000, max: 1000, displayDecimals: 2 },
        { key: "gravity_y", index: 22, name: "Gravity Y", kind: "slider", min: -1000, max: 1000, displayDecimals: 2 },
        { key: "gravity_z", index: 23, name: "Gravity Z", kind: "slider", min: -1000, max: 1000, displayDecimals: 2 },
        { key: "linear_drag", index: 24, name: "Linear Drag", kind: "slider", min: 0, max: 100, displayDecimals: 3 },
        { key: "color_start", index: 17, name: "Color Start", kind: "color", min: 0, max: 255, displayDecimals: 0 },
        { key: "color_end", index: 18, name: "Color End", kind: "color", min: 0, max: 255, displayDecimals: 0 },
        { key: "particle_size_end", index: 14, name: "Size Over Life", kind: "slider", min: 0, max: 100000, displayDecimals: 2 },
        { key: "opacity_end", index: 16, name: "Opacity Over Life", kind: "slider", min: 0, max: 1, displayDecimals: 3 }
    ];

    // Appended, hidden AE streams store bounded age/value pairs in the project.
    // A final supervised nonce commits the complete batch to Node Graph mode.
    function appendCurveBindings(label, prefix, countIndex, firstPointIndex, valueMax, valueDecimals) {
        BINDINGS.push({ key: prefix + "_curve_count", index: countIndex,
            name: label + " Curve Count", kind: "slider", min: 0, max: 8, displayDecimals: 0 });
        for (var point = 0; point < 8; point++) {
            BINDINGS.push({ key: prefix + "_curve_point_" + point + "_age",
                index: firstPointIndex + point * 2, name: label + " Curve Point " + point + " Age",
                kind: "slider", min: 0, max: 1, displayDecimals: 3 });
            BINDINGS.push({ key: prefix + "_curve_point_" + point + "_value",
                index: firstPointIndex + point * 2 + 1, name: label + " Curve Point " + point + " Value",
                kind: "slider", min: 0, max: valueMax, displayDecimals: valueDecimals });
        }
    }
    appendCurveBindings("Size", "size", 45, 46, 100000, 2);
    appendCurveBindings("Opacity", "opacity", 62, 63, 1, 3);
    BINDINGS.push({ key: "curve_edit_commit", index: 79, name: "Curve Edit Commit",
        kind: "slider", min: -1000000, max: 1000000, displayDecimals: 0 });

    var LAYOUT_BINDINGS = [
        { nodeId: "emitter", axis: "x", key: "layout_emitter_x", index: 33, name: "Layout Emitter X", min: -1000000000, max: 1000000000, defaultValue: 235 },
        { nodeId: "emitter", axis: "y", key: "layout_emitter_y", index: 34, name: "Layout Emitter Y", min: -1000000000, max: 1000000000, defaultValue: 22 },
        { nodeId: "force", axis: "x", key: "layout_force_x", index: 35, name: "Layout Force X", min: -1000000000, max: 1000000000, defaultValue: 235 },
        { nodeId: "force", axis: "y", key: "layout_force_y", index: 36, name: "Layout Force Y", min: -1000000000, max: 1000000000, defaultValue: 178 },
        // Indices 37/38 keep their shipped storage identity; the panel now assigns
        // that card to the logical Particle stage instead of Appearance.
        { nodeId: "particle", axis: "x", key: "layout_appearance_x", index: 37, name: "Layout Appearance X", min: -1000000000, max: 1000000000, defaultValue: 235 },
        { nodeId: "particle", axis: "y", key: "layout_appearance_y", index: 38, name: "Layout Appearance Y", min: -1000000000, max: 1000000000, defaultValue: 100 },
        { nodeId: "output", axis: "x", key: "layout_output_x", index: 39, name: "Layout Output X", min: -1000000000, max: 1000000000, defaultValue: 235 },
        { nodeId: "output", axis: "y", key: "layout_output_y", index: 40, name: "Layout Output Y", min: -1000000000, max: 1000000000, defaultValue: 256 }
    ];

    // Protocol v1 is a fixed parameter view, not a live graph snapshot. Display
    // the product's intended stream order while graph-backed editing is qualified.
    var CHAIN = [
        { id: "emitter", label: "Emitter", keys: ["birth_rate", "seed", "particle_lifetime",
                                                  "emitter_shape", "emitter_origin", "velocity_x", "velocity_y",
                                                  "velocity_z", "emitter_size", "emitter_size_x",
                                                  "emitter_size_y", "emitter_size_z", "velocity_spread"] },
        { id: "particle", label: "Particle", keys: ["particle_size", "particle_size_end", "opacity",
                                                      "opacity_end", "color_start", "color_end"] },
        { id: "force", label: "Force", keys: ["gravity_x", "gravity_y", "gravity_z", "linear_drag"] },
        // Max Particles is a global output budget in the panel. Its public AE parameter
        // identity remains unchanged; only its node-editor presentation moves here.
        { id: "output", label: "Output", keys: ["particle_count"] }
    ];
    var EDGES = [["emitter", "particle"], ["particle", "force"], ["force", "output"]];

    function bindingFor(key) {
        for (var i = 0; i < BINDINGS.length; i++) {
            if (BINDINGS[i].key === key) return BINDINGS[i];
        }
        return null;
    }

    function layoutBindingFor(nodeId, axis) {
        for (var i = 0; i < LAYOUT_BINDINGS.length; i++) {
            if (LAYOUT_BINDINGS[i].nodeId === nodeId && LAYOUT_BINDINGS[i].axis === axis) return LAYOUT_BINDINGS[i];
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
            // Any selected layer carrying the effect counts, whatever its kind. Text
            // layers were skipped here before, which made a selected text layer with the
            // effect read as "no target".
            var candidateEffects = effectCount(candidate);
            if (candidateEffects > 0) {
                if (count === 0) layer = candidate;
                count += candidateEffects;
            }
        }
        if (count === 0) return { error: { code: "no_target", message: "Select a layer carrying Starfield Particle." } };
        if (count > 1) return { error: { code: "ambiguous_target", message: "Select exactly one Starfield Particle effect." } };
        var effect = findEffect(layer);
        if (!effect) return { error: { code: "no_effect", message: "The selected layer has no Starfield Particle effect." } };
        return { comp: comp, layer: layer, effect: effect };
    }

    // A pinned panel target is resolved by the opaque identity captured from
    // getState. It remains independent of AE's current selection, but fails closed
    // if the project, comp, layer, or effect instance no longer exists.
    function findTargetByToken(token) {
        if (!app.project) return { error: { code: "no_project", message: "No project is open." } };
        if (typeof token !== "string") return { error: { code: "invalid_request", message: "A pinned target token is required." } };
        var match = /^p([0-9]+)-c([0-9]+)-l([0-9]+)-e([0-9]+)$/.exec(token);
        if (!match) return { error: { code: "invalid_request", message: "The pinned target token is malformed." } };
        if (!app.project.rootFolder || String(app.project.rootFolder.id) !== match[1]) {
            return { error: { code: "stale_target", message: "The pinned effect belongs to a different project. Unlock it to follow the current selection." } };
        }
        var comp = null;
        for (var i = 1; i <= app.project.numItems; i++) {
            var item = app.project.item(i);
            if (item instanceof CompItem && String(item.id) === match[2]) { comp = item; break; }
        }
        if (!comp) return { error: { code: "stale_target", message: "The pinned composition no longer exists." } };
        var layer = null;
        for (var l = 1; l <= comp.numLayers; l++) {
            var candidate = comp.layer(l);
            if (String(candidate.id) === match[3]) { layer = candidate; break; }
        }
        if (!layer) return { error: { code: "stale_target", message: "The pinned layer no longer exists." } };
        var parade = layer.property("ADBE Effect Parade");
        var effect = parade ? parade.property(Number(match[4])) : null;
        if (!effect || effect.matchName !== MATCH_NAME) {
            return { error: { code: "stale_target", message: "The pinned Starfield effect no longer exists at that location." } };
        }
        return { comp: comp, layer: layer, effect: effect };
    }

    function findRequestTarget(request) {
        if (request && request.pinTarget === true) {
            return findTargetByToken(request.target && request.target.token);
        }
        return findTarget();
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
        if (report) report.missingParameter = binding.name;
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

    // Keep a detached copy of the exact host value for transaction rollback. The
    // panel's normalized value intentionally omits color alpha, so rollback must
    // preserve the raw AE value instead of reconstructing it from readValue().
    function copyHostValue(value) {
        if (value === null || typeof value !== "object" || typeof value.length !== "number") return value;
        var copy = [];
        for (var i = 0; i < value.length; i++) copy.push(value[i]);
        return copy;
    }

    function hostValueFor(binding, value) {
        if (binding.kind === "color") {
            return [value[0] / 255, value[1] / 255, value[2] / 255, 1];
        }
        if (binding.kind === "point3d") {
            return [value[0], value[1], value[2]];
        }
        return value;
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

    function currentRevision(target, report) {
        // Cheap opaque token over every bound value: identifies the edit base without
        // exposing host pointers. Include the selected comp/layer/effect identity so
        // equal parameter values on a newly selected effect cannot accept an old edit.
        // Not a hash of the arbitrary graph.
        var identity = targetToken(target);
        if (identity === null) return null;
        var text = "target=" + identity + ";";
        for (var i = 0; i < BINDINGS.length; i++) {
            var property = resolveProperty(target.effect, BINDINGS[i], report);
            if (!property) return null;
            text += BINDINGS[i].key + "=" + String(readValue(property, BINDINGS[i])) + ";";
        }
        var layoutValues = {};
        var layoutSupported = true;
        for (var l = 0; l < LAYOUT_BINDINGS.length; l++) {
            var layoutBinding = LAYOUT_BINDINGS[l];
            var layoutProperty = resolveProperty(target.effect, layoutBinding, report);
            var layoutValue = null;
            try {
                if (layoutProperty) layoutValue = Number(layoutProperty.value);
            } catch (ignored) { layoutProperty = null; }
            if (!layoutProperty || !isFinite(layoutValue)) {
                layoutSupported = false;
                break;
            }
            if (!layoutValues[layoutBinding.nodeId]) layoutValues[layoutBinding.nodeId] = {};
            layoutValues[layoutBinding.nodeId][layoutBinding.axis] = layoutValue;
        }
        if (report) {
            report.layoutPersistence = layoutSupported;
            report.layoutValues = layoutSupported ? layoutValues : null;
        }
        if (layoutSupported) {
            for (var lv = 0; lv < LAYOUT_BINDINGS.length; lv++) {
                var valueBinding = LAYOUT_BINDINGS[lv];
                text += valueBinding.key + "=" + String(layoutValues[valueBinding.nodeId][valueBinding.axis]) + ";";
            }
        } else {
            text += "layoutPersistence=session-only;";
        }
        var hash = 5381;
        for (var c = 0; c < text.length; c++) hash = ((hash * 33) ^ text.charCodeAt(c)) & 0xffffffff;
        return (hash >>> 0).toString(16);
    }

    // AE 2023 exposes persistent Item.id and Layer.id values (both were added
    // before AE 2023); the effect's propertyIndex distinguishes its instance on
    // the layer. The project root ID prevents an open-project switch from reusing
    // a token when two projects happen to reuse item IDs. The panel treats this
    // string as an opaque echo token.
    function targetToken(target) {
        if (!target || !target.comp || !target.layer || !target.effect || !app.project || !app.project.rootFolder) return null;
        var projectId = Number(app.project.rootFolder.id);
        var compId = Number(target.comp.id);
        var layerId = Number(target.layer.id);
        var effectIndex = Number(target.effect.propertyIndex);
        if (!isFinite(projectId) || Math.floor(projectId) !== projectId || projectId < 0 ||
            !isFinite(compId) || Math.floor(compId) !== compId || compId < 0 ||
            !isFinite(layerId) || Math.floor(layerId) !== layerId || layerId < 0 ||
            !isFinite(effectIndex) || Math.floor(effectIndex) !== effectIndex || effectIndex < 1) return null;
        return "p" + projectId + "-c" + compId + "-l" + layerId + "-e" + effectIndex;
    }

    function resolveCarrier(effect, key) {
        var binding = GRAPH_CARRIERS[key];
        if (!binding) return null;
        var property = effect.property(binding.name);
        if (property) return property;
        property = effect.property(binding.index);
        return property && property.name === binding.name ? property : null;
    }

    function crc32Hex(hex) {
        var crc = 0xffffffff;
        for (var i = 0; i < hex.length; i += 2) {
            crc ^= parseInt(hex.substr(i, 2), 16);
            for (var bit = 0; bit < 8; bit++) {
                crc = (crc >>> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
            }
        }
        var text = ((crc ^ 0xffffffff) >>> 0).toString(16);
        while (text.length < 8) text = "0" + text;
        return text;
    }

    function parseGraphSnapshot(expression) {
        if (typeof expression !== "string" || expression.length === 0) {
            return { initialized: false };
        }
        var match = /^\/\*SFLDSNAP1:([0-9]+):([0-9]+):([0-9a-fA-F]{8}):([0-9a-fA-F]+)\*\/0$/.exec(expression);
        if (!match) return null;
        var revision = Number(match[1]);
        var byteCount = Number(match[2]);
        var graphHex = match[4].toLowerCase();
        if (!isFinite(revision) || Math.floor(revision) !== revision || revision < 1 || revision > MAX_GRAPH_REVISION ||
            !isFinite(byteCount) || Math.floor(byteCount) !== byteCount || byteCount < 32 || byteCount > MAX_GRAPH_BYTES ||
            graphHex.length !== byteCount * 2 || crc32Hex(graphHex) !== match[3].toLowerCase()) return null;
        return { initialized: true, revision: revision, byteCount: byteCount, crc32: match[3].toLowerCase(), graphHex: graphHex };
    }

    function nextGraphNonce(commitProperty, receiptProperty) {
        var value = ((new Date()).getTime() + (++graphNonceCounter)) % MAX_GRAPH_NONCE;
        if (value < 1) value = 1;
        var commitValue = Number(commitProperty.value);
        var receiptValue = Number(receiptProperty.value);
        for (var attempts = 0; attempts < 4; attempts++) {
            if (value !== commitValue && value !== receiptValue && value !== -receiptValue) break;
            value = value >= MAX_GRAPH_NONCE ? 1 : value + 1;
        }
        return value;
    }

    function graphCarrierTarget(request) {
        var target = findRequestTarget(request);
        if (target.error) return { error: target.error };
        var token = targetToken(target);
        if (token === null) return { error: { code: "host_error", message: "AE did not provide stable IDs for the project, composition, layer, and effect." } };
        if (!request.target || request.target.token !== token) {
            return { error: { code: "stale_target", message: "The pinned effect changed; reload the graph before editing." } };
        }
        var properties = {};
        for (var key in GRAPH_CARRIERS) {
            if (!Object.prototype.hasOwnProperty.call(GRAPH_CARRIERS, key)) continue;
            properties[key] = resolveCarrier(target.effect, key);
            if (!properties[key]) {
                return { error: { code: "missing_parameter", message: "This plug-in build has no script-visible graph carrier stream: " + GRAPH_CARRIERS[key].name + "." } };
            }
        }
        return { target: target, token: token, properties: properties };
    }

    function readGraphSnapshot(request) {
        var resolved = graphCarrierTarget(request);
        if (resolved.error) return fail(resolved.error.code, resolved.error.message);
        var snapshot = parseGraphSnapshot(resolved.properties.snapshot.expression);
        if (snapshot === null) return fail("invalid_graph_snapshot", "The stored graph snapshot is malformed or failed its checksum.");
        return reply({ ok: true, operation: "getGraphSnapshot", requestId: request.requestId || "",
                       target: { token: resolved.token }, snapshot: snapshot });
    }

    function triggerGraphCarrier(resolved, expression, undoLabel, requestId, nonce) {
        var mailbox = resolved.properties.request;
        var commit = resolved.properties.commit;
        var receipt = resolved.properties.receipt;
        var oldExpression = mailbox.expression;
        var oldExpressionEnabled = mailbox.expressionEnabled;
        if (typeof nonce !== "number") nonce = nextGraphNonce(commit, receipt);
        var failed = null;
        var attemptedCommit = false;
        var groupOpen = false;
        try {
            app.beginUndoGroup(undoLabel);
            groupOpen = true;
            mailbox.expression = expression;
            mailbox.expressionEnabled = false;
            attemptedCommit = true;
            commit.setValue(nonce);
        } catch (error) {
            failed = error.toString();
            if (!attemptedCommit) {
                try {
                    mailbox.expression = oldExpression;
                    mailbox.expressionEnabled = oldExpressionEnabled;
                } catch (rollbackError) {
                    failed += " Mailbox rollback failed: " + rollbackError.toString();
                }
            }
        } finally {
            if (groupOpen) {
                try { app.endUndoGroup(); }
                catch (endError) { failed = failed || endError.toString(); }
            }
        }
        if (failed) return { ok: false, error: { code: "graph_commit_failed", message: failed },
                            context: { commitAttempted: attemptedCommit } };
        var acceptedNonce = Number(receipt.value);
        if (acceptedNonce !== nonce) {
            if (acceptedNonce === -nonce) {
                return { ok: false, error: { code: "graph_commit_rejected", message: "The effect rejected the graph transaction. The existing graph was kept." },
                         context: { nonce: nonce } };
            }
            return { ok: false, error: { code: "graph_commit_unconfirmed", message: "The effect did not acknowledge the graph transaction. Reload the graph before making another edit." },
                     context: { nonce: nonce } };
        }
        return { ok: true, nonce: nonce, requestId: requestId || "" };
    }

    function syncGraphSnapshot(request) {
        var resolved = graphCarrierTarget(request);
        if (resolved.error) return fail(resolved.error.code, resolved.error.message);
        var nonce = nextGraphNonce(resolved.properties.commit, resolved.properties.receipt);
        var transaction = "/*SFLDSYNC1:" + nonce + "*/0";
        var receipt = triggerGraphCarrier(resolved, transaction, "Starfield: initialize graph snapshot", request.requestId, nonce);
        if (!receipt.ok) return reply(receipt);
        var snapshot = parseGraphSnapshot(resolved.properties.snapshot.expression);
        if (!snapshot || !snapshot.initialized) return fail("graph_snapshot_unconfirmed", "The effect accepted the request but did not publish a readable graph snapshot.");
        return reply({ ok: true, operation: "syncGraphSnapshot", requestId: request.requestId || "",
                       target: { token: resolved.token }, nonce: receipt.nonce, snapshot: snapshot });
    }

    function submitGraph(request) {
        var resolved = graphCarrierTarget(request);
        if (resolved.error) return fail(resolved.error.code, resolved.error.message);
        if (typeof request.baseGraphRevision !== "number" || !isFinite(request.baseGraphRevision) ||
            Math.floor(request.baseGraphRevision) !== request.baseGraphRevision || request.baseGraphRevision < 1 ||
            request.baseGraphRevision > MAX_GRAPH_REVISION) {
            return fail("invalid_request", "baseGraphRevision must be the integer revision from getGraphSnapshot.");
        }
        if (typeof request.graphHex !== "string" || request.graphHex.length < 64 || request.graphHex.length > MAX_GRAPH_BYTES * 2 ||
            request.graphHex.length % 2 !== 0 || !/^[0-9a-fA-F]+$/.test(request.graphHex)) {
            return fail("invalid_request", "graphHex must contain a complete bounded schema-1 graph payload.");
        }
        var graphHex = request.graphHex.toLowerCase();
        var snapshot = parseGraphSnapshot(resolved.properties.snapshot.expression);
        if (!snapshot || !snapshot.initialized) return fail("graph_snapshot_uninitialized", "Initialize the graph snapshot before submitting an edit.");
        if (snapshot.revision !== request.baseGraphRevision) {
            return fail("stale_graph", "The project graph changed since this edit began; reload before editing.");
        }
        var nonce = nextGraphNonce(resolved.properties.commit, resolved.properties.receipt);
        var byteCount = graphHex.length / 2;
        var transaction = "/*SFLDTXN1:" + nonce + ":" + snapshot.revision + ":" + byteCount + ":" +
                          crc32Hex(graphHex) + ":" + graphHex + "*/0";
        var receipt = triggerGraphCarrier(resolved, transaction, "Starfield: edit graph", request.requestId, nonce);
        if (!receipt.ok) return reply(receipt);
        var updated = parseGraphSnapshot(resolved.properties.snapshot.expression);
        if (!updated || !updated.initialized || updated.revision !== snapshot.revision + 1 || updated.graphHex !== graphHex) {
            return fail("graph_snapshot_unconfirmed", "The effect acknowledged the edit but the saved graph snapshot did not match it.");
        }
        return reply({ ok: true, operation: "submitGraph", requestId: request.requestId || "",
                       target: { token: resolved.token }, nonce: receipt.nonce, snapshot: updated });
    }

    function controlSource(effect) {
        var property = effect.property("Control Source");
        if (!property) return "unknown";
        return Number(property.value) === 2 ? "Node Graph" : "AE Controls";
    }

    function getState(request) {
        var target = findRequestTarget(request);
        if (target.error) return fail(target.error.code, target.error.message);
        var report = { resolution: "name" };
        var token = targetToken(target);
        if (token === null) return fail("host_error", "AE did not provide stable IDs for the project, composition, layer, and effect.");
        var revision = currentRevision(target, report);
        if (revision === null) return fail("missing_parameter", "A render parameter could not be resolved" +
            (report.missingParameter ? ": " + report.missingParameter : ".") + " Check that the plug-in and panel builds match.");
        var sourceMode = controlSource(target.effect);
        var nodes = [];
        var nodeValues = {};
        for (var i = 0; i < CHAIN.length; i++) {
            var node = { id: CHAIN[i].id, label: CHAIN[i].label, params: [] };
            for (var k = 0; k < CHAIN[i].keys.length; k++) {
                var binding = bindingFor(CHAIN[i].keys[k]);
                var property = resolveProperty(target.effect, binding, report);
                if (!property) return fail("missing_parameter", "Missing parameter: " + binding.name);
                var parameter = { key: binding.key, label: binding.name, kind: binding.kind,
                                  value: readValue(property, binding),
                                  displayDecimals: binding.displayDecimals,
                                  animated: property.numKeys > 0 || property.isTimeVarying };
                if (typeof binding.min === "number") parameter.min = binding.min;
                if (typeof binding.max === "number") parameter.max = binding.max;
                if (binding.choices) parameter.choices = binding.choices;
                node.params.push(parameter);
                nodeValues[binding.key] = parameter.value;
            }
            nodes.push(node);
        }
        var curves = {
            size: readCurve(target.effect, "size", nodeValues.particle_size,
                            nodeValues.particle_size_end, sourceMode === "AE Controls", report),
            opacity: readCurve(target.effect, "opacity", nodeValues.opacity,
                               nodeValues.opacity_end, sourceMode === "AE Controls", report)
        };
        if (!curves.size || !curves.opacity) {
            return fail("invalid_curve", report.invalidCurve || "An over-life curve parameter is invalid.");
        }
        var commitBinding = bindingFor("curve_edit_commit");
        var commitProperty = resolveProperty(target.effect, commitBinding, report);
        if (!commitProperty) return fail("missing_parameter", "Missing curve commit stream.");
        var layout = {};
        var oldDefaultLayout = {
            emitter: { x: 180, y: 22 }, force: { x: 180, y: 190 },
            particle: { x: 180, y: 358 }, output: { x: 180, y: 526 }
        };
        var oldDefaultsUnchanged = report.layoutPersistence;
        if (oldDefaultsUnchanged) {
            for (var oldId in oldDefaultLayout) {
                if (!Object.prototype.hasOwnProperty.call(oldDefaultLayout, oldId)) continue;
                var saved = report.layoutValues[oldId];
                if (!saved || saved.x !== oldDefaultLayout[oldId].x || saved.y !== oldDefaultLayout[oldId].y) {
                    oldDefaultsUnchanged = false;
                    break;
                }
            }
        }
        for (var n = 0; n < CHAIN.length; n++) {
            var nodeId = CHAIN[n].id;
            layout[nodeId] = report.layoutPersistence && !oldDefaultsUnchanged ? report.layoutValues[nodeId] : {
                x: layoutBindingFor(nodeId, "x").defaultValue,
                y: layoutBindingFor(nodeId, "y").defaultValue
            };
        }
        return reply({ ok: true, operation: "getState", requestId: request.requestId || "",
                       target: { token: token, comp: target.comp.name, layer: target.layer.name,
                                 effectIndex: target.effect.propertyIndex ? target.effect.propertyIndex : 0 },
                       controlSource: sourceMode, resolution: report.resolution,
                        revision: revision, nodes: nodes, edges: EDGES, layout: layout, curves: curves,
                        curveEditCommit: Number(readValue(commitProperty, commitBinding)),
                        layoutPersistence: report.layoutPersistence });
    }

    function readCurve(effect, prefix, startValue, endValue,
                       useCurrentEndpoints, report) {
        var countBinding = bindingFor(prefix + "_curve_count");
        var countProperty = resolveProperty(effect, countBinding, report);
        if (!countProperty) return null;
        var count = Number(readValue(countProperty, countBinding));
        if (!isFinite(count) || Math.floor(count) !== count || count < 0 || count > 8) {
            report.invalidCurve = prefix + " curve point count is invalid.";
            return null;
        }
        if (count === 0) {
            return { custom: false, points: [{ age: 0, value: Number(startValue) },
                                              { age: 1, value: Number(endValue) }] };
        }
        if (count < 2) {
            report.invalidCurve = prefix + " curve needs at least two endpoints.";
            return null;
        }
        var points = [];
        for (var i = 0; i < count; i++) {
            var ageBinding = bindingFor(prefix + "_curve_point_" + i + "_age");
            var valueBinding = bindingFor(prefix + "_curve_point_" + i + "_value");
            var ageProperty = resolveProperty(effect, ageBinding, report);
            var valueProperty = resolveProperty(effect, valueBinding, report);
            if (!ageProperty || !valueProperty) return null;
            var age = Number(readValue(ageProperty, ageBinding));
            var value = Number(readValue(valueProperty, valueBinding));
            if (!isFinite(age) || !isFinite(value) || age < 0 || age > 1 ||
                value < 0 || value > valueBinding.max || (i > 0 && age <= points[i - 1].age)) {
                report.invalidCurve = prefix + " curve contains an invalid or unordered point.";
                return null;
            }
            points.push({ age: age, value: value });
        }
        // Scalar endpoint controls may be animated in AE Controls mode. They remain
        // authoritative at the current comp time while interior knots stay constant.
        if (useCurrentEndpoints) {
            points[0].value = Number(startValue);
            points[points.length - 1].value = Number(endValue);
        }
        if (points[0].age !== 0 || points[points.length - 1].age !== 1) {
            report.invalidCurve = prefix + " curve endpoints must remain at ages 0 and 1.";
            return null;
        }
        return { custom: true, points: points };
    }

    function validateCurveBankCommit(effect, planned, report) {
        function valueFor(key) {
            for (var p = 0; p < planned.length; p++) {
                if (planned[p].binding.key === key) return planned[p].value;
            }
            var binding = bindingFor(key);
            var property = binding && resolveProperty(effect, binding, report);
            return property ? readValue(property, binding) : null;
        }

        var banks = [
            { prefix: "size", label: "Size", max: 100000 },
            { prefix: "opacity", label: "Opacity", max: 1 }
        ];
        for (var b = 0; b < banks.length; b++) {
            var bank = banks[b];
            var count = valueFor(bank.prefix + "_curve_count");
            if (typeof count !== "number" || !isFinite(count) || Math.floor(count) !== count ||
                count < 0 || count > 8 || count === 1) {
                return bank.label + " curve point count must be 0 or an integer from 2 to 8.";
            }
            if (count === 0) continue;
            var previousAge = -1;
            for (var i = 0; i < count; i++) {
                var age = valueFor(bank.prefix + "_curve_point_" + i + "_age");
                var value = valueFor(bank.prefix + "_curve_point_" + i + "_value");
                if (typeof age !== "number" || !isFinite(age) || age < 0 || age > 1 ||
                    typeof value !== "number" || !isFinite(value) || value < 0 || value > bank.max ||
                    (i > 0 && age <= previousAge)) {
                    return bank.label + " curve contains an invalid or unordered point.";
                }
                if ((i === 0 && age !== 0) || (i === count - 1 && age !== 1)) {
                    return bank.label + " curve endpoints must remain at ages 0 and 1.";
                }
                previousAge = age;
            }
        }
        return null;
    }

    function getFrameStatus(request) {
        var target = findRequestTarget(request);
        if (target.error) return fail(target.error.code, target.error.message);
        var token = targetToken(target);
        if (token === null) return fail("host_error", "AE did not provide stable IDs for the project, composition, layer, and effect.");
        if (!request.target || request.target.token !== token) {
            return fail("stale_state", "The selected effect changed since the panel loaded it; refresh before reading frame status.");
        }
        if (controlSource(target.effect) !== "AE Controls") {
            return reply({ ok: true, operation: "getFrameStatus", requestId: request.requestId || "",
                           targetToken: token, available: false });
        }
        var timeSeconds = Number(target.comp.time);
        if (!isFinite(timeSeconds)) return fail("host_error", "AE did not provide a finite composition time.");

        function scalarAtTime(key) {
            var binding = bindingFor(key);
            var property = binding ? resolveProperty(target.effect, binding, null) : null;
            if (!property || typeof property.valueAtTime !== "function") return null;
            var value = Number(property.valueAtTime(timeSeconds, false));
            return isFinite(value) ? value : null;
        }

        var birthRate = scalarAtTime("birth_rate");
        var lifetimeSeconds = scalarAtTime("particle_lifetime");
        var maxParticles = scalarAtTime("particle_count");
        if (birthRate === null || lifetimeSeconds === null || maxParticles === null) {
            return fail("missing_parameter", "AE could not read the emitter rate, lifetime, or output particle cap at the current frame.");
        }
        return reply({ ok: true, operation: "getFrameStatus", requestId: request.requestId || "",
                       targetToken: token, available: true, timeSeconds: timeSeconds, birthRate: birthRate,
                       lifetimeSeconds: lifetimeSeconds, maxParticles: maxParticles });
    }

    function setParameters(request) {
        var target = findRequestTarget(request);
        if (target.error) return fail(target.error.code, target.error.message);
        if (!request.target || typeof request.target.token !== "string" || request.target.token.length === 0) {
            return fail("invalid_request", "target.token from the last getState response is required.");
        }
        var token = targetToken(target);
        if (token === null) return fail("host_error", "AE did not provide stable IDs for the project, composition, layer, and effect.");
        if (request.target.token !== token) {
            return fail("stale_state", "The selected effect changed since the panel loaded it; reload before editing.");
        }
        if (typeof request.baseRevision !== "string" || request.baseRevision.length === 0) {
            return fail("invalid_request", "baseRevision from the last getState response is required.");
        }
        if (!(request.changes instanceof Array) || request.changes.length === 0 || request.changes.length > MAX_CHANGES) {
            return fail("invalid_request", "changes must contain 1 to " + MAX_CHANGES + " entries.");
        }
        var report = { resolution: "name" };
        var revision = currentRevision(target, report);
        if (revision === null) return fail("missing_parameter", "A bound effect parameter could not be resolved" +
            (report.missingParameter ? ": " + report.missingParameter : "."));
        if (request.baseRevision !== revision) {
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
            if (property.numKeys > 0 || property.isTimeVarying) {
                // Animated controls are owned by AE keyframes; the panel only writes
                // constants so it cannot silently destroy scalar, color, or point
                // animation. `dimensions === 1` would miss vector properties.
                return fail("animated_parameter", binding.name + " is animated; edit it in the timeline.");
            }
            planned.push({ binding: binding, property: property, value: change.value,
                           previousValue: copyHostValue(property.value) });
        }

        // The nonce is the curve bank's commit marker. Validate both complete banks
        // against the pending batch before writing any AE property, so a malformed
        // curve cannot be persisted and only then rejected by the refreshed read.
        if (seen.curve_edit_commit) {
            var curveError = validateCurveBankCommit(target.effect, planned, report);
            if (curveError) return fail("invalid_curve", curveError);
        }

        var failed = null;
        var rollbackFailure = null;
        var groupOpen = false;
        try {
            app.beginUndoGroup("Starfield: set parameters");
            groupOpen = true;
            for (var p = 0; p < planned.length; p++) {
                try {
                    planned[p].property.setValue(hostValueFor(planned[p].binding, planned[p].value));
                } catch (error) {
                    failed = { binding: planned[p].binding, message: error.toString() };
                    // Include the failing property: a host setter can throw after
                    // partially applying its value.
                    for (var r = p; r >= 0; r--) {
                        try {
                            planned[r].property.setValue(planned[r].previousValue);
                        } catch (rollbackError) {
                            if (!rollbackFailure) {
                                rollbackFailure = { binding: planned[r].binding, message: rollbackError.toString() };
                            }
                        }
                    }
                    break;
                }
            }
        } catch (error) {
            failed = failed || { binding: null, message: error.toString() };
        } finally {
            if (groupOpen) {
                try {
                    app.endUndoGroup();
                } catch (error) {
                    failed = failed || { binding: null, message: error.toString() };
                }
            }
        }
        if (failed) {
            var message = failed.message;
            var context = {};
            if (failed.binding) context.key = failed.binding.key;
            if (rollbackFailure) {
                context.rollbackFailed = true;
                message += " Rollback also failed for " + rollbackFailure.binding.name + ": " + rollbackFailure.message;
            } else if (failed.binding) {
                context.rollbackFailed = false;
                message += " Earlier writes were restored.";
            }
            return fail("host_write_failed", message, context);
        }

        var updated = getState({ requestId: request.requestId, pinTarget: request.pinTarget,
                                 target: request.target });
        return updated;
    }

    function setNodeLayout(request) {
        var target = findRequestTarget(request);
        if (target.error) return fail(target.error.code, target.error.message);
        if (!request.target || typeof request.target.token !== "string" || request.target.token.length === 0) {
            return fail("invalid_request", "target.token from the last getState response is required.");
        }
        var token = targetToken(target);
        if (token === null) return fail("host_error", "AE did not provide stable IDs for the project, composition, layer, and effect.");
        if (request.target.token !== token) {
            return fail("stale_state", "The selected effect changed since the panel loaded it; reload before editing.");
        }
        if (typeof request.baseRevision !== "string" || request.baseRevision.length === 0) {
            return fail("invalid_request", "baseRevision from the last getState response is required.");
        }
        if (!request.layout || typeof request.layout !== "object") {
            return fail("invalid_request", "layout must contain the x and y position for each fixed node.");
        }

        var report = { resolution: "name" };
        var revision = currentRevision(target, report);
        if (revision === null) return fail("missing_parameter", "A bound effect parameter could not be resolved" +
            (report.missingParameter ? ": " + report.missingParameter : "."));
        if (!report.layoutPersistence) {
            return fail("layout_persistence_unavailable", "This effect build has no script-accessible project layout streams. Update the plug-in to save node positions in the AE project.");
        }
        if (request.baseRevision !== revision) {
            return fail("stale_state", "The effect changed since the panel loaded it; reload before editing.");
        }

        // Validate and resolve all eight streams before mutating any of them.
        var planned = [];
        for (var i = 0; i < LAYOUT_BINDINGS.length; i++) {
            var binding = LAYOUT_BINDINGS[i];
            var position = request.layout[binding.nodeId];
            if (!position || typeof position !== "object") {
                return fail("invalid_request", "Missing layout position for " + binding.nodeId + ".");
            }
            var value = position[binding.axis];
            if (typeof value !== "number" || !isFinite(value) || value < binding.min || value > binding.max) {
                return fail("invalid_value", "Rejected layout coordinate for " + binding.nodeId + " " + binding.axis + ".");
            }
            var property = resolveProperty(target.effect, binding, report);
            if (!property) return fail("missing_parameter", "Missing saved node layout stream: " + binding.name);
            if (property.numKeys > 0 || property.isTimeVarying) {
                return fail("animated_parameter", binding.name + " must remain a constant layout value.");
            }
            planned.push({ binding: binding, property: property, value: value,
                           previousValue: Number(property.value) });
        }

        var failed = null;
        var rollbackFailure = null;
        var groupOpen = false;
        try {
            app.beginUndoGroup("Starfield: move nodes");
            groupOpen = true;
            for (var p = 0; p < planned.length; p++) {
                try {
                    planned[p].property.setValue(planned[p].value);
                } catch (error) {
                    failed = { binding: planned[p].binding, message: error.toString() };
                    for (var r = p; r >= 0; r--) {
                        try {
                            planned[r].property.setValue(planned[r].previousValue);
                        } catch (rollbackError) {
                            if (!rollbackFailure) {
                                rollbackFailure = { binding: planned[r].binding, message: rollbackError.toString() };
                            }
                        }
                    }
                    break;
                }
            }
        } catch (error) {
            failed = failed || { binding: null, message: error.toString() };
        } finally {
            if (groupOpen) {
                try {
                    app.endUndoGroup();
                } catch (error) {
                    failed = failed || { binding: null, message: error.toString() };
                }
            }
        }
        if (failed) {
            var message = failed.message;
            var context = {};
            if (failed.binding) context.key = failed.binding.key;
            if (rollbackFailure) {
                context.rollbackFailed = true;
                message += " Rollback also failed for " + rollbackFailure.binding.name + ": " + rollbackFailure.message;
            } else if (failed.binding) {
                context.rollbackFailed = false;
                message += " Earlier writes were restored.";
            }
            return fail("host_write_failed", message, context);
        }

        return getState({ requestId: request.requestId, pinTarget: request.pinTarget,
                          target: request.target });
    }

    // Public entry points. Both take and return JSON strings, so the panel never
    // depends on ExtendScript object marshalling.
    //
    // They are published on the ExtendScript global object below. This matters: CEP
    // evaluates this file from the manifest's ScriptPath into the host's scripting
    // engine, and everything in this file lives inside this IIFE. Without the export the
    // panel's evalScript("SFLD_getState(...)") is a ReferenceError, CEP hands back the
    // string "EvalScript error.", and the panel can only report that its reply was
    // unreadable - which is exactly the failure the first host run produced.
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

    function SFLD_getFrameStatus(requestJson) {
        var request = parseRequest(requestJson);
        if (!request) return fail("invalid_request", "Unsupported or malformed request envelope.");
        try {
            if (request.operation !== "getFrameStatus") return fail("unknown_operation", request.operation);
            return getFrameStatus(request);
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

    function SFLD_setNodeLayout(requestJson) {
        var request = parseRequest(requestJson);
        if (!request) return fail("invalid_request", "Unsupported or malformed request envelope.");
        try {
            if (request.operation !== "setNodeLayout") return fail("unknown_operation", request.operation);
            return setNodeLayout(request);
        } catch (error) {
            return fail("host_error", error.toString());
        }
    }

    function SFLD_getGraphSnapshot(requestJson) {
        var request = parseRequest(requestJson);
        if (!request) return fail("invalid_request", "Unsupported or malformed request envelope.");
        try {
            if (request.operation !== "getGraphSnapshot") return fail("unknown_operation", request.operation);
            return readGraphSnapshot(request);
        } catch (error) {
            return fail("host_error", error.toString());
        }
    }

    function SFLD_syncGraphSnapshot(requestJson) {
        var request = parseRequest(requestJson);
        if (!request) return fail("invalid_request", "Unsupported or malformed request envelope.");
        try {
            if (request.operation !== "syncGraphSnapshot") return fail("unknown_operation", request.operation);
            return syncGraphSnapshot(request);
        } catch (error) {
            return fail("host_error", error.toString());
        }
    }

    function SFLD_submitGraph(requestJson) {
        var request = parseRequest(requestJson);
        if (!request) return fail("invalid_request", "Unsupported or malformed request envelope.");
        try {
            if (request.operation !== "submitGraph") return fail("unknown_operation", request.operation);
            return submitGraph(request);
        } catch (error) {
            return fail("host_error", error.toString());
        }
    }

    // Publish the entry points on the ExtendScript global object; everything above is
    // private to this IIFE (see the note next to the entry points).
    var host = (typeof $ !== "undefined" && $.global) ? $.global : this;
    host.SFLD_getState = SFLD_getState;
    host.SFLD_getFrameStatus = SFLD_getFrameStatus;
    host.SFLD_setParameters = SFLD_setParameters;
    host.SFLD_setNodeLayout = SFLD_setNodeLayout;
    host.SFLD_getGraphSnapshot = SFLD_getGraphSnapshot;
    host.SFLD_syncGraphSnapshot = SFLD_syncGraphSnapshot;
    host.SFLD_submitGraph = SFLD_submitGraph;
    // Readiness probe for the panel's self-loading path: cheap, side-effect free.
    host.SFLD_ready = function () { return PROTOCOL + "/" + VERSION + "/" + GATEWAY_BUILD; };
})();
