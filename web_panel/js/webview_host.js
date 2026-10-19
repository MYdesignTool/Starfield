// Transport adapter only. Existing graph, native transactions and JSX are shared
// with CEP during qualification; neither parameter semantics nor wire IDs change.
(function (root) {
    "use strict";
    var config = root.__starfieldWebViewConfig, web = root.chrome && root.chrome.webview;
    if (!config || !web || root.__adobe_cep__) return;
    var pending = Object.create(null), count = 0, sequence = 0, closed = false;
    var channel = "org.starfieldfx.webview2/1";
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
        if (kind === "evalScript") request.script = script;
        pending[id] = {callback:callback || function () {},build:build}; count++;
        try { web.postMessage(request); }
        catch (error) { var entry = pending[id]; delete pending[id]; count--; entry.callback(compatibleError(failure("webview_transport_failed", String(error)),entry.build)); }
    }
    web.addEventListener("message", function (event) {
        var reply = event.data;
        if (!reply || reply.channel !== channel || typeof reply.id !== "string" ||
            typeof reply.result !== "string" || !Object.prototype.hasOwnProperty.call(pending, reply.id)) return;
        var entry = pending[reply.id]; delete pending[reply.id]; count--; entry.callback(compatibleError(reply.result,entry.build));
    });
    root.__adobe_cep__ = {
        evalScript:function (script, callback) { send("evalScript", String(script), callback); },
        getSystemPath:function (kind) { return kind === "extension" ? config.root : ""; },
        requestOpenExtension:function (id) { if (id === "org.starfieldfx.panel.presets") send("openPresets"); },
        closeExtension:function () { send("close"); }
    };
    root.addEventListener("pagehide", function () {
        closed = true;
        var callbacks = pending; pending = Object.create(null); count = 0;
        Object.keys(callbacks).forEach(function (id) { var entry=callbacks[id];entry.callback(compatibleError(failure("panel_closed", "The native panel was closed."),entry.build)); });
    });
    // Browser text controls retain their local Undo. Canvas focus routes the
    // established AE Undo/Redo commands through the same deferred host queue.
    root.addEventListener("keydown", function (event) {
        var element = event.target, key = String(event.key).toLowerCase();
        if (element && (/^(INPUT|TEXTAREA|SELECT)$/.test(element.tagName) || element.isContentEditable)) return;
        if ((event.ctrlKey || event.metaKey) && !event.altKey && (key === "z" || key === "y")) {
            event.preventDefault(); if (!event.repeat) send(key === "y" || event.shiftKey ? "redo" : "undo");
        }
    });
}(window));
