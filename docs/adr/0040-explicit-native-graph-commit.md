# ADR 0040: explicit native graph command and saved publication

- Status: implementation candidate, AE2023 acceptance open.
- Task: M3-17, native69 / CEP72; supersedes ADR0019's supervised CEP trigger.

## Failure and evidence

The owner reports first-effect initialization alternates between binding failure
and an unacknowledged graph transaction. This does not identify which host callback
failed. The old gateway assumes Property.setValue sends USER_CHANGED_PARAM and
that callback params/change flags become saved streams before the next script read.
The May2023 SDK documents supervision for user interaction with a standard control;
it provides no scripting-delivery guarantee. Previous gateway fixtures acknowledge
from setValue, and old carrier tests invoke the callback directly. Neither tests
the missing-callback case. This is a demonstrated contract/coverage gap, not a
claim that AE's exact failure has been reproduced.

## Decision

2026-10-12 owner reports native74/CEP78 still raises parameter-value errors on
deletion (different alias streams, screenshot stream258). Physical removal
ordering was necessary but did not cover expression mutation: install disables
and clears aliases, writes the unavailable sentinel, and retires old spans
before the new graph is saved. Rollback restores the sentinel base before
re-enabling the original expression. Both expose an invalid value to the still
published graph. This is a source ordering defect; the exact AE callback timing
remains a hypothesis until host evidence.

The native75/CEP79 correction keeps retired expressions live through graph publication
and saved-value verification, then detaches them before deleting originals.
Active expression replacement uses a finite handoff: evaluate any previously
published alias while its source is still present, validate it, write that
resolved value beneath the enabled expression, then disable/replace/re-enable
the expression. A new unused alias uses its independently compiled source value.
No sentinel is written into a span used by the published graph. Source resolution
and final expression evaluation remain mandatory; invalid old/new bindings still
fail rather than becoming zero. Retired aliases retain their validated resolved
value as the disabled base, protecting an already checked-out old graph without
keeping a dependency on the removed effect. Rollback restores expression text
and enabled state before restoring the exact old base. Retirement failure uses
the same graph/scalar/alias rollback as publication failure. No ID/schema/ABI
change. Test reentrant sampling at every expression/value mutation, retirement
after publication, partial failures and rollback; do not infer AE qualification
from fixtures.

Frozen82c460f full May2023 ten-module SDK, seven-kind mutation-hook ASAN and
complete panel audit pass; deployed13:54+08 with fresh AE0,10/18/27/selector and
backup10/18 independently matched.74/78 rollback remains. Actual AE correction/
undo/reopen are open; source/receipts/one-step rollback in current-state.md.

2026-10-12 deletion correction (native74/CEP78 deployed11:18+08): ordinary/Model prepare previously
physically removed source effects before graph/binding publication, exposing old
aliases to missing sources during script-triggered re-render. Owner reports
parameter-value stream144/error516 though deletion succeeds. This ordering gap
is confirmed statically; exact AE callback timing remains unqualified.
All graph deletions use the existing full native EffectGraphBackup transaction,
even without Model assets. Reserve transient node guard3 for pending removal,
distinct from backup2 (which rollback preserves). Native/CEP node inventories and
asset selection exclude3, while original effects/animation remain alive. Publish
and verify the desired graph/retired aliases, then remove original IDs. Any failure
including partial removal returns to the native full backup restore. Ordinary
submitGraph refuses removal before mutation so direct callers cannot bypass this
contract. Source edits/additions retain ordinary fast commits when no assets change.
No parameter ID/type, record schema or CoreABI changes; guard already permits
signed32 positive values. A complete native74/CEP78 pair is required because old
native readers do not recognize3. Guard3 is never a successful saved author state.
Do not suppress dialogs or accept sentinel/missing binding values to hide the gap.
Frozen41777fc full May2023 ten-module SDK, all-seven panel/delete-render/reload/
ES3, full backup/Model/asset/Circle/frame ASAN and paired hash/rollback gates pass.
Actual AE deletion warning correction is not yet confirmed; current-state.md
records exact receipts, scope and one-step undo.

CEP writes the existing nonce (index41/disk43), then synchronously invokes
`Starfield Commit Node Graph`. The resident Host finds the unique pending guarded
renderer across the open project, independently of selection. It rejects ambiguity,
invalid nonce/type, excessive inventory, and same-thread nested commands before
publication. Guard2 transaction backups are excluded. An already saved nonce is
idempotent. No ExecuteScript, dialog, idle queue or retained SDK handle is introduced.
The existing UI exclusion covers idle; Model's outer exclusion can own the scope.

The command sends a versioned private GraphCommitRequest to that renderer through
EffectCallGeneric. The renderer compiles current saved node streams, using the same
publisher as native control edits without applying an authored-value override.
It reads live guard/nonce/ready and previous graph, installs/validates bindings,
publishes source/revision/CRC/graph/receipt through AEGP, and reads all saved values
back. A failed or partially applied write restores graph, scalars, receipt and
bindings. The Host writes a negative terminal nonce on rejection. A missing generic
acknowledgement cannot become success. Compile/binding/publish stages are reported.

Bootstrap preflights command availability before adding effects. On failure it
removes only newly created nodes, restores prior wiring/ready, and clears only its
own guard. Deliberate deletion of every node stays Output-only. Normal edits reject
a foreign guard before mutation. The gateway always reacquires saved Properties.

## Compatibility and bounds

No parameter type, disk ID, effect matchName, packed-version convention, sequence
schema, node schema or Core ABI8 changes. The hidden nonce's supervision flag is
removed. Cached old supervision flags are harmless: EffectMain no longer dispatches
graph publication from USER_CHANGED_PARAM. Its legacy carrier helper is retained
only for existing direct-call adapter fixtures; it is not the production CEP path.
The new command and receiver require the complete native69/CEP72 pair. A missing
command fails before bootstrap/edit mutation; no legacy fallback or timed replay.
Private borrowed handles live only during the synchronous call. Project scan is
bounded to65,536 item/layer/effect visits. If multiple pending renderers exist, none
is published; stale guards require explicit recovery rather than guessing a target.

## Required evidence

- Real gateway with setValue having no callback: bootstrap, missing command,
  missing receipt rollback, retry, add/delete/copy and guarded transactions.
- Actual resident command hook: backup exclusion, selection independence,
  ambiguity, idempotence, missing receiver, typed reads, nested/worker rejection,
  outer Model exclusion and reference cleanup.
- Actual native compiler/binding/serializer/publisher: explicit success without
  USER_CHANGED, receipt-write rollback, retry, missing generic geometry/time,
  Output-only deletion; standard and ASAN.
- Complete frozen May2023 SDK build, ES3 and full panel/registration matrix,
  paired deployment/hash checks and one-step rollback.
- AE23.5x52: first-effect initialization, repeated refresh, graph and Model edits,
  failure recovery, undo/reopen. Offline evidence cannot close this host gate.
