"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const root=process.env.STARFIELD_PANEL_ROOT;if(!root)throw Error("Set STARFIELD_PANEL_ROOT to the isolated candidate.");
const source=fs.readFileSync(path.join(__dirname,"../tools/candidates/native_record_contract.js"),"utf8");
const context=vm.createContext({$:{global:{}}});vm.runInContext(source,context);const contract=vm.runInContext("createNativeRecordContract()",context);
const edits=require(path.join(root,"js/graph_edits.js")),copy=x=>JSON.parse(JSON.stringify(x,(_,v)=>ArrayBuffer.isView(v)?Array.from(v):v));
let checks=0;function eq(a,b){checks++;assert.deepEqual(a,b);}function bad(fn){checks++;assert.throws(fn,e=>e.code==="invalid_native_node_record");}
for(const name of ["js/graph_edits.js","jsx/starfield_gateway.jsx"]){const embedded=fs.readFileSync(path.join(root,name),"utf8");
    eq(embedded.split("// BEGIN STARFIELD NATIVE RECORD CONTRACT\n")[1].split("\n    var nativeRecords=")[0],source.trimEnd());}
vm.runInContext(fs.readFileSync(path.join(root,"jsx/starfield_gateway.jsx"),"utf8"),context);
const api=context.$.global.__SFLD_modelTransactionAPI;eq(api.recordContractId,contract.id);
const foreign=vm.createContext({});
const create=kind=>edits.apply({version:1,nodes:[],edges:[],optionalRecords:[]},{type:"addNode",nodeType:kind},()=>"1".repeat(32)).nodes[0];
for(const kind of ["emitter","particle","force","transform","model","material"]){const node=copy(create(kind));
    for(let label=1;label<=edits.schemaVersion(kind);label++){node.schemaVersion=label;
        for(const boundary of [contract.schema,api.recordSchema,edits.nativeRecordSchema])eq(boundary(node),edits.schemaVersion(kind));
        eq(contract.schema(vm.runInContext("("+JSON.stringify(node)+")",foreign)),edits.schemaVersion(kind));eq(node.schemaVersion,label);}
    for(const label of [0,-1,1.5,NaN,Infinity,"1",null,edits.schemaVersion(kind)+1])bad(()=>contract.schema({...node,schemaVersion:label}));
    for(let i=0;i<node.parameters.length;i++){
        const field=node.parameters[i],wrong=copy(node);wrong.parameters[i].type=99;bad(()=>contract.schema(wrong));
        wrong.parameters[i]=copy(field);wrong.parameters[i].value=field.type===5?[0,NaN,0]:field.type===7?[0,256]:NaN;bad(()=>contract.schema(wrong));
        wrong.parameters[i]=copy(field);wrong.parameters.push(copy(field));bad(()=>contract.schema(wrong));
    }
    for(const extra of [{key:"999",type:4,value:0},{key:"01",type:4,value:0},{key:"__proto__",type:4,value:0}])bad(()=>contract.schema({...node,parameters:node.parameters.concat([extra])}));
    bad(()=>contract.schema({...node,parameters:{length:0}}));bad(()=>contract.schema({...node,parameters:[]}));
    // No invented defaults or schema mutation on rejection.
    const raw=copy(node);bad(()=>contract.schema({...node,parameters:node.parameters.slice(1).concat(node.parameters[1])}));eq(node,raw);
}
bad(()=>contract.schema(copy(create("output"))));
eq(contract.array(vm.runInContext("[]",foreign)),true);eq(contract.array({length:0}),false);
console.log(`native_record_contract_tests: ${checks} checks passed; identical ES3 factory in browser/gateway, all six kinds, cross-realm data and strict wire/layout rejection`);
