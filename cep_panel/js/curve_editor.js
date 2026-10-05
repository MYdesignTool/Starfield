(function(root){
    "use strict";
    function copy(points){var result=points.map(function(p){return {age:p.age,value:p.value};});result.interpolation=points.interpolation||0;return result;}
    function sample(points,age){
        age=Math.max(0,Math.min(1,age));if(age<=points[0].age)return points[0].value;
        var i=1;while(i<points.length-1 && age>points[i].age)i++;
        var a=points[i-1],b=points[i],t=(age-a.age)/(b.age-a.age),mode=points.interpolation||0;
        if(mode===1)return age<b.age?a.value:b.value;
        if(mode!==2)return a.value+(b.value-a.value)*t;
        function secant(a,b){return (points[b].value-points[a].value)/(points[b].age-points[a].age);}
        function slope(index){
            if(index===0)return secant(0,1);if(index===points.length-1)return secant(index-1,index);
            var left=secant(index-1,index),right=secant(index,index+1);if(left*right<=0)return 0;
            var h0=points[index].age-points[index-1].age,h1=points[index+1].age-points[index].age,w0=2*h1+h0,w1=h1+2*h0;
            return (w0+w1)/(w0/left+w1/right);
        }
        var t2=t*t,t3=t2*t,h=b.age-a.age,value=(2*t3-3*t2+1)*a.value+(t3-2*t2+t)*h*slope(i-1)+(-2*t3+3*t2)*b.value+(t3-t2)*h*slope(i);
        return Math.max(Math.min(a.value,b.value),Math.min(Math.max(a.value,b.value),value));
    }
    function dense(points){var result=[];for(var i=0;i<64;i++)result.push({age:i/63,value:sample(points,i/63)});result.interpolation=3;return result;}
    function simplify(points){
        var minimum=points[0].value,maximum=minimum,retained=[],i;
        for(i=0;i<points.length;i++){minimum=Math.min(minimum,points[i].value);maximum=Math.max(maximum,points[i].value);retained.push(false);}
        var tolerance=Math.max(1e-6,(maximum-minimum)*0.04),count=2;
        retained[0]=retained[points.length-1]=true;
        while(count<12){
            var candidate=points.length,left=0,error=tolerance;
            for(var right=1;right<points.length;right++){
                if(!retained[right])continue;
                var a=points[left],b=points[right];
                for(i=left+1;i<right;i++){
                    var point=points[i],amount=(point.age-a.age)/(b.age-a.age),deviation=Math.abs(point.value-(a.value+(b.value-a.value)*amount));
                    if(deviation>error){candidate=i;error=deviation;}
                }
                left=right;
            }
            if(candidate===points.length)break;
            retained[candidate]=true;count++;
        }
        var result=[];for(i=0;i<points.length;i++)if(retained[i])result.push({age:points[i].age,value:points[i].value});
        result.interpolation=0;return result;
    }
    function cycle(points){
        var mode=points.interpolation||0,result;
        if(mode===3)return simplify(points);
        var next=(mode+1)%4;if(next===3)return dense(points);
        result=copy(points);result.interpolation=next;return result;
    }
    function stroke(points,last,age,value){
        var next=Math.max(0,Math.min(points.length-1,Math.round(age*(points.length-1)))),previous=points[last].value;
        for(var i=Math.min(last,next);i<=Math.max(last,next);i++)points[i].value=last===next?value:previous+(value-previous)*(i-last)/(next-last);
        return next;
    }
    function presets(){return root.StarfieldEditorPresets.curves.map(function(p){var points=p.points.map(function(v){return {age:v[0],value:v[1]};});points.interpolation=p.interpolation;return {name:p.name,points:points};});}
    root.StarfieldCurveTools={copy:copy,sample:sample,dense:dense,cycle:cycle,stroke:stroke,presets:presets,modes:["Linear","Hold","Bezier","Draw"]};
}(window));
