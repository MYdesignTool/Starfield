# ADR 0008: AE owns the persistent graph parameter

- Status: accepted implementation contract for G-04; host qualification pending.
- Date: 2026-09-27.
- Supersedes the sequence-data ownership assumption in the initial architecture.

## Storage and undo

The graph is a non-animated `PF_Param_ARBITRARY_DATA` parameter, appended at index
and disk ID 14. Its handle contains canonical schema-1 codec bytes, never C++
objects, host pointers or caches. AE uses the arbitrary-data lifecycle callbacks
to copy, flatten, restore and compare these values. The binary graph schema is
unchanged. Sequence data is unused; there is no second authoritative graph copy.

An editable graph is project state, so it must participate in parameter changes,
host invalidation and undo. The supplied May 2023 `AE_Effect.h` explicitly states
that changes reported through `PF_ChangeFlag_CHANGED_VALUE` in
`PF_Cmd_USER_CHANGED_PARAM` are undoable/redoable. Graph edits must use this route
or a documented parameter stream transaction, never mutate render/sequence state.

All eleven arbitrary-data callbacks are implemented. External bytes and text are
size-bounded and parsed before accepting a value; unknown versions fail instead of
resetting the graph. Print/scan uses `SFLDGRAPH1:` followed by hexadecimal codec
bytes. Interpolation is disabled; if requested, it uses hold-left until t=1.
Copies own separate AE handles. No callback requires a non-null effect instance.

## Parameters and migration

Manifest revision 6 regrouped the Effect Controls Window into Emitter / Particle /
Physics / Render topics, which renumbered every parameter: the hidden graph data is now
index 31, Control Source 29 and Capture Current Controls 30 (topic markers occupy
indices, see `docs/parameter-mapping.md`). Disk IDs are unchanged.

- 31: hidden graph data, arbitrary callback ID 31, no keyframes.
- 29: Control Source, popup `AE Controls | Node Graph`, no keyframes. **Defaults to
  AE Controls** (revision 6): a freshly applied effect must drive the visible controls,
  and Node Graph is opt-in through capture or the panel. Old projects receive AE
  Controls via `PF_ParamFlag_USE_VALUE_FOR_OLD_PROJECTS`.
- 30: Capture Current Controls, supervised action button. Capture explicitly
  replaces the stored graph with an emitter/force/appearance/output graph of current
  control values and switches to Node Graph in the same user-change transaction.

Old animated controls remain stored and are still sampled in AE Controls mode.
Capturing creates a constant snapshot, not a conversion of historical keyframes.
Switching modes alone never overwrites a stored graph. Control labels and panel
layout will make these sources clearer in P-02/M3-03; the current controls remain
available during integration. Default node/edge UUIDs are scoped to the graph;
copying an effect intentionally preserves them.

## Render transport

Pre-render checks out Control Source and the selected source parameters, creates
an immutable owned graph, and checks all parameter handles back in. Smart render
uses that exact snapshot. This records parameter dependencies before AE chooses
cached output and prevents an edit between selectors from changing the snapshot.
Legacy controls also convert through the graph constructor after Settings bounds
validation. Rendering never changes persisted graph data.

The time-dependent particle renderer declares `PF_OutFlag_NON_PARAM_VARY`, as
required by the May 2023 header for effects changing with frame time even when
parameters are constant. PiPL/runtime flag equality remains compile-time checked.
MFR, GPU and Compute Cache flags remain disabled.

## Qualification

Native callback tests can verify byte round-trips, independent handles, malformed
input, bounded buffers, disposal and failures. They cannot prove AE's save/reopen,
effect duplication, old-project default selection, UI refresh, or undo behavior.
Those are explicit AE 2023 host gates; until exercised they remain unqualified.
The panel and the force/appearance kernels are separate remaining Alpha work.

## Primary reference

Local Adobe May 2023 SDK `Examples/Headers/AE_Effect.h`: arbitrary-data callback
structures, `PF_ParamFlag_USE_VALUE_FOR_OLD_PROJECTS`,
`PF_ChangeFlag_CHANGED_VALUE`, and `PF_OutFlag_NON_PARAM_VARY` comments.
`Examples/Headers/AE_EffectCB.h`: host handle allocation/locking/size callbacks.
