// Pointer-based palette: local preview only until one valid canvas drop.
(function (root) {
    "use strict";
    var kinds = { emitter: "Emitter", auxiliary: "Auxiliary", particle: "Particle", force: "Force" };
    function create(options) {
        var palette=options.root, items=options.items, toggle=options.toggle;
        var viewport=options.viewport, doc=options.document || document, win=options.window || window;
        if (!palette || !items || !toggle || !viewport) return null;
        var buttons=items.querySelectorAll("[data-node-type]"), drag=null, collapsed=false;

        function stop(event) { if(event.preventDefault)event.preventDefault(); if(event.stopPropagation)event.stopPropagation(); }
        function entry(target) {
            while(target && target!==items) {
                if(target.getAttribute && kinds[target.getAttribute("data-node-type")])return target;
                target=target.parentNode;
            }
            return null;
        }
        function hit(x,y) {
            var bounds=viewport.getBoundingClientRect();
            if (!isFinite(x) || !isFinite(y) || x<bounds.left || y<bounds.top || x>=bounds.right || y>=bounds.bottom)return false;
            // A floating inspector/minimap above the canvas is not a drop surface.
            var surface=doc.elementFromPoint ? doc.elementFromPoint(x,y) : null;
            if(surface && options.isBlocked && options.isBlocked(surface))return false;
            return !surface || viewport.contains(surface);
        }
        function refresh() {
            var enabled=!!options.guard();
            for(var i=0;i<buttons.length;i++)buttons[i].disabled=!enabled;
        }
        function cancel() {
            if(!drag)return;
            var old=drag; drag=null;
            if(old.preview.parentNode)old.preview.parentNode.removeChild(old.preview);
            if(old.button.releasePointerCapture && old.pointerId!==undefined) {
                try { old.button.releasePointerCapture(old.pointerId); } catch(ignored) { /* already lost capture */ }
            }
            viewport.classList.remove("palette-drop-target");
            if(doc.body && doc.body.classList)doc.body.classList.remove("dragging-palette-node");
            refresh();
        }
        function move(event) {
            if(!drag || event.pointerId!==drag.pointerId)return;
            if(event.buttons===0) { cancel(); return; }
            if(options.guard()!==drag.guard) { cancel(); return; }
            stop(event);
            drag.lastX=event.clientX; drag.lastY=event.clientY;
            if(Math.abs(drag.lastX-drag.startX)+Math.abs(drag.lastY-drag.startY)>=4)drag.moved=true;
            var zoom=options.zoom ? options.zoom() : 1;
            drag.preview.style.left=(event.clientX-55*zoom)+"px";
            drag.preview.style.top=(event.clientY-27*zoom)+"px";
            drag.preview.style.transform="scale("+zoom+")";
            var valid=hit(event.clientX,event.clientY);
            drag.preview.classList.toggle("valid-drop",valid);
            viewport.classList.toggle("palette-drop-target",valid);
        }
        function begin(event) {
            var button=entry(event.target);
            if(!button || button.disabled || drag || collapsed || event.button!==0 || event.isPrimary===false)return;
            if(options.prepare)options.prepare();
            var guard=options.guard(); if(!guard)return;
            stop(event);
            var type=button.getAttribute("data-node-type"), preview=doc.createElement("div");
            preview.className="palette-node-preview kind-"+type;
            preview.textContent=kinds[type]; preview.setAttribute("aria-hidden","true");
            doc.body.appendChild(preview);
            drag={type:type,guard:guard,button:button,pointerId:event.pointerId,preview:preview,
                  startX:event.clientX,startY:event.clientY,lastX:event.clientX,lastY:event.clientY,moved:false};
            if(button.setPointerCapture && event.pointerId!==undefined) {
                try { button.setPointerCapture(event.pointerId); } catch(ignored) { /* window listeners still work */ }
            }
            if(doc.body && doc.body.classList)doc.body.classList.add("dragging-palette-node");
            move(event);
        }
        function end(event) {
            if(!drag || event.pointerId!==drag.pointerId)return;
            stop(event);
            var current=drag, valid=current.moved && options.guard()===current.guard && hit(event.clientX,event.clientY);
            cancel();
            if(valid)options.add(current.type,event.clientX,event.clientY);
        }
        function setCollapsed(value) {
            cancel(); collapsed=!!value;
            palette.classList.toggle("collapsed",collapsed); items.hidden=collapsed;
            toggle.setAttribute("aria-expanded",collapsed?"false":"true");
            toggle.setAttribute("aria-label",collapsed?"Expand node palette":"Collapse node palette");
            toggle.title=collapsed?"Expand nodes":"Collapse nodes";
            toggle.textContent=collapsed?"›":"‹";
            if(options.resize)options.resize();
        }
        items.addEventListener("pointerdown",begin);
        items.addEventListener("dragstart",function(event){event.preventDefault();});
        items.addEventListener("lostpointercapture",function(event){if(drag && event.pointerId===drag.pointerId)cancel();});
        toggle.addEventListener("click",function(){setCollapsed(!collapsed);});
        win.addEventListener("pointermove",move);
        win.addEventListener("pointerup",end);
        win.addEventListener("pointercancel",function(event){if(drag && event.pointerId===drag.pointerId)cancel();});
        win.addEventListener("blur",cancel);
        doc.addEventListener("visibilitychange",function(){if(doc.hidden)cancel();});
        doc.addEventListener("keydown",function(event){if(event.key==="Escape" && drag){stop(event);cancel();}});
        setCollapsed(false); refresh();
        return {isDragging:function(){return !!drag;},cancel:cancel,refresh:refresh};
    }
    root.StarfieldNodePalette={create:create};
}(typeof window!=="undefined"?window:this));
