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

2026-10-12 owner rejects native78 for add-node host_timeout after8s. Exact77/81
is restored after owner closes AE38548;10/18/selector hashes match. Actual78 log
has15 PF graph observations with identical graph/receipt CRC833224704 and nodes7;
no successful UI publication/final record. This does not support the previous
generation-mixture hypothesis. Withdraw the CRC expression scheme from active
source; keep its archive/history. Do not raise the general8s transport timeout as
a substitute for diagnosis.

Two confirmed lifecycle gaps need repair. queue acknowledgements already recover
through polling, but readModelGraphTransaction host_timeout immediately calls end,
which releases/cancels the applying native transaction. Retry that same read with1s
backoff within the original5min deadline; no second queue/native write. Terminal
errors and explicit cancellation retain their semantics. A deadline without a
receipt remains outcome_unknown, never committed/failed by inference. Keep draft
projection and serial write lock while the result is unresolved.

EffectGraphBackup::restore physically replaces all original instances. Its saved
names/UUIDs/values do not restore host expression dependencies; success already
has a finalizer but failure did not. Call the same non-script finalizer after all
restore mutations while the reconstructed renderer holds guard1. Release only on
successful rebind; report rollback_error and keep guard1 if rebind fails. Restoring
legacy AE Controls is a valid no-alias state. No request wire, public ID/type/schema,
ABI9 change. Retain native77's expression algorithm and preserve prior Motion work.
The combined false-cancel/stale-restoration explanation is a hypothesis until host
timing confirms it. Passive UI phase clocks record entry, binding and result, not
only success; no AE launch, script dispatch or automatic acceptance.

2026-10-12 native77 is rejected by owner for deletion; CEP81 responsiveness is
accepted. Actual PID24564 journal contains61 normal guard0 frames,6 unavailable
stream258 frames and14 guard1 cancellations. AEGP final readback does not prove
PF uses the same graph/expression generation. Existing programs carry source UUID
but no graph identity, so the renderer cannot distinguish stale programs from a
missing current source. That protocol gap is confirmed; exact AE cache causality
is not. Earlier adversarial simulations did not establish host correction.

Native78 reuses the existing CRC halves and codec header; IDs/types/schema/ABI9
stay unchanged. Explicit command-generated expressions carry their graph CRC and
first resolve their own renderer through propertyGroup(1).propertyIndex, then
check its CRC halves before visiting a sibling. This uses documented expression
hierarchy/index APIs; it does not enumerate heterogeneous effects. Different
graphs produce different programs. A dedicated stale-generation sentinel returns
PF_Interrupt_CANCEL, not zero/valid pixels. PF also checks codec CRC against both
checked-out halves before any aliases, registering publication dependencies.
Historical node/rate/lifetime sampling propagates cancellation through Core.
Matching-generation unavailable/nonfinite/invalid types and checkin errors stay
strict. A fully old coherent render snapshot is not proven by this protocol to
have a live source; that host possibility stays open and must not be hidden.

Before publication, a staged program may return only its known stale marker while
the old receipt is retained. All publication streams are typed/captured before any
alias/resource mutation. After saved graph/receipts are verified, require enabled,
finite, resolved, non-stale bindings before retirement/acceptance, and again after
parade cleanup. Restore graph/receipts and exact expression/base state on failure.
Legacy delivered-param carrier retains its older unversioned path; explicit SFG1
and SFR1 paths are the deployed gateway contract. No released request wire changes.
Upgrade existing aliases on the next authored command; no load-time script/migration
is introduced. UI publish/final and PF graph journal records include CRC/node count;
strict failures include node UUID. No owner diagnostic script is required.

Reference: Adobe's official expression reference, propertyGroup/propertyIndex and
effect(index): https://helpx.adobe.com/after-effects/desktop/work-with-expressions/expression-language-reference/expression-language-reference.html

2026-10-12 owner reports native76 guard0 unavailable binding stream258; real
journal confirms guarded cancellations and failures after guard release. No more
automatic AE acceptance requests/runs per owner; keep manual host gates open and
continue development. Panel validated edits project immediately and yield for
paint before host submission. Failed commits restore the previous canvas; pending
state never certifies a saved graph. Host writes remain serialized.

Publication currently verifies aliases before physical retirement/backup cleanup.
Add an explicit final binding refresh after all structural removals and before
renderer guard release. Resolve fresh effect/stream refs from the final parade,
reinstall literal source expressions against the saved graph and verify enabled,
finite, resolved output. The saved graph/IDs/nonce/schema/CoreABI stay unchanged.
Use a separate version1 private GraphBindingRefreshRequest magic rather than
changing the released graph commit request. Full77/CEP81 pairing is required.
No script/modal/idle dispatch is added to this finalizer. Cleanup failures remain
reported committed-state failures under the existing full-backup contract.
AE cached reference behavior is a hypothesis; adversarial post-removal tests
must be labeled accordingly, not represented as host reproduction.

Final refresh also restamps previously retired slots within the reserved alias
bank: finite bases with empty/disabled text or the exact native expression prefix.
Preserve that validated base; leave sentinel slots and non-native text untouched.
Use a disabled, dependency-free `value;` text transition then clear it, since an
identical empty/disabled write has no documented cache invalidation guarantee.
An adversarial old-frame fixture requires an actual text change and verifies
finite old-graph sampling afterward. No source fallback or zero substitution.
The first unshipped frozen77/81 source2532799 is retained; final candidate adds
this retired-span coverage before publication. No ABI/schema/version migration.

2026-10-12 owner reports75/79 still raises parameter-value stream268/error516.
The install mutation tests do not include the full EffectGraphBackup lifecycle.
The duplicated renderer retains the old graph and aliases through physical
deletion/backup cleanup. Disabling its ACTIVE flag is not a render-entry contract;
checkout_render_graph currently ignores guard1/2 entirely. In addition, original
renderer resources/graph can be halfway changed while guard1 is held. These are
confirmed code/coverage gaps; which callback AE actually schedules is still a
hypothesis, not a reproduced host defect.

Define render quiescence for the existing renderer guard:0 samples normally;
1 (authored edit) and2 (private full backup) return PF_Interrupt_CANCEL before
graph/aliases/history/resources are sampled. This is an explicit canceled frame,
not successful empty pixels, suppression of dialogs or acceptance of bad aliases.
Check out/check in the guard so AE tracks its dependency; clearing it after
complete publication/restore permits a new frame from the final saved state.
Unknown/nonfinite guard values and all normal guard0 binding failures remain
errors. No persistent ID/type/schema/ABI change. Add a full lifecycle fixture
using real EffectGraphBackup and checkout_render_graph, sampling every renderer,
including disabled copies, after every mutation; prove old source failure and
normal rendering after commit/rollback. Actual AE scheduling/cancellation must
be qualified separately. Native error messages also identify the compiled build,
guard and value failure category, without changing strict guard0 validation.

Developer-only observation: explicit full unpublished BuildWindows -RenderJournal
generates an ASCII configuration header under that frozen checkout's artifacts,
fingerprinted as an AEX input, with a log path confined to its artifacts folder.
Default full builds generate a disabled configuration; CoreOnly preserves the
prior input and still requires an exact fingerprint. Parameters' render wrapper
appends bounded JSON lines (build/PID/effect identity/time/guard/result/message)
only in that opted-in candidate. No registry/environment/host-folder write or
SDK handle retention. Log errors cannot change rendering. The AE acceptance
wrapper reads only new lines, verifies actual render activity and fails on native
errors; suppressing test dialogs is not accepted as evidence of zero errors.
This is test-build instrumentation, with no public control/ID/schema/ABI change.

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
