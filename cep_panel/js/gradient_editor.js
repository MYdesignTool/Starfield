(function (root) {
    "use strict";
    var clipboard=null;
    function copy(stops) {return stops.map(function(s){return {position:s.position,color:s.color.slice()};});}
    function valid(stops) {
        if(!Array.isArray(stops) || stops.length<2 || stops.length>8 || stops[0].position!==0 || stops[stops.length-1].position!==1)return false;
        return stops.every(function(s,i){return typeof s.position==="number" && isFinite(s.position) &&
            (!i || s.position>stops[i-1].position) && Array.isArray(s.color) && s.color.length===3 &&
            s.color.every(function(c){return typeof c==="number" && isFinite(c) && c>=0 && c<=1;});});
    }
    var presets=[
        {name:"White",stops:[{position:0,color:[1,1,1]},{position:1,color:[1,1,1]}]},
        {name:"Fire",stops:[{position:0,color:[1,0.12,0.02]},{position:0.35,color:[1,0.6,0.05]},
            {position:0.7,color:[1,0.9,0.3]},{position:1,color:[1,1,0.9]}]},
        {name:"Spectrum",stops:[{position:0,color:[1,0.4,0.08]},{position:0.25,color:[0.08,0.6,0.4]},
            {position:0.5,color:[0.5,1,0.7]},{position:0.75,color:[0.92,1,0.62]},{position:1,color:[0.78,0.62,0.35]}]}
    ];
    root.StarfieldGradientTools={
        valid:valid,
        flip:function(stops){return valid(stops)?copy(stops).reverse().map(function(s){s.position=1-s.position;return s;}):null;},
        copy:function(stops){if(!valid(stops))return false;clipboard=copy(stops);return true;},
        paste:function(){return clipboard?copy(clipboard):null;},
        presets:function(){return presets.map(function(p){return {name:p.name,stops:copy(p.stops)};});}
    };
}(window));
