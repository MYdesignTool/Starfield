"use strict";

// AE ordinary properties, with disk-ID aliases for topic controls whose display
// labels duplicate scalar labels. All values remain mutable; locators are not.
const schema = require("../schema/parameters.json");
const clone = v => Array.isArray(v) ? v.slice() : v;
const control = (name, value, matchName) => ({name, value: clone(value), matchName});
function mainControls() {
    const properties = {};
    for (const p of schema.parameters) {
        const c = control(p.label, p.default === undefined ? 0 : p.default, "org.starfieldfx.particle-" + (p.diskId || p.id));
        c.propertyIndex = p.id;
        properties[p.id] = properties[p.label] = properties[c.matchName] = c;
    }
    properties.Origin.value = [1920,1080,1080];
    return properties;
}
function nodeControls() {
    const values = {
        "Type":1,"Particles Per Second":100,"Random Seed":1000,"Particle Size":10,
        "Opacity":100,"Origin XY":[1920,1080],"Origin Z":0,"Velocity X":0,"Velocity Y":0,"Velocity Z":0,
        "Disc Size":.05,"Velocity Random":0,"Size X":100,"Size Y":100,"Size Z":100,
        "Speed":100,"Speed Random":0,"Angle X":0,"Angle Y":0,"Angle Z":0,"Direction":2,"Direction Span":60,
        "Emitting":1,"Auxiliary Source":0,"Emit Chance":100,"Emit Life Start":0,"Emit Life End":100,
        "Inherit Velocity":0,"Inherit Size":0,"Inherit Opacity":0,"Inherit Color":0,"Orient X":0,"Orient Y":0,"Orient Z":0,
        "Color":[1,1,1,1],"Particle Color":1,"Size (Pixels)":10,"Size Y (Pixels)":10,
        "Size Over Life":100,"Opacity Over Life":100,"Life (Seconds)":2,"Life Random":0,"Shape":1,
        "Size Random":0,"Opacity Random":0,"Particle Feather":0,"Up Axis":3,"Orient To":1,
        "Angle Random":0,"Rotation Speed Random":0,"Speed X":0,"Speed Y":0,"Speed Z":0,
        "Limit To 2D":0,"Random Limit":1,"Limit Angle":0,"Anchor X (Percent)":50,"Anchor Y (Percent)":50,
        "Gravity":0,"Air Density":0,"Gravity random":0,"Wind X":0,"Wind Y":0,"Wind Z":0,
        "Spin":0,"Spin Frequency":0,"Spin resist":0,"Spin Delay (Seconds)":0,
        "Color Gradient":2,"Color Gradient Interpolation":0,"Panel Sync Guard":0,
        "Node Layout X":0,"Node Layout Y":0,"Outgoing Connection Count":0
    };
    for(let i=0;i<8;i++) values["Node UUID "+i]=0;
    for(let s=0;s<4;s++) for(let i=0;i<8;i++) {
        values["Connection "+s+" Target UUID "+i]=0;values["Connection "+s+" Edge UUID "+i]=0;
    }
    for(let i=0;i<8;i++) {
        values["Color Gradient "+i+" Position"]=i === 1 ? 100 : 0;
        values["Color Gradient "+i+" Color"]=[1,1,1,1];
    }
    const properties=Object.fromEntries(Object.entries(values).map(([name,value])=>[name,control(name,value)]));
    function disk(name,value,id) {
        const c=control(name,value,"org.starfieldfx.node.particle-"+id);
        properties[c.matchName]=properties["org.starfieldfx.node.particle-"+String(id).padStart(4,"0")]=c;
        return c;
    }
    const opacity=properties.Opacity;
    opacity.matchName="org.starfieldfx.node.particle-204";
    properties[opacity.matchName]=opacity;
    for(const [label,count,age,value,extraAge,extraValue,mode] of [
        ["Size",700,710,720,3000,3100,3610],["Opacity",800,810,820,3200,3300,3611],
        ["Rotation",960,970,980,3400,3500,3612],["Wind and Spin",900,910,920,3700,3800,3614]]) {
        const effect=label==="Wind and Spin"?"org.starfieldfx.node.force":"org.starfieldfx.node.particle";
        for(const [name,val,id] of [[label+" Curve Count",0,count],[label+" Curve Interpolation",0,mode],
            ...Array.from({length:64},(_,i)=>[label+" Curve "+i+" Age",i===63?1:0,i<8?age+i:extraAge+i-8]),
            ...Array.from({length:64},(_,i)=>[label+" Curve "+i+" Value",label==="Rotation"?0:100,i<8?value+i:extraValue+i-8])]) {
            const c=disk(name,val,id);
            if(effect.endsWith("force")) {
                c.matchName=effect+"-"+id;
                properties[c.matchName]=properties[effect+"-"+String(id).padStart(4,"0")]=c;
            }
            properties[name]=c;
        }
    }
    return properties;
}
module.exports={mainControls,nodeControls,control,clone};
