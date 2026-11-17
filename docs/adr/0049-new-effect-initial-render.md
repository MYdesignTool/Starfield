# ADR 0049: one initial appearance for newly applied effects

- Task: UX-16, owner request and composition-preview clarification, 2026-11-16.
- Supersedes ADR0008's new-effect Control Source default only.
- Status: native129/panel123 deployed with offline checks; actual AE2023 pending. Evidence and rollback: [current state](../current-state.md).

Native128 registers new effects in legacy AE Controls mode. The first graph-panel
read initializes Emitter and Particle sibling effects and publishes Node Graph mode.
The old mode has different emitter defaults, so the composition can change even if
the panel never paints the old four-node view. Panel122 fixes only that stale view.

New/reset effects select Node Graph through the popup's `dephault=2`. Keep `value=1`
and USE_VALUE_FOR_OLD_PROJECTS so old projects lacking this parameter retain their
legacy behavior. Existing persisted source values, graph bytes, animation and native
author records are not migrated or replaced. Parameter index28/disk29, choice values,
effect match names, sequence envelope and Core ABI remain fixed. The manifest metadata
revision records the new default; it is not a new graph encoding.

Before first publication (Node Graph, ready0, revision0, guard0, and an exact match
to the factory ARB template with no optional author records), render an owned
three-node numeric graph using initial native-node defaults and real layer height.
It has no Force, AE Controls sampling, native aliases, host writes or author handles.
Use the same graph-scoped Emitter/Particle/edge identities for initial empty-layer
bootstrap, so deterministic particle randomness does not jump when author effects
are published. Existing sibling effects retain their UUIDs and values. Ready1 or a
nonzero revision always uses the saved graph and normal strict binding validation.
Guard/UI-mutation cancellation remains before either node-mode path.

The startup projection is render data, never an editable snapshot or publication
receipt. Actual author initialization still uses the existing guarded Undo transaction,
native compilation, saved receipt, rollback and failure policy. It does not depend on
render-time AEGP calls. Core algorithms and Settings defaults for legacy callers stay
unchanged. Offline checks must compare first and published numerical output, not just
the presence of three nodes; AE startup, reset, Undo and saved-project behavior remain
host qualification gates.
