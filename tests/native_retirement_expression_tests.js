"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const parser=require("internal/deps/acorn/acorn/dist/acorn");
const header=fs.readFileSync(path.join(__dirname,"../ae_plugin/NativeRetiredAlias.hpp"),"utf8");
const expressions=[...header.matchAll(/std::u16string_view [ab] =\s*((?:u"(?:[^"\\]|\\.)*"\s*)+);/g)].map(match=>
    [...match[1].matchAll(/u("(?:[^"\\]|\\.)*")/g)].map(part=>JSON.parse(part[1])).join(""));
assert.equal(expressions.length,2);assert.notEqual(expressions[0],expressions[1]);
for(const expression of expressions){
    assert.ok(expression.startsWith("var result = -1099511627776; var fx = null; var match = false;"),"old126 native ownership prefix remains recognizable");
    parser.parse(expression,{ecmaVersion:3,allowReserved:"never"});
    for(const value of [0,7.25,-2,1e30])assert.equal(vm.runInNewContext(expression,{value}),value);
    assert.doesNotMatch(expression,/thisComp|thisLayer|\.param\(|\.effect\(/,"retirement owns no source dependency");
}
console.log("native retirement expressions: ES3, exact zero/finite handoff and126 prefix passed (not AE qualification)");
