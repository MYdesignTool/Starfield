(function (root) {
    "use strict";
    var clipboard=null;
    function copy(stops) {var result=stops.map(function(s){return {position:s.position,color:s.color.slice()};});result.interpolation=stops.interpolation||0;return result;}
    function valid(stops) {
        if(!Array.isArray(stops) || stops.length<2 || stops.length>8 || (stops.interpolation!==undefined && stops.interpolation!==0 && stops.interpolation!==1))return false;
        return stops.every(function(s,i){return typeof s.position==="number" && isFinite(s.position) &&
            s.position>=0 && s.position<=1 && (!i || s.position>stops[i-1].position) && Array.isArray(s.color) && s.color.length===3 &&
            s.color.every(function(c){return typeof c==="number" && isFinite(c) && c>=0 && c<=1;});});
    }
    var presets=[
        {name:"White",stops:[{position:0,color:[1,1,1]},{position:1,color:[1,1,1]}]},
        {name:"Fire",stops:[{position:0,color:[1,0.12,0.02]},{position:0.35,color:[1,0.6,0.05]},
            {position:0.7,color:[1,0.9,0.3]},{position:1,color:[1,1,0.9]}]},
        {name:"Spectrum",stops:[{position:0,color:[1,0.4,0.08]},{position:0.25,color:[0.08,0.6,0.4]},
            {position:0.5,color:[0.5,1,0.7]},{position:0.75,color:[0.92,1,0.62]},{position:1,color:[0.78,0.62,0.35]}]}
    ];
    function sample(stops,position) {
        if(position<=stops[0].position)return stops[0].color.slice();
        for(var i=1;i<stops.length;i++)if(position<=stops[i].position) {
            var a=stops[i-1],b=stops[i],f=(position-a.position)/(b.position-a.position);
            if(stops.interpolation===1)return (position<b.position?a:b).color.slice();
            return a.color.map(function(c,ch){return c+(b.color[ch]-c)*f;});
        }
        return stops[stops.length-1].color.slice();
    }
    function insert(stops,position) {
        if(!valid(stops) || !isFinite(position) || stops.length>=8)return null;
        position=Math.max(0,Math.min(1,position));
        if(stops.some(function(s){return Math.abs(s.position-position)<0.001-1e-12;}))return null;
        var result=copy(stops),index=0;
        while(index<result.length && result[index].position<position)index++;
        result.splice(index,0,{position:position,color:sample(stops,position)});
        return {stops:result,index:index};
    }
    function move(stops,index,position) {
        if(!valid(stops) || !isFinite(position) || index<0 || index>=stops.length || Math.floor(index)!==index)return null;
        var result=copy(stops),stop=result.splice(index,1)[0],closest=2,chosen=stop.position;
        position=Math.max(0,Math.min(1,position));
        function consider(value) {
            if(value<0 || value>1 || result.some(function(s){return Math.abs(value-s.position)<0.001-1e-12;}))return;
            var distance=Math.abs(value-position);if(distance<closest){closest=distance;chosen=value;}
        }
        consider(position);consider(0);consider(1);
        result.forEach(function(s){consider(s.position-0.001);consider(s.position+0.001);});
        if(closest===2)return null;
        stop.position=chosen;var next=0;while(next<result.length && result[next].position<chosen)next++;
        result.splice(next,0,stop);return {stops:result,index:next};
    }
    root.StarfieldGradientTools={
        valid:valid,
        sample:sample,insert:insert,move:move,
        flip:function(stops){if(!valid(stops))return null;var result=copy(stops).reverse();result.forEach(function(s){s.position=1-s.position;});return result;},
        copy:function(stops){if(!valid(stops))return false;clipboard=copy(stops);return true;},
        paste:function(){return clipboard?copy(clipboard):null;},
        presets:function(){return root.StarfieldEditorPresets.gradients.map(function(p){var stops=p.stops.map(function(s){return {position:s[0],color:s.slice(1)};});stops.interpolation=p.interpolation;return {name:p.name,stops:stops};});}
    };
}(window));
