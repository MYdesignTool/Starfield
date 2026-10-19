# ADR 0046: AEGP Panel with WebView2, existing node behavior retained

- Status: accepted by owner after native98/panel100 retest; web_panel source migration explicitly authorized, 2026-10-19.
- Owner request: 2026-10-19. Baseline commit c43e293 (native96 / CEP99).
- Task: P-04. Windows x64 / May2023 SDK / AE2023 only.

Current source lives under web_panel/; do not recreate the retired cep_panel/
directory or auto-register CEP. The owner accepted the canvas, preset repair and
save-confirmation/quit retest, then authorized the source rename and recent-five
backup retention. Actual Windows registration removal and the installed pairing
are recorded only in current-state.md. Owner release acceptance is separate from
an instrumented full qualification matrix; unreported scenarios stay unreported.

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
