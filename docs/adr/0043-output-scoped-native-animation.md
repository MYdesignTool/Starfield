# ADR 0043: Output-scoped native animation and orphan retirement

- Status: implementation candidate; owner-led AE2023 qualification remains open.
- Task: M3-20 stability repair. New feature development is paused.

## Evidence

Read-only examination of the owner's saved `testproject.aep` on 2026-10-13
found a live graph with Emitter -> Particle -> Output and two disconnected
Turbulence records. One Turbulence effect no longer exists in the saved effect
parade. Its persisted slot142 / stream240 expression still names that missing
effect and checks its UUID. The owner's startup error names stream240. The
decoded evidence and extracted graph remain confined to
`artifacts/m3-20-stability-20261013/`; the original project is unchanged.

Core evaluation already uses only Output ancestors, but native current-frame,
history and idle metadata sampling read every binding record. Consequently a
disconnected missing source can reject a valid Output frame and makes idle
metadata repeatedly retry. This saved-data inconsistency is confirmed. The
operation that left the record behind and other active-node failures remain
unconfirmed; this evidence does not establish AE cache behavior.

## Decision

Use the Output ancestor set, following every upstream edge including resource
dependencies, for native frame sampling, historical plan/proofs and idle
metadata capture. Preserve parked nodes and their authored parameters. Explicit
single-node sampling continues to read the requested node. Active missing,
nonfinite or malformed bindings still fail; no fallback pixels or zero values
are introduced.

For alias handoff, require evaluated old values only for spans consumed by the
old Output graph. A disconnected retired span can use its finite recorded
value: no old render reads it under the same scope rule. The next explicit
graph edit recompiles from the actual native inventory and can remove the
orphan without recreating a deleted effect or a load-time project mutation.

Pure node additions preserve enabled, identical expressions for existing
sources. Removals, retyping, backups and final refresh retain strict rebinding.
Do not change transport timeouts or resurrect withdrawn native78/79 proposals.

Diagnostics identify the failing node, parameter and component. Public IDs,
match names, binding record v11, sequence schema and Core ABI9 are unchanged.
Offline fixtures and compilation can establish code behavior only. No automatic
AE acceptance, process launch or project edit is authorized by this task.
