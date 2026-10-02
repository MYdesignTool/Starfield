"use strict";

const assert = require("assert");
const fs = require("fs");
const path = require("path");
const vm = require("vm");

class FakeElement {
    constructor(id) {
        this.id = id;
        this.className = "";
        this.textContent = "";
        this.innerHTML = "";
        this.value = "";
        this.dataset = {};
        this.style = {};
        this.children = [];
        this.listeners = {};
    }

    addEventListener(type, callback) { this.listeners[type] = callback; }
    appendChild(child) { this.children.push(child); }
}

const elements = {};
for (const id of ["banner", "chain", "targetLine", "modeLine", "revisionLine", "resolutionLine", "preset", "refresh", "graphScroll"]) {
    elements[id] = new FakeElement(id);
}

const timers = new Map();
let nextTimerId = 1;
const gatewayHost = {};
vm.runInNewContext(fs.readFileSync(path.join(__dirname,"..","cep_panel","jsx","starfield_gateway.jsx"),"utf8"), {$:{global:gatewayHost}});
const actualReadyToken = gatewayHost.SFLD_ready();
let stateCalls = 0, gatewayLoads = 0, gatewayProbes = 0;
let failParameterWrite = true;
let simulateBootstrapFailure = false, bootstrapCalls = 0;
const windowListeners = {};
const resizeObservers = [], animationFrames = [];
const window = {
    addEventListener(type, callback) { windowListeners[type] = callback; },
    ResizeObserver: class { constructor(callback) { resizeObservers.push(callback); } observe() {} },
    requestAnimationFrame(callback) { animationFrames.push(callback); },
    __adobe_cep__: {
        getSystemPath() { return "C:/Starfield/cep_panel"; },
        evalScript(script, callback) {
            if (script.indexOf("SFLD_ready") >= 0) {
                if(script.indexOf("$.evalFile") >= 0) {gatewayLoads++;callback(actualReadyToken);}
                else {gatewayProbes++;callback(gatewayProbes===1 ? "org.starfieldfx.panel/1/stale-gateway" : actualReadyToken);}
                return;
            }
            if (script.indexOf("SFLD_getState(") >= 0) {
                stateCalls += 1;
                if (stateCalls === 1) {
                    callback(""); // CEP can return an empty result during AE startup.
                    return;
                }
                const response = stateCalls < 7
                    ? { protocol: "org.starfieldfx.panel", ok: false,
                        error: { code: "no_target", message: "Target is not ready yet." } }
                    : { protocol: "org.starfieldfx.panel", ok: true, revision: "r2", controlSource: "AE Controls",
                        resolution: "name", layoutPersistence: true, layout: {},
                        target: { token: "target-1", comp: "Comp 1", layer: "Particles" }, nodes: [] };
                callback(JSON.stringify(response));
                return;
            }
            if (script.indexOf("SFLD_setParameters(") >= 0) {
                callback(JSON.stringify(failParameterWrite ? { protocol:"org.starfieldfx.panel", ok:false,
                    error:{code:"graph_commit_failed",message:"Particle: creation failed; exact host detail"} } :
                    {protocol:"org.starfieldfx.panel",ok:true,revision:"r3",controlSource:"AE Controls",
                        resolution:"name",layoutPersistence:true,layout:{},
                        target:{token:"target-1",comp:"Comp 1",layer:"Particles"},nodes:[]}));
                return;
            }
            if (simulateBootstrapFailure && script.indexOf("SFLD_getGraphSnapshot(") >= 0) {
                callback(JSON.stringify({protocol:"org.starfieldfx.panel",ok:true,snapshot:{initialized:false}}));
                return;
            }
            if (simulateBootstrapFailure && script.indexOf("SFLD_syncGraphSnapshot(") >= 0) {
                bootstrapCalls++;
                callback(JSON.stringify({protocol:"org.starfieldfx.panel",ok:false,
                    error:{code:"node_effect_sync_failed",message:"Particle: bootstrap failed"}}));
                return;
            }
            callback("undefined");
        }
    },
    setTimeout(callback, delay) {
        const id = nextTimerId++;
        timers.set(id, { callback, delay });
        return id;
    },
    clearTimeout(id) { timers.delete(id); }
};

const document = {
    getElementById(id) { return elements[id]; },
    createElement() { return new FakeElement("created"); },
    addEventListener() {}
};

const graphViewSource = fs.readFileSync(path.join(__dirname, "..", "cep_panel", "js", "graph_view.js"), "utf8");
vm.runInNewContext(graphViewSource, { window, document, Date, Math, JSON, String, Number, isFinite });
const source = fs.readFileSync(path.join(__dirname, "..", "cep_panel", "js", "panel.js"), "utf8");
vm.runInNewContext(source, { window, document, Date, Math, JSON, String, Number, isFinite });

assert.strictEqual(stateCalls, 1, "panel should request the current target immediately on startup");
assert.strictEqual(gatewayLoads, 1, "panel reloads a stale gateway and accepts the actual paired JSX readiness token");
assert.match(elements.banner.textContent, /retrying shortly/i, "empty startup reply should show a retry state");

const expectedRetryDelays = [250, 750, 1500, 3000, 5000, 5000];
for (let i = 0; i < expectedRetryDelays.length; i++) {
    const expectedDelay = expectedRetryDelays[i];
    const entry = Array.from(timers.entries()).find(([, timer]) => timer.delay === expectedDelay);
    assert.ok(entry, "panel should keep retrying with a capped backoff");
    timers.delete(entry[0]);
    entry[1].callback();
    if (i < expectedRetryDelays.length - 1) {
        assert.match(elements.banner.textContent, /retrying shortly/i,
            "target discovery should continue while the project/effect is unavailable");
    }
}

assert.strictEqual(stateCalls, 7, "panel should keep discovering until the target becomes available");
assert.strictEqual(elements.targetLine.textContent, "Comp 1 / Particles", "retry should populate target details");
assert.strictEqual(elements.banner.className, "banner hidden",
    "successful discovery should clear the banner: " + elements.banner.textContent);
assert.strictEqual(timers.size, 0, "successful discovery should stop the retry loop");

elements.preset.value = "defaults";
elements.preset.listeners.change();
const retainedMessage = elements.banner.textContent;
assert.match(retainedMessage, /graph_commit_failed.*exact host detail/,
    "a failed edit must leave the complete diagnostic visible after its immediate refresh");
windowListeners.focus();
assert.equal(elements.banner.textContent, retainedMessage,
    "a successful background read must not erase the edit error");
assert.equal(elements.banner.className, "banner error");
assert.equal(window.onerror("ResizeObserver loop limit exceeded", "index.html", 0), true);
assert.equal(elements.banner.textContent, retainedMessage, "resize warnings must not mask the native edit failure");
resizeObservers[0]();
resizeObservers[0]();
windowListeners.resize();
assert.equal(animationFrames.length, 1, "observer and resize events defer/coalesce layout writes");
animationFrames.shift()();
resizeObservers[0]();
assert.equal(animationFrames.length, 0, "unchanged observed dimensions must not start a resize feedback loop");
elements.refresh.listeners.click();
assert.equal(elements.banner.className, "banner hidden", "manual Refresh acknowledges the retained error");
elements.preset.value = "defaults";
elements.preset.listeners.change();
failParameterWrite = false;
elements.preset.value = "defaults";
elements.preset.listeners.change();
assert.equal(elements.banner.className, "banner hidden", "a successful user edit clears the previous edit error");

window.StarfieldGraphCodec = {};
window.StarfieldGraphTransactions = {};
window.StarfieldGraphEdits = {};
simulateBootstrapFailure = true;
elements.refresh.listeners.click();
assert.equal(bootstrapCalls, 1);
assert.match(elements.banner.textContent, /node_effect_sync_failed.*Particle: bootstrap failed/);
windowListeners.focus();
windowListeners.focus();
assert.equal(bootstrapCalls, 1, "background reads must not repeatedly mutate a failing bootstrap");
assert.match(elements.banner.textContent, /Particle: bootstrap failed/);
elements.refresh.listeners.click();
assert.equal(bootstrapCalls, 2, "manual Refresh explicitly retries bootstrap once");

console.log("Panel startup and retained error checks passed (discovery, failed edit survives background refresh, manual/successful edit acknowledgement, bootstrap retry suppression).");
