// Starfield Node Editor: graph and preset operations only. Node parameters are
// edited in AE Effect Controls; selecting a node focuses its native effect.

(function () {
    "use strict";

    var REQUEST_TIMEOUT_MS = 8000;
    var GATEWAY_BUILD = "native-presets-100";
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
    var state = { revision: null, targetToken: null, nodes: [], edges: [],
                  selectedNodeId: "emitter", selectedNodeIds: {}, pending: false,
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

    // Check and invoke in the same host turn. Matching-generation code is reused;
    // request-local caches clear at parse/reply, never across a host mutation.
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
        var modelOperation=operation.indexOf("ModelGraph")>=0;
        var script="(function(){try{"+
            "if(typeof SFLD_ready!=='function' || SFLD_ready()!=="+quote(GATEWAY_READY_TOKEN)+
            (modelOperation?" || typeof __SFLD_modelTransactionAPI==='undefined'":" || typeof SFLD_"+operation+"!=='function'")+
            "){$.evalFile(new File("+quote(root+"/jsx/starfield_gateway.jsx")+"));}"+
            "if(typeof SFLD_ready!=='function' || SFLD_ready()!=="+quote(GATEWAY_READY_TOKEN)+")throw new Error('Gateway generation mismatch.');"+
            (modelOperation?"if(typeof SFLD_modelTransactionReady!=='function' || SFLD_modelTransactionReady()!=="+quote(GATEWAY_BUILD)+
                " || typeof SFLD_"+operation+"!=='function'){$.evalFile(new File("+quote(root+"/jsx/model_transaction_transport.jsx")+"));}":"")+
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
        emitter: true, particle: true, force: true, transform: true,motion:true,turbulence:true, model: true, material:true, output: true
    };
    var DEFAULT_NODE_POSITIONS = {
        emitter: { x: 235, y: 22 }, particle: { x: 235, y: 100 },
        force: { x: 235, y: 178 }, transform: {x:235,y:220},motion:{x:235,y:220},turbulence:{x:235,y:262}, model:{x:90,y:100}, material:{x:90,y:30}, output: { x: 235, y: 256 }
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
        return !!(dragState || panState || marqueeState || connectionState || edgePressState || minimapPanState || (!ignorePalette && nodePalette && nodePalette.isDragging()) || (elements.contextMenu && !elements.contextMenu.hidden));
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
        var colors = { emitter: "#e5aa69", particle: "#b7a0e9", force: "#94a9ed", transform:"#8dc9dd",motion:"#8dc9dd",turbulence:"#a7a1ec", model:"#8fc4a5",material:"#d3b67d",
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
        if (side === "model") return {x:display.x,y:display.y+NODE_HEIGHT/2};
        return { x: display.x + NODE_WIDTH / 2, y: display.y };
    }

    function edgePath(edge, sides) {
        var startSide = sides[edge[0]] ? sides[edge[0]].output : "bottom";
        var endSide = edge.inputPort === "3" ? "model" : sides[edge[1]] ? sides[edge[1]].input : "top";
        var start = portPoint(edge[0], startSide);
        var end = portPoint(edge[1], endSide);
        if (!start || !end) return null;
        var distance = Math.max(12, Math.min(80, Math.abs(end.y - start.y) * 0.45));
        return "M" + start.x + " " + start.y + " C" + start.x + " " + (start.y + distance) + " " +
               (endSide==="model"?end.x-distance:end.x) + " " + (endSide==="model"?end.y:end.y-distance) + " " + end.x + " " + end.y;
    }

    function placePort(port, side) {
        if (!port) return;
        if(side==="model") {port.style.left="-6px";port.style.top="calc(50% - 5px)";return;}
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
        if(kind==="material")return graphParameterValue(node,1)===1?"Material Color":"Particle Color";
        if(kind==="motion")return "Circle";
        if(kind==="turbulence")return graphParameterValue(node,8)===0?"Normal":graphParameterValue(node,8)===3?"Axis":"One Side";
        if(kind==="model")return graphParameterValue(node,13)===1?"OBJ":"Cube";
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
        button.title = "Edit " + node.label + " in AE Effect Controls";
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
            selectNativeEffect(node.id);
            syncNodeSelectionStyles();
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
        if(node.modelInputPort) {
            var modelInput=document.createElement("span");
            modelInput.className="port port-in port-model";modelInput.title="Model input";
            modelInput.setAttribute("data-node-id",node.id);modelInput.setAttribute("data-port-direction","in");
            modelInput.setAttribute("data-port-key",node.modelInputPort);
            modelInput.addEventListener("pointerdown",function(event){beginPortDrag(node.id,"in",event);});
            card.appendChild(modelInput);portElements.modelInput=modelInput;
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
            placePort(graphNodeElements[nodeId].modelInput,"model");
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
            var fixedSide = connectionState.direction === "in" && connectionState.portKey==="3" ? "model" :
                connectionState.direction === "out" ? side.output : side.input;
            var fixedPoint = portPoint(connectionState.nodeId, fixedSide ||
                (connectionState.direction === "out" ? "bottom" : "top"));
            if (fixedPoint) {
                var start = connectionState.direction === "out" ? fixedPoint : connectionState.point;
                var end = connectionState.direction === "out" ? connectionState.point : fixedPoint;
                var bend = Math.max(12, Math.min(80, Math.abs(end.y - start.y) * 0.45));
                var c1x = start.x;
                var c1y = start.y + bend;
                var modelEnd=connectionState.direction === "in" ? connectionState.portKey==="3" :
                    connectionState.snapPort && connectionState.snapPort.getAttribute("data-port-key")==="3";
                var c2x = modelEnd ? end.x-bend : end.x;
                var c2y = modelEnd ? end.y : end.y-bend;
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

    var topologyPreview = null;
    function previewTopologyGraph(graph, snapshot) {
        var view = window.StarfieldGraphView.project(graph, null, snapshot.geometry, snapshot.layerResources);
        topologyPreview = {graph:state.graph,nodes:state.nodes,edges:state.edges,positions:nodePositions,
            graphMode:state.graphMode,topologyReady:state.topologyReady,layoutPersistence:state.layoutPersistence,
            selectedNodeId:state.selectedNodeId,selectedNodeIds:JSON.parse(JSON.stringify(state.selectedNodeIds))};
        state.graph=graph;state.nodes=view.nodes;state.edges=view.edges;nodePositions=view.positions;
        state.graphMode=true;state.topologyReady=true;state.layoutPersistence=true;
        render(state);
    }
    function restoreTopologyPreview() {
        if (!topologyPreview) return;
        nodePositions=topologyPreview.positions;
        Object.keys(topologyPreview).forEach(function(key){if(key!=="positions")state[key]=topologyPreview[key];});
        topologyPreview=null;render(state);
    }
    function confirmTopologySnapshot(snapshot) {
        var graph=window.StarfieldGraphCodec.fromHex(snapshot.graphHex);
        var view=window.StarfieldGraphView.project(graph,null,snapshot.geometry,snapshot.layerResources);
        state.graph=graph;state.nodes=view.nodes;state.edges=view.edges;nodePositions=view.positions;
        state.graphSnapshot=snapshot;state.graphMode=true;state.topologyReady=true;state.layoutPersistence=true;
        topologyPreview=null;render(state);
    }
    function afterTopologyPaint(submit) {
        function frame(fn){if(window.requestAnimationFrame)window.requestAnimationFrame(fn);else window.setTimeout(fn,16);}
        var sent=false;
        function once(){if(sent)return;sent=true;submit();}
        // A second frame lets Chromium paint the local edit before evalScript
        // begins a potentially synchronous AE command. Hidden panels still progress.
        if(document.hidden){window.setTimeout(once,0);return;}
        window.setTimeout(once,100); // A panel hidden between frames still submits.
        frame(function(){frame(function(){window.setTimeout(once,0);});});
    }
    function loadModelModules(callback) {
        if(!window.StarfieldOptionalModules){callback({ok:false,error:{code:"model_transaction_unavailable",message:"Optional module loader is unavailable."}});return;}
        window.StarfieldOptionalModules.loadModel(GATEWAY_BUILD.split("-").pop(),callback);
    }
    function getGraphTransactionClient() {
        if (!graphTransactionClient && window.StarfieldGraphTransactions && window.StarfieldGraphCodec &&
            window.StarfieldGraphEdits) {
            graphTransactionClient = window.StarfieldGraphTransactions.create({
                call: call,
                codec: window.StarfieldGraphCodec,
                edits: window.StarfieldGraphEdits,
                loadModelModules: loadModelModules,
                onPlanned: previewTopologyGraph,
                scheduleSubmit: afterTopologyPaint
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
                    restoreTopologyPreview();
                    var error = result && result.error || { code: "graph_edit_failed", message: "The graph transaction failed." };
                    showError(error.code, error.message, true);
                    refresh(false, false);
                    return;
                }
                try { confirmTopologySnapshot(result.snapshot); }
                catch (confirmationError) {
                    topologyPreview=null;
                    showError("graph_snapshot_unconfirmed",confirmationError.message || String(confirmationError),true);
                    refresh(false,false);return;
                }
                var completion=result.diagnostics,completionWarning=completion && (completion.cleanupError || completion.undoError || completion.notificationPending);
                if(completionWarning)showError("graph_applied_with_warning","The graph was applied, but After Effects reported a completion issue. Refresh before continuing.",true);
                else clearBanner(true);
                refresh(false, false);
            }, targetToken, snapshotResponse.snapshot.revision, snapshotResponse);
        }
        if (state.graphMode && state.topologyReady && state.graphSnapshot) {
            commitMappedEdit({ ok: true, target:{token:targetToken}, snapshot: state.graphSnapshot });
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
            if (!edge || edge.length !== 2 || edge.inputPort==="3" || edge[0] === nodeId || edge[1] === nodeId) continue;
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

    function schedulePanelResize() {
        if (resizeUpdatePending) return;
        resizeUpdatePending = true;
        var defer = window.requestAnimationFrame || function (callback) { window.setTimeout(callback, 16); };
        defer.call(window, function () {
            resizeUpdatePending = false;
            updateCanvasBounds();
        });
    }

    function render(state) {
        if (nodePalette) nodePalette.refresh();
        elements.chain.innerHTML = "";
        graphNodeElements = {};
        if (!state.nodes.length) {
            elements.chain.innerHTML = "<p class=\"hint\">Select one layer carrying Starfield Particle.</p>";
            renderEdges();
            return;
        }
        var selectedExists = false;
        for (var i = 0; i < state.nodes.length; i++) {
            var node = state.nodes[i];
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
            var candidates=connectionState.direction === "out" ?
                [graphNodeElements[nodeId].input,graphNodeElements[nodeId].modelInput] : [graphNodeElements[nodeId].output];
            for(var candidateIndex=0;candidateIndex<candidates.length;candidateIndex++) {
            var port=candidates[candidateIndex];
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
                                              connectionState.direction === "out" ? (port.getAttribute("data-port-key")==="3"?"model":"top") : "bottom");
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
        } else {
            syncNodeSelectionStyles();
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
        if (minimapPanState) { moveViewToMinimapPoint(event); return; }
        if (panState) { updateCanvasPan(event); return; }
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
        if (document.body && document.body.classList) document.body.classList.remove("dragging-node");
        window.setTimeout(function () { suppressNextNodeClick = false; }, 0);
        if (shouldCommitLayout) commitNodeLayout();
    }

    function adoptState(response, graphSnapshot, graphUnavailable) {
        lastFullRefreshAt = Date.now();
        var targetChanged = state.targetToken !== response.target.token;
        var graphChanged = window.StarfieldGraphView.graphSnapshotChanged(state.graphSnapshot, graphSnapshot);
        var resourcesChanged=!!graphSnapshot && (!state.graphSnapshot ||
            JSON.stringify(state.graphSnapshot.layerResources)!==JSON.stringify(graphSnapshot.layerResources));
        var graphLayoutChanged = false;
        var changed = graphUnavailable || targetChanged || state.revision !== response.revision || graphChanged || resourcesChanged ||
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
        } else if (state.graphMode || graphUnavailable) {
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
        if (graphUnavailable || (state.graphMode && !state.topologyReady)) {
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

    function retainGraphOnReadFailure(targetToken) {
        if (!targetToken || targetToken !== state.targetToken || !state.graph || !state.graphSnapshot) return false;
        // Keep the confirmed canvas visible. The next edit must obtain a fresh
        // valid receipt instead of submitting against this retained snapshot.
        state.topologyReady = false;
        if (nodePalette) nodePalette.refresh();
        return true;
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
                if (retainGraphOnReadFailure(response.target && response.target.token)) return;
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
                    if (!retainGraphOnReadFailure(response.target.token)) adoptState(response, null, true);
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
