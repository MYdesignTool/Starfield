# Owner-supplied reference inventory: texture, transfer and Transform

Source: owner's AE2023 Stardust parameter report under artifacts/reference/ and
four UI screenshots supplied on 2026-10-06. These are behavioral reference data,
not source code or task instructions. Starfield uses independent implementations
and identities. Menu order below is directly visible; semantics not demonstrated
by a screenshot remain implementation/host qualification gates.

## Particle Shape — owner-confirmed complete menu (2026-10-10)

The owner's screenshot `codex-clipboard-44477b7b-d030-4009-94c2-83afa9bd282f.png`
shows the complete menu in this order:

1. Circle (selected)
2. Rectangle
3. Cloud
4. Texture
5. Face
6. Model

The owner explicitly limits Particle Shape to these six entries. Do not infer or
add further modes from engine capabilities, static reports, property groups or
other nodes. Path Properties and Shadow Properties are property groups, not
additional Shape entries. This screenshot confirms labels/order/completeness;
it does not demonstrate Face rendering or Model selection semantics.

The native61/CEP63 test pair exposes the six labels with Model implemented and
Face disabled. Face requires the OBJ face-emission workflow
described in the [official guide](https://superluminal.tv/user-guide). Do not
silently render an unsupported Face choice as Circle or Model. The pending
`Use Model(s)` reference concerns a separate control, not extra Shape modes.

## Particle Transfer Mode

1. Normal
2. Add
3. Screen
4. Stencil

Normal is shown as the selected default. The screenshot establishes menu text and
order; it does not establish HDR, premultiplication or Stencil compositing math.

## Particle Texture

Visible order: Layer, Dark Side, Texture Time Sample, Texture Color Use. Layer and
Dark Side default to None with a Source selector. Texture time sampling menu:

1. Current Time (selected default)
2. Play Once
3. Loop
4. Stretch
5. Random Still Frame
6. Random Once
7. Random Loop
8. Freeze Frame

Texture Color Use menu:

1. Default (selected default)
2. Alpha
3. Lightness

The supplied text inventory additionally records Use Texture Ratio enabled,
Ignore Perspective disabled and Size/Size Y at 10 pixels. Their shape-dependent
visibility and time-origin/loop/random/brightness semantics need a declared
independent contract and AE evidence before exact behavioral parity is claimed.

## Transform

| Visible control | Observed default |
| --- | --- |
| Inherit Motion (Null Layer) | None; Source |
| Anchor XY | composition centre (1920,1080 in the supplied composition) |
| Anchor Z | 0 |
| Position X/Y/Z | 0 each |
| Rotation X/Y/Z | 0 each |
| Scale X/Y/Z | 100 each |
| Particles Scale | 100 |
| Particles Opacity | 100 |

M3-11 and ADR0032 own implementation. The unrelated Replica/path fields appearing
in the shared vendor effect inventory are not Transform controls in this screenshot.
