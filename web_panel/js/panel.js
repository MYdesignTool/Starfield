// Starfield Node Editor: graph and preset operations only. Node parameters are
// edited in AE Effect Controls; selecting a node focuses its native effect.

(function () {
    "use strict";

    var REQUEST_TIMEOUT_MS = 8000;
    var GATEWAY_BUILD = "native-presets-124";
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
    var SELECTION_POLL_INTERVAL_MS=200,SELECTION_SETTLE_MS=120,DIAGNOSTICS_QUIET_MS=800;
    var selectionCandidateStamp=null,selectionCandidateAt=0,lastStatusPollAt=0;
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
    var editQueue=null,queuedDraft=null,warningReceipt=null,nodeWarnings={},diagnosticEpoch=0;
    var nativeNodeIssues=[],nodeDiagnosticTooltip=null,nodeDiagnosticOwner=null,nodeDiagnosticHideTimer=null;
    var nodeDiagnosticsTimer=null;
    var canvasHydrationSerial=0,canvasHydrating=false,canvasEditIntents=[];
    var observedPanelSize = null;

    // Minimal CEP bridge. CEP injects window.__adobe_cep__ into extension panels;
    // Adobe's full CSInterface library can replace this shim later without changing
    // the protocol code below.
    function evalScript(script, callback, keepGraphFocus) {
        if (!window.__adobe_cep__ || typeof window.__adobe_cep__.evalScript !== "function") {
            callback('{"ok":false,"error":{"code":"no_host","message":"CEP host bridge is unavailable."}}');
            return;
        }
        window.__adobe_cep__.evalScript(script, callback, keepGraphFocus === true);
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
                  graphMode: false, graphSnapshot: null, topologyReady: false,
                  addingNodes: false, additionEpoch: 0 };
    var graphTransactionClient = null;
    var graphAdditionQueue = null;
    var elements = {
        banner: document.getElementById("banner"),
        chain: document.getElementById("chain"),
        workspace: document.getElementById("workspace"),
        graphCanvas: document.querySelector ? document.querySelector(".graph-canvas") : null,
        graphScroll: document.getElementById("graphScroll"),
        graphViewport: document.getElementById("graphViewport"),
        graphEmpty: document.getElementById("graphEmpty"),
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
        var inspection=operation==="getPanelState" || operation==="getPanelPulse";
        if(inspection){
            var follow=!!(extra && extra.followSelection) || !elements.autoRefresh || elements.autoRefresh.checked;
            envelope.pinTarget=!!state.pinnedTargetToken || (!follow && !!state.targetToken);
            envelope.retainTarget=!state.pinnedTargetToken && !!state.targetToken && !!state.graphSnapshot;
            envelope.projectScope=state.graphSnapshot && state.graphSnapshot.projectScope || "";
        }else if(state.targetToken){
            envelope.pinTarget=true;
        }
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
            "}catch(error){return ("+gatewayLoadFailure.toString()+")(String(error),"+quote(GATEWAY_BUILD)+");}}())";
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
        }, operation === "selectNodeEffect");
    }

    // This function is embedded in the AE script; it must work without global JSON.
    function gatewayLoadFailure(message, build) {
        function escaped(value) {
            return '"' + String(value).replace(/[\\"\x00-\x1f\u2028\u2029\ud800-\udfff]/g, function (character) {
                return "\\u" + ("0000" + character.charCodeAt(0).toString(16)).slice(-4);
            }) + '"';
        }
        return '{"protocol":"org.starfieldfx.panel","version":1,"gatewayBuild":' + escaped(build) +
            ',"ok":false,"error":{"code":"gateway_load_failed","message":' + escaped(message) + '}}';
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
        var descriptions={
            graph_transaction_busy:["SF-T101","Another edit is still active. Wait for it to finish; refresh if the warning persists."],
            graph_commit_rejected:["SF-T102","The graph change was rejected. The previous graph was kept."],
            node_effect_sync_failed:["SF-T109","AE could not synchronize this edit. Expand details to locate the failed stage."],
            graph_snapshot_unconfirmed:["SF-T103","The edit result could not be confirmed. Refresh before editing again."],
            host_timeout:["SF-T104","AE did not answer in time. Refresh to check the actual result."],
            graph_edit_outcome_unknown:["SF-T105","The edit result is unknown. Refresh before editing again."],
            native_delete_outcome_unknown:["SF-T105","The deletion result is unknown. Refresh before editing again."],
            native_graph_delete_cleanup_failed:["SF-T106","The graph was applied, but AE cleanup needs attention."],
            native_graph_delete_failed:["SF-T110","AE could not finish this graph operation. Expand details."],
            stale_graph:["SF-T107","The graph changed in AE. Refresh and try again."],
            stale_target:["SF-T108","The selected effect changed. Refresh before editing."],
            invalid_native_patch:["SF-T111","The connection change could not be submitted."],
            edit_queue_full:["SF-T112","The editor is catching up. Wait for pending edits to finish."],
            connection_capacity:["SF-G120","This edit would exceed four outgoing wires on a node. Disconnect another wire first."],
            node_capacity:["SF-G121","This graph already has 63 author nodes. Delete a node before adding another."]
        };
        var description=descriptions[code] || ["SF-P999",String(code)+": "+String(message).split(" Last graph trace:")[0]];
        elements.banner.textContent="["+description[0]+"] "+description[1];
        var details=document.createElement("details"),summary=document.createElement("summary"),text=document.createElement("div");
        summary.textContent="Error details";text.textContent=String(code)+": "+String(message);details.appendChild(summary);details.appendChild(text);elements.banner.appendChild(details);
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
        elements.targetLock.disabled = state.pending || (!locked && !state.targetToken);
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
        if (!state.layoutPersistence || !state.targetToken || !state.revision || state.pending || !state.nodes.length || canvasInteractionActive()) return;
        if (state.graphMode) {
            if(!state.topologyReady){refresh(false,false);return;}
            var positions = {};
            for (var dynamicIndex = 0; dynamicIndex < state.nodes.length; dynamicIndex++) {
                var dynamicNode = state.nodes[dynamicIndex];
                var dynamicPosition = positionFor(dynamicNode, dynamicIndex);
                positions[dynamicNode.id] = { x: Number(dynamicPosition.x), y: Number(dynamicPosition.y) };
            }
            if(!state.layoutQueue) {
                if(!window.StarfieldGraphLayoutQueue){showError("layout_module_missing","Reopen the graph panel before saving positions.",true);return;}
                state.layoutQueue=window.StarfieldGraphLayoutQueue.create({
                    setTimeout:function(fn,delay){return window.setTimeout(fn,delay);},
                    clearTimeout:function(timer){window.clearTimeout(timer);},
                    save:function(context,changes,done){
                        call("setGraphNodeLayout",{target:{token:context.token},pinTarget:true,projectScope:context.projectScope,positions:changes},function(result){
                            if(result && result.ok && (!result.target || result.target.token!==context.token)) {
                                done({ok:false,error:{code:"layout_target_mismatch",message:"The layout reply did not match its target. Refresh before continuing."}});return;
                            }
                            done(result);
                        });
                    },
                    saved:function(){state.layoutPlanningDirty=true;},
                    failed:function(error){state.topologyReady=false;showError(error.code,error.message,true);if(!canvasInteractionActive())refresh(false,false);}
                });
                var savedGraph=state.graphSnapshot.canvasOnly?null:window.StarfieldGraphCodec.fromHex(state.graphSnapshot.graphHex);
                state.layoutQueue.reset({token:state.targetToken,projectScope:state.graphSnapshot.projectScope || ""},
                    savedGraph?window.StarfieldGraphView.project(savedGraph,null,state.graphSnapshot.geometry,state.graphSnapshot.layerResources).positions:
                    window.StarfieldNativeGraphSnapshot.canvasGraph(state.graphSnapshot).positions);
            }
            refreshEpoch++; // A read started before this local drag cannot repaint it.
            state.layoutQueue.enqueue(positions);
            // A rejected/fully compensated write must also project the next
            // host snapshot, even when its hex equals our older cached graph.
            if(state.layoutQueue.hasWork())state.layoutPlanningDirty=true;
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
        if(state.graphSnapshot && state.graphSnapshot.canvasOnly)return;
        if (frameStatusInFlight || panelPulseInFlight || !resolvedTarget || !state.targetToken || state.pending ||
            refreshInFlight || !state.nodes.length) return;
        frameStatusInFlight = true;
        call("getFrameStatus", null, function (response) {
            frameStatusInFlight = false;
            adoptFrameStatus(response);
        });
    }

    function adoptFrameStatus(response) {
            if(state.graphSnapshot && state.graphSnapshot.canvasOnly)return;
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
        if(state.graphMode && (!state.topologyReady || !state.graph || !state.graph.nodes.some(function(node){return node.id===selection.id;})))return;
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
        button.title = "Click " + node.label + " to edit in AE Effect Controls";
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
            if (event.detail > 1) return;
            queuedEffectSelection = null;
            if (event.shiftKey || event.ctrlKey) {
                if (state.selectedNodeIds[node.id]) delete state.selectedNodeIds[node.id];
                else state.selectedNodeIds[node.id] = true;
            } else {
                state.selectedNodeIds = {};
                state.selectedNodeIds[node.id] = true;
            }
            state.selectedNodeId = node.id;
            syncNodeSelectionStyles();
            // A double-click already includes its first click. Open once;
            // modifier selection and drag suppression remain graph-local.
            editNativeNode(event);
        });
        function editNativeNode(event) {
            if (suppressNextNodeClick || state.pending || canvasInteractionActive() ||
                event.shiftKey || event.ctrlKey || event.metaKey || event.altKey || event.repeat) return;
            event.preventDefault();
            state.selectedNodeIds = {};
            state.selectedNodeIds[node.id] = true;
            state.selectedNodeId = node.id;
            syncNodeSelectionStyles();
            selectNativeEffect(node.id);
        }
        button.addEventListener("keydown", function (event) {
            if (event.key === "Enter") editNativeNode(event);
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
                if (event.button !== 0 || event.isPrimary === false || (state.pending && !state.queuedEditing)) return;
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
        if(state.graphMode && !state.topologyReady)return;
        if (!state.targetToken || !state.revision || (state.pending && !state.queuedEditing && !(state.addingNodes && edit.type==="addNode"))) return;
        if(state.graphSnapshot && state.graphSnapshot.canvasOnly){hydrateCanvasEdit(edit);return;}
        applyTopologyEdit(edit);
    }
    function hydrateCanvasEdit(edit){
        if(canvasEditIntents.length>=64){showError("edit_queue_full","Wait for pending edits to finish.");return;}
        canvasEditIntents.push(JSON.parse(JSON.stringify(edit)));
        if(!retainedError){elements.banner.className="banner";elements.banner.textContent="Preparing "+canvasEditIntents.length+" edit"+(canvasEditIntents.length===1?"":"s")+"…";}
        if(canvasHydrating)return;
        canvasHydrating=true;refreshInFlight=true;cancelNodeDiagnostics();
        var serial=++canvasHydrationSerial,target=state.targetToken,scope=state.graphSnapshot.projectScope || "",epoch=state.additionEpoch;
        var origins=JSON.parse(JSON.stringify(nodePositions));
        function current(){return serial===canvasHydrationSerial && state.targetToken===target && state.additionEpoch===epoch;}
        function fail(error){
            if(!current())return;canvasHydrating=false;refreshInFlight=false;canvasEditIntents=[];
            showError(error.code || "graph_snapshot_unconfirmed",error.message || "Complete controls could not be read. No structural edit was sent.",true);
        }
        function accept(result){
            if(!current())return;
            if(!result || !result.ok || !result.target || result.target.token!==target || !result.snapshot || !result.snapshot.initialized || result.snapshot.canvasOnly ||
                (result.snapshot.projectScope || "")!==scope){fail(result && result.error || {code:"stale_target",message:"The editing target changed. Refresh before editing."});return;}
            if(canvasInteractionActive()){window.setTimeout(function(){accept(result);},50);return;}
            var changed={};Object.keys(nodePositions).forEach(function(id){
                if(origins[id] && (origins[id].x!==nodePositions[id].x || origins[id].y!==nodePositions[id].y))changed[id]={x:nodePositions[id].x,y:nodePositions[id].y};
            });
            var intents=canvasEditIntents;canvasEditIntents=[];canvasHydrating=false;refreshInFlight=false;
            var envelope={target:{token:target,comp:state.canvasTarget && state.canvasTarget.comp || "",layer:state.canvasTarget && state.canvasTarget.layer || ""},
                controlSource:"Node Graph",revision:"graph-"+result.snapshot.revision+"-"+result.snapshot.recordStamp,resolution:"native",layoutPersistence:true};
            adoptState(envelope,result.snapshot);
            Object.keys(changed).forEach(function(id){if(nodePositions[id])nodePositions[id]=changed[id];});
            if(Object.keys(changed).length){state.layoutPlanningDirty=true;render(state);}
            intents.forEach(requestTopologyEdit);
        }
        function read(error){if(error){fail(error);return;}if(current())loadGraphSnapshot(target,accept);}
        if(state.layoutQueue && state.layoutQueue.hasWork())state.layoutQueue.flush(read);else read(null);
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
            call("syncGraphSnapshot", { target: { token: targetToken },
                syncReason:response.snapshot.repairReason || "Snapshot was not initialized." }, function (initialized) {
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
        state.layoutPlanningDirty=false;
        if(state.layoutQueue)state.layoutQueue.reset({token:state.targetToken,projectScope:snapshot.projectScope || ""},nodePositions);
        if(snapshot.panelStamp)lastPulseStamp=snapshot.panelStamp;
        lastFullRefreshAt=Date.now();
        topologyPreview=null;render(state);
        requestNodeDiagnostics(snapshot);
    }
    function hideNodeDiagnosticTooltip() {
        if(nodeDiagnosticHideTimer!==null)window.clearTimeout(nodeDiagnosticHideTimer);
        nodeDiagnosticHideTimer=null;nodeDiagnosticOwner=null;
        if(nodeDiagnosticTooltip && nodeDiagnosticTooltip.parentNode)nodeDiagnosticTooltip.parentNode.removeChild(nodeDiagnosticTooltip);
        nodeDiagnosticTooltip=null;
    }
    function deferHideNodeDiagnosticTooltip() {
        if(nodeDiagnosticHideTimer!==null)window.clearTimeout(nodeDiagnosticHideTimer);
        nodeDiagnosticHideTimer=window.setTimeout(hideNodeDiagnosticTooltip,150);
    }
    function showNodeDiagnosticTooltip(marker,messages) {
        hideNodeDiagnosticTooltip();
        var popup=document.createElement("div");popup.className="node-diagnostic-tooltip";
        popup.id="node-diagnostic-tooltip";popup.setAttribute("role","tooltip");
        messages.forEach(function(issue){
            var item=document.createElement("div"),heading=document.createElement("strong"),detail=document.createElement("div");
            item.className="node-diagnostic-detail "+issue.severity;
            heading.textContent=(issue.severity==="error"?"Error":"Warning")+" · "+issue.code;
            detail.textContent=issue.message;item.appendChild(heading);item.appendChild(detail);popup.appendChild(item);
        });
        popup.style.maxWidth=Math.max(0,window.innerWidth-16)+"px";
        document.body.appendChild(popup);nodeDiagnosticTooltip=popup;
        nodeDiagnosticOwner={id:marker.getAttribute("data-node-id"),signature:JSON.stringify(messages)};
        popup.addEventListener("mouseenter",function(){
            if(nodeDiagnosticHideTimer!==null)window.clearTimeout(nodeDiagnosticHideTimer);nodeDiagnosticHideTimer=null;
        });
        popup.addEventListener("mouseleave",deferHideNodeDiagnosticTooltip);
        var anchor=marker.getBoundingClientRect(),box=popup.getBoundingClientRect();
        var left=anchor.right+10;if(left+box.width>window.innerWidth-8)left=anchor.left-box.width-10;
        popup.style.left=Math.max(8,Math.min(left,window.innerWidth-box.width-8))+"px";
        popup.style.top=Math.max(8,Math.min(anchor.top,window.innerHeight-box.height-8))+"px";
    }
    function updateNodeWarnings() {
        var owner=nodeDiagnosticHideTimer===null?nodeDiagnosticOwner:null;
        hideNodeDiagnosticTooltip();
        nodeWarnings=window.StarfieldGraphView.nodeDiagnostics(state.graph,nativeNodeIssues);
        Object.keys(graphNodeElements).forEach(function(id){
            var card=graphNodeElements[id].card,old=card.querySelector(".node-warning");if(old)old.remove();
            card.classList.remove("has-node-error");
            var messages=nodeWarnings[id];if(!messages || !messages.length)return;
            var isError=messages.some(function(issue){return issue.severity==="error";});
            if(isError)card.classList.add("has-node-error");
            var warning=document.createElement("span");warning.className="node-warning "+(isError?"error":"warning");warning.textContent="!";
            warning.setAttribute("tabindex","0");warning.setAttribute("role","img");
            warning.setAttribute("data-node-id",id);
            warning.setAttribute("aria-label",messages.map(function(issue){return (issue.severity==="error"?"Error":"Warning")+" ["+issue.code+"]: "+issue.message;}).join("\n"));
            warning.setAttribute("aria-describedby","node-diagnostic-tooltip");
            warning.addEventListener("mouseenter",function(){showNodeDiagnosticTooltip(warning,messages);});
            warning.addEventListener("focus",function(){showNodeDiagnosticTooltip(warning,messages);});
            warning.addEventListener("mouseleave",deferHideNodeDiagnosticTooltip);
            warning.addEventListener("blur",hideNodeDiagnosticTooltip);
            warning.addEventListener("keydown",function(event){if(event.key==="Escape")hideNodeDiagnosticTooltip();});
            card.appendChild(warning);
            if(owner && owner.id===id && owner.signature===JSON.stringify(messages))showNodeDiagnosticTooltip(warning,messages);
        });
    }
    function requestNodeDiagnostics(snapshot) {
        cancelNodeDiagnostics();
        if(!snapshot || !window.StarfieldNativeGraph || !window.StarfieldNativeGraph.diagnostics)return;
        var target=state.targetToken,scope=snapshot.projectScope || "",revision=snapshot.revision,crc=snapshot.checksum;
        var receipt=JSON.stringify([target,scope,revision,crc,state.additionEpoch]);
        if(warningReceipt===receipt)return;warningReceipt=receipt;nativeNodeIssues=[];updateNodeWarnings();
        var epoch=++diagnosticEpoch;
        window.StarfieldNativeGraph.diagnostics(target,revision,crc,scope,function(result){
            var now=state.graphSnapshot;
            if(epoch!==diagnosticEpoch || state.targetToken!==target || !now || now.projectScope!==scope || now.revision!==revision || now.checksum!==crc ||
                receipt!==JSON.stringify([state.targetToken,now.projectScope || "",now.revision,now.checksum,state.additionEpoch]))return;
            if(!result || !result.ok || result.revision!==revision || result.checksum!==parseInt(crc,16)){
                warningReceipt=null;state.diagnosticsUnavailable=true;
                if(elements.resolutionLine){elements.resolutionLine.textContent=String(elements.resolutionLine.textContent || "").replace(/ · Warnings unavailable$/,"")+" · Warnings unavailable";elements.resolutionLine.title="Refresh to read node warning details.";}return;
            }
            state.diagnosticsUnavailable=false;
            if(elements.resolutionLine){elements.resolutionLine.textContent=String(elements.resolutionLine.textContent || "").replace(/ · Warnings unavailable$/,"");elements.resolutionLine.title="";}
            nativeNodeIssues=result.issues || [];updateNodeWarnings();
        });
    }
    function cancelNodeDiagnostics(){
        if(nodeDiagnosticsTimer!==null)window.clearTimeout(nodeDiagnosticsTimer);nodeDiagnosticsTimer=null;
    }
    function scheduleNodeDiagnostics(snapshot){
        cancelNodeDiagnostics();
        if(!snapshot || !window.StarfieldNativeGraph || !window.StarfieldNativeGraph.diagnostics)return;
        var target=state.targetToken,epoch=state.additionEpoch;
        function attempt(){
            nodeDiagnosticsTimer=null;
            var now=state.graphSnapshot;
            if(target!==state.targetToken || epoch!==state.additionEpoch ||
                !now || now.revision!==snapshot.revision || now.checksum!==snapshot.checksum || now.projectScope!==snapshot.projectScope)return;
            if(state.pending || refreshInFlight || panelPulseInFlight || canvasInteractionActive() || document.hidden){
                nodeDiagnosticsTimer=window.setTimeout(attempt,document.hidden?IDLE_POLL_INTERVAL_MS:SELECTION_POLL_INTERVAL_MS);return;
            }
            requestNodeDiagnostics(snapshot);
        }
        nodeDiagnosticsTimer=window.setTimeout(attempt,DIAGNOSTICS_QUIET_MS);
    }
    function renderQueuedDraft(graph,snapshot,status) {
        if(status.targetToken!==state.targetToken || status.epoch!==state.additionEpoch)return;
        state.pending=status.pending;state.queuedEditing=status.pending;state.graphSnapshot=snapshot;
        if(canvasInteractionActive()){queuedDraft={graph:graph,snapshot:snapshot,status:status};return;}
        var positions=window.StarfieldGraphLayout.resolve(graph);
        Object.keys(positions).forEach(function(id){if(nodePositions[id])positions[id]=nodePositions[id];});
        graph=window.StarfieldGraphLayout.set(graph,positions);
        var same=state.graph && window.StarfieldGraphCodec.toHex(state.graph)===window.StarfieldGraphCodec.toHex(graph);
        state.graph=graph;state.graphMode=true;state.topologyReady=true;state.layoutPersistence=true;
        if(!same){var view=window.StarfieldGraphView.project(graph,null,snapshot.geometry,snapshot.layerResources);state.nodes=view.nodes;state.edges=view.edges;nodePositions=view.positions;render(state);}
        if(nodePalette)nodePalette.refresh();syncTargetLock();
        if(status.pending && !retainedError){elements.banner.className="banner";elements.banner.textContent=status.count+" edit"+(status.count===1?"":"s")+" pending…";}
        if(!status.pending){
            var savedPositions=window.StarfieldGraphLayout.resolve(window.StarfieldGraphCodec.fromHex(snapshot.graphHex));
            if(state.layoutQueue && !state.layoutQueue.adopt({token:state.targetToken,projectScope:snapshot.projectScope || ""},savedPositions)){
                state.topologyReady=false;showError("layout_receipt_overlap","A layout write is still active. Refresh after it finishes before editing again.",true);return;
            }
            lastFullRefreshAt=Date.now();requestNodeDiagnostics(snapshot);
        }
        updateNodeWarnings();
    }
    function queueTopologyEdit(edit) {
        if(edit.type==="duplicateNodes") {
            edit=Object.assign({},edit,{origins:{}});
            edit.nodeIds.forEach(function(id){if(nodePositions[id])edit.origins[id]={x:Number(nodePositions[id].x),y:Number(nodePositions[id].y)};});
        }
        if(!editQueue)editQueue=window.StarfieldEditQueue.create({codec:window.StarfieldGraphCodec,edits:window.StarfieldGraphEdits,schedule:afterTopologyPaint,
            isCurrent:function(token,scope,epoch){return state.targetToken===token && state.graphSnapshot && (state.graphSnapshot.projectScope || "")===scope && state.additionEpoch===epoch;},
            onView:renderQueuedDraft,onSettled:function(){state.pending=false;state.queuedEditing=false;if(state.topologyReady){clearBanner(true);commitNodeLayout();}syncTargetLock();},
            onError:function(result,info){if(info.targetToken!==state.targetToken)return;queuedDraft=null;queuedEffectSelection=null;state.pending=false;state.queuedEditing=false;state.topologyReady=false;state.additionEpoch++;diagnosticEpoch++;
                var error=result && result.error || {code:"graph_edit_outcome_unknown",message:"The edit could not be confirmed."};
                showError(error.code,error.message+" Pending edits were stopped. Refresh before continuing.",true);syncTargetLock();},
            submit:function(base,graph,done,items){
                var previous=window.StarfieldGraphCodec.fromHex(base.snapshot.graphHex),delta=window.StarfieldEditQueue.delta(base.snapshot,graph,previous);
                if(delta && window.StarfieldNativeGraph && window.StarfieldNativeGraph.patch) {
                    if(!delta.rows.length && !delta.removed.length){done({ok:true,target:base.target,snapshot:base.snapshot,noOp:true});return;}
                    window.StarfieldNativeGraph.patch(base.target.token,base.snapshot.revision,base.snapshot.checksum,delta,function(result){
                        if(!result || !result.ok){done(result);return;}
                        call("getGraphSnapshot",{target:base.target},function(saved){
                            if(!saved || !saved.ok || !saved.snapshot || !saved.snapshot.initialized || saved.snapshot.revision!==result.revision){done({ok:false,error:{code:"graph_snapshot_unconfirmed",message:"The edit was applied, but its saved author records could not be confirmed."}});return;}
                            var actual=window.StarfieldGraphCodec.fromHex(saved.snapshot.graphHex);
                            var expectedNodes=graph.nodes.map(function(n){return n.id;}).sort(),actualNodes=actual.nodes.map(function(n){return n.id;}).sort();
                            function edges(g){return g.edges.map(function(e){return [e.id,e.sourceNode,e.sourcePort,e.destinationNode,e.destinationPort].join(":");}).sort();}
                            if(JSON.stringify(expectedNodes)!==JSON.stringify(actualNodes) || JSON.stringify(edges(graph))!==JSON.stringify(edges(actual))){done({ok:false,error:{code:"graph_snapshot_unconfirmed",message:"Saved connections differ from this edit. Refresh before continuing."}});return;}
                            done(saved);
                        });
                    },base.snapshot.projectScope);return;
                }
                var copies=[];items.forEach(function(item){copies=copies.concat(item.modelResourceCopies || []);});
                var client=window.StarfieldGraphTransactions.create({call:call,codec:window.StarfieldGraphCodec,edits:window.StarfieldGraphEdits,loadModelModules:loadModelModules});
                client.apply({type:"compiledBatch",graph:graph,modelResourceCopies:copies},done,base.target.token,base.snapshot.revision,base);
            }});
        refreshEpoch++;refreshInFlight=false;
        var result=editQueue.enqueue(edit,{ok:true,target:{token:state.targetToken},snapshot:state.graphSnapshot},state.additionEpoch);
        if(!result.ok)showError(result.error.code,result.error.message,true);
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

    function renderAdditionDraft(graph, snapshot, status) {
        if(status.targetToken!==state.targetToken)return;
        var same=state.graph && window.StarfieldGraphCodec.toHex(state.graph)===window.StarfieldGraphCodec.toHex(graph);
        state.pending=status.pending;state.addingNodes=status.pending;
        state.graphSnapshot=snapshot;state.graph=graph;
        state.graphMode=true;state.topologyReady=true;state.layoutPersistence=true;
        if(!same) {
            var view=window.StarfieldGraphView.project(graph,null,snapshot.geometry,snapshot.layerResources);
            state.nodes=view.nodes;state.edges=view.edges;nodePositions=view.positions;render(state);
        } else if(nodePalette)nodePalette.refresh();
        if(!status.pending) {
            state.layoutPlanningDirty=false;
            if(snapshot.panelStamp)lastPulseStamp=snapshot.panelStamp;
            lastFullRefreshAt=Date.now();
            if(state.layoutQueue)state.layoutQueue.reset({token:state.targetToken,projectScope:snapshot.projectScope || ""},nodePositions);
        }
        syncTargetLock();
        if(status.pending && !retainedError) {
            elements.banner.className="banner";
            elements.banner.textContent="Adding "+status.count+" node"+(status.count===1?"":"s")+"…";
        }
    }

    function queueTopologyAddition(edit) {
        if(!graphAdditionQueue)graphAdditionQueue=window.StarfieldGraphTransactions.createAdditionQueue({
            codec:window.StarfieldGraphCodec,edits:window.StarfieldGraphEdits,call:call,
            loadModelModules:loadModelModules,schedule:afterTopologyPaint,
            isCurrentTarget:function(token){return state.targetToken===token;},onView:renderAdditionDraft,
            onSettled:function(){clearBanner(true);syncTargetLock();},
            onError:function(result,info){
                if(info.targetToken!==state.targetToken)return;
                state.pending=false;state.addingNodes=false;state.topologyReady=false;state.additionEpoch++;
                if(nodePalette)nodePalette.cancel();syncTargetLock();
                var error=result && result.error || {code:"graph_edit_failed",message:"The additions could not be confirmed."};
                showError(error.code,error.message,true);refresh(false,false);
            }
        });
        refreshEpoch++;refreshInFlight=false;
        var result=graphAdditionQueue.enqueue(edit,{ok:true,target:{token:state.targetToken},snapshot:state.graphSnapshot});
        if(!result.ok)showError(result.error.code,result.error.message,true);
    }

    function applyTopologyEdit(edit) {
        if(state.graphSnapshot && state.graphSnapshot.canvasOnly){requestTopologyEdit(edit);return;}
        // Flush cosmetic writes before a structural operation. Only manifest-
        // based edits need a fresh authoring stamp after layout/time changes;
        // native126 deletion consumes the unchanged saved graph revision/CRC.
        var nativeDelete=edit.type==="deleteNodes" && window.StarfieldNativeGraph;
        if(!state.pending && ((state.layoutQueue && state.layoutQueue.hasWork()) || (state.layoutPlanningDirty && !nativeDelete))) {
            state.pending=true;refreshEpoch++;refreshInFlight=false;
            var layoutTarget=state.targetToken,layoutEpoch=refreshEpoch;
            function resumeLayoutEdit(error) {
                if(state.targetToken!==layoutTarget || refreshEpoch!==layoutEpoch)return;
                if(error){state.pending=false;showError(error.code,error.message,true);return;}
                if(nativeDelete){state.pending=false;applyTopologyEdit(edit);return;}
                call("getGraphSnapshot",{target:{token:layoutTarget},pinTarget:true},function(result){
                    if(state.targetToken!==layoutTarget || refreshEpoch!==layoutEpoch)return;
                    state.pending=false;
                    if(!result || !result.ok || !result.snapshot || !result.snapshot.initialized) {
                        state.topologyReady=false;
                        var problem=result && result.error || {code:"graph_snapshot_unconfirmed",message:"Refresh the graph before continuing."};
                        showError(problem.code,problem.message,true);return;
                    }
                    try{confirmTopologySnapshot(result.snapshot);}catch(invalid){state.topologyReady=false;showError("graph_snapshot_unconfirmed",String(invalid),true);return;}
                    applyTopologyEdit(edit);
                });
            }
            if(state.layoutQueue)state.layoutQueue.flush(resumeLayoutEdit);else resumeLayoutEdit(null);
            return;
        }
        if(state.graphMode && state.topologyReady && window.StarfieldEditQueue && (!state.pending || state.queuedEditing)) {
            queueTopologyEdit(edit);return;
        }
        if(edit.type==="addNode" && state.graphMode && state.topologyReady &&
            window.StarfieldGraphTransactions && window.StarfieldGraphTransactions.createAdditionQueue) {
            if(!state.pending || state.addingNodes)queueTopologyAddition(edit);
            return;
        }
        if(state.pending)return;
        var client = getGraphTransactionClient();
        if (!client || !state.targetToken) {
            showError("graph_edit_transport_unavailable", "The graph transaction bridge did not load.");
            return;
        }
        var targetToken = state.targetToken;
        refreshEpoch += 1;
        refreshInFlight = false;
        state.pending = true;
        if (edit.type === "deleteNodes" && window.StarfieldNativeGraph && state.graphSnapshot && state.graphSnapshot.initialized) {
            var deleteEpoch=refreshEpoch,deleteBase=state.graphSnapshot;
            elements.banner.className=retainedError?"banner error":"banner";
            elements.banner.textContent="Deleting selected nodes…"+(retainedError?" Previous error: "+retainedError.code+": "+retainedError.message:"");
            if(nodePalette)nodePalette.refresh();
            afterTopologyPaint(function(){
            if(state.targetToken!==targetToken || refreshEpoch!==deleteEpoch)return;
            window.StarfieldNativeGraph.deleteNodes(targetToken,deleteBase.revision,deleteBase.checksum,edit.nodeIds,function(result){
                if(state.targetToken!==targetToken || refreshEpoch!==deleteEpoch)return;
                function failed(code,message){state.pending=false;if(nodePalette)nodePalette.refresh();showError(code,message,true);}
                if(!result || !result.ok){
                    var nativeError=result && result.error || {code:"native_delete_outcome_unknown",message:"No native deletion result. Refresh before continuing."};
                    failed(nativeError.code,nativeError.message+(result && result.diagnostics?" "+JSON.stringify(result.diagnostics):""));return;
                }
                // One read-only snapshot for painting, after the native receipt.
                // Never enter initialization/reconciliation as a hidden retry.
                call("getGraphSnapshot",{target:{token:targetToken}},function(saved){
                    if(state.targetToken!==targetToken || refreshEpoch!==deleteEpoch)return;
                    if(!saved || !saved.ok || !saved.snapshot || !saved.snapshot.initialized || saved.snapshot.revision!==result.revision){
                        failed("graph_snapshot_unconfirmed","Deletion was committed, but its current graph snapshot could not be confirmed. Refresh before continuing.");return;
                    }
                    try{confirmTopologySnapshot(saved.snapshot);}catch(error){failed("graph_snapshot_unconfirmed",String(error));return;}
                    state.pending=false;if(nodePalette)nodePalette.refresh();clearBanner(true);
                });
            },deleteBase.projectScope);
            });
            return;
        }
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
        queuedEffectSelection = null;
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
        queuedEffectSelection = null;
        var ids = editableSelectedNodeIds();
        if (!ids.length || (state.pending && !state.queuedEditing) || canvasInteractionActive()) return;
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
        if(elements.graphEmpty)elements.graphEmpty.hidden=state.nodes.length>0;
        if(elements.graphMinimap)elements.graphMinimap.hidden=state.nodes.length===0;
        if (!state.nodes.length) {
            hideNodeDiagnosticTooltip();
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
        updateNodeWarnings();
    }

    function beginNodeDrag(nodeId, event) {
        if (event.button !== 0 || event.isPrimary === false || (state.pending && !state.queuedEditing)) return;
        if(state.graphMode && !state.topologyReady)return;
        queuedEffectSelection = null;
        if (event.shiftKey || event.ctrlKey) return;
        if(state.layoutQueue)state.layoutQueue.defer();
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
        if(!event.altKey && origins.length===1 && window.StarfieldNodeShake){dragState.shake=window.StarfieldNodeShake.create();dragState.shake.reset(event.clientX,event.clientY,Date.now());}
    }

    function beginPortDrag(nodeId, direction, event) {
        if (event.button !== 0 || event.isPrimary === false || (state.pending && !state.queuedEditing)) return;
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
            if (!Object.prototype.hasOwnProperty.call(graphNodeElements, nodeId)) continue;
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
        queuedEffectSelection = null;
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

    var dragPaintPending=false;
    function flushDragPaint() {
        if(!dragPaintPending)return;
        dragPaintPending=false;updateCanvasBounds();renderEdges();
    }
    function scheduleDragPaint() {
        if(dragPaintPending)return;
        dragPaintPending=true;
        if(window.requestAnimationFrame)window.requestAnimationFrame(flushDragPaint);
        else window.setTimeout(flushDragPaint,16);
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
        if(dragState.shake)dragState.shake.sample(event.clientX,event.clientY,Date.now());
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
        scheduleDragPaint();
    }

    function endNodeDrag(event) {
        flushDragPaint(); // Drop hit-testing must see the final local position.
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
                    if (otherNodeId) {
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
            var shaken=draggedNode && dragState.shake && dragState.shake.triggered();
            var edge = !shaken && draggedNode ? edgeUnderNode(draggedNode.id) : null;
            if(shaken)requestTopologyEdit({type:"detachNodes",nodeIds:[draggedNode.id]});
            else if (draggedNode && edge) {
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
        var finishedQueuedDraft=false;
        if(queuedDraft && !canvasInteractionActive()){var view=queuedDraft;queuedDraft=null;renderQueuedDraft(view.graph,view.snapshot,view.status);finishedQueuedDraft=!view.status.pending;}
        if (document.body && document.body.classList) document.body.classList.remove("dragging-node");
        window.setTimeout(function () { suppressNextNodeClick = false; }, 0);
        if (shouldCommitLayout || finishedQueuedDraft) commitNodeLayout();
        if(state.layoutQueue)state.layoutQueue.resume();
    }

    function adoptState(response, graphSnapshot, graphUnavailable) {
        lastFullRefreshAt = Date.now();lastStatusPollAt=lastFullRefreshAt;
        var targetChanged = state.targetToken !== response.target.token || !!(state.graphSnapshot && graphSnapshot &&
            (state.graphSnapshot.projectScope || "")!==(graphSnapshot.projectScope || ""));
        var graphChanged = window.StarfieldGraphView.graphSnapshotChanged(state.graphSnapshot, graphSnapshot);
        if(targetChanged){canvasHydrationSerial++;canvasHydrating=false;canvasEditIntents=[];hideNodeDiagnosticTooltip();diagnosticEpoch++;warningReceipt=null;nativeNodeIssues=[];nodeWarnings={};}
        if(targetChanged && editQueue){editQueue.cancel();queuedDraft=null;state.pending=false;state.queuedEditing=false;}
        if(targetChanged && graphAdditionQueue) {graphAdditionQueue.cancel();state.pending=false;state.addingNodes=false;}
        var resourcesChanged=!!graphSnapshot && (!state.graphSnapshot ||
            JSON.stringify(state.graphSnapshot.layerResources)!==JSON.stringify(graphSnapshot.layerResources));
        if(targetChanged || graphChanged || resourcesChanged || (graphSnapshot && state.graphSnapshot &&
            JSON.stringify(graphSnapshot.geometry)!==JSON.stringify(state.graphSnapshot.geometry)))
            state.additionEpoch=(state.additionEpoch || 0)+1;
        var graphLayoutChanged = false;
        var changed = graphUnavailable || targetChanged || state.revision !== response.revision || graphChanged || resourcesChanged ||
                      state.controlSource !== response.controlSource;
        if (targetChanged) resetViewForTarget();
        var layoutChanged = false;
        state.controlSource = response.controlSource;
        state.graphMode = response.controlSource === "Node Graph";
        state.topologyReady = !!(graphSnapshot && graphSnapshot.initialized);
        state.canvasTarget=response.target;
        if(state.graphMode && state.topologyReady && graphSnapshot.canvasOnly){
            try{
                var canvas=window.StarfieldNativeGraphSnapshot.canvasGraph(graphSnapshot);
                var canvasView=window.StarfieldGraphView.project(canvas.graph,canvas.positions,graphSnapshot.geometry,[]);
                graphLayoutChanged=!window.StarfieldGraphView.samePositions(nodePositions,canvasView.positions);
                state.graph=canvas.graph;state.nodes=canvasView.nodes;state.edges=canvasView.edges;nodePositions=canvasView.positions;
                state.graphSnapshot=graphSnapshot;state.layoutPersistence=true;state.frameStatus={available:false};state.liveParticleCount=null;
            }catch(canvasError){state.topologyReady=false;showError(canvasError.code || "invalid_canvas_snapshot",canvasError.message || String(canvasError));return;}
        }else if (state.graphMode && state.topologyReady) {
            try {
                var reusable = !state.layoutPlanningDirty && !targetChanged && state.graph && state.graphSnapshot &&
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
        if(graphSnapshot && typeof graphSnapshot.panelStamp==="string")lastPulseStamp=graphSnapshot.panelStamp;
        state.layoutPlanningDirty=false;
        if(graphSnapshot && graphSnapshot.panelStamp)lastPulseStamp=graphSnapshot.panelStamp;
        if(state.layoutQueue && !state.layoutQueue.hasWork())state.layoutQueue.reset(
            state.graphMode?{token:state.targetToken,projectScope:graphSnapshot && graphSnapshot.projectScope || ""}:null,nodePositions);
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
        if(state.graphMode && state.topologyReady)scheduleNodeDiagnostics(graphSnapshot);
        if (nodePalette) nodePalette.refresh();
        if(response.frameStatus && response.frameStatus.ok)adoptFrameStatus(response.frameStatus);
        else if (latestFrameStatusResponse && latestFrameStatusResponse.targetToken === state.targetToken) adoptFrameStatus(latestFrameStatusResponse);
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

    function clearAbsentTarget() {
        canvasHydrationSerial++;canvasHydrating=false;canvasEditIntents=[];
        cancelNodeDiagnostics();
        if(editQueue)editQueue.cancel();
        if(graphAdditionQueue)graphAdditionQueue.cancel();
        if(state.layoutQueue)state.layoutQueue.reset(null,{});
        diagnosticEpoch++;state.additionEpoch=(state.additionEpoch || 0)+1;
        warningReceipt=null;nativeNodeIssues=[];nodeWarnings={};queuedDraft=null;
        state.targetToken=null;state.revision=null;state.graph=null;state.graphSnapshot=null;
        state.nodes=[];state.edges=[];state.topologyReady=false;state.frameStatus=null;state.liveParticleCount=null;
        state.layoutPersistence=false;latestFrameStatusResponse=null;
        nodePositions={};resolvedTarget=false;lastPulseStamp=null;
        elements.targetLine.textContent="No target";elements.revisionLine.textContent="";elements.resolutionLine.textContent="";
        clearBanner();render(state);syncTargetLock();if(nodePalette)nodePalette.refresh();
    }

    function refresh(autoRetry, resetRetryBudget, forceSelection) {
        if (refreshInFlight || state.pending || (state.layoutQueue && state.layoutQueue.hasWork())) return;
        if(!forceSelection && elements.autoRefresh && !elements.autoRefresh.checked && !state.targetToken && !state.pinnedTargetToken)return;
        if (startupRetryTimer !== null) {
            window.clearTimeout(startupRetryTimer);
            startupRetryTimer = null;
        }
        if (resetRetryBudget) startupRetryAttempt = 0;
        var epoch = ++refreshEpoch;
        refreshInFlight = true;
        function pausedSwitch(response,snapshot){
            snapshot=snapshot || response && response.snapshot;
            return !forceSelection && elements.autoRefresh && !elements.autoRefresh.checked && response && response.ok && response.target &&
                (response.target.token!==state.targetToken || !!(state.graphSnapshot && snapshot &&
                (state.graphSnapshot.projectScope || "")!==(snapshot.projectScope || "")));
        }
        call("getPanelState", {followSelection:!!forceSelection,canvasOnly:true}, function (response) {
            if (epoch !== refreshEpoch) { refreshInFlight = false; lastPulseStamp = null; return; }
            if(pausedSwitch(response)){refreshInFlight=false;lastPulseStamp=null;return;}
            if (canvasInteractionActive() || state.pending || (state.layoutQueue && state.layoutQueue.hasWork())) { refreshInFlight = false; lastPulseStamp = null; return; }
            if (!response.ok) {
                refreshInFlight = false;
                var error = response.error || { code: "unknown", message: "Unknown failure." };
                if(error.code==="no_target" || error.code==="no_project" || error.code==="no_active_comp" || error.code==="no_effect" ||
                    (error.code==="stale_target" && !state.pinnedTargetToken)){
                    startupRetryAttempt=0;clearAbsentTarget();return;
                }
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
            var initializingGraph = !!(response.snapshot && !response.snapshot.initialized);
            loadGraphSnapshot(response.target.token, function (snapshotResponse) {
                refreshInFlight = false;
                if (epoch !== refreshEpoch || pausedSwitch(response,snapshotResponse && snapshotResponse.snapshot) || canvasInteractionActive() || state.pending || (state.layoutQueue && state.layoutQueue.hasWork())) { lastPulseStamp = null; return; }
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
                if (initializingGraph) {
                    // The confirmed native publication also switches Control Source.
                    // Discard the pre-sync legacy view and frame data on this paint.
                    response = {target:response.target,controlSource:"Node Graph",resolution:"native",
                        layoutPersistence:true,revision:"graph-"+snapshotResponse.snapshot.revision+"-"+snapshotResponse.snapshot.recordStamp};
                }
                clearBanner();
                adoptState(response, snapshotResponse.snapshot);
            }, response);
        });
    }

    function schedulePanelPoll(delay) {
        if (panelPollTimer !== null) window.clearTimeout(panelPollTimer);
        panelPollTimer = window.setTimeout(pollPanelState,delay);
    }

    // The existing native126 history command returns after AE dispatch. Flush
    // cosmetic metadata first so Ctrl+Z addresses the move the user just made.
    window.StarfieldGraphHistory=function(perform) {
        if(state.pending || canvasInteractionActive())return;
        state.pending=true;refreshEpoch++;refreshInFlight=false;
        var historyTarget=state.targetToken,epoch=refreshEpoch;
        function ready(error) {
            if(epoch!==refreshEpoch || historyTarget!==state.targetToken)return;
            if(error){state.pending=false;showError(error.code,error.message,true);return;}
            var settled=false,timer=window.setTimeout(function(){
                finish({ok:false,error:{code:"history_outcome_unknown",message:"History did not return in time. Refresh before continuing."}});
            },REQUEST_TIMEOUT_MS);
            function finish(result) {
                if(settled)return;settled=true;window.clearTimeout(timer);
                if(epoch!==refreshEpoch || historyTarget!==state.targetToken)return;
                state.pending=false;
                if(!result || !result.ok){
                    var problem=result && result.error || {code:"history_outcome_unknown",message:"History could not be confirmed."};
                    state.topologyReady=false;showError(problem.code,problem.message,true);
                }
                // Layout-only Undo has no graph revision change. This explicit
                // read also adopts external author/target changes after history.
                refresh(false,false);
            }
            try{perform(finish);}catch(failure){finish({ok:false,error:{code:"history_failed",message:String(failure)}});}
        }
        if(state.layoutQueue)state.layoutQueue.flush(ready);else ready(null);
    };

    function pollPanelState() {
        panelPollTimer = null;
        var dormant=document.hidden || (elements.autoRefresh && !elements.autoRefresh.checked);
        if (panelPulseInFlight || refreshInFlight || frameStatusInFlight || state.pending ||
            effectSelectionInFlight || (state.layoutQueue && state.layoutQueue.hasWork()) || canvasInteractionActive() || document.hidden ||
            (elements.autoRefresh && !elements.autoRefresh.checked)) {
            schedulePanelPoll(dormant?IDLE_POLL_INTERVAL_MS:SELECTION_POLL_INTERVAL_MS); return;
        }
        panelPulseInFlight = true;
        call("getPanelPulse",{selectionOnly:true},function(response) {
            panelPulseInFlight = false;
            if (state.pending || (elements.autoRefresh && !elements.autoRefresh.checked) || (state.layoutQueue && state.layoutQueue.hasWork()) || canvasInteractionActive() || document.hidden) {
                schedulePanelPoll(document.hidden || (elements.autoRefresh && !elements.autoRefresh.checked)?IDLE_POLL_INTERVAL_MS:SELECTION_POLL_INTERVAL_MS); return;
            }
            if(response && response.ok && response.selectionOnly===true){
                var token=response.target && response.target.token || null;
                if(selectionCandidateStamp!==response.selectionStamp){selectionCandidateStamp=response.selectionStamp;selectionCandidateAt=Date.now();}
                var different=token!==state.targetToken || !!(token && state.graphSnapshot &&
                    (response.projectScope || "")!==(state.graphSnapshot.projectScope || ""));
                if(different){
                    cancelNodeDiagnostics();
                    if(Date.now()-selectionCandidateAt>=SELECTION_SETTLE_MS){
                        if(token)refresh(false,false);else clearAbsentTarget();
                    }
                }else if(token && Date.now()-lastFullRefreshAt>=FULL_AUDIT_INTERVAL_MS){
                    refresh(false,false);
                }else if(token && Date.now()-lastStatusPollAt>=IDLE_POLL_INTERVAL_MS){
                    lastStatusPollAt=Date.now();panelPulseInFlight=true;
                    call("getPanelPulse",null,function(status){
                        panelPulseInFlight=false;
                        if(!state.pending && !canvasInteractionActive() && !document.hidden && (!elements.autoRefresh || elements.autoRefresh.checked)){
                            if(!status || !status.ok || (status.target && status.target.token===state.targetToken &&
                                (status.projectScope || "")===(state.graphSnapshot && state.graphSnapshot.projectScope || "")))adoptPanelPulse(status);
                        }
                        schedulePanelPoll(SELECTION_POLL_INTERVAL_MS);
                    });return;
                }
                schedulePanelPoll(SELECTION_POLL_INTERVAL_MS);return;
            }
            // Compatibility for a bridge without the lightweight capability.
            adoptPanelPulse(response);
            schedulePanelPoll(unchangedPulseCount >= 3 ? IDLE_POLL_INTERVAL_MS : ACTIVE_POLL_INTERVAL_MS);
        });
    }
    function adoptPanelPulse(response){
            if (response && response.ok) {
                var changed = response.stamp !== lastPulseStamp;
                lastPulseStamp = response.stamp;
                unchangedPulseCount = changed ? 0 : unchangedPulseCount+1;
                adoptFrameStatus(response.frameStatus);
                if(state.graphSnapshot && typeof response.compTime==="number" && response.compTime!==state.graphSnapshot.compTime)
                    state.layoutPlanningDirty=true;
                if(state.graphSnapshot && state.graphSnapshot.renderer && typeof response.authorTimeSeconds==="number" &&
                    response.authorTimeSeconds!==state.graphSnapshot.renderer.timeRemapSeconds)state.layoutPlanningDirty=true;
                if (changed || !resolvedTarget || Date.now()-lastFullRefreshAt >= FULL_AUDIT_INTERVAL_MS) refresh(false,false);
            } else {
                unchangedPulseCount++;
                lastPulseStamp = null;
                if (state.targetToken) refresh(false,false);
            }
    }

    // Prove the panel body executed, even when the host bridge is unavailable.
    elements.banner.className = "banner";
    elements.banner.textContent = "Panel script loaded; asking the host for the selected effect...";

    elements.refresh.addEventListener("click", function () {
        if (state.pending || refreshInFlight) return;
        failedGraphInitializations = {};
        clearBanner(true);
        refresh(true, true, true);
    });
    if (elements.graphCanvas) elements.graphCanvas.addEventListener("contextmenu", showGraphContextMenu);
    if (window.StarfieldNodePalette) nodePalette = window.StarfieldNodePalette.create({
        root: document.getElementById("nodePalette"),
        items: document.getElementById("nodePaletteItems"),
        toggle: document.getElementById("nodePaletteToggle"), viewport: elements.graphScroll,
        prepare: hideGraphContextMenu,
        guard: function () {
            if (!state.targetToken || !state.revision || (state.pending && !state.addingNodes && !state.queuedEditing) ||
                (state.graphMode && !state.topologyReady) || (graphAdditionQueue && !graphAdditionQueue.canAccept()) || (editQueue && !editQueue.canAccept()) || canvasInteractionActive(true)) return null;
            // Our own receipts may arrive during the next drag. Only an external
            // graph/geometry/target adoption invalidates that in-progress gesture.
            return JSON.stringify([state.targetToken,state.additionEpoch]);
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
        if(state.pending)return;
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
            if (event.repeat || (state.pending && !state.queuedEditing)) return;
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
    window.addEventListener("starfield:nativeDelete", function () {
        var target = document.activeElement;
        if (target && (target.tagName === "INPUT" || target.tagName === "TEXTAREA" ||
                       target.tagName === "SELECT" || target.isContentEditable)) return;
        if ((state.pending && !state.queuedEditing) || !editableSelectedNodeIds().length) return;
        hideGraphContextMenu();
        deleteSelection();
    });
    window.addEventListener("starfield:nativeGraphEdited",function(event){
        var result=event.detail;
        if(!result || !result.target || result.target.token!==state.targetToken)return;
        if(!result.ok){
            var error=result.error || {code:"native_graph_delete_failed",message:"Native deletion requires attention."};
            showError(error.code,error.message+" "+JSON.stringify({clearDispatched:!!result.clearDispatched,committed:!!result.committed,
                guarded:!!result.guarded,outcomeUnknown:!!result.outcomeUnknown,diagnostics:result.diagnostics || {}}),true);return;
        }
        refresh(false,false);
    });
    syncTargetLock();

    // AE can change the effect behind the panel's back: Ctrl+Z, keyframes, or an edit in
    // the Effect Controls window. The protocol has no push channel, so without this the
    // panel keeps displaying whatever it last adopted (host-side undo left a stale 40 on
    // screen). Re-read whenever the panel regains focus, but never race a pending write.
    if (window.addEventListener) {
        window.addEventListener("pagehide",function(){canvasHydrationSerial++;canvasHydrating=false;canvasEditIntents=[];cancelNodeDiagnostics();hideNodeDiagnosticTooltip();if(editQueue)editQueue.cancel();diagnosticEpoch++;queuedDraft=null;});
        window.addEventListener("scroll",function(event){if(!nodeDiagnosticTooltip || !nodeDiagnosticTooltip.contains(event.target))hideNodeDiagnosticTooltip();},true);
        window.addEventListener("resize",hideNodeDiagnosticTooltip);
        window.addEventListener("keydown",function(event){if(event.key==="Escape")hideNodeDiagnosticTooltip();});
        window.addEventListener("focus", function () {
            if(effectSelectionInFlight || queuedEffectSelection)return;
            if(elements.autoRefresh && !elements.autoRefresh.checked)refresh(false,false);
            else schedulePanelPoll(0);
        });
        window.addEventListener("blur",function(){
            hideNodeDiagnosticTooltip();
            if(!canvasInteractionActive() && state.layoutQueue && state.layoutQueue.hasWork())state.layoutQueue.flush(function(){});
        });
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
    schedulePanelPoll(SELECTION_POLL_INTERVAL_MS);
    if (document.addEventListener) {
        document.addEventListener("visibilitychange", function () {
            if (!document.hidden &&
                (!elements.autoRefresh || elements.autoRefresh.checked)) refresh(false, false);
        });
    }
    if(elements.autoRefresh)elements.autoRefresh.checked=true;
    if (elements.autoRefresh) elements.autoRefresh.addEventListener("change", function () {
        if (elements.autoRefresh.checked) refresh(false, false);
    });

    refresh(true, true);
})();
