/* Adapted from the owner's Ae-ToolKit glass implementation after reviewing its fixes.
 * Refs: 3481e25 (continuous reflections), abc04dd (bevel/reflection restoration).
 * No logo renderer, theme storage, navigation transition or host code is included. */
(function () {
  'use strict';
  const motion = window.matchMedia('(prefers-reduced-motion: reduce)');
  const surfaces = '.site-header,.feature,.graph-demo,.workflow-steps li,.start,.faq-list details';
  // One highlight update per frame, with no background timer or touch tracking.
  if (window.matchMedia('(pointer:fine)').matches) {
    document.querySelectorAll(surfaces).forEach(function (card) {
      var frame = 0, pointer = null, active = false, last = 0;
      var lightX = 30, lightY = 0, lightAlpha = 0, aimX = 30, aimY = 0;
      function resetLight() {
        cancelAnimationFrame(frame); frame = 0; pointer = null; active = false;
        lightX = aimX = 30; lightY = aimY = lightAlpha = 0;
        ['--mx','--my','--sheen-opacity','--tilt-x','--tilt-y'].forEach(function (name) { card.style.removeProperty(name); });
      }
      function paintLight(now) {
        frame = 0;
        if (motion.matches || document.hidden) { resetLight(); return; }
        var dt = Math.min((now - last) / 1000, .05), follow = 1 - Math.exp(-dt * 12), fade = 1 - Math.exp(-dt * 10);
        last = now;
        if (pointer) {
          var rect = card.getBoundingClientRect();
          var px = Math.max(0,Math.min(100,(pointer.clientX - rect.left) / rect.width * 100));
          var py = Math.max(0,Math.min(100,(pointer.clientY - rect.top) / rect.height * 100));
          // A broad reflection moves less than the pointer, rather than acting as a spotlight.
          aimX = 15 + px * .7; aimY = py * .8;
          var tiltScale = card.classList.contains('site-header') ? .35 : 1;
          card.style.setProperty('--tilt-x', (0.5 - py / 100) * 3 * tiltScale + 'deg');
          card.style.setProperty('--tilt-y', (px / 100 - 0.5) * 4 * tiltScale + 'deg');
          pointer = null;
        }
        lightX += (aimX - lightX) * follow; lightY += (aimY - lightY) * follow;
        lightAlpha += ((active ? 1 : 0) - lightAlpha) * fade;
        card.style.setProperty('--mx', lightX + '%'); card.style.setProperty('--my', lightY + '%');
        card.style.setProperty('--sheen-opacity', lightAlpha.toFixed(4));
        if (Math.abs(aimX-lightX) + Math.abs(aimY-lightY) > .05 || Math.abs((active?1:0)-lightAlpha) > .002) frame = requestAnimationFrame(paintLight);
        else if (!active) resetLight();
      }
      function wakeLight() {
        if (!frame) { last = performance.now(); frame = requestAnimationFrame(paintLight); }
      }
      function trackPointer(event) {
        if (motion.matches || event.pointerType === 'touch') return;
        active = true; pointer = event; wakeLight();
      }
      card.addEventListener('pointerenter', function (event) {
        card.style.removeProperty('transition-delay'); trackPointer(event);
      });
      card.addEventListener('pointermove', trackPointer);
      function leaveLight() {
        pointer = null; active = false; aimX = 30; aimY = 0;
        card.style.removeProperty('--tilt-x'); card.style.removeProperty('--tilt-y');
        if (motion.matches) resetLight(); else wakeLight();
      }
      card.addEventListener('pointerleave', leaveLight);
      card.addEventListener('pointercancel', leaveLight);
      motion.addEventListener('change', function () { if (motion.matches) resetLight(); });
      document.addEventListener('visibilitychange', function () { if (document.hidden) resetLight(); });
    });
  }


  // Rounded-rectangle surface normals displace only the bevel, keeping the glass center calm.
  if (!window.CSS || !CSS.supports('backdrop-filter', 'url("#glass-optics")') || !window.ResizeObserver || !window.IntersectionObserver) return;
  const ns = 'http://www.w3.org/2000/svg';
  function svg(tag, attrs) { const el = document.createElementNS(ns, tag); for (const [key,value] of Object.entries(attrs)) el.setAttribute(key,value); return el; }
  const filters = svg('svg', { width:0, height:0, 'aria-hidden':'true', focusable:'false' });
  filters.style.position = 'absolute'; document.body.appendChild(filters);
  const records = new Map();
  function distance(x,y,w,h,r) { const qx=Math.abs(x-w/2)-(w/2-r), qy=Math.abs(y-h/2)-(h/2-r); return Math.hypot(Math.max(qx,0),Math.max(qy,0))+Math.min(Math.max(qx,qy),0)-r; }
  function update(el) {
    const record = records.get(el); if (!record || !record.visible) return;
    const w=el.clientWidth+2, h=el.clientHeight+2; if (w===record.width && h===record.height) return;
    record.width=w; record.height=h;
    const raw=getComputedStyle(el).borderTopLeftRadius, radius=Math.min(w/2,h/2,parseFloat(raw)*(raw.includes('%')?Math.min(w,h)/100:1));
    const map=document.createElement('canvas'), ratio=Math.min(1,512/Math.max(w,h)); map.width=Math.max(1,Math.round(w*ratio)); map.height=Math.max(1,Math.round(h*ratio));
    const ctx=map.getContext('2d'); if (!ctx) return;
    const image=ctx.createImageData(map.width,map.height), data=image.data;
    for(let y=0;y<map.height;y++)for(let x=0;x<map.width;x++) {
      const px=(x+.5)/ratio,py=(y+.5)/ratio,d=-distance(px,py,w,h,radius),i=(y*map.width+x)*4;
      let nx=0,ny=0;
      if(d>=0&&d<18){const dx=distance(px+.5,py,w,h,radius)-distance(px-.5,py,w,h,radius),dy=distance(px,py+.5,w,h,radius)-distance(px,py-.5,w,h,radius),n=Math.hypot(dx,dy)||1,curve=Math.pow(1-d/18,1.8)*.42;nx=dx/n*curve;ny=dy/n*curve;}
      data[i]=(0.5+nx)*255;data[i+1]=(0.5+ny)*255;data[i+2]=128;data[i+3]=255;
    }
    ctx.putImageData(image,0,0); record.map.setAttribute('href',map.toDataURL()); el.dataset.refracted='true';
  }
  const observer = new ResizeObserver(entries => entries.forEach(entry => update(entry.target)));
  const intersection = new IntersectionObserver(entries => entries.forEach(entry => { const r=records.get(entry.target);r.visible=entry.isIntersecting; if(r.visible)update(entry.target); }), { rootMargin:'100px' });
  document.querySelectorAll(surfaces).forEach((el,index) => {
    const id='glass-optics-'+index, filter=svg('filter',{id,x:0,y:0,width:'100%',height:'100%','color-interpolation-filters':'sRGB'});
    const map=svg('feImage',{x:0,y:0,width:'100%',height:'100%',preserveAspectRatio:'none',result:'bevel'});
    filter.append(map,svg('feDisplacementMap',{in:'SourceGraphic',in2:'bevel',scale:32,xChannelSelector:'R',yChannelSelector:'G'}));filters.appendChild(filter);
    el.style.setProperty('--refraction','url("#'+id+'")'); records.set(el,{map,width:0,height:0,visible:false});observer.observe(el);intersection.observe(el);
  });
})();
