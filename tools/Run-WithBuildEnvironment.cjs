"use strict";

// Some launchers supply both Path and PATH. MSBuild's case-insensitive environment
// dictionary rejects that block before CL starts. Normalize only this child; no
// user/system environment or Adobe process is changed.
const { spawnSync } = require("node:child_process");
const [command, ...args] = process.argv.slice(2);
if (!command) throw new Error("Usage: node Run-WithBuildEnvironment.cjs <command> [arguments]");
const env = {};
for (const key of Object.keys(process.env)) env[key.toUpperCase()] = process.env[key];
if (process.env.Path) env.PATH = process.env.Path;
const result = spawnSync(command, args, { env, stdio: "inherit", windowsHide: true });
if (result.error) throw result.error;
process.exit(result.status === null ? 1 : result.status);
