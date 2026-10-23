/* This adapter replaces the host boundary; reused UI functions are inserted below. */
var NODE_WIDTH=110,NODE_HEIGHT=54,CANVAS_MIN_WIDTH=580,CANVAS_MIN_HEIGHT=320,ZOOM_MIN=.5,ZOOM_MAX=2;
var NODE_TYPES={emitter:true,particle:true,force:true,transform:true,motion:true,turbulence:true,model:true,material:true,output:true};
var DEFAULT_NODE_POSITIONS={},nodePositions={},canvasOffset={x:0,y:0},viewPan={x:0,y:0},zoom=1;
var dragState=null,marqueeState=null,panState=null,connectionState=null,edgePressState=null,contextEdge=null,contextGraphPoint=null;
var minimapPanState=null,minimapTransform=null,centerGraphOnNextRender=true,graphNodeElements={},suppressNextNodeClick=false;
var resizeUpdatePending=false,nodePalette=null,history=[],keyboardPort=null;
var state={revision:1,targetToken:'website-memory',nodes:[],edges:[],selectedNodeId:null,selectedNodeIds:{},pending:false,graphMode:true,topologyReady:true,layoutPersistence:true};
var elements={};
['banner','chain','workspace','graphScroll','graphViewport','graphMinimap','graphEdges','edgePaths','selectionBox','zoomReadout'].forEach(function(id){elements[id]=document.getElementById(id);});
elements.graphCanvas=document.querySelector('.graph-canvas');elements.contextMenu=document.getElementById('graphContextMenu');elements.disconnectContextAction=document.getElementById('disconnectContextAction');
var statusLine=document.getElementById('demoStatus'),undoButton=document.getElementById('demoUndo');
var serial=0;
function freshId(){serial++;return serial.toString(16).padStart(32,'0');}
function clone(graph){return window.StarfieldGraphCodec.fromHex(window.StarfieldGraphCodec.toHex(graph));}
function announce(message){statusLine.textContent=message;}
function showError(code,message){announce('未完成：'+message);}
function initialGraph(){
    var graph={version:1,nodes:[],edges:[],optionalRecords:[]};
    var compact=elements.graphScroll.clientWidth<440;
    var positions=compact?[[70,20],[70,110],[70,200],[70,290]]:[[65,35],[215,115],[365,35],[365,225]];
    ['emitter','particle','force','output'].forEach(function(kind,index){graph=window.StarfieldGraphEdits.apply(graph,{type:'addNode',nodeType:kind,position:{x:positions[index][0],y:positions[index][1]},layerHeightPixels:1080},freshId);});
    for(var i=1;i<4;i++)graph=window.StarfieldGraphEdits.apply(graph,{type:'connect',from:graph.nodes[i-1].id,to:graph.nodes[i].id},freshId);
    return graph;
}
function remember(){history.push(clone(state.graph));if(history.length>30)history.shift();undoButton.disabled=false;}
function adopt(graph){
    var view=window.StarfieldGraphView.project(graph,null,{width:1920,height:1080,pixelAspect:1});
    state.graph=graph;state.nodes=view.nodes;state.edges=view.edges;nodePositions=view.positions;state.revision++;
    render(state);if(nodePalette)nodePalette.refresh();
}
function applyTopologyEdit(edit){
    if(['addNode','connect','disconnect','deleteNodes','duplicateNodes','moveNodes','insertNode'].indexOf(edit.type)<0){announce('这里只体验节点与连线。');return;}
    var increase=edit.type==='addNode'?1:edit.type==='duplicateNodes'?(edit.nodeIds||[]).length:0;
    if(state.nodes.length+increase>24){announce('最多可体验 24 个节点，先删除一些节点再试。');return;}
    try{
        var next=window.StarfieldGraphEdits.apply(state.graph,edit,freshId);
        if(window.StarfieldGraphCodec.toHex(next)===window.StarfieldGraphCodec.toHex(state.graph))return;
        remember();adopt(next);
        announce(({addNode:'已添加节点。',connect:'已连接节点。',disconnect:'已断开连线。',deleteNodes:'已删除选中节点。',duplicateNodes:'已复制节点。',moveNodes:'已移动节点。',insertNode:'已将节点插入连线。'})[edit.type]+' 可继续编辑或撤销。');
    }catch(error){showError(error.code,error.message||String(error));}
}
function commitNodeLayout(){
    var positions={};state.nodes.forEach(function(node){var p=nodePositions[node.id];positions[node.id]={x:p.x,y:p.y};});
    applyTopologyEdit({type:'moveNodes',positions:positions});
}
function nodeSummary(){return 'UI PLAYGROUND';}
function announceSelection(id){var node=state.nodes.filter(function(n){return n.id===id;})[0];if(node)announce('已选择 '+node.label+' · 拖动移动，Delete 删除。');}
/* PANEL_UI_FUNCTIONS */
function render(current){
    renderPanel(current);
    Object.keys(graphNodeElements).forEach(function(id){
        var entry=graphNodeElements[id];
        [entry.input,entry.output,entry.modelInput].forEach(function(port){
            if(!port)return;
            var direction=port.getAttribute('data-port-direction');
            var label=entry.node.label+' '+id.slice(-3)+' '+(port===entry.modelInput?'模型输入口':direction==='out'?'输出口':'输入口');
            port.tabIndex=0;port.setAttribute('role','button');port.setAttribute('aria-label',label);
            port.addEventListener('keydown',function(event){
                if(event.key!=='Enter'&&event.key!==' ')return;
                event.preventDefault();event.stopPropagation();
                var key=port.getAttribute('data-port-key');
                if(!keyboardPort||keyboardPort.direction===direction){keyboardPort={id:id,direction:direction,key:key};announce('已选择连接口，再用 Tab 选择另一端并按 Enter；Esc 取消。');return;}
                var from=direction==='in'?keyboardPort.id:id,to=direction==='in'?id:keyboardPort.id;
                var outputPort=direction==='in'?keyboardPort.key:key,inputPort=direction==='in'?key:keyboardPort.key;
                keyboardPort=null;applyTopologyEdit({type:'connect',from:from,to:to,outputPort:outputPort,inputPort:inputPort});
            });
        });
    });
}
function undo(){if(!history.length)return;adopt(history.pop());undoButton.disabled=!history.length;announce('已撤销上一步。');}
function addNode(type,x,y){requestTopologyEdit({type:'addNode',nodeType:type,position:{x:x,y:y},layerHeightPixels:1080});}
document.getElementById('demoAdd').addEventListener('click',function(){
    var point=canvasPoint(elements.graphScroll.getBoundingClientRect().left+elements.graphScroll.clientWidth*.5,elements.graphScroll.getBoundingClientRect().top+elements.graphScroll.clientHeight*.5);
    addNode(document.getElementById('demoNodeType').value,point.x-canvasOffset.x-55,point.y-canvasOffset.y-27);
});
undoButton.addEventListener('click',undo);
document.getElementById('demoDelete').addEventListener('click',deleteSelection);
document.getElementById('demoReset').addEventListener('click',function(){remember();centerGraphOnNextRender=true;state.selectedNodeIds={};adopt(initialGraph());announce('已恢复初始画布。');});
elements.graphCanvas.addEventListener('contextmenu',showGraphContextMenu);elements.contextMenu.addEventListener('click',handleContextMenuAction);
nodePalette=window.StarfieldNodePalette.create({root:document.getElementById('nodePalette'),items:document.getElementById('nodePaletteItems'),toggle:document.getElementById('nodePaletteToggle'),viewport:elements.graphScroll,prepare:hideGraphContextMenu,
    guard:function(){return !state.pending&&!canvasInteractionActive(true)&&state.nodes.length<24?'memory-demo':null;},isBlocked:function(el){return !!closestElement(el,'.graph-minimap');},zoom:function(){return zoom;},resize:schedulePanelResize,
    add:function(type,x,y){var point=canvasPoint(x,y);addNode(type,point.x-canvasOffset.x-55,point.y-canvasOffset.y-27);}
});
elements.graphScroll.addEventListener('pointerdown',beginCanvasPan);elements.graphScroll.addEventListener('pointerdown',beginMarquee);
elements.graphScroll.addEventListener('auxclick',function(e){if(e.button===1)e.preventDefault();});
elements.graphMinimap.addEventListener('pointerdown',beginMinimapPan);
// Embedded pages keep ordinary wheel scrolling available to the surrounding website.
elements.graphScroll.addEventListener('wheel',function(event){if(event.ctrlKey||event.metaKey)zoomAt(event);},{passive:false});
window.addEventListener('pointermove',moveNodeDrag);
function finishPointer(event){endNodeDrag(event);if(nodePalette)nodePalette.refresh();}
window.addEventListener('pointerup',finishPointer);window.addEventListener('pointercancel',finishPointer);window.addEventListener('resize',fitDemo);
document.addEventListener('pointerdown',function(e){if(!elements.contextMenu.contains(e.target))hideGraphContextMenu();});
document.addEventListener('keydown',function(e){
    if(e.key==='Escape'){hideGraphContextMenu();nodePalette.cancel();keyboardPort=null;if(connectionState&&connectionState.snapPort)connectionState.snapPort.classList.remove('connection-snap');connectionState=null;renderEdges();return;}
    if(e.target.matches('input,select,textarea'))return;
    if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='z'){e.preventDefault();undo();}
    if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='d'){e.preventDefault();duplicateSelection(28,28);}
    if(e.key==='Delete'||e.key==='Backspace'){e.preventDefault();hideGraphContextMenu();deleteSelection();}
});
var zoomTools=document.createElement('span');zoomTools.className='demo-zoom';
['−','＋','适应'].forEach(function(text,i){var button=document.createElement('button');button.type='button';button.textContent=text;button.setAttribute('aria-label',['缩小画布','放大画布','适应画布'][i]);button.addEventListener('click',function(){
    if(i===2){fitDemo();return;}
    var bounds=elements.graphScroll.getBoundingClientRect();zoomAt({clientX:bounds.left+bounds.width/2,clientY:bounds.top+bounds.height/2,deltaY:i===0?1:-1,preventDefault:function(){}});
});zoomTools.appendChild(button);});document.querySelector('.graph-tools').appendChild(zoomTools);
function fitDemo(){
    if(!state.nodes.length)return;
    var positions=state.nodes.map(function(n){return nodePositions[n.id];});
    var width=Math.max.apply(null,positions.map(function(p){return p.x+NODE_WIDTH;}))-Math.min.apply(null,positions.map(function(p){return p.x;}));
    var height=Math.max.apply(null,positions.map(function(p){return p.y+NODE_HEIGHT;}))-Math.min.apply(null,positions.map(function(p){return p.y;}));
    zoom=Math.max(ZOOM_MIN,Math.min(1,(elements.graphScroll.clientWidth-32)/width,(elements.graphScroll.clientHeight-32)/height));
    centerGraphView();schedulePanelResize();
}
if(window.ResizeObserver)new ResizeObserver(fitDemo).observe(elements.graphScroll);
zoom=elements.graphScroll.clientWidth<440?.85:1;adopt(initialGraph());fitDemo();
