"use strict";
// Use Node's bundled Acorn without adding a dependency or vendoring its sources.
// Run: node --expose-internals tests/extendscript_syntax_tests.js [panel root]
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path");
const parser=require("internal/deps/acorn/acorn/dist/acorn"),options={ecmaVersion:3,allowReserved:"never"};
const root=path.resolve(process.argv[2] || process.env.STARFIELD_PANEL_ROOT || path.join(__dirname,"../cep_panel"));
let checks=0;
for(const source of ["var byte=0;","for(var byte=0;byte<16;byte++){}","var native;","var enum;",
    "var public;","var x={default:1};","let x=0;","const x=0;","var f=()=>1;","var s=`text`;"]){
    assert.throws(()=>parser.parse(source,options),undefined,source);checks++;
}
for(const source of ["var byteIndex=0;","var x={'byte':1}; x['byte'];","// byte native class\nvar text='byte';",
    "(function(){var values=[];for(var i=0;i<16;i++)values.push(i);}());"]){parser.parse(source,options);checks++;}
const jsxRoot=path.join(root,"jsx");
const files=fs.readdirSync(jsxRoot).filter(name=>name.endsWith(".jsx")).sort();
assert.ok(files.length>0);checks++;
for(const name of files){
    try{parser.parse(fs.readFileSync(path.join(jsxRoot,name),"utf8"),options);checks++;}
    catch(error){throw new Error(name+": "+error.message);}
}
console.log(`extendscript_syntax_tests: ${checks} checks passed; ${files.length} complete JSX files parse as ES3 (Acorn ${parser.version}); AE execution remains a host gate`);
