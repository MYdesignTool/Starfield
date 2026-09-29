// Starfield Node Editor panel. Thin client for the ADR 0009 protocol v1: it draws
// the fixed emitter -> force -> appearance -> output graph, edits the supervised AE
// parameter streams through the ExtendScript gateway, and never touches the effect's
// arbitrary-data graph parameter or any host-private state.

(function () {
    "use strict";

    var REQUEST_TIMEOUT_MS = 8000;
    var STARTUP_RETRY_DELAYS_MS = [250, 750, 1500, 3000, 5000];
    var TARGET_POLL_INTERVAL_MS = 1200;
    var NODE_WIDTH = 220;
    var NODE_HEIGHT = 108;
    var CANVAS_MIN_WIDTH = 580;
    var CANVAS_MIN_HEIGHT = 320;
    var startupRetryAttempt = 0;
    var startupRetryTimer = null;
    var refreshEpoch = 0;
    var refreshInFlight = false;
    var resolvedTarget = false;
    var dragState = null;
    var inspectorDragState = null;
    var nodePositions = {};
    var graphNodeElements = {};
    var shownInspectorNodeId = null;

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

    var state = { revision: null, targetToken: null, nodes: [], edges: [], values: {},
                  selectedNodeId: "emitter", inspectorOpen: false, pending: false };
    var elements = {
        banner: document.getElementById("banner"),
        chain: document.getElementById("chain"),
        workspace: document.getElementById("workspace"),
        graphCanvas: document.querySelector ? document.querySelector(".graph-canvas") : null,
        graphEdges: document.getElementById("graphEdges"),
        edgePaths: document.getElementById("edgePaths"),
        inspectorTitle: document.getElementById("inspectorTitle"),
        inspectorMeta: document.getElementById("inspectorMeta"),
        inspectorBody: document.getElementById("inspectorBody"),
        inspector: document.getElementById("inspector"),
        inspectorDrag: document.getElementById("inspectorDrag"),
        closeInspector: document.getElementById("closeInspector"),
        targetLine: document.getElementById("targetLine"),
        modeLine: document.getElementById("modeLine"),
        revisionLine: document.getElementById("revisionLine"),
        resolutionLine: document.getElementById("resolutionLine"),
        preset: document.getElementById("preset"),
        refresh: document.getElementById("refresh"),
        autoRefresh: document.getElementById("autoRefresh")
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

    var NODE_KICKERS = {
        emitter: "01 / SOURCE", force: "02 / MOTION",
        appearance: "03 / LOOK", output: "04 / RESULT"
    };
    var DEFAULT_NODE_POSITIONS = {
        emitter: { x: 25, y: 22 }, force: { x: 335, y: 22 },
        appearance: { x: 335, y: 190 }, output: { x: 25, y: 190 }
    };

    function positionFor(node, index) {
        if (!nodePositions[node.id]) {
            var preset = DEFAULT_NODE_POSITIONS[node.id];
            nodePositions[node.id] = preset ? { x: preset.x, y: preset.y } : {
                x: 25 + (index % 2) * 310,
                y: 22 + Math.floor(index / 2) * 168
            };
        }
        return nodePositions[node.id];
    }

    function updateCanvasBounds() {
        if (!elements.graphCanvas) return;
        var width = CANVAS_MIN_WIDTH;
        var height = CANVAS_MIN_HEIGHT;
        for (var i = 0; i < state.nodes.length; i++) {
            if (!NODE_KICKERS[state.nodes[i].id]) continue;
            var position = positionFor(state.nodes[i], i);
            width = Math.max(width, position.x + NODE_WIDTH + 25);
            height = Math.max(height, position.y + NODE_HEIGHT + 22);
        }
        elements.graphCanvas.style.width = width + "px";
        elements.graphCanvas.style.height = height + "px";
        if (elements.graphEdges) elements.graphEdges.setAttribute("viewBox", "0 0 " + width + " " + height);
    }

    function chooseSide(dx, dy, fallback) {
        if (Math.abs(dx) > Math.abs(dy)) return dx >= 0 ? "right" : "left";
        if (Math.abs(dy) > 0) return dy >= 0 ? "bottom" : "top";
        return fallback;
    }

    function computePortSides() {
        var vectors = {};
        for (var i = 0; i < state.edges.length; i++) {
            var edge = state.edges[i];
            if (!edge || edge.length !== 2 || !nodePositions[edge[0]] || !nodePositions[edge[1]]) continue;
            var source = nodePositions[edge[0]];
            var destination = nodePositions[edge[1]];
            var sourceCenterX = source.x + NODE_WIDTH / 2;
            var sourceCenterY = source.y + NODE_HEIGHT / 2;
            var destinationCenterX = destination.x + NODE_WIDTH / 2;
            var destinationCenterY = destination.y + NODE_HEIGHT / 2;
            if (!vectors[edge[0]]) vectors[edge[0]] = { outX: 0, outY: 0, inX: 0, inY: 0 };
            if (!vectors[edge[1]]) vectors[edge[1]] = { outX: 0, outY: 0, inX: 0, inY: 0 };
            vectors[edge[0]].outX += destinationCenterX - sourceCenterX;
            vectors[edge[0]].outY += destinationCenterY - sourceCenterY;
            vectors[edge[1]].inX += sourceCenterX - destinationCenterX;
            vectors[edge[1]].inY += sourceCenterY - destinationCenterY;
        }
        var sides = {};
        for (var nodeId in nodePositions) {
            if (!Object.prototype.hasOwnProperty.call(nodePositions, nodeId)) continue;
            var vector = vectors[nodeId] || {};
            sides[nodeId] = {
                output: chooseSide(vector.outX || 0, vector.outY || 0, "right"),
                input: chooseSide(vector.inX || 0, vector.inY || 0, "left")
            };
        }
        return sides;
    }

    function portPoint(nodeId, side) {
        var position = nodePositions[nodeId];
        if (!position) return null;
        if (side === "right") return { x: position.x + NODE_WIDTH, y: position.y + NODE_HEIGHT / 2 };
        if (side === "left") return { x: position.x, y: position.y + NODE_HEIGHT / 2 };
        if (side === "bottom") return { x: position.x + NODE_WIDTH / 2, y: position.y + NODE_HEIGHT };
        return { x: position.x + NODE_WIDTH / 2, y: position.y };
    }

    function edgePath(edge, sides) {
        var startSide = sides[edge[0]] ? sides[edge[0]].output : "right";
        var endSide = sides[edge[1]] ? sides[edge[1]].input : "left";
        var start = portPoint(edge[0], startSide);
        var end = portPoint(edge[1], endSide);
        if (!start || !end) return null;
        var distance = Math.max(42, Math.min(180,
            (Math.abs(end.x - start.x) + Math.abs(end.y - start.y)) * 0.45));
        var vectors = {
            right: { x: 1, y: 0 }, left: { x: -1, y: 0 },
            bottom: { x: 0, y: 1 }, top: { x: 0, y: -1 }
        };
        var first = vectors[startSide];
        var second = vectors[endSide];
        return "M" + start.x + " " + start.y + " C" + (start.x + first.x * distance) + " " +
               (start.y + first.y * distance) + " " + (end.x + second.x * distance) + " " +
               (end.y + second.y * distance) + " " + end.x + " " + end.y;
    }

    function placePort(port, side) {
        if (!port) return;
        port.style.left = side === "left" ? "-6px" : side === "right" ? (NODE_WIDTH - 4) + "px" : "calc(50% - 5px)";
        port.style.top = side === "top" ? "-6px" : side === "bottom" ? (NODE_HEIGHT - 4) + "px" : "calc(50% - 5px)";
    }

    function parameterValue(node, key) {
        for (var i = 0; i < node.params.length; i++) {
            if (node.params[i].key === key) return node.params[i].value;
        }
        return null;
    }

    function shortNumber(value) {
        return typeof value === "number" ? String(Math.round(value * 100) / 100) : "–";
    }

    function nodeSummary(node) {
        if (node.id === "emitter") {
            return shortNumber(parameterValue(node, "birth_rate")) + "/s · max " +
                   shortNumber(parameterValue(node, "particle_count"));
        }
        if (node.id === "force") {
            return "Gravity Y " + shortNumber(parameterValue(node, "gravity_y")) +
                   " · drag " + shortNumber(parameterValue(node, "linear_drag"));
        }
        if (node.id === "appearance") {
            return "Size " + shortNumber(parameterValue(node, "particle_size")) +
                   " · opacity " + shortNumber(parameterValue(node, "opacity"));
        }
        return "Transparent particle output";
    }

    function addGraphNode(node) {
        var card = document.createElement("section");
        var position = positionFor(node, state.nodes.indexOf(node));
        card.className = "graph-node " + node.id +
                         (state.selectedNodeId === node.id ? " selected" : "");
        card.style.left = position.x + "px";
        card.style.top = position.y + "px";
        var portElements = { card: card, input: null, output: null };
        var button = document.createElement("button");
        button.type = "button";
        button.className = "node-select";
        button.setAttribute("aria-pressed", state.selectedNodeId === node.id ? "true" : "false");
        button.title = "Inspect " + node.label;
        var kicker = document.createElement("span");
        kicker.className = "node-kicker";
        kicker.textContent = NODE_KICKERS[node.id];
        var title = document.createElement("span");
        title.className = "node-title";
        title.textContent = node.label;
        var summary = document.createElement("span");
        summary.className = "node-summary";
        summary.textContent = nodeSummary(node);
        button.appendChild(kicker);
        button.appendChild(title);
        button.appendChild(summary);
        button.addEventListener("click", function () {
            state.selectedNodeId = node.id;
            state.inspectorOpen = true;
            render(state);
            refresh(false, false);
        });
        button.addEventListener("pointerdown", function (event) {
            beginNodeDrag(node.id, event, card);
        });
        card.appendChild(button);
        if (node.id !== "emitter") {
            var input = document.createElement("span");
            input.className = "port port-in";
            input.title = "Input";
            card.appendChild(input);
            portElements.input = input;
        }
        if (node.id !== "output") {
            var output = document.createElement("span");
            output.className = "port port-out";
            output.title = "Output";
            card.appendChild(output);
            portElements.output = output;
        }
        elements.chain.appendChild(card);
        graphNodeElements[node.id] = portElements;
    }

    function renderEdges() {
        if (!elements.edgePaths) return;
        elements.edgePaths.innerHTML = "";
        var sides = computePortSides();
        for (var nodeId in graphNodeElements) {
            if (!Object.prototype.hasOwnProperty.call(graphNodeElements, nodeId)) continue;
            var nodeSides = sides[nodeId] || { input: "left", output: "right" };
            placePort(graphNodeElements[nodeId].input, nodeSides.input);
            placePort(graphNodeElements[nodeId].output, nodeSides.output);
        }
        for (var i = 0; i < state.edges.length; i++) {
            var edge = state.edges[i];
            if (!edge || edge.length !== 2) continue;
            var pathData = edgePath(edge, sides);
            if (!pathData) continue;
            var path = document.createElementNS("http://www.w3.org/2000/svg", "path");
            path.setAttribute("class", "edge");
            path.setAttribute("d", pathData);
            elements.edgePaths.appendChild(path);
        }
    }

    function renderInspector() {
        if (!elements.inspectorBody || !elements.inspector) return;
        elements.inspector.hidden = !state.inspectorOpen;
        if (!state.inspectorOpen) { shownInspectorNodeId = null; return; }
        elements.inspectorBody.innerHTML = "";
        var node = null;
        for (var i = 0; i < state.nodes.length; i++) {
            if (state.nodes[i].id === state.selectedNodeId) node = state.nodes[i];
        }
        if (!node) {
            elements.inspectorTitle.textContent = "Inspector";
            elements.inspectorMeta.textContent = "Select a node";
            elements.inspectorBody.innerHTML = "<p class=\"hint\">Select one layer carrying Starfield Particle.</p>";
            shownInspectorNodeId = null;
            return;
        }
        elements.inspectorTitle.textContent = node.label;
        elements.inspectorMeta.textContent = node.params.length + " parameters";
        if (!node.params.length) {
            elements.inspectorBody.innerHTML = "<p class=\"inspector-note\">Particle output is transparent. This stage has no editable parameters in protocol v1.</p>";
            shownInspectorNodeId = node.id;
            positionInspector(node.id);
            return;
        }
        var grid = document.createElement("div");
        grid.className = "parameter-grid";
        for (var p = 0; p < node.params.length; p++) {
            grid.appendChild(renderParameter(node.params[p]));
        }
        elements.inspectorBody.appendChild(grid);
        if (shownInspectorNodeId !== node.id) positionInspector(node.id);
        shownInspectorNodeId = node.id;
    }

    function positionInspector(nodeId) {
        var position = nodePositions[nodeId];
        if (!position || !elements.workspace || !elements.inspector) return;
        var width = elements.workspace.clientWidth;
        var height = elements.workspace.clientHeight;
        var popupWidth = elements.inspector.offsetWidth;
        var popupHeight = elements.inspector.offsetHeight;
        var nodeOnRight = position.x + NODE_WIDTH / 2 > width / 2;
        var nodeInUpperHalf = position.y + NODE_HEIGHT / 2 < height / 2;
        var left = nodeOnRight ? 8 : width - popupWidth - 8;
        var top = nodeInUpperHalf ? height - popupHeight - 8 : 8;
        elements.inspector.style.left = Math.max(8, left) + "px";
        elements.inspector.style.top = Math.max(8, top) + "px";
    }

    function render(state) {
        elements.chain.innerHTML = "";
        graphNodeElements = {};
        state.values = {};
        if (!state.nodes.length) {
            elements.chain.innerHTML = "<p class=\"hint\">Select one layer carrying Starfield Particle.</p>";
            renderEdges();
            renderInspector();
            return;
        }
        var selectedExists = false;
        for (var i = 0; i < state.nodes.length; i++) {
            var node = state.nodes[i];
            for (var p = 0; p < node.params.length; p++) {
                state.values[node.params[p].key] = node.params[p].value;
            }
            if (node.id === state.selectedNodeId) selectedExists = true;
        }
        if (!selectedExists) state.selectedNodeId = state.nodes[0].id;
        updateCanvasBounds();
        for (var n = 0; n < state.nodes.length; n++) {
            if (NODE_KICKERS[state.nodes[n].id]) addGraphNode(state.nodes[n]);
        }
        renderEdges();
        renderInspector();
    }

    function beginNodeDrag(nodeId, event, card) {
        if (event.button !== 0 || event.isPrimary === false) return;
        var position = positionFor({ id: nodeId }, 0);
        dragState = { nodeId: nodeId, card: card, startX: event.clientX, startY: event.clientY,
                      originX: position.x, originY: position.y, moved: false };
    }

    function moveNodeDrag(event) {
        if (inspectorDragState) {
            var panelWidth = elements.workspace.clientWidth;
            var panelHeight = elements.workspace.clientHeight;
            var nextLeft = inspectorDragState.originX + event.clientX - inspectorDragState.startX;
            var nextTop = inspectorDragState.originY + event.clientY - inspectorDragState.startY;
            nextLeft = Math.max(8, Math.min(panelWidth - elements.inspector.offsetWidth - 8, nextLeft));
            nextTop = Math.max(8, Math.min(panelHeight - 36, nextTop));
            elements.inspector.style.left = nextLeft + "px";
            elements.inspector.style.top = nextTop + "px";
            if (event.preventDefault) event.preventDefault();
            return;
        }
        if (!dragState) return;
        var dx = event.clientX - dragState.startX;
        var dy = event.clientY - dragState.startY;
        if (!dragState.moved && Math.abs(dx) + Math.abs(dy) < 4) return;
        dragState.moved = true;
        if (document.body.classList) document.body.classList.add("dragging-node");
        if (event.preventDefault) event.preventDefault();
        var position = nodePositions[dragState.nodeId];
        position.x = Math.max(12, Math.min(12000, dragState.originX + dx));
        position.y = Math.max(12, Math.min(12000, dragState.originY + dy));
        dragState.card.style.left = position.x + "px";
        dragState.card.style.top = position.y + "px";
        updateCanvasBounds();
        renderEdges();
    }

    function endNodeDrag() {
        dragState = null;
        inspectorDragState = null;
        if (document.body.classList) document.body.classList.remove("dragging-node");
    }

    function beginInspectorDrag(event) {
        if (event.button !== 0 || event.isPrimary === false || !elements.inspector) return;
        if (event.target === elements.closeInspector) return;
        inspectorDragState = {
            startX: event.clientX,
            startY: event.clientY,
            originX: elements.inspector.offsetLeft,
            originY: elements.inspector.offsetTop
        };
    }

    function renderParameter(parameter) {
        var label = document.createElement("div");
        label.className = "param-label";
        label.textContent = parameter.label;
        label.title = parameter.key;
        var holder = document.createElement("div");
        holder.className = "param-value" +
                           (parameter.kind === "color" || parameter.kind === "point3d" ? " multi" : "");

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
        wrapper.className = "parameter-row";
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
        // A poll started before this write may return an older snapshot after the
        // write succeeds. Invalidate that response so only the write or a later
        // read can update the panel state.
        refreshEpoch += 1;
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
        var changed = state.targetToken !== response.target.token || state.revision !== response.revision;
        state.nodes = response.nodes;
        state.edges = response.edges || [];
        state.revision = response.revision;
        state.targetToken = response.target.token;
        resolvedTarget = true;
        elements.targetLine.textContent = response.target.comp + " / " + response.target.layer;
        elements.modeLine.textContent = "Mode: " + response.controlSource;
        elements.revisionLine.textContent = "Revision: " + response.revision;
        elements.resolutionLine.textContent = "Lookup: " + response.resolution;
        if (changed) render(state);
    }

    function isStartupRetryable(code) {
        return code === "gateway_missing" || code === "host_timeout" || code === "host_not_ready" || code === "no_host" ||
               code === "no_project" || code === "no_active_comp" || code === "no_target" || code === "no_effect";
    }

    function refresh(autoRetry, resetRetryBudget) {
        if (refreshInFlight || state.pending) return;
        if (startupRetryTimer !== null) {
            window.clearTimeout(startupRetryTimer);
            startupRetryTimer = null;
        }
        if (resetRetryBudget) startupRetryAttempt = 0;
        var epoch = ++refreshEpoch;
        refreshInFlight = true;
        call("getState", null, function (response) {
            refreshInFlight = false;
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
                state.revision = null;
                state.targetToken = null;
                state.nodes = [];
                state.edges = [];
                render(state);
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
    if (window.addEventListener) {
        window.addEventListener("focus", function () { refresh(false, false); });
        window.addEventListener("pointermove", moveNodeDrag);
        window.addEventListener("pointerup", endNodeDrag);
        window.addEventListener("pointercancel", endNodeDrag);
        window.addEventListener("resize", function () {
            if (state.inspectorOpen) positionInspector(state.selectedNodeId);
        });
    }
    if (elements.inspectorDrag) elements.inspectorDrag.addEventListener("pointerdown", beginInspectorDrag);
    if (elements.closeInspector) elements.closeInspector.addEventListener("click", function () {
        state.inspectorOpen = false;
        elements.inspector.hidden = true;
        shownInspectorNodeId = null;
    });
    if (window.setInterval) {
        window.setInterval(function () {
            if (!resolvedTarget || state.pending || document.hidden ||
                (elements.autoRefresh && !elements.autoRefresh.checked)) return;
            refresh(false, false);
        }, TARGET_POLL_INTERVAL_MS);
    }
    if (document.addEventListener) {
        document.addEventListener("visibilitychange", function () {
            if (!document.hidden && resolvedTarget &&
                (!elements.autoRefresh || elements.autoRefresh.checked)) refresh(false, false);
        });
    }
    if (elements.autoRefresh) elements.autoRefresh.addEventListener("change", function () {
        if (elements.autoRefresh.checked && resolvedTarget) refresh(false, false);
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
