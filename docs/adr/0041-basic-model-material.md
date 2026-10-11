# ADR 0041 — Model output and basic material

Status: implementation candidate, 2026-10-11. Owner task M3-17.

## Owner correction — 2026-10-12

New authoring uses Model output1 → Material input2 → Output input1, or Material
output1 → Particle's model input3. Material forwards immutable model geometry
with its own shading values; it never rasterizes an intermediate image. The
geometry and inherited particle color remain available to the final renderer.
Material accepts one geometry input. Model may fan out to independent material
branches. New CEP connections cannot feed Material back into Model.

Migration: existing schema1 Material→Model input2 graphs remain readable and
render with the previous attachment behavior. The registry retains that legacy
input and its explicit type union. Schema1 Material adds optional geometry input2
and forwards a model stream on output1; native IDs, six author controls, graph
envelope, binding record layout and CoreABI9 stay unchanged. The next deliberate
CEP edit converts a legacy attachment by reversing its edge (preserving edge ID)
and forwarding the Model's previous destinations through Material. The read-only
refresh preserves shared legacy materials; they keep their attachment ports until
the owner replaces those connections, avoiding duplicated animated controls. The
refresh itself does not edit the project. Frozen paired rollback restores old
readers and the pre-edit graph. Presets may carry either supported direction.

Ordinary edits with unchanged Model identities/resources use the normal guarded
explicit graph commit. Mesh restore, Model addition/removal/duplication continue
through the complete asset transaction. This removes effect duplication from
material color/source/roughness edits. Binding serialization preserves existing
UUID/kind/field alias slots; rebinding disables and clears the previous expression
before installing a new source. Offline tests cover these ordering guarantees;
obsolete aliases are disabled/cleared with transactional rollback, and changes
to node identities or transaction backups force active expressions to rebuild.
the reported AE stream mismatch cause remains a hypothesis until host acceptance.

The original topology paragraphs below describe the legacy slice.

The owner reports native69/CEP72 testing has shown no errors, and requests direct
Model output plus a basic Material node. This is limited owner evidence, not a
complete undo/reopen/render qualification.

Model output1 may also feed Output input1. This explicit built-in union does not
change other port types. Material output1 is numeric metadata feeding Model's new
optional input2 (one material). Existing Model schema1 and Output schema4 remain
readable with unchanged keys; absent material preserves the unlit particle color.
Direct output creates one stationary model, centered in the layer, one mesh unit
per pixel for imported geometry (100 pixels for the builtin cube), white inherited color. Model offset/scale/rotation still apply. It has
no emitter lifetime or birth clock. A model may fan out to Output and Particle.

New independent identities: graph type org.starfieldfx.nodes.material/schema1,
native kind6, effect org.starfieldfx.node.material, AEX StarfieldMaterial. Controls
1..6: Color Source (Particle Color / Material Color), Color, Roughness 0..100%,
Metallic 0..100%, Opacity 0..100%, AE Lights on/off. Disk IDs1601..1606 are new;
metadata uses the existing layout scheme after base6. No released ID is reused.
Graph keys1..6 follow these controls, values enum uint32, color Vec3, three unit
interval doubles, bool uint32. Defaults0/white/.5/0/1/1.

Material uses GGX distribution, Smith visibility, Schlick Fresnel (dielectric
F0=.04), energy-weighted Lambert diffuse. OBJ corner normals use inverse transpose;
missing normals use geometric face normals. World position/normal interpolation
is perspective correct. Basic AE ambient, parallel, point and spot lights include
color/intensity, cone feather and falloff. No shadows, texture maps, IBL or new
particle shapes in this slice. No effective lights uses the base color so material
color remains editable before lights are created. Actual lights can make a surface
black. Capture is numeric/owned, per sample, mixed into the SmartFX cache key.

Core ABI9 appends lighting staging after the exact ABI8 prefix; ABI7/8 request
sizes remain accepted without lighting. Full adapter/Core/node paired build is
required. Snapshot11 extends Model instance records with material values, retaining
readers3..10 and writing old formats when material is absent. Binding8 admits only
new kind6; readers1..7 retain their meanings. Sequence envelope is unchanged.

M3-17 owns Render.hpp, Settings.hpp, Graph/temporal/snapshot/Model scene contracts,
the numeric lighting module and AE light capture, Material native registration,
binding/controls, isolated CEP authoring, schema identities and paired tools for
this slice. Preserve unrelated Motion/MNT edits. Qualification requires actual
registration and graph publication, color-source/roughness/metallic/opacity pixels,
light transforms/types/animated values, mixed graph/preset/undo persistence,
bounded work/cancellation, SDK build and full paired rollback. Compilation and
offline checks cannot establish AE host qualification.

2026-10-11 authoring amendment: graph Color Source0/1 maps only to native popup
1/2; graph RGB maps to native RGBA with alpha1. Roughness/metallic/opacity unit
intervals map to native0..100, and AE Lights is strictly0/1. Shared browser/ES3
preflight rejects invalid Material values before changing streams. Readback
rejects invalid native popup values instead of manufacturing a portable enum.
No ID, graph schema, ABI or native build changes are required for this CEP fix.
Actual registration DOM fixtures must enforce popup ranges and color dimensions
and create a default Material through HostPrepare before editing it.
