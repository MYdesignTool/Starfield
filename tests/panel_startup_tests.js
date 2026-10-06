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
for (const id of ["banner", "chain", "targetLine", "modeLine", "revisionLine", "resolutionLine", "preset", "refresh", "graphScroll", "autoRefresh"]) {
    elements[id] = new FakeElement(id);
}
elements.autoRefresh.checked = true;

const timers = new Map();
let nextTimerId = 1;
const gatewayHost = {};
vm.runInNewContext(fs.readFileSync(path.join(__dirname,"..","cep_panel","jsx","starfield_gateway.jsx"),"utf8"), {$:{global:gatewayHost}});
const actualReadyToken = gatewayHost.SFLD_ready();
let stateCalls = 0, gatewayLoads = 0, gatewayProbes = 0;
let pulseCalls = 0, pulseStamp = "idle", holdPulse = false, pendingPulse;
let now = 1000;
class Clock extends Date { static now() { return now; } }
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
            const deliver = callback;
            callback = text => {
                try {
                    const reply = JSON.parse(text);
                    if (reply.protocol === "org.starfieldfx.panel") {
                        reply.version = 1;
                        reply.gatewayBuild = actualReadyToken.split("/").pop();
                        text = JSON.stringify(reply);
                    }
                } catch (_) { /* empty startup replies stay empty */ }
                deliver(text);
            };
            if (script.indexOf("SFLD_ready") >= 0 && !/return SFLD_(get|set|sync)/.test(script)) {
                if(script.indexOf("$.evalFile") >= 0) {gatewayLoads++;callback(actualReadyToken);}
                else {gatewayProbes++;callback(gatewayProbes===1 ? "org.starfieldfx.panel/1/stale-gateway" : actualReadyToken);}
                return;
            }
            if (script.indexOf("SFLD_getPanelState(") >= 0) {
                if (!gatewayLoads) gatewayLoads++;
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
                        target: { token: "target-1", comp: "Comp 1", layer: "Particles" }, nodes: [], snapshot:{initialized:false} };
                callback(JSON.stringify(response));
                return;
            }
            if (script.indexOf("SFLD_getPanelPulse(") >= 0) {
                pulseCalls++;
                const respond = () => callback(JSON.stringify({protocol:"org.starfieldfx.panel",ok:true,
                    stamp:pulseStamp,frameStatus:{ok:true,targetToken:"target-1",available:false}}));
                if (holdPulse) pendingPulse = respond;
                else respond();
                return;
            }
            if (script.indexOf("SFLD_getFrameStatus(") >= 0) {
                callback(JSON.stringify({protocol:"org.starfieldfx.panel",ok:true,targetToken:"target-1",available:false}));
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
vm.runInNewContext(source, { window, document, Date:Clock, Math, JSON, String, Number, isFinite });

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
assert.strictEqual(timers.size, 1, "successful discovery leaves only one background poll timer");

function poll(delay) {
    const entry = Array.from(timers.entries()).find(([, timer]) => timer.delay === delay);
    assert.ok(entry, "one completion-scheduled poll exists at " + delay + "ms");
    timers.delete(entry[0]);
    entry[1].callback();
}
const beforeIdle = stateCalls;
poll(2000); // First stamp establishes current target markers.
assert.equal(stateCalls, beforeIdle + 1);
for (let i = 0; i < 3; i++) poll(500);
assert.equal(stateCalls, beforeIdle + 1, "unchanged pulses never reread native nodes");
assert.equal(timers.size, 1, "no independent frame-status/full-refresh interval remains");
document.hidden = true;
const beforeHidden = pulseCalls;
poll(2000);
assert.equal(pulseCalls, beforeHidden, "hidden panel makes no host request");
document.hidden = false;
elements.autoRefresh.checked = false;
poll(2000);
assert.equal(pulseCalls, beforeHidden, "disabled automatic refresh makes no host request");
elements.autoRefresh.checked = true;
holdPulse = true;
poll(2000);
assert.equal(timers.size, 1, "while a host call is pending, only its timeout exists");
assert.equal(Array.from(timers.values())[0].delay, 8000);
holdPulse = false;
pendingPulse();
assert.equal(timers.size, 1, "the next poll is scheduled only after completion");
now += 15001;
poll(2000);
assert.equal(stateCalls, beforeIdle + 2, "periodic audit catches same-frame changes absent from markers");
pulseStamp = "changed";
poll(2000);
assert.equal(stateCalls, beforeIdle + 3, "changed markers trigger one combined full inspection");

resizeObservers[0]();
resizeObservers[0]();
windowListeners.resize();
assert.equal(animationFrames.length, 1, "observer and resize events defer/coalesce layout writes");
animationFrames.shift()();
resizeObservers[0]();
assert.equal(animationFrames.length, 0, "unchanged dimensions must not start a resize feedback loop");

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

const retainedMessage = elements.banner.textContent;
assert.equal(window.onerror("ResizeObserver loop limit exceeded", "index.html", 0), true);
assert.equal(elements.banner.textContent, retainedMessage, "resize warnings must not mask bootstrap failure");
console.log("Panel discovery, adaptive pulse, pause, bounded requests, audit, resize and bootstrap retry checks passed.");
