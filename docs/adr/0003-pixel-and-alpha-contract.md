# ADR 0003: render coordinates, pixels, and alpha

- Status: accepted for M0/M1.
- Date: 2026-09-27

The renderer's canonical composition space is centered on the comp center, has +X right, +Y up, and +Z toward the viewer. One world unit is one comp-height; pixel aspect and downsample are applied only at the AE boundary. Time and geometry calculations use double precision; color accumulation uses float or wider.

Input and output buffers state width, height, rowbytes, pixel format, color space, alpha mode, and ROI explicitly. Never assume tightly packed rows or equal input/output rowbytes. The AE adapter converts to/from the host's 8/16/32-bpc representation and respects AE's channel accessors. Compositing is premultiplied-alpha internally; convert straight-alpha sources on input and convert to the requested host convention on output.

Host working-space conversions go through documented AE color-management suites when available. The core has explicit linear-light color operations and never guesses a display profile from the OS.
