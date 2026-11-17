"use strict";
const assert=require("node:assert/strict"),path=require("node:path"),child=require("node:child_process");
const repo=path.resolve(__dirname,".."),panel=process.env.STARFIELD_PANEL_ROOT || path.join(repo,"web_panel");
const result=child.spawnSync("powershell",["-NoProfile","-ExecutionPolicy","Bypass","-File",path.join(repo,"tools/Assert-PanelContract.ps1"),"-PanelRoot",panel],{encoding:"utf8",windowsHide:true});
assert.equal(result.status,0,(result.stdout || "")+(result.stderr || ""));
assert.match(result.stdout,/Read-only panel invariant passed/);
console.log("Deployment panel invariant: manifest entries, gateway/cache generations and embedded record contracts passed");
