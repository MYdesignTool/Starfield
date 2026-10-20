/* Independent website artwork. No Starfield render code or AE bridge is used. */
(() => {
  'use strict';
  const menu = document.querySelector('.menu-toggle');
  const navigation = document.getElementById('navigation');
  if (menu && navigation) {
    menu.hidden = false;
    document.querySelector('.site-header').classList.add('menu-enhanced');
    const closeMenu = () => {
      menu.setAttribute('aria-expanded', 'false');
      navigation.classList.remove('is-open');
    };
    menu.addEventListener('click', () => {
      const open = menu.getAttribute('aria-expanded') !== 'true';
      menu.setAttribute('aria-expanded', String(open));
      navigation.classList.toggle('is-open', open);
    });
    navigation.addEventListener('click', event => {
      if (event.target.closest('a')) closeMenu();
    });
    document.addEventListener('keydown', event => {
      if (event.key === 'Escape' && menu.getAttribute('aria-expanded') === 'true') {
        closeMenu();
        menu.focus();
      }
    });
  }

  const canvas = document.getElementById('particle-canvas');
  if (!canvas || !canvas.getContext) return;
  const context = canvas.getContext('2d');
  if (!context) return;
  const stage = canvas.closest('.particle-stage');
  const toggle = document.getElementById('motion-toggle');
  const reducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)');
  let paused = reducedMotion.matches;
  let onScreen = true;
  let width = 0;
  let height = 0;
  let frame = 0;
  let previous = null;
  let time = 0;
  let lastPaint = -Infinity;
  let seed = 7234;
  const random = () => {
    seed = (seed * 1664525 + 1013904223) >>> 0;
    return seed / 4294967296;
  };
  // Fixed artwork, independent of viewport size; no simulated performance claims.
  const points = Array.from({ length: 3600 }, () => {
    const lane = random();
    const angle = Math.floor(random() * 3) * Math.PI * 2 / 3 + (random() - 0.5) * (0.36 + lane * 0.42);
    const radius = 0.04 + Math.pow(lane, 0.72) * 0.94;
    return {
      angle,
      radius,
      elevation: (random() - 0.5) * (0.035 + lane * 0.19),
      brightness: 0.4 + random() * 0.6,
      size: random() > 0.975 ? 1.9 : 0.55 + random() * 0.85,
      speed: 0.025 + (1 - lane) * 0.055,
      champagne: random() < 0.34
    };
  });
  const stars = Array.from({ length: 70 }, () => ({ x: random(), y: random(), alpha: random() * 0.28 + 0.05 }));

  function paint() {
    if (!width || !height) return;
    context.clearRect(0, 0, width, height);
    for (const star of stars) {
      context.fillStyle = `rgba(213,231,249,${star.alpha})`;
      context.fillRect(star.x * width, star.y * height, 1, 1);
    }
    const scale = Math.min(width * 0.51, height * 0.47);
    const cx = width * 0.55;
    const cy = height * 0.48;
    const rotation = -0.57;
    const cos = Math.cos(rotation);
    const sin = Math.sin(rotation);
    const halo = context.createRadialGradient(cx, cy, scale * 0.1, cx, cy, scale);
    halo.addColorStop(0, 'rgba(255,211,141,0.3)');
    halo.addColorStop(0.15, 'rgba(69,159,244,0.18)');
    halo.addColorStop(0.55, 'rgba(22,115,222,0.085)');
    halo.addColorStop(1, 'rgba(22,115,222,0)');
    context.fillStyle = halo;
    context.fillRect(0, 0, width, height);
    // Fine luminous filaments echo the existing preset banner's visual language.
    for (let arm = 0; arm < 3; arm++) {
      for (let strand = 0; strand < 3; strand++) {
        context.beginPath();
        for (let step = 0; step <= 90; step++) {
          const radius = 0.04 + step / 90 * 0.94;
          const angle = arm * Math.PI * 2 / 3 + radius * 5.5 + (strand - 1) * 0.045 + time * 0.025;
          const x = Math.cos(angle) * radius;
          const y = Math.sin(angle) * radius * 0.55;
          const px = cx + (x * cos - y * sin) * scale;
          const py = cy + (x * sin + y * cos) * scale;
          if (step === 0) context.moveTo(px, py);
          else context.lineTo(px, py);
        }
        context.lineWidth = strand === 1 ? 0.85 : 0.45;
        context.strokeStyle = arm === 1 ? 'rgba(255,196,109,0.7)' : 'rgba(69,179,255,0.58)';
        context.stroke();
      }
    }
    const projected = points.map(point => {
      const angle = point.angle + time * point.speed;
      const spiral = angle + point.radius * 5.5;
      const x = Math.cos(spiral) * point.radius;
      const depth = Math.sin(spiral) * point.radius;
      const y = depth * 0.55 + point.elevation * (0.6 + Math.sin(angle) * 0.15);
      return { point, x: cx + (x * cos - y * sin) * scale, y: cy + (x * sin + y * cos) * scale, depth };
    }).sort((a, b) => a.depth - b.depth);
    for (const { point, x, y, depth } of projected) {
      const alpha = Math.min(1, point.brightness * (0.65 + (depth + 1) * 0.2));
      const size = point.size * (0.75 + (depth + 1) * 0.25);
      context.fillStyle = point.champagne ? `rgba(255,${Math.round(175 + point.brightness * 55)},${Math.round(72 + point.brightness * 60)},${alpha})` : `rgba(${Math.round(40 + point.brightness * 60)},${Math.round(150 + point.brightness * 80)},255,${alpha})`;
      context.fillRect(x, y, size, size);
      if (point.size > 1.5) {
        const glow = context.createRadialGradient(x, y, 0, x, y, size * 4);
        glow.addColorStop(0, point.champagne ? `rgba(255,199,121,${alpha * 0.55})` : `rgba(81,194,255,${alpha * 0.55})`);
        glow.addColorStop(1, 'rgba(81,194,255,0)');
        context.fillStyle = glow;
        context.fillRect(x - size * 4, y - size * 4, size * 8, size * 8);
      }
    }
    stage.classList.add('is-ready');
  }

  function canRun() {
    return !paused && onScreen && !document.hidden;
  }
  function tick(timestamp) {
    frame = 0;
    if (!canRun()) { previous = null; return; }
    if (timestamp - lastPaint >= 1000 / 30) {
      if (previous !== null) time += Math.min((timestamp - previous) / 1000, 0.1);
      previous = timestamp;
      lastPaint = timestamp;
      paint();
    }
    frame = window.requestAnimationFrame(tick);
  }
  function sync() {
    if (frame) window.cancelAnimationFrame(frame);
    frame = 0;
    previous = null;
    if (canRun()) frame = window.requestAnimationFrame(tick);
    if (toggle) {
      toggle.setAttribute('aria-pressed', String(paused));
      toggle.setAttribute('aria-label', paused ? '播放粒子动画' : '暂停粒子动画');
      toggle.firstElementChild.textContent = paused ? '▷' : 'Ⅱ';
    }
  }
  function resize() {
    const bounds = canvas.getBoundingClientRect();
    width = bounds.width;
    height = bounds.height;
    const ratio = Math.min(window.devicePixelRatio || 1, 2);
    canvas.width = Math.round(width * ratio);
    canvas.height = Math.round(height * ratio);
    context.setTransform(ratio, 0, 0, ratio, 0, 0);
    paint();
  }
  if (toggle) {
    toggle.hidden = false;
    toggle.addEventListener('click', () => { paused = !paused; sync(); });
  }
  reducedMotion.addEventListener('change', event => { paused = event.matches; sync(); });
  document.addEventListener('visibilitychange', sync);
  if ('IntersectionObserver' in window) {
    new IntersectionObserver(entries => {
      onScreen = entries[0].isIntersecting;
      sync();
    }, { threshold: 0 }).observe(stage);
  }
  if ('ResizeObserver' in window) new ResizeObserver(resize).observe(stage);
  else window.addEventListener('resize', resize);
  resize();
  sync();
})();
