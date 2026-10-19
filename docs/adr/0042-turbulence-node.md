# ADR 0042 — Turbulence node, staged authoring

Status: M3-19 first stage retained in the deployed M3-20 native84 / Core ABI9 /
CEP86 stability pair (ADR0043). The owner
explicitly deferred Spherical and Spherical 2; this stage exposes only Normal,
Axis, and One Side. Build and paired deployment are recorded in
`docs/current-state.md`; manual AE2023 qualification remains open.

## Scope and evidence

The reference screenshots establish the visible control labels and popup
choices. The owner has confirmed that the two Spherical modes can wait. This
stage therefore registers one independent Turbulence node and does not expose
those two modes. It retains the screenshot's axis, Infinite/Sphere, All/Noise,
Over Life, and noise parameter controls.

The screenshots do not define numerical equations, ranges, or the meaning of
every offset. The mappings below are explicit implementation choices so the
node is connected end to end. They are not claims about Stardust's private
implementation. Manual AE2023 review can refine them without changing the
native node identity or Core ABI.

## Core field and graph order

The independent noise kernel is deterministic, allocation-free 3D gradient
noise with bounded octave count and finite output. Turbulence is a particle
stream node. It evaluates in upstream-to-downstream edge order, per route, and
keeps sibling fan-out results isolated. Native time-varying controls are sampled
at the requested frame. No particle snapshot or Render ABI fields were added.

Available modes:

- **Normal** applies three independent noise channels to XYZ.
- **Axis** applies one noise channel along the selected X, Y, Z, or Normal axis.
- **One Side** uses the selected axis and clamps the noise displacement to the
  positive direction.

For the **Normal** axis option, this implementation uses the particle's current
velocity direction and falls back to +Z when velocity is zero. Treat that as a
qualification hypothesis until the owner exercises it in AE.

Noise is sampled from particle position relative to the origin, plus offset and
time drift. Seed identity includes the render seed, particle identity, and node
identity. The optional Sphere affect uses distance from the origin and a
smoothstep feather. The reference confirms a feathered sphere control but not
its exact falloff curve.

## Parameter behavior in this stage

AE pixel positions, offsets, amplitude, and sphere size convert to Core world
units using layer height; pixel aspect is applied on X. Noise Scale is a
percentage, with 100 leaving coordinates unchanged. Noise Frequency,
Fractal Speed, and Fractal Evolution are used directly by the Core field.
Fractal Evolution adds a coordinate phase in degrees. Randomize applies a
stable per-particle attenuation from 1 down to `1 - percent / 100`.
Over Life multiplies the field by a 0–100 curve.

**All** applies the current field to position, particle size, color, opacity,
and rotation using the Core equations. **Noise** stores the sampled vector on
the particle for later graph consumers. The current renderer has no separate
noise consumer, so Noise mode does not change the rendered image by itself.
The control remains available as the staged graph output contract; this limit
must be stated in the UI or removed before a customer release if no consumer is
added.

Offsets and affected-property equations are provisional implementation
choices because the supplied reference does not specify their arithmetic or
units. These values and the Noise-only behavior are part of manual acceptance,
not verified Stardust parity.

## Native and CEP contract

`org.starfieldfx.node.turbulence` is native kind 8, schema 1, PiPL resource
16017, and binding record version 11. It owns control streams 1–153 and disk
IDs recorded in `schema/node-parameters.json`. The Over Life curve occupies
streams 13–142 and disk IDs 4300–4363 for ages, 4400–4463 for values, with
1813/1814 for count/interpolation. Noise and sphere controls occupy 143–153.
Older binding readers remain unchanged; Turbulence records require version 11.

The CEP canvas, inspector, curve editor, preset graph defaults, native record
validation, and ExtendScript read/write paths use the same graph keys. Both
Spherical choices are rejected by validation and absent from the native and
panel menus. Graph origin defaults to the layer center in canonical Core world
coordinates.

The gateway stores pixel-valued Noise Amplitude and Sphere size as values
relative to the target layer height. New-node defaults must use the same graph
units before the gateway writes them back to AE controls. The CEP planner now
normalizes both defaults at node creation; otherwise a 2160-pixel layer turned
the displayed amplitude default of 50 into an invalid AE value of 108000.

The native binding record is sparse for the Over Life curve. Capture always
stores its point count, but stores interpolation and age/value streams only when
the count is nonzero, and stores only the active points. The decoder requires
the controls read by Turbulence evaluation plus those active curve fields; it
must not require the unused point bank or interpolation stream for an empty
curve. This fixes new-node rejection at `decode binding record` without changing
binding record version11, parameter IDs, or Core ABI9.

## Qualification boundary

Core noise/graph fixtures were run earlier in this M3-19 task; no test suite or
automatic AE acceptance was run during the noise-amplitude repair. The May2023
x64 SDK build and paired installation hash checks completed. AE2023 behavior,
control persistence, animated controls, curve edits, menu mapping, Noise output,
and each available mode remain for owner-led manual qualification. The
high-node-count deletion warning remains deferred as requested.
