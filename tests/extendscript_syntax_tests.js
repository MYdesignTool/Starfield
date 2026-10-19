"use strict";
// Use Node's bundled Acorn without adding a dependency or vendoring its sources.
// Run: node --expose-internals tests/extendscript_syntax_tests.js [panel root]
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path");
const parser=require("internal/deps/acorn/acorn/dist/acorn"),options={ecmaVersion:3,allowReserved:"never"};
const conditionals=require("./extendscript_conditionals.cjs"),vm=require("node:vm");
const root=path.resolve(process.argv[2] || process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"../web_panel"));
let checks=0;
for(const source of ["var byte=0;","for(var byte=0;byte<16;byte++){}","var native;","var enum;",
    "var public;","var x={default:1};","let x=0;","const x=0;","var f=()=>1;","var s=`text`;"]){
    assert.throws(()=>parser.parse(source,options),undefined,source);checks++;
}
for(const source of ["var byteIndex=0;","var x={'byte':1}; x['byte'];","// byte native class\nvar text='byte';",
    "(function(){var values=[];for(var i=0;i<16;i++)values.push(i);}());"]){parser.parse(source,options);checks++;}
const jsxRoot=path.join(root,"jsx");
for(const source of ["a?b:c?d:e;","a?b?c:d:e;","a?b:\n c?d:e;"]){
    assert.ok(conditionals.ungrouped(source).length,source);checks++;
}
for(const source of ["a?b:(c?d:e);","a?(b?c:d):e;","a?call(b?c:d):e;"]){
    assert.equal(conditionals.ungrouped(source).length,0,source);checks++;
}
// Firsthand Adobe report + owner raw1/Core0: model only this association rule.
for(const [mode,expected] of [[1,0],[2,0],[3,2]]){
    const source="var mode="+mode+"; mode===1?1:mode===2?0:2;";
    assert.equal(vm.runInNewContext(conditionals.modelLeftAssociation(source)),expected);checks++;
}
assert.equal(vm.runInNewContext(conditionals.modelLeftAssociation("true?'A':false?'B':true?'C':'D';")),"C");checks++;
assert.equal(vm.runInNewContext(conditionals.modelLeftAssociation("true?'A':(false?'B':'C');")),"A");checks++;
const files=fs.readdirSync(jsxRoot).filter(name=>name.endsWith(".jsx")).sort();
parser.parse(fs.readFileSync(path.join(__dirname,"../tools/WebPanelQualification.jsx"),"utf8"),options);checks++;
assert.ok(files.length>0);checks++;
for(const name of files){
    try{const source=fs.readFileSync(path.join(jsxRoot,name),"utf8");parser.parse(source,options);checks++;
        const unsafe=conditionals.ungrouped(source);
        assert.equal(unsafe.length,0,name+": ungrouped nested conditional at lines "+unsafe.map(n=>n.loc.start.line).join(","));checks++;}
    catch(error){throw new Error(name+": "+error.message);}
}
console.log(`extendscript_syntax_tests: ${checks} checks passed; ${files.length} complete JSX files parse as ES3 with no ungrouped nested conditionals (Acorn ${parser.version}); AE execution remains a host gate`);
