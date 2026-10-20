/* Website navigation and controlled hero motion. No AE bridge. */
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

  const stage = document.querySelector('.particle-stage');
  const toggle = document.getElementById('motion-toggle');
  if (!stage || !toggle) return;
  const reducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)');
  let paused = reducedMotion.matches;
  let onScreen = true;
  stage.classList.add('twinkle-enabled');

  function sync() {
    stage.classList.toggle('is-still', paused || !onScreen || document.hidden);
    toggle.hidden = reducedMotion.matches;
    toggle.setAttribute('aria-pressed', String(paused));
    toggle.setAttribute('aria-label', paused ? '播放主视觉动效' : '暂停主视觉动效');
    toggle.firstElementChild.textContent = paused ? '▷' : 'Ⅱ';
  }
  toggle.addEventListener('click', () => { paused = !paused; sync(); });
  reducedMotion.addEventListener('change', event => { paused = event.matches; sync(); });
  document.addEventListener('visibilitychange', sync);
  if ('IntersectionObserver' in window) {
    new IntersectionObserver(entries => {
      onScreen = entries[0].isIntersecting;
      sync();
    }, { threshold: 0 }).observe(stage);
  }
  sync();
})();
