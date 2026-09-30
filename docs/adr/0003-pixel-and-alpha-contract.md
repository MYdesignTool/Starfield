# ADR 0003: render coordinates, pixels, and alpha

- Status: accepted for M0/M1.
- Date: 2026-09-27

The renderer's canonical composition space is centered on the comp center, has +X right, +Y up, and +Z toward the viewer. One world unit is one comp-height; pixel aspect and downsample are applied only at the AE boundary. Time and geometry calculations use double precision; color accumulation uses float or wider.

Host input and output buffers state width, height, rowbytes, pixel format, color space, alpha mode, and ROI explicitly. Never assume tightly packed rows or equal input/output rowbytes. The AE adapter converts to/from the host's 8/16/32-bpc representation and respects AE's channel accessors. The particle output is accumulated internally with premultiplied alpha over transparent black and encoded in the requested output alpha mode; the current effect does not consume input pixel values (ADR 0005).

Host working-space conversions go through documented AE color-management suites when available. The core has explicit linear-light color operations and never guesses a display profile from the OS.

## M2 implementation notes

- Core buffers are byte-order explicit (`r, g, b, a` in memory) with 1, 2 or 4 bytes per
  channel; the AE adapter converts to and from the host's `a, r, g, b` pixel structs
  (`PF_Pixel`, `PF_Pixel16`, `PF_PixelFloat`) at the boundary.
- Host 16-bpc channels are scaled to `PF_MAX_CHAN16 == 32768`, not 65535; 32-bpc worlds carry
  normalized floats. The adapter uses exactly these scales in both directions.
- Particle colors are accumulated as premultiplied alpha and encoded in `FrameSpec::alpha_mode`
  for AE's 8/16/32-bpc world representation. The current AE candidate requests straight alpha at
  the output boundary. Input worlds are not copied into the output; transparent pixels remain zeroed.
- Pixel aspect and downsample are resolved by the adapter into `FrameSpec` (see ADR 0005); the
  core never re-derives them from display assumptions.
- M2 performs no color-space conversion; see ADR 0005 for that scoped decision.

## Reopened host qualification (2026-09-30)

The owner supplied an AE screenshot in which nominally white particles appeared dark against a
saturated-blue composition during translucent portions of their lifetime. A three-frame render of
the paired candidate reproduced the defect: a blue background sample was `[0,108,255,255]`, while
an interior particle sample was `[16,98,209,255]`; 137,499 to 152,247 pixels per frame were darker
than the background. The values are consistent with a compositor multiplying RGB by alpha after
the core had already premultiplied it. M3-06 therefore makes the AE candidate request straight
output while preserving premultiplied internal accumulation. This diagnosis remains an inference
until the revised candidate is rendered over both transparency and an opaque lower layer in AE
2023.5.0 Build 52.
