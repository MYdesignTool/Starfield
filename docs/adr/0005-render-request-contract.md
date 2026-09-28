# ADR 0005: render request, region of interest, and the M2 compositing default

- Status: accepted for M2.
- Date: 2026-09-27
- Task: M2-01 (contract), consumed by M2-03/M2-04/M2-05.

## Frame description

`FrameSpec` carries three distinct geometries, and mixing them is a defect:

1. `layer_width` / `layer_height`: the full-resolution layer size. This is the world-space
   reference: one world unit is one layer height, the origin is the layer center, +X is right,
   +Y is up, +Z is toward the viewer (ADR 0003).
2. `frame_width` / `frame_height`: the full render-resolution pixel grid, i.e. the layer grid
   already scaled by the host downsample factor.
3. `region_of_interest`: the half-open sub-rect of the frame grid that this render must fill.

Coordinates are half-open on both axes (`[left, right) x [top, bottom)`). An empty region of
interest is legal and must produce an empty output without allocating. The world-to-pixel
mapping used by the renderer is:

```text
aspect      = layer_width * pixel_aspect_ratio / layer_height
pixel_x     = (0.5 + world_x / aspect) * frame_width  - region.left
pixel_y     = (0.5 - world_y) * frame_height          - region.top
pixel_scale = frame_width / layer_width               (sprite scale, ~ 1 / downsample factor)
```

`pixel_aspect_ratio` is the host's pixel width/height ratio. It widens the world mapping; it does
not turn sprites into ellipses, because `particle_size` is a pixel diameter, not a world length.

## Output ownership and errors

The renderer returns an owned staging buffer that covers exactly the requested region, with
`row_bytes` chosen by the core and reported explicitly. The AE adapter copies it into the host
world while the host's pointer is valid and then releases every checkout on every exit path.

Failures use the `ErrorCode` taxonomy, never host error codes. Structural problems
(`invalid_request`), rational overflow (`invalid_time`), unavailable formats
(`unsupported_format`), bounded allocation failure (`allocation_failed`), excessive work
(`work_limit_exceeded`), and host cancellation (`cancelled`) are distinguishable so the adapter
can map them onto a stable AE error.

Work is bounded on purpose: the CPU renderer accumulates a sprite-coverage budget and returns
`work_limit_exceeded` instead of blocking the host for an unbounded time. The budget is a
documented constant, not a hidden truncation.

## Compositing default (open behavior question from the M2-04 card)

Particles composite **over** the source with premultiplied "over"; wherever no sprite covers a
pixel the source passes through, and an absent source means transparent black. This is a chosen
default for the first vertical slice, recorded here instead of being presented as verified
reference compatibility. It must be confirmed against the observed reference effect before the
project claims parity for input-dependent cases.

## Color

M2 performs no color-space conversion. Source samples are composited as given in the declared
`ColorSpace` and the output is written in the same space. Color management through documented AE
suites is deferred until the reference behavior is confirmed (ADR 0003 remains the long-term
contract).

## AE adapter geometry: observed, never derived from the downsample factor

The first implementation derived the render-resolution layer grid from
`in_data->downsample_x/y`, which is ambiguous in the SDK itself:

- `AE_Effect.h` (PF_InData documentation) says `width`/`height` are "the full-resolution dimensions
  of the input layer", that "the width and height of all effect parameters (including layers) will
  be automatically adjusted to compensate" for downsampling, and that "downsample factors will be
  in the range 1 to 999+" — reading the stored `PF_RationalScale` as a divisor (2 at half
  resolution).
- The SDK's own samples read it the other way: `Resizer.cpp` and `PathMaster.cpp` multiply a
  full-resolution distance by `num / den` with comments stating the result is the *smaller*
  render-space distance, which only holds if the value is a scale (1/2 at half resolution).

Both readings cannot be right, and the factor is not a safe basis for placing world-space
particles. The adapter therefore derives every render-space quantity from what the host delivers:

| Quantity | Source |
|---|---|
| Pixels per host rect unit | checked-out world dimension ÷ the rect we asked for (`PF_PreRenderOutput::result_rect`), for x and y separately |
| Layer extent in host rect units | `PF_CheckoutResult::max_result_rect` (documented as request independent) |
| Full-resolution world reference | `PF_CheckoutResult::ref_width/ref_height` ("original size of layer … disregarding any downsample factors"), falling back to `in_data->width/height` |
| Pixel aspect ratio | `PF_CheckoutResult::par`, falling back to `in_data->pixel_aspect_ratio` |
| Region of interest | the output world's own origin and dimensions, scaled by the observed pixels-per-rect ratio |

`PF_PreRenderOutput::pre_render_data` carries those pre-render facts to the render phase as a small
POD block that AE owns and frees through `delete_pre_render_data_func`. A render without that state
fails loudly (`internal_failure`) instead of guessing coordinates.

Consequences: a wrong assumption about the downsample convention can no longer move particles, and
the geometry follows AE's own rectangles at any preview resolution. The tradeoff is that the
adapter must observe the world before it can map it, so it cannot pre-compute bounds from
`in_data->width/height` alone. The adapter still never assumes tightly packed rows or equal
input/output row bytes, and unverified host behavior is recorded in
`docs/compatibility-matrix.md` until a host pass confirms it.

## Point controls are preview-scaled positions

The user's AE 2023.5.0 Build 52 readouts confirm that the point control's absolute layer-pixel
coordinates are scaled by the preview resolution. The same 3840×2160 center is delivered as
1920/1080/1080 at Full (`downsample 1/1`) and 480/270/270 at Quarter (`1/4`), while the render
reference remains 3840×2160 and the actual grid changes to 960×540. The adapter restores full-size
coordinates by multiplying X by `downsample_x.den / downsample_x.num` and Y/Z by the vertical
reciprocal before calling `layer_point_to_world()`. Invalid or absent scale values use 1:1.

This empirical rule is host-qualified only for AE 23.5.0 Build 52; another AE 2023 build must pass
the same readout before being claimed. It is separate from render-grid derivation: the render grid
continues to come from observed worlds and rectangles above, not from the ambiguous scale field.
The core treats the normalized point as an absolute pixel coordinate and preserves legitimate
off-layer positions; it no longer guesses percentages or fixed-point encodings from the magnitude.

Point controls are used for positions (emitter origin) only. Rates and directions are scalar sliders;
a point control would give the user a pick widget and pixel-valued numbers for a quantity that is not
a position. Revision 1 read points as percentages; later revisions missed the preview scaling and
divided Quarter-sized values by the full reference. Both errors move geometry silently, so future
unit changes require a paired host readout and adapter regression.

## Not in scope

Threaded rendering (MFR), Compute Cache, GPU backends, float-color advertisement beyond the
implemented 32-bpc path, and sequence serialization stay out of M2 and require their own ADRs.
