# ADR 0016: project-owned Particle over-life curves

- Status: accepted for implementation
- Date: 2026-09-30
- Scope: AE 2023 CEP authoring, AE parameter persistence, and core Particle/Appearance evaluation

## Context

The current Size Over Life and Opacity Over Life controls store only one end value,
so the renderer interpolates a straight line from the birth value. The Particle node
needs a compact, editable curve with a small fixed work and storage bound. AE 2023's
ExtendScript DOM cannot read or write the effect's `CUSTOM_VALUE` graph parameter;
the CEP panel therefore has to use ordinary script-visible effect parameters and let
the native effect rebuild its graph during a supervised parameter change.

## Decision

Each curve uses normalized age `x ∈ [0,1]` and an ordinate in percent, bounded by
0…100. Both plots always use that fixed vertical range. The Size curve percentage
multiplies the Particle node's base Size in full-resolution layer pixels; the
Opacity curve percentage multiplies its base Opacity in 0…1. The rendered values
remain pixels and unit opacity, respectively.
- A curve has two fixed endpoints and zero to six interior knots, for at most eight
  points. Ages are strictly increasing; the first and last ages must be exactly 0
  and 1. Evaluation is piecewise linear.
- The CEP plot converts pointer positions through the SVG screen transform before
  adding or moving points, so CSS scale and border width do not shift the hit location.
  Clicking within 10 screen pixels of a segment inserts a knot on that segment, keeping
  the current shape until the new knot is moved. Clicking farther away uses the pointer's
  value. The plot moves points on drag and allows direct numeric
  entry for the selected point's Life percentage and value. These numeric fields also
  support the panel's left/right scrub gesture. Previous/next controls select points;
  endpoint ages stay pinned and interior ages stay between their neighbors. Interior
  points can be removed. The interpolation control selects the algorithm between
  adjacent knots; `Linear` never removes or resets knots. Piecewise linear is the
  only implemented algorithm. Bezier remains a later interpolation option.
- A zero point count means a linear curve from 100% at birth to the endpoint
  percentage at the end of life. Endpoint controls default to 100%.
- Color keeps its existing linear start/end interpolation.

The AE effect appends hidden, non-time-varying scalar streams for both point banks
(IDs 45–78), followed by a supervised curve-edit nonce (ID 79). Endpoint parameters
14 and 16 mirror only the final curve percentage; the first curve percentage remains
in the project curve bank or graph payload. CEP writes point slots, final endpoint
mirrors, the active count, and finally the nonce in one AE undo group. The nonce is
the sole commit trigger for a panel curve edit; after it fires, the adapter constructs
the canonical graph from the complete bank. Editing base Size or Opacity does not
change curve ordinates. Editing an endpoint control updates only the final active
curve point in the same parameter-change callback. In AE Controls mode, animated
endpoint percentages remain authoritative at the sampled render time while interior
knots and the first ordinate stay constant project data. The CEP editor locks an
animated final ordinate and directs the user to its AE keyframes; interior knots
remain editable. Node Graph values keep the existing
constant-value semantics from ADR 0007.

The graph stores an active curve as an optional opaque parameter on Particle and
Appearance nodes. The nested payload is version 1: a four-byte header containing
payload version, point count, and two zero reserved bytes, followed by little-endian
IEEE-754 binary64 age/value pairs. A missing optional key means the straight-line
curve from 100% to the stored endpoint percentage. The outer graph codec continues
to provide the graph-level bounds and CRC.

## Consequences

- Curve authoring and hidden curve-bank persistence use AE project data, so save,
  duplicate, and undo follow AE's normal parameter behavior. CEP local storage is
  not authoritative.
- A new curve changes the serialized graph and requires the updated effect binary;
  the revision-9 panel reports a missing parameter if paired with an older binary.
- Parameter IDs and the graph codec's existing scalar keys stay unchanged; this is
  a pre-release semantic revision with no old-project migration requirement.
- This decision does not qualify ExtendScript stream access, callback commit ordering,
  rendering, undo, or save/reopen in AE. Those remain explicit AE 2023 host gates.
- The curve editor lives in the CEP Particle inspector. Native Effect Controls retain
  the endpoint parameters; they expose the final curve percentages, while Size and
  Opacity remain independent base controls.
- Particle Size is a full-resolution layer-pixel value bounded at 100,000 px; both
  over-life curves are bounded percentages. Emitter Size X/Y/Z are also direct full-resolution
  layer-pixel dimensions, with a 100,000 px bound; their pre-release percentage
  interpretation was dropped by owner direction (ADR 0017). Disc Size remains a
  separate layer-height diameter control.

## Validation rules

The adapter and core reject active curves with fewer than two or more than eight
points, non-finite values, out-of-range ages/ordinates, unordered ages, or endpoints
that move away from 0 and 1. The panel validates its proposed point list, and the
CEP gateway validates both effective point banks again when the nonce is present,
before opening an undo group or writing host parameters. Unused hidden slots are
ignored once the count is known.
