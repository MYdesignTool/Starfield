"use strict";
// Executes actual C++ generated programs. This models expression dependencies,
// not AE's cache implementation, and never starts or dispatches to AE.
const fs = require("fs"), vm = require("vm"), assert = require("assert");
const expressions = JSON.parse(fs.readFileSync("artifacts/graph-generation-expressions.json", "utf8"));
const stale = -1099511627775, missing = -1099511627776;
let checks = 0;
function run(text, checksum, absent = false, wrongIdentity = false) {
    let sourceCalls = 0;
    const owner = { param(name) { return { value: name === "Graph Checksum High" ? checksum >>> 16 : checksum & 65535 }; } };
    const words = [...text.matchAll(/fx\.param\("Node UUID (\d)"\)\.value === (\d+)/g)];
    const identity = Object.fromEntries(words.map(m => ["Node UUID " + m[1], Number(m[2])]));
    const component = /result = fx\.param\(\d+\)\.value\[/.test(text);
    const source = { param(name) { return { value: typeof name === "string" ? identity[name] + (wrongIdentity ? 1 : 0) : component ? [42, 43, 44] : 42 }; } };
    const context = {
        thisProperty: { propertyGroup(up) { assert.strictEqual(up, 1); return { propertyIndex: 3 }; } },
        thisLayer: { effect(key) { if (key === 3) return owner; ++sourceCalls; if (absent) throw Error("removed source"); return source; } }
    };
    return { value: vm.runInNewContext(text, context, { timeout: 1000 }), sourceCalls };
}
for (const text of expressions) {
    const checksum = Number(/\/\/ Starfield graph checksum (\d+)/.exec(text)[1]);
    for (const delta of [1, 65536]) {
        const result = run(text, (checksum ^ delta) >>> 0, true);
        assert.strictEqual(result.value, stale);assert.strictEqual(result.sourceCalls, 0);checks += 2;
    }
    const removed = run(text, checksum, true);
    assert.strictEqual(removed.value, missing);assert.strictEqual(removed.sourceCalls, 1);checks += 2;
    const replaced = run(text, checksum, false, true);
    assert.strictEqual(replaced.value, missing);checks++;
    if (!text.includes("// Starfield Transform affine entry ")) {
        const valid = run(text, checksum);
        assert(Number.isFinite(valid.value) && valid.value >= 42 && valid.value <= 44);
        assert.strictEqual(valid.sourceCalls, 1);checks += 2;
    }
}
assert(expressions.length > 100);
console.log(`Graph binding generation: ${checks} checks, ${expressions.length} actual expressions, passed`);
