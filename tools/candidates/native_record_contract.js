// One ES3 factory embedded verbatim in both CEP planning and the AE gateway.
// Portable control layouts only: native disk IDs/ABI/defaults stay elsewhere.
function createNativeRecordContract() {
    var prefix="org.starfieldfx.nodes.",specs={
        emitter:{version:7,required:{"2":4,"3":3,"5":3,"6":5,"7":5,"8":4,"9":4,"10":4,"11":4,"12":4,
            "14":4,"15":4,"16":4,"17":3,"18":4,"19":4,"20":4,"21":4,"22":4,"23":3,
            "24":4,"25":4,"26":4,"27":4,"28":4,"29":4,"30":4,"31":3,"32":5},optional:{}},
        particle:{version:7,required:{"1":5,"3":4,"4":4,"5":4,"6":4,"9":4,"10":4,"11":4,"12":3,"13":7,
            "14":4,"15":3,"16":4,"17":3,"18":5,"19":4,"20":5,"21":4,"22":3,"23":4,
            "24":3,"25":3,"26":4,"28":4,"29":4},optional:{"7":7,"8":7,"27":7,"30":3,"31":3,
            "32":3,"33":3,"34":3,"35":3,"36":3,"37":3,"38":4,"39":4,"40":2,"41":4}},
        force:{version:3,required:{"1":5,"2":4,"3":4,"4":5,"5":4,"6":4,"7":4,"8":4},optional:{"9":7}},
        transform:{version:1,required:{"1":5,"2":5,"3":5,"4":5,"5":4,"6":4},optional:{"7":7,"8":3}},
        motion:{version:1,required:{"1":3,"2":5,"3":5,"4":4,"5":4,"11":4,"12":3,"13":3,"14":5,"15":5,"17":4,"18":3,"19":6,"20":4},optional:{"6":7,"8":5}},
        turbulence:{version:1,required:{"1":5,"2":4,"3":4,"4":3},optional:{"5":4,"6":5,"7":7,"8":3,"9":3,"10":3,"11":4,"12":4,"13":4,"14":3,"15":4,"16":4,"17":4,"18":4,"19":4,"20":4,"21":4}},
        material:{version:1,required:{"1":3,"2":5,"3":4,"4":4,"5":4,"6":3},optional:{}},
        model:{version:1,required:{"1":7,"2":3,"4":5,"5":5,"6":5,"7":3,"8":3,"9":3,"10":3,"11":3,"12":7,"13":3},optional:{}}
    };
    function own(object,key){return Object.prototype.hasOwnProperty.call(object,key);}
    function array(value){return Object.prototype.toString.call(value)==="[object Array]";}
    function integer(value,min,max){return typeof value==="number" && isFinite(value) && Math.floor(value)===value && value>=min && value<=max;}
    function spec(type){if(typeof type!=="string" || type.substr(0,prefix.length)!==prefix)return null;
        var kind=type.substr(prefix.length);if(!own(specs,kind))return null;return specs[kind];}
    function invalid(node,detail){var error=new Error("Native node "+(node && node.id || "missing")+" ("+
        (node && node.type || "missing")+") control record: "+detail+".");error.code="invalid_native_node_record";throw error;}
    function wire(field){var value=field.value;
        if(field.type===2)return integer(value,-2147483648,2147483647);
        if(field.type===3)return integer(value,0,4294967295);
        if(field.type===4)return typeof value==="number" && isFinite(value);
        if(field.type===5){if(!array(value) || value.length!==3)return false;
            for(var axis=0;axis<3;axis++)if(typeof value[axis]!=="number" || !isFinite(value[axis]))return false;return true;}
        if(field.type===6){if(typeof value!=="string" || value.indexOf("\u0000")>=0)return false;
            try{return encodeURIComponent(value).replace(/%[0-9a-f]{2}/gi,"x").length<=128;}catch(ignored){return false;}}
        if(field.type===7){var tag=Object.prototype.toString.call(value);
            if(!array(value) && tag!=="[object Uint8Array]")return false;
            if(!integer(value.length,0,24576))return false;
            for(var byteIndex=0;byteIndex<value.length;byteIndex++)if(!integer(value[byteIndex],0,255))return false;return true;}
        return false;
    }
    function schema(node){var layout=node && spec(node.type),found={};
        if(!layout)invalid(node,"unsupported node type");
        if(!integer(node.schemaVersion,1,layout.version))invalid(node,"unsupported schemaVersion "+node.schemaVersion);
        if(!array(node.parameters) || node.parameters.length>64)invalid(node,"missing or oversized parameters");
        for(var p=0;p<node.parameters.length;p++){
            var field=node.parameters[p];if(!field || !/^[1-9][0-9]*$/.test(String(field.key)))invalid(node,"invalid parameter identity");
            var key=String(field.key),type=0;
            if(own(layout.required,key))type=layout.required[key];else if(own(layout.optional,key))type=layout.optional[key];
            if(!type || found[key] || field.type!==type || !wire(field))invalid(node,"invalid parameter "+key+" type, identity or value");
            if(node.type===prefix+"motion") {
                if(key==="1" && !integer(field.value,0,2))invalid(node,"unsupported Motion mode");
                if(key==="13" && !integer(field.value,0,2))invalid(node,"invalid Circle Origin Type");
                if(key==="5" && (field.value<0 || field.value>100))invalid(node,"invalid Circle Speed Random");
                if(key==="4" && Math.abs(field.value)>100000*(2*Math.PI/100))invalid(node,"Circle Speed outside native range");
                if(key==="3" && (field.value[0]!==0 || field.value[1]!==0 || field.value[2]!==1))invalid(node,"Circle axis must be authored through Angle XYZ");
                if(key==="14")for(var axis=0;axis<3;axis++)if(Math.abs(field.value[axis])>=32768)invalid(node,"Circle angle outside native range");
                if(key==="11" && (field.value<0 || field.value>1000000))invalid(node,"invalid Light Path Max Delay");
                if(key==="12" && !integer(field.value,0,1))invalid(node,"invalid Light Path Orient To Path");
                if(key==="17" && (field.value<0 || field.value>100000))invalid(node,"invalid Light Path Speed");
                if(key==="18" && !integer(field.value,0,1))invalid(node,"invalid Light Path end behavior");
                if(key==="20" && (field.value<0 || field.value>100))invalid(node,"invalid Light Path Delay Random");
            }
            if(node.type===prefix+"material") {
                if((key==="1" || key==="6") && !integer(field.value,0,1))invalid(node,"material switch "+key+" must be 0 or 1");
                if(key==="2")for(var channel=0;channel<3;channel++)
                    if(field.value[channel]<0 || field.value[channel]>1)invalid(node,"material color must be in 0..1");
                if((key==="3" || key==="4" || key==="5") && (field.value<0 || field.value>1))
                    invalid(node,"material parameter "+key+" must be in 0..1");
            }
            if(node.type===prefix+"turbulence") {
                if(key==="2" && (field.value<0 || field.value>100000))invalid(node,"Noise Amplitude outside supported range");
                if(key==="3" && (field.value<=0 || field.value>100000))invalid(node,"Noise frequency outside supported range");
                if(key==="4" && !integer(field.value,1,8))invalid(node,"Noise Levels must be 1..8");
                if(key==="8" && [0,3,4].indexOf(field.value)<0)invalid(node,"unsupported Turbulence Type");
                if(key==="9" && !integer(field.value,0,3))invalid(node,"invalid Turbulence axis");
                if((key==="10" || key==="14") && !integer(field.value,0,1))invalid(node,"invalid Turbulence selector");
                if(key==="11" && (field.value<=0 || field.value>100000000))invalid(node,"invalid Turbulence sphere size");
                if((key==="12" || key==="13") && (field.value<0 || field.value>100))invalid(node,"Turbulence amount outside 0..100");
                if(key>="15" && key<="19" && (field.value < -100 || field.value > 100))invalid(node,"Turbulence property offset outside -100..100");
                if(key==="20" && (field.value<=0 || field.value>100000))invalid(node,"invalid Noise Scale");
                if(key==="21" && Math.abs(field.value)>100000)invalid(node,"invalid Fractal Evolution");
            }
            found[key]=true;
        }
        for(var key in layout.required)if(own(layout.required,key) && !found[key])invalid(node,"missing parameter "+key);
        return layout.version;
    }
    return {id:"native-control-contract-1",array:array,schema:schema,version:function(type){var layout=spec(type);
        if(!layout)invalid({type:type},"unsupported node type");return layout.version;}};
}
