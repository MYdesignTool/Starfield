# ADR 0030: native editor presets and interpolation

Status: implemented candidate build40, AE qualification pending. Owner provides curve/color palette
screenshots on2026-10-05, explicitly excludes Stardust's directory structure,
places entry points at each editor's Presets button, and requires exactly
Linear/Hold color interpolation. Recreate shapes/colors independently; do not
copy Stardust files, scripts, resources or algorithms.

## Owned contract

Provide a native, owner-attached modal thumbnail picker from Particle Color
Gradient and Over Life Size/Opacity/Rotation. It applies through the existing
whole-bank native edit/undo/publication path. No external processes, CEP dialog
installation, filesystem preset directories or host-wide settings are involved.
Keep the separate main graph preset manager for complete particle systems.

Curve presets require dense random profiles and real smooth interpolation.
Increase bounded age storage from8 to64 knots and add Linear/Hold/Bezier/Draw modes;
Bezier uses independently computed shape-preserving cubic Hermite segments
(equivalent cubic Bezier control points), not vendor control points. Graph curve
payload byte2 selects0 Linear/1 Hold/2 Bezier/3 Draw; byte3 remains0 and knot pairs stay16bytes.
Color gradient byte2 selects0 Linear/1 Hold; byte3 remains0, with2..8 stops.
Hold switches at the next stop's position and retains the nearest endpoint color
outside the stop range. UI toggles affect drawing, evaluation and saved graphs.
The catalog is numeric source data in schema/editor-presets.json. A reporting-by-
default generator with an explicit Generate switch produces native/CEP catalog
data; native build outputs remain under ignored artifacts.

Particle gains64-knot Size/Opacity/Rotation banks, hidden constant interpolation
streams and native Size/Opacity editors; Force's existing age bank also expands
to64 with an interpolation stream for roundtrip coherence. Native Particle
schema7/base442 and Force schema3/base140 require fresh development effects/
graphs. Emitter7/Output4 and main26/index1 are retained. Existing first8 knot disk
IDs remain explicit; extra knots use distinct3000-series ranges. Update all native
and CEP schemas/readers/writers/defaults together. No old-project migration per
the owner's development policy; paired rollback restores old binaries and panel
sources and requires recreating these new effects/graphs.

Core Settings/AgeCurve internal storage changes, but Render.hpp and numeric
PluginApi C ABI3 do not carry these C++ objects and remain unchanged. The GPU
receives the evaluated particle scene; CPU/GPU therefore use the same authored
age/gradient modes without adding GPU buffer transport. Keep snapshot3/200 bytes.
Core hot publication must be paired with the new native effects; do not install
mixed schema generations. Exact vendor numerical/color parity is not claimed:
the supplied screenshots ground the independently authored palettes and profiles.
No tests are requested/run; native builds and actual owner AE checks are separate.

The owner supplied the additional curve mode screenshot on2026-10-05.
Curve mode buttons cycle Linear, Hold, Bezier, Draw. Draw is a64-knot freehand
editing mode with linear sample evaluation; Hold changes at the next knot.
Bezier uses automatic shape-preserving tangents rather than movable vendor
control handles. The independently computed Force motion integrates each
constant/linear/cubic segment under constant drag; this task also owns
src/core/ParticleSimulation.cpp to keep the expanded Force bank coherent.
Parameters.cpp explicitly retains the main effect eight-knot transport.

## Panel generation41: scoped native bank names

Owner build40 reports node_effect_sync_failed looking for Rotation Curve Count.
This literal is absent from current source; its runtime producer is unconfirmed.
The node client had cached readiness and separately invoked global host functions,
leaving a gap when another CEP page reloads those functions. Generation41 reloads
every mutation and calls its operation within one evalScript turn; read-only calls
reuse only a matching generation. Requests/replies carry and check the generation.
Both extension versions and HTML URLs advance together. No cache/registry changes.

Static inspection also identifies a definite duplicate name: Particle Properties
Opacity and Over Life Opacity. Curve controls now use an explicit count/mode/point
name table with Over Life or Rotation Properties scopes, including the actual
Rotation Over Life count name. Scalar Opacity resolves in Particle Properties.
Readers and writers share these scopes; no fallback to an ambiguous root label.
Native binaries remain build40 (Particle7/Force3, main26, ABI3); only the21-file
CEP bundle advances. Save paired40 panel and unchanged native files for rollback.
JS/JSX syntax compilation is the implemented check; actual AE add remains open.
