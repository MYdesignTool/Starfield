"use strict";
// Exercise only synthetic Junctions inside artifacts; never the installed CEP.
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),crypto=require("node:crypto"),{spawnSync}=require("node:child_process");
const repo=path.resolve(__dirname,".."),fixture=path.join(repo,"artifacts","webview2","retirement-tests-"+crypto.randomUUID());
const cep=path.join(fixture,"Adobe","CEP"),extension=path.join(cep,"extensions","cep_panel"),retained=path.join(cep,"starfield-legacy-cep-disabled");
fs.mkdirSync(path.dirname(extension),{recursive:true});fs.symlinkSync(path.join(repo,"web_panel"),retained,"junction");
const powershell=path.join(process.env.SystemRoot || "C:\\Windows","System32","WindowsPowerShell","v1.0","powershell.exe");
function run(action,extra=[]) {
    const result=spawnSync(powershell,["-NoProfile","-ExecutionPolicy","Bypass","-File",path.join(repo,"tools","Set-LegacyCepPanel.ps1"),"-ExtensionPath",extension,"-Action",action,...extra],{cwd:repo,encoding:"utf8",windowsHide:true});
    if(result.error)throw result.error;return result;
}
let result=run("Report");assert.equal(result.status,0,result.stderr);assert(fs.existsSync(retained));assert(!fs.existsSync(extension));
result=run("Restore");assert.equal(result.status,0,result.stderr);assert(fs.lstatSync(extension).isSymbolicLink());assert(!fs.existsSync(retained));
assert.equal(fs.realpathSync(extension),fs.realpathSync(path.join(repo,"web_panel")));
result=run("Disable");assert.notEqual(result.status,0,"missing actual qualification refuses retirement");assert(fs.existsSync(extension));
const report=path.join(fixture,"offline-report.json");fs.writeFileSync(report,JSON.stringify({host:"fake",kind:"offline",gates:{}}));
result=run("Disable",["-QualificationReport",report]);assert.notEqual(result.status,0,"offline results never qualify host UI");assert(fs.existsSync(extension));
result=run("Restore");assert.notEqual(result.status,0,"refuse overwriting an active registration");assert(fs.existsSync(extension));
const panelRoot=path.join(repo,"web_panel");
function snapshot(directory,relative="") {
    return fs.readdirSync(directory,{withFileTypes:true}).flatMap(entry=>{
        const name=relative+entry.name,full=path.join(directory,entry.name);
        return entry.isDirectory()?snapshot(full,name+"/"):[{path:name,sha256:crypto.createHash("sha256").update(fs.readFileSync(full)).digest("hex").toUpperCase()}];
    });
}
const accepted={kind:"actual-ae-webview2-owner-acceptance",host:"After Effects 2023.5.0 Build 52",acceptedBy:"project-owner",
    retirementApproved:true,ownerStatement:"Synthetic schema fixture, not installed host qualification",panelFiles:snapshot(panelRoot),
    nativeHostSha256:crypto.createHash("sha256").update(fs.readFileSync(path.join(repo,"dist/StarfieldHost.aex"))).digest("hex").toUpperCase()};
function acceptedRun(value){fs.writeFileSync(report,JSON.stringify(value));return run("Disable",["-QualificationReport",report]);}
result=acceptedRun({...accepted,retirementApproved:false});assert.notEqual(result.status,0,"missing owner approval rejects");
result=acceptedRun({...accepted,nativeHostSha256:"0".repeat(64)});assert.notEqual(result.status,0,"different native host rejects");
result=acceptedRun({...accepted,panelFiles:accepted.panelFiles.slice(1)});assert.notEqual(result.status,0,"incomplete UI snapshot rejects");
const altered=JSON.parse(JSON.stringify(accepted));altered.panelFiles[0].sha256="0".repeat(64);
result=acceptedRun(altered);assert.notEqual(result.status,0,"different accepted UI bytes reject");assert(fs.existsSync(extension));
result=acceptedRun(accepted);assert.equal(result.status,0,result.stderr);assert(!fs.existsSync(extension));assert(fs.existsSync(retained));
result=run("Restore");assert.equal(result.status,0,result.stderr);assert(!fs.existsSync(retained));assert.equal(fs.realpathSync(extension),fs.realpathSync(panelRoot));
fs.unlinkSync(extension);fs.symlinkSync(path.join(repo,"cep_panel"),retained,"junction");
result=run("Restore");assert.equal(result.status,0,result.stderr);assert.equal(fs.realpathSync(extension),fs.realpathSync(panelRoot),"legacy source path is rebound to web_panel on explicit restore");
fs.unlinkSync(extension);fs.symlinkSync(path.join(repo,"cep_panel"),extension,"junction");
result=acceptedRun(accepted);assert.equal(result.status,0,result.stderr);assert(fs.lstatSync(retained).isSymbolicLink(),"retire the already-dangling historical registration without traversing its target");
result=run("Restore");assert.equal(result.status,0,result.stderr);assert.equal(fs.realpathSync(extension),fs.realpathSync(panelRoot));
console.log("CEP retirement: report, owner/hash guards, exact Junction restore and legacy-path rebind passed in checkout-only fixture.");
