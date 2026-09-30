# Delivery examples (Spark, Snow, Floating Light)

Three acceptance examples for the current Alpha. Values below are the same ones the
panel's **Example** presets apply (`cep_panel/js/panel.js`), and they can also be typed
into the Effect Controls Window by hand — no panel required, so the examples stay
verifiable if the CEP panel is unavailable.

`Emitter Origin` is intentionally not part of a preset. Its host unit (layer pixels vs
percent) is still an open question (backlog D-05), and guessing it could silently move
the emitter off-canvas. Set it by hand if you want the emitter somewhere other than the
control's default centre.

| Parameter | Spark | Snow | Floating Light |
|---|---|---|---|
| Particle Count | 4000 | 2500 | 300 |
| Birth Rate | 220 | 90 | 14 |
| Random Seed | 7 | 21 | 3 |
| Particle Lifetime | 1.1 | 6.5 | 9 |
| Emitter Shape | Box | Sphere | Disc |
| Disc Size | 0 | 0.05 | 0.9 |
| Size X | 16 | 2160 | 100 |
| Size Y | 16 | 1080 | 100 |
| Size Z | 16 | 2160 | 100 |
| Velocity X | 0 | 0.06 | 0 |
| Velocity Y | 1.6 | -0.14 | 0.16 |
| Velocity Z | 0 | 0 | 0 |
| Velocity Spread | 1.1 | 0.35 | 0.4 |
| Gravity X | 0 | 0 | 0 |
| Gravity Y | -2.6 | -0.05 | 0.06 |
| Gravity Z | 0 | 0 | 0 |
| Linear Drag | 0.9 | 0.15 | 0.35 |
| Particle Size | 3.2 | 4.5 | 14 |
| Size End | 0.6 | 4.5 | 3 |
| Opacity | 1 | 0.9 | 0.85 |
| Opacity End | 0 | 0.75 | 0 |
| Color Start (RGB) | 255, 240, 180 | 235, 245, 255 | 255, 232, 150 |
| Color End (RGB) | 255, 90, 20 | 200, 215, 235 | 255, 140, 60 |

Units: velocities, gravity, and drag follow `docs/parameter-mapping.md`; sizes are
full-resolution layer pixels; colors are 8-bit channels.

## What each example should show

- **Spark** — a fast upward burst that falls back and fades: short lifetime, strong
  upward birth velocity, downward gravity, heavy drag, size and opacity collapsing to
  zero, color warming from pale yellow to red-orange.
- **Snow** — a wide, slow, steady drift: box emitter, small downward velocity with
  modest spread, gentle gravity and drag, near-constant size, opacity easing from 0.9
  to 0.75, color cooling from white-blue to grey-blue.
- **Floating Light** — sparse, large, slowly rising soft lights: sphere emitter, low
  birth rate, long lifetime, weak upward gravity, moderate drag, size shrinking from
  14 to 3 px, opacity fading to zero, color going from warm yellow to amber.

## Acceptance record

Record each example in `docs/compatibility-matrix.md`: host build, comp bit depth,
resolution, a still frame at t >= 1 s, and whether the trail matches the description
above. A preset that renders the wrong shape, the wrong direction, or nothing is a
behaviour finding, not a preset problem — the same values must be typed by hand to
separate a panel/gateway failure from a renderer failure.
