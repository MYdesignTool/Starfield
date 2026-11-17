(function(root,factory){
    var api=factory();
    if(typeof module==="object" && module.exports)module.exports=api;
    else root.StarfieldNodeShake=api;
}(typeof window!=="undefined"?window:this,function(){
    "use strict";
    function create(){
        var originX=0,originY=0,axis=null,direction=0,peak=0,started=null,lastTime=null,reversals=0,latched=false;
        function reset(x,y,time){originX=x;originY=y;axis=null;direction=0;peak=0;started=null;lastTime=time;reversals=0;latched=false;}
        function sample(x,y,time){
            if(!isFinite(x) || !isFinite(y) || !isFinite(time))return false;
            if(latched)return false;
            if(lastTime===null || time<lastTime || (started!==null && time-started>700)){reset(x,y,time);return false;}
            lastTime=time;
            if(axis===null){
                var dx=x-originX,dy=y-originY;
                if(Math.max(Math.abs(dx),Math.abs(dy))<12)return false;
                axis=Math.abs(dx)>=Math.abs(dy)?"x":"y";
                var delta=axis==="x"?dx:dy;direction=delta>0?1:-1;peak=axis==="x"?x:y;started=time;return false;
            }
            var value=axis==="x"?x:y;
            if((direction>0 && value>peak) || (direction<0 && value<peak)){peak=value;return false;}
            if(direction*(peak-value)<12)return false;
            direction=-direction;peak=value;reversals++;
            if(reversals<4)return false;
            latched=true;return true;
        }
        return {reset:reset,sample:sample,triggered:function(){return latched;}};
    }
    return {create:create};
}));
