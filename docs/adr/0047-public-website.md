# ADR 0047: public website on GitHub Pages

- Status: accepted for WEB-01 (owner direction, 2026-10-20).

The product website is a buildless, Chinese-first HTML/CSS/JavaScript site under
`docs/site/`. GitHub Pages is the requested host; the Sites hosting service is not
used. The independent site has no access to the AE panel bridge, native modules,
SDK, user projects or local deployment tools.

GitHub Actions validates the site on pull requests and publishes only `docs/site/`
on main pushes or manual main runs. Pages receives neither the repository root nor
the surrounding engineering documents. Relative asset links support the `/Starfield/`
project path, local files and a future domain without bundler configuration.

The site describes the independent Starfield identity and current Windows / AE2023
development scope. Generated raster particle artwork is labelled as an illustration.
The interactive node playground reuses project-owned panel UI modules and 46 selected
canvas functions inside a sandboxed iframe. An in-memory adapter replaces host
selection, reads, transactions and layout commits; no host bridge, polling, parameter
transport, preset/file operations or renderer is loaded. The frame has only
`allow-scripts`, a CSP with `connect-src 'none'`, and no persistence. Its UI-only scope
is visible in the frame and caption. Shared source/export details belong in
`docs/website-canvas.md`. Publication
does not qualify a host, device or performance claim. There is no fabricated
installer, testimonial, price, performance figure or unsupported compatibility claim.

No runtime dependency, third-party font, analytics or remote media is required.
The owner's Ae-ToolKit site provides the product-family glass reference. Starfield
uses a dedicated `brand.css` layer for shared color, type, translucent surfaces and
responsive framing; implementation/provenance belong in `docs/website-style.md`.
The hero uses a local JPEG with a small CSS twinkle overlay, rather than a procedural
orbit simulation. Asset provenance and the generation prompt are recorded in
`docs/website-artwork.md`. Subtle CSS image drift, breathing light and twinkles share
one pause control and respect reduced motion, offscreen state, page
visibility and a pause control; core content and navigation remain usable without
JavaScript, with a static image and static glints.

The site and workflow can be reverted independently of plugin development. Remote
publication evidence and remaining setup belong in `docs/website.md`, not in the
native deployment pairing recorded by `docs/current-state.md`.
