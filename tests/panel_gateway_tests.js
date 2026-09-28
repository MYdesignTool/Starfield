"use strict";

// Host-independent regression checks for the ExtendScript gateway. Run with:
//   node tests/panel_gateway_tests.js
// The fake DOM covers only the scripting properties used by protocol v1; it does
// not replace the AE 2023 host qualification gate.

const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");

const source = fs.readFileSync(path.join(__dirname, "..", "cep_panel", "jsx", "starfield_gateway.jsx"), "utf8");

const initialValues = {
    "Max Particles": 1000,
    "Particles Per Second": 100,
    "Random Seed": 1,
    "Lifetime": 2,
    "Type": 1,
    "Origin": [50, 50, 50],
    "Velocity X": 0,
    "Velocity Y": 0.3,
    "Velocity Z": 0,
    "Size": 8,
    "Opacity": 1,
    "Emitter Size": 0.05,
    "Speed Random": 0.15,
    "Gravity X": 0,
    "Gravity Y": 0,
    "Gravity Z": 0,
    "Linear Drag": 0,
    "Color Start": [1, 1, 1, 0.25],
    "Color End": [1, 1, 1, 1],
    "Size Over Life": 8,
    "Opacity Over Life": 1,
    "Control Source": 2
};

function createHarness(options = {}) {
    const values = {};
    const writes = [];
    let failName = options.failName || null;
    let failCount = options.failCount || 0;
    for (const [name, value] of Object.entries(initialValues)) {
        const property = {
            name,
            value: Array.isArray(value) ? value.slice() : value,
            dimensions: Array.isArray(value) ? value.length : 1,
            numKeys: name === options.animatedName ? 2 : 0,
            isTimeVarying: name === options.animatedName,
            setValue(next) {
                writes.push({ name, value: Array.isArray(next) ? next.slice() : next });
                if (name === failName && failCount > 0) {
                    failCount--;
                    throw new Error("injected setValue failure");
                }
                this.value = Array.isArray(next) ? next.slice() : next;
            }
        };
        values[name] = property;
    }

    const effect = {
        matchName: "org.starfieldfx.particle",
        propertyIndex: 1,
        property(name) { return values[name] || null; }
    };
    const duplicateEffect = { matchName: "org.starfieldfx.particle" };
    const effectInstances = options.effectInstances || 1;
    const parade = {
        numProperties: effectInstances,
        property(index) {
            if (index === 1) return effect;
            return index <= effectInstances ? duplicateEffect : null;
        }
    };
    const layer = {
        id: 29,
        selected: true,
        name: "Particle Layer",
        property(name) { return name === "ADBE Effect Parade" ? parade : null; }
    };
    function CompItem() {}
    const comp = new CompItem();
    comp.id = 17;
    comp.name = "Test Comp";
    comp.numLayers = 1;
    comp.layer = index => index === 1 ? layer : null;

    const undo = { begins: 0, ends: 0 };
    const rootFolder = { id: 5 };
    const app = {
        project: { activeItem: comp, rootFolder },
        beginUndoGroup() { undo.begins++; },
        endUndoGroup() { undo.ends++; }
    };
    const exported = {};
    vm.runInNewContext(source, { app, CompItem, $: { global: exported } }, { filename: "starfield_gateway.jsx" });
    const initialState = JSON.parse(exported.SFLD_getState(JSON.stringify({
        protocol: "org.starfieldfx.panel",
        version: 1,
        requestId: "initial",
        operation: "getState"
    })));

    function call(changes, options = {}) {
        const request = {
            protocol: "org.starfieldfx.panel",
            version: 1,
            requestId: "test",
            operation: "setParameters",
            target: options.omitTarget ? {} : { token: initialState.target ? initialState.target.token : "missing-target" },
            changes
        };
        if (!options.omitRevision) {
            request.baseRevision = options.baseRevision === undefined ? initialState.revision : options.baseRevision;
        }
        return JSON.parse(exported.SFLD_setParameters(JSON.stringify(request)));
    }

    return {
        call,
        values,
        writes,
        undo,
        setLayerId(id) { layer.id = id; },
        setProjectId(id) { rootFolder.id = id; }
    };
}

function testVectorAnimationIsProtected() {
    for (const [name, key, value] of [
        ["Origin", "emitter_origin", [10, 20, 30]],
        ["Color Start", "color_start", [255, 0, 0]]
    ]) {
        const host = createHarness({ animatedName: name });
        const before = JSON.stringify(host.values[name].value);
        const response = host.call([{ key, value }]);
        assert.equal(response.ok, false, name + " should reject panel writes when animated");
        assert.equal(response.error.code, "animated_parameter");
        assert.equal(JSON.stringify(host.values[name].value), before);
        assert.equal(host.undo.begins, 0, "animation is rejected before the undo group");
    }
}

function testFailedBatchRollsBackAndPreservesRawColorAlpha() {
    const host = createHarness({ failName: "Gravity Y", failCount: 1 });
    const beforeX = host.values["Gravity X"].value;
    const beforeY = host.values["Gravity Y"].value;
    const beforeColor = JSON.stringify(host.values["Color Start"].value);
    const response = host.call([
        { key: "color_start", value: [255, 0, 0] },
        { key: "gravity_x", value: 4 },
        { key: "gravity_y", value: 5 }
    ]);

    assert.equal(response.ok, false);
    assert.equal(response.error.code, "host_write_failed");
    assert.equal(response.context.rollbackFailed, false);
    assert.equal(host.values["Gravity X"].value, beforeX);
    assert.equal(host.values["Gravity Y"].value, beforeY);
    assert.equal(JSON.stringify(host.values["Color Start"].value), beforeColor);
    assert.equal(host.undo.begins, 1);
    assert.equal(host.undo.ends, 1);
}

function testRollbackFailureIsReported() {
    const host = createHarness({ failName: "Gravity Y", failCount: 2 });
    const response = host.call([
        { key: "color_start", value: [255, 0, 0] },
        { key: "gravity_x", value: 4 },
        { key: "gravity_y", value: 5 }
    ]);
    assert.equal(response.ok, false);
    assert.equal(response.error.code, "host_write_failed");
    assert.equal(response.context.rollbackFailed, true);
    assert.match(response.error.message, /Rollback also failed/);
    assert.equal(host.undo.begins, 1);
    assert.equal(host.undo.ends, 1);
}

function testSuccessfulWriteStillRefreshesState() {
    const host = createHarness();
    const response = host.call([{ key: "gravity_x", value: 4 }]);
    assert.equal(response.ok, true);
    assert.equal(response.operation, "getState");
    assert.equal(host.values["Gravity X"].value, 4);
    assert.equal(host.undo.begins, 1);
    assert.equal(host.undo.ends, 1);
}

function testDuplicateInstancesOnOneLayerAreAmbiguous() {
    const host = createHarness({ effectInstances: 2 });
    const response = host.call([{ key: "gravity_x", value: 4 }]);
    assert.equal(response.ok, false);
    assert.equal(response.error.code, "ambiguous_target");
    assert.equal(host.writes.length, 0, "the gateway must not pick the first duplicate effect silently");
    assert.equal(host.undo.begins, 0, "ambiguous targets are rejected before opening an undo group");
}

function testRevisionAndTargetTokensAreRequired() {
    const missingRevision = createHarness();
    const missingRevisionResponse = missingRevision.call([{ key: "gravity_x", value: 4 }], { omitRevision: true });
    assert.equal(missingRevisionResponse.ok, false);
    assert.equal(missingRevisionResponse.error.code, "invalid_request");
    assert.equal(missingRevision.writes.length, 0);
    assert.equal(missingRevision.undo.begins, 0);

    const missingTarget = createHarness();
    const missingTargetResponse = missingTarget.call([{ key: "gravity_x", value: 4 }], { omitTarget: true });
    assert.equal(missingTargetResponse.ok, false);
    assert.equal(missingTargetResponse.error.code, "invalid_request");
    assert.equal(missingTarget.writes.length, 0);
    assert.equal(missingTarget.undo.begins, 0);
}

function testSelectedLayerSwitchRejectsOldRevision() {
    const host = createHarness();
    host.setLayerId(30); // Same controls on a different target must not accept the old token.
    const response = host.call([{ key: "gravity_x", value: 4 }]);
    assert.equal(response.ok, false);
    assert.equal(response.error.code, "stale_state");
    assert.equal(host.writes.length, 0);
    assert.equal(host.undo.begins, 0);
}

function testProjectSwitchRejectsOldTargetToken() {
    const host = createHarness();
    host.setProjectId(6); // Layer and controls are unchanged; project identity still differs.
    const response = host.call([{ key: "gravity_x", value: 4 }]);
    assert.equal(response.ok, false);
    assert.equal(response.error.code, "stale_state");
    assert.equal(host.writes.length, 0);
    assert.equal(host.undo.begins, 0);
}

function testStaleBaseRevisionIsRequiredToMatch() {
    const host = createHarness();
    const response = host.call([{ key: "gravity_x", value: 4 }], { baseRevision: "old-revision" });
    assert.equal(response.ok, false);
    assert.equal(response.error.code, "stale_state");
    assert.equal(host.writes.length, 0);
    assert.equal(host.undo.begins, 0);
}

testVectorAnimationIsProtected();
testFailedBatchRollsBackAndPreservesRawColorAlpha();
testRollbackFailureIsReported();
testSuccessfulWriteStillRefreshesState();
testDuplicateInstancesOnOneLayerAreAmbiguous();
testRevisionAndTargetTokensAreRequired();
testSelectedLayerSwitchRejectsOldRevision();
testProjectSwitchRejectsOldTargetToken();
testStaleBaseRevisionIsRequiredToMatch();
console.log("Panel gateway fake-host checks passed.");
