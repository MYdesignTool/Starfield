"use strict";
const assert=require("node:assert/strict"),fs=require("node:fs"),path=require("node:path"),vm=require("node:vm");
const repo=path.join(__dirname,".."),panel=process.env.STARFIELD_PANEL_ROOT || path.join(repo,"cep_panel");
let checks=0;function eq(a,b){checks++;assert.deepEqual(a,b);}function ok(value){checks++;assert.ok(value);}
const schema=JSON.parse(fs.readFileSync(path.join(repo,"schema/parameters.json"),"utf8"));
eq(schema.manifestVersion,29);
eq(schema.topics.map(t=>[t.label,t.index,t.endIndex]),[["Time Remapping",90,93],["Render Settings",94,97],
    ["Motion Blur",610,619],["Simulation Settings",620,622],["GPU Rendering",623,625]]);
for(const [id,disk,index] of [[611,1601,621],[614,1611,624],...[617,618,619,620,621,622,623,624].map(id=>[id,1641+id-617,id-6])]){
    const p=schema.parameters.find(p=>p.id===id);eq([p.diskId,p.index],[disk,index]);
}
const remap=schema.parameters.find(p=>p.diskId===921);eq([remap.id,remap.label,remap.default],[91,"On / Off",0]);
eq([schema.presetLauncher.index,schema.presetLauncher.diskId,schema.presetLauncher.uiHeight],[1,1631,130]);
eq(schema.presetLauncher.actions,["Panel: Click To Open","Presets: Browse"]);
const header=fs.readFileSync(path.join(repo,"ae_plugin/MotionBlur.hpp"),"utf8");
eq(header.match(/kMotionParameterIds\{([^}]+)\}/)[1].split(",").map(Number),[611,612,613,614,615,616,617,618]);
const params=fs.readFileSync(path.join(repo,"ae_plugin/Parameters.cpp"),"utf8");
ok(params.indexOf("append_motion_parameters(in_data,out_data)")<params.indexOf('PF_ADD_TOPIC("Simulation Settings"'));
ok(params.indexOf('PF_ADD_TOPIC("Simulation Settings"')<params.indexOf('PF_ADD_TOPIC("GPU Rendering"'));
ok(params.includes('PF_ADD_CHECKBOX("On / Off", "", FALSE, PF_ParamFlag_SUPERVISE, 921)'));
const gateway=fs.readFileSync(path.join(panel,"jsx/starfield_gateway.jsx"),"utf8");
const context={JSON,Math,Date,isFinite,$:{global:{}}};vm.createContext(context);
vm.runInContext(gateway.replace("host.SFLD_getState = SFLD_getState;",
    "host.mainFixture={read:readRendererRecord,write:writeRendererRecord,time:timeRemapControl};host.SFLD_getState = SFLD_getState;"),context);
const fixture=context.$.global.mainFixture;ok(fixture);
function makeEffect(label="On / Off",disk=true){
    const properties=[];function add(name,value,matchName=name){const p={name,matchName,value,numProperties:0,numKeys:0,
        setValue(v){this.value=v;},setValueAtTime(t,v){this.value=v;this.lastTime=t;},property(){return null;}};properties.push(p);return p;}
    for(const [name,value] of Object.entries({"Max Particles":1000000,"Layout Output X":100,"Layout Output Y":200,
        "Time (Seconds)":2,"Preview":0,"Particle chance":100,"Time Sampling":1,"Acceleration":1}))add(name,value);
    const time=add(label,1,disk?"org.starfieldfx.particle-0921":label);
    for(let i=0;i<8;i++)add("Motion "+i,[2,360,0,1,8,70,0,1][i],"org.starfieldfx.particle-"+(1641+i));
    return {time,properties,get numProperties(){return properties.length;},property(key){return typeof key==="number"?properties[key-1]:
        properties.find(p=>p.name===key || p.matchName===key) || null;}};
}
for(const [label,disk] of [["On / Off",true],["Time Remapping On / Off",false]]){
    const effect=makeEffect(label,disk),resolved={target:{effect,comp:{time:3.5}}};
    eq(fixture.time(effect),effect.time);const record=JSON.parse(JSON.stringify(fixture.read(resolved)));
    eq([record.timeRemapEnabled,record.timeRemapSeconds,record.motionBlur,record.motionBlurType],[1,2,1,0]);
    effect.time.numKeys=1;record.timeRemapEnabled=0;record.timeRemapSeconds=6;record.motionBlurType=1;
    fixture.write(resolved,record);eq([effect.time.value,effect.time.lastTime],[0,3.5]);
    const roundtrip=JSON.parse(JSON.stringify(fixture.read(resolved)));eq([roundtrip.timeRemapEnabled,roundtrip.timeRemapSeconds,roundtrip.motionBlurType],[0,6,1]);
}
const effect=makeEffect();effect.properties.unshift({name:"On / Off",matchName:"unrelated",value:0,numProperties:0,property(){return null;}});
eq(fixture.time(effect),effect.time);
effect.properties.splice(effect.properties.indexOf(effect.time),1);effect.properties.shift();
checks++;assert.throws(()=>fixture.time(effect),/disk921/);
console.log(`main_order_tests: ${checks} checks passed; saved AE2023 migration remains open`);
