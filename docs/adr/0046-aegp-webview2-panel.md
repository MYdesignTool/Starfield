# ADR 0046: AEGP Panel with WebView2, existing node behavior retained

## STB-20: JSON-independent clean-engine startup — 2026-11-16

The gateway owns a private ES3 JSON parser/serializer for plain protocol data.
It does not read, create or replace the host's global JSON, or depend on another
extension warming that global. Optional Model and preset-file transports borrow
the paired gateway's codec. Page-generated loader-error replies contain their own
small string encoder, so even a gateway-load failure can return the real reason
before any gateway codec exists. Cached gateway calls keep the same independence.

Parsing uses JSON grammar, never eval or Function; reject trailing tokens, invalid
escapes/control characters, __proto__ object keys, and nesting beyond64. Serialization
handles plain arrays/objects, strings, booleans, finite/nonfinite numbers and omitted
undefined fields; cycles and nesting beyond64 reject. Date/toJSON/reviver/replacer
hooks are outside this private plain-data interface. Existing transport byte bounds,
request validation, identities, guards, mutations and error retention remain in force.
No new script-loaded file, host setting, native/Core change or startup retry is added.
Clean Node VM/ES3 syntax evidence is not actual AE startup qualification.

## STB-08: failure evidence in the existing busy response — 2026-10-27

The existing script engine may retain bounded plain diagnostic data separately
from transaction sessions:32 recent events, at most8 first failures, and at most12
events in each captured trail. A failure key includes the saved project fsName
and pinned renderer target. Valid new guard0 acquisition/request acceptance may
clear that key's previous failure. Busy/read retries, gateway reload and other
targets cannot replace its first cause while retained; bounded oldest-key eviction
is allowed. No AE objects, properties, graph payloads or assets are retained.

The busy response may append this evidence. Its reader reconstructs only bounded
plain string fields and refuses invalid/unknown shapes and responses over32768
characters. Missing, unreadable or overlong saved project identity means engine
history only, never evidence about the present lock. Script-global corruption,
getter/serializer failures or diagnostic allocation failure cannot change author
commands, guard ownership/release, rollback or the original error outcome.

The recorder observes guard acquisition/release, native requested/saved nonce,
and existing Model prepare/commit/result boundaries. It introduces no file I/O,
new host command, wire operation, public parameter, schema or Core ABI change.
It cannot reconstruct unrecorded earlier failures or authorize lock recovery.
This is future-failure evidence, not qualification of the owner's exact AE bug
or performance; no AE/GUI action is authorized by this change.

## STB-02: diagnostic content and bounded failure details — 2026-10-26

ExecuteScript's optional diagnostic is a host-owned string, not an error flag
or a bounded response payload. A positive allocation size does not make an
empty NUL-first string an error, including allocations larger than8192bytes.
Classification reads only its first byte after a successful size/lock; it does
not allocate or scan the declared capacity. Missing suite functions, failed
memory operations and nonempty diagnostics still fail closed. A nonzero SDK
execution status remains a failure even if diagnostic text is empty.

The panel keeps host_script_failed, but includes the actual AE status and at
most1024 diagnostic bytes as display text. Truncation, invalid UTF-8 and memory
failures cannot create success or discard the original failure. Error text is
JSON-quoted, never executed, and normal SDK handle/quiet-error ownership stays
paired. No automatic retry, timeout change, modal alert or project edit is
introduced. Model/preset clients share only the corrected classification.

The former size guard is independently reproduced as a false rejection in the
fake SDK suite. Whether it caused the observed real AE panel banner remains
unproved. Candidate111 requires separate owner-approved AE verification; the
latest instruction prohibits unapproved computer interaction.

## PERF-03 / panel104: continuous additions and receipt adoption — 2026-10-25

The local panel accepts independent node additions while an addition batch is
pending, using ADR0040's bounded single-writer queue. Draft UUIDs and positions
remain stable across its own receipts, so a receipt arriving during the next
palette drag does not invalidate that gesture. External graph, geometry,
resource or target adoption changes a separate gesture epoch. Other topology
edits and target pin changes remain exclusive while additions are pending.

Matching saved drafts update the confirmed snapshot without rebuilding the
canvas. Success no longer performs an immediate full getPanelState after each
addition; the existing adaptive pulse reconciles after the queue settles.
Failures/warnings discard unsent drafts, keep confirmed additions and request
authoritative refresh. Target changes ignore old local callbacks; in-flight
host writes retain their original validated target and existing lifecycle rules.

Gateway/cache markers advance together to104. Native106, Core ABI9, selectors,
parameter IDs and all render algorithms remain unchanged. Existing ordinary
submitGraph transactions still perform all native validation and rollback.
Batching reduces repeated author compilation/binding/readback work; it does not
make AE native effect registration asynchronous or qualify AE responsiveness
from simulated delayed callbacks. A batch is one native Undo action, and actual
continuous-add/Undo/save/reopen tests remain owner host qualification gates.

- Status: accepted by owner after native98/panel100 retest; web_panel source migration explicitly authorized, 2026-10-19.
- Owner request: 2026-10-19. Baseline commit c43e293 (native96 / CEP99).
- Task: P-04. Windows x64 / May2023 SDK / AE2023 only.
- Completion: old CEP registration moved outside the scan root after successful UAC; the web_panel migration is integrated into local main. Exact installation and acceptance evidence remain in current-state.md.

Current source lives under web_panel/; do not recreate the retired cep_panel/
directory or auto-register CEP. The owner accepted the canvas, preset repair and
save-confirmation/quit retest, then authorized the source rename and recent-five
backup retention. Actual Windows registration removal and the installed pairing
are recorded only in current-state.md. Owner release acceptance is separate from
an instrumented full qualification matrix; unreported scenarios stay unreported.

## Window menu cleanup — P-05, 2026-10-19

Owner authorized keeping only the graph and preset panel entries in Window.
Native100 keeps the six internal command names, allocated IDs and original
handlers, registering their definitions with SDK AEGP_Menu_NONE. Panel101
resolves them from a versioned session map prepended by the native host to each
safe idle script call. IDs are never persisted or guessed. Invalid/missing IDs
in a present map fail closed; only an absent map permits older-host named-menu
fallback for explicit rollback. No app method is monkey-patched. Registration
failure publishes ID zero and handlers reject zero callbacks. The graph panel
flyout exposes Diagnostics > Repair Selected Graph, with valid/live panel,
main-thread, host readiness and transaction exclusion checks. Repair remains
an explicit user action through the existing native repair handler.

Offline fake-suite/JS evidence is separate from real AE Menu_NONE dispatch,
flyout visibility, preset/file/Model and save/quit acceptance. Current evidence
and installed pairing belong in current-state.md. Historical recovery scripts
retain their dated frozen baselines; they are not current deployment commands.

The General AEGP owns the dockable graph panel and a separate modeless preset
window. Reuse all current HTML, graph/preset JavaScript, optional Model modules,
and the existing generation-checked JSX/native transaction gateway. The small
WebView2 adapter implements the four CEP transport methods used by that UI.
This replaces the CEP container, not the ExtendScript authoring contract.
Do not restore retired inspector/Appearance controls or alter any node IDs,
parameters, animation, graph serialization, Core ABI or rendering contract.

Only bundled local content at the exact two allowlisted HTTPS virtual-host
documents can send bounded messages. Navigation, frames and new windows are
restricted. No Node integration, localhost server, registry or environment
switches. WebView2 callbacks only enqueue owned strings, never call AE suites.
AE scripts execute from the resident main-thread idle hook, outside existing
optional idle exclusion. Keep request ordering, generation/revision checks,
readback, rollback and queued Model/preset operations intact. Queue admission
has byte/count limits; expired or closed/navigation-stale requests cannot write.
Native request IDs are single use per document; replies use JSON messages.

Create WebView2 lazily on the AE UI STA; report COM/runtime failures in the panel.
Do not change the host's COM apartment or DPI awareness. Own controllers and
async lifetimes explicitly; do not load UI in suppressed/headless contexts.
Use the installed Evergreen Runtime, with a pinned official SDK under ignored
artifacts. Explicit development profiles live under checkout artifacts, never
beside AfterFX.exe or in Adobe's shared caches. No runtime installation implied.

The pre-retirement policy kept CEP enabled during qualification. A successful native host pass or the owner's explicit version acceptance
authorizes retirement of this project's two CEP entries, with a backed-up,
reversible exact-path action. Never change PlayerDebugMode, other extensions,
shared cache or host-wide switches. Native compilation/fake-host/browser tests
do not qualify docking, IME, Undo or persistence in AE. All existing node kinds,
topology operations, native parameter selection, animation, Model import,
presets Add/Replace/import/export and save/reopen must retain baseline behavior.

Official references:
- Local May2023 SDK AE_GeneralPlugPanels.h and utility/memory suites.
- https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/threading-model
- https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/security
- https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/user-data-folder

## Owner preset qualification failure — panel100 candidate, 2026-10-19

The owner reports the native node canvas has no visible faults. Presets fail
at Model commit / verify published records, Particle parameter8. The supplied
bytes decode to .1 versus .10000000149011612 and .8 versus .800000011920929:
normal binary32 positions promoted to binary64, with unchanged payload header.
Baseline99's production transaction replay in Node already accepts this exact
rounding. Do not claim an identified ExtendScript engine defect or a reproduced
offline failure; actual stale-helper/dispatch/runtime cause remains unconfirmed.

Panel100 uses explicit per-kind bank dispatch and explicit finite scalar
comparison. Recognized curve/gradient banks never fall into byte comparisons.
Preserve the existing1e-6 relative/absolute-floor tolerance and1/255 color
tolerance; keep version/count/mode/reserved bytes and length strict. On rejection,
report the decoded byte offset, wanted/observed numbers and delta without any
extra AE reads. No authored curve bytes, native parameters, IDs, schema, Core or
render behavior is rewritten. Preserve publication verification and full rollback.

Advance manifest/browser/gateway generation together to100 so matching99 globals
cannot keep an older helper alive. Prepare under artifacts while AE runs.
The candidate passes53 suites and2479 actual-transport/fake-host checks, including
the reported Opacity bytes, all supported numeric bank roundtrips, genuine age/
value/mode/non-finite corruption rejection and graph/revision restoration.
The owner subsequently accepted the panel100 preset retest; it is no longer an open release gate.

## Modal quit failure — native98, 2026-10-19

Owner accepted the panel100 retest and requested retirement/main merge, then
reported AE refusing a script while waiting for a modal dialog during exit.
The owner identifies that dialog as the save-project confirmation on quit.
An idle hook is not sufficient evidence that AE accepts scripts: our exclusion
tracks only plug-in transactions. Require a valid enabled AE main HWND and
AEGP_IsScriptingAvailable before consuming valid queued requests, including
Undo/Redo and window commands. Fail closed on missing/error window lookup.
Retain FIFO and existing deadlines; discard expired/stale requests even during
modality. Do not request immediate idle wakeups while the host is blocked.

For the remaining readiness race, bracket only ExecuteScript with SDK
StartQuietErrors/EndQuietErrors(FALSE), restoring the opaque prior state on
every return. Never retry rejected writes or replace typed errors with success.
If quiet-error scope cannot start, do not call ExecuteScript. Do not change AE
environment, registry or any persistent/global error preference. Exact HWND
readiness and paired error-state tests do not replace actual quit acceptance.

## STB-11 graph selection keyboard ownership — 2026-10-28

Only exact selectNodeEffect calls opt into the private
evalScriptKeepGraphFocus kind. It retains the same four bounded string fields,
source checks, serial/replay rejection, FIFO, deadlines and navigation epochs as
evalScript. Other scripts and presets retain normal focus behavior.

At execution, capture graph HWND ownership only if its window/child has focus
and its actual top-level root is the current foreground window. RAII may restore
that same graph HWND only if still alive at the same epoch, visible/enabled,
unchanged HWND/root/foreground and last input timestamp, and focus has left that
view. This includes script failure/unwind. Never restore if the user already
switched before execution, entered more input, switched windows, docked/reparented
the panel to a different root or replaced/closed/navigated its view. SetFocus
uses the existing WM_SETFOCUS/controller MoveFocus path; no global key hook or
host settings change. Timestamp checks are conservative Win32 observations,
not proof of every physical user's intent or actual AE focus behavior.

The owner's119 rejection text identifies a synchronization path, not a proven
focus/root cause. Preserve first sync phase/repair reason/bounded trace on the
first error without requiring a busy retry. Explicit development journaling can
record Host dispatch and SDK/ack/status plus the effect's native refusal; a
pre-call dispatch result0 must never be interpreted as committed success.
No automatic repair, replay, guard reset, public parameter/schema or Core ABI
change is authorized by this evidence. Real AE Delete/continued editing remains
a separate acceptance gate.

## STB-12 graph-owned Clear command — 2026-10-28

The owner's120 trace and native journal establish that automatic synchronization
rejects an unreadable old Force alias at `verify finite alias handoff`, after a
manifest connection refers to a missing node. This does not prove the physical
keyboard path. Do not relax finite handoff or auto-repair/reset an unknown guard.

Register the documented AEGP_Command_ALL hook at BeforeAE. Resolve Clear once
through public app.findMenuCommandId on a safe UI idle, under UI exclusion and
quiet script errors; no hardcoded private ID. This lookup is language dependent:
current qualification scope is the owner's English AE2023. A zero/error result
leaves interception unavailable and is recorded in the opt-in journal. Real
AE accelerator order remains a separate acceptance gate (the SDK documents
priority limitations).

Only a physical unmodified Delete/Backspace Clear with the graph's actual HWND
or child in focus and its root foreground is consumed. Presets, other windows,
commands, modifiers, already-handled commands and workers retain their behavior.
The command hook only records bounded owned notification state, never script,
SDK mutation, message pumping or file I/O. Busy/queued/bridge-unready input is
consumed and dropped, never replayed after the graph becomes ready.

Before idle forwarding, require the same HWND, root, foreground, last-input
time, navigation epoch, random page token and input epoch, no intervening request,
and a500ms deadline. The browser additionally checks a250ms delivery deadline,
document focus, integer serial replay guard and the same page/input context.
Pointer/focus and every other keydown invalidate older input. Native and DOM
handling deduplicate one physical key; held keys do not retry failed edits.
Conservative invalidation can drop a key (including a fast release before idle)
rather than act on a changed selection. A fresh key is required; there is no
background automatic retry. Metadata is strictly bounded plain data, retaining
the existing source/serial validation and never evaluated as script.

A nontext notification invokes the existing deleteSelection topology path;
Force and other removals still require the complete Model graph transaction.
INPUT/TEXTAREA/contenteditable use Chromium local delete/forwardDelete editing;
SELECT/readonly fields never delete nodes. No global keyboard hook, persistent
host setting, public parameter/schema/ABI or Core math changes are introduced.
Existing damaged author graphs and unknown locks are not silently repaired.

## STB-13 foreground GUI queue — 2026-10-29

Owner121 failed again. Its198-record journal shows successful Clear lookup,
no forwarded graph Delete and no complete transaction event, then the same
old finite-alias-handoff refusal. That does not prove why the earlier physical
node removal occurred. Do not infer a keyboard root cause from absent events.

GetFocus observes only the calling thread's input queue. Both STB11 selection
retention and STB12 Clear admission used that value as the keyboard owner. A
controlled cross-queue fixture reproduces false rejection when local focus is
null/stale and the foreground GUI queue's real graph child owns input. The old
actual production predicate gives705 checks/one failure in that fixture.

Replace those focus observations with GetGUIThreadInfo(0), strictly requiring
its active HWND equals the sampled foreground, focus HWND exists and has that
same root, foreground remains unchanged, and no menu/move loop is reported.
Keep original graph HWND/root/visibility/enabled, input timestamp, page/epoch,
queue/deadline and no-replay checks. After selection, an unavailable GUI query
must not be treated as evidence that focus left the graph. Never AttachThreadInput,
change user focus on an unrelated command or change global input settings.

The explicit journal reports the ALL-hook registration result with resolved
Clear ID. A matching Clear stores at most one first plain observation plus a
capped repeat count until idle; no script, suite mutation, file I/O or pumping
in the command hook. Gate1=already handled,2=key/modifier mismatch,3=no graph
keyboard owner,4=latched/DOM-owned,5=busy/queue/page unavailable,6=input query
failed,7=notification queued. Gate7 is not a graph commit or browser receipt.
Exact handles are transient diagnostics, never graph identities.

Microsoft primary API references checked against this Windows-only adapter:
https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getfocus
https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getguithreadinfo
The controlled fixture and API contract do not establish actual AE/WebView2
queue layout or resolve the owner's Force incident. Those host gates remain
open, and current broken graphs/unknown guards are never silently repaired.

## STB-14 one resident command dispatcher — 2026-10-29

Owner confirms122 fails after Delete. Its81-record journal establishes that
Clear21 lookup and hook registration succeeded, but contains no matching Clear
observation, forwarding or complete transaction. Sync33357 still rejects old
alias handoff after missing-node manifest detection. Registration success is not
proof that a callback remains reachable after later registrations.

The official AEGP implementation guide prescribes one CommandHook per AEGP and
switching by command. The old WebPanelHost registered ALL plus two panel hooks;
StarfieldHostEntry then registered six hidden commands with separate handlers.
An actual old-entry fixture captures six registrations even with WebView
initialization stubbed;14 checks/one failure. This proves our registration count,
not an observed AE replacement rule or proof of the physical deletion path.

Define all session command IDs before registering one final BeforeAE/ALL hook.
The resident router selects exactly one of the six existing handlers or the
WebPanel adapter (graph, presets, graph-owned Clear). Preserve all native
handlers, session ID injection, menu invisibility, existing idle/exclusion,
transaction/reveal/repair behavior and no-replay checks. Unknown commands remain
unhandled; already-handled, stopped, worker or missing output do not dispatch.
The WebPanel initializer defines menus but never registers a command callback.
The sole final registration result is passed to its existing journal diagnostic.
If it fails, do not publish executable native command bindings to scripts.

Do not alter persistent IDs/schema/Core/strict handoff/guard or silently repair
an existing damaged graph. Test actual entry registration and real callback
routing, rather than invoking isolated clear_hook and calling that host proof.
No installed AE/GUI/project operation is part of offline qualification.

Primary SDK reference (also checked against local May2023 header):
https://ae-plugins.docsforadobe.dev/aegps/implementation/#example-adding-a-menu-item
The guide's single-callback requirement motivates this correction; only an
actual host exercise can confirm that it resolves the owner's lost Delete.

## STB-15 WebView2 controller focus — 2026-10-29

Native123 owner PID7640 logs Delete1/gate3 and local=GUI=foreground HWND,
followed266ms later by missing Force render failure. The unified command hook
is now reached; native-HWND graph ownership still rejects the key. The log's
other8 command rows/PID30276 came from a parent offline graph command fixture,
not AE. Separate45 owner rows from53 total. That test now disables production
journaling explicitly; existing raw evidence is retained unchanged.

Subscribe to public controller GotFocus/LostFocus events with weak View captures,
UI-thread/liveness/sender/subscription-owner checks, and require BOTH successful
subscriptions. Partial registration grants no logical ownership; failed Lost
subscription removes Got. Keep the controller that owns event tokens alive so
cleanup/identity never rely on a reused raw pointer. On LostFocus discard pending
Delete immediately. Close detaches subscriptions before releasing controller;
process failure disables logical focus and late callbacks cannot restore it.
Navigation keeps controller-level focus but retains existing page/epoch resets.

Native focus in the graph HWND/child remains sufficient under existing gates.
Only if native focus is exactly the current foreground root may verified current
controller focus supplement it; require same graph root, visible/enabled graph,
valid GUI query, no menu/move loop and nonpreset view. An explicit different
native child, background window, invalid query, lost/closed/failed control never
uses that supplement. Selection retention uses the same ownership predicate.
Keep all saved-window/input/page/deadline/queue/dedup guards and full graph
transaction routing. No mouse-hover or saved-click inference, SDK script inside
the key callback, missing-value substitution or unknown-guard reset.

The old production predicate in a real-interface controlled GotFocus fixture
fails726 checks/one failure. The mock is an independent COM boundary, not an AE
or actual browser process. It reproduces the logged root-HWND mode under a
control-owned premise; actual event ordering/runtime layout still needs owner
qualification. Registration gate values distinguish attached focus events and
controller focus in subsequent Clear evidence.

Primary public WebView2 base interface (introduced0.9.488, no new AE API):
https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2controller#add_gotfocus
https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2controller#add_lostfocus
