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
    const angle = random() * Math.PI * 2;
    const lane = random();
    const radius = 0.40 + Math.pow(lane, 0.7) * 0.58;
    return {
      angle,
      radius,
      elevation: (random() - 0.5) * (0.11 + lane * 0.27),
      brightness: 0.4 + random() * 0.6,
      size: random() > 0.975 ? 1.9 : 0.55 + random() * 0.85,
      speed: 0.025 + (1 - lane) * 0.055,
      warm: random() > 0.23
    };
  });
  const stars = Array.from({ length: 70 }, () => ({ x: random(), y: random(), alpha: random() * 0.28 + 0.05 }));

  function paint() {
    if (!width || !height) return;
    context.clearRect(0, 0, width, height);
    for (const star of stars) {
      context.fillStyle = `rgba(219,207,177,${star.alpha})`;
      context.fillRect(star.x * width, star.y * height, 1, 1);
    }
    const scale = Math.min(width * 0.51, height * 0.47);
    const cx = width * 0.55;
    const cy = height * 0.48;
    const rotation = -0.57;
    const cos = Math.cos(rotation);
    const sin = Math.sin(rotation);
    const halo = context.createRadialGradient(cx, cy, scale * 0.1, cx, cy, scale);
    halo.addColorStop(0, 'rgba(172,68,23,0)');
    halo.addColorStop(0.5, 'rgba(182,75,29,0.06)');
    halo.addColorStop(1, 'rgba(182,75,29,0)');
    context.fillStyle = halo;
    context.fillRect(0, 0, width, height);
    const projected = points.map(point => {
      const angle = point.angle + time * point.speed;
      const spiral = angle + point.radius * 3.2;
      const x = Math.cos(spiral) * point.radius;
      const depth = Math.sin(spiral) * point.radius;
      const y = depth * 0.55 + point.elevation * (0.6 + Math.sin(angle) * 0.15);
      return { point, x: cx + (x * cos - y * sin) * scale, y: cy + (x * sin + y * cos) * scale, depth };
    }).sort((a, b) => a.depth - b.depth);
    for (const { point, x, y, depth } of projected) {
      const alpha = Math.min(1, point.brightness * (0.65 + (depth + 1) * 0.2));
      const size = point.size * (0.75 + (depth + 1) * 0.25);
      context.fillStyle = point.warm ? `rgba(247,${Math.round(114 + point.brightness * 66)},${Math.round(48 + point.brightness * 65)},${alpha})` : `rgba(218,215,181,${alpha * 0.85})`;
      context.fillRect(x, y, size, size);
      if (point.size > 1.5) {
        context.fillStyle = `rgba(255,191,126,${alpha * 0.12})`;
        context.fillRect(x - 2, y - 2, size + 4, size + 4);
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
