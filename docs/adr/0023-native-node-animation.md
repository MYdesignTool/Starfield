# ADR 0023: native node animation through owned render inputs

Status: development implementation; AE 2023 owner qualification required.

Build 17 native edits now take effect according to the owner, but public node
controls were registered CANNOT_TIME_VARY. Removing that flag alone would leave
rendering on a constant graph. P-02J completes both paths.

Native effects own keyframes and user expressions. Identity, topology, layout,
guards and over-life knot banks remain constants. The main effect appends 512
hidden numeric render inputs (indices 98..609, disk IDs 1000..1511). UI commits
bind these through AE expressions to native properties by their eight-word UUID,
so node reorder and rename do not redirect them. These are derived dependencies,
not duplicate authored controls. Capacity overflow rejects a transaction.

Optional graph record 0x8002/version 1 saves typed raw fields and binding slots.
Pre-render checks out only its own PF streams at the requested layer time, then
reuses native-to-core conversion. No AEGP query/mutation runs on render threads.
AE evaluates source keyframes/expressions and tracks their dependencies, including
renders with CEP closed. Failed graph publication restores prior binding expressions.

Main manifest becomes 22; existing indices/disk IDs, node schemas/layout and Core
ABI 2 remain stable. Recreate development effects for qualification; no old-project
migration. Build 18 is unreleased.

Settings are sampled at the requested frame. The stateless kernel applies that
frame's settings to survivors; birth-time emitter trajectories and integrated
animated-force history require a separate history contract. Curve knots stay static.

Qualification: public stopwatches; constant metadata; scalar/point/color sampling
at multiple and reverse-order times without AEGP; rename/reorder ownership;
failed-publication restoration; owner AE interpolation, CEP closed, undo/reopen.


Candidate evidence: 761 scoped adapter/registration checks, actual generated
expression execution and CEP keyed-write protection, and all five May 2023 SDK
Release /MT AEX links pass. No AE session operated; owner qualification pending.
