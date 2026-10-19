## M3-20 / native Starting With author transaction — 2026-10-18

Native92 restores the retired prefix editor through an AE custom control and
native text dialog. ADR0039 preserves the existing numeric bank and prefix
semantics. The explicit modal owner snapshots all44 values plus source UUID,
mode, guard, project, composition, layer and time; no PF checkout, suite or
source stream crosses the dialog. A second capture rejects stale targets.

Only a complete validated UTF-8 prefix edit may use the owned Motion publication
entry. MotionAuthorTransaction verifies the exact old bank, acquires the source
guard, writes/rechecks each field, then accepts only after graph publication and
guard release verify. Failure rolls back bank, graph and guards together; failed
restoration remains explicit. Normal mode transactions retain their semantics.
The compiler must honor read_one_d(record=false) for edit overrides too: the
constant string belongs in the graph, never in numeric animation binding records.
Native registration retains disk1715, all43 chunk IDs, names, count and schema.
The current Light Path follow-up changes only temporal resource retention and
the custom control drawing; it does not alter this author-intent transaction or
the saved numeric bank.

## M3-20 / ExtendScript conditional association — 2026-10-16

The owner reports CEP95/AE23.5x52 snapshot raw1/Core0 for UUID
7af2e9d4a5044669a2ba4aec1eec7a6b, with native named/disk samples1 and guard0.
This localizes the discrepancy to mapping, not sampling. The exact expression
`motionMode===1?1:motionMode===2?0:2` yields0 for raw1 when parsed with
ExtendScript's reported left association. This matches the owner trace and the
firsthand Adobe report https://community.adobe.com/bug-reports-528/bug-in-extendscript-s-conditional-ternary-operator-1216218 .
Node/Acorn ES3 parsing alone did not model this interpreter behavior.

CEP96 replaces both Motion enum mappings with explicit branches and adds
explicit grouping to other nested conditionals in shipped JSX (Model bounds,
Transform/Motion coordinate conversion, validation limits, sorting, Turbulence).
The canonical native-record factory must remain identical. Extend syntax gates
to reject ungrouped direct conditional branches in every shipped JSX source.
Regression tests exercise all Motion modes and model the old association as a
negative control; actual post96 AE behavior remains a host gate. No IDs, ABI,
native writer, schema, performance retry or features change.

## M3-20 / one Motion snapshot reader — 2026-10-16

The owner reproduced CEP94 on AE23.5x52: node5ece0f138ac34ad496220df785d3cc5f
reports expected Core1 / snapshot0, while all six fresh named/disk ordinary,
zero/current samples are native1 and guard0. CEP93/94 did not close the host gate.

The normal reader still conditionally chooses valueAtTime using JavaScript
typeof and silently falls back to .value, whereas diagnostics call valueAtTime
directly. Remove the alternate scalar path for this nonanimated selector. Call
the required AE sampling API directly; a missing/failed sample rejects the read.
Determine the control kind before obtaining its Property, then consume the
Property immediately without another effect getter. Do not infer that the host
actually took the fallback: the evidence does not record that branch.

Retain the exact raw sampled popup and mapped Core value as plain request-local
evidence, reset for each inventory read and request. Include it only on mismatch;
never use a later diagnostic sample or requested value to repair the snapshot.
No additional successful-path SDK read, retry, selector write, enum migration or
feature is introduced. CEP95 is deployed under M3-20; native89 remains unchanged.

## M3-20 / failure-only Motion evidence — 2026-10-16

The owner reports the failure persists after retiring the CEP numeric editor.
Installed native89/CEP93 hashes agree with their deployment receipt; the supplied
UUID also appears in a native88 journal. This does not establish which generation
produced the latest failure. Do not infer a new host cause from either fact.

CEP94 installs a bounded diagnostic attached to the existing mismatch error,
captured before rollback. Include gateway/host version, snapshot Core values,
unique full UUID resolution, guard and fresh named/disk ordinary, zero-time and
current-time popup samples. Keep only plain values; diagnostic getter failures
must not replace the original mismatch or prevent rollback. Successful comparison
does no additional host reads. No retry, coercion, extra writer, host file write,
public parameter/schema/ABI change or feature is introduced. This is evidence
collection, not a claim that Motion is repaired or AE is qualified.

## M3-20 / CEP93 installed — host proves a readback discrepancy — 2026-10-15

Owner-run AE23.5x52 trace before rollback observes the exact failed UUID with
Motion Type1 through all fresh named/disk and time samples, guard0, while the
cached gateway snapshot says Core0. This supersedes the unproven deferred-setter
cause as an explanation of the current failure. Native author intent/mode
persistence already succeeded. Fix gateway reference ownership under ADR0009;
do not add another mode writer, coerce a saved enum or weaken strict comparison.
The stale-Property negative control fails92 with0 versus1 and passes93; actual
post-install addition/undo/reopen remains an AE gate. Native89's Motion code is
unchanged apart from the shared guarded-sync implementation/version pairing.

# ADR 0045: explicit Motion author intent and complete callback delivery

## M3-20 / CEP92 — one selector writer — 2026-10-15

The owner observes new Circle expected Core1 / saved0 on native88/CEP91. CEP91
still uses a scripting popup setter before native88's definitive AEGP write.
CEP92 stops the scripting selector write; guard4/5/6 remains the verified intent,
and native88 alone writes/verifies/releases that selector. Remaining controls
stay guarded and use the requested mode for shared Speed conversions. No native,
ID/schema/ABI change, retry, mode coercion or readback relaxation is introduced.

The adversarial full gateway/DOM fixture queues an old popup delivery from a
script setter and applies it after native publication. CEP91 reproduces exactly
the expected1/actual0 publication failure; CEP92 passes the same ordering. This
demonstrates why two writer paths are unnecessary, not AE's precise callback
cause. Native saved-mode/guard checks and the gateway's final strict comparison
remain; actual AE addition/persistence/undo acceptance stays open.

- Status: native87 rejected; native88 lifetime repair deployed2026-10-14, host gates open.
- Current pairing: native88 / Core ABI9 / CEP92; current paired rollback restores88/91.

Matched dump/PDB and x64 unwind evidence identify an access violation inside
AEGP_GetStreamType called by MotionAuthorTransaction::read during the post-graph
accept step. Retained source references across publication/effect release are a
lifetime hypothesis; exact invalidation needs evidence. Fake SDK references did
not model this invalidation. Preserve87/90 for analysis; no replacement candidate
was authorized during the rollback pause. The owner has now requested Motion
repair and architecture/performance hardening, with new features paused. See
`artifacts/m3-20-native87-crash-20261014/analysis.md`.

## Resumed lifetime repair

Transaction state owns only UUIDs and numeric before/desired values. Source
effect references and streams are local to an operation, with streams disposed
before their effect owner. Reacquire each stream after writes and resolve unique
kind+UUID identities from the current parade at prepare, accept and rollback.
Never retain a source stream across graph/alias publication or effect disposal.
Resolve the complete changed-source set in one inventory pass per phase with
sorted UUID lookup; reject missing/duplicate identities before phase writes.
Inactive Motion sources allocate no transaction entry. Native edits unrelated
to the Motion selector skip the author-intent inventory entirely.

Rollback is explicit and returns an error to the publication owner; incomplete
restoration must not be reported as restored. Restoration verifies both the mode
and guard and keeps a failed source guarded where the host permits the write.
Adversarial fake SDK tests expire source refs on effect release and graph writes;
they test the conservative lifetime contract, not AE's exact invalidation cause.
Full host qualification remains open. Public IDs, schemas, ABI and features stay
unchanged. M3-20 also owns GraphParameter/NativeGraphCommit serialization reuse
and focused tests to avoid encoding the same publication twice.

The owner still observes Circle Core1 becoming Look At2 after native publication,
in both authoring interfaces, and an intermittent deletion dialog identifying
actual Motion / expected Force. Native86's guarded scripting write/fresh getter
and strict published-mode check did not close that host failure. Do not claim
the screenshot establishes exactly which callback or host cache caused it.

The full source trace finds consistent public IDs and enum conversions. But
the bridge has no explicit native author input: its compiler trusts a subsequent
saved getter after scripting independently reports successful authoring. Existing
fixtures mostly share one backing value between these two authorities. Add real
Motion-compiled NodeGraphSync tests with delayed saved values, the actual panel
publisher, native write/readback rejection and exclusion callbacks.

## Private transient guard extension

Keep Motion disk1701, popup Circle1 / Light Path2 / Look At3 and Core modes1/0/2.
No released ID, match name, parameter order, graph/sequence/binding schema or
Core ABI changes. Existing guard0/1/2/3 retain their meanings. Only Motion adds
transaction states4+Core mode:4=Light Path,5=Circle,6=Look At. They live in the
existing hidden nonanimated guard and represent an explicit pending author intent.

CEP writes the existing controls under guard1, then holds the bounded intent.
It verifies the intent rather than certifying a popup solely from `.value`.
The native publication owner captures the old popup/guard, sets the requested
popup through AEGP, verifies its saved value, then compiles/publishes bindings and
graph. It rechecks the popup before and after releasing the source guard and
checks the saved guard is zero after its release; a discarded release is a
failed transaction, not a successful edit leaving the source permanently busy.
Rejection restores the source values; an unconfirmed source restore stays guarded. Existing
outer graph/asset rollback and strict final mode comparison remain in force.
Mode readback uses `valueAtTime(0,true)` where available, because this selector
cannot animate; mocks without that API retain their ordinary scalar getter.

Native Effects Controls edits use the delivered mode to perform the same native
source write and mark the delivered parameter CHANGED_VALUE after success. Other
Motion parameter edits do not replace the selector. No retries or coercion of a
disagreeing saved mode are added.

Successful publication clears every consumed intent to0. Bootstrap/read/idle do
not interpret a pending intent as permission to mutate. Existing projects in
states0..3 load unchanged. Interrupted transactions remain guarded and recover
through the existing explicit full rollback; restore paired native/CEP backups,
never an old executable against a half-completed transaction. This is a private
transaction protocol extension, not a persistent project schema migration.

## Callback lifetime protection

The SDK describes PF_InData.num_params as the number of received input parameters.
USER_CHANGED_PARAM must match the node's complete registered count before code
reads its guard/UUID. Generic callbacks keep their separately documented context
contract. A zero, partial or foreign-kind delivery defers without SDK access.
Motion curve events likewise require their complete delivery before reading the
bank/UUID, while CLOSE_CONTEXT still releases local UI state without a delivery.
Take only the layer identity from PF; stop opening parameter streams through
GetNewEffectForEffect on the borrowed PF effect binding. Resolve a fresh unique
installed kind+saved UUID in the current parade. Retired/guarded/absent sources
defer and duplicate live identities reject. A stale former Force binding cannot
be used as a Motion parameter layout. No SDK handle persists across callbacks.

Every source supervision callback holds the resident Host's UI exclusion
through its SDK work. Main-effect supervision and Motion Blur UI updates also
defer while that exclusion is held, rather than relying on a possibly older
delivered guard. This closes confirmed protection gaps; actual causality for the
owner's Motion/Force dialog remains a manual host qualification gate. A private
source-scope begin/query distinguishes these callbacks from the Model Host owner;
the graph command rejects entry during a source scope before SDK access. Model's
existing explicit command while it owns the generic exclusion remains valid.

All changes remain M3-20 stability work; no feature expansion, AE launch or
automatic acceptance. Prepare in artifacts while AE runs and complete the
applicable offline suite before deployment. Preserve accepted panel response,
Output scope, orphan retirement and append reuse.
