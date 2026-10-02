# ADR 0020 — Current node interaction, camera and auxiliary emission

Status: accepted development implementation; AE 2023 host qualification pending.

## Owner scope

The owner requests the current nodes to approach the observed Stardust interaction:
native and CEP edits agree, a node click selects its native effect, parameter
labels/order/steps follow the visible reference, particles respond to the camera,
and the Emitter can emit from particles as an auxiliary source.

## Native edits

`PF_ParamDef::uu.id` belongs to PARAMS_SETUP. At edit time the union contains
change flags. Dispatch native edits by the registered runtime index and validate
the parameter type and saved UUID values; never compare that union to a disk ID.
Rendering retains immutable graph snapshots. No render-time sibling-effect reads
or external process communication is introduced.

## Selection

CEP resolves the saved node UUID to its current effect instance, then sets native
property selection. Output selects the renderer. Selection changes no graph
parameters and creates no undo group. Requests are coalesced during fast clicks.

## Depth contract

Extend the host-independent render request with a value-only camera snapshot and
extend the Core C ABI to version 2. The adapter owns SDK matrices and suite
lifetimes; the Core receives only numbers. Camera matrices are row based in the
SDK and inverted before projection. Capture active-camera geometry during Smart
Render, retain transparent CPU output and ROI clipping, sort visible sprites
back to front, and include camera use in the host dependency flags. A missing
camera uses host default geometry when available, otherwise flat layer mapping.
Canonical +Z follows AE Z, away from the viewer in the default view (correcting
the formerly inconsistent ADR 0003 prose before release). Non-square source/comp
PAR and extreme layer transforms still require AE qualification.
Unsupported/singular explicit camera geometry returns a
typed error rather than NaN coordinates. Compilation is not camera qualification.

## Auxiliary contract

Keep one Emitter AEX. Append an Emitting popup (Default/Auxiliary), Emit Chance,
Emit Life Start/End percentages and inheritance percentages. The optional upper
port receives parent particle streams; Auxiliary -> Particle retains the normal
lower output port. Default Emitters have no parent input; Auxiliary Emitters
without a connected parent are transparent. Cycles remain invalid. Births are
evaluated at absolute time, sampling parent positions at each child's birth;
children survive independently of their parent's later death. Output owns the
global cap. Evaluation is cancellable and explicitly bounds recursive work.
The independent first implementation uses a comp-zero emission clock per parent
stream, deterministic chance per parent identity and a 0..100% parent-life window.
It does not claim the reference's timing/distribution kernel. Switching Emitting
to Default in CEP also removes parent edges in the same saved transaction. For
native mode changes, disconnect parents first; an active Default parent input is
rejected. Auxiliary graphs currently show live count unavailable rather than an
incorrect simple emitter estimate. A true evaluated live counter is a follow-up.
Speed Over Life, Inertia, Origin Time Sample, Orient and Time Offset have no
completed contracts and are not advertised as functional controls.

## Development upgrade and rollback

This project has no production users. Append new disk IDs, bump the Emitter node
schema and native layout/manifest revisions, build a paired adapter/Core ABI 2
bundle and require fresh development effects. No old-project migration or
compatibility carrier is added. Keep build 12 as the one-step deployment backup.
Only claim the exact owner-exercised AE 2023 behavior after their acceptance.

## References

Local May 2023 SDK `AE_Effect.h`, `AE_GeneralPlug.h` and the SDK guide's
[camera interface](https://ae-plugins.docsforadobe.dev/aegps/aegp-suites/)
and [SmartFX selectors](https://ae-plugins.docsforadobe.dev/smartfx/smartfx/).
The owner's screenshots are behavior references, not implementation sources.
