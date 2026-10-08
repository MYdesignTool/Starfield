// Starfield Node Editor panel. Thin client for the ADR 0009 protocol v1: it draws
// the fixed emitter -> Particle -> force -> output view, edits the supervised AE
// parameter streams through the ExtendScript gateway, and never touches the effect's
// arbitrary-data graph parameter or any host-private state.

(function () {
    "use strict";

    var REQUEST_TIMEOUT_MS = 8000;
    var GATEWAY_BUILD = "native-presets-56";
    var GATEWAY_READY_TOKEN = "org.starfieldfx.panel/1/" + GATEWAY_BUILD;
    var openPresetsButton=document.getElementById("openPresets");
    if(openPresetsButton)openPresetsButton.addEventListener("click",function(){
        var bridge=window.__adobe_cep__;
        if(bridge && bridge.requestOpenExtension)bridge.requestOpenExtension("org.starfieldfx.panel.presets","");
    });
    var STARTUP_RETRY_DELAYS_MS = [250, 750, 1500, 3000, 5000];
    var ACTIVE_POLL_INTERVAL_MS = 500;
    var IDLE_POLL_INTERVAL_MS = 2000;
    var FULL_AUDIT_INTERVAL_MS = 15000;
    var NODE_WIDTH = 110;
    var NODE_HEIGHT = 54;
    var CANVAS_MIN_WIDTH = 580;
    var CANVAS_MIN_HEIGHT = 320;
    var ZOOM_MIN = 0.5;
    var ZOOM_MAX = 2.0;
    var PIN_STORAGE_KEY = "org.starfieldfx.panel.pinnedTarget.v1";
    var startupRetryAttempt = 0;
    var startupRetryTimer = null;
    var refreshEpoch = 0;
    var refreshInFlight = false;
    var resolvedTarget = false;
    var dragState = null;
    var inspectorDragState = null;
    var numericScrubState = null;
    var curveDragState = null;
    var selectedCurvePoints = { size: 0, opacity: 0 };
    var marqueeState = null;
    var panState = null;
    var connectionState = null;
    var edgePressState = null;
    var contextEdge = null;
    var contextGraphPoint = null;
    var nodePositions = {};
    var canvasOffset = { x: 0, y: 0 };
    var centerGraphOnNextRender = true;
    var graphNodeElements = {};
    var shownInspectorNodeId = null;
    var suppressNextNodeClick = false;
    var zoom = 1;
    var viewPan = { x: 0, y: 0 };
    var minimapPanState = null;
    var minimapTransform = null;
    var frameStatusInFlight = false;
    var latestFrameStatusResponse = null;
    var panelPulseInFlight = false, panelPollTimer = null, lastPulseStamp = null;
    var unchangedPulseCount = 0, lastFullRefreshAt = 0;
    var retainedError = null;
    var failedGraphInitializations = {};
    var resizeUpdatePending = false;
    var nodePalette = null;
    var observedPanelSize = null;

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

    // Surface load-time failures instead of showing a blank panel: a syntax error or an
    // exception during startup is exactly what "the panel is recognised but will not open"
    // looks like from the outside.
    window.onerror = function (message, source, line) {
        // Chromium reports skipped resize notifications as window errors. They
        // are not script exceptions and must not cover a native edit failure.
        if (/^ResizeObserver loop (limit exceeded|completed with undelivered notifications\.?$)/.test(String(message))) return true;
        try {
            var banner = document.getElementById("banner");
            if (banner) {
                showError("panel_script_error", message + " (" + source + ":" + line + ")", true);
            }
        } catch (ignored) { /* nothing else we can do */ }
        return false;
    };

    var pinnedTargetToken = null;
    try { pinnedTargetToken = window.localStorage.getItem(PIN_STORAGE_KEY) || null; }
    catch (ignored) { /* target pin remains available for this panel session */ }
    var state = { revision: null, targetToken: null, nodes: [], edges: [], values: {}, curves: null,
                  selectedNodeId: "emitter", selectedNodeIds: {}, inspectorOpen: false, pending: false,
                  pinnedTargetToken: pinnedTargetToken, layoutPersistence: false,
                  frameStatus: null, liveParticleCount: null, controlSource: "AE Controls",
                  graphMode: false, graphSnapshot: null, topologyReady: false };
    var graphTransactionClient = null;
    var elements = {
        banner: document.getElementById("banner"),
        chain: document.getElementById("chain"),
        workspace: document.getElementById("workspace"),
        graphCanvas: document.querySelector ? document.querySelector(".graph-canvas") : null,
        graphScroll: document.getElementById("graphScroll"),
        graphViewport: document.getElementById("graphViewport"),
        graphMinimap: document.getElementById("graphMinimap"),
        graphEdges: document.getElementById("graphEdges"),
        edgePaths: document.getElementById("edgePaths"),
        selectionBox: document.getElementById("selectionBox"),
        zoomReadout: document.getElementById("zoomReadout"),
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
        refresh: document.getElementById("refresh"),
        autoRefresh: document.getElementById("autoRefresh"),
        targetLock: document.getElementById("targetLock")
    };
    elements.contextMenu = document.getElementById("graphContextMenu");
    elements.disconnectContextAction = document.getElementById("disconnectContextAction");

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

    // Read operations can reuse the current generation. Every write reloads and
    // invokes the gateway in one host turn, so a different CEP page cannot replace
    // global entry points between a cached readiness probe and a mutation.
    function call(operation, extra, callback) {
        var root = extensionRoot();
        if (!root) {
            callback({ ok:false, error:{code:"gateway_missing",message:"CEP did not return an extension folder path."} });
            return;
        }
        var envelope = {
            protocol:"org.starfieldfx.panel",version:1,requestId:requestId(),
            operation:operation,
            target:(state.pinnedTargetToken || state.targetToken)?{token:state.pinnedTargetToken || state.targetToken}:{},
            pinTarget:!!state.pinnedTargetToken,baseRevision:state.revision,changes:[]
        };
        if (extra) for (var key in extra) {
            if (Object.prototype.hasOwnProperty.call(extra,key)) envelope[key]=extra[key];
        }
        envelope.gatewayBuild=GATEWAY_BUILD;
        var readOnly=operation==="getState" || operation==="getFrameStatus" || operation==="getGraphSnapshot" ||
            operation==="getPanelState" || operation==="getPanelPulse";
        var script="(function(){try{"+
            "if("+(readOnly?"false":"true")+" || typeof SFLD_ready!=='function' || SFLD_ready()!=="+quote(GATEWAY_READY_TOKEN)+
            "){$.evalFile(new File("+quote(root+"/jsx/starfield_gateway.jsx")+"));}"+
            "if(typeof SFLD_ready!=='function' || SFLD_ready()!=="+quote(GATEWAY_READY_TOKEN)+")throw new Error('Gateway generation mismatch.');"+
            "return SFLD_"+operation+"("+quote(JSON.stringify(envelope))+");"+
            "}catch(error){return JSON.stringify({protocol:'org.starfieldfx.panel',version:1,gatewayBuild:"+quote(GATEWAY_BUILD)+
            ",ok:false,error:{code:'gateway_load_failed',message:error.toString()}});}}())";
        var settled=false;
        var timer=window.setTimeout(function(){
            if(settled)return;settled=true;
            callback({ok:false,error:{code:"host_timeout",message:"The host did not answer within "+(REQUEST_TIMEOUT_MS/1000)+"s."}});
        },REQUEST_TIMEOUT_MS);
        evalScript(script,function(raw){
            if(settled)return;settled=true;window.clearTimeout(timer);
            var response;
            try {response=JSON.parse(raw);} catch(error){
                var text=raw===null || typeof raw==="undefined"?"":String(raw).trim();
                if(!text || text==="undefined" || /^EvalScript error\.?$/i.test(text)){
                    callback({ok:false,error:{code:"host_not_ready",message:"After Effects has not returned a panel response yet."}});
                } else callback({ok:false,error:{code:"bad_response",message:"The gateway returned unreadable data: "+snippet(raw)}});
                return;
            }
            if(response && response.ok===false && response.error && response.error.code==="no_host"){callback(response);return;}
            if(!response || response.protocol!=="org.starfieldfx.panel" || response.version!==1 || response.gatewayBuild!==GATEWAY_BUILD){
                callback({ok:false,error:{code:"gateway_generation_mismatch",message:"The node panel and host gateway versions do not match. Reopen the panel."}});
                return;
            }
            callback(window.StarfieldNativeGraphSnapshot?window.StarfieldNativeGraphSnapshot.normalize(response):response);
        });
    }

    function quote(text) {
        // ExtendScript-facing string literal; JSON already escapes quotes and
        // backslashes, so only the wrapping quotes are added here.
        return "\"" + text.replace(/\\/g, "\\\\").replace(/"/g, "\\\"") + "\"";
    }

    function showError(code, message, retain) {
        // Background inspection must not erase or replace a failed user edit.
        if (retainedError && !retain) return;
        if (retain) retainedError = { code: code, message: message };
        elements.banner.className = "banner error";
        elements.banner.textContent = code + ": " + message;
    }

    function clearBanner(acknowledge) {
        if (retainedError && !acknowledge) return;
        retainedError = null;
        elements.banner.className = "banner hidden";
        elements.banner.textContent = "";
    }

    function syncTargetLock() {
        if (!elements.targetLock) return;
        var locked = !!state.pinnedTargetToken;
        elements.targetLock.textContent = locked ? "Pinned" : "Pin Target";
        elements.targetLock.setAttribute("aria-pressed", locked ? "true" : "false");
        elements.targetLock.disabled = !locked && !state.targetToken;
        elements.targetLock.title = locked ? "Unlock target and follow the current AE selection" :
            "Keep this effect targeted when AE selection changes";
    }

    var NODE_TYPES = {
        emitter: true, particle: true, force: true, transform: true, output: true
    };
    var DEFAULT_NODE_POSITIONS = {
        emitter: { x: 235, y: 22 }, particle: { x: 235, y: 100 },
        force: { x: 235, y: 178 }, transform: {x:235,y:220}, output: { x: 235, y: 256 }
    };

    function positionFor(node, index) {
        if (!nodePositions[node.id]) {
            var preset = node.position || DEFAULT_NODE_POSITIONS[node.id];
            nodePositions[node.id] = preset ? { x: preset.x, y: preset.y } : {
                x: (CANVAS_MIN_WIDTH - NODE_WIDTH) / 2,
                y: 22 + index * 78
            };
        }
        return nodePositions[node.id];
    }

    function nodeKind(node) {
        return node && (node.kind || node.id);
    }

    function isSupportedNode(node) {
        return !!NODE_TYPES[nodeKind(node)];
    }

    function displayPosition(position) {
        return { x: position.x + canvasOffset.x, y: position.y + canvasOffset.y };
    }

    function resetViewForTarget() {
        nodePositions = {};
        canvasOffset = { x: 0, y: 0 };
        viewPan = { x: 0, y: 0 };
        zoom = 1;
        selectedCurvePoints = { size: 0, opacity: 0 };
        curveDragState = null;
        if (document.body && document.body.classList) document.body.classList.remove("dragging-age-curve");
        centerGraphOnNextRender = true;
    }

    function applyProjectNodeLayout(layout) {
        if (!layout || typeof layout !== "object") return false;
        var changed = false;
        for (var nodeId in DEFAULT_NODE_POSITIONS) {
            if (!Object.prototype.hasOwnProperty.call(DEFAULT_NODE_POSITIONS, nodeId)) continue;
            var position = layout[nodeId];
            if (!position || !isFinite(Number(position.x)) || !isFinite(Number(position.y))) continue;
            var x = Number(position.x);
            var y = Number(position.y);
            if (!nodePositions[nodeId] || nodePositions[nodeId].x !== x || nodePositions[nodeId].y !== y) changed = true;
            nodePositions[nodeId] = { x: x, y: y };
        }
        return changed;
    }

    function centerGraphView() {
        if (!elements.graphScroll || !state.nodes.length) return;
        var left = Infinity;
        var top = Infinity;
        var right = -Infinity;
        var bottom = -Infinity;
        for (var i = 0; i < state.nodes.length; i++) {
            if (!isSupportedNode(state.nodes[i])) continue;
            var position = displayPosition(positionFor(state.nodes[i], i));
            left = Math.min(left, position.x);
            top = Math.min(top, position.y);
            right = Math.max(right, position.x + NODE_WIDTH);
            bottom = Math.max(bottom, position.y + NODE_HEIGHT);
        }
        if (left === Infinity) return;
        viewPan.x = (elements.graphScroll.clientWidth - (right - left) * zoom) / 2 - left * zoom;
        viewPan.y = (elements.graphScroll.clientHeight - (bottom - top) * zoom) / 2 - top * zoom;
    }

    function canvasInteractionActive(ignorePalette) {
        return !!(dragState || panState || marqueeState || connectionState || edgePressState || minimapPanState || inspectorDragState || curveDragState ||
                  numericScrubState || (!ignorePalette && nodePalette && nodePalette.isDragging()) || (elements.contextMenu && !elements.contextMenu.hidden));
    }

    function commitNodeLayout() {
        if (!state.layoutPersistence || !state.targetToken || !state.revision || state.pending || !state.nodes.length) return;
        if (state.graphMode) {
            var positions = {};
            for (var dynamicIndex = 0; dynamicIndex < state.nodes.length; dynamicIndex++) {
                var dynamicNode = state.nodes[dynamicIndex];
                var dynamicPosition = positionFor(dynamicNode, dynamicIndex);
                positions[dynamicNode.id] = { x: Number(dynamicPosition.x), y: Number(dynamicPosition.y) };
            }
            applyTopologyEdit({ type: "moveNodes", positions: positions });
            return;
        }
        var layout = {};
        for (var i = 0; i < state.nodes.length; i++) {
            var nodeId = state.nodes[i].id;
            if (!NODE_TYPES[nodeId]) continue;
            var position = positionFor(state.nodes[i], i);
            layout[nodeId] = { x: Number(position.x), y: Number(position.y) };
        }
        refreshEpoch += 1;
        state.pending = true;
        call("setNodeLayout", { layout: layout }, function (response) {
            state.pending = false;
            if (response.ok) {
                clearBanner();
                adoptState(response);
                return;
            }
            var error = response.error || { code: "unknown", message: "Unknown failure." };
            call("getState", null, function (latest) {
                if (latest.ok) adoptState(latest);
                showError(error.code, error.message);
            });
        });
    }

    function drawMinimap() {
        var canvas = elements.graphMinimap;
        if (!canvas || !canvas.getContext || !elements.graphScroll) return;
        var context = canvas.getContext("2d");
        if (!context) return;
        var width = canvas.width;
        var height = canvas.height;
        var padding = 9;
        var viewportLeft = -viewPan.x / zoom;
        var viewportTop = -viewPan.y / zoom;
        var viewportRight = viewportLeft + elements.graphScroll.clientWidth / zoom;
        var viewportBottom = viewportTop + elements.graphScroll.clientHeight / zoom;
        var bounds = { left: viewportLeft, top: viewportTop,
                       right: viewportRight, bottom: viewportBottom };
        var colors = { emitter: "#e5aa69", particle: "#b7a0e9", force: "#94a9ed", transform:"#8dc9dd",
                       output: "#83c9b1" };
        for (var i = 0; i < state.nodes.length; i++) {
            if (!isSupportedNode(state.nodes[i])) continue;
            var position = displayPosition(positionFor(state.nodes[i], i));
            bounds.left = Math.min(bounds.left, position.x);
            bounds.top = Math.min(bounds.top, position.y);
            bounds.right = Math.max(bounds.right, position.x + NODE_WIDTH);
            bounds.bottom = Math.max(bounds.bottom, position.y + NODE_HEIGHT);
        }
        var rangeWidth = Math.max(1, bounds.right - bounds.left);
        var rangeHeight = Math.max(1, bounds.bottom - bounds.top);
        var scale = Math.min((width - padding * 2) / rangeWidth, (height - padding * 2) / rangeHeight);
        var mapWidth = rangeWidth * scale;
        var mapHeight = rangeHeight * scale;
        var offsetX = (width - mapWidth) / 2;
        var offsetY = (height - mapHeight) / 2;
        minimapTransform = { left: bounds.left, top: bounds.top, scale: scale,
                             offsetX: offsetX, offsetY: offsetY };

        context.clearRect(0, 0, width, height);
        context.fillStyle = "rgba(32,34,39,0.94)";
        context.fillRect(0, 0, width, height);
        context.save();
        context.beginPath();
        context.rect(padding, padding, width - padding * 2, height - padding * 2);
        context.clip();
        for (var n = 0; n < state.nodes.length; n++) {
            var node = state.nodes[n];
            if (!isSupportedNode(node)) continue;
            var nodePosition = displayPosition(positionFor(node, n));
            var nodeX = offsetX + (nodePosition.x - bounds.left) * scale;
            var nodeY = offsetY + (nodePosition.y - bounds.top) * scale;
            context.fillStyle = colors[nodeKind(node)] || "#aab2bf";
            context.fillRect(nodeX, nodeY, Math.max(3, NODE_WIDTH * scale), Math.max(2, NODE_HEIGHT * scale));
        }
        var viewX = offsetX + (viewportLeft - bounds.left) * scale;
        var viewY = offsetY + (viewportTop - bounds.top) * scale;
        var viewWidth = (viewportRight - viewportLeft) * scale;
        var viewHeight = (viewportBottom - viewportTop) * scale;
        context.fillStyle = "rgba(116,164,235,0.12)";
        context.fillRect(viewX, viewY, viewWidth, viewHeight);
        context.strokeStyle = "#c5d9fa";
        context.lineWidth = 1.5;
        context.strokeRect(viewX + 0.75, viewY + 0.75, Math.max(0, viewWidth - 1.5), Math.max(0, viewHeight - 1.5));
        context.restore();
        context.strokeStyle = "#596170";
        context.lineWidth = 1;
        context.strokeRect(0.5, 0.5, width - 1, height - 1);
    }

    function moveViewToMinimapPoint(event) {
        if (!minimapTransform || !elements.graphMinimap || !elements.graphScroll) return;
        var rect = elements.graphMinimap.getBoundingClientRect();
        var mapX = (event.clientX - rect.left) * (elements.graphMinimap.width / rect.width);
        var mapY = (event.clientY - rect.top) * (elements.graphMinimap.height / rect.height);
        var pointX = minimapTransform.left + (mapX - minimapTransform.offsetX) / minimapTransform.scale;
        var pointY = minimapTransform.top + (mapY - minimapTransform.offsetY) / minimapTransform.scale;
        viewPan.x = elements.graphScroll.clientWidth / 2 - pointX * zoom;
        viewPan.y = elements.graphScroll.clientHeight / 2 - pointY * zoom;
        updateCanvasBounds();
    }

    function updateCanvasBounds() {
        if (!elements.graphCanvas) return;
        var width = CANVAS_MIN_WIDTH;
        var height = CANVAS_MIN_HEIGHT;
        var minX = Infinity;
        var minY = Infinity;
        var maxX = -Infinity;
        var maxY = -Infinity;
        for (var i = 0; i < state.nodes.length; i++) {
            if (!isSupportedNode(state.nodes[i])) continue;
            var position = positionFor(state.nodes[i], i);
            minX = Math.min(minX, position.x);
            minY = Math.min(minY, position.y);
            maxX = Math.max(maxX, position.x + NODE_WIDTH);
            maxY = Math.max(maxY, position.y + NODE_HEIGHT);
        }
        if (dragState && dragState.copy && dragState.moved) {
            var copyDx = (dragState.lastX - dragState.startX) / zoom;
            var copyDy = (dragState.lastY - dragState.startY) / zoom;
            for (var copyIndex = 0; copyIndex < dragState.origins.length; copyIndex++) {
                var copyOrigin = dragState.origins[copyIndex];
                minX = Math.min(minX, copyOrigin.x + copyDx);
                minY = Math.min(minY, copyOrigin.y + copyDy);
                maxX = Math.max(maxX, copyOrigin.x + copyDx + NODE_WIDTH);
                maxY = Math.max(maxY, copyOrigin.y + copyDy + NODE_HEIGHT);
            }
        }
        if (minX === Infinity) { minX = 0; minY = 0; maxX = CANVAS_MIN_WIDTH; maxY = CANVAS_MIN_HEIGHT; }
        // Keep the canvas dimensions proportional to the node spread, not their
        // absolute graph coordinates. This supports long drags in every direction
        // without Chromium's large-element limit becoming an invisible boundary.
        var nextOffsetX = 24 - minX;
        var nextOffsetY = 24 - minY;
        viewPan.x -= (nextOffsetX - canvasOffset.x) * zoom;
        viewPan.y -= (nextOffsetY - canvasOffset.y) * zoom;
        canvasOffset = { x: nextOffsetX, y: nextOffsetY };
        width = Math.max(width, maxX + canvasOffset.x + 25);
        height = Math.max(height, maxY + canvasOffset.y + 22);
        elements.graphCanvas.style.width = width + "px";
        elements.graphCanvas.style.height = height + "px";
        elements.graphCanvas.style.transform = "translate(" + viewPan.x + "px," + viewPan.y + "px) scale(" + zoom + ")";
        for (var nodeId in graphNodeElements) {
            if (!Object.prototype.hasOwnProperty.call(graphNodeElements, nodeId) || !nodePositions[nodeId]) continue;
            var nodePosition = displayPosition(nodePositions[nodeId]);
            graphNodeElements[nodeId].card.style.left = nodePosition.x + "px";
            graphNodeElements[nodeId].card.style.top = nodePosition.y + "px";
        }
        if (dragState && dragState.copy && dragState.previewElements.length) {
            var previewDx = (dragState.lastX - dragState.startX) / zoom;
            var previewDy = (dragState.lastY - dragState.startY) / zoom;
            for (var previewIndex = 0; previewIndex < dragState.previewElements.length; previewIndex++) {
                var previewOrigin = dragState.origins[previewIndex];
                var previewPosition = displayPosition({ x: previewOrigin.x + previewDx, y: previewOrigin.y + previewDy });
                dragState.previewElements[previewIndex].style.left = previewPosition.x + "px";
                dragState.previewElements[previewIndex].style.top = previewPosition.y + "px";
            }
        }
        if (elements.graphViewport) {
            elements.graphViewport.style.width = (elements.graphScroll ? elements.graphScroll.clientWidth : width) + "px";
            elements.graphViewport.style.height = (elements.graphScroll ? elements.graphScroll.clientHeight : height) + "px";
        }
        if (elements.graphScroll) {
            var gridSize = 16 * zoom;
            elements.graphScroll.style.backgroundSize = gridSize + "px " + gridSize + "px";
            elements.graphScroll.style.backgroundPosition =
                (((viewPan.x % gridSize) + gridSize) % gridSize) + "px " +
                (((viewPan.y % gridSize) + gridSize) % gridSize) + "px";
            elements.graphScroll.scrollLeft = 0;
            elements.graphScroll.scrollTop = 0;
        }
        if (elements.graphEdges) elements.graphEdges.setAttribute("viewBox", "0 0 " + width + " " + height);
        if (elements.zoomReadout) elements.zoomReadout.textContent = Math.round(zoom * 100) + "%";
        drawMinimap();
    }

    function syncNodeSelectionStyles() {
        for (var nodeId in graphNodeElements) {
            if (!Object.prototype.hasOwnProperty.call(graphNodeElements, nodeId)) continue;
            var card = graphNodeElements[nodeId].card;
            var selected = !!state.selectedNodeIds[nodeId];
            card.className = "graph-node " + nodeKind(graphNodeElements[nodeId].node) +
                             (selected ? " selected" : "");
            var button = card.querySelector(".node-select");
            if (button) button.setAttribute("aria-pressed", state.selectedNodeId === nodeId ? "true" : "false");
        }
    }

    function canvasPoint(clientX, clientY) {
        var bounds = elements.graphScroll.getBoundingClientRect();
        return { x: (clientX - bounds.left - viewPan.x) / zoom,
                 y: (clientY - bounds.top - viewPan.y) / zoom };
    }

    function computePortSides() {
        var sides = {};
        for (var nodeId in graphNodeElements) {
            if (!Object.prototype.hasOwnProperty.call(graphNodeElements, nodeId)) continue;
            sides[nodeId] = { input: "top", output: "bottom" };
        }
        return sides;
    }

    function portPoint(nodeId, side) {
        var position = nodePositions[nodeId];
        if (!position) return null;
        var display = displayPosition(position);
        if (side === "bottom") return { x: display.x + NODE_WIDTH / 2, y: display.y + NODE_HEIGHT };
        return { x: display.x + NODE_WIDTH / 2, y: display.y };
    }

    function edgePath(edge, sides) {
        var startSide = sides[edge[0]] ? sides[edge[0]].output : "bottom";
        var endSide = sides[edge[1]] ? sides[edge[1]].input : "top";
        var start = portPoint(edge[0], startSide);
        var end = portPoint(edge[1], endSide);
        if (!start || !end) return null;
        var distance = Math.max(12, Math.min(80, Math.abs(end.y - start.y) * 0.45));
        return "M" + start.x + " " + start.y + " C" + start.x + " " + (start.y + distance) + " " +
               end.x + " " + (end.y - distance) + " " + end.x + " " + end.y;
    }

    function placePort(port, side) {
        if (!port) return;
        port.style.left = "calc(50% - 5px)";
        port.style.top = side === "top" ? "-6px" : side === "bottom" ? (NODE_HEIGHT - 4) + "px" : "calc(50% - 5px)";
    }

    function parameterValue(node, key) {
        for (var i = 0; i < node.params.length; i++) {
            if (node.params[i].key === key || node.params[i].legacyKey === key) return node.params[i].value;
        }
        return null;
    }

    function graphParameterValue(node, key) {
        for (var i = 0; i < node.params.length; i++) {
            if (node.params[i].graphKey === String(key)) return node.params[i].value;
        }
        return null;
    }

    function shortNumber(value) {
        return typeof value === "number" ? String(Math.round(value * 100) / 100) : "–";
    }

    function nodeSummary(node) {
        var kind = nodeKind(node);
        if (kind === "emitter") {
            return shortNumber(parameterValue(node, "birth_rate") || graphParameterValue(node, 2)) + "/s";
        }
        if (kind === "force") {
            var gravity = graphParameterValue(node, 1);
            return "Gravity " + shortNumber(gravity) + " · air " + shortNumber(graphParameterValue(node, 2));
        }
        if (kind === "particle") {
            return "Size " + shortNumber(parameterValue(node, "particle_size") || graphParameterValue(node, 3)) +
                   " · opacity " + shortNumber(parameterValue(node, "opacity") || graphParameterValue(node, 5)) + (state.graphMode ? "%" : "");
        }
        if (kind === "transform") return "Scale "+shortNumber(graphParameterValue(node,5))+"% · opacity "+shortNumber(graphParameterValue(node,6))+"%";
        var status = state.frameStatus;
        if (status && status.available === false) return "Live count unavailable";
        var maxParticles = status ? status.maxParticles :
            (node.maxParticles !== undefined ? node.maxParticles : parameterValue(node, "particle_count"));
        var live = state.liveParticleCount === null ? "–" : String(state.liveParticleCount);
        return "Live " + live + " · Max " + shortNumber(maxParticles);
    }

    function updateFrameStatus() {
        if (frameStatusInFlight || panelPulseInFlight || !resolvedTarget || !state.targetToken || state.pending ||
            refreshInFlight || !state.nodes.length) return;
        frameStatusInFlight = true;
        call("getFrameStatus", null, function (response) {
            frameStatusInFlight = false;
            adoptFrameStatus(response);
        });
    }

    function adoptFrameStatus(response) {
            if (!response || !response.ok || response.targetToken !== state.targetToken) return;
            latestFrameStatusResponse = response;
            var status = response;
            if (state.graphMode) {
                var emission = window.StarfieldGraphView.activeEmitterParameters(state.graph);
                if (!emission) {
                    status = { available: false, targetToken: response.targetToken,
                               timeSeconds: response.timeSeconds };
                } else {
                    status = { available: true, targetToken: response.targetToken,
                               timeSeconds: response.timeSeconds, emission: emission,
                               maxParticles: emission.maxParticles };
                }
            }
            state.frameStatus = status;
            state.liveParticleCount = status.available === false ? null :
                state.graphMode ? window.StarfieldGraphView.countGraphLiveParticles(status.timeSeconds, status.emission) :
                window.StarfieldGraphView.countLiveParticles(status.timeSeconds, status.birthRate,
                                                            status.lifetimeSeconds, status.maxParticles,
                                                            status.branchLifetimes);
            var output = null;
            for (var i = 0; i < state.nodes.length; i++) {
                if (nodeKind(state.nodes[i]) === "output") { output = graphNodeElements[state.nodes[i].id]; break; }
            }
            if (output && output.summary && output.node) output.summary.textContent = nodeSummary(output.node);
    }

    var effectSelectionInFlight = false;
    var queuedEffectSelection = null;
    function selectNativeEffect(nodeId) {
        queuedEffectSelection = {id:nodeId, target:state.targetToken};
        flushEffectSelection();
    }
    function flushEffectSelection() {
        if (effectSelectionInFlight || !queuedEffectSelection) return;
        if (state.pending) { window.setTimeout(flushEffectSelection, 100); return; }
        var selection = queuedEffectSelection;
        queuedEffectSelection = null;
        if (!selection.target || selection.target !== state.targetToken) return;
        effectSelectionInFlight = true;
        call("selectNodeEffect", {nodeId:selection.id}, function (response) {
            effectSelectionInFlight = false;
            if (!response.ok && !queuedEffectSelection && selection.target === state.targetToken) {
                showError(response.error.code, response.error.message);
            }
            flushEffectSelection();
        });
    }

    function addGraphNode(node) {
        var card = document.createElement("section");
        var position = positionFor(node, state.nodes.indexOf(node));
        var display = displayPosition(position);
        card.setAttribute("data-node-id", node.id);
        card.className = "graph-node " + nodeKind(node) +
                         (state.selectedNodeIds[node.id] ? " selected" : "");
        card.style.left = display.x + "px";
        card.style.top = display.y + "px";
        var portElements = { card: card, node: node, input: null, output: null, summary: null };
        var button = document.createElement("button");
        button.type = "button";
        button.className = "node-select";
        button.setAttribute("aria-pressed", state.selectedNodeId === node.id ? "true" : "false");
        button.title = "Inspect " + node.label;
        var title = document.createElement("span");
        title.className = "node-title";
        title.textContent = node.label;
        var summary = document.createElement("span");
        summary.className = "node-summary";
        summary.textContent = nodeSummary(node);
        portElements.summary = summary;
        button.appendChild(title);
        button.appendChild(summary);
        button.addEventListener("click", function (event) {
            if (suppressNextNodeClick) { suppressNextNodeClick = false; return; }
            if (event.shiftKey || event.ctrlKey) {
                if (state.selectedNodeIds[node.id]) delete state.selectedNodeIds[node.id];
                else state.selectedNodeIds[node.id] = true;
            } else {
                state.selectedNodeIds = {};
                state.selectedNodeIds[node.id] = true;
            }
            state.selectedNodeId = node.id;
            state.inspectorOpen = true;
            selectNativeEffect(node.id);
            syncNodeSelectionStyles();
            renderInspector();
        });
        button.addEventListener("pointerdown", function (event) {
            beginNodeDrag(node.id, event);
        });
        card.appendChild(button);
        if (node.inputPort !== null && node.inputPort !== undefined ? node.inputPort !== false : nodeKind(node) !== "emitter") {
            var input = document.createElement("span");
            input.className = "port port-in";
            input.title = "Input";
            input.setAttribute("data-node-id", node.id);
            input.setAttribute("data-port-direction", "in");
            input.setAttribute("data-port-key", node.inputPort || "1");
            input.addEventListener("pointerdown", function (event) { beginPortDrag(node.id, "in", event); });
            card.appendChild(input);
            portElements.input = input;
        }
        if (node.outputPort !== null && node.outputPort !== undefined ? node.outputPort !== false : nodeKind(node) !== "output") {
            var output = document.createElement("span");
            output.className = "port port-out";
            output.title = "Output";
            output.setAttribute("data-node-id", node.id);
            output.setAttribute("data-port-direction", "out");
            output.setAttribute("data-port-key", node.outputPort || (nodeKind(node) === "emitter" ? "1" : "2"));
            output.addEventListener("pointerdown", function (event) { beginPortDrag(node.id, "out", event); });
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
            var nodeSides = sides[nodeId] || { input: "top", output: "bottom" };
            placePort(graphNodeElements[nodeId].input, nodeSides.input);
            placePort(graphNodeElements[nodeId].output, nodeSides.output);
        }
        for (var i = 0; i < state.edges.length; i++) {
            var edge = state.edges[i];
            if (!edge || edge.length !== 2) continue;
            var pathData = edgePath(edge, sides);
            if (!pathData) continue;
            var path = document.createElementNS("http://www.w3.org/2000/svg", "path");
            path.setAttribute("class", "edge-hit");
            path.setAttribute("d", pathData);
            path.setAttribute("data-from", edge[0]);
            path.setAttribute("data-to", edge[1]);
            if (edge.id) path.setAttribute("data-edge-id", edge.id);
            path.setAttribute("tabindex", "0");
            path.setAttribute("role", "button");
            path.setAttribute("aria-label", "Disconnect " + edge[0] + " from " + edge[1]);
            path.addEventListener("pointerdown", function (event) {
                if (event.button !== 0 || event.isPrimary === false || state.pending) return;
                event.preventDefault();
                event.stopPropagation();
                capturePointer(event);
                edgePressState = { pointerId: event.pointerId, startX: event.clientX, startY: event.clientY,
                                   edit: edgeDisconnectEdit(this) };
            });
            path.addEventListener("click", function (event) {
                // Physical clicks commit on pointerup; synthetic/accessibility
                // activation has no pointer sequence and commits here.
                if (event && event.detail > 0) return;
                requestTopologyEdit(edgeDisconnectEdit(this));
            });
            path.addEventListener("keydown", function (event) {
                if (event.key === "Enter" || event.key === " ") {
                    event.preventDefault();
                    requestTopologyEdit(edgeDisconnectEdit(this));
                }
            });
            elements.edgePaths.appendChild(path);
            var visible = document.createElementNS("http://www.w3.org/2000/svg", "path");
            visible.setAttribute("class", "edge");
            visible.setAttribute("d", pathData);
            visible.setAttribute("aria-hidden", "true");
            elements.edgePaths.appendChild(visible);
        }
        if (connectionState) {
            var currentSides = computePortSides();
            var side = currentSides[connectionState.nodeId] || {};
            var fixedSide = connectionState.direction === "out" ? side.output : side.input;
            var fixedPoint = portPoint(connectionState.nodeId, fixedSide ||
                (connectionState.direction === "out" ? "bottom" : "top"));
            if (fixedPoint) {
                var start = connectionState.direction === "out" ? fixedPoint : connectionState.point;
                var end = connectionState.direction === "out" ? connectionState.point : fixedPoint;
                var bend = Math.max(12, Math.min(80, Math.abs(end.y - start.y) * 0.45));
                var c1x = start.x;
                var c1y = start.y + bend;
                var c2x = end.x;
                var c2y = end.y - bend;
                var preview = document.createElementNS("http://www.w3.org/2000/svg", "path");
                preview.setAttribute("class", "edge-preview");
                preview.setAttribute("d", "M" + start.x + " " + start.y + " C" + c1x + " " + c1y + " " +
                    c2x + " " + c2y + " " + end.x + " " + end.y);
                preview.setAttribute("aria-hidden", "true");
                elements.edgePaths.appendChild(preview);
            }
        }
    }

    function requestTopologyEdit(edit) {
        if (!state.targetToken || !state.revision || state.pending) return;
        applyTopologyEdit(edit);
    }

    function edgeDisconnectEdit(path) {
        return { type: "disconnect", from: path.getAttribute("data-from"),
                 to: path.getAttribute("data-to"), edgeId: path.getAttribute("data-edge-id") || undefined };
    }

    function loadGraphSnapshot(targetToken, callback, currentResponse) {
        if (!window.StarfieldGraphCodec || !window.StarfieldGraphView ||
            !window.StarfieldGraphTransactions || !window.StarfieldGraphEdits) {
            callback({ ok: false, error: { code: "graph_modules_missing", message: "The graph editor modules did not load." } });
            return;
        }
        function received(response) {
            if (!response || !response.ok || !response.snapshot) { callback(response); return; }
            if (response.snapshot.initialized) { callback(response); return; }
            var failed = failedGraphInitializations[targetToken];
            if (failed) { callback(failed); return; }
            call("syncGraphSnapshot", { target: { token: targetToken } }, function (initialized) {
                if (!initialized || !initialized.ok || !initialized.snapshot || !initialized.snapshot.initialized) {
                    failed = initialized && !initialized.ok ? initialized : { ok: false, error: {
                        code: "graph_snapshot_unconfirmed", message: "The project graph could not be initialized." } };
                    failedGraphInitializations[targetToken] = failed;
                    callback(failed);
                    return;
                }
                callback(initialized);
            });
        }
        if (currentResponse) received(currentResponse);
        else call("getGraphSnapshot", { target: { token: targetToken } }, received);
    }

    function getGraphTransactionClient() {
        if (!graphTransactionClient && window.StarfieldGraphTransactions && window.StarfieldGraphCodec &&
            window.StarfieldGraphEdits) {
            graphTransactionClient = window.StarfieldGraphTransactions.create({
                call: call,
                codec: window.StarfieldGraphCodec,
                edits: window.StarfieldGraphEdits
            });
        }
        return graphTransactionClient;
    }

    function applyTopologyEdit(edit) {
        var client = getGraphTransactionClient();
        if (!client || !state.targetToken) {
            showError("graph_edit_transport_unavailable", "The graph transaction bridge did not load.");
            return;
        }
        var targetToken = state.targetToken;
        refreshEpoch += 1;
        refreshInFlight = false;
        state.pending = true;
        function commitMappedEdit(snapshotResponse) {
            if (!snapshotResponse || !snapshotResponse.ok || !snapshotResponse.snapshot) {
                state.pending = false;
                if (nodePalette) nodePalette.refresh();
                var loadError = snapshotResponse && snapshotResponse.error ||
                    { code: "graph_snapshot_unavailable", message: "The project graph snapshot could not be read." };
                showError(loadError.code, loadError.message, true);
                return;
            }
            var preparedEdit = edit;
            try {
                if (!state.graphMode) {
                    var sourceGraph = window.StarfieldGraphCodec.fromHex(snapshotResponse.snapshot.graphHex);
                    preparedEdit = window.StarfieldGraphView.mapLegacyEdit(sourceGraph, edit);
                }
            } catch (error) {
                state.pending = false;
                if (nodePalette) nodePalette.refresh();
                showError(error && error.code || "invalid_graph", error && error.message || String(error), true);
                return;
            }
            client.apply(preparedEdit, function (result) {
                state.pending = false;
                if (nodePalette) nodePalette.refresh();
                if (!result || !result.ok) {
                    var error = result && result.error || { code: "graph_edit_failed", message: "The graph transaction failed." };
                    showError(error.code, error.message, true);
                    refresh(false, false);
                    return;
                }
                clearBanner(true);
                refresh(false, false);
            });
        }
        if (state.graphMode && state.graphSnapshot) {
            commitMappedEdit({ ok: true, snapshot: state.graphSnapshot });
        } else {
            loadGraphSnapshot(targetToken, commitMappedEdit);
        }
    }

    function closestElement(target, selector) {
        var element = target;
        while (element && element !== document) {
            if (element.matches && element.matches(selector)) return element;
            element = element.parentNode;
        }
        return null;
    }

    function hideGraphContextMenu() {
        if (elements.contextMenu) elements.contextMenu.hidden = true;
        contextEdge = null;
        contextGraphPoint = null;
    }

    function showGraphContextMenu(event) {
        if (!elements.contextMenu || !elements.graphCanvas) return;
        event.preventDefault();
        event.stopPropagation();
        var edgePath = closestElement(event.target, ".edge-hit");
        var nodeCard = closestElement(event.target, ".graph-node");
        contextEdge = edgePath ? {
            from: edgePath.getAttribute("data-from"),
            to: edgePath.getAttribute("data-to"),
            edgeId: edgePath.getAttribute("data-edge-id")
        } : null;
        if (nodeCard) {
            var nodeId = nodeCard.getAttribute("data-node-id");
            if (nodeId && !state.selectedNodeIds[nodeId]) {
                state.selectedNodeIds = {};
                state.selectedNodeIds[nodeId] = true;
                state.selectedNodeId = nodeId;
                syncNodeSelectionStyles();
            }
        }
        var point = canvasPoint(event.clientX, event.clientY);
        contextGraphPoint = { x: point.x - canvasOffset.x, y: point.y - canvasOffset.y };
        if (elements.disconnectContextAction) elements.disconnectContextAction.hidden = !contextEdge;
        var selectedCount = editableSelectedNodeIds().length;
        var selectionActions = elements.contextMenu.querySelectorAll('[data-action="duplicateNodes"], [data-action="deleteNodes"]');
        for (var i = 0; i < selectionActions.length; i++) selectionActions[i].disabled = selectedCount === 0;
        elements.contextMenu.hidden = false;
        if (elements.graphScroll && elements.graphScroll.focus) elements.graphScroll.focus();
        var menuWidth = elements.contextMenu.offsetWidth || 190;
        var menuHeight = elements.contextMenu.offsetHeight || 250;
        elements.contextMenu.style.left = Math.max(4, Math.min(window.innerWidth - menuWidth - 4, event.clientX)) + "px";
        elements.contextMenu.style.top = Math.max(4, Math.min(window.innerHeight - menuHeight - 4, event.clientY)) + "px";
    }

    function selectedNodeIds() {
        var ids = [];
        for (var nodeId in state.selectedNodeIds) {
            if (Object.prototype.hasOwnProperty.call(state.selectedNodeIds, nodeId)) ids.push(nodeId);
        }
        return ids;
    }

    function capturePointer(event) {
        var target = event.currentTarget;
        if (!target || typeof target.setPointerCapture !== "function" || event.pointerId === undefined) return;
        try { target.setPointerCapture(event.pointerId); } catch (ignored) { /* window listeners still handle in-panel drags */ }
    }

    function editableSelectedNodeIds() {
        return state.nodes.filter(function (node) {
            return state.selectedNodeIds[node.id] && nodeKind(node) !== "output";
        }).map(function (node) { return node.id; });
    }

    function deleteSelection() {
        var ids = editableSelectedNodeIds();
        if (!ids.length || state.pending || canvasInteractionActive()) return;
        requestTopologyEdit({ type: "deleteNodes", nodeIds: ids });
    }

    function duplicateSelection(offsetX, offsetY) {
        var ids = editableSelectedNodeIds();
        if (!ids.length) return;
        requestTopologyEdit({ type: "duplicateNodes", nodeIds: ids,
                              offset: { x: offsetX || 28, y: offsetY || 28 } });
    }

    function handleContextMenuAction(event) {
        var button = closestElement(event.target, "[data-action]");
        if (!button || !elements.contextMenu.contains(button)) return;
        event.preventDefault();
        var action = button.getAttribute("data-action");
        var edit;
        if (action === "addNode") {
            edit = { type: "addNode", nodeType: button.getAttribute("data-node-type"),
                     position: contextGraphPoint || { x: 280, y: 40 } };
        } else if (action === "duplicateNodes") {
            edit = { type: "duplicateNodes", nodeIds: editableSelectedNodeIds(), offset: { x: 28, y: 28 } };
        } else if (action === "deleteNodes") {
            edit = { type: "deleteNodes", nodeIds: editableSelectedNodeIds() };
        } else if (action === "disconnect" && contextEdge) {
            edit = { type: "disconnect", from: contextEdge.from, to: contextEdge.to,
                     edgeId: contextEdge.edgeId || undefined };
        }
        hideGraphContextMenu();
        if (edit && edit.nodeIds && !edit.nodeIds.length) return;
        if (edit) requestTopologyEdit(edit);
    }

    function edgeUnderNode(nodeId) {
        var position = nodePositions[nodeId];
        if (!position) return null;
        var display = displayPosition(position);
        var left = display.x - 8;
        var right = display.x + NODE_WIDTH + 8;
        var top = display.y - 8;
        var bottom = display.y + NODE_HEIGHT + 8;
        var sides = computePortSides();
        for (var i = 0; i < state.edges.length; i++) {
            var edge = state.edges[i];
            if (!edge || edge.length !== 2 || edge[0] === nodeId || edge[1] === nodeId) continue;
            var start = portPoint(edge[0], sides[edge[0]] ? sides[edge[0]].output : "bottom");
            var end = portPoint(edge[1], sides[edge[1]] ? sides[edge[1]].input : "top");
            if (!start || !end) continue;
            var distance = Math.max(12, Math.min(80, Math.abs(end.y - start.y) * 0.45));
            var c1 = { x: start.x, y: start.y + distance };
            var c2 = { x: end.x, y: end.y - distance };
            for (var sample = 1; sample <= 32; sample++) {
                var t = sample / 32;
                var inverse = 1 - t;
                var x = inverse * inverse * inverse * start.x + 3 * inverse * inverse * t * c1.x +
                        3 * inverse * t * t * c2.x + t * t * t * end.x;
                var y = inverse * inverse * inverse * start.y + 3 * inverse * inverse * t * c1.y +
                        3 * inverse * t * t * c2.y + t * t * t * end.y;
                if (x >= left && x <= right && y >= top && y <= bottom) return edge;
            }
        }
        return null;
    }

    function currentCurveState(kind, nodeId) {
        if (state.graphMode) {
            var selectedId = nodeId || state.selectedNodeId;
            for (var i = 0; i < state.nodes.length; i++) {
                if (state.nodes[i].id === selectedId) return state.nodes[i].curves;
            }
            return null;
        }
        return state.curves;
    }

    function selectedGraphNode(nodeId) {
        var selectedId = nodeId || state.selectedNodeId;
        for (var i = 0; i < state.nodes.length; i++) {
            if (state.nodes[i].id === selectedId) return state.nodes[i];
        }
        return null;
    }

    function graphParameterRecord(node, key) {
        if (!node || !node.graphParameters) return null;
        for (var i = 0; i < node.graphParameters.length; i++) {
            if (node.graphParameters[i].key === String(key)) return node.graphParameters[i];
        }
        return null;
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
        var kind = nodeKind(node);
        elements.inspectorMeta.textContent = kind === "particle"
            ? "Size / Opacity over life" : node.params.length + " parameters";
        if (!node.params.length) {
            elements.inspectorBody.innerHTML = "<p class=\"inspector-note\">This node has no parameters.</p>";
            shownInspectorNodeId = node.id;
            positionInspector(node.id);
            return;
        }
        var grid = document.createElement("div");
        grid.className = "parameter-grid";
        for (var p = 0; p < node.params.length; p++) {
            if ((kind === "particle") &&
                (node.params[p].legacyKey === "particle_size_end" || node.params[p].legacyKey === "opacity_end" ||
                 node.params[p].graphKey === "4" || node.params[p].graphKey === "6")) continue;
            grid.appendChild(renderParameter(node.params[p]));
        }
        elements.inspectorBody.appendChild(grid);
        if(kind==="particle" && node.gradient && node.colorMode!==0) elements.inspectorBody.appendChild(renderGradientEditor(node));
        var curves = currentCurveState("size", node.id);
        if (kind === "force" && curves) {
            elements.inspectorBody.appendChild(renderCurveEditor("size", "Wind and Spin Over Life (%)", curves.size, 100));
            drawCurvePlot("size");
        }
        if ((kind === "particle") && curves) {
            elements.inspectorBody.appendChild(renderCurveEditor("size", "Size Over Life (%)", curves.size, 100));
            elements.inspectorBody.appendChild(renderCurveEditor("opacity", "Opacity Over Life (%)", curves.opacity, 100));
            drawCurvePlot("size");
            drawCurvePlot("opacity");
            elements.inspectorBody.appendChild(renderRotationCurveEditor(node));
        }
        if (shownInspectorNodeId !== node.id) positionInspector(node.id);
        shownInspectorNodeId = node.id;
    }

    function renderGradientEditor(node) {
        var section=document.createElement("section");section.className="gradient-editor";
        var title=document.createElement("strong");title.textContent="Color Gradient";section.appendChild(title);
        var stops=node.gradient.map(function(stop){return {position:stop.position,color:stop.color.slice()};}),selected=0;stops.interpolation=node.gradient.interpolation||0;
        var bar=document.createElement("div");bar.className="gradient-bar";section.appendChild(bar);
        var tools=window.StarfieldGradientTools;
        var toolbar=document.createElement("div");toolbar.className="gradient-toolbar";section.appendChild(toolbar);
        var interpolation=document.createElement("button");interpolation.textContent=stops.interpolation?"Hold":"Linear";toolbar.appendChild(interpolation);
        interpolation.addEventListener("click",function(){if(state.pending)return;stops.interpolation=stops.interpolation?0:1;commit();});
        var flip=document.createElement("button");flip.textContent="Flip";toolbar.appendChild(flip);
        var copy=document.createElement("button");copy.textContent="Copy";toolbar.appendChild(copy);
        var paste=document.createElement("button");paste.textContent="Paste";paste.disabled=!tools.paste();toolbar.appendChild(paste);
        var presets=document.createElement("select");presets.setAttribute("aria-label","Color Gradient Presets");toolbar.appendChild(presets);
        var placeholder=document.createElement("option");placeholder.textContent="Presets…";placeholder.value="";presets.appendChild(placeholder);
        tools.presets().forEach(function(p,index){var option=document.createElement("option");option.textContent=p.name;option.value=String(index);presets.appendChild(option);});
        var controls=document.createElement("div");controls.className="gradient-controls";section.appendChild(controls);
        var color=document.createElement("input");color.type="color";color.setAttribute("aria-label","Selected gradient color");controls.appendChild(color);
        var position=document.createElement("input");position.type="number";position.min=0;position.max=100;position.step=0.1;
        position.setAttribute("aria-label","Gradient stop position (%)");controls.appendChild(position);
        var remove=document.createElement("button");remove.textContent="Remove";controls.appendChild(remove);
        var add=document.createElement("button");add.textContent="Add";controls.appendChild(add);
        var instruction=document.createElement("small");instruction.textContent="Click the bar to add; drag a marker to move; double-click to edit its color.";section.appendChild(instruction);
        function hex(rgb) {return "#"+rgb.map(function(c){return ("0"+Math.round(Math.max(0,Math.min(1,c))*255).toString(16)).slice(-2);}).join("");}
        function commit() {
            if(state.pending || !tools.valid(stops)) return;
            applyTopologyEdit({type:"setParameters",changes:[{nodeId:node.id,parameterKey:"13",valueType:7,value:window.StarfieldGraphView.encodeGradient(stops)}]});
        }
        function draw() {
            bar.innerHTML="";
            var colors=[];stops.forEach(function(stop,i){if(stops.interpolation===1 && i)colors.push(hex(stops[i-1].color)+" "+stop.position*100+"%");colors.push(hex(stop.color)+" "+stop.position*100+"%");});
            bar.style.background="linear-gradient(to right,"+colors.join(",")+")";interpolation.textContent=stops.interpolation?"Hold":"Linear";
            stops.forEach(function(stop,index){
                var marker=document.createElement("button");marker.className="gradient-stop"+(selected===index?" selected":"");
                marker.style.left=stop.position*100+"%";marker.style.backgroundColor=hex(stop.color);
                marker.setAttribute("aria-label","Gradient stop "+(index+1));bar.appendChild(marker);
                marker.addEventListener("click",function(event){event.stopPropagation();selected=index;selectStop();});
                marker.addEventListener("dblclick",function(event){event.stopPropagation();selected=index;selectStop();color.click();});
                marker.addEventListener("pointerdown",function(event){
                    if(event.button!==0 || state.pending) return;
                    event.preventDefault();event.stopPropagation();selected=index;
                    var moved=false,original=stops.map(function(s){return {position:s.position,color:s.color.slice()};});original.interpolation=stops.interpolation||0;
                    function move(e) {
                        var rect=bar.getBoundingClientRect(),value=(e.clientX-rect.left)/rect.width;
                        var result=tools.move(stops,selected,value);if(!result)return;
                        moved=true;stops=result.stops;selected=result.index;draw();
                    }
                    function up(e){
                        window.removeEventListener("pointermove",move);window.removeEventListener("pointerup",up);window.removeEventListener("pointercancel",up);
                        if(e.type==="pointercancel"){stops=original;selected=index;draw();}
                        else if(moved) commit();else selectStop();
                    }
                    window.addEventListener("pointermove",move);window.addEventListener("pointerup",up);window.addEventListener("pointercancel",up);
                });
            });
            selectStop();
        }
        function selectStop() {
            Array.prototype.forEach.call(bar.children,function(marker,index){
                marker.className="gradient-stop"+(selected===index?" selected":"");
            });
            color.value=hex(stops[selected].color);position.value=(stops[selected].position*100).toFixed(1);
            position.disabled=false;
            remove.disabled=stops.length<=2;add.disabled=stops.length>=8;
        }
        function insert(value) {
            var result=tools.insert(stops,value);if(result){stops=result.stops;selected=result.index;commit();}
        }
        bar.addEventListener("click",function(event){if(event.target!==bar)return;var rect=bar.getBoundingClientRect();insert((event.clientX-rect.left)/rect.width);});
        color.addEventListener("change",function(){var value=color.value;stops[selected].color=[1,3,5].map(function(start){return parseInt(value.substr(start,2),16)/255;});commit();});
        position.addEventListener("change",function(){
            var value=Number(position.value)/100;
            var result=tools.move(stops,selected,value);if(result){stops=result.stops;selected=result.index;commit();}
        });
        remove.addEventListener("click",function(){if(stops.length>2){stops.splice(selected,1);selected=0;commit();}});
        flip.addEventListener("click",function(){if(state.pending)return;stops=tools.flip(stops);selected=stops.length-1-selected;commit();});
        copy.addEventListener("click",function(){tools.copy(stops);paste.disabled=false;});
        paste.addEventListener("click",function(){if(state.pending)return;var copied=tools.paste();if(copied){stops=copied;selected=0;commit();}});
        presets.addEventListener("change",function(){if(state.pending || presets.value==="")return;
            stops=tools.presets()[Number(presets.value)].stops;selected=0;commit();});
        add.addEventListener("click",function(){
            var widest=1;for(var i=2;i<stops.length;i++) if(stops[i].position-stops[i-1].position>stops[widest].position-stops[widest-1].position)widest=i;
            insert((stops[widest].position+stops[widest-1].position)/2);
        });
        draw();return section;
    }

    function renderRotationCurveEditor(node) {
        var section=document.createElement("section");section.className="age-curve-editor";
        var title=document.createElement("strong");title.textContent="Rotation Over Life";section.appendChild(title);
        var points=copyCurvePoints(node.curves.rotation.points),selected=0;
        var plot=document.createElementNS("http://www.w3.org/2000/svg","svg");plot.setAttribute("viewBox","0 0 240 100");plot.style.width="100%";plot.style.height="110px";section.appendChild(plot);
        var toolbar=document.createElement("div");toolbar.className="gradient-toolbar";section.appendChild(toolbar);
        function button(label,fn){var b=document.createElement("button");b.textContent=label;b.addEventListener("click",function(){if(!state.pending)fn();});toolbar.appendChild(b);return b;}
        var life=document.createElement("input");life.type="number";life.min=0;life.max=100;life.step=.1;life.setAttribute("aria-label","Rotation point life percent");
        var value=document.createElement("input");value.type="number";value.min=-32768;value.max=32768;value.step=.1;value.setAttribute("aria-label","Rotation degrees");
        var fields=document.createElement("div");fields.className="gradient-controls";fields.appendChild(life);fields.appendChild(value);section.appendChild(fields);
        var rotationClipboard=window.StarfieldRotationClipboard;
        var mode=button(window.StarfieldCurveTools.modes[points.interpolation||0],function(){points=window.StarfieldCurveTools.cycle(points);selected=0;commit();});
        button("Flip",function(){points.reverse().forEach(function(p){p.age=1-p.age;});selected=points.length-1-selected;commit();});
        button("Copy",function(){window.StarfieldRotationClipboard=copyCurvePoints(points);paste.disabled=false;});
        var paste=button("Paste",function(){if(window.StarfieldRotationClipboard){points=copyCurvePoints(window.StarfieldRotationClipboard);selected=0;commit();}});paste.disabled=!rotationClipboard;
        var presets=document.createElement("select");presets.setAttribute("aria-label","Rotation curve presets");var option=document.createElement("option");option.textContent="Presets…";option.value="";presets.appendChild(option);
        window.StarfieldCurveTools.presets().forEach(function(p,i){var option=document.createElement("option");option.value=String(i);option.textContent=p.name;presets.appendChild(option);});toolbar.appendChild(presets);
        presets.addEventListener("change",function(){if(state.pending || presets.value==="")return;points=window.StarfieldCurveTools.presets()[Number(presets.value)].points;selected=0;commit();});
        button("<",function(){selected=Math.max(0,selected-1);draw();});button(">",function(){selected=Math.min(points.length-1,selected+1);draw();});
        var remove=button("Remove",function(){if(selected>0 && selected<points.length-1){points.splice(selected,1);selected=0;commit();}});
        function rotationPath(span){var result=[];for(var i=0;i<=220;i++)result.push((10+i)+","+(50-window.StarfieldCurveTools.sample(points,i/220)/span*40));return result.join(" ");}
        function commit(){applyCurveChanges("rotation",points,true,node.id);draw();}
        function draw(){
            while(plot.firstChild)plot.removeChild(plot.firstChild);
            var span=Math.max.apply(null,[360].concat(points.map(function(p){return Math.abs(p.value);}))),ns=plot.namespaceURI;
            var path=document.createElementNS(ns,"polyline");path.setAttribute("points",rotationPath(span));path.setAttribute("fill","none");path.setAttribute("stroke","#9bc5eb");plot.appendChild(path);
            if(points.interpolation!==3)points.forEach(function(p,index){var marker=document.createElementNS(ns,"circle");marker.setAttribute("cx",10+p.age*220);marker.setAttribute("cy",50-p.value/span*40);marker.setAttribute("r",4);marker.setAttribute("fill",index===selected?"#fff":"#70a8d3");
                marker.addEventListener("pointerdown",function(event){if(state.pending)return;event.preventDefault();event.stopPropagation();selected=index;marker.setPointerCapture(event.pointerId);
                    var original=copyCurvePoints(points),changed=false;
                    function move(e){var rect=plot.getBoundingClientRect(),x=(e.clientX-rect.left)*240/rect.width,y=(e.clientY-rect.top)*100/rect.height;if(index>0 && index<points.length-1)points[index].age=Math.max(points[index-1].age+.001,Math.min(points[index+1].age-.001,(x-10)/220));points[index].value=Math.max(-32768,Math.min(32768,(50-y)*span/40));changed=true;path.setAttribute("points",rotationPath(span));marker.setAttribute("cx",10+points[index].age*220);marker.setAttribute("cy",50-points[index].value/span*40);}
                    function end(e){marker.removeEventListener("pointermove",move);marker.removeEventListener("pointerup",end);marker.removeEventListener("pointercancel",cancel);if(changed)commit();else draw();}
                    function cancel(){points=original;changed=false;end();}
                    marker.addEventListener("pointermove",move);marker.addEventListener("pointerup",end);marker.addEventListener("pointercancel",cancel);
                });plot.appendChild(marker);
            });
            mode.textContent=window.StarfieldCurveTools.modes[points.interpolation||0];
            plot.onclick=function(event){if(points.interpolation===3 || event.target!==plot || state.pending || points.length>=64)return;var rect=plot.getBoundingClientRect(),age=Math.max(.001,Math.min(.999,((event.clientX-rect.left)*240/rect.width-10)/220)),i=1;while(i<points.length && points[i].age<age)i++;if(age-points[i-1].age<.001 || points[i].age-age<.001)return;points.splice(i,0,{age:age,value:Math.max(-32768,Math.min(32768,(50-(event.clientY-rect.top)*100/rect.height)*span/40))});selected=i;commit();};
            life.value=(points[selected].age*100).toFixed(1);life.disabled=selected===0 || selected===points.length-1;value.value=points[selected].value.toFixed(1);remove.disabled=life.disabled;
        }
        plot.addEventListener("pointerdown",function(event){
            if(state.pending || points.interpolation!==3 || event.button!==0)return;event.preventDefault();var span=Math.max.apply(null,[360].concat(points.map(function(p){return Math.abs(p.value);}))),original=copyCurvePoints(points);
            function position(e){var r=plot.getBoundingClientRect();return {age:Math.max(0,Math.min(1,((e.clientX-r.left)*240/r.width-10)/220)),value:Math.max(-32768,Math.min(32768,(50-(e.clientY-r.top)*100/r.height)*span/40))};}
            points=window.StarfieldCurveTools.dense(points);var p=position(event),last=Math.round(p.age*63);points[last].value=p.value;plot.setPointerCapture(event.pointerId);draw();
            function move(e){var p=position(e);last=window.StarfieldCurveTools.stroke(points,last,p.age,p.value);draw();}
            function end(e){plot.removeEventListener("pointermove",move);plot.removeEventListener("pointerup",end);plot.removeEventListener("pointercancel",cancel);if(e && plot.hasPointerCapture(e.pointerId))plot.releasePointerCapture(e.pointerId);commit();}
            function cancel(e){points=original;end(e);}
            plot.addEventListener("pointermove",move);plot.addEventListener("pointerup",end);plot.addEventListener("pointercancel",cancel);
        });
        life.addEventListener("change",function(){if(state.pending)return;var age=Number(life.value)/100;if(isFinite(age)){points[selected].age=Math.max(points[selected-1].age+.001,Math.min(points[selected+1].age-.001,age));commit();}});
        value.addEventListener("change",function(){if(state.pending)return;var amount=Number(value.value);if(isFinite(amount)){points[selected].value=Math.max(-32768,Math.min(32768,amount));commit();}});
        draw();return section;
    }

    function renderCurveEditor(kind, label, curve, maximum) {
        var section = document.createElement("section");
        section.className = "age-curve-editor";
        section.dataset.curveKind = kind;
        var heading = document.createElement("div");
        heading.className = "age-curve-heading";
        var title = document.createElement("strong");
        title.textContent = label;
        heading.appendChild(title);
        var actions = document.createElement("div");
        actions.className = "age-curve-actions";
        var tools=window.StarfieldCurveTools;
        function editButton(label,fn){var button=document.createElement("button");button.textContent=label;button.addEventListener("click",function(){if(!state.pending)fn();});actions.appendChild(button);return button;}
        var interpolation=editButton(tools.modes[curve.points.interpolation||0],function(){
            var current=currentCurveState(kind)[kind],points=tools.cycle(current.points);
            applyCurveChanges(kind,points,true);
        });interpolation.disabled=!state.graphMode;
        if(state.graphMode){
            editButton("Flip",function(){var points=copyCurvePoints(currentCurveState(kind)[kind].points);points.reverse().forEach(function(p){p.age=1-p.age;});applyCurveChanges(kind,points,true);});
            editButton("Copy",function(){window.StarfieldCurveClipboard=copyCurvePoints(currentCurveState(kind)[kind].points);});
            editButton("Paste",function(){if(window.StarfieldCurveClipboard && window.StarfieldCurveClipboard.every(function(p){return p.value>=0 && p.value<=100;}))applyCurveChanges(kind,copyCurvePoints(window.StarfieldCurveClipboard),true);});
            var presets=document.createElement("select");presets.setAttribute("aria-label",label+" presets");var option=document.createElement("option");option.textContent="Presets…";option.value="";presets.appendChild(option);
            tools.presets().forEach(function(p,i){var option=document.createElement("option");option.textContent=p.name;option.value=String(i);presets.appendChild(option);});actions.appendChild(presets);
            presets.addEventListener("change",function(){if(!state.pending && presets.value!=="")applyCurveChanges(kind,tools.presets()[Number(presets.value)].points,true);});
        }
        var remove = document.createElement("button");
        remove.type = "button";
        remove.textContent = "Remove Point";
        remove.className = "age-curve-remove";
        remove.title = "Remove the selected interior control point";
        remove.addEventListener("click", function () {
            var curves = currentCurveState(kind);
            var currentCurve = curves && curves[kind];
            var selectedIndex = curveSelectedIndex(kind, currentCurve);
            if (!currentCurve || selectedIndex <= 0 || selectedIndex >= currentCurve.points.length - 1) return;
            var points = copyCurvePoints(currentCurve.points);
            points.splice(selectedIndex, 1);
            selectedCurvePoints[kind] = Math.max(0, selectedIndex - 1);
            applyCurveChanges(kind, points, true);
        });
        actions.appendChild(remove);
        heading.appendChild(actions);
        section.appendChild(heading);
        var svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
        svg.setAttribute("viewBox", "0 0 320 140");
        svg.setAttribute("preserveAspectRatio", "none");
        svg.setAttribute("class", "age-curve-plot");
        svg.dataset.curveKind = kind;
        svg.dataset.valueMax = String(maximum);
        svg.addEventListener("pointerdown", function (event) {
            if (event.button !== 0 || state.pending || closestElement(event.target, ".age-curve-point")) return;
            var current=currentCurveState(kind)[kind];
            if(state.graphMode && current.points.interpolation===3){
                var original=copyCurvePoints(current.points),points=window.StarfieldCurveTools.dense(current.points),position=curvePosition(event,svg,100),last=Math.round(position.age*63);
                points[last].value=position.value;current.points=points;svg.setPointerCapture(event.pointerId);drawCurvePlot(kind);
                function move(e){var p=curvePosition(e,svg,100);last=window.StarfieldCurveTools.stroke(points,last,p.age,p.value);drawCurvePlot(kind);}
                function end(e){svg.removeEventListener("pointermove",move);svg.removeEventListener("pointerup",end);svg.removeEventListener("pointercancel",cancel);if(e && svg.hasPointerCapture(e.pointerId))svg.releasePointerCapture(e.pointerId);applyCurveChanges(kind,points,true);}
                function cancel(e){current.points=original;points=original;end(e);}
                svg.addEventListener("pointermove",move);svg.addEventListener("pointerup",end);svg.addEventListener("pointercancel",cancel);
            } else addCurvePointFromEvent(kind, event, svg, maximum);
            if (event.preventDefault) event.preventDefault();
        });
        section.appendChild(svg);

        var controls = document.createElement("div");
        controls.className = "age-curve-controls";
        var lifeControl = makeCurvePointControl(kind, "age", maximum);
        var valueControl = makeCurvePointControl(kind, "value", maximum);
        controls.appendChild(lifeControl);
        controls.appendChild(valueControl);
        var navigation = document.createElement("div");
        navigation.className = "age-curve-navigation";
        var previous = document.createElement("button");
        previous.type = "button";
        previous.className = "age-curve-previous";
        previous.textContent = "‹";
        previous.title = "Select previous control point";
        previous.addEventListener("click", function () {
            var curves = currentCurveState(kind);
            var currentCurve = curves && curves[kind];
            if (!currentCurve) return;
            selectedCurvePoints[kind] = Math.max(0, curveSelectedIndex(kind, currentCurve) - 1);
            drawCurvePlot(kind);
        });
        navigation.appendChild(previous);
        var pointIndexLabel = document.createElement("span");
        pointIndexLabel.className = "age-curve-point-index";
        pointIndexLabel.setAttribute("aria-live", "polite");
        navigation.appendChild(pointIndexLabel);
        var next = document.createElement("button");
        next.type = "button";
        next.className = "age-curve-next";
        next.textContent = "›";
        next.title = "Select next control point";
        next.addEventListener("click", function () {
            var curves = currentCurveState(kind);
            var currentCurve = curves && curves[kind];
            if (!currentCurve) return;
            selectedCurvePoints[kind] = Math.min(currentCurve.points.length - 1,
                                                   curveSelectedIndex(kind, currentCurve) + 1);
            drawCurvePlot(kind);
        });
        navigation.appendChild(next);
        controls.appendChild(navigation);
        section.appendChild(controls);
        return section;
    }

    function curveSelectedIndex(kind, curve) {
        if (!curve || !curve.points || !curve.points.length) return -1;
        var index = Number(selectedCurvePoints[kind]);
        if (!isFinite(index)) index = 0;
        index = Math.max(0, Math.min(curve.points.length - 1, Math.floor(index)));
        selectedCurvePoints[kind] = index;
        return index;
    }

    function makeCurvePointControl(kind, field, maximum) {
        var wrapper = document.createElement("label");
        wrapper.className = "age-curve-control";
        var caption = document.createElement("span");
        caption.textContent = field === "age" ? "Life" : "Value (%)";
        wrapper.appendChild(caption);
        var input = document.createElement("input");
        input.type = "number";
        input.className = field === "age" ? "age-curve-life" : "age-curve-value";
        input.dataset.curveKind = kind;
        input.dataset.curveField = field;
        input.dataset.decimals = field === "age" ? "1" : "1";
        input.dataset.scrubStep = field === "age" ? "0.1" : "1";
        input.dataset.min = "0";
        input.dataset.max = String(field === "age" ? 100 : maximum);
        input.min = "0";
        input.max = input.dataset.max;
        input.step = field === "age" ? "0.1" : "0.1";
        input.title = "Drag left or right to adjust · Shift: faster · Ctrl: finer · Click to type";
        input._curveCommit = function (nextValue) {
            setCurvePointControl(kind, field, nextValue, maximum);
        };
        input.addEventListener("pointerdown", beginNumericScrub);
        input.addEventListener("change", function () {
            setCurvePointControl(kind, field, Number(input.value), maximum);
        });
        wrapper.appendChild(input);
        return wrapper;
    }

    function setCurvePointControl(kind, field, value, maximum) {
        var curves = currentCurveState(kind);
        var curve = curves && curves[kind];
        var selected = curveSelectedIndex(kind, curve);
        if (!curve || selected < 0 || state.pending || !isFinite(value)) return;
        var points = copyCurvePoints(curve.points);
        if (field === "age") {
            if (selected === 0 || selected === points.length - 1) return;
            var minAge = points[selected - 1].age * 100 + 0.5;
            var maxAge = points[selected + 1].age * 100 - 0.5;
            value = Math.max(minAge, Math.min(maxAge, value));
            points[selected].age = roundCurveNumber(value / 100, 3);
        } else {
            if (curveEndpointIsAnimated(kind, selected)) return;
            value = Math.max(0, Math.min(maximum, value));
            points[selected].value = roundCurveNumber(value, kind === "opacity" ? 3 : 2);
        }
        applyCurveChanges(kind, points, curve.custom);
    }

    function copyCurvePoints(points) {
        var copy = [];
        for (var i = 0; i < points.length; i++) copy.push({ age: points[i].age, value: points[i].value });
        copy.interpolation=points.interpolation||0;return copy;
    }

    function curvePlotBounds() {
        return { left: 26, right: 312, top: 10, bottom: 112 };
    }

    function curvePlotMaximum(kind, maximum, points) {
        return 100;
    }

    function curveEndpointIsAnimated(kind, index, nodeId) {
        var curves = currentCurveState(kind, nodeId);
        var curve = curves && curves[kind];
        if (!curve || index !== curve.points.length - 1 || index === 0) return false;
        var key = kind === "size" ? "particle_size_end" : "opacity_end";
        if (state.graphMode) {
            var graphNode = selectedGraphNode(nodeId);
            var graphKey = kind === "size" ? "4" : "6";
            var graphParameter = graphParameterRecord(graphNode, graphKey);
            return !!(graphParameter && graphParameter.animated === true);
        }
        for (var i = 0; i < state.nodes.length; i++) {
            for (var p = 0; p < state.nodes[i].params.length; p++) {
                if (state.nodes[i].params[p].key === key) return state.nodes[i].params[p].animated === true;
            }
        }
        return false;
    }

    function drawCurvePlot(kind, forcedMaximum) {
        var plots = elements.inspectorBody ? elements.inspectorBody.querySelectorAll(".age-curve-plot") : [];
        var plot = null;
        for (var i = 0; i < plots.length; i++) {
            if (plots[i].dataset.curveKind === kind) { plot = plots[i]; break; }
        }
        var curves = currentCurveState(kind);
        var curve = curves && curves[kind];
        if (!plot || !curve) return;
        var selectedPoint = curveSelectedIndex(kind, curve);
        while (plot.firstChild) plot.removeChild(plot.firstChild);
        var bounds = curvePlotBounds();
        var max = typeof forcedMaximum === "number" ? forcedMaximum :
                  curvePlotMaximum(kind, Number(plot.dataset.valueMax), curve.points);
        plot.dataset.displayMaximum = String(max);
        function px(age) { return bounds.left + age * (bounds.right - bounds.left); }
        function py(value) { return bounds.bottom - Math.max(0, Math.min(max, value)) / max * (bounds.bottom - bounds.top); }
        var ns = "http://www.w3.org/2000/svg";
        var border = document.createElementNS(ns, "rect");
        border.setAttribute("x", bounds.left); border.setAttribute("y", bounds.top);
        border.setAttribute("width", bounds.right - bounds.left); border.setAttribute("height", bounds.bottom - bounds.top);
        border.setAttribute("class", "age-curve-border"); plot.appendChild(border);
        for (var gridIndex = 1; gridIndex < 4; gridIndex++) {
            var x = bounds.left + (bounds.right - bounds.left) * gridIndex / 4;
            var gridLine = document.createElementNS(ns, "line");
            gridLine.setAttribute("x1", x); gridLine.setAttribute("x2", x);
            gridLine.setAttribute("y1", bounds.top); gridLine.setAttribute("y2", bounds.bottom);
            gridLine.setAttribute("class", "age-curve-grid"); plot.appendChild(gridLine);
            var y = bounds.top + (bounds.bottom - bounds.top) * gridIndex / 4;
            var horizontal = document.createElementNS(ns, "line");
            horizontal.setAttribute("x1", bounds.left); horizontal.setAttribute("x2", bounds.right);
            horizontal.setAttribute("y1", y); horizontal.setAttribute("y2", y);
            horizontal.setAttribute("class", "age-curve-grid"); plot.appendChild(horizontal);
        }
        var path = document.createElementNS(ns, "polyline");
        var coordinates = [];
        for (var p=0;p<=286;p++) coordinates.push(px(p/286)+","+py(window.StarfieldCurveTools.sample(curve.points,p/286)));
        path.setAttribute("points", coordinates.join(" "));
        path.setAttribute("class", "age-curve-line");
        plot.appendChild(path);
        for (var pointIndex = 0; curve.points.interpolation!==3 && pointIndex < curve.points.length; pointIndex++) {
            var circle = document.createElementNS(ns, "circle");
            circle.setAttribute("cx", px(curve.points[pointIndex].age));
            circle.setAttribute("cy", py(curve.points[pointIndex].value));
            circle.setAttribute("r", "4.5");
            circle.setAttribute("class", "age-curve-point" +
                (selectedPoint === pointIndex ? " selected" : "") +
                (curveEndpointIsAnimated(kind, pointIndex) ? " locked" : ""));
            circle.dataset.pointIndex = String(pointIndex);
            circle.dataset.curveKind = kind;
            if (curveEndpointIsAnimated(kind, pointIndex)) {
                circle.setAttribute("aria-label", "Animated endpoint; edit its AE keyframes");
                circle.setAttribute("title", "This endpoint follows AE keyframes and cannot be changed here.");
            }
            circle.addEventListener("pointerdown", beginCurvePointDrag);
            circle.addEventListener("click", function (event) {
                selectedCurvePoints[kind] = Number(event.currentTarget.dataset.pointIndex);
                drawCurvePlot(kind);
                if (event.stopPropagation) event.stopPropagation();
            });
            plot.appendChild(circle);
        }
        var zero = document.createElementNS(ns, "text");
        zero.setAttribute("x", bounds.left); zero.setAttribute("y", "132"); zero.setAttribute("class", "age-curve-axis-label");
        zero.textContent = "0"; plot.appendChild(zero);
        var one = document.createElementNS(ns, "text");
        one.setAttribute("x", bounds.right); one.setAttribute("y", "132"); one.setAttribute("text-anchor", "end");
        one.setAttribute("class", "age-curve-axis-label"); one.textContent = "Life"; plot.appendChild(one);
        var maxLabel = document.createElementNS(ns, "text");
        maxLabel.setAttribute("x", "4"); maxLabel.setAttribute("y", "16"); maxLabel.setAttribute("class", "age-curve-axis-label");
        maxLabel.textContent = "100"; plot.appendChild(maxLabel);
        var minLabel = document.createElementNS(ns, "text");
        minLabel.setAttribute("x", "4"); minLabel.setAttribute("y", String(bounds.bottom)); minLabel.setAttribute("class", "age-curve-axis-label");
        minLabel.textContent = "0"; plot.appendChild(minLabel);
        var section = plot.parentNode;
        var lifeInput = section && section.querySelector(".age-curve-life");
        var valueInput = section && section.querySelector(".age-curve-value");
        var previousButton = section && section.querySelector(".age-curve-previous");
        var nextButton = section && section.querySelector(".age-curve-next");
        var pointIndexLabel = section && section.querySelector(".age-curve-point-index");
        var removeButton = section && section.querySelector(".age-curve-remove");
        if (selectedPoint >= 0 && selectedPoint < curve.points.length) {
            var selectedAge = curve.points[selectedPoint].age * 100;
            var animatedEndpoint = curveEndpointIsAnimated(kind, selectedPoint);
            if (lifeInput) {
                lifeInput.value = formatParameterNumber(selectedAge, 1);
                lifeInput.disabled = selectedPoint === 0 || selectedPoint === curve.points.length - 1;
                lifeInput.dataset.min = selectedPoint > 0
                    ? String(curve.points[selectedPoint - 1].age * 100 + 0.5) : "0";
                lifeInput.dataset.max = selectedPoint < curve.points.length - 1
                    ? String(curve.points[selectedPoint + 1].age * 100 - 0.5) : "100";
                lifeInput.min = lifeInput.dataset.min;
                lifeInput.max = lifeInput.dataset.max;
                lifeInput.title = lifeInput.disabled
                    ? "Endpoint age is fixed at the beginning or end of life."
                    : "Drag left or right to adjust · Shift: faster · Ctrl: finer · Click to type";
            }
            if (valueInput) {
                valueInput.disabled = animatedEndpoint;
                valueInput.value = formatParameterNumber(curve.points[selectedPoint].value, 1);
                valueInput.title = animatedEndpoint
                ? "This endpoint follows AE keyframes and cannot be changed here."
                : "Drag left or right to adjust · Shift: faster · Ctrl: finer · Click to type";
            }
            if (pointIndexLabel) pointIndexLabel.textContent = (selectedPoint + 1) + " / " + curve.points.length;
            if (previousButton) previousButton.disabled = selectedPoint <= 0;
            if (nextButton) nextButton.disabled = selectedPoint >= curve.points.length - 1;
            if (removeButton) removeButton.disabled = selectedPoint <= 0 || selectedPoint >= curve.points.length - 1;
        }
    }

    function curvePosition(event, plot, maximum) {
        var bounds = curvePlotBounds();
        var x = NaN;
        var y = NaN;
        if (typeof plot.createSVGPoint === "function" && typeof plot.getScreenCTM === "function") {
            try {
                var point = plot.createSVGPoint();
                point.x = event.clientX;
                point.y = event.clientY;
                var matrix = plot.getScreenCTM();
                if (matrix && typeof matrix.inverse === "function") {
                    var local = point.matrixTransform(matrix.inverse());
                    x = local.x;
                    y = local.y;
                }
            } catch (ignored) { /* use the scaled-viewport fallback below */ }
        }
        if (!isFinite(x) || !isFinite(y)) {
            var rect = plot.getBoundingClientRect();
            var width = Math.max(1, plot.clientWidth || rect.width);
            var height = Math.max(1, plot.clientHeight || rect.height);
            var left = rect.left + (plot.clientLeft || 0);
            var top = rect.top + (plot.clientTop || 0);
            x = (event.clientX - left) / width * 320;
            y = (event.clientY - top) / height * 140;
        }
        return {
            age: Math.max(0, Math.min(1, (x - bounds.left) / (bounds.right - bounds.left))),
            value: Math.max(0, Math.min(maximum, (bounds.bottom - y) / (bounds.bottom - bounds.top) * maximum))
        };
    }

    function addCurvePointFromEvent(kind, event, plot, maximum) {
        var curves = currentCurveState(kind);
        var curve = curves && curves[kind];
        if (!curve) return;
        if (curve.points.length >= (state.graphMode?64:8)) { showError("curve_full", "The curve point limit has been reached."); return; }
        var displayMaximum = curvePlotMaximum(kind, maximum, curve.points);
        var position = curvePosition(event, plot, displayMaximum);
        if (position.age <= 0.01 || position.age >= 0.99) return;
        var points = copyCurvePoints(curve.points);
        var insertAt = 1;
        while (insertAt < points.length - 1 && points[insertAt].age < position.age) insertAt += 1;
        if (position.age - points[insertAt - 1].age < 0.005 || points[insertAt].age - position.age < 0.005) return;
        // Clicking close to an existing segment should insert a knot on that segment,
        // preserving the current curve. The user can then drag the new point to reshape it.
        var left = points[insertAt - 1];
        var right = points[insertAt];
        var span = right.age - left.age;
        var amount = span > 0 ? (position.age - left.age) / span : 0;
        var onCurveValue = left.value + (right.value - left.value) * amount;
        var bounds = curvePlotBounds();
        var verticalDistance = Math.abs(position.value - onCurveValue) /
            displayMaximum * (bounds.bottom - bounds.top);
        var rect = plot.getBoundingClientRect();
        var screenScaleY = rect.height / 140;
        if (isFinite(screenScaleY) && verticalDistance * screenScaleY <= 10) {
            position.value = onCurveValue;
        }
        points.splice(insertAt, 0, { age: roundCurveNumber(position.age, 3),
                                     value: roundCurveNumber(position.value, 1) });
        selectedCurvePoints[kind] = insertAt;
        applyCurveChanges(kind, points, true);
    }

    function beginCurvePointDrag(event) {
        if (event.button !== 0 || state.pending || curveDragState) return;
        var kind = event.currentTarget.dataset.curveKind;
        var index = Number(event.currentTarget.dataset.pointIndex);
        var curves = currentCurveState(kind);
        var curve = curves && curves[kind];
        if (!curve || !isFinite(index) || index < 0 || index >= curve.points.length) return;
        selectedCurvePoints[kind] = index;
        if (curveEndpointIsAnimated(kind, index)) {
            drawCurvePlot(kind);
            if (event.stopPropagation) event.stopPropagation();
            if (event.preventDefault) event.preventDefault();
            return;
        }
        curveDragState = { kind: kind, index: index, nodeId: state.selectedNodeId, pointerId: event.pointerId,
                           startX: event.clientX, startY: event.clientY,
                           original: copyCurvePoints(curve.points), moved: false,
                           valueScale: curvePlotMaximum(kind,
                               Number(event.currentTarget.parentNode.dataset.valueMax), curve.points) };
        var plot = event.currentTarget.parentNode;
        if (plot && typeof plot.setPointerCapture === "function" && event.pointerId !== undefined) {
            try { plot.setPointerCapture(event.pointerId); } catch (ignored) { /* window handlers remain available */ }
        }
        if (event.currentTarget.classList) event.currentTarget.classList.add("selected");
        if (document.body && document.body.classList) document.body.classList.add("dragging-age-curve");
        var removeButton = plot.parentNode.querySelector(".age-curve-remove");
        if (removeButton) removeButton.disabled = index <= 0 || index >= curve.points.length - 1;
        drawCurvePlot(kind);
        if (event.stopPropagation) event.stopPropagation();
        if (event.preventDefault) event.preventDefault();
    }

    function moveCurvePoint(event) {
        if (event.pointerId !== undefined && curveDragState.pointerId !== undefined &&
            event.pointerId !== curveDragState.pointerId) return;
        var dx = event.clientX - curveDragState.startX;
        var dy = event.clientY - curveDragState.startY;
        if (!curveDragState.moved && Math.abs(dx) + Math.abs(dy) < 3) return;
        curveDragState.moved = true;
        var curves = currentCurveState(curveDragState.kind, curveDragState.nodeId);
        var curve = curves && curves[curveDragState.kind];
        if (!curve) return;
        var plot = null;
        var plots = elements.inspectorBody.querySelectorAll(".age-curve-plot");
        for (var i = 0; i < plots.length; i++) {
            if (plots[i].dataset.curveKind === curveDragState.kind) { plot = plots[i]; break; }
        }
        if (!plot) return;
        var position = curvePosition(event, plot, curveDragState.valueScale);
        var index = curveDragState.index;
        if (index > 0 && index < curve.points.length - 1) {
            position.age = Math.max(curve.points[index - 1].age + 0.005,
                                    Math.min(curve.points[index + 1].age - 0.005, position.age));
            curve.points[index].age = roundCurveNumber(position.age, 3);
        }
        curve.points[index].value = roundCurveNumber(position.value, 1);
        drawCurvePlot(curveDragState.kind, curveDragState.valueScale);
        if (event.preventDefault) event.preventDefault();
    }

    function roundCurveNumber(value, decimals) {
        var scale = Math.pow(10, decimals);
        return Math.round(value * scale) / scale;
    }

    function applyCurveChanges(kind, points, custom, nodeId) {
        if (state.pending || !points || points.length < 2 || points.length > (state.graphMode?64:8)) return;
        if (state.graphMode) {
            var node = selectedGraphNode(nodeId);
            var keys = node && node.curveParameterKeys;
            var curveState = node && node.curves && node.curves[kind];
            if (!node || !keys || !curveState) return;
            if (!custom && !curveState.custom &&
                Math.abs(curveState.points[0].value - points[0].value) < 1e-9 &&
                Math.abs(curveState.points[curveState.points.length - 1].value - points[points.length - 1].value) < 1e-9) return;
            var graphChanges = [];
            var endKey = kind === "rotation" ? null : kind === "size" ? keys.sizeEnd : keys.opacityEnd;
            var endParameter = endKey ? graphParameterRecord(node, endKey) : null;
            if (endParameter && Math.abs(Number(endParameter.value) - points[points.length - 1].value) > 1e-9) {
                graphChanges.push({ nodeId: node.id, parameterKey: endKey,
                                    valueType: endParameter.type, value: points[points.length - 1].value });
            }
            var curveKey = keys[kind];
            if (custom) {
                var maximum = kind === "rotation" ? 32768 : 100;
                var payload = window.StarfieldGraphView.encodeCurve(points);
                window.StarfieldGraphView.decodeCurve(payload, kind === "rotation" ? -32768 : 0, maximum, points[0].value,
                                                       points[points.length - 1].value);
                graphChanges.push({ nodeId: node.id, parameterKey: curveKey, valueType: 7, value: payload });
            } else {
                graphChanges.push({ nodeId: node.id, parameterKey: curveKey, remove: true });
            }
            applyTopologyEdit({ type: "setParameters", changes: graphChanges });
            return;
        }
        var prefix = kind === "size" ? "size" : "opacity";
        var changes = [];
        for (var point = 0; point < 8; point++) {
            var value = point < points.length ? points[point] : { age: 0, value: 0 };
            changes.push({ key: prefix + "_curve_point_" + point + "_age", value: value.age });
            changes.push({ key: prefix + "_curve_point_" + point + "_value", value: value.value });
        }
        var endKey = kind === "size" ? "particle_size_end" : "opacity_end";
        if (Math.abs(Number(state.values[endKey]) - points[points.length - 1].value) > 1e-9) {
            changes.push({ key: endKey, value: points[points.length - 1].value });
        }
        changes.push({ key: prefix + "_curve_count", value: custom ? points.length : 0 });
        var nonce = Number(state.curveEditCommit) || 0;
        changes.push({ key: "curve_edit_commit", value: nonce >= 999999 ? -999999 : nonce + 1 });
        state.curves[kind] = { custom: !!custom, points: copyCurvePoints(points) };
        renderInspector();
        applyChanges(changes);
    }

    function positionInspector(nodeId) {
        var position = nodePositions[nodeId];
        if (!position || !elements.workspace || !elements.inspector) return;
        var width = elements.workspace.clientWidth;
        var height = elements.workspace.clientHeight;
        var popupWidth = elements.inspector.offsetWidth;
        var popupHeight = elements.inspector.offsetHeight;
        var card = graphNodeElements[nodeId] && graphNodeElements[nodeId].card;
        var workspaceBounds = elements.workspace.getBoundingClientRect();
        var cardBounds = card ? card.getBoundingClientRect() : null;
        var nodeOnRight = cardBounds ?
            cardBounds.left + cardBounds.width / 2 > workspaceBounds.left + width / 2 :
            position.x + NODE_WIDTH / 2 > width / 2;
        var nodeInUpperHalf = cardBounds ?
            cardBounds.top + cardBounds.height / 2 < workspaceBounds.top + height / 2 :
            position.y + NODE_HEIGHT / 2 < height / 2;
        var left = nodeOnRight ? 8 : width - popupWidth - 8;
        var top = nodeInUpperHalf ? height - popupHeight - 8 : 8;
        var bounded = clampInspectorPosition(left, top);
        elements.inspector.style.left = bounded.left + "px";
        elements.inspector.style.top = bounded.top + "px";
    }

    function clampInspectorPosition(left, top) {
        var panelWidth = elements.workspace.clientWidth;
        var panelHeight = elements.workspace.clientHeight;
        var maxLeft = Math.max(8, panelWidth - elements.inspector.offsetWidth - 8);
        var maxTop = Math.max(8, panelHeight - elements.inspector.offsetHeight - 8);
        return {
            left: Math.max(8, Math.min(maxLeft, left)),
            top: Math.max(8, Math.min(maxTop, top))
        };
    }

    function clampInspectorToWorkspace() {
        if (!elements.workspace || !elements.inspector || elements.inspector.hidden) return;
        var bounded = clampInspectorPosition(elements.inspector.offsetLeft, elements.inspector.offsetTop);
        if (elements.inspector.style.left !== bounded.left + "px") elements.inspector.style.left = bounded.left + "px";
        if (elements.inspector.style.top !== bounded.top + "px") elements.inspector.style.top = bounded.top + "px";
    }

    function schedulePanelResize() {
        if (resizeUpdatePending) return;
        resizeUpdatePending = true;
        var defer = window.requestAnimationFrame || function (callback) { window.setTimeout(callback, 16); };
        defer.call(window, function () {
            resizeUpdatePending = false;
            updateCanvasBounds();
            if (state.inspectorOpen) clampInspectorToWorkspace();
        });
    }

    function render(state) {
        if (nodePalette) nodePalette.refresh();
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
        var selectedCount = 0;
        for (var selectedId in state.selectedNodeIds) {
            if (!Object.prototype.hasOwnProperty.call(state.selectedNodeIds, selectedId)) continue;
            var foundSelection = false;
            for (var nodeIndex = 0; nodeIndex < state.nodes.length; nodeIndex++) {
                if (state.nodes[nodeIndex].id === selectedId) { foundSelection = true; break; }
            }
            if (!foundSelection) delete state.selectedNodeIds[selectedId];
            else selectedCount += 1;
        }
        if (selectedCount === 0) state.selectedNodeIds[state.selectedNodeId] = true;
        updateCanvasBounds();
        if (centerGraphOnNextRender) {
            centerGraphView();
            centerGraphOnNextRender = false;
            updateCanvasBounds();
        }
        for (var n = 0; n < state.nodes.length; n++) {
            if (isSupportedNode(state.nodes[n])) addGraphNode(state.nodes[n]);
        }
        renderEdges();
        renderInspector();
    }

    function beginNodeDrag(nodeId, event) {
        if (event.button !== 0 || event.isPrimary === false || state.pending) return;
        if (event.shiftKey || event.ctrlKey) return;
        capturePointer(event);
        if (event.altKey && event.preventDefault) event.preventDefault();
        if (!state.selectedNodeIds[nodeId]) {
            state.selectedNodeIds = {};
            state.selectedNodeIds[nodeId] = true;
            state.selectedNodeId = nodeId;
            syncNodeSelectionStyles();
        }
        var origins = [];
        for (var selectedId in state.selectedNodeIds) {
            if (!Object.prototype.hasOwnProperty.call(state.selectedNodeIds, selectedId)) continue;
            var selected = graphNodeElements[selectedId];
            if (!selected) continue;
            var position = nodePositions[selectedId];
            origins.push({ id: selectedId, card: selected.card, x: position.x, y: position.y });
        }
        dragState = { nodeId: nodeId, startX: event.clientX, startY: event.clientY,
                      lastX: event.clientX, lastY: event.clientY, origins: origins,
                      moved: false, copy: !!event.altKey, previewElements: [] };
    }

    function beginPortDrag(nodeId, direction, event) {
        if (event.button !== 0 || event.isPrimary === false || state.pending) return;
        event.preventDefault();
        event.stopPropagation();
        capturePointer(event);
        connectionState = { nodeId: nodeId, direction: direction,
                            portKey: event.currentTarget.getAttribute("data-port-key"),
                            point: canvasPoint(event.clientX, event.clientY) };
        renderEdges();
    }

    function nearestConnectionPort(event) {
        if (!connectionState || !state.graph || !window.StarfieldGraphEdits || !window.StarfieldGraphEdits.canConnect) return null;
        var nearest = null;
        var distanceSquared = 22 * 22;
        for (var nodeId in graphNodeElements) {
            if (!Object.prototype.hasOwnProperty.call(graphNodeElements, nodeId) || nodeId === connectionState.nodeId) continue;
            var port = connectionState.direction === "out" ? graphNodeElements[nodeId].input : graphNodeElements[nodeId].output;
            if (!port) continue;
            var bounds = port.getBoundingClientRect();
            var dx = event.clientX - (bounds.left + bounds.width / 2);
            var dy = event.clientY - (bounds.top + bounds.height / 2);
            var distance = dx * dx + dy * dy;
            if (distance > distanceSquared) continue;
            var from = connectionState.direction === "out" ? connectionState.nodeId : nodeId;
            var to = connectionState.direction === "in" ? connectionState.nodeId : nodeId;
            var outputPort = connectionState.direction === "out" ? connectionState.portKey : port.getAttribute("data-port-key");
            var inputPort = connectionState.direction === "in" ? connectionState.portKey : port.getAttribute("data-port-key");
            if (!window.StarfieldGraphEdits.canConnect(state.graph, from, to, outputPort, inputPort)) continue;
            nearest = port;
            distanceSquared = distance;
        }
        return nearest;
    }

    function updateConnectionPoint(event) {
        var port = nearestConnectionPort(event);
        if (connectionState.snapPort && connectionState.snapPort !== port) {
            connectionState.snapPort.classList.remove("connection-snap");
        }
        connectionState.snapPort = port;
        if (port) {
            port.classList.add("connection-snap");
            connectionState.point = portPoint(port.getAttribute("data-node-id"),
                                              connectionState.direction === "out" ? "top" : "bottom");
        } else {
            connectionState.point = canvasPoint(event.clientX, event.clientY);
        }
    }

    function beginMarquee(event) {
        if (event.button !== 0 || event.isPrimary === false || !elements.graphScroll || state.pending) return;
        if (event.target.closest && (event.target.closest(".graph-node") || event.target.closest(".edge-hit") ||
                                     event.target.closest(".graph-minimap"))) return;
        if (elements.graphScroll.focus) elements.graphScroll.focus();
        capturePointer(event);
        var point = canvasPoint(event.clientX, event.clientY);
        marqueeState = { start: point, current: point, moved: false, additive: !!(event.shiftKey || event.ctrlKey) };
        if (!marqueeState.additive) state.selectedNodeIds = {};
        if (elements.selectionBox) {
            elements.selectionBox.hidden = true;
            elements.selectionBox.style.left = point.x + "px";
            elements.selectionBox.style.top = point.y + "px";
            elements.selectionBox.style.width = "0px";
            elements.selectionBox.style.height = "0px";
        }
    }

    function updateMarquee(event) {
        if (!marqueeState) return;
        var point = canvasPoint(event.clientX, event.clientY);
        marqueeState.current = point;
        var dx = point.x - marqueeState.start.x;
        var dy = point.y - marqueeState.start.y;
        if (!marqueeState.moved && Math.abs(dx) + Math.abs(dy) < 4 / zoom) return;
        marqueeState.moved = true;
        var left = Math.min(point.x, marqueeState.start.x);
        var top = Math.min(point.y, marqueeState.start.y);
        if (elements.selectionBox) {
            elements.selectionBox.hidden = false;
            elements.selectionBox.style.left = left + "px";
            elements.selectionBox.style.top = top + "px";
            elements.selectionBox.style.width = Math.abs(dx) + "px";
            elements.selectionBox.style.height = Math.abs(dy) + "px";
        }
        if (document.body && document.body.classList) document.body.classList.add("marquee-selecting");
        if (event.preventDefault) event.preventDefault();
    }

    function finishMarquee() {
        if (!marqueeState) return;
        if (marqueeState.moved) {
            var left = Math.min(marqueeState.start.x, marqueeState.current.x);
            var right = Math.max(marqueeState.start.x, marqueeState.current.x);
            var top = Math.min(marqueeState.start.y, marqueeState.current.y);
            var bottom = Math.max(marqueeState.start.y, marqueeState.current.y);
            for (var i = 0; i < state.nodes.length; i++) {
                var node = state.nodes[i];
                var position = nodePositions[node.id];
                if (!position) continue;
                var display = displayPosition(position);
                var intersects = display.x <= right && display.x + NODE_WIDTH >= left &&
                                 display.y <= bottom && display.y + NODE_HEIGHT >= top;
                if (intersects) state.selectedNodeIds[node.id] = true;
            }
            var first = null;
            for (var selectedId in state.selectedNodeIds) {
                if (Object.prototype.hasOwnProperty.call(state.selectedNodeIds, selectedId)) { first = selectedId; break; }
            }
            if (first) state.selectedNodeId = first;
            syncNodeSelectionStyles();
            if (first) {
                state.inspectorOpen = true;
                renderInspector();
            } else {
                state.inspectorOpen = false;
                renderInspector();
            }
        } else {
            syncNodeSelectionStyles();
            if (!marqueeState.additive) {
                state.inspectorOpen = false;
                renderInspector();
            }
        }
        marqueeState = null;
        if (elements.selectionBox) elements.selectionBox.hidden = true;
        if (document.body && document.body.classList) document.body.classList.remove("marquee-selecting");
    }

    function zoomAt(event) {
        if (!elements.graphScroll || !elements.graphCanvas) return;
        if (event.preventDefault) event.preventDefault();
        var bounds = elements.graphScroll.getBoundingClientRect();
        var anchorX = event.clientX - bounds.left;
        var anchorY = event.clientY - bounds.top;
        var logicalX = (anchorX - viewPan.x) / zoom;
        var logicalY = (anchorY - viewPan.y) / zoom;
        var step = event.deltaY < 0 ? 1.1 : 1 / 1.1;
        zoom = Math.max(ZOOM_MIN, Math.min(ZOOM_MAX, zoom * step));
        viewPan.x = anchorX - logicalX * zoom;
        viewPan.y = anchorY - logicalY * zoom;
        updateCanvasBounds();
    }

    function beginCanvasPan(event) {
        if (!elements.graphScroll || event.button !== 1) return;
        event.preventDefault();
        capturePointer(event);
        panState = { startX: event.clientX, startY: event.clientY,
                     panX: viewPan.x, panY: viewPan.y };
        elements.graphScroll.classList.add("panning");
        if (document.body && document.body.classList) document.body.classList.add("panning-canvas");
    }

    function beginMinimapPan(event) {
        if (!elements.graphMinimap || event.button !== 0 || event.isPrimary === false) return;
        event.preventDefault();
        capturePointer(event);
        minimapPanState = true;
        moveViewToMinimapPoint(event);
    }

    function updateCanvasPan(event) {
        if (!panState || !elements.graphScroll) return;
        viewPan.x = panState.panX + (event.clientX - panState.startX);
        viewPan.y = panState.panY + (event.clientY - panState.startY);
        updateCanvasBounds();
        if (event.preventDefault) event.preventDefault();
    }

    function clearCopyPreviews() {
        if (!dragState || !dragState.previewElements) return;
        for (var i = 0; i < dragState.previewElements.length; i++) {
            var preview = dragState.previewElements[i];
            if (preview.parentNode) preview.parentNode.removeChild(preview);
        }
        dragState.previewElements.length = 0;
    }

    function updateCopyPreview(dx, dy) {
        if (!dragState.copy) return;
        if (!dragState.previewElements.length) {
            for (var i = 0; i < dragState.origins.length; i++) {
                var source = dragState.origins[i];
                var preview = source.card.cloneNode(true);
                preview.className += " drag-copy-preview";
                preview.setAttribute("aria-hidden", "true");
                preview.style.pointerEvents = "none";
                var previewPosition = displayPosition({ x: source.x + dx, y: source.y + dy });
                preview.style.left = previewPosition.x + "px";
                preview.style.top = previewPosition.y + "px";
                elements.chain.appendChild(preview);
                dragState.previewElements.push(preview);
            }
        } else {
            for (var j = 0; j < dragState.previewElements.length; j++) {
                var origin = dragState.origins[j];
                var position = displayPosition({ x: origin.x + dx, y: origin.y + dy });
                dragState.previewElements[j].style.left = position.x + "px";
                dragState.previewElements[j].style.top = position.y + "px";
            }
        }
    }

    function moveNodeDrag(event) {
        if (edgePressState) {
            var pressDx = event.clientX - edgePressState.startX;
            var pressDy = event.clientY - edgePressState.startY;
            if (pressDx * pressDx + pressDy * pressDy > 25) edgePressState.moved = true;
            return;
        }
        if (numericScrubState) { moveNumericScrub(event); return; }
        if (curveDragState) { moveCurvePoint(event); return; }
        if (minimapPanState) { moveViewToMinimapPoint(event); return; }
        if (panState) { updateCanvasPan(event); return; }
        if (inspectorDragState) {
            var nextLeft = inspectorDragState.originX + event.clientX - inspectorDragState.startX;
            var nextTop = inspectorDragState.originY + event.clientY - inspectorDragState.startY;
            var bounded = clampInspectorPosition(nextLeft, nextTop);
            elements.inspector.style.left = bounded.left + "px";
            elements.inspector.style.top = bounded.top + "px";
            if (event.preventDefault) event.preventDefault();
            return;
        }
        if (connectionState) {
            updateConnectionPoint(event);
            renderEdges();
            if (event.preventDefault) event.preventDefault();
            return;
        }
        if (marqueeState) { updateMarquee(event); return; }
        if (!dragState) return;
        var dx = event.clientX - dragState.startX;
        var dy = event.clientY - dragState.startY;
        dragState.lastX = event.clientX;
        dragState.lastY = event.clientY;
        if (!dragState.moved && Math.abs(dx) + Math.abs(dy) < 4) return;
        dragState.moved = true;
        suppressNextNodeClick = true;
        if (document.body && document.body.classList) document.body.classList.add("dragging-node");
        if (event.preventDefault) event.preventDefault();
        dx /= zoom;
        dy /= zoom;
        if (dragState.copy) {
            updateCopyPreview(dx, dy);
            updateCanvasBounds();
            return;
        }
        for (var i = 0; i < dragState.origins.length; i++) {
            var origin = dragState.origins[i];
            var position = nodePositions[origin.id];
            position.x = origin.x + dx;
            position.y = origin.y + dy;
        }
        updateCanvasBounds();
        renderEdges();
    }

    function endNodeDrag(event) {
        if (edgePressState) {
            if (!event || (event.pointerId !== undefined && event.pointerId !== edgePressState.pointerId)) return;
            var edgePress = edgePressState;
            edgePressState = null;
            var edgeDx = event.clientX - edgePress.startX;
            var edgeDy = event.clientY - edgePress.startY;
            if (event.type !== "pointercancel" && !edgePress.moved && edgeDx * edgeDx + edgeDy * edgeDy <= 25) {
                requestTopologyEdit(edgePress.edit);
            }
            return;
        }
        if (numericScrubState) {
            if (event && event.pointerId !== undefined && numericScrubState.pointerId !== event.pointerId) return;
            var scrub = numericScrubState;
            numericScrubState = null;
            if (document.body && document.body.classList) document.body.classList.remove("scrubbing-number");
            if (event && event.type === "pointercancel") {
                scrub.input.value = String(scrub.startValue);
            } else if (scrub.moved && Math.abs(scrub.value - scrub.startValue) >= scrub.halfStep) {
                scrub.input.value = String(scrub.value);
                if (scrub.commit) scrub.commit(scrub.value);
                else onEdit({ target: scrub.input });
            }
            return;
        }
        if (curveDragState) {
            var curveDrag = curveDragState;
            curveDragState = null;
            if (document.body && document.body.classList) document.body.classList.remove("dragging-age-curve");
            if (event && event.type === "pointercancel") {
                var originalCurves = currentCurveState(curveDrag.kind, curveDrag.nodeId);
                if (originalCurves && originalCurves[curveDrag.kind]) {
                    originalCurves[curveDrag.kind].points = curveDrag.original;
                }
                drawCurvePlot(curveDrag.kind);
            } else if (curveDrag.moved) {
                var editedCurves = currentCurveState(curveDrag.kind, curveDrag.nodeId);
                if (editedCurves && editedCurves[curveDrag.kind]) {
                    applyCurveChanges(curveDrag.kind, editedCurves[curveDrag.kind].points, true, curveDrag.nodeId);
                }
            }
            return;
        }
        if (minimapPanState) {
            minimapPanState = null;
        }
        if (panState) {
            panState = null;
            if (elements.graphScroll) elements.graphScroll.classList.remove("panning");
            if (document.body && document.body.classList) document.body.classList.remove("panning-canvas");
        }
        if (connectionState) {
            if (event && event.type !== "pointercancel") {
                updateConnectionPoint(event);
                var port = connectionState.snapPort;
                if (port && port.getAttribute("data-port-direction") !== connectionState.direction) {
                    var otherNodeId = port.getAttribute("data-node-id");
                    if (otherNodeId && otherNodeId !== connectionState.nodeId) {
                        var from = connectionState.direction === "out" ? connectionState.nodeId : otherNodeId;
                        var to = connectionState.direction === "in" ? connectionState.nodeId : otherNodeId;
                        var outputPort = connectionState.direction === "out" ? connectionState.portKey : port.getAttribute("data-port-key");
                        var inputPort = connectionState.direction === "in" ? connectionState.portKey : port.getAttribute("data-port-key");
                        // Port direction defines particle flow. Card positions are
                        // presentation only, so a valid output may connect to a node
                        // drawn above it.
                        requestTopologyEdit({ type: "connect", from: from, to: to,
                                              outputPort: outputPort, inputPort: inputPort });
                    }
                }
            }
            if (connectionState.snapPort) connectionState.snapPort.classList.remove("connection-snap");
            connectionState = null;
            renderEdges();
        }
        if (marqueeState) finishMarquee();
        if (dragState && dragState.moved && dragState.copy && (!event || event.type !== "pointercancel")) {
            var copyDx = (dragState.lastX - dragState.startX) / zoom;
            var copyDy = (dragState.lastY - dragState.startY) / zoom;
            requestTopologyEdit({ type: "duplicateNodes", nodeIds: selectedNodeIds(),
                                  offset: { x: copyDx, y: copyDy } });
        } else if (dragState && dragState.moved && (!event || event.type !== "pointercancel")) {
            var draggedNode = state.nodes.filter(function (node) { return node.id === dragState.nodeId; })[0];
            var edge = draggedNode ? edgeUnderNode(draggedNode.id) : null;
            if (draggedNode && edge) {
                var dropPosition = nodePositions[draggedNode.id];
                var groupPositions = {};
                for (var movedIndex = 0; movedIndex < dragState.origins.length; movedIndex++) {
                    var movedNodeId = dragState.origins[movedIndex].id;
                    if (movedNodeId === draggedNode.id || !nodePositions[movedNodeId]) continue;
                    groupPositions[movedNodeId] = {
                        x: Number(nodePositions[movedNodeId].x),
                        y: Number(nodePositions[movedNodeId].y)
                    };
                }
                requestTopologyEdit(window.StarfieldGraphEdits.createInsertEdit(
                    draggedNode.id, edge, dropPosition, groupPositions));
            }
        } else if (dragState && dragState.moved && event && event.type === "pointercancel") {
            for (var restoreIndex = 0; restoreIndex < dragState.origins.length; restoreIndex++) {
                var restoreOrigin = dragState.origins[restoreIndex];
                nodePositions[restoreOrigin.id] = { x: restoreOrigin.x, y: restoreOrigin.y };
            }
            updateCanvasBounds();
            renderEdges();
        }
        var shouldCommitLayout = !!(dragState && dragState.moved && !dragState.copy &&
                                    (!event || event.type !== "pointercancel"));
        clearCopyPreviews();
        dragState = null;
        inspectorDragState = null;
        if (document.body && document.body.classList) document.body.classList.remove("dragging-node");
        window.setTimeout(function () { suppressNextNodeClick = false; }, 0);
        if (shouldCommitLayout) commitNodeLayout();
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
                           (parameter.kind === "color" || parameter.kind === "point3d" || parameter.kind === "point2d" ? " multi" : "");

        if (parameter.kind === "color") {
            var swatch = document.createElement("span");
            swatch.className = "swatch";
            swatch.style.background = "rgb(" + parameter.value[0] + "," + parameter.value[1] + "," +
                                      parameter.value[2] + ")";
            holder.appendChild(swatch);
            for (var c = 0; c < 3; c++) {
                holder.appendChild(numberInput(parameter, c));
            }
        } else if (parameter.kind === "point3d" || parameter.kind === "point2d") {
            for (var a = 0; a < parameter.value.length; a++) {
                holder.appendChild(numberInput(parameter, a));
            }
        } else if (parameter.kind === "popup") {
            holder.appendChild(popupInput(parameter));
        } else {
            holder.appendChild(numberInput(parameter, null));
            if (parameter.unit) {
                var unit = document.createElement("span");
                unit.className = "param-unit";
                unit.textContent = parameter.unit;
                holder.appendChild(unit);
            }
        }
        var wrapper = document.createElement("div");
        wrapper.className = "parameter-row";
        wrapper.appendChild(label);
        wrapper.appendChild(holder);
        return wrapper;
    }

    function popupInput(parameter) {
        var select = document.createElement("select");
        var choices = parameter.choices && parameter.choices.length ? parameter.choices :
                      ["Point", "Box", "Sphere", "Disc"];
        for (var i = 0; i < choices.length; i++) {
            var option = document.createElement("option");
            option.value = String(i + 1);
            option.textContent = choices[i];
            select.appendChild(option);
        }
        select.value = String(parameter.value);
        select.dataset.key = parameter.key;
        select.dataset.channel = "";
        select.dataset.decimals = "0";
        if (parameter.graphNodeId) {
            select.dataset.graphNodeId = parameter.graphNodeId;
            select.dataset.graphKey = parameter.graphKey;
            select.dataset.graphType = String(parameter.graphType);
            select.disabled = parameter.readonly === true;
        }
        select.title = parameter.label;
        select._graphParameter = parameter;
        select.addEventListener("change", onEdit);
        return select;
    }

    function numberInput(parameter, channel) {
        var input = document.createElement("input");
        input.type = "number";
        var decimals = parameterDecimals(parameter);
        input.step = String(numericScrubStep(parameter, decimals));
        var value = channel === null ? parameter.value : parameter.value[channel];
        input.value = formatParameterNumber(value, decimals);
        input.dataset.key = parameter.key;
        input.dataset.channel = channel === null ? "" : String(channel);
        input.dataset.decimals = String(decimals);
        input._graphParameter = parameter;
        if (parameter.graphNodeId) {
            input.dataset.graphNodeId = parameter.graphNodeId;
            input.dataset.graphKey = parameter.graphKey;
            input.dataset.graphType = String(parameter.graphType);
            input.disabled = parameter.readonly === true;
        }
        input.dataset.scrubStep = String(numericScrubStep(parameter, decimals));
        var min = parameter.kind === "color" ? 0 : parameter.min;
        var max = parameter.kind === "color" ? 255 : parameter.max;
        if (channel !== null && parameter.channelMin) min = parameter.channelMin[channel];
        if (channel !== null && parameter.channelMax) max = parameter.channelMax[channel];
        if (typeof min === "number") {
            input.min = String(min);
            input.dataset.min = String(min);
        }
        if (typeof max === "number") {
            input.max = String(max);
            input.dataset.max = String(max);
        }
        input.title = "Drag left or right to adjust · Shift: faster · Ctrl: finer · Click to type";
        input.addEventListener("pointerdown", beginNumericScrub);
        input.addEventListener("change", onEdit);
        return input;
    }

    function parameterDecimals(parameter) {
        var decimals = Number(parameter.displayDecimals);
        if (!isFinite(decimals)) decimals = parameter.kind === "popup" || parameter.kind === "color" ? 0 : 2;
        return Math.max(0, Math.min(6, Math.floor(decimals)));
    }

    function formatParameterNumber(value, decimals) {
        var numeric = Number(value);
        if (!isFinite(numeric)) return String(value);
        var scale = Math.pow(10, decimals);
        var rounded = Math.round(numeric * scale) / scale;
        if (decimals === 0) return String(Math.round(rounded));
        return rounded.toFixed(decimals);
    }

    function numericScrubStep(parameter, decimals) {
        var explicit = Number(parameter.scrubStep);
        if (isFinite(explicit) && explicit > 0) return explicit;
        return decimals === 0 ? 1 : Math.pow(10, -decimals);
    }

    function beginNumericScrub(event) {
        if (event.button !== 0 || event.isPrimary === false || state.pending || numericScrubState) return;
        var input = event.currentTarget;
        var value = Number(input.value);
        if (!isFinite(value)) return;
        var step = Number(input.dataset.scrubStep);
        var decimals = Number(input.dataset.decimals);
        if (!isFinite(step) || step <= 0) step = 1;
        if (!isFinite(decimals)) decimals = 2;
        numericScrubState = {
            input: input,
            pointerId: event.pointerId,
            startX: event.clientX,
            lastX: event.clientX,
            startValue: value,
            rawValue: value,
            value: value,
            step: step,
            decimals: decimals,
            halfStep: 0.5 * Math.pow(10, -decimals),
            min: input.dataset.min === undefined ? -Infinity : Number(input.dataset.min),
            max: input.dataset.max === undefined ? Infinity : Number(input.dataset.max),
            commit: typeof input._curveCommit === "function" ? input._curveCommit : null,
            moved: false
        };
        capturePointer(event);
    }

    function moveNumericScrub(event) {
        var scrub = numericScrubState;
        if (!scrub || (event.pointerId !== undefined && scrub.pointerId !== event.pointerId)) return;
        var x = event.clientX;
        if (!scrub.moved) {
            if (Math.abs(x - scrub.startX) < 3) return;
            scrub.moved = true;
            if (document.body && document.body.classList) document.body.classList.add("scrubbing-number");
        }
        var deltaX = x - scrub.lastX;
        scrub.lastX = x;
        var speed = event.shiftKey ? 10 : (event.ctrlKey ? 0.1 : 1);
        scrub.rawValue += deltaX * scrub.step * speed;
        scrub.rawValue = Math.max(scrub.min, Math.min(scrub.max, scrub.rawValue));
        var scale = Math.pow(10, scrub.decimals);
        scrub.value = Math.round(scrub.rawValue * scale) / scale;
        scrub.value = Math.max(scrub.min, Math.min(scrub.max, scrub.value));
        scrub.input.value = formatParameterNumber(scrub.value, scrub.decimals);
        if (event.preventDefault) event.preventDefault();
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
        var decimals = Number(input.dataset.decimals);
        if (isFinite(decimals)) {
            var scale = Math.pow(10, Math.max(0, Math.min(6, Math.floor(decimals))));
            raw = Math.round(raw * scale) / scale;
            input.value = formatParameterNumber(raw, Math.max(0, Math.min(6, Math.floor(decimals))));
        }
        if (state.graphMode) {
            var parameter = input._graphParameter;
            if (!parameter || parameter.readonly || !input.dataset.graphNodeId ||
                !/^\d+$/.test(input.dataset.graphKey || "")) {
                showError("parameter_not_editable", "This graph parameter has no editable panel binding.");
                return;
            }
            var minimum = parameter.kind === "color" ? 0 : parameter.min;
            var maximum = parameter.kind === "color" ? 255 : parameter.max;
            if (channel !== null && parameter.channelMin) minimum = parameter.channelMin[channel];
            if (channel !== null && parameter.channelMax) maximum = parameter.channelMax[channel];
            if (typeof minimum === "number") raw = Math.max(minimum, raw);
            if (typeof maximum === "number") raw = Math.min(maximum, raw);
            input.value = formatParameterNumber(raw, isFinite(decimals) ? decimals : 2);
            var displayValue = Object.prototype.toString.call(parameter.value) === "[object Array]"
                ? parameter.value.slice() : parameter.value;
            if (channel === null) displayValue = raw;
            else displayValue[channel] = raw;
            var graphValue = window.StarfieldGraphView.parameterToGraphValue(parameter, displayValue);
            var graphChanges = [{ nodeId: input.dataset.graphNodeId,
                                  parameterKey: input.dataset.graphKey,
                                  valueType: Number(input.dataset.graphType), value: graphValue }];
            applyChanges(graphChanges);
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
        if (state.graphMode) {
            if (!changes || !changes.length || !changes[0].nodeId) {
                showError("graph_parameter_missing", "Graph edits require a node ID and graph parameter key.");
                return;
            }
            applyTopologyEdit({ type: "setParameters", changes: changes });
            return;
        }
        // A poll started before this write may return an older snapshot after the
        // write succeeds. Invalidate that response so only the write or a later
        // read can update the panel state.
        refreshEpoch += 1;
        state.pending = true;
        call("setParameters", { changes: changes }, function (response) {
            state.pending = false;
            if (!response.ok) {
                var error = response.error || { code: "unknown", message: "Unknown failure." };
                showError(error.code, error.message, true);
                refresh();
                return;
            }
            clearBanner(true);
            adoptState(response);
        });
    }

    function adoptState(response, graphSnapshot) {
        lastFullRefreshAt = Date.now();
        var targetChanged = state.targetToken !== response.target.token;
        var graphChanged = window.StarfieldGraphView.graphSnapshotChanged(state.graphSnapshot, graphSnapshot);
        var resourcesChanged=!!graphSnapshot && (!state.graphSnapshot ||
            JSON.stringify(state.graphSnapshot.layerResources)!==JSON.stringify(graphSnapshot.layerResources));
        var graphLayoutChanged = false;
        var changed = targetChanged || state.revision !== response.revision || graphChanged || resourcesChanged ||
                      state.controlSource !== response.controlSource;
        if (targetChanged) resetViewForTarget();
        var layoutChanged = false;
        state.controlSource = response.controlSource;
        state.graphMode = response.controlSource === "Node Graph";
        state.topologyReady = !!(graphSnapshot && graphSnapshot.initialized);
        if (state.graphMode && state.topologyReady) {
            try {
                var reusable = !targetChanged && state.graph && state.graphSnapshot &&
                    state.graphSnapshot.graphHex === graphSnapshot.graphHex &&
                    JSON.stringify(state.graphSnapshot.geometry) === JSON.stringify(graphSnapshot.geometry) &&
                    JSON.stringify(state.graphSnapshot.layerResources) === JSON.stringify(graphSnapshot.layerResources);
                if (!reusable) {
                    var graph = window.StarfieldGraphCodec.fromHex(graphSnapshot.graphHex);
                    var view = window.StarfieldGraphView.project(graph, null, graphSnapshot.geometry, graphSnapshot.layerResources);
                    graphLayoutChanged = !window.StarfieldGraphView.samePositions(nodePositions, view.positions);
                    state.graph = graph;
                    state.nodes = view.nodes;
                    state.edges = view.edges;
                    nodePositions = view.positions;
                }
                state.graphSnapshot = graphSnapshot;
                state.layoutPersistence = true;
            } catch (graphError) {
                state.topologyReady = false;
                state.nodes = [];
                state.edges = [];
                showError(graphError.code || "invalid_graph", graphError.message || String(graphError));
            }
        } else if (state.graphMode) {
            state.graph = null;
            state.graphSnapshot = null;
            state.nodes = [];
            state.edges = [];
            state.layoutPersistence = true;
        } else {
            state.graph = null;
            state.graphSnapshot = graphSnapshot || null;
            state.nodes = response.nodes || [];
            state.edges = response.edges || [];
            layoutChanged = response.layoutPersistence === true ? applyProjectNodeLayout(response.layout) : false;
            state.layoutPersistence = response.layoutPersistence === true;
        }
        state.curves = response.curves || null;
        state.curveEditCommit = response.curveEditCommit;
        state.revision = response.revision;
        state.targetToken = response.target.token;
        if (targetChanged) {
            state.frameStatus = null;
            state.liveParticleCount = null;
        }
        resolvedTarget = true;
        elements.targetLine.textContent = response.target.comp + " / " + response.target.layer;
        elements.modeLine.textContent = "Mode: " + response.controlSource;
        elements.revisionLine.textContent = "Revision: " + response.revision +
            (state.graphMode && graphSnapshot ? " / Graph " + graphSnapshot.revision : "");
        elements.resolutionLine.textContent = "Lookup: " + response.resolution +
            (state.graphMode ? " · Layout: AE graph" : state.layoutPersistence ? " · Layout: AE project" : " · Layout: session only");
        if (state.graphMode && !state.topologyReady) {
            if (!elements.banner.classList.contains("error")) showError("graph_snapshot_unavailable", "The canonical graph could not be read from this effect.");
        } else if (state.graphMode || state.layoutPersistence) {
            clearBanner();
        } else if (!retainedError) {
            elements.banner.className = "banner";
            elements.banner.textContent = "This plug-in build lacks node-layout streams. The graph uses default positions; update the plug-in to save node moves in the AE project.";
        }
        syncTargetLock();
        if (changed || layoutChanged || graphLayoutChanged) render(state);
        if (nodePalette) nodePalette.refresh();
        if (latestFrameStatusResponse && latestFrameStatusResponse.targetToken === state.targetToken) adoptFrameStatus(latestFrameStatusResponse);
        else updateFrameStatus();
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
        call("getPanelState", null, function (response) {
            if (epoch !== refreshEpoch) { refreshInFlight = false; lastPulseStamp = null; return; }
            if (canvasInteractionActive() || state.pending) { refreshInFlight = false; lastPulseStamp = null; return; }
            if (!response.ok) {
                refreshInFlight = false;
                var error = response.error || { code: "unknown", message: "Unknown failure." };
                if (autoRetry && isStartupRetryable(error.code)) {
                    var retryIndex = Math.min(startupRetryAttempt, STARTUP_RETRY_DELAYS_MS.length - 1);
                    var delay = STARTUP_RETRY_DELAYS_MS[retryIndex];
                    startupRetryAttempt += 1;
                    if (!retainedError) {
                        elements.banner.className = "banner";
                        elements.banner.textContent = "Connecting to After Effects; retrying shortly...";
                    }
                    startupRetryTimer = window.setTimeout(function () {
                        startupRetryTimer = null;
                        if (epoch === refreshEpoch) refresh(true, false);
                    }, delay);
                    return;
                }
                showError(error.code, error.message);
                if (state.pinnedTargetToken) {
                    elements.targetLine.textContent = "Pinned target unavailable";
                    state.revision = null;
                    state.targetToken = null;
                    state.nodes = [];
                    state.edges = [];
                    state.frameStatus = null;
                    state.liveParticleCount = null;
                    render(state);
                    syncTargetLock();
                    return;
                }
                elements.targetLine.textContent = "No target";
                state.revision = null;
                state.targetToken = null;
                state.nodes = [];
                state.edges = [];
                state.frameStatus = null;
                state.liveParticleCount = null;
                render(state);
                return;
            }
            startupRetryAttempt = 0;
            loadGraphSnapshot(response.target.token, function (snapshotResponse) {
                refreshInFlight = false;
                if (epoch !== refreshEpoch || canvasInteractionActive() || state.pending) { lastPulseStamp = null; return; }
                if (!snapshotResponse || !snapshotResponse.ok || !snapshotResponse.snapshot ||
                    snapshotResponse.snapshot.initialized !== true) {
                    var graphError = snapshotResponse && snapshotResponse.error ||
                        { code: "graph_snapshot_unavailable", message: "The canonical graph could not be read from this effect." };
                    showError(graphError.code, graphError.message,
                        !!failedGraphInitializations[response.target.token]);
                    // Keep the last valid canvas during a host read failure.
                    if (!state.nodes.length || state.targetToken !== response.target.token) adoptState(response, null);
                    if (nodePalette) nodePalette.refresh();
                    return;
                }
                // The actual sibling effects own topology and values, including
                // the first automatic Emitter + Particle initialization.
                clearBanner();
                adoptState(response, snapshotResponse.snapshot);
            }, response);
        });
    }

    function schedulePanelPoll(delay) {
        if (panelPollTimer !== null) window.clearTimeout(panelPollTimer);
        panelPollTimer = window.setTimeout(pollPanelState,delay);
    }

    function pollPanelState() {
        panelPollTimer = null;
        if (panelPulseInFlight || refreshInFlight || frameStatusInFlight || state.pending ||
            effectSelectionInFlight || canvasInteractionActive() || document.hidden ||
            (elements.autoRefresh && !elements.autoRefresh.checked)) {
            schedulePanelPoll(IDLE_POLL_INTERVAL_MS); return;
        }
        panelPulseInFlight = true;
        call("getPanelPulse",null,function(response) {
            panelPulseInFlight = false;
            if (state.pending || canvasInteractionActive() || document.hidden) {
                schedulePanelPoll(IDLE_POLL_INTERVAL_MS); return;
            }
            if (response && response.ok) {
                var changed = response.stamp !== lastPulseStamp;
                lastPulseStamp = response.stamp;
                unchangedPulseCount = changed ? 0 : unchangedPulseCount+1;
                adoptFrameStatus(response.frameStatus);
                if (changed || !resolvedTarget || Date.now()-lastFullRefreshAt >= FULL_AUDIT_INTERVAL_MS) refresh(false,false);
            } else {
                unchangedPulseCount++;
                lastPulseStamp = null;
                if (state.targetToken) refresh(false,false);
            }
            schedulePanelPoll(unchangedPulseCount >= 3 ? IDLE_POLL_INTERVAL_MS : ACTIVE_POLL_INTERVAL_MS);
        });
    }

    // Prove the panel body executed, even when the host bridge is unavailable.
    elements.banner.className = "banner";
    elements.banner.textContent = "Panel script loaded; asking the host for the selected effect...";

    elements.refresh.addEventListener("click", function () {
        if (state.pending || refreshInFlight) return;
        failedGraphInitializations = {};
        clearBanner(true);
        refresh(true, true);
    });
    if (elements.graphCanvas) elements.graphCanvas.addEventListener("contextmenu", showGraphContextMenu);
    if (window.StarfieldNodePalette) nodePalette = window.StarfieldNodePalette.create({
        root: document.getElementById("nodePalette"),
        items: document.getElementById("nodePaletteItems"),
        toggle: document.getElementById("nodePaletteToggle"), viewport: elements.graphScroll,
        prepare: hideGraphContextMenu,
        guard: function () {
            if (!state.targetToken || !state.revision || state.pending || canvasInteractionActive(true)) return null;
            return JSON.stringify([state.targetToken,state.revision]);
        },
        isBlocked: function (element) { return !!closestElement(element,".graph-minimap"); },
        zoom: function () { return zoom; }, resize: schedulePanelResize,
        add: function (type,clientX,clientY) {
            var point=canvasPoint(clientX,clientY);
            requestTopologyEdit({type:"addNode",nodeType:type,
                position:{x:point.x-canvasOffset.x-NODE_WIDTH/2,y:point.y-canvasOffset.y-NODE_HEIGHT/2}});
            if (nodePalette) nodePalette.refresh();
        }
    });
    if (elements.contextMenu) elements.contextMenu.addEventListener("click", handleContextMenuAction);
    if (elements.graphScroll) {
        elements.graphScroll.addEventListener("pointerdown", beginCanvasPan);
        elements.graphScroll.addEventListener("auxclick", function (event) {
            if (event.button === 1) event.preventDefault();
        });
        elements.graphScroll.addEventListener("contextmenu", function (event) { event.preventDefault(); });
    }
    if (elements.graphMinimap) elements.graphMinimap.addEventListener("pointerdown", beginMinimapPan);
    if (elements.targetLock) elements.targetLock.addEventListener("click", function () {
        if (state.pinnedTargetToken) {
            state.pinnedTargetToken = null;
            try { window.localStorage.removeItem(PIN_STORAGE_KEY); } catch (ignored) { /* session unlock still works */ }
        } else if (state.targetToken) {
            state.pinnedTargetToken = state.targetToken;
            try { window.localStorage.setItem(PIN_STORAGE_KEY, state.pinnedTargetToken); } catch (ignored) { /* session pin still works */ }
        }
        syncTargetLock();
        refresh(true, true);
    });
    if (elements.graphScroll) elements.graphScroll.addEventListener("pointerdown", beginMarquee);
    if (elements.graphScroll) elements.graphScroll.addEventListener("wheel", zoomAt, false);
    document.addEventListener("pointerdown", function (event) {
        if (elements.contextMenu && !elements.contextMenu.hidden && !elements.contextMenu.contains(event.target)) {
            hideGraphContextMenu();
        }
    });
    document.addEventListener("keydown", function (event) {
        if (event.key === "Escape") { hideGraphContextMenu(); if(nodePalette)nodePalette.cancel(); return; }
        var target = event.target;
        if (target && (target.tagName === "INPUT" || target.tagName === "TEXTAREA" ||
                       target.tagName === "SELECT" || target.isContentEditable)) return;
        if ((event.key === "Delete" || event.key === "Backspace") &&
            !event.ctrlKey && !event.metaKey && !event.altKey && editableSelectedNodeIds().length) {
            event.preventDefault();
            if (event.repeat || state.pending) return;
            hideGraphContextMenu();
            deleteSelection();
            return;
        }
        if ((event.ctrlKey || event.metaKey) && String(event.key).toLowerCase() === "d" &&
            editableSelectedNodeIds().length) {
            event.preventDefault();
            duplicateSelection(28, 28);
        }
    });
    syncTargetLock();

    // AE can change the effect behind the panel's back: Ctrl+Z, keyframes, or an edit in
    // the Effect Controls window. The protocol has no push channel, so without this the
    // panel keeps displaying whatever it last adopted (host-side undo left a stale 40 on
    // screen). Re-read whenever the panel regains focus, but never race a pending write.
    if (window.addEventListener) {
        window.addEventListener("focus", function () { refresh(false, false); });
        window.addEventListener("pointermove", moveNodeDrag);
        window.addEventListener("pointerup", endNodeDrag);
        window.addEventListener("pointercancel", endNodeDrag);
        window.addEventListener("resize", schedulePanelResize);
    }
    if (window.ResizeObserver && elements.graphScroll) {
        var graphViewportObserver = new window.ResizeObserver(function () {
            var size = elements.graphScroll.clientWidth + ":" + elements.graphScroll.clientHeight + ":" +
                (elements.workspace ? elements.workspace.clientWidth + ":" + elements.workspace.clientHeight : "");
            if (size === observedPanelSize) return;
            observedPanelSize = size;
            // Write layout in a later frame, outside ResizeObserver delivery.
            schedulePanelResize();
        });
        graphViewportObserver.observe(elements.graphScroll);
        if (elements.workspace) graphViewportObserver.observe(elements.workspace);
    }
    if (elements.inspectorDrag) elements.inspectorDrag.addEventListener("pointerdown", beginInspectorDrag);
    if (elements.closeInspector) elements.closeInspector.addEventListener("click", function () {
        state.inspectorOpen = false;
        elements.inspector.hidden = true;
        shownInspectorNodeId = null;
    });
    schedulePanelPoll(IDLE_POLL_INTERVAL_MS);
    if (document.addEventListener) {
        document.addEventListener("visibilitychange", function () {
            if (!document.hidden &&
                (!elements.autoRefresh || elements.autoRefresh.checked)) refresh(false, false);
        });
    }
    if (elements.autoRefresh) elements.autoRefresh.addEventListener("change", function () {
        if (elements.autoRefresh.checked) refresh(false, false);
    });

    refresh(true, true);
})();
