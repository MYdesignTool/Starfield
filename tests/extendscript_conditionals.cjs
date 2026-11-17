"use strict";
// Narrow compatibility check/model for reported ExtendScript conditional and
// mixed logical operator association. Parentheses are authoritative boundaries.
// This is not an ExtendScript engine or evidence of actual host execution.
const parser=require("internal/deps/acorn/acorn/dist/acorn");
const options={ecmaVersion:3,allowReserved:"never",preserveParens:true,locations:true};
function children(node){return Object.values(node).flatMap(value=>Array.isArray(value)?value:[value])
    .filter(value=>value && typeof value.type==="string" && typeof value.start==="number")
    .sort((a,b)=>a.start-b.start);}
function ungrouped(source){
    const result=[];
    function visit(node){
        if(node.type==="ConditionalExpression")for(const key of ["consequent","alternate"])
            if(node[key].type==="ConditionalExpression")result.push(node[key]);
        children(node).forEach(visit);
    }
    visit(parser.parse(source,options));return result;
}
function ungroupedLogical(source){
    const result=[];
    function visit(node){
        // Modern JS binds && before ||. With equal precedence/left association,
        // only an ungrouped && on an OR's right changes the evaluation tree.
        if(node.type==="LogicalExpression" && node.operator==="||" &&
            node.right.type==="LogicalExpression" && node.right.operator==="&&")result.push(node);
        children(node).forEach(visit);
    }
    visit(parser.parse(source,options));return result;
}
function modelLeftAssociation(source){
    function render(node){
        if(node.type==="LogicalExpression"){
            const operands=[],operators=[];
            function flatten(part){
                if(part.type!=="LogicalExpression"){operands.push(part);return;}
                flatten(part.left);operators.push(part.operator);flatten(part.right);
            }
            flatten(node);
            let result=render(operands[0]);
            for(let i=0;i<operators.length;i++)result="("+result+" "+operators[i]+" "+render(operands[i+1])+")";
            return result;
        }
        if(node.type==="ConditionalExpression" && node.consequent.type==="ConditionalExpression")
            throw Error("Ungrouped true branch is unsupported by ExtendScript");
        if(node.type==="ConditionalExpression" && node.alternate.type==="ConditionalExpression"){
            let test=render(node.test),yes=render(node.consequent),tail=node.alternate;
            while(tail.type==="ConditionalExpression"){
                test="("+test+" ? "+yes+" : "+render(tail.test)+")";
                yes=render(tail.consequent);tail=tail.alternate;
            }
            return "("+test+" ? "+yes+" : "+render(tail)+")";
        }
        let start=node.start,result="";
        for(const child of children(node)){result+=source.slice(start,child.start)+render(child);start=child.end;}
        return result+source.slice(start,node.end);
    }
    return render(parser.parse(source,options));
}
module.exports={ungrouped,ungroupedLogical,modelLeftAssociation};
