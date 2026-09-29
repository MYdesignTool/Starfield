# ADR 0014: Store node layout in the AE project

- Status: accepted for implementation; After Effects 2023 host qualification remains open.
- Date: 2026-09-29.
- Depends on ADR 0009 and the append-only parameter identity contract in ADR 0001.

## Context

Node positions describe the effect instance's authored graph layout. Saving them in
CEP `localStorage` makes the arrangement machine-local, separates it from the effect,
and fails when a project moves to another machine. AE already owns effect parameter
streams across project save/reopen, effect duplication, and undo.

The graph itself is stored as `PF_Param_ARBITRARY_DATA`, which the owner's AE 2023
ExtendScript probe could not read. Layout does not need to share that stream: the
protocol-v1 panel shows a fixed four-node parameter view, so eight ordinary scalar
values are sufficient and remain independent of rendering and graph evaluation.

## Decision

- Append eight hidden, non-animated standard float-slider parameters after the existing
  parameter index 32: X and Y for Emitter, Force, the logical Particle card, and Output. Their
  persistent IDs are 33–40; earlier indices and IDs do not move.
- Keep IDs 37–38 and their underlying `layout_appearance_x/y` storage keys unchanged;
  the panel now maps these saved coordinates to the Particle card. If all four positions
  remain at the old default coordinates, present the new top-down default arrangement
  (Emitter → Particle → Force → Output) without mutating the project. Authored positions
  remain intact and are still written to those AE-owned streams when a drag is committed.
- Use a symmetric coordinate range of −1,000,000,000 to +1,000,000,000 canvas units.
  The canvas derives its element dimensions from the spread between nodes, so absolute
  positive or negative coordinates do not expand it toward a browser element-size cap.
- Extend `getState` with the four node positions. Add `setNodeLayout`, which validates
  the complete eight-value layout and the current target/revision before writing all
  values in one AE undo group. On failure, restore the prior values where possible and
  report whether rollback failed.
- The panel sends a layout transaction when a node/group drag completes. Positions are
  read from the effect on each accepted state response, so project reopen, effect copy,
  and AE undo/redo use AE-owned values. No node-position data is written to
  `localStorage`.
- Canvas pan, zoom, and selection remain transient view state. On the first state for a
  target, center the saved node arrangement in the viewport.
- If the running effect predates revision 7, or AE scripting cannot resolve the new
  streams, `getState` still returns the four current default positions and marks project layout
  persistence unavailable. The panel remains usable for that session and clearly reports
  that node moves will not survive panel reload until the matching plug-in is installed.
- Layout values are not supervised render controls and are not copied into the graph
  arbitrary-data stream or core `Settings`.

## Qualification gate

Source implementation alone does not prove the scripting DOM can access the hidden
streams. In AE 2023.5.0 Build 52, qualify name-based reads and `setValue`, save/close/
reopen, effect duplication, and undo/redo for a node move. The new parameters append to
the effect schema; projects without them should receive their declared default layout.
Do not claim this behavior host-supported until that pass succeeds.
