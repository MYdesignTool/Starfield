// Transport adapter only. Existing graph, native transactions and JSX are shared
// with CEP during qualification; neither parameter semantics nor wire IDs change.
(function (root) {
    "use strict";
    var config = root.__starfieldWebViewConfig, web = root.chrome && root.chrome.webview;
    if (!config || !web || root.__adobe_cep__) return;
    var pending = Object.create(null), count = 0, sequence = 0, closed = false;
    var channel = "org.starfieldfx.webview2/1";
    var doc = root.document, graphInput = !!(doc && doc.getElementById("graphScroll"));
    var page = Date.now().toString(36) + "-" + Math.random().toString(36).slice(2), inputEpoch = 1;
    var deleteDown = false, nativeDeleteDelivered = false, lastDeleteSerial = 0;
    function textTarget(element) { return element && (/^(INPUT|TEXTAREA|SELECT)$/.test(element.tagName) || element.isContentEditable); }
    function inputState(changed) {
        if (!graphInput || closed) return;
        if (changed) { inputEpoch++; nativeDeleteDelivered = false; }
        try { web.postMessage({channel:channel,id:"wv-" + (++sequence),kind:"graphInputState",
            script:page + "|" + inputEpoch + "|" + (deleteDown ? "1" : "0")}); } catch (ignored) {}
    }
    function failure(code, message) { return JSON.stringify({ok:false,error:{code:code,message:message}}); }
    function compatibleError(result, build) {
        if (!build || result.indexOf('{"ok":false,"error":') !== 0) return result;
        try { var error = JSON.parse(result); error.protocol = "org.starfieldfx.panel"; error.version = 1;
            error.gatewayBuild = build; return JSON.stringify(error); } catch (ignored) { return result; }
    }
    function send(kind, script, callback) {
        var builds = script && script.match(/native-presets-[0-9]+/g), build = builds ? builds[builds.length - 1] : null;
        if (closed || count >= 64 || (script && script.length > 1048576)) {
            if (callback) callback(compatibleError(failure("webview_unavailable", "The native panel is closed or its request queue is full."), build));
            return;
        }
        var id = "wv-" + (++sequence), request = {channel:channel,id:id,kind:kind};
        if (kind === "evalScript" || kind === "evalScriptKeepGraphFocus" || kind === "nativeGraphDelete" || kind === "nativeGraphPatch" || kind === "nativeGraphDiagnostics") request.script = script;
        pending[id] = {callback:callback || function () {},build:build}; count++;
        if((kind==="nativeGraphPatch" || kind==="nativeGraphDelete" || kind==="nativeGraphDiagnostics") && typeof root.setTimeout==="function") {
            pending[id].timer=root.setTimeout(function(){
                var entry=pending[id];if(!entry)return;delete pending[id];count--;
                entry.callback(failure(kind==="nativeGraphDiagnostics"?"graph_diagnostics_unavailable":"graph_edit_outcome_unknown",
                    "AE did not return this graph request in time. Refresh to inspect the actual result; no retry was made."));
            },30000);
        }
        try { web.postMessage(request); }
        catch (error) { var entry = pending[id]; delete pending[id]; count--;if(entry.timer && root.clearTimeout)root.clearTimeout(entry.timer);entry.callback(compatibleError(failure("webview_transport_failed", String(error)),entry.build)); }
    }
    web.addEventListener("message", function (event) {
        var reply = event.data;
        if (reply && reply.channel === channel && reply.event === "nativeGraphEdited") {
            if (graphInput && !closed && reply.result && typeof reply.result === "object")
                root.dispatchEvent(new root.CustomEvent("starfield:nativeGraphEdited", {detail:reply.result}));
            return;
        }
        if (reply && reply.channel === channel && reply.event === "graphDelete") {
            if (!graphInput || closed || reply.page !== page || reply.inputEpoch !== inputEpoch || deleteDown ||
                !Number.isSafeInteger(reply.serial) || reply.serial <= lastDeleteSerial || typeof reply.backspace !== "boolean" ||
                !Number.isSafeInteger(reply.expires) || reply.expires < Date.now() || reply.expires > Date.now() + 1000 || !doc.hasFocus()) return;
            lastDeleteSerial = reply.serial;
            nativeDeleteDelivered = true;
            var element = doc.activeElement;
            if (textTarget(element)) {
                if (element.tagName !== "SELECT" && !element.readOnly && !element.disabled)
                    doc.execCommand(reply.backspace ? "delete" : "forwardDelete");
            } else root.dispatchEvent(new root.CustomEvent("starfield:nativeDelete"));
            return;
        }
        if (!reply || reply.channel !== channel || typeof reply.id !== "string" ||
            typeof reply.result !== "string" || !Object.prototype.hasOwnProperty.call(pending, reply.id)) return;
        var entry = pending[reply.id]; delete pending[reply.id]; count--;if(entry.timer && root.clearTimeout)root.clearTimeout(entry.timer);entry.callback(compatibleError(reply.result,entry.build));
    });
    root.__adobe_cep__ = {
        evalScript:function (script, callback, keepGraphFocus) { send(keepGraphFocus === true ? "evalScriptKeepGraphFocus" : "evalScript", String(script), callback); },
        getSystemPath:function (kind) { return kind === "extension" ? config.root : ""; },
        requestOpenExtension:function (id) { if (id === "org.starfieldfx.panel.presets") send("openPresets"); },
        closeExtension:function () { send("close"); }
    };
    if (config.nativeGraphDelete === true && graphInput) root.StarfieldNativeGraph = {
        patch:function(target,revision,checksum,delta,callback,scope){
            var uuid=/^[0-9a-f]{32}$/,token=/^p0-c[1-9][0-9]*-l[1-9][0-9]*$/;
            function list(items){if(!Array.isArray(items) || items.length>4)throw new Error("Invalid connection count.");return items.map(function(e){if(!e || !uuid.test(e.target) || !uuid.test(e.id))throw new Error("Invalid connection identity.");return e.target+e.id;}).join(",");}
            var payload;
            try{if(!token.test(target) || !Number.isInteger(revision) || revision<1 || revision>16777214 || !/^[0-9a-f]{8}$/.test(checksum) || typeof scope!=="string" || scope.length>4096 || /[\x00-\x1f|]/.test(scope) || !delta || !Array.isArray(delta.removed) || delta.removed.length>63 || !Array.isArray(delta.rows) || delta.rows.length>63)throw new Error("Invalid native patch.");
                delta.removed.forEach(function(id){if(!uuid.test(id))throw new Error("Invalid node identity.");});
                var rows=delta.rows.map(function(row){if(!row || !uuid.test(row.source))throw new Error("Invalid source identity.");return row.source+":"+list(row.expected)+">"+list(row.desired);}).join(";");
                payload=target+"|"+revision+"|"+checksum+"|"+scope+"|"+delta.removed.join(",")+"|"+rows;
                if(payload.length>48000)throw new Error("Native patch capacity exceeded.");
            }catch(e){callback(JSON.parse(failure("invalid_native_patch",String(e))));return;}
            send("nativeGraphPatch",payload,function(text){var reply;try{reply=JSON.parse(text);}catch(e){reply=JSON.parse(failure("graph_edit_outcome_unknown","No valid native patch result. Refresh before continuing."));}callback(reply);});
        },
        diagnostics:function(target,revision,checksum,scope,callback){
            if(!/^p0-c[1-9][0-9]*-l[1-9][0-9]*$/.test(target) || !Number.isInteger(revision) || revision<1 || revision>16777214 || !/^[0-9a-f]{8}$/.test(checksum) || typeof scope!=="string" || scope.length>4096 || /[\x00-\x1f|]/.test(scope)){callback({ok:false});return;}
            send("nativeGraphDiagnostics",target+"|"+revision+"|"+checksum+"|"+scope+"|",function(text){try{callback(JSON.parse(text));}catch(e){callback({ok:false});}});
        },
        deleteNodes:function (target, revision, checksum, ids, callback, scope) {
            if (typeof target !== "string" || !/^p0-c[1-9][0-9]*-l[1-9][0-9]*$/.test(target) ||
                !Number.isInteger(revision) || revision < 1 || revision > 16777214 ||
                typeof checksum !== "string" || !/^[0-9a-f]{8}$/.test(checksum) ||
                typeof scope !== "string" || scope.length > 4096 || /[\x00-\x1f|]/.test(scope) ||
                !Array.isArray(ids) || !ids.length || ids.length > 63 || ids.some(function(id,index){
                    return typeof id !== "string" || !/^[0-9a-f]{32}$/.test(id) || /^0{32}$/.test(id) ||
                        id === "000000000000000000000000000000ff" || ids.indexOf(id) !== index;
                })) {
                callback(JSON.parse(failure("invalid_native_delete", "Invalid native graph deletion request."))); return;
            }
            send("nativeGraphDelete", target + "|" + revision + "|" + checksum + "|" + scope + "|" + ids.join(","), function(text){
                var result;try { result=JSON.parse(text); } catch (error) {
                    result=JSON.parse(failure("native_delete_outcome_unknown", "No valid native deletion result. Refresh before continuing."));
                }
                callback(result);
            });
        }
    };
    root.addEventListener("pagehide", function () {
        closed = true;
        var callbacks = pending; pending = Object.create(null); count = 0;
        Object.keys(callbacks).forEach(function (id) { var entry=callbacks[id];if(entry.timer && root.clearTimeout)root.clearTimeout(entry.timer);entry.callback(compatibleError(failure("panel_closed", "The native panel was closed."),entry.build)); });
    });
    // Browser text controls retain their local Undo. Canvas focus routes the
    // established AE Undo/Redo commands through the same deferred host queue.
    root.addEventListener("keydown", function (event) {
        var element = event.target, key = String(event.key).toLowerCase();
        if (graphInput && (key === "delete" || key === "backspace") && !event.ctrlKey && !event.metaKey && !event.altKey && !event.shiftKey) {
            if (nativeDeleteDelivered) { event.preventDefault(); event.stopImmediatePropagation(); return; }
            if (!deleteDown) { deleteDown = true; inputState(true); }
        } else if (graphInput) inputState(true);
        if (textTarget(element)) return;
        if ((event.ctrlKey || event.metaKey) && !event.altKey && (key === "z" || key === "y")) {
            event.preventDefault();
            if (!event.repeat) {
                var kind=key === "y" || event.shiftKey ? "redo" : "undo";
                function perform(done){send(kind,null,function(raw){
                    if(raw==="ok"){if(done)done({ok:true});return;}
                    var result;try{result=JSON.parse(raw);}catch(ignored){result={ok:false,error:{code:"history_outcome_unknown",message:"No valid history result. Refresh before continuing."}};}
                    if(done)done(result && result.ok===false?result:{ok:false,error:{code:"history_outcome_unknown",message:"No confirmed history result. Refresh before continuing."}});
                });}
                if(graphInput && typeof root.StarfieldGraphHistory==="function")root.StarfieldGraphHistory(perform);
                else perform();
            }
        }
    }, true);
    if (graphInput) {
        root.addEventListener("keyup", function (event) {
            if (event.key === "Delete" || event.key === "Backspace") {
                deleteDown = false; nativeDeleteDelivered = false; inputState(false);
            }
        }, true);
        ["pointerdown", "focusin", "focusout", "blur", "focus"].forEach(function (name) {
            root.addEventListener(name, function () { deleteDown = false; inputState(true); }, true);
        });
        inputState(false);
    }
}(window));
