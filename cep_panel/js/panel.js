// Starfield Particle Controls panel. Thin client for the ADR 0009 protocol v1: it renders
// the fixed emitter -> force -> appearance -> output chain, edits the supervised AE
// parameter streams through the ExtendScript gateway, and never touches the effect's
// arbitrary-data graph parameter or any host-private state.

(function () {
    "use strict";

    var REQUEST_TIMEOUT_MS = 8000;
    var STARTUP_RETRY_DELAYS_MS = [250, 750, 1500, 3000, 5000];
    var startupRetryAttempt = 0;
    var startupRetryTimer = null;
    var refreshEpoch = 0;

    // Minimal CEP bridge. CEP injects window.__adobe_cep__ into extension panels;
    // Adobe's full CSInterface library can replace this shim later without changing
    // the protocol code below.
    function evalScript(script, callback) {
        if (!window.__adobe_cep__ || typeof window.__adobe_cep__.evalScript !== "function") {
            callback('{"ok":false,"error":{"code":"no_host","message":"CEP host bridge is unavailable."}}');
            return;
        }
        window.__adobe_cep__.evalScript(script, callback);
    }

    // Example presets. Only values with agreed units are included: the emitter origin
    // stays untouched because its host unit (pixels vs percent) is still an open
    // question (backlog D-05) and a wrong guess would silently move the emitter.
    var PRESETS = {
        defaults: {
            particle_count: 1000, birth_rate: 30, seed: 1, particle_lifetime: 2, emitter_shape: 1,
            velocity_x: 0, velocity_y: 0.3, velocity_z: 0, emitter_size: 0.05, velocity_spread: 0.15,
            gravity_x: 0, gravity_y: 0, gravity_z: 0, linear_drag: 0,
            particle_size: 8, particle_size_end: 8, opacity: 1, opacity_end: 1,
            color_start: [255, 255, 255], color_end: [255, 255, 255]
        },
        spark: {
            particle_count: 4000, birth_rate: 220, seed: 7, particle_lifetime: 1.1, emitter_shape: 1,
            velocity_x: 0, velocity_y: 1.6, velocity_z: 0, emitter_size: 0, velocity_spread: 1.1,
            gravity_x: 0, gravity_y: -2.6, gravity_z: 0, linear_drag: 0.9,
            particle_size: 3.2, particle_size_end: 0.6, opacity: 1, opacity_end: 0,
            color_start: [255, 240, 180], color_end: [255, 90, 20]
        },
        snow: {
            particle_count: 2500, birth_rate: 90, seed: 21, particle_lifetime: 6.5, emitter_shape: 2,
            velocity_x: 0.06, velocity_y: -0.14, velocity_z: 0, emitter_size: 1.1, velocity_spread: 0.35,
            gravity_x: 0, gravity_y: -0.05, gravity_z: 0, linear_drag: 0.15,
            particle_size: 4.5, particle_size_end: 4.5, opacity: 0.9, opacity_end: 0.75,
            color_start: [235, 245, 255], color_end: [200, 215, 235]
        },
        floating_light: {
            particle_count: 300, birth_rate: 14, seed: 3, particle_lifetime: 9, emitter_shape: 3,
            velocity_x: 0, velocity_y: 0.16, velocity_z: 0, emitter_size: 0.9, velocity_spread: 0.4,
            gravity_x: 0, gravity_y: 0.06, gravity_z: 0, linear_drag: 0.35,
            particle_size: 14, particle_size_end: 3, opacity: 0.85, opacity_end: 0,
            color_start: [255, 232, 150], color_end: [255, 140, 60]
        }
    };

    // Surface load-time failures instead of showing a blank panel: a syntax error or an
    // exception during startup is exactly what "the panel is recognised but will not open"
    // looks like from the outside.
    window.onerror = function (message, source, line) {
        try {
            var banner = document.getElementById("banner");
            if (banner) {
                banner.className = "banner error";
                banner.textContent = "Panel script error: " + message + " (" + source + ":" + line + ")";
            }
        } catch (ignored) { /* nothing else we can do */ }
        return false;
    };

    var state = { revision: null, targetToken: null, nodes: [], values: {}, pending: false };
    var elements = {
        banner: document.getElementById("banner"),
        chain: document.getElementById("chain"),
        targetLine: document.getElementById("targetLine"),
        modeLine: document.getElementById("modeLine"),
        revisionLine: document.getElementById("revisionLine"),
        resolutionLine: document.getElementById("resolutionLine"),
        preset: document.getElementById("preset"),
        refresh: document.getElementById("refresh")
    };

    function requestId() {
        return "r" + Date.now().toString(36) + Math.floor(Math.random() * 1e6).toString(36);
    }

    function snippet(text) {
        var value = String(text);
        if (value.length === 0) return "(empty reply)";
        return value.length > 140 ? value.slice(0, 140) + "..." : value;
    }

    function extensionRoot() {
        try {
            if (window.__adobe_cep__ && typeof window.__adobe_cep__.getSystemPath === "function") {
                var path = window.__adobe_cep__.getSystemPath("extension");
                if (path) return path.replace(/\\/g, "/");
            }
        } catch (ignored) { /* fall back to whatever the host already loaded */ }
        return null;
    }

    // The manifest registers the gateway as this extension's ScriptPath, so CEP normally
    // evaluates it into the host engine when the extension loads. Depending on that alone
    // means a panel opened in a session where the script was not evaluated - or an
    // installed copy that lags the panel - can only report an unreadable reply. Loading
    // the gateway by path on the first call removes both cases, and it is why a panel
    // edit needs no After Effects restart, only a panel reload.
    var gatewayReady = false;

    function ensureGateway(callback) {
        if (gatewayReady) { callback(true); return; }
        evalScript("(typeof SFLD_ready === 'function') ? SFLD_ready() : 'missing'", function (probe) {
            if (String(probe).indexOf("org.starfieldfx.panel") === 0) { gatewayReady = true; callback(true); return; }
            var root = extensionRoot();
            if (!root) { callback(false); return; }
            evalScript("$.evalFile(" + quote(root + "/jsx/starfield_gateway.jsx") + ")", function () {
                evalScript("(typeof SFLD_ready === 'function') ? SFLD_ready() : 'missing'", function (second) {
                    gatewayReady = String(second).indexOf("org.starfieldfx.panel") === 0;
                    callback(gatewayReady);
                });
            });
        });
    }

    function call(operation, extra, callback) {
        ensureGateway(function (ready) {
            if (!ready) {
                callback({ ok: false, error: {
                    code: "gateway_missing",
                    message: "The ExtendScript gateway is not loaded. Check the install steps in cep_panel/README.md."
                } });
                return;
            }
            var envelope = {
                protocol: "org.starfieldfx.panel",
                version: 1,
                requestId: requestId(),
                operation: operation,
                target: state.targetToken ? { token: state.targetToken } : {},
                baseRevision: state.revision,
                changes: []
            };
            if (extra) {
                for (var key in extra) {
                    if (Object.prototype.hasOwnProperty.call(extra, key)) envelope[key] = extra[key];
                }
            }
            var script = "SFLD_" + operation + "(" + quote(JSON.stringify(envelope)) + ")";
            var settled = false;
            var timer = window.setTimeout(function () {
                if (settled) return;
                settled = true;
                callback({ ok: false, error: {
                    code: "host_timeout",
                    message: "The host did not answer within " + (REQUEST_TIMEOUT_MS / 1000) + "s."
                } });
            }, REQUEST_TIMEOUT_MS);
            evalScript(script, function (raw) {
                if (settled) return;
                settled = true;
                window.clearTimeout(timer);
                var response = null;
                try {
                    response = JSON.parse(raw);
                } catch (error) {
                    var replyText = raw === null || typeof raw === "undefined" ? "" : String(raw).trim();
                    if (!replyText || replyText === "undefined" || /^EvalScript error\.?$/i.test(replyText)) {
                        // During AE startup CEP can complete evalScript with no usable
                        // result before the project/ExtendScript context is ready.
                        gatewayReady = false;
                        callback({ ok: false, error: {
                            code: "host_not_ready", message: "After Effects has not returned a panel response yet."
                        } });
                        return;
                    }
                    // Non-empty malformed replies are useful diagnostics and should not be
                    // collapsed into the transient startup state handled above.
                    callback({ ok: false, error: {
                        code: "bad_response", message: "The gateway returned unreadable data: " + snippet(raw)
                    } });
                    return;
                }
                if (!response || response.protocol !== "org.starfieldfx.panel") {
                    callback({ ok: false, error: {
                        code: "bad_response", message: "Unexpected response envelope: " + snippet(raw)
                    } });
                    return;
                }
                callback(response);
            });
        });
    }

    function quote(text) {
        // ExtendScript-facing string literal; JSON already escapes quotes and
        // backslashes, so only the wrapping quotes are added here.
        return "\"" + text.replace(/\\/g, "\\\\").replace(/"/g, "\\\"") + "\"";
    }

    function showError(code, message) {
        elements.banner.className = "banner error";
        elements.banner.textContent = code + ": " + message;
    }

    function clearBanner() {
        elements.banner.className = "banner hidden";
        elements.banner.textContent = "";
    }

    function formatValue(binding, value) {
        if (binding.kind === "color") return value;
        if (binding.kind === "point3d") return value;
        return value;
    }

    function render(state) {
        elements.chain.innerHTML = "";
        state.values = {};
        for (var i = 0; i < state.nodes.length; i++) {
            var node = state.nodes[i];
            var box = document.createElement("section");
            box.className = "node";
            var head = document.createElement("div");
            head.className = "node-head";
            head.textContent = node.label;
            box.appendChild(head);
            var body = document.createElement("div");
            body.className = "node-body";
            if (!node.params.length) {
                var empty = document.createElement("div");
                empty.className = "empty";
                empty.textContent = "chain output";
                body.appendChild(empty);
            }
            for (var p = 0; p < node.params.length; p++) {
                body.appendChild(renderParameter(node.params[p]));
            }
            box.appendChild(body);
            elements.chain.appendChild(box);
            if (i < state.nodes.length - 1) {
                var connector = document.createElement("div");
                connector.className = "connector";
                connector.textContent = "↓ particles";
                elements.chain.appendChild(connector);
            }
        }
    }

    function renderParameter(parameter) {
        state.values[parameter.key] = parameter.value;
        var label = document.createElement("div");
        label.className = "param-label";
        label.textContent = parameter.label;
        label.title = parameter.key;
        var holder = document.createElement("div");
        holder.className = "param-value";

        if (parameter.kind === "color") {
            var swatch = document.createElement("span");
            swatch.className = "swatch";
            swatch.style.background = "rgb(" + parameter.value[0] + "," + parameter.value[1] + "," +
                                      parameter.value[2] + ")";
            holder.appendChild(swatch);
            for (var c = 0; c < 3; c++) {
                holder.appendChild(numberInput(parameter, c));
            }
        } else if (parameter.kind === "point3d") {
            for (var a = 0; a < 3; a++) {
                holder.appendChild(numberInput(parameter, a));
            }
        } else {
            holder.appendChild(numberInput(parameter, null));
        }
        var wrapper = document.createElement("div");
        wrapper.style.display = "contents";
        wrapper.appendChild(label);
        wrapper.appendChild(holder);
        return wrapper;
    }

    function numberInput(parameter, channel) {
        var input = document.createElement("input");
        input.type = "number";
        input.step = parameter.kind === "popup" ? "1" : "any";
        input.value = channel === null ? parameter.value : parameter.value[channel];
        input.dataset.key = parameter.key;
        input.dataset.channel = channel === null ? "" : String(channel);
        input.addEventListener("change", onEdit);
        return input;
    }

    function onEdit(event) {
        var input = event.target;
        var key = input.dataset.key;
        var channel = input.dataset.channel === "" ? null : Number(input.dataset.channel);
        var raw = Number(input.value);
        if (!isFinite(raw)) {
            showError("invalid_value", "Enter a finite number.");
            return;
        }
        var value = channel === null ? raw : null;
        if (channel !== null) {
            value = state.values[key].slice();
            value[channel] = raw;
        }
        applyChanges([{ key: key, value: value }]);
    }

    function applyChanges(changes) {
        if (state.pending) return;
        state.pending = true;
        call("setParameters", { changes: changes }, function (response) {
            state.pending = false;
            if (!response.ok) {
                var error = response.error || { code: "unknown", message: "Unknown failure." };
                showError(error.code, error.message);
                refresh();
                return;
            }
            clearBanner();
            adoptState(response);
        });
    }

    function adoptState(response) {
        state.nodes = response.nodes;
        state.revision = response.revision;
        state.targetToken = response.target.token;
        elements.targetLine.textContent = response.target.comp + " / " + response.target.layer;
        elements.modeLine.textContent = "Mode: " + response.controlSource;
        elements.revisionLine.textContent = "Revision: " + response.revision;
        elements.resolutionLine.textContent = "Lookup: " + response.resolution;
        render(response);
    }

    function isStartupRetryable(code) {
        return code === "gateway_missing" || code === "host_timeout" || code === "host_not_ready" || code === "no_host" ||
               code === "no_project" || code === "no_active_comp" || code === "no_target" || code === "no_effect";
    }

    function refresh(autoRetry, resetRetryBudget) {
        if (startupRetryTimer !== null) {
            window.clearTimeout(startupRetryTimer);
            startupRetryTimer = null;
        }
        if (resetRetryBudget) startupRetryAttempt = 0;
        var epoch = ++refreshEpoch;
        call("getState", null, function (response) {
            if (epoch !== refreshEpoch) return;
            if (!response.ok) {
                var error = response.error || { code: "unknown", message: "Unknown failure." };
                if (autoRetry && isStartupRetryable(error.code)) {
                    var retryIndex = Math.min(startupRetryAttempt, STARTUP_RETRY_DELAYS_MS.length - 1);
                    var delay = STARTUP_RETRY_DELAYS_MS[retryIndex];
                    startupRetryAttempt += 1;
                    elements.banner.className = "banner";
                    elements.banner.textContent = "Connecting to After Effects; retrying shortly...";
                    startupRetryTimer = window.setTimeout(function () {
                        startupRetryTimer = null;
                        if (epoch === refreshEpoch) refresh(true, false);
                    }, delay);
                    return;
                }
                showError(error.code, error.message);
                elements.targetLine.textContent = "No target";
                elements.chain.innerHTML = "<p class=\"hint\">Select one layer carrying Starfield Particle, then press Refresh.</p>";
                return;
            }
            startupRetryAttempt = 0;
            clearBanner();
            adoptState(response);
        });
    }

    // Prove the panel body executed, even when the host bridge is unavailable.
    elements.banner.className = "banner";
    elements.banner.textContent = "Panel script loaded; asking the host for the selected effect...";

    elements.refresh.addEventListener("click", function () { refresh(true, true); });

    // AE can change the effect behind the panel's back: Ctrl+Z, keyframes, or an edit in
    // the Effect Controls window. The protocol has no push channel, so without this the
    // panel keeps displaying whatever it last adopted (host-side undo left a stale 40 on
    // screen). Re-read whenever the panel regains focus, but never race a pending write.
    window.addEventListener("focus", function () {
        if (!state.pending) refresh(false, false);
    });

    elements.preset.addEventListener("change", function () {
        var preset = PRESETS[elements.preset.value];
        elements.preset.value = "";
        if (!preset) return;
        var changes = [];
        for (var key in preset) {
            if (Object.prototype.hasOwnProperty.call(preset, key)) changes.push({ key: key, value: preset[key] });
        }
        applyChanges(changes);
    });

    refresh(true, true);
})();
