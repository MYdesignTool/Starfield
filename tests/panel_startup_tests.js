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
for (const id of ["banner", "chain", "targetLine", "modeLine", "revisionLine", "resolutionLine", "preset", "refresh"]) {
    elements[id] = new FakeElement(id);
}

const timers = new Map();
let nextTimerId = 1;
let stateCalls = 0;
const window = {
    __adobe_cep__: {
        evalScript(script, callback) {
            if (script.indexOf("SFLD_ready") >= 0) {
                callback("org.starfieldfx.panel/1/native-node-sync-1");
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

console.log("panel startup discovery passed (empty reply and delayed target recovered without manual Refresh)");
