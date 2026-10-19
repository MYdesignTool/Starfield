"use strict";

// Discover every panel suite, keep running after a failure, and retain its log.
const fs = require("node:fs");
const path = require("node:path");
const {spawnSync} = require("node:child_process");
const repository = path.resolve(__dirname, "..");
const output = path.join(repository, "artifacts", "panel-tests");
fs.mkdirSync(output, {recursive: true});
const results = [];
for (const suite of fs.readdirSync(__dirname).filter(name => name.endsWith("_tests.js")).sort()) {
    // Child processes do not inherit the parent's --expose-internals option.
    // Syntax/UI suites use the bundled Acorn instead of a vendored dependency.
    const result = spawnSync(process.execPath, ["--expose-internals",path.join(__dirname, suite)], {
        cwd: repository, encoding: "utf8", windowsHide: true,
        env: {...process.env,STARFIELD_PANEL_ROOT:process.env.STARFIELD_PANEL_ROOT || path.join(repository,"cep_panel")}
    });
    fs.writeFileSync(path.join(output, suite + ".log"),
        (result.stdout || "") + (result.stderr || "") + (result.error ? String(result.error) : ""));
    const passed = !result.error && result.status === 0;
    results.push({suite, passed, exitCode: result.status});
    console.log(`${passed ? "PASS" : "FAIL"} ${suite}`);
}
fs.writeFileSync(path.join(output, "results.json"), JSON.stringify({node: process.version, results}, null, 2) + "\n");
const failures = results.filter(result => !result.passed).length;
console.log(`${results.length} suites, ${failures} failures. Logs: artifacts/panel-tests/`);
process.exitCode = failures ? 1 : 0;
