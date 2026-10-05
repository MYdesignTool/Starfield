# ADR 0029: main picture launcher and procedural preset manager

Status: implemented in build35; AE qualification pending. P-03 (preset-manager card; distinct from the
historical overlay-gizmo backlog row). Owner explicitly requests an original
generated picture in the main Effect Controls and a clickable preset manager.

## Build36 owner feedback

### Build39 follow-up: hidden writes and canonical rotation defaults

Owner build38 shows Warm Sparks blocked by AE's hidden Color Gradient property,
and Orbital Drift reporting an unconfirmed graph after application. The gateway
sets Panel Sync Guard but the stream was not supervised; its native callback
could not synchronously open mode-hidden controls before script setValue calls.
Register the guard with SUPERVISE, handle its change before the early guarded
return, and make native conditional visibility honor the guard. Open only the
Color Gradient and Size Y conditional streams, retaining their registration.
The gateway's finally reset restores mode/shape visibility even after a failure;
ordinary UPDATE_PARAMS_UI passes also honor an active guard. No force event
loops, arbitrary-data scripting writes or persistent host flags are introduced.

Native Rotation Over Life always has at least two knots. The preset planner
omitted key27, while named native readback includes its default flat-zero bank;
the old equivalence check rejected that additional field. Author the canonical
two-point bank in new graph nodes and use the same reset for absent curves rather
than setting count0 below the native minimum. Only this exact flat-zero default
is equivalent to absence; nonzero/custom rotation curves are still compared.
Preserve count/schema/identity/layout/connection and value checking, and report
the first differing field when readback fails. Other host differences remain
qualification gates, not silently accepted states.

### Build38 follow-up: rejected title fix and retained schema evidence

Owner build37 reports `Updated project: particle has unsupported node schema 2;
expected 6`. Catalog validation therefore passed; the retained target graph is
the failing phase. Static inspection finds no Particle2 producer in the current
planner, named native control reader, native compiler, codec or layout copy.
This does not prove which previously loaded runtime produced the stale metadata.
The manager's cached `ready` boolean nevertheless leaves a concrete lifecycle gap:
later operations invoke whichever global SFLD functions are then installed in AE.

Load the checkout gateway and invoke the requested operation inside one evalScript
turn. Check readiness after loading, stamp every reply with gatewayBuild, and check
that stamp before accepting the response. The pinned project/comp/layer/effect
token remains DOM-derived, and revision/recordStamp checks, rollback and strict
node schema validation remain intact. Request generation, page resources and
bundle version advance together to38. No saved graph is retagged or migrated and
no AE cache/registry/process changes are made. This closes the inspected lifecycle
gap; actual AE Add/Replace remains an owner gate, not a claimed verified fix.

Owner evidence also rejects the build37 title overpaint: the arrow remains.
Only the callback's supplied title rectangle is painted; do not paint outside it
or cover neighboring AE UI. The May2023 headers define PF_PUI_CONTROL as the
foldable body and offer no located public flag for suppressing its disclosure
gutter. Keep the visible/clickable banner and its current parameter registration;
removing this remaining arrow is unresolved. Build38 also defaults all native
angle controls to collapsed per the new owner screenshot (ADR0028).

### Build37 follow-up

The owner confirms the manager now shows its catalog and pinned target, but Add
reports invalid_preset/unsupported node schema. Current source tables agree at
Emitter7/Particle6/Force2/Output4, and prior source-only catalog checks passed;
the exact failing runtime node/version is absent from the old generic message.
Mixed cached script generations are a hypothesis, not a confirmed host diagnosis.
Expose the edit planner's schemaVersion accessor and use it for preset validation,
removing the duplicate version table. Both HTML pages load their scripts/styles
with the same build37 resource suffix; gateway/bundle identities advance together.
Validation remains strict and Core remains authoritative. Errors now identify
Selected preset versus Updated project, node kind and actual/expected versions.
No silent imported-preset migration or manual CEP cache manipulation is added.

Add Up, Folders and All presets navigation. Up returns to the root category grid,
clears selection, and disables while a transaction is pending. Remove the old
Example dropdown, its unused table and listener from the node editor by owner
request. Palette/curve presets remain separate supported editor features.

Owner evidence disproves the assumption that an untitled NO_DATA control removes
AE's title twirly. Keep index1/disk1631/main26 and own PF_PUI_TOPIC plus CONTROL
and DONT_ERASE_TOPIC. Draw the complete parameter title frame in the host's
background color; consume title clicks instead of letting AE collapse the picture.
Only the image control area launches the manager. Path/brush objects remain local
and balanced; Draw/Click-only handling avoids reading the wrong event union.
Native removal of the residual title icon still needs actual AE confirmation.

The main image renders and its click opens an AE window, but the owner reports
empty content. Read-only inspection confirms the installed system CEP Junction
points to this checkout and both presets.html and its stylesheet exist. The
manifest incorrectly sets AutoVisible=false for this visible Modeless extension;
set true and advance the extension/bundle version to36. The Adobe manifest schema
defines this flag as making the UI visible when started, and its Modeless sample
uses true. This is the identified configuration defect; actual AE correction
still requires owner confirmation. No caches, registry or shared CEF flags change.

The owner also requires no Presets folding title and exactly half-sized artwork.
Replace the scalar launcher with an untitled PF_Param_NO_DATA custom control at
the same PF registration index1, fresh disk1631/main manifest26. Retain all actual
authored parameter IDs and native node schemas. No legacy migration per owner;
create a fresh main development effect. Native events still target its control
area. Resample the original2172x724 PNG to1086x362, preserve composition/alpha,
retain original bytes in the paired backup and record the new hash/provenance.
The manager reports missing dependencies visibly if an import fails.

Primary reference: [Adobe CEP manifest schema](https://github.com/Adobe-CEP/CEP-Resources/blob/master/CEP_7.x/ExtensionManifest_v_7_0.xsd)
and [Adobe Modeless sample](https://github.com/Adobe-CEP/CEP-Resources/blob/master/CEP_12.x/Samples/CEP_HTML_Test_Extension-12.0/CSXS/manifest.xml).

## Original build35 contract

Main stream1 was an unused hidden bootstrap placeholder. Replace that placeholder
with constant UI-only Presets (fresh disk ID1630) at the same stream index. Public
renderer globals, hidden animation aliases98..609, graph persistence, and C ABI3
remain unchanged. Main manifest25 requires fresh development effects; no old
project migration per owner. The existing CUSTOM_UI declaration already covers
PF_Cmd_EVENT; the handler now also services the ECW image control. Embed the PNG
as a resource in the main AEX; decode only into owned CPU pixels, and create/release
all Drawbot objects inside each callback. Negotiate supported pixel layouts before
any image call; a failed image path stays disabled for that context. No modal
warnings from repaint and no retained Adobe suite objects.

The click executes the AE menu command for a separate Starfield Presets CEP
Modeless extension. The node panel also exposes an Open Presets command through
the documented CEP bridge. No shared CEF switches, registry, extension-cache or
installation changes. Missing launcher availability is reported only on explicit
click. Actual AE menu discovery and Modeless behavior require owner qualification.

The manager owns an independently authored procedural catalog, category folders,
search and selection, Save/Import through explicit file dialogs, and Add/Replace.
Only available emitter/particle/force features are represented. Application uses
the existing getGraphSnapshot/submitGraph transaction, bounded24 KiB codec,
target pinning, revision/record-stamp guards, native bank synchronization, rollback
and compiled readback. Preset graphs never include bindings/history snapshots;
adding remaps every node/edge UUID, retains one existing Output, preserves existing
native node values/animation, and positions the added chain separately. Adding
enforces the per-node four-output bound, total graph bounds and active-stage rules.
Replace is an explicit manager button. Unchecked Apply Render Settings preserves
the current Output parameters; checked uses the preset Output settings.

Disk files use a versioned Starfield preset JSON envelope carrying bounded graph
hex, title and category. They are portable authoring data, never executable code.
My Presets lists imports and saves for the current manager session; files persist
at user-selected paths and can be imported again. No automatic profile library
index is installed. Card illustrations are procedural previews, not AE renders.
The owner chooses the save/import path in the application. No agent-created files
or library installation outside this checkout. Paired deployment/rollback records
retain the previous CEP files and whether each new manager/asset file existed.

Artwork is original, generated by the built-in imagegen tool; prompt and asset
hash are recorded in docs/preset-banner-provenance.md. No Stardust artwork,
binaries, libraries or private identities are incorporated.
## Panel47 follow-up: Add and native control projection (2026-10-05)

The owner reports successful Replace, but Add rejects a retained Emitter tagged
schema3/expected7. Current JSX reads all controls by name and already emits7;
static inspection has not located the actual producer of the reported3 label.
Do not claim cache mixing as a confirmed cause or migrate serialized graph data.

The CEP native snapshot adapter now derives each portable schema from the shared
graph_edits registry after checking the complete current ordinary-control key/type
layout. This corrects stale projection metadata only; it neither synthesizes
missing controls nor interprets legacy graph bytes. Unknown/duplicate/wrong-type
fields, absent required controls and future record versions reject before writes.
Optional native curve banks may be absent at count0. Initialized snapshots lacking
ordinary native records no longer fall back to a supplied graphHex. Output uses
the same registry. Imported presets keep the existing strict version policy.
Retained identities, parameter values, connections and layout survive Add, and
the existing native diff writer preserves unchanged animated property streams.
Atomic gateway invocation, generation/revision/record-stamp guards, host rollback
and compiled acknowledgement remain in place. Native46 layouts/IDs are unchanged.

Panel47 also scopes dark 8px WebKit scrollbars to the extension documents; inspector
rows wrap and the body scrolls vertically. No shared CEF switches, registry/cache,
other extensions or host processes change. Syntax/hash evidence is distinct from
actual owner AE Add/Replace and narrow-window qualification. No tests requested.
## Panel48 follow-up: preset operation cost (2026-10-05)

The owner requests faster preset behavior after accepting panel47 qualitatively.
An initialized Add/Replace previously read getGraphSnapshot, ensureNodeEffects,
and the transaction's getGraphSnapshot before submit. It now reads one immediate
native snapshot for planning and passes that receipt into the transaction client.
Target identity is checked before planning; submit independently rereads native
records and validates revision/record stamp before any write. The current-state
semantic acknowledgement, native guards, diff writes and rollback remain strict.
Uninitialized graphs still explicitly run syncGraphSnapshot and validate its
receipt. No old project/preset schema migration or missing-control defaults added.

Manager reads check the exact gateway generation and reuse it in the same host
turn. Writes always reload/invoke atomically. No cached ready boolean. The lookup
and numeric identity-index optimizations are scoped in ADR0009/P-02K. Graph wire,
native IDs/layouts, Core ABI and rendering remain at native46. Actual Add/Replace,
animated existing values, rollback/undo and operation timings remain AE gates.
No test suites requested/run and no measured latency reduction is claimed.
