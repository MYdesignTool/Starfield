# Agent-ready implementation backlog

## P-02L - Collapsible CEP node palette

Owner's 2026-10-06 screenshot requests a left icon palette, expanded by default,
a bottom collapse/expand arrow, and dragging an entry onto the canvas to create
a node. Exclude Output: one canvas has one fixed Output. Offer only implemented
Emitter/Auxiliary/Particle/Force kinds; add Transform when M3-11 is functional.
Own cep_panel/index.html, panel.css, new node_palette.js, panel.js, paired resource/
gateway/manifest versions and README, focused palette/startup tests, Deploy-TestBuild
KeepNative mode, deployment/
rollback staging and ADR0009/architecture/roadmap/build/qualification docs.
Reuse requestTopologyEdit and existing coordinates/atomic native transaction;
no new host transport, writes during pointer movement or render contract changes.
Suspend polling during palette drag; release outside canvas, Escape, pointer
cancel, lost capture, blur or target changes cancel without host writes.
AE is currently running. Prepare/test under artifacts/prepared/ before changing
the live CEP source Junction. Save panel48/native46 for paired rollback and publish
only after a fresh no-AE check under the existing deployment authorization.
Status: implementation and native46/CEP49 publication complete. Seven focused
Node24.19.0 suites pass against installed sources, plus markup/resource/XML/JS and
PowerShell syntax checks. Preserve all six native hashes and the Core selector;
22 CEP files are hash verified. Native46/CEP48 backup:
artifacts/disabled/p-02l-panel49-node-palette-20261006. Paired rollback report
passes; one-step action is artifacts/build49-rollback.ps1 with AE closed. No
process or registry changes. Actual AE drag/appearance/undo qualification remains
open. M3-11 graph/native/render integration resumes after this source milestone.

## M3-11 - Transform node

Owner requests the Stardust Transform inventory on AE2023. The supplied parameter
report and 2026-10-06 screenshot establish this order: Inherit Motion (Null Layer),
Anchor XY/Z, Position X/Y/Z, Rotation X/Y/Z, Scale X/Y/Z, Particles Scale and
Particles Opacity. Use independent Starfield identities and implementation.
Own Settings.hpp, new ParticleTransform.hpp/.cpp, graph construction/evaluation,
native Transform effect/parameter and sampled layer adapter, native temporal
capture, sprite basis/render transport, CEP graph/inspector/gateway/preset paths,
schema declarations, build inputs and scoped tests, ADR0032 and related docs.
Render.hpp and Core ABI changes require explicit versioning in ADR0032 before
implementation. Existing released IDs and kinds retain their values.

Work starts with a host-independent affine primitive and focused tests, compiled
once per sampled node. System Scale changes particle positions; Particles Scale
changes sprite size. Preserve inherited affine reflection/shear and distinguish
position, velocity and sprite axes. This foundation alone does not implement a
graph node. Subsequent gates are ordered graph semantics (including Force and
auxiliary emission), native/preset roundtrip, sampled Null capture in the effect's
coordinate frame, CPU/GPU sprite transforms, cancellation and bounded work.
Do not advertise inert controls or deploy the primitive as a finished node.
Status: affine foundation implemented; 268 focused MSVC checks pass (2026-10-06).
Graph/native/CEP/render integration and AE qualification remain open. Reference
menus are recorded in reference-texture-transfer-transform.md. This first source
milestone changes no installed bundle, graph kind, parameter ID, wire or Core ABI.

2026-10-06 render-transport milestone: independently compose centre/sprite maps;
add a shared basis index/table, bounded snapshot4 with snapshot3 compatibility,
CPU/GPU scene projection and typed Linear motion interpolation. Core ABI4 is
declared in ADR0032 before implementation. Own the BuildWindows runtime-only
native-pair hash guard and focused build_publication_tests; its old artifact-only
fingerprint could allow an unpublished AEX to authorize an incompatible Core
switch. Extend focused CurrentNodes ownership to its stale gradient fixture:
the existing byte2 Hold mode is valid; corrupt reserved byte3 or mode2 instead.
Production gradient code is unchanged. Math466/transport3544/current-node437/
publication11 checks pass and the final native candidate builds without publishing.
At that transport milestone no Transform graph/native/CEP kind was exposed.

2026-10-06 ordered graph milestone: core Transform schema1/keys1..7 and typed
ports are implemented under ADR0032. Static and temporal evaluators share bounded
Transform planning; each Force uses its downstream suffix, independent Particle
streams retain separate frames, and divergent chains of one stream reject an
ambiguous merge. Auxiliary birth prefixes, inherited velocity/style, historical
origin offsets and sprite orientation use the appropriate frame. Pseudoinverse
compilation handles reflected, sheared and singular inherited bases once per pose.
TransformGraph586/math483/transport3544/current-node437 checks pass. Native effect,
CEP authoring, sampled Null capture and actual AE qualification remain open.
Continue M3-11; this source milestone does not finish the owner's broader goal.

## P-02K - CEP idle and preset-operation performance

Owner qualitatively accepts panel47 functionality. Multiple emitters cause frequent
busy cursors; owner subsequently confirms they stop when CEP closes, and asks to
improve general responsiveness including preset application. Own panel.js,
preset_manager, graph_transactions, gateway, paired CEP version declarations/
README and ADR0009/0029/architecture/roadmap/build/qualification docs. Retain
native46, motion blur and native temporal bootstrap. Combine read-only panel
inspection, adaptively poll lightweight target/time/receipt markers, refresh full
node data on changes/focus and bounded periodic audit, reuse fresh preset planning
snapshots, and cache native property lookups only inside one host request. Numeric
identity indices may survive own append/remove operations; reacquire effect
references and clear property caches after every Effect Parade structural edit.
Preserve native guards, exact disk IDs, strict schema/value validation, transaction
revision/record-stamp checks, semantic acknowledgement and paired rollback. No
tests requested/run; observed host latency and cursor improvement remain owner
qualification. Source operation counts are not measured timing attribution.

Sourcebb0df2e installed as native46/panel48 with AE absent; six binary/all21 CEP
hashes and paired native46/panel47 read-only rollback verification pass. Source
syntax/manifest/wrapper/whitespace gates pass. Implementation and deployment are
complete; owner idle-cursor, preset latency and refresh/undo qualification remain.

2026-10-06 follow-up under the owner's minimal-test authorization: this card also
owns the focused startup/transaction/native gateway/preset suites, the two current
ordinary-control fixture helpers and preset_manager_performance_tests. All five
suites pass against panel48. Coverage includes adaptive/pause/audit scheduling,
one fallback traversal per effect/request, no sibling values or graph mutations
from pulses, prepared-receipt target/revision rejection, host record-stamp conflict
checks, actual manager Add/Replace call counts, explicit bootstrap, copy/deletion
and rollback. The fake host models ordinary properties and indexed invalidation;
it is not AE or the C++ compiler. Signed zero/decimal transport CRC remains
diagnostic; semantic field checks remain strict. Installed runtime sources are
unchanged. Actual cursor and wall-clock measurements remain owner gates.

## M3-10 - Main-effect motion blur

Owner accepts native45/panel45 curve behavior and requests the supplied Motion
Blur inventory. Owner explicitly defers both PTF disregard choices until Physics
Time Factor exists. Implement Off / Comp Settings / On; custom Shutter Angle /
Phase, Linear / Subframe Sample, Levels8, Linear Accuracy70, Opacity Boost0 and
Disregard Nothing / Camera Motion. Actual exposure rendering on CPU and native
GPU is required. Read comp/layer switches and comp shutter without modifying them.
Linear uses stable particle identity and interpolates two endpoint states;
Subframe evaluates each midpoint. Share historical native captures across samples.
Accumulate premultiplied floating exposures, normalize once and boost alpha while
preserving color. Keep cancellation, pixel/ROI guards and bounded memory/work.
Owned: new MotionBlur source/headers, Graph optional metadata/registry/defaults,
EmitterHistoryCapture,
SmartRender, Camera, GpuRender/kernel, Parameters/EffectMain/flags/version, main
schema, CEP render-settings/preset roundtrip and version markers, build project/
fingerprint inputs, ADR0031 and architecture/roadmap/qualification docs. Main
parameter indices append only; native layouts, graph wire, Core ABI3 and snapshot3
stay unchanged. No tests requested/run. Compile May2023 /MT, save paired45 rollback
and qualify actual AE visuals/performance with the owner. No process/registry changes.
Implementation includes CPU/GPU exposure averaging, one temporal plan per shutter,
pre-render camera sampling, mode UI and all native/CEP render-setting roundtrips.
Main27/build46/CEP46 compile and syntax evidence are recorded in build-matrix.
Source63224b0 is installed as native46/panel46; six native/Core and21 paired CEP
hashes and read-only rollback verification pass. Backup:
artifacts/disabled/m3-10-build46-motion-blur-20261005.
Actual AE qualification remains open. Native45 behavior is accepted qualitatively.

## M3-09 - Editor curve/color presets and interpolation

Current owner follow-up: build44 Draw performance is basically normal, but
Draw -> Linear exposes all 64 samples as unwanted handles. Build45 owns
ParticleGradientUI, new CurveEditorModel, PluginVersion, CEP curve_editor/panel mode handlers, shared
bundle/cache/gateway version markers and ADR0030/build/qualification docs.
The owner clarifies that the overall shape must remain. Simplify by the largest
vertical deviation, preserving endpoints with a 4% relative error target and
at most 12 handles; the cap can leave larger errors on complex strokes. Preserve
other mode transitions, presets/paste, build44 latency changes and atomic undo.
No schema/ID/ABI change. Save native44/panel42 and deploy native45/panel45 only
with AE closed. No tests requested/run; compilation and AE behavior are separate.
Full May2023 /MT candidate and changed JS/JSX/deployment-wrapper syntax pass.
Log: artifacts/build45-native.log; no compiler diagnostics. Patch whitespace
passes. The 21-file panel42 rollback baseline is prepared and hash verified.
Source9755fc0 is deployed as native45/panel45; six installed/saved binary hashes
and all 21 CEP sources verified with retained native44/panel42 rollback. Owner
AE Draw -> Linear shape/handle count and undo/reopen qualification remains open.

Current owner follow-up: native43 is usable, but a Draw edit waits nearly ten
seconds before composition rendering. Native44 owns ParticleGradientUI,
NativeNodeGraph, PluginVersion and ADR0030/build/qualification docs. Keep the
64 samples and existing IDs/schemas. Draw uses an owned UI draft during a stroke
and one atomic publication on release; cancel on keyboard, external changes or
context closure. Stroke the preview once and mark only changed native leaves.
Check every alias's exact expression/type/enabled state, but snapshot and evaluate
only installed/repaired aliases. Preserve rollback and render-time validation.
Source overheads are inspection findings; their share of AE latency is unmeasured.
No tests requested/run. Full build, paired native43/panel42 rollback and actual
owner Draw timing remain required; other owner qualification gates stay open.
Full May2023 /MT candidate compilation and patch whitespace pass. The final log
is artifacts/build44-native.log; no tests requested/run. The 21-file panel42
baseline is prepared and hash verified. Source6774b2f is deployed as native44/
panel42; six installed/saved binaries and 21 paired CEP sources are verified.
Paired native43/panel42 rollback is retained. Actual AE Draw timing remains open.

Current owner follow-up: panel42 most functions and preset application pass;
native mode click still fails animation binding, parameter36/stream-1/error516.
Native43 owns NativeNodeGraph binding writer/reader bounds, PluginVersion and
ADR0030/build/qualification docs. Draw expands to64 knots; old81-field decode
limit rejects this valid record. Use per-kind base counts34/442/140 for count
and index validation, preserve alias/type/version and transaction guards. Panel42
and native schemas/IDs stay unchanged. Full candidate build, paired40+42 rollback
and owner AE native-click qualification required. No tests requested/run.
Candidate May2023 /MT full build and patch whitespace pass without compiler
diagnostics; artifacts/build43-native.log records compilation. AE clicks pending.
Source73f478d is deployed as native43/panel42; six installed/saved binaries and
21 paired CEP files are verified with native40/panel42 rollback. AE gate open.

Current owner follow-up: panel41 fails addition at Size under Over Life; the error
identifies the PF topic2912 as the attempted scope.42 owns root disk-ID matchName
binding for count/mode/all64 knot slots and Particle scalar Opacity on both read
and write paths. Reject the41 topic-as-PropertyGroup assumption. Native40 and
schemas remain unchanged. Preserve paired41 rollback and qualify actual AE
effect addition/readback. No tests requested/run. Owned files: gateway, both
client version markers, manifest/HTML URLs, schema binding policy and docs.
Source8f38c37 is deployed as panel42/native40; all21 CEP entries and6 native
files are verified with paired41 rollback. Actual AE repair gate remains open.

Owner follow-up: build40 fails effect/node addition with missing Rotation Curve
Count. Panel generation41 owns explicit scoped curve bindings and atomic node
gateway mutations with request/reply generation checks. The reported old label's
producer remains unconfirmed; the duplicate scalar/curve Opacity name is found
in source and corrected on both read/write paths. Native40 is unchanged; retain
paired40 rollback and qualify actual AE addition. No tests requested/run.
Source49720c4 is deployed as panel41/native40; paired21 CEP entries and6 native
files verified. Scope/bridge implementation complete; actual AE repair gate open.

Owner: primary agent, current card after P-03 fix a624455/build39 compilation.
Implementation candidate: build40 compiled cleanly with May2023 SDK /MT;
JS/JSX syntax and patch whitespace pass. No tests requested/run. Actual AE
Add/Replace, native modal picker and persisted four-mode behavior are owner gates.
Local source8d6fb33 is deployed as40; six native/Core candidates and21 CEP source
entries are verified with paired38 rollback. Implementation/deployment complete;
actual AE qualification remains open. See docs/build-matrix.md for receipts/undo.
Owner supplies Over Life/color screenshots, excludes directory recreation, and
requires editor-specific Presets entries plus exactly Linear/Hold gradients.
ADR0030 owns numeric independent catalogs, native modal picker, Size/Opacity
curve UI,64-knot curve/four-mode contract, Linear/Hold gradient persistence, native
schemas/layout/readers/publishers, CEP editor roundtrips, generator/build/version
and qualification docs. Owned: Settings/AgeCurve/ColorGradient, graph registry and Force curve integration,
native NodeEffects/ParticleLayout/NodeRecord/NodeGraphSync/NativeNodeGraph/
ParticleGradientUI/models, new catalog/picker/generator, schema/node-parameters,
CEP graph/editor/default/bridge and build/docs. Render.hpp, C ABI3, GPU kernels
and main parameter indices are outside this task; immutable evaluated scenes
already carry resulting appearance. Fresh Particle7/Force3 are required, paired
rollback to38 is retained, and actual AE behavior remains an owner gate.

## M3-08 - Reference procedural rotation controls

Owner: primary agent. Depends on committed M3-07 movable gradients (381be4f).
Current implementation card; develop on codex/m3-07-particle-gradient. ADR0028
owns the Particle Random Limit/Limit Angle/Rotation Over Life/anchor semantics,
Emitter Angle/Orient split, native AE dials and reference defaults/names/order.
Owned files: Graph/Settings/ParticleInstance/GraphEvaluation/SpriteGeometry,
EmitterHistory snapshot, node definition/layout/readers/sync/schema, CEP gradient
and curve inspector/gateway, build/version and focused checks/docs. Render.hpp,
GPU frameworks/kernel transport and main binding layout are unchanged. No inactive
external Light/Null/source/model control placeholders. Fresh schema6 Particle and
schema7 Emitter effects/graphs required; paired rollback retains prior bytes.
Status: implementation complete. Native build succeeds; 435 current-node,
6200 native-sync + 12 camera, 3688 visual-editor and 91 Particle registration
checks pass. Integrate the candidate with subsequent P-03
before the next deployment; main remains owner-accepted build31.

## P-03 - Main picture launcher and preset manager

Panel47 follow-up (2026-10-05): owner reports Replace succeeds but Add rejects a
retained Emitter schema3/expected7, and the CEP inspector shows white vertical
and horizontal scrollbars. Own graph_edits/native_graph_snapshot, local panel
and preset CSS, paired CEP generation declarations and ADR0029/architecture/
roadmap/build/compatibility records. Derive portable versions only from complete
current ordinary-control records; reject incomplete/unknown/future layouts and
missing native snapshots. Keep imported preset validation strict and preserve
existing identities, values, connections, layout and native animation. The source
of the observed schema3 label remains a hypothesis: current JSX emits7 already.
Use local dark scrollbar styling and wrapping inspector rows. Native46 and
motion blur remain in place; no test suites are requested or run. Actual AE Add
and narrow inspector appearance require owner qualification.

Panel47 source798757d is installed with AE absent, six native46 hashes/all21 CEP
files verified and native46/panel46 paired rollback checked without restoring.
Implementation/deployment complete; actual owner AE qualification remains open.

Build39 follow-up (2026-10-05): owner build38 reports Warm Sparks failing to write
the dynamically hidden Color Gradient and Orbital Drift failing graph readback.
Own NodeEffects guard flags/NodeGraphSync visibility plus gateway/graph_edits/
graph_transactions and paired version/docs. Supervise Panel Sync Guard, unhide
conditional gradient/Size Y synchronously during guarded writes and restore in
finally. Canonicalize absent Rotation Over Life to two zero points and expose
specific readback differences without weakening schema/topology/value checks.
No tests requested or run. Native AE verification remains an owner gate.
Subsequent owner request is a separate editor preset card: no directory replica;
entry points belong to Color Gradient and Over Life's Presets buttons.

Build38 follow-up: owner supplies precise Updated project Particle2/expected6
failure and rejects build37 title painting. Own the preset bridge lifecycle:
atomic gateway load/invocation, request/reply generation checks and paired38
resources. Actual origin of stale metadata remains a hypothesis; strict schemas
are not weakened. Extend this card's NodeEffects.cpp/schema/docs ownership to
default-collapse native angle dials, without altering M3-08 values/IDs/contracts.
Remaining banner disclosure arrow has no located public May2023 hide flag and
is unresolved; preserve the owned drawing rectangle. No tests requested or run.
Actual AE Add/Replace and initial dial appearance remain qualification gates.
Candidate4bf5d26 builds and is deployed as38 with all six binary/all19 CEP hash
checks and paired37 rollback verification. Main remains owner-accepted31.

Build37 follow-up: owner confirms build36 catalog loading, but preset application
fails schema validation; parent navigation is missing and native blank title still
has a twirly. Owner also removes the CEP Example feature. Unify schema validation
with graph_edits, version page resources/gateway, report exact mismatched versions,
add Up/Folders navigation and remove Example data/UI/listener. Own title paint and
click through PF_PUI_TOPIC/DONT_ERASE_TOPIC. Keep main26/index1/disk1631 and native
schemas unchanged. Cached-generation mixing is a hypothesis pending host evidence.
May2023 compilation and JS syntax checks pass; no regression suites are run here.
Candidate4e85ec3 is deployed as37, with verified binary/CEP hashes and paired36
rollback. Actual Add/Replace, Up and native title drawing remain owner gates.

Build36 follow-up: owner reports a blank preset window, requires the image without
its folding Presets title, and requests half-size packaging. Read-only installed
path inspection finds the files present; correct Modeless AutoVisible=false to
true and advance bundle version. Register untitled NO_DATA/disk1631 at main index1
(manifest26, fresh main effect), halve artwork to1086x362 and retain paired35
bytes. No cache, registry, host process or shared CEF switches change. The actual
AE window/content correction and no-title UI are pending owner confirmation.
Candidate41c07c5 is built and deployed as36; before/after binaries, all19 CEP
sources and one-step build35 rollback are verified (build-matrix.md). Main remains
at owner-accepted31 and the development branch remains unchanged.

Owner: primary agent, current card after M3-08. User explicitly requests one generated
Starfield banner in the main ECW and a functional preset manager with categories,
search, render-settings option and Add/Replace. The built-in imagegen banner is
integrated under cep_panel/assets and embedded in the main AEX. Own main custom UI/resources/
build/parameters/schema/capabilities, CEP manifest/assets/manager/preset model/
gateway, and matching focused checks/docs. Keep authoring on the existing graph
transaction and target identity/revision guards. Do not add registry, shared CEF
flags, third-party presets or inactive unsupported-category downloads. Actual AE
launcher and graph application require host qualification.
Status: implementation complete in build35, integrated with M3-07 movable stops
and M3-08 rotation. Six authored catalogs pass real Core parsing/evaluation;
Add/Replace, portable file dialogs, stale targets and native readback checks pass.
Final native candidate builds without compiler warnings/errors. Actual AE picture
click, modeless manager, undo/reopen and rotation visuals remain host gates.
Local browser policy rejects file URLs; no actual manager screenshot is claimed.
Build35 candidate381404b is deployed with verified binary/CEP receipts and paired
build34 rollback (build-matrix.md). Main remains at accepted build31; host feedback
is the next gate before further integration.

## M3-07 - Particle Color Over Life editor and control organization

Owner reports build34 usable, then requests movable first/last markers. The
gradient implementation now permits all stops within [0,1], preserves color
identity while dragging across stops, and holds the nearest color outside their
range. Native/CEP/JSX/codec/renderer validation agree. Implementation checks:
3431 gradient UI, 394 current-node core, 5717 native sync + 12 camera, zero failures;
both gradient JS suites pass. This closes the implementation slice; integrated
candidate deployment and actual AE end-marker qualification follow the next cards.

Build34 follow-up: owner build33 testing is blocked by a repeating Unsupported
Pixel Format warning on gradient expansion. Correct unnegotiated 24RGB image
creation using supplier BGRA/ARGB capabilities, opaque 32-bit bytes, safe path
fallback and a per-context image failure guard (ADR0027). Continue the same task;
native AE qualification and fresh-node hidden metadata remain open.
Build34 implementation/full May2023 compilation and 3943 focused checks pass;
see build-matrix.md for scopes/logs. Native AE warning-free drawing and editing
are pending. No schema or renderer changes; main remains at accepted build31.
Candidate bfbca0c / build34 is now deployed with verified before/after hashes and
unchanged CEP/Core. Retest warning-free expansion and native edits before main
integration; rollback is recorded in build-matrix.md.

Build33 follow-up: owner rejects build32 native UI (adding-stop compile failure,
strip seams and internal controls visible even on fresh Emitter/Force). Correct
custom dimensions, continuous bitmap drawing, whole-bank events and actual AE
dynamic visibility. Continue M3-07 qualification; main remains at 8857303.
Candidate 549b1b9 / build33 is built, checked and deployed with verified binary
hashes and unchanged CEP/Core. Host retest is pending; rollback is in build-matrix.md.

Owner: primary agent. Depends on owner-accepted build31 / ADR0026. Development
branch: codex/m3-07-particle-gradient; main retains the accepted checkpoint.
Implement the supplied native Effect Controls gradient interaction and matching
CEP actions, reorganize supported Particle controls by reference names/order,
and verify edits, serialization, rollback, keyframes and drawing resource lifetime.
Owned files/contract: ADR0027, native gradient model/editor, NodeEffects/flags,
NodeRecord/NativeNodeGraph/NodeGraphSync, build/version, node schema, CEP gradient
UI/gateway, Particle schema registry (Graph.cpp/GraphConstruction.cpp), paired
CEP rollback tooling/tests, and matching build/compatibility/reference docs.
No Render.hpp or renderer changes. New stream order requires fresh development
Particle effects; no legacy migration. Keep unsupported source/texture/blend
families in later cards instead of advertising inert controls.
Status: implementation/build/scoped checks complete. Actual AE UI/undo/reopen/
render qualification remains open; see build-matrix.md for the candidate evidence.
Candidate d70c6c8 / build32 is deployed with verified hashes and paired rollback;
main remains at owner-accepted 8857303. No automatic merge of this new work.

## Owner acceptance / main integration - 2026-10-04

The owner reports the build31 issues are basically resolved and explicitly
authorizes merging this installed version into main. This is an accepted
development checkpoint for AE 2023, superseding the previous merge hold.
No new exact timing numbers or exhaustive host matrix results were supplied;
unreported render-queue/MFR/new-host gates remain open. Development continues
on a separate branch for the Particle gradient editor and reference controls.
The installed build31 and its paired rollback remain unchanged by integration.

## M3-06 / build 31 - post-load bootstrap and Appearance removal

Build30 failed owner no-Options startup qualification: W57/57 P57 E0 but
Temporal0/57, PF193372 and history1671.3ms (CUDA16.3ms). Build31 (32799/0x801F)
adds a session-resident General AEGP for bounded, read-only main-thread idle
certification after load; sequence callbacks only maintain SFU1. Actual generic
PF context availability and no-Options first/reopen preview remain owner gates.
See ADR0026 for the protocol, lifetime rules, retry/state checks and limitations.

Owner explicitly requires complete Appearance removal without compatibility.
Only Emitter (including Auxiliary), Particle, Force and Output remain registered.
Particle owns style/Over Life; connection/layout edits only write node records.
The retired AEX is archived on installation; one-step rollback restores it and
removes the new StarfieldHost.aex. Main manifest24/IDs, remaining native schemas,
C ABI3 and SFU1 remain paired. Old graphs containing Appearance must be recreated.
Gateway native-idle-31; reopen CEP. Main integration waits for AE qualification.

Build31 implementation and scoped candidate checks are complete (see
docs/build-matrix.md for exact counts). M3-06 remains open for owner AE2023
qualification: no-Options first/reopen preview, generic PF context availability,
and Particle -> Force authoring. Main integration remains pending those gates.

## M3-06 / build 30 - evaluate restored bindings before state certification

The owner rejects build29 startup performance: Options is still required. The
2026-10-03 screenshot reports B2 (two sequence capture attempts), Temporal 0/57,
PF58 / N2/2, followed by 57 valid proofs after Options. This establishes that
bootstrap ran but does not establish usable render proofs. It reports no last
GPU frame and preparation at time zero; it is not a preview latency measurement.

Source inspection identifies a missing step: Options' binding transaction reads
all active aliases after expressions before capture, whereas startup only read
PF states and source metadata. Lazy expression dependency discovery may change a
pre-evaluation state during first rendering. This is a hypothesis about AE, not
host confirmation. Build30 (32798 / 0x801E) evaluates all recorded active aliases
through callback-local AEGP UI streams BEFORE any proof state is captured. It
checks OneD type, expression enabled, finite value and unavailable sentinel.
Only successfully evaluated components are eligible for the existing source
metadata plus before/after all-time state certification and render revalidation.
No expression, source value/key, project stream or selection is written.

Scoped fake-host tests model a dependency generation changing on first expression
evaluation. A negative control demonstrates that pre-evaluation tokens become
invalid. Cold setup and save/reopen then retain proofs after first render reads;
read/type/nonfinite/disabled-expression failures publish no proof for that input
and release references. Denied PF checkout/checkin in sequence callbacks, absent
params, source keys/expressions and worker/render-only exclusions remain covered.
5,157 scoped checks pass (native sync 5,145; camera 12); evidence is
artifacts/build30-native-sync.log. Native build/deployment is recorded separately.

Diagnostics snapshot the previous automatic reader BEFORE Options refreshes it:
W is evaluated/active aliases, P published proofs, E optional warmup error; B
remains sequence attempts. Build30/proofs is the subsequent explicit Options
result. Process-global readings may belong to another effect callback.
Actual AE no-Options first preview and save/reopen are still mandatory gates.
The owner-authorized main integration waits for those results. Core ABI3, public
IDs, match names, graph codec, SFU1 sequence schema, CEP and PiPL flags are unchanged.
Owned scope: NativeNodeGraph, NativeTemporalCache, Diagnostics, PluginVersion,
scoped native sync fixture and these M3-06/ADR0026 records.

## M3-06 / build 29 - remove forbidden sequence parameter callbacks

Owner rejects build28: AE2023 reports effect cannot use checkout/checkin callbacks
in SEQUENCE_RESETUP (25:83), both on project open and during use. The absent-array
fallback added in build28 was invalid. Its fake host allowed a callback AE forbids;
that coverage did not qualify the host selector. Automatic startup remains unproven.

Build29 (32797 / 0x801D) never reads sequence params[] and never calls PF parameter
checkout/checkin in SETUP/RESETUP. On the recorded main UI thread only, it obtains
the current effect's graph stream through AEGP_PFInterfaceSuite1/StreamSuite6,
checks ARB type, copies the graph while the returned value is alive, then reads
source metadata through the existing reader. Value, stream and effect references
are released in reverse order within the callback. Missing graph/type/suites is
an optional miss; it neither rejects project opening nor publishes a guessed proof.
All-time owned-alias states still bracket source metadata and are checked in render.
Worker/render-only resetup skips AEGP entirely. No render-thread AEGP, retained
source handle, idle hook or cross-effect generic message is added.

Qualification: 3,974 scoped checks pass (native sync 3,962; camera 12). The full
May 2023 SDK Release /MT candidate builds all five AEXs and Core without compiler
warnings. Evidence: artifacts/build29-native-sync.log and build29-native-build.log.

Schema SFU1 remains exactly four flat bytes; graph codec, node controls, public
IDs, Core ABI3, CEP and PiPL flags are unchanged. Full paired AEX deployment is
required. Focused tests now deny PF checkout/checkin in sequence callbacks, pass
an undersized params array, omit callback functions, inject graph read/type failures,
and check stream/ARB cleanup, recovery, keys and worker exclusions. Actual AE
open/run without 25:83, no-Options first preview and save/reopen performance are
mandatory gates before the owner-authorized main integration.

## M3-06 / build 28 - initialize history proofs when opening projects

The owner confirms build27 cuts uncached-frame preparation from 6-8 seconds to
approximately 1-2 seconds, but only after clicking Options, including after saving
and reopening. This qualifies a visible improvement, not full frame-time parity.
The constant/key metadata registry is process-local; the DRAW refresh added in
build26 is not delivered for every unopened/unselected main effect.

Build28 (32796 / 0x801C) reads optional source metadata in main-thread
SEQUENCE_SETUP/RESETUP, before preview needs it. Worker/render-only callbacks
perform no AEGP source queries. Options and DRAW remain optional refresh routes;
source keys/expressions and all-time PF states are revalidated after every load.
No equality-of-values shortcut is used. Diagnostics B counts sequence refresh
attempts; Static/Temporal and PF/N indicate actual render-side use.

Qualification: 3,627 scoped checks pass (native sync 3,615; camera 12); all five
AEXs and Core build with May 2023 SDK Release /MT without compiler warnings.
Evidence: artifacts/build28-native-sync.log and build28-native-build.log.

Private main sequence-data schema SFU1 consists of exactly four byte-ordered ASCII
bytes, with no handles, references, PF states, graph values or cached particles.
Legacy null data is provisioned during setup/resetup and also during flatten/save;
unknown non-null schema/size is rejected without replacement. SETDOWN releases the
host allocation. Saved non-null data provides a RESETUP opportunity on reopening.
An old null-data project may lack that callback; first cold legacy open and first
save/reopen are separate AE acceptance gates. Build28's partial/absent-array checkout fallback was subsequently rejected by AE
(25:83); build29 replaces it with an AEGP graph stream read. Source metadata can
be unavailable while siblings restore; later DRAW can retry. Fake-host results
cannot establish AE callback order or availability.

Scope: EffectMain, NativeTemporalUI, Diagnostics, PluginVersion, scoped native
sync tests and these records; no Core/Render transport, source node controls,
public parameter IDs, match names, graph codec, CEP or PiPL flag changes. The
private sequence migration is recorded in ADR0026. Full paired AEX deployment
is necessary. The owner-authorized main merge awaits no-Options reopen validation.

## M3-06 / build 27 - native dependency lookup and constant-node reuse

Owner AE2023 feedback for build26 confirms that delay no longer grows with time,
but uncached frames still take a subjective 6-8 seconds even with CEP closed.
Options reports roughly 39 ms preparation and 11 ms CUDA rendering in one sample.
Those selector timings exclude AE work before/after entry. The earlier screenshot
with 56/57 constants used Life queries; the fully certified Static path used zero
birth-node queries. Therefore the proposed (node, birth-time) cache-miss analysis
cannot by itself explain the observed Static-frame wait.

Source inspection finds a distinct dependency defect: UUID lookup scanned every
effect and read numeric properties before establishing that they were UUID fields.
Emitter UUID indices 99..106 and Particle indices 143..150 overlap the renderer's
98..609 expression inputs. This could introduce self/cross-renderer expression
dependencies. Build27 (32795 / 0x801B) skips the expression's own effect by
propertyGroup(1).propertyIndex and gates other effects by the fixed hidden property
name before reading all eight UUID words. Effect display names/order remain free.
A generated-expression regression proves the old lookup reads renderer values and
the new lookup performs zero such reads. Actual AE latency impact remains a hypothesis.

Constant nodes are converted once per frame and reused across every birth time,
including mixed/Force graphs; dynamic nodes retain exact-time sampling. Equality
of sampled values never certifies a node. Static simple graphs reuse the already
sampled current-frame graph without a second batch of alias checkouts. Dependency
changes discard the optimization. No render-thread AEGP access is introduced.

Options upgrades the generated owned bindings in existing development projects;
click it once after deployment. Source keys/expressions and graph values are preserved.
Diagnostics include node requests/actual samples (N), separate main GPU setup and
UI refresh maxima, and the render's own prepared time (p). A time/duration-mismatched
SmartFX pair is rejected before borrowed pixel access, with equivalent rational times
accepted. Process-global last-call diagnostics do not measure full AE preview time.

Qualification: 3,383 scoped C++ checks pass (native sync 2,938; camera 12;
actual CUDA/OpenCL drivers 433), plus generated-expression/keyframe JS checks.
May 2023 SDK Release /MT builds all five AEXs and Core without compiler warnings.
Evidence: artifacts/build27-native-sync.log, build27-expressions.log,
build27-gpu.log and build27-native-build.log. These are not AE frame-time results.

Main/native IDs, parameter counts, schemas, PiPL flags, CEP native-gpu-24 and Core
C ABI3 stay unchanged. Full paired AEX deployment is required. AE 6-8 second latency,
key/expression edits, copy/reorder/undo/reopen remain owner gates; M3-06 stays open.
The requested main integration is pending this candidate's qualification.

## M3-06 / build 26 — UI/render proof matching and actual preparation costs

Owner reports build25 remains slow or becomes slower at later frames. Its
Options `History` count was measured in the UI and did not prove render-side
use. The strict equality of UI/render `effect_ref` was an invalid instance-key
assumption: the SDK defines it as an opaque callback reference. Whether this
was the owner's actual failure remains an AE qualification question.

Build26 (32794 / 0x801A) matches proofs by current graph node UUID, owned alias
index, and AE's all-time dependency-state comparison. A different callback
reference alone no longer rejects a proof or prefix. Copied layers with unequal
states remain isolated. Prefix versions are bounded and matched by host state;
UI analytic-profile publication also invalidates sampled prefixes created by
render copies. Comparison errors and unavailable proofs retain exact sampling.

Main CUSTOM_UI is implemented through a zero-sized COMP registration and EVENT
DRAW handler; PiPL/runtime flags agree at 0x02008466. On AE's recorded main thread,
DRAW refreshes numeric metadata after dependency changes. Equal all-time non-layer
states skip source reads. No overlay, ECW area, project write, undo record or
rerender request is created. Worker/non-DRAW/reentrant calls are ignored before
SDK access. This automatic path depends on the main effect receiving composition
DRAW events (selection and visible layer controls); Options remains an explicit
refresh when no DRAW callback arrives. No render/sequence-worker AEGP query is added.

Options now snapshots real last pre-render costs before UI refresh: total,
controls/history/scene milliseconds, Static/Temporal path, constant/total inputs,
actual history PF checkouts, PPS and Life queries. Smart Render wall time includes
GPU upload, dispatch, synchronization, resource cleanup and host world checkouts.
These are coherent process-global
last-call diagnostics, not an instance-specific or full preview-time measurement.
UI proofs are labelled separately. Main/native parameter layouts and IDs, CEP
native-gpu-24, and Core C ABI3 remain unchanged; full paired AEX build is required.

Focused checks cover distinct UI/render references with equal dependencies,
copy isolation, key/expression invalidation, undo, prefix publication, static
history bypass, UI callback/throttling/thread safety and real preparation timing.
3,253 scoped checks pass: native sync 2,671, camera 12, emission cache 157,
CUDA/OpenCL drivers 413. May 2023 SDK Release /MT builds all five AEXs and Core.
Actual AE t=2/10/100 performance, cross-project state equality, UI event delivery,
key/expression edits and reopen remain owner gates. M3-06 stays open.

## M3-06 / build 25 — bounded native history preparation

Owner confirms build24 runs on CUDA device0, but 10,000 PPS / Life 2, no keys
or extra nodes, becomes slower at later frames. The reverse birth loop continues
through expired births because the 1,000,000 Output cap exceeds the live population.
The previous global Life bound is 10,000 seconds; it is not the authored Life.

Build25 (32793 / 0x8019) certifies native source controls on supervised main UI
commits or Options: no source expression/keys, matching all-time owned-alias PF_State
before/after metadata capture. Render validates with PF_ParamUtilsSuite3; no render
AEGP read or retained source handles. Certified fields are hoisted into a frame-local plan.
Simple static Default Emitter/Particle/Output graphs with Life Random 0 use the
closed-form alive-slot path, skipping historical checkouts and snapshot encoding.
Other temporal graphs use certified Life bounds and optional emission prefix leases.
Linear/hold Life bounds use the complete key envelope; Life Random only shortens Life.
Bezier/expression Life retains the safe global bound. PPS constants/linear/hold keys
use analytic integration; nonlinear sources reuse fixed-lattice prefixes under equal
all-time dependency stamps and 30/60/120 Hz. Changes/errors/contention/eviction fall
back safely. Complete cached/uncached particle snapshots compare byte-identically.

3,480 focused C++ checks pass: native sync 2,101, camera 12, emission timelines 397,
emission cache 131, current nodes 379, CUDA/OpenCL drivers 409, main controls 51.
At cap1,000,000 / PPS10,000 / Life2, t=100 and t=10,000 both perform 20,000 Life
queries; core temporal evaluation measured 25.442/27.272 ms. The certified static
core path measured 2.421/2.033 ms. These exclude AE checkout, GPU dispatch and host
preview overhead and are not an AE frame-time claim.

Main/native layouts, public IDs, CEP native-gpu-24 token and C ABI3 stay unchanged.
A full paired build is required because the adapter and statically linked Core contract
both change. Existing build24 effects may remain. For an existing project, click main
Options once to capture metadata; it shows `History: N certified inputs` and requests
rerender. External Effect Controls/key/expression edits may require Options recapture
for full optimization; invalid proofs preserve correct historical rendering meanwhile.
Automatic recapture after every external edit remains an open workflow gate.

Owner AE gates: real end-to-end timings at t=2/10/100, key/expression invalidation,
reverse seek, undo/reopen and surrounding effects. No AE process was operated by the
agent. Do not close M3-06 from compilation or standalone timings.

## M3-06 / build 24 — AE native CUDA/OpenCL candidate

Build 24 (32792 / 0x8018) implements host-proposed CUDA/OpenCL device setup/setdown,
per-frame SmartFX eligibility and SMART_RENDER_GPU into AE GPU_BGRA128 output.
Main Acceleration is GPU by default, with a CPU selection at index 614/disk 1611.
The four control-node effects also implement GPU pass-through to avoid our own
CPU image copy between nodes. Main/node PiPL and runtime capability flags agree.
Unimplemented frameworks or initialization failures reject GPU support cleanly.
MFR, Compute Cache and DirectX support remain disabled; no private graphics context.

Core C ABI 3 adds an immutable, typed, ROI-relative sprite/tile scene and paired
release callback. CPU simulation/history/projection prepare the scene; GPU kernels
perform deterministic ordered rasterization. CPU/GPU share camera/shape geometry.
There is no full-frame CPU staging image or GPU image readback in the product GPU
path. Numeric scene uploads still cost time. GPU scene budgets reject before GPU
output checkout and negotiate CPU without truncating particles.

May 2023 SDK full Release /MT build succeeds. 2,432 scoped C++ checks and focused
JS round trips pass. 409 actual-driver checks exercise CUDA and OpenCL (including
an out-of-order OpenCL queue), main SmartFX and the real node-effect GPU selectors,
Circle/Rectangle/Cloud, camera, HDR/alpha, padded rows/ROI, cancellation, partial
allocation failure, ABI rejection and cleanup. Small-scene maximum float component
difference versus CPU is 0.00000175834; this is scoped evidence, not a global error bound.
A standalone 512x512, 20,001-sprite benchmark takes roughly 3.4–3.7 ms preparation
plus 1.2–1.6 ms allocation/upload/kernel/synchronization, versus 67–69 ms CPU rendering.
AE parameter/history capture, device startup and surrounding host effects are excluded.
Actual AE GPU dispatch, preview scales/ROI, render queue and end-to-end timings remain
owner qualification gates. Do not claim AE GPU support from these tests alone.

Fresh main effects are required (main manifest/schema 24, registered count 616
including implicit input). Existing IDs 1..612 and native node layouts are unchanged.
Reopen CEP for native-gpu-24. Full paired AEX/Core deployment is required for ABI 3;
no old-project migration. AE-certified PPS metadata and cross-frame prefix-cache
invalidation/reuse remain open; build 24 still samples native PPS.


## M3-06 — reference Particle controls and GPU performance

Active owner task; ADR 0026 owns the render/particle/transport contracts, shared
core and platform render sources, adapter sampling/node/main parameters, schema,
CEP, focused checks and build/deployment documentation. Fix Emitting/Auxiliary,
Uniform defaults, Life Random, procedural shapes/rotation/color ordering. Implement
AE-native CUDA/OpenCL rendering and default GPU selection; optimize temporal sampling.
AE 2023 only, fresh development effects, single-Junction standing deployment.
ADR 0026 was revised on 2026-10-03 after owner review: AE proposes the GPU framework;
use device setup/setdown, per-frame eligibility and SMART_RENDER_GPU with shared AE
worlds. No private D3D12/OpenGL context or full-frame CPU readback. The May 2023 SDK
has no DirectX backend contract. GPU implementation/host qualification are still open.

Build 23 completes the reference control/timing and prepared-node-sampling phase;
2,017 scoped C++ checks and focused JS checks pass. Core rate metadata/analytic
integration and prefix cache are implemented; AE metadata capture/invalidation
and cross-frame reuse remain open. Native GPU handlers/backends/default GPU UI
remain open. See build-matrix.md and compatibility-matrix.md; do not close M3-06.

## M3-05 — temporal controls and Particle color / build 22

Status: source implemented, 1,420 scoped checks pass, SDK build and deployment
verified; owner AE 2023 qualification remains open (ADR 0025).

Owner confirms Origin birth behavior in build 21. Audit and implement the entire
existing public parameter temporal contract, emission integration and reference
Particle color controls; see ADR 0025. Owns shared Graph/SequenceCodec/simulation/
history/evaluation/types, adapter sampling/parameters/records/SmartRender/compiler,
CEP inspector/gateway/editor, parameter schema, build fingerprint/version, scoped
regressions and documentation. Render.hpp / C ABI 2 remain unchanged. Native
Emitter/Particle control/layout changes require fresh effects; no compatibility migration.
Process start/stop and host settings remain separately authorized.


## M3-03 — emitter Origin birth history / build 21

Owner confirms build 20 renders but Origin keys translate every live particle.
Owns GraphEvaluation.hpp/.cpp, new core EmitterHistory.hpp/.cpp, native origin
binding accessors and new adapter EmitterHistory capture, SmartRender.cpp,
Starfield/Core MSBuild and CMake source lists, build fingerprint/targeted runner,
current/native scoped checks, PluginVersion, PluginFlags/EffectMain for paired
historical dependency declarations, and ADR 0024/architecture/roadmap/build/
compatibility/checkpoint records. Render.hpp and C ABI 2 remain unchanged.
Sample native Origin XY/Z at particle birth times through owned PF checkouts in
pre-render; encode a transient immutable history record for the DLL. Preserve
determinism, Auxiliary parent birth positions, force-relative motion, cancellation
and bounded memory. No project migration or process-global simulation state.
Animated rates/lifetime/forces and other birth controls require separate semantics.
AE visual interpolation/reopen remain owner qualification.

Build 21 source/SDK candidate complete: 921 focused checks pass, covering actual
historical PF checkouts, frozen graph/CPU pixels, Auxiliary birth origins and
failure/cancellation behavior. All five AEXs plus Core build; owner AE gate open.
Paired build 21 deployed through the existing single Junction with AE absent;
six hashes, prior backups and selected/pinned Core verified. Rollback recorded.

## P-02I — native Effect Controls commit / build 17

Build 17 owner evidence: build 16 still fails at delivery/parameter 4/stream -1/516.
Replace the cross-effect generic transport with direct node-owned graph compilation
and AEGP publication, with explicit caller registration ID. Preserve independent
node records, immutable saved render graph and existing schema/ID/ABI contracts.
Extend this card's owned files to NativeGraphCommit.*, NodeEffect.vcxproj,
Starfield.vcxproj, BuildWindows.ps1, Parameters.cpp and the targeted fixture runner /
compiler stub, as required to compile the shared publisher inside each node AEX.
Native sync 348/camera 12 checks pass with a generic API that rejects every call
and absent main registration. Renderer controls 38 scoped checks and final May 2023
SDK x64 Release /MT build pass at packed 32785 / 0x8011, no compiler warnings/errors
found. All four independent node AEX links pass. Owner AE acceptance remains open.

Source b0e9e6b pushed before build-17 AE-absent installation. Existing single
Junction, unchanged Core, all six installed hashes and build-16 backup hashes /
selector verified. Before/after records and rollback are in native-node-checkpoint.md.
No AE session or other host setting changed; real native editing gate remains open.

Build 16 follow-up: owner reports all tried native edits fail with error 516. Build 15 host
acceptance failed. Remove assumptions that a generic callback supplies a complete
params array/input image/count and renderer geometry. Read only required main
streams via AEGP; use synchronous borrowed renderer/layer refs and numeric geometry
from the node callback. Add phase/index diagnostics and fixtures with null generic
params/effect_ref/utils/pica and num_params=0. Split scalar/graph publish and verify
phases, validate stream types and keep exact checks only for integer receipts.
Keep the same owned files and schemas. Native sync 330/camera 12 scoped checks pass;
float storage and maximum revision are covered. Renderer controls 38 scoped checks
and final May 2023 SDK x64 Release /MT build pass, packed 32784 / 0x8010, no compiler
warnings/errors found. Owner AE qualification remains open.

Source 76801bb pushed before build-16 AE-absent installation through the existing
single Junction. Installed candidate hashes, unchanged Core and build-15 backup
bundle/selector verified. Before/after records and one-step rollback recorded in
native-node-checkpoint.md. No AE process or other host setting changed.

Owner reports native controls only reach rendering after a subsequent CEP edit.
Owned files: NodeGraphSync.*, NativeNodeGraph.*, GraphCarrier.*, EffectMain.cpp,
PluginVersion.h, targeted native sync fixtures/runner and checkpoint/ADR docs.
Dependencies: P-02G native records and P-02H layout 6. Keep schemas, parameter
IDs, Core ABI and separate node effects unchanged.

Replace the synthetic USER_CHANGED_PARAM cross-effect call with an acknowledged
COMPLETELY_GENERAL request. Carry the callback's edited value by UUID so saved
sibling streams cannot substitute an old value. Explicitly persist the renderer
graph/revision/checksum through AEGP streams, verify readback and rollback failed
publishes. Test a host that discards generic params-array writes and retains old
node stream values until the native callback returns. AE acceptance remains open.

Native sync 252/camera 12 and renderer controls 38 scoped checks pass. May 2023
SDK x64 Release /MT candidate build passes at packed 32783; schemas/IDs/Core ABI
unchanged. Source publication and AE-absent installation follow the checks.

Source 96ecf6d pushed before build-15 installation. Existing single Junction
retained; six installed hashes, selected unchanged Core and build-14 backups
verified. Standing AE-absent authorization used; no AE session operated. Native
Effect Controls behavior, undo/redo and reopen await owner qualification.

## Current P-02H candidate — 2026-10-02

Build 14 adds ordinary Force controls/curve, renderer Time Remapping/Preview and
fresh Max Particles 1000000, with bounded initial Auxiliary storage. SDK/editor
includes no longer name the nonexistent Headers/Win directory. Minimal requested
checks pass: Core 58, native Force 79, renderer setup/checkout 38; current-node CEP
suite and six-source parsing pass. May 2023 SDK candidate builds at packed 32782,
main 21/native layout 6/Force 2/Output 3/Core ABI 2. No AE session operated.
Source push precedes installation; deployment evidence goes in the checkpoint.

Source 6a32f94 was pushed before deployment. Build 14 is now installed under the
standing AE-absent permission; six installed/selected Core hashes and build-13
backups match. One existing Junction retained; no AE session operated. Host
acceptance and the reference main-effect capability matrix remain open.

## Current P-02G checkpoint — 2026-10-02

Build 13 candidate fixes native runtime edit dispatch, adds UUID effect selection,
camera projection and unified Auxiliary sources. Minimum owner-authorized tests
and the May 2023 SDK build pass; six modified JavaScript sources parse. Core ABI 2,
Emitter schema 5/native layout 5 require one paired deployment and fresh effects.
Source 2751a56 was pushed, then build 13 deployed with AE absent. Six installed
hashes/selected Core and build-12 backups verified; one Junction retained.
Source/publish/deployment evidence is tracked in native-node-checkpoint.md;
all new host behavior remains owner qualification work. No AE session operated.

## Previous P-02F checkpoint — 2026-10-02

Build 12 source implements shared Particle inputs, stable independent emitter
births, corrected Life range/steps, reference control naming/splits/units,
independent base and percentage curves, multi-emitter counts and preserved
copy connections. Sprite rasterization skips zero alpha and uses cancellable
exact rendering without a default coverage budget. May 2023 SDK and four
JavaScript parse gates pass. No test suites added/run or AE session operated.
Main 20/native layout 4/Emitter schema 4; no development migration. Source
pushed as cadab2a, then build 12 deployed with AE absent under standing permission.
Six installed/selected Core hashes verified; build 11 backed up; one Junction
retained. Owner acceptance remains open.
[Checkpoint](native-node-checkpoint.md).

## Previous P-02E checkpoint — 2026-10-02

Owner reports canvas deletion unavailable and redundant collapsed outer topics.
Build 11 adds Delete/Backspace, filters fixed Output out of delete/duplicate
selection, and pauses polling while the context menu is open. Actual outer
topic markers are removed from all native effects; main/native indices and
gateway bindings are compacted together. Main schema 19, native layout 3,
gateway native-node-sync-11; surviving disk IDs unchanged, no development
migration. SDK build and CEP syntax checks pass; no tests added/rerun. Source
pushed as cb53d50, then deployed with AE absent under standing authorization.
Six installed hashes verified; Core/generation unchanged, one Junction retained,
build 10 backed up. Owner AE acceptance remains open.
[Checkpoint](native-node-checkpoint.md).

## Previous G-06 checkpoint — 2026-10-02

Owner describes build 9 as basically working but reports cancellation dialogs,
multiple-active-emitter rejection and ineffective wire-click disconnect. Build 10
implements quiet interrupts, bounded multi-emitter population merge and wire/port
interaction fixes. SDK build and both CEP syntax checks pass; no test suites
added/rerun. Source pushed as c368bbf, then build 10 deployed with AE absent under
standing authorization; six hashes and selected runtime Core verified. Precise
owner AE acceptance remains pending.

## Previous P-02D checkpoint — 2026-10-01

Independent node records and transactions are source-integrated. Owner evidence:
build 3 renders but crashes on layer selection. Build 4 removes unused hidden
structural groups and passes 721 adapter checks and the May 2023 SDK build.
One-folder deployment/rollback checks pass; build 4 is deployed after authorized AE closure.
The previous eight CEP suite results remain valid for unchanged panel code.
Selection safety and native-node host acceptance remain open.
See [native-node-checkpoint.md](native-node-checkpoint.md).

This backlog is the task source for staged implementation work. Assign one task ID per branch/worktree. Tasks below have explicit file ownership to reduce conflicts; the integrating owner reviews and merges interfaces in dependency order.

## Current checkpoint

### P-02H — Reference Force, renderer globals and population defaults (2026-10-02)

- **Owner:** integration lead; owner follow-up to P-02G.
- **Dependencies:** P-02G, ADRs 0002, 0005, 0012, 0015, 0019, 0021.
- **Owned files:** current core settings/simulation/graph/renderer, native Force records and parameter setup, main renderer controls, CEP gateway/view/edit/curve surface, parameter manifests, SDK include configurations, focused current-node/Force checks and corresponding architecture, mapping, build and deployment documentation.
- **Scope:** remove the nonexistent May 2023 SDK Headers/Win include; set fresh Output's cap to 1000000 without allocating that cap for small populations. Align ordinary Force with observed Gravity/random, Wind, Spin/frequency/resist/delay, Air Density and a Wind/Spin percentage curve. Add functional Time Remapping and Preview globals on the renderer effect. Retain separately saved node effects. Record every remaining reference main-effect capability explicitly rather than registering inactive controls.
- **Acceptance:** owner-authorized minimal current-node/native Force/panel checks and May 2023 SDK candidate build; sources pushed before AE-absent deployment through the existing single Junction. Fresh effects for the development layout; exact reference motion and real AE acceptance require owner verification.

### P-02G — Native edits, effect selection, camera and auxiliary emission (2026-10-02)

- **Owner:** integration lead; current task after P-02F.
- **Dependencies:** P-02F; ADRs 0003, 0005, 0009, 0012, 0015, 0019, 0020.
- **Owned files:** core graph, simulation, renderer and render contract (`Render.hpp`), C ABI `PluginApi.h/.cpp`; adapter native synchronization, camera capture, SmartRender, flags/version/build metadata; CEP gateway, graph model/edits/view, panel/styles; native manifests, scoped current-node/native-camera/Emitter/Particle/panel checks, project C++ editor configuration, scoped compiler include tracing and corresponding architecture, ADR, mapping, build, checkpoint and compatibility documentation.
- **Scope:** fix native Effect Controls edits using runtime indices instead of the setup-only `uu.id` union; select the UUID-owned AE effect on node click; improve current controls and avoid redundant stream writes. Add immutable camera projection across the render boundary. Emitter supports an Auxiliary mode with incoming particle sources, probability, normalized parent-life interval and inheritance. Preserve independent node effects and project storage. Development schema/ABI changes require the explicit ADR 0020 plan, without old-project migration.
- **Acceptance:** May 2023 SDK candidate builds and modified JavaScript parses; owner verifies native Origin XY/Z changes, effect selection, camera movement/zoom and auxiliary births, undo/reopen and current-node interaction. No tests or AE sessions run without request. Prefer one paired deployment after AE closes, retaining the single Junction and verified rollback.
- **Evidence:** owner goal explicitly permits necessary minimal tests. Core current-node scope passes 28 checks; native edit callback 14, camera capture 12, actual Emitter/Particle selector scopes 77/73; three focused CEP suites pass. Build 13 candidate uses packed 32781, native layout 5/Emitter schema 5/Core ABI 2 and main manifest 20. No AE session operated. Native Origin/highlight/camera/Auxiliary host acceptance, non-square PAR, advanced reference source controls and Auxiliary live counter remain open. See ADR 0020 and native-node-checkpoint.
- **Editor follow-up:** owner reports 14 C/C++(135) missing camera members. Current ABI 2 defines every reported member; actual NativeSync -TraceIncludes compile passes 26 checks and proves current-header inclusion. Add project-scoped C++20/MSVC/Core/SDK configuration, clean workspace entry and ignored editor cache paths. Stale editor indexing remains a hypothesis pending owner reload; installed runtime unchanged. See editor-setup.md.

### P-02F — Shared Particle inputs, parameter controls and curve rendering (2026-10-02)

- **Owner:** integration lead; current task after P-02E.
- **Dependencies:** G-06, P-02E, ADRs 0005, 0009, 0015, 0016, 0019.
- **Owned files:** core `Graph.hpp`, `Graph.cpp`, `GraphConstruction.cpp`, `GraphEvaluation.cpp`, `Settings.hpp`, `CpuRenderer.hpp/.cpp`; adapter `NodeEffects.cpp`, `NodeRecord.hpp`, `NodeGraphSync.cpp`, `NativeNodeGraph.cpp`, `Parameters.cpp`, `PluginVersion.h`; CEP `graph_edits.js`, `graph_view.js`, `panel.js`, `starfield_gateway.jsx`, README; main/native parameter manifests and corresponding ADR, architecture, roadmap, mapping, build, compatibility and deployment docs. `Render.hpp` and the C ABI remain unchanged.
- **Scope:** Particle accepts multiple direct Emitters; each emitter keeps its own UUID/birth allocation while sharing Particle lifetime/appearance. Audit visible scalar ranges and explicit CEP scrub steps; Life defaults to 2, caps at 10000 seconds and scrubs by 0.1. Align supported labels/units with the observed reference table and split native Origin XY/Z. Remove accidental base-value writes to percentage curve knots. Skip transparent sprites and use cancellable exact rasterization without the default arbitrary per-frame coverage cutoff; retain opt-in finite work limits.
- **Acceptance:** May 2023 SDK candidate builds; changed JavaScript parses. Owner tests shared Particle inputs, controls/curve independence, direct native edits, rendering, undo and reopen. No tests added/run without request. Native layout 4/main manifest 20 and packed build 12 have no development migration. AE-closed deployment follows standing authorization; no process start/stop.

### P-02E — Canvas deletion and flat Effect Controls (2026-10-02)

- **Owner:** integration lead; active task after G-06 integration.
- **Dependencies:** P-02D, G-06, ADRs 0009, 0011, 0019.
- **Owned files:** `cep_panel/js/panel.js`, `cep_panel/jsx/starfield_gateway.jsx`, `cep_panel/index.html`, `cep_panel/README.md`, `ae_plugin/NodeEffects.cpp`, `ae_plugin/NodeRecord.hpp`, `ae_plugin/NodeGraphSync.hpp`, `ae_plugin/NativeNodeGraph.cpp`, `ae_plugin/Parameters.hpp/.cpp`, `ae_plugin/GraphParameter.hpp/.cpp`, `ae_plugin/PluginVersion.h`, `schema/parameters.json`, `schema/node-parameters.json`, and matching ADR, architecture, roadmap, parameter mapping, checkpoint, build and compatibility documentation.
- **Scope:** owner reports canvas node deletion unavailable and redundant collapsed top-level Output/Emitter/Particle/Force parameter topics. Add Delete/Backspace and share editable-selection filtering with the context menu; fixed Output cannot block deletion of selected editable nodes. Pause automatic refresh while the context menu is open. Remove actual outer topic markers and compact native/main stream indices, keeping surviving disk IDs and independent node records. Main manifest revision 19; native layout revision 3; no development migration or placeholder topic streams. Update gateway bindings/token together.
- **Acceptance:** SDK candidate builds and modified JavaScript parses; owner verifies canvas deletion/native effect removal and directly visible controls in AE 2023. No tests added/run without a request. AEX deployment requires AE absent or separately authorized closure under ADR 0011.

### G-06 — Multiple emitters and normal render cancellation (2026-10-02)

- **Owner:** integration lead; active task.
- **Dependencies:** G-05, P-02D, M2-05; ADRs 0005, 0012, 0015.
- **Owned files:** `src/core/GraphEvaluation.cpp`, `include/starfield/core/ParticleSimulation.hpp`, `src/core/ParticleSimulation.cpp`, `ae_plugin/SmartRender.cpp`, `ae_plugin/PluginVersion.h`, `ae_plugin/BuildWindows.ps1`, `cep_panel/js/panel.js`, `cep_panel/js/graph_edits.js`, `cep_panel/css/panel.css`, `cep_panel/README.md`, and matching architecture, roadmap, backlog, ADR, checkpoint, build and compatibility documentation.
- **Scope:** handle owner build-9 feedback: two normal cancellation dialogs and the one-active-emitter restriction. Keep independent native node effects, saved IDs, graph schema, the C ABI and Render.hpp unchanged. Partition each emitter's births over its own active Particle children; merge live births under Output's single cap. Preserve deterministic identity `(emitter UUID, local birth slot)`, bounded memory/work, force merge and one-pass appearance precedence. Never fill AE's error message for normal cancellation.
- **Acceptance:** May 2023 SDK candidate builds; owner verifies multi-emitter output and editing cancellation in AE 2023. Do not add/run tests without an owner request. Compilation is not host qualification.
- **Status:** SDK build and both CEP syntax checks pass; source pushed as c368bbf before build-10 deployment. AE-absent standing deployment completed; six installed hashes and selected Core verified, one Junction retained, build 9 backed up. Owner verification pending. Owner says build 9 is basically working but reports the three remaining errors; exact node-operation and bit-depth acceptance remains unrecorded.
- **Owner steering:** fix wire-click disconnect and snap a dragged connection to compatible ports within a constant screen-space radius. The node container must allow wire hit testing; hold automatic refresh during a wire press and commit once on release. Existing native graph transactions remain the save/undo path.

- **BUILD-23 (integration lead):** owner scope is now AE 2023 only. Script/MSBuild defaults and primary docs use the May 2023 SDK and `artifacts/plugin/2023/`; newer-host tasks are deferred. Owns build defaults and policy documentation only.
- **G-03:** emitter/output core runtime and graph/flat parity implemented in `070c33e`; the force/appearance kernels now complete the Alpha chain (closed-form gravity/drag integration, linear age curves for size/opacity/color, stage-order enforcement) and graph/flat parity is pinned by the core tests. The current core suite passes 11,909 checks and the May 2023 SDK build succeeds.
- **G-04:** `GraphParameter.cpp` implements AE arbitrary-data ownership/codec callbacks; legacy controls preserve old-project animation, Node Graph mode consumes a pre-render snapshot, and Capture Current Controls uses AE's supervised parameter-change path. `graph_from_controls` builds Emitter -> Particle -> Force -> Output, and every bound control is supervised so a Node Graph edit rewrites the canonical graph in the same user-change transaction. Native adapter tests pass 660 checks after the preview-scale, output-world, and appearance-projection regressions; the May 2023 SDK build succeeds. AE Controls save/reopen, same-layer duplicate, same-name copy/paste replacement and undo/redo passed. The H-01 host pass reopened the saved project in a new process with Control Source still in Node Graph mode, saved values, particle preview and CEP panel target intact; integrated graph-transaction undo, byte-level graph comparison, and build-1 project migration remain unqualified in AE.
- **M3-02 core + authoring surface:** gravity (17-19), linear drag (20), Color Start/End (21/22), Size End (23) and Opacity End (24) are registered AE controls whose defaults reproduce the previous look, so the visible improvement is reachable without the panel; the same controls are the panel's edit surface.
- **M3-01B implementation:** pre-release revision 12 uses direct full-resolution layer-pixel Size X/Y/Z values (0–100000, default 100 px) in AE controls, the CEP Emitter inspector, and graph keys 19–21. Box uses all axes and Sphere forms an ellipsoid; Disc keeps its dedicated Disc Size. Dimensions convert through full-resolution layer height and pixel aspect. The prior percentage semantics are intentionally dropped during development. Core/adapter regressions and AE 2023 host qualification are tracked in `docs/compatibility-matrix.md`.
- **P-02 panel / P-02A canvas:** `cep_panel/` now projects the revisioned canonical graph into a node canvas and selected-node inspector over ADR 0009 protocol v1. The earlier AE 2023.5.0 Build 52 form populated automatically after project/layer selection, a `Size` write updated the frame, and host undo restored the picture. The owner confirmed the node canvas now displays normally after reloading the CEP panel and reports no issue with the revision-7 AEX load. Integrated topology edits, layout save/reopen, duplication, undo/redo, stale-state rejection, and Node Graph synchronization remain to be qualified in AE.
- **Examples:** Spark, Snow and Floating Light are documented as exact parameter recipes in `docs/examples.md` and shipped as panel presets, so they can be accepted with or without the panel.
- **Time-dependent output flag:** `NON_PARAM_VARY` is present in PiPL/runtime because frame time changes rendered particles with constant parameters. MFR/GPU flags remain disabled.

- M0 architecture contracts and M1 SDK shell are in the source tree.
- The user confirmed the empty M1 shell loads in AE 2023. The current build-2, 24-active-parameter graph build has AE 2023 host evidence for controls, lifecycle, all bit depths, time consistency, and transparent particle output.
- **G-03/G-04 core and adapter:** the initial 6,196-core/395-adapter baseline is historical. The current core suite passes 11,909 checks and the adapter fake-host suite passes 678 checks. The May 2023 SDK build succeeds.
- **M2-06 AE 2023.5.0 Build 52 host pass:** candidate `D22D43BAD15C5173867907369B2EF3293A3FD601C308158665A1F3FD0AB0816B` was rebuilt, installed and loaded on 2026-09-28. A forced fresh render showed particles on the transparency grid, with no solid input pixels. The owner reports all 8/16/32-bpc render correctly and time consistency passes (exact frame-comparison procedure not recorded). Save/reopen, Ctrl+D, same-name Ctrl+C/Ctrl+V replacement, undo and redo passed. The CEP panel populated after project/layer selection without manual Refresh. That candidate was superseded: the installed build is now the H-01 split pair (`StarfieldParticle.aex` `B7362B01AC0E935D8AD596A70D61690DA4D586EEEC3E939328BA1BDC420069D5`, with the authorized development junction selecting `StarfieldCore-095219764514FFCA.dll`). The row-stride build `C0830F649A149990942B40531E25E23C0841FA6BED9C9805B9D3E233AB1ABCAB` was never loaded in AE. Other gates include Node Graph persistence, render queue, cancellation, and diagnostic isolation across multiple render contexts. See `docs/compatibility-matrix.md` and `docs/parameter-mapping.md`.
- **Product direction recorded (owner, 2026-09-27):** node-based editing is the essence of the reference product, so the graph foundation is promoted ahead of the M3 feature families; those families will be implemented as node types. See `Wave G` below and the raised `docs/roadmap.md` policy.
- **Static review carried out by the owner (2026-09-27):** the adapter geometry was rewritten to derive render-space scale from observed host worlds. M3-01 added Box/Sphere/Disc and seeded per-particle variation. AE 2023.5.0 Build 52 confirms Full/Half/Third/Quarter centre normalization, Quarter Point playback, and visually distinct Box/Sphere/Disc distributions. Exact distribution parity remains open.
- **Host feedback after manifest revision 2 (2026-09-27):** the first revision-2 build rendered nothing because point controls were read as percentages. A separate later preview-offset fix then assumed point values stayed full-resolution. The owner's 2026-09-28 AE 23.5.0 Build 52 screenshots prove that values are absolute pixels scaled by preview resolution; D-05 now restores that scale and removes the magnitude-based unit ladder.
- **Feedback rule adopted:** every unit or coordinate assumption that can move geometry silently must be covered by either a core test or a printed readout. Silent geometry shifts produce no AE error, so they must be observable by construction.
- Current Windows x64 `.aex` build succeeds against the May 2023 SDK; dual-SDK builds are historical. Local SDKs and artifacts are ignored by Git.
- Product direction: independent implementation of observed behavior. Static analysis is a feature-discovery aid, not proof of exact behavior or a source-code specification.

## Before agent work: human-owned Git checkpoint

### H-01 — Split the AE adapter from a reloadable core DLL

- **Owner:** integration lead.
- **Dependencies:** M2-05, G-03/G-04, ADR 0012. AE 2023 only.
- **Owned files:** `include/starfield/core/PluginApi.h`, `src/core/PluginApi.cpp`, `src/core/GraphConstruction.cpp`, `src/core/GraphEvaluation.cpp`, `ae_plugin/CoreLoader.*`, `ae_plugin/StarfieldCore.vcxproj`, `ae_plugin/Starfield.vcxproj`, `ae_plugin/SmartRender.cpp`, `ae_plugin/WorldBridge.*`, `ae_plugin/Diagnostics.cpp`, `ae_plugin/EffectMain.cpp`, `ae_plugin/PluginFlags.h`, `ae_plugin/BuildWindows.ps1`, `tools/Deploy-HotCore.ps1`, the legacy installer guard, `CMakeLists.txt`, `tests/core_tests.cpp`, `tests/RunCoreTests.ps1`, `tests/CoreLoaderTests.vcxproj`, `tests/core_loader_tests.cpp`, and matching architecture/build/compatibility documentation. Preserve the existing uncommitted work in shared files.
- **Scope:** use a fixed-width, versioned C ABI; load unique core DLL generations at runtime; pin the generation across pre-render/render; expose an explicit Options reload action; mix the generation into the SmartFX cache key; publish a workspace-local runtime manifest atomically. Keep parameter IDs, match name, graph schema and packed plug-in version unchanged.
- **Acceptance:** the `.aex` and DLL build independently; core-only edits do not rebuild the `.aex`; render output matches the monolithic build; failed reload retains the prior generation; an in-flight render never executes an unloaded generation; AE 2023 confirms manual reload updates Full/Quarter previews and survives save/reopen. Exact installed paths, hashes and host results go in `compatibility-matrix.md`. Host changes require ADR 0011 authorization.
- **Status:** repository build and automated gate passed; AE 2023 smoke gate passed.
  The core C ABI equals direct renderer pixels in a normal frame, rejects
  malformed input and releases its result. A loader harness confirms switching,
  failed-reload fallback, concurrent API leases, and a real render completing on
  its pinned generation before the retired DLL unloads. The full May 2023 build
  succeeds; `-CoreOnly` leaves the `.aex` SHA-256 unchanged. AE 2023.5.0 Build
  52 confirmed Full/Quarter manual hot reload, missing-DLL fallback, visual
  8/16/32-bpc particle output, transparency, and save/close/reopen. Subsequent
  owner checks covered effect copy/undo, Half/Third geometry, lower-layer
  compositing, emitter shapes, gravity and size changes on the split build. The first
  `/MD` split build crashed in AE's old app-local C++ runtime; `/MT` in both
  modules fixed that observed crash. AE `aerender` exported three Full-resolution
  8-bpc frames from both the current split pair and the earlier AE-qualified
  monolith; decoded RGBA pixels matched exactly at frames 51–53, and an
  alternate-color Core changed frame 51 as a cache control. Broader pixel parity
  and an in-flight AE render switch remain open. The 2026-09-29 loader
  parser now rejects a second manifest line; the loader harness and AE 2023.5.0
  Build 52 confirmed that a malformed manifest retains the prior Core. Current
  hashes and rollback are in `docs/compatibility-matrix.md`.

| ID | Owner | Work | Done when |
|---|---|---|---|
| GIT-00 | User | Initialize the Git repository and make a clean baseline commit. | Source, schemas, docs, `AGENTS.md`, and build scripts are tracked; `AdobeSDK/` and `artifacts/` remain ignored. **Observed: done — clean tree on `main`.** |

No worker should create a branch/worktree from an uncommitted moving baseline. After this checkpoint, create one branch/worktree per assigned task ID.

## M1 closeout gate

| ID | Owner | Work | Depends on | Done when |
|---|---|---|---|---|
| HOST-01 | User / host operator | Finish the M1 host smoke pass in AE 2023 and record exact AE build, OS, and result. | M1 shell already loads | Effect can be added/removed, renders the input unchanged, duplicates, undo/redo works, and a saved project reopens. Record each result in `compatibility-matrix.md`. |
| HOST-02 | User / host operator | Newer-host qualification, deferred by owner direction. | Owner reopens scope | Do not infer support from historical SDK builds. |

These are host-operated checks; code agents can prepare a concise checklist but cannot mark the results without host evidence.

## Wave A — tasks suitable for parallel assignment

### M2-01 — Freeze the render request contract and rational-time helpers

- **Owner:** core agent.
- **Dependencies:** Git baseline; ADRs 0002 and 0003.
- **Owned files:** `include/starfield/core/Render.hpp`, new core time/helper files, and the corresponding ADR edits.
- **Scope:** specify normalized signed rational time, overflow behavior, half-open ROI coordinates, downsample/pixel-aspect spaces, pixel row layout, and cancellation/error semantics. Keep AE types out of the core. Reconcile the request/output ownership comments with the actual value types.
- **Do not do:** add SmartFX code, particle physics, or a new third-party library.
- **Acceptance:** all invalid/overflow cases have defined errors; frame and output structures have unambiguous units and ownership; the AE adapter can convert SDK values without reinterpretation.
- **Status:** implemented. `Error.hpp`, `Time.hpp/.cpp`, `Render.hpp/.cpp`, ADR 0005, and amendments to ADR 0002/0003. `FrameSpec` separates layer geometry, the render-resolution frame grid, and the ROI; `RenderOutput` owns a staging buffer with an explicit `row_bytes`; `validate_frame` covers the invalid cases; `tests/core_tests.cpp` covers normalization, overflow, and rejection paths.

### M2-02 — Bind the parameter manifest to AE controls

- **Owner:** AE UI/adapter agent.
- **Dependencies:** Git baseline; `schema/parameters.json` and `Settings.hpp` are the public contract.
- **Owned files:** new parameter adapter files under `ae_plugin/`, `ae_plugin/EffectMain.cpp` parameter dispatch, `include/starfield/core/Settings.hpp`, `src/core/Settings.cpp`, plus mapping documentation. Coordinate before touching the shared entry-point file.
- **Scope:** register all eight controls with stable IDs, labels, ranges, defaults, and correct PF types; map popup values and AE point controls into core settings. Keep ID 0 reserved for AE's implicit input. Explicitly reconcile the one-based popup default (`Point` = 1 in AE) with the zero-based `EmitterShape::point` core enum.
- **Do not do:** add graph controls, custom panels, or render algorithms.
- **Acceptance:** each schema row maps to one AE control and one core field; bounds/defaults match `Settings.cpp`; seed validation respects the manifest's `2147483647` maximum; parameter IDs and match name remain unchanged.
- **Status:** implemented through manifest revision 3. `ae_plugin/Parameters.hpp/.cpp` registers thirteen rows (index 0 is the implicit input, so `num_params = 14`), including three velocity sliders, Emitter Origin, Emitter Size, and Velocity Spread. The seed now drives per-particle shape and velocity streams. Full mapping and unit caveats are in `docs/parameter-mapping.md`; current AE control behavior still needs host confirmation.

These two tasks can run at the same time: M2-01 owns render/time contracts; M2-02 owns parameter registration and settings mapping. M2-03 waits for M2-02's settings mapping, while M2-04 waits for the M2-01 buffer contract and M2-03's particle type.

## Wave B — deterministic CPU vertical slice

### M2-03 — Implement deterministic point-emitter simulation

- **Owner:** simulation agent.
- **Dependencies:** M2-01 and M2-02; `Settings` validation.
- **Owned files:** new `include/starfield/core/ParticleSimulation.hpp` and `src/core/ParticleSimulation.cpp`; avoid AE adapter and pixel-buffer files.
- **Scope:** generate point-emitter particles from seed and absolute rational time. Define stable particle IDs, birth ordering, lifetime boundary, velocity integration, and cancellation polling. Each render request must stand alone; do not advance process-global state.
- **Acceptance:** same request yields the same ordered particle list; out-of-order frame requests agree with chronological requests; invalid settings are bounded before allocation.
- **Status:** implemented in `ParticleSimulation.hpp/.cpp`. Emission slots are `k / birth_rate`, the lifetime interval is half-open, the population cap keeps the newest slots, and the clock is anchored at host time 0. Determinism, frame-order independence, cap behavior, and cancellation are covered by `tests/core_tests.cpp`. The point emitter honours all three axes of velocity but renders depth only from M5 onward.

### M2-04 — Implement the CPU sprite rasterizer and alpha-only particle output

- **Owner:** CPU renderer agent.
- **Dependencies:** M2-01; consume M2-03's particle instance type after its interface is agreed.
- **Owned files:** new CPU renderer and pixel helper files under `src/core/` and `include/starfield/core/`; do not touch AE selectors.
- **Scope:** rasterize particles into owned output storage over transparent black; honor ROI, dimensions, row bytes, pixel format and premultiplied alpha. Use checked size arithmetic and return typed errors.
- **Acceptance:** bounds/ROI are respected, padding bytes are not assumed absent, alpha is correct, and allocation/unsupported-format failures return without leaking buffers.
- **Status:** implemented in `CpuRenderer.hpp/.cpp` with alpha-only semantics recorded in ADR 0005. Sprites carry a one-pixel analytic coverage ramp; ROI, row bytes, transparent pixels, premultiplied source-over accumulation, requested straight/premultiplied output encoding, 8/16/32-bpc, bounded work, and cancellation during output encoding are covered by core tests. An empty ROI is a legal, allocation-free request. The public render request has no source pixel field.

### M2-05 — Add the SmartFX AE transport adapter

- **Owner:** AE rendering agent.
- **Dependencies:** M2-01 and M2-02; agree the renderer call contract with M2-03/M2-04 before integration.
- **Owned files:** AE adapter implementation under `ae_plugin/`; coordinate shared `EffectMain.cpp` edits with M2-02.
- **Scope:** replace legacy `PF_Cmd_RENDER` with the documented SmartFX pre-render/render path; use pre-render input metadata for geometry, calculate bounded ROI, copy particle output while host pointers are valid, and release every checkout on every exit path.
- **Do not do:** set MFR, float-color, or GPU flags as placeholders; keep a CPU fallback.
- **Acceptance:** host resources have one cleanup path, errors/cancellation map predictably, and PiPL/runtime flags agree with implemented selectors.
- **Status:** implemented in `ae_plugin/SmartRender.*`, `Parameters.*`, `EffectMain.cpp`, `PluginFlags.h`, and `StarfieldPiPL.r`. Pre-render requests empty input metadata for layer bounds/reference geometry and declares `result_rect = request ∩ layer` with request-independent `max_result_rect`. Smart Render performs the matching empty pixel checkout required before `checkout_output`, but never reads it; it generates particles over transparent black, clears the AE output world, and copies the core buffer. The clear is required because AE may seed output pixels from the source layer. `PF_OutFlag2_REVEALS_ZERO_ALPHA` is included in PiPL and runtime flags. Parameter checkouts use RAII; cancellation is polled during simulation, rasterization/encoding, and output-world writes; core errors map to stable AE errors. May 2023 SDK build and AE 2023 alpha-output visual check pass.
- **Deliberate M2 limits (documented, not hidden):** the adapter does not narrow `result_rect` to analytic particle bounds, so it always fills the requested region; content-bounds narrowing is deferred to a profiling task.
- **Geometry revision after static review:** the adapter no longer derives the render grid from `in_data->downsample_x/y` (the SDK documents that factor inconsistently: the header describes a divisor in the range 1–999+, while `Resizer.cpp` and `PathMaster.cpp` multiply by it as a scale). Render geometry now comes from observed checked-out worlds plus `max_result_rect`, `ref_width/ref_height`, and `par`, carried from pre-render to render in `pre_render_data`; a missing state block fails loudly. `tests/core_tests.cpp` pins the half-resolution mapping. Preview-resolution behavior still needs host confirmation.

### M2-06 — Integrate and qualify the first particle render

- **Owner:** integration lead.
- **Dependencies:** M2-02 through M2-05.
- **Owned files:** integration fixes across adapter/core plus compatibility evidence.
- **Scope:** produce the first visible point emitter over a defined frame, confirm transparent particle alpha, and capture repeatable outputs at the supported bit depths. Qualify AE 2023 only; newer-host adaptation is deferred by owner direction.
- **Acceptance:** AE effect controls affect deterministic rendered pixels; repeated/reverse-time requests match; project save/reopen preserves controls; all claimed host evidence is recorded.
- **Status:** AE 2023.5.0 Build 52 host checks confirm the visible offset is fixed, save/reopen and effect lifecycle work, all bit depths render normally, and the owner-reported time-consistency check passes. The rebuilt candidate renders particles over transparency; the adapter clears the output world before copying sparse particle pixels. The CEP parameter form auto-populates after project and effect-layer selection without clicking Refresh. Remaining host gates include Node Graph persistence, render-queue, cancellation, and diagnostic isolation across multiple render contexts.

## Diagnostics and shipped defaults (current triage)

| ID | Owner | Work | Done when |
|---|---|---|---|
| D-01 | AE adapter agent | Options-button readout of what the effect actually receives: frame time, live particle count, layer/frame grid, downsample, pixel aspect, every parameter as read, and the converted velocity | Implemented in `ae_plugin/Diagnostics.cpp` (read-only; not part of the render contract). A support tool, kept until M8 diagnostics supersede it |
| D-02 | Owner + lead | Decide the shipped default look. With `velocity = (0,0,0)` and a 2 s lifetime a fresh instance shows one overlapping cluster at the layer center, and at comp time 0 exactly one particle exists by construction, so the effect can look static even though it works | **Done (owner delegated):** the manifest default is Velocity `(0, 0.3, 0)` layer heights/s plus `velocity_spread = 0.15`, mirrored by `Settings`, `schema/parameters.json`, and `docs/parameter-mapping.md`. Defaults affect new instances only |
| D-03 | Lead | Split Velocity into scalars and keep point controls for positions only | **Done:** manifest revision 2 (three velocity sliders in layer heights/s plus a 3D-point Emitter Origin). Recorded in `docs/parameter-mapping.md`, `schema/parameters.json`, and `Settings.hpp` |
| D-04 | Build owner | Make a PiPL/runtime flag mismatch impossible to ship: declare the flags/version headers as `AdditionalInputs` for the PiPL custom build and verify the generated resource against them | **Done:** `Starfield.vcxproj` declares `AdditionalInputs` (`Outputs` stays, or MSBuild skips the step with MSB8018) and `BuildPiPL.ps1` fails the build when `StarfieldPiPL.rc` disagrees with `PluginFlags.h`/`PluginVersion.h`. This was the cause of the AE "global outflags mismatch" report |
| D-05 | AE adapter agent | Treat point controls as host-pixel positions, including preview scaling | **Measured, fixed, and host-qualified at Full/Quarter in AE 2023.5.0 Build 52:** Full delivered `1920,1080,1080`; Quarter delivered `480,270,270` with `ds 1/4`, `ref 3840x2160`, `grid 960x540`. Both normalize to the same center position; the adapter reverses the rational factors and the core no longer guesses percentages/fixed point from magnitude. Full/Quarter/anisotropic, Capture, and graph-sync regressions are added. Half and Third are now host-measured as well (`ds 1/2` grid `1920x1080`; `ds 1/3` grid `1280x720`; both normalize to the same centre), recorded in the 2026-09-28 acceptance block of `docs/compatibility-matrix.md`. |
| D-06 | AE adapter agent | Keep the emitter-shape readout visible: the closing `shape/esz/vspr/size/not` line is dropped from every Options report because the 255-character `return_msg` budget is spent by the lines printed before it, so the only readout of the emitter shape never appears | **Recorded 2026-09-28, deliberately deferred by the owner.** Fix = compact the writer (fold `shape` into the `cnt` line, or shorten the `layer`/`org` lines), which means a full AEX rebuild and reinstall, changing the recorded installed hash and needing ADR 0011 authorization. Geometry, force and count lines are unaffected. |

## Wave G — graph foundation (promoted ahead of Wave C by owner direction)

The owner confirmed that node-based editing is the product's essence. Wave A/B built the render boundary that the graph will feed; building M3's emitter/force families as flat parameters now would mean implementing each of them twice. Wave G therefore lands first, and M3 families become node types afterwards. M4-01/M4-02 are absorbed here.

| ID | Owner | Work | Dependencies | Done when |
|---|---|---|---|---|
| G-01 | Core graph agent | Typed node/port/edge/parameter model with stable identity domains and a validator (unknown node type, type mismatch, missing input, cycle, duplicate id) | ADR 0001 identity rules, ADR 0006 | **Implemented:** `Graph.hpp/.cpp` defines UUID node/edge IDs, typed port and parameter keys/values, bounded validation, emitter/output schemas and typed failures. Covered by reorder-stability, cardinality, cycle and feedback-boundary checks. G-02 adds core serialization; G-03 evaluates emitter/output graphs |
| G-02 | Core serialization agent | Bounded core serializer/parser with magic, schema version, counts, CRC, and safe unknown-version handling for `schema/sequence-format.md` | G-01 | **Core codec implemented:** `SequenceCodec.hpp/.cpp` validates before writing, emits canonical order, and bounds parsing before allocation. Regression cases cover all seven value kinds, round-trip, CRC, optional records, and malformed headers/records. No earlier graph schema exists to migrate. G-04 consumes these bytes in AE arbitrary parameter callbacks (ADR 0008) |
| G-03 | Core evaluation agent | Graph evaluation into the existing `RenderRequest`: dependency order, evaluation at rational time, cancellation, and emitter/output pixel parity | G-01, M2-01/03/04 | **Emitter/output runtime implemented:** `GraphEvaluation.hpp/.cpp`, ADR 0007. Stable dependency traversal, active-output reachability, semantic bounds, graph precedence and per-particle opacity, plus the M3-02 force/appearance stages. The current core suite passes 11,909 checks. Current schema stores constant parameters; animated sampling/history requires M3-03's contract |
| G-04 | AE adapter agent | Persist graph as AE arbitrary parameter data; retain animated legacy-control mode and provide explicit capture into node mode | G-02, G-03 | **Code complete; AE Controls host pass, integrated Node Graph gate open:** graph data, Control Source and Capture are registered; AE owns graph bytes and lifecycle callbacks; old projects select animated AE Controls via `USE_VALUE_FOR_OLD_PROJECTS`; capture writes the current-time graph and selects Node Graph in one supervised parameter transaction. SmartFX consumes a pre-render snapshot. AE 2023.5 Build 52 preview-scale point conversion is shared by render, capture, and graph synchronization; native adapter tests pass 660 checks. AE Controls save/reopen, copy and undo/redo passed on the build-2 candidate; the H-01 pass reopened the project in a new process with Control Source still in Node Graph mode and saved values intact. Integrated topology/curve transactions, byte-level graph comparison and build-1 project migration remain open in AE |
| G-05 | Core graph agent | Extend graph evaluation for Particle nodes, emitter fan-out, force chains and parallel force branches; preserve deterministic output and bounded work | G-01–G-04, ADRs 0006–0008, 0015, M3-01/M3-02 | **Portable core plus focused regression coverage pass; AE persistence/undo gates remain open:** `org.starfieldfx.nodes.particle` carries per-branch age curves; active Particle nodes are UUID-ordered and receive global emission slots by `slot % branch_count`, preserving total rate/cap and slot IDs. Force nodes support fan-in; each Particle branch accumulates reachable Force nodes once in stable dependency order. Active Force/Appearance bypass paths in explicit Particle mode are rejected, not silently dropped. Particle/Appearance precedence is resolved once per particle. Evaluation allocates one global-ID-ordered output buffer and writes each modulo branch directly into it without per-branch particle vectors or a final sort. The uniform gravity/drag kernel sums fields and integrates once. Emitter schema 3 has no Max Particles value; Output schema 2 owns the cap. Core suite: 11,909 checks; adapter suite: 678 checks. Multiple active emitters and stochastic allocation remain deferred. An output ancestry with Emitter but no Particle is transparent; active Force/Appearance without a Particle source fails. Emitter/Particle schema-1 and pre-release emitter/output snapshots are unsupported during development. A traversal work cap is pinned in ADR 0015. P-02B projects dynamic graph topology into CEP and routes add, connect/reconnect, disconnect, splice, delete, duplicate, move, and typed parameter changes through revision-checked transactions. AE carrier, persistence, undo, and render behavior remain unqualified. |
| P-01 | Owner + lead | Choose the AE 2023 dockable-panel bridge and specify its versioned protocol and failure behavior | Wave G contracts | **Architecture accepted in ADR 0009:** CEP `CSInterface.evalScript` → ExtendScript → supervised script-visible AE parameter streams → effect updates canonical arb graph in `PF_Cmd_USER_CHANGED_PARAM`. No direct arb-stream writes, sockets, or render-time AEGP queries. AE 2023 host gate remains open for scripted hidden-stream access and undo behavior. |
| P-02 | Panel agent | Build the dockable effect-control panel over ADR 0009, with grouped parameter editing through supervised AE streams | P-01; force/appearance parameter bindings | **Bridge and automatic discovery implemented; partial AE qualification:** source is installed through the authorized Junction and the earlier form auto-populated after project/layer selection without Refresh. Parameter reads/writes, undo grouping, stale-state rejection, and Node Graph synchronization remain to be qualified. P-02A supplies the new fixed topology view. |
| P-02A | Panel agent | Add a visible dockable node canvas for Emitter → Particle → Force → Output; show compact node cards, top/bottom ports, connectors, and a selected-node inspector. Permit node dragging and refresh selected target state without a manual click | P-02, ADR 0009 protocol v1, ADR 0014 | **UI visibility confirmed; layout lifecycle qualification open:** `cep_panel/` implements the compact top-down node canvas, type colors, target pin, viewport-wide marquee/group movement, cursor-anchored wheel zoom, middle-button pan, signed coordinates, minimap, and movable inspector. The owner confirmed the canvas displays after reloading the CEP panel. The initial fixed four-stage view and eight AE layout streams remain for compatibility; P-02B now projects the canonical graph and persists UUID-keyed positions in the graph record. Dynamic graph edit and layout undo/save/reopen behavior still require the P-02B AE 2023 pass. |
| P-02B | Graph/panel | Deliver add, connect/reconnect, disconnect, splice, delete, duplicate, move and parameter editing | P-02A, G-01–G-05, ADRs 0013, 0015, 0019 | **Source/build pass; host acceptance open.** Revision 16 uses separate hidden node effects and numeric trigger 43; request 90 is removed. Guard 42 batches Output changes. Rollback restores native records and compiled render state; semantic acknowledgement permits AE quantization. Eight focused CEP suites and 687 adapter checks pass. The build blocker is resolved; build 3 is deployed. The preceding build-2 preview crashed before node tests. Safe apply/preview, actual node operations, rendered response, undo and reopen remain open. See the current checkpoint. |
| P-02D | AE adapter/panel | Make independent node effects the saved source and compile them into the renderer snapshot | P-02B, G-04/G-05, ADRs 0008, 0009, 0012, 0019 | **Build 9 deployed 2026-10-02; six hashes verified; fresh-effect owner test pending.** Build 8 still fails with the same duplicate-matchname error. All node FourCC disk IDs violated the SDK's 1..9999 contract. Registration and supervised lookup now share explicit numeric IDs. Compile-time guards cover allocation range/uniqueness and name budget. Native identity revision 2 has no development migration. Stream indices, main schema 18 and CEP token native-node-sync-6 unchanged. SDK build passes; no tests added/rerun. Host truncation is inferred. Owner confirms selection safety and Emitter duplication. Particle creation, native 32-bpc operation, render/cache response, undo and reopen remain open. |
| P-02C | Panel/core/adapter | Add editable piecewise-linear Size and Opacity over-life curves to the Particle inspector, persist them in the AE project, and evaluate them in Particle/Appearance nodes | P-02A, G-03–G-05, M3-02, ADR 0016 | **Curve editor and graph persistence source implemented; percentage contract being integrated; AE host gates open:** both fixed 0–100% curves multiply the Particle base Size and Opacity. The CEP plot adds, drags, removes, and selects up to eight knots. Life percentage and value support numeric entry and horizontal scrubbing. In AE Controls mode the hidden project curve bank commits with the final-percentage endpoint and a supervised nonce; the gateway validates both banks before opening an undo group. In Node Graph mode the optional version-1 opaque curve payload and final percentage commit in one revision-checked graph transaction. Missing payloads mean a straight curve from 100% to the endpoint. Base Size/Opacity edits do not rewrite curve points. AE parameter binding, graph carrier undo, save/reopen, and rendered response need the owner’s AE 2023 pass. |
| P-03 | Panel agent | On-screen emitter gizmo using `PF_EffectCustomUISuite2` + overlay theme (Drawbot only; no graph editing in-effect) | P-02, ADR 0005 geometry | Dragging the gizmo changes the emitter position and matches the rendered trail |

P-02A display follow-up (2026-09-30): center node text; show Max Particles on the
Output inspector while preserving the existing AE parameter identity; replace the
Output placeholder caption with a live/max count sampled from the current comp time.
P-02B now projects the committed canonical graph into the canvas. The Output count
reads the unique emitter in the active output ancestry, including Node Graph mode;
parked emitters are ignored and multiple active emitters fail closed. Focused graph
view and gateway tests cover the count calculation and host-time response. AE carrier
and dynamic-layout host passes remain open.

P-02B integration update (2026-09-30): `graph_layout.js` stores a complete UUID-keyed
position map in the graph's optional layout record. The edit planner persists layout
through add, duplicate, delete, splice, and move; a wire splice reuses the dragged node.
The panel now loads the project graph and sends topology, layout, and parameter changes
through the revision-checked carrier. Native validation accepts incomplete/disconnected
topology so a wire disconnect can persist as one edit; when Output has no active emitter,
the renderer returns transparent output, and reconnecting restores deterministic particles.
Core regressions cover disconnecting each link in the default emitter/particle/force/output
chain, codec round-trip, transparent rendering, and reconnect parity. Source tests cover the
codec, projection, planner, transactions, and startup. AE callback, undo, save/reopen, stale
rejection, and render parity gates remain open; do not describe the integration as AE-qualified.

P-02B owner acceptance update (2026-09-30): despite the canvas and transaction
implementation, node addition and removal still do not work in the owner's AE
test. Do not report dynamic topology editing as delivered. The owner confirmed
the desired direction: each node should be an independent AE effect instance,
with node parameters saved on that instance. P-02D and ADR 0019 now
define the first native-effect synchronization spike before resuming general
graph editing.

P-02B splice follow-up (2026-09-30): the canvas now passes the node's pointer-drop
position into the splice transaction and includes other moved selection positions in
that same edit. A focused CEP edit test checks the exact node drop coordinate, grouped
layout, and codec round-trip. The AE carrier and gesture still need host qualification.

P-02B duplication follow-up (2026-09-30): Alt-drag and Ctrl+D use the same graph
duplication policy from ADR 0019. Particle and Force copies retain compatible links;
Emitter copies stay disconnected, Appearance copies stay disconnected, and links
between selected nodes are redirected to their copies. Focused planner coverage checks
copied branches reaching the existing Output. AE effect creation, undo, cache update,
and save/reopen remain unqualified.

## Wave C — MVP controls and behavior families (after Wave G)

| ID | Work | Dependencies | Main ownership | Gate |
|---|---|---|---|---|
| M3-01 | Box/sphere/disc emitter distributions and documented coordinate mapping, implemented as node parameters | Wave G, M2-03/04 | Core simulation | **Core implementation complete:** `core::Random` supplies per-particle streams keyed by (seed, id, purpose); emitter shape, extent, velocity, and spread flow through the emitter graph node. Each shape is bounded and reproducible, and `seed` changes pixels. AE playback and preview-resolution confirmation remain open |
| M3-01B | Add reusable per-axis emitter size controls for Box and Sphere | M3-01, G-03/G-04, ADRs 0003/0006/0007 | Core simulation, AE adapter, CEP authoring | **Pre-release semantics revised by owner:** Size X/Y/Z at AE indices 81–83 and graph keys 19–21 are direct full-resolution layer pixels, bounded 0–100000 with a 100 px default. Box uses all axes; Sphere forms an ellipsoid. The common size parameter is now Disc Size, and Point/Disc ignore axis dimensions. `EmitterDimensionContext` converts X through pixel aspect and every axis through layer height. Regression/build gates and AE visual/save/undo qualification are recorded in `docs/compatibility-matrix.md`. Owned files: `Settings.hpp/.cpp`, `Graph.hpp/.cpp`, `GraphConstruction.cpp`, `GraphEvaluation.cpp`, `ParticleSimulation.hpp/.cpp`, `CpuRenderer.cpp`, `ae_plugin/Parameters.hpp/.cpp`, `schema/parameters.json`, `cep_panel/js/graph_view.js`, `cep_panel/js/panel.js`, `cep_panel/jsx/starfield_gateway.jsx`, `tests/core_tests.cpp`, `tests/graph_parameter_tests.cpp`, and matching ADR/parameter/compatibility/build documentation. |
| M3-02 | Particle age curves, size/opacity over life, gravity and drag as node types | Wave G | Core simulation/graph | **Core implemented; host confirmation pending:** force and appearance node types plus the four-stage chain (`GraphEvaluation.cpp`), closed-form gravity/drag integration and linear age curves (`ParticleSimulation.cpp`), per-particle RGB in the rasterizer, and AE controls 17-24 for authoring. Boundary behavior (birth, death, negative time, subframes) is pinned by `tests/core_tests.cpp`; AE playback and the panel's edit path still need the AE 2023 host pass |
| M3-05 | Deterministic Particle Size Random and Opacity Random controls | M3-01, M3-02, G-03–G-05, ADR 0018 | Core simulation, graph, AE adapter, CEP inspector | Add 0–100% stable per-particle attenuation after each age curve; append AE revision-11 controls and optional Particle/Appearance keys without changing prior project output. Focused core, graph, adapter, and panel checks pass; May 2023 SDK build succeeds; AE 2023 visual/save/undo qualification remains open. Owned files: `include/starfield/core/Settings.hpp`, `src/core/Settings.cpp`, `src/core/ParticleSimulation.cpp`, `src/core/Graph.cpp`, `src/core/GraphConstruction.cpp`, `src/core/GraphEvaluation.cpp`, `include/starfield/core/Graph.hpp`, `ae_plugin/Parameters.hpp/.cpp`, `schema/parameters.json`, `cep_panel/js/graph_view.js`, `cep_panel/jsx/starfield_gateway.jsx`, `tests/core_tests.cpp`, `tests/graph_parameter_tests.cpp`, relevant adapter tests, and matching docs. |
| M3-06 | Correct Particle lifetime ownership, curve authoring, and alpha compositing | G-05, M3-02, P-02C, ADRs 0003/0005/0015/0016 | Core graph/evaluation, CEP inspector, AE render boundary | **Source implementation, focused regressions, and the AE 2023 candidate build are complete; host qualification remains open.** Particle lifetime is owned by each Particle node. Revision 13 makes both curves fixed 0–100% multipliers of independent base Size (pixels) and Opacity (0…1); the native adapter keeps curve point zero from the project bank. The CEP gateway uses matching 0–100 bounds for both hidden curve banks. A new regression edits Size while preserving an existing Opacity curve with ordinates above 1. Core tests pass 11,905 checks, focused graph-view/edit/transaction, gateway, and startup checks pass, and the May 2023 SDK build succeeds. Candidate hashes and open AE checks are in `docs/compatibility-matrix.md`; straight-alpha rendering, curve response, undo, and save/reopen still require AE 2023 qualification. Owned files: `include/starfield/core/Graph.hpp`, `include/starfield/core/Render.hpp`, `src/core/Graph.cpp`, `src/core/GraphConstruction.cpp`, `src/core/GraphEvaluation.cpp`, `src/core/Render.cpp`, `src/core/CpuRenderer.cpp`, `ae_plugin/SmartRender.cpp`, `ae_plugin/WorldBridge.cpp`, `ae_plugin/Parameters.cpp`, `cep_panel/js/graph_view.js`, `cep_panel/js/panel.js`, `cep_panel/jsx/starfield_gateway.jsx`, `cep_panel/css/panel.css`, `tests/core_tests.cpp`, `tests/panel_gateway_tests.js`, `tests/panel_startup_tests.js`, `docs/architecture.md`, `docs/roadmap.md`, `docs/adr/0003-pixel-and-alpha-contract.md`, `docs/adr/0005-render-request-contract.md`, `docs/adr/0015-graph-particle-branches.md`, `docs/adr/0016-particle-over-life-curves.md`, `docs/parameter-mapping.md`, `docs/agent-backlog.md`, `docs/build-matrix.md`, and `docs/compatibility-matrix.md`. |
| M3-03 | Define animation/history semantics for graph values and finish remaining control organization | M2-02, Wave G | AE adapter/core contract | Legacy AE controls are checked out at requested render time and revision 6 groups the current controls. Graph values remain constants until an animation/history contract and migration are accepted. |
| M4-03 | Versioned preset import/export over the graph | G-02 | Core codec and AE commands | Round-trip and malformed-input behavior are specified; no implicit filesystem writes |

## Wave D — feature parity by observable family

Use `docs/reference-inventory.md` to choose one independently reviewable family per task: common forces/manipulators; texture/layer sources; mesh/material/light rendering; volumetrics; master switches and presets. Before implementation, add an observable reference case to `compatibility-matrix.md`. Static RTTI names alone are not acceptance criteria. P-01/P-02 protocol v1 is implemented; dynamic graph editing will need a later panel protocol after an observable topology-editing case is defined.

## Wave E — qualification and shipping

| ID | Work | Dependencies | Gate |
|---|---|---|---|
| M6-01 | Thread-safety audit and AE MFR qualification | Stable serial renderer, graph snapshots, cache ownership | Only then advertise threaded rendering; serial/MFR output agrees across repeated and out-of-order frames |
| M7-01 | Profile and choose any AE GPU backend | Correct CPU renderer and measured bottleneck | Documented API only, CPU fallback always available, bounded CPU/GPU output differences |
| M8-01 | Installer, migration/upgrade path, diagnostics, release matrix | Feature and MFR gates accepted | Clean install/uninstall, project migration, and supported-host evidence |

## Agent task handoff template

> Implement **[task ID]** only. Read `AGENTS.md`, the listed ADRs, and this task card. Keep edits within the owned files unless integration is explicitly assigned. Preserve parameter/match/version/schema contracts. Return a short summary, changed files, actual build/host evidence, and any unresolved decision. Do not claim an untested AE host is supported.


## P-02J — native node keyframes (2026-10-02)

Owner: primary adapter agent. Dependency: P-02I build 17, owner confirms edits work.
Owns NodeEffects, NativeNodeGraph, NativeGraphCommit, GraphCarrier, Parameters,
PluginVersion, schemas/parameters + node-parameters, gateway animated value writes,
scoped native/registration tests and runner, and matching architecture/roadmap/
ADR 0023/build/compatibility records. Core render API and simulation are unchanged.
Implement public stopwatches plus per-frame owned PF dependency inputs; constants
remain only for topology/identity/curve banks. Source/build checks precede deployment.
AE interpolation, undo/reopen and CEP-closed animation remain owner qualification.

Build 18 follow-up: owner reports loader token mismatch and incorrect animation.
Extend this card to own panel.js startup handshake and its scoped startup tests/
README. Correct paired-version validation using the actual JSX readiness response,
then investigate render-time sampling and owner-described animation behavior.
Also own NodeGraphSync.hpp's local failure-stage diagnostics for animation bindings.

Build 19 source candidate: confirmed panel/JSX mismatch fixed; typed effect/param
bindings, evaluated-value/state validation, sentinel rejection and rollback added.
798 scoped checks plus actual CPU pixels, generated-expression and JSX startup
regressions pass. Owner Origin XY black output is recorded; its binding-object
cause is a hypothesis pending AE 2023 qualification. SDK Release candidate builds.
Build 19 deployed with AE absent; all six installed hashes, prior bundle backup
hashes and selected/pinned Core parity verified. Owner fresh-layer animation gate
remains open; deployment and one-step rollback recorded in build-matrix.md.

Build 20 follow-up owns the same adapter/tests/docs files: owner opening error
stream -1; remove SmartFX's incorrect delivered-parameter-count guards from
animation sampling and render globals. Count-zero rejection reproduced before
repair, full graph/CPU pixel checks pass after. 812 scoped checks pass; new phase/
stream/count diagnostics preserve real checkout failures. Owner host acceptance
remains open; CEP and render contracts unchanged.
Build 20 installed with AE absent. Six candidate/installed and prior-backup hashes
plus pinned/selected Core verified; deployment and rollback recorded in build-matrix.


P-02J source/build/deployment complete: 761 scoped checks plus generated expression/
CEP keyed-write checks pass; build 18 installed through the existing Junction,
all six hashes/backups verified. Owner AE animation gate remains pending.

## M3-12 — Particle Transfer Mode 与节点效果定位（2026-10-08）

owner 要求：Normal/Add/Screen/Stencil 粒子间叠加；点击 CEP 节点后展开并滚动到原生效果。
拥有：Particle Core 类型/graph/求值/快照/CPU/GPU，Particle native 控件/记录/绑定，StarfieldHost UI reveal 命令、CEP graph inspector/gateway/preset 映射，node schema、Core ABI/version、配对发布、ADR0033。
依赖：M3-11 当前 native51/CEP51；owner 本次已确认上次删除和相对 Null 引用修复无问题。广泛 undo/reopen gate 保留。
迁移：ADR0033 先行；追加 disk232/stream519，保留现有 streams/IDs，graph key30 optional/Normal，private binding v3/snapshot5/Core ABI5。
完成条件：全链路实现与构建，AE 关闭后配对发布并核对哈希/回滚；本次不新增或运行实现测试。实机行为由 owner 验收。

## M3-13 — Particle Texture / Layer 采样（2026-10-08）

当前完整 goal 继续；不是以已完成的 Transform/Transfer 代替剩余要求。
拥有：Settings/ParticleTexture、Render.hpp/C ABI/快照、图求值/CPU/GPU fallback、AE texture 作者与 SmartFX checkout、CEP/资源/预设、schema/version、focused texture tests、ADR0034。
依赖：已部署 native52/CEP52、ABI5；参考字段和八种采样菜单已核对。Freeze Frame 经 owner 明确选择为粒子出生时的图层画面。
迁移：ADR0034 先行；Particle optional keys31..36，shape3，snapshot6/Core ABI6；新增 native/main 控件只追加，适配器布局在更改前补充 ADR。
完成条件：八种采样、正反面、颜色使用、比例/透视全链路实现，必要最小测试与 AE2023 构建、闭宿主配对发布；真实 AE 图层/undo/reopen 验收。其余 Face/Model/Cloud/Path/Shadow 等按后续卡推进，完整目标保持开放。
