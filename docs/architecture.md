# Starfield plug-in architecture

## M3-11 - shared sprite basis transport (source milestone)

ParticleInstance carries a uint32 basis index; EvaluatedGraph owns up to4096 shared
3x3 bases. CPU and portable GPU scene projection retain the table and apply the
same world/display/camera map. Identity index0 preserves the prior path. The
transient snapshot4 adds a table without expanding the200-byte particle record;
snapshot3 remains readable and is still emitted for graphs with no basis table.
Linear motion blur remaps/interpolates each endpoint pair once and reports typed
cancellation/bounds errors. Centre and sprite maps compose independently.

Core ABI4 guards the new capability with unchanged exported C/GPU layouts. Full
paired native deployment is required; runtime-only publication checks installed
AEX hashes against all five paired build artifacts before changing the selector.
This candidate is built with NoDistPublish/NoRuntimePublish. Installed native46/
CEP49 remains selected. Scoped math466/transport3544/current-node437/publication11
checks pass; actual AE and hardware GPU execution are not inferred. Graph kind,
ordered Force/auxiliary stages, native controls and sampled Null capture remain
the next M3-11 integration gates under ADR0032.

## P-02L / native46 + panel49 - left node palette

The expanded icon strip lists Emitter/Auxiliary/Particle/Force with a bottom
collapse/expand arrow. node_palette.js owns pointer capture, local ghost and
drop/cancel handling; panel.js owns coordinate conversion and the existing
guarded graph transaction. Output stays fixed; Transform is deferred until
functional. Movement makes no graph writes; polling pauses during drag. Existing
read requests cannot overwrite a committed graph due to interaction/epoch guards.
No render/native/wire change. Seven focused source/fake-host suites pass; actual
AE interaction remains open. CEP49 is installed with all22 hashes verified and
native46/selector retained; paired46/48 rollback is verified in report mode.

## M3-11 - Transform affine foundation

ParticleTransformSettings is a separate value type with no existing Settings/ABI
layout change. Compile an immutable affine transform once per sampled node, with
separate centre, velocity and sprite-axis maps. Local system scale affects spacing;
particle scale/opacity are independent multipliers. Full inherited linear motion
retains reflection/shear. 268 focused MSVC checks pass. No graph/native/CEP node
is exposed yet; ordered Force/auxiliary semantics, Null capture and CPU/GPU basis
transport remain open under ADR0032. Installed native46/panel48 is unchanged.

## P-02K / native46 + panel48 - background and preset cost

The owner confirms idle busy cursors stop when CEP closes. Replace200ms status/
1200ms full-refresh intervals with one500ms-to2s completion-scheduled pulse that
reads only current target/time/receipt/main-global markers. Full read-only state
and native graph share one host turn on changes/focus and nominal15s safety audits.
Reuse unchanged graph/geometry views. Native values and renderer sampling stay
authoritative; external same-frame changes absent from markers may wait for audit.
Initialized presets reuse one immediate planning snapshot; the host still rereads
and checks revision/stamp, validates and acknowledges before accepting the edit.
Native properties use request-only direct lookup memoization with one bounded
fallback index; clear on structural mutation and reply. Checked numeric UUID slots
replace repeated whole-parade lookup, with descending removal. Native46/Core ABI3,
temporal bootstrap and motion blur are unchanged. No speedup inferred from syntax
checks; actual multi-emitter idle/seek/native-edit/undo/Add/Replace remain AE gates.

## P-03 / native46 + panel47 - native snapshot versions and inspector scrollbars

The snapshot adapter resolves portable schemas through graph_edits only after
the full current ordinary-control key/type layout validates. It preserves existing
node data and does not upgrade imported graph hex or fill missing controls. Missing
native snapshots reject before authoring. Add keeps the existing native diff,
revision/stamp guards and semantic readback. Observed Emitter3 metadata producer
remains unconfirmed; current JSX emits7. Native46/motion blur contracts stay intact.
Local dark scrollbars and wrapping editor rows remove the bright inspector track
and unnecessary horizontal scrolling. Actual AE behavior remains owner evidence.

## M3-10 / native46 + panel46 - main-effect motion blur

Append the main Motion Blur topic and eight controls (indices616..625, disk1640..1649).
Comp Settings reads the host shutter interval and comp/layer flags. On uses custom
angle/phase; Off preserves the prior single-frame path. Type selects endpoint
particle interpolation or actual subframes. Capture one historical native plan
per exposure and immutable per-sample camera/graph data in pre-render. Core ABI3,
Render.hpp, snapshot3/200 bytes and native node layouts remain unchanged.
Output4 gains optional renderer metadata8..15; older authored presets acquire
defaults before snapshot matching. Native publishing, main checkout and CEP
roundtrip all include the controls. Disk lookup distinguishes the Motion Blur
topic from its popup. Main27/build46/CEP46 replace main26/build45/CEP45.

CPU averages complete float premultiplied samples before depth conversion. CUDA
and OpenCL composite each ordered tile list independently, then average; opacity
boost changes alpha while preserving color. Camera disregard freezes camera time
and retains sampled layer transforms. Nominal image-to-layer mapping applies to
every shutter sample. Camera geometry, resolved exposure and Core generation mix
into SmartFX cache identity. No additional host objects or DLL generations escape
their existing leases. Capture, graph, accumulation and GPU memory have explicit
limits; cancellation checks span sampling, interpolation, scene packing and copy.
Linear Accuracy maps to2..16 midpoint samples (70 gives12); Levels maps to2..64
exact subframes. Known short lifetimes use exact sampling at the Linear count.
Endpoint extrapolation for births/deaths and bounded samples are approximations;
numeric parity with Stardust is not claimed. See ADR0031.
Owner qualitatively accepts native45 curve behavior; undo/reopen gates stay open.

## M3-09 / native45 + panel45 - simplify Draw for Linear editing

Owner reports native44 Draw performance basically normal, but its samples become
an unwanted dense set of Linear handles. A mode-button transition from Draw to
Linear simplifies samples to at most 12 handles, preserving endpoints and the
overall shape. The owner's clarification requires approximate shape preservation.
CurveEditorModel and the CEP helper split the segment with the largest vertical
deviation until below 4% of the source value range or the handle budget is reached.
The budget can leave larger errors on complex/noisy strokes; exact parity is not
claimed. Native Particle Size/Opacity/Rotation and CEP editors use this rule. Other mode transitions
and explicit preset/paste operations retain their authored points. Atomic bank
publication, rollback and build44 stroke performance behavior are retained.
No disk ID, schema, wire, Render.hpp or Core ABI change. Advance AEX to45 and the
21-file CEP bundle/cache/gateway markers to45; save native44/panel42 for rollback.

## M3-09 / native44 - Draw stroke publication

Draw owns a CPU-only 64-sample draft keyed by UI context, native node UUID and
curve bank. Mouse events update this draft and invalidate only the control;
the last drag event publishes the whole bank once and requests composition
rendering. Keyboard cancellation, a newer host curve or context closure discards
the draft. No AE stream/world/suite objects escape a callback. UI state is bounded.
No-op strokes skip publication; only changed bank leaves receive AE change flags.
Curve previews use one Drawbot stroked path with at most 221 sampled vertices,
replacing the previous per-pixel rectangle sequence without changing evaluation.

Binding installation still resolves every alias and checks its stream type,
enabled state and exact expression against the freshly compiled identity and
parameter. Matching aliases need no value snapshot/evaluation. Installed or
repaired aliases retain sentinel setup, rollback and evaluated-value checks.
Render-time availability/identity checks remain intact. There is no readiness
cache. Main26, Particle7, Force3, Core ABI3, snapshot3 and panel42 are retained.
Build44 requires full AEX deployment; AE timing is not established by compilation.

## M3-09 / native43 - dense binding-record limits

Owner panel42/native40 reports working preset application but native Size mode
switch fails at animation binding, parameter36/stream-1/error516. Draw expands
the bank to64 knots; binding decode still rejected any node with more than81
recorded fields. Native43 derives both serialization and readback limits from
the existing per-kind base_parameter_count: Emitter34/Particle442/Force140.
Fields stay unique and within their node's authored indices; type, slot capacity,
finite-value, record length and version checks remain. Dense constant knots use
no animation aliases. Optional record0x8002/version1, all IDs/native schemas,
main26, CoreABI3, snapshot3 and renderer contracts remain unchanged. Panel42 is
retained. Full AEX deployment and actual owner click/Draw qualification required.

## M3-09 / panel42 - native disk-ID bindings

Owner panel41 evidence reports Size missing from Over Life, whose matchName is
org.starfieldfx.node.particle-2912. Registration defines2912 as a PF topic marker;
the41 assumption that it is a scripting PropertyGroup is rejected by this host
result.42 resolves curve count, interpolation and all64 knot slots from the root
effect by exact disk-ID matchName. Particle scalar Opacity uses204; curve count
uses800. IDs mirror schema/node-parameters.json and NodeRecord.hpp, including the
first8 and extra knot ranges. No name/topic/index fallback is used for these
controls. Reader and writer share the resolver; existing guard, revision,
rollback, timeout and atomic gateway-generation checks remain. Native40 unchanged.

## M3-09 / panel41 - curve scopes and atomic node calls

Native curve names are explicit entries bound to Over Life or Rotation Properties;
scalar Particle Opacity resolves in Particle Properties. This separates duplicate
public names while keeping native40 IDs/layout intact. Node panel mutations load
and call the gateway in one evalScript turn and validate generation in requests/
replies. Frequent read operations reuse a matching generation. Existing timeout,
pinned identity, revision and graph confirmation/rollback guards remain in place.
The owner-reported Rotation Curve Count is not in current source; stale runtime
origin is a hypothesis, pending owner-visible evidence. No native rebuild, host
cache/settings or protocol-version change accompanies this panel-only update.

## M3-09 / build40 - editor catalogs and four curve modes

Independent numeric data in schema/editor-presets.json generates native and CEP
catalogs. Native Particle Color Gradient and Size/Opacity/Rotation Presets open
an AE-owned modal thumbnail picker (20 curves,19 gradients, no folder replica).
Curve payload byte2 carries Linear/Hold/Bezier/Draw; Draw stores64 sampled knots,
Bezier uses automatic shape-preserving cubic tangents. Gradient byte2 carries
Linear/Hold. Editors, graph codecs, native bank writers/readers and evaluated
scene appearance share these modes. Force integrates constant/linear/cubic
segments under drag. Active native banks are captured/published together.

Particle7/base442 and Force3/base140 require fresh development effects/graphs.
First8 knot disk IDs are retained, additional knots/interpolation use explicit
3000-series IDs. Main26/index1/disk1631 retains its legacy8-knot transport;
Emitter7/base34, Output4, Render.hpp, numeric Core ABI3 and snapshot3/200 bytes
remain unchanged. Full paired deployment/rollback is required (ADR0030).
Build39's guarded visibility and canonical rotation confirmation are included.

## P-03 / build39 - guarded writes and semantic readback

The supervised native Panel Sync Guard opens conditional Color Gradient/Size Y
streams before script writes and restores their mode visibility on reset. It
does not publish partial authored banks. Missing Rotation Over Life is explicitly
reset/authored as the native two-point zero curve; confirmation recognizes only
that canonical default and reports other differences. Native IDs/schema/render
contracts are retained. Actual AE application remains an owner gate.

## P-03 / build38 - atomic preset bridge and dial defaults

Owner build37 evidence isolates a retained Particle2 target record; current named
native records and graph planners author Particle6. The stale producer is not
confirmed. Preset operations now load and invoke the gateway in one evalScript
turn with checked request/reply generation, retaining pinned identities and graph
transaction/readback/rollback guards. Native angles start collapsed without UI
callbacks overriding user expansion. Title paint failed to remove AE's disclosure
arrow; no public hide flag has been found and this issue remains unresolved.
All persistent IDs, node schemas, main26, Core/Render and bitmap remain unchanged.

## P-03 / build37 - preset version source and title ownership

Owner build36 evidence qualifies catalog loading, but rejects preset application
and removal of the remaining twirly. The planner now exposes its schemaVersion
accessor; preset validation uses the same table as node creation. Resource URLs
and gateway/bundle generations advance together. No versions are silently changed
in imported/project graphs. Errors preserve the graph phase and actual/expected
version evidence. Up/Folders/All presets navigation and Example removal stay in
CEP. ADR0029 extends the existing NO_DATA picture to own its title paint/click;
all persistent IDs, schemas, Core/Render contracts remain unchanged.

## P-03 / build36 - visible manager and picture-only entry

The owner exercises the build35 image and menu launcher, but sees an empty window.
Installed CEP path inspection finds the HTML/CSS present. ADR0029 corrects the
visible Modeless extension's AutoVisible flag, advances bundle/extension version,
and adds a visible dependency failure message. The lifecycle defect is identified
in code; AE content qualification is pending. No host cache or shared flags change.
The owner requests no folding title: main index1 is now untitled PF_Param_NO_DATA
with fresh disk1631/main manifest26. All authored indices and native schemas stay
unchanged. The packaged original illustration is resampled to1086x362; native
WIC/Drawbot callback ownership and image-failure guards remain unchanged.

## P-03 / M3-08 / build35 - presets and reference rotation

ADR0028 adds independent Emitter Orient, native AE angle dials, Particle Random
Limit/Limit Angle, Rotation Over Life and Anchor X/Y, alongside movable gradient
endpoints. Fresh Emitter schema7/base34 and Particle schema6/base102 effects are
required. Birth sampling remains intact; rotating particles use shared CPU/GPU
sprite geometry and snapshot3/200-byte transport. Core ABI3 and Render.hpp stay
unchanged. External Light/Null/source orientation contracts remain deferred.

ADR0029 replaces unused main stream1 with a constant Presets custom picture
(new disk1630, main manifest25). The original PNG is embedded in the AEX; WIC
retains only owned CPU pixels, and Drawbot objects stay within each callback.
The explicit click opens a separate CEP Modeless manager through the AE menu.
The manager's six procedural catalog entries and imported presets use the existing
bounded graph transactions, target/revision guards, native sync and readback.
No bindings/history are stored in portable files. Add remaps identities and keeps
one existing Output; Replace is explicit. Paired rollback covers both native
binaries and all CEP sources. AE launcher/manager qualification remains pending.

## M3-07 / build34 - negotiate Drawbot image layouts

Owner build33 gradient expansion triggers repeating Unsupported Pixel Format
warnings. ADR0027 replaces unnegotiated 24RGB with supplier-supported BGRA/ARGB
opaque bitmaps. A context-local guard precedes image creation, suppresses failure
retries, and retains path drawing when image support is absent. No schema,
authored-value, CEP, Core, render or GPU contract changes are involved.

## M3-07 / build33 - native UI correction

Build32 fails owner UI qualification (add-stop compile error, seams and internal
node parameters visible). ADR0027 now specifies one complete gradient bitmap and
event publication, zero ordinary/hidden UI dimensions, and actual AE dynamic
stream visibility. Gradient leaf supervision is removed to prevent intermediate
host-bank recompilation; public animated controls retain their supervision.
Schema5/base81/main manifest24/CoreABI3 and renderer boundaries are unchanged.

## M3-07 / build32 - native Particle gradient editor

Accepted build31 is on main/origin main at 8857303. Build32 is developed on
codex/m3-07-particle-gradient. ADR0027 defines native Drawbot gradient events,
bounded session UI state, native undo flags, whole-bank publication and the shared
Particle stream layout. Particle schema5/base81 uses new group IDs and existing
authored IDs; graph codec, main manifest24, CoreABI3, simulation and GPU remain
unchanged. Fresh Particle effects/graphs are required in development.
Gradient drawing/update callbacks only draw/update UI; click/drag/key callbacks
publish one complete stop bank, including values not yet visible to AEGP reads.
Native reader/binding schema and the Particle registry are updated together.
The paired rollback records both saved CEP files and newly introduced sources.

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


## M3-06 / build 23 — Particle controls and temporal preparation

AE 2023 / May 2023 SDK candidate, packed version 32791 (0x8017). Emitting now
means Default / Once / Sequenced / Randomized; Auxiliary Source is separate.
Unordered Point/Box/Sphere/Disc sources use default ordering for Sequenced/Randomized;
Once births an initial PPS-sized batch. Direction defaults to Uniform.
Particle schema 4 adds Life Random, Circle/Rectangle/Cloud, Size Y, Feather,
Up Axis, Orient To, Angle X/Y/Z and spin Speed X/Y/Z with native AE Angle controls,
Angle/Speed Random and Limit to 2D. Existing four color modes/gradient are retained;
the initial gradient colors are visible in Effect Controls. Cloud is an independent,
fixed five-circle procedural cluster, not the complete reference Cloud settings.
Shapes and rotation preserve transparent output in 8/16/32-bpc CPU rendering.

Main manifest 23 appends Simulation Settings 610..612: Time Sampling at index 611 /
disk 1601 offers 30/60/120 Hz, default 30. Emitter schema 6/base 31 and Particle
schema 4/base 75 require fresh development effects. Output schema 4 carries Hz.
Transient 0x8004 version 2 preserves the added sprite fields in paired AEX/Core;
C ABI 2 and Render.hpp remain unchanged. Reopen CEP for ready marker
native-particle-controls-23. No old-project compatibility path is added.

Historical capture now decodes bindings once per frame and samples one node
without copying the entire graph; Force conversions share a bounded evaluation
cache. Core EmissionTimeline implements metadata-certified rate*t, exact linear/hold
integration and fixed-lattice prefix reuse/inversion. 367 focused checks prove those
Core paths. **AE rate metadata capture, reliable invalidation, and prefix reuse across
AE frames are still open; the current adapter still samples PPS.** No constant curve
is guessed from equal samples, and no render-thread AEGP read is introduced.

GPU ADR 0026 follows host CUDA/OpenCL negotiation, GPU_DEVICE_SETUP/SETDOWN,
per-frame GPU_RENDER_POSSIBLE and SMART_RENDER_GPU into AE GPU_BGRA128 worlds.
The May 2023 SDK has no DirectX contract. GPU handlers/kernels, main Acceleration
selection, actual AE GPU qualification and end-to-end timings remain open. Build 23
is CPU-only and retains disabled GPU/MFR/Compute Cache capability flags.

Scoped qualification: 2,017 C++ checks across current-node behavior/pixels,
native sync/camera, emission timelines, all four node registrations and main
renderer controls; focused JS checks cover production gateway round trips,
generated UUID expressions/keyframe preservation, gradient encoding, panel
startup/error retention and node interactions. All pass. The paired five AEXs
and Core build with the May 2023 SDK Release /MT. No AE session is operated;
owner host rendering/keyframes/undo/reopen remain open.

## M3-06 GPU design correction — 2026-10-03

The planned GPU integration follows AE's framework/device negotiation and native
SMART_RENDER_GPU output worlds. Windows AE 2023 targets CUDA/OpenCL, using AE's
contexts, queues and device-memory suite; the May 2023 SDK does not expose DirectX.
The private D3D12/readback proposal was withdrawn before implementation. Core
prepares typed immutable scene data; host GPU resources and dispatch remain in
ae_plugin. Shared GPU output removes our full-frame readback, while CPU scene
uploads and historical sampling still have measurable costs. See
[ADR 0026](adr/0026-reference-particle-and-native-gpu.md) for selectors, the planned
scene ABI revision, resource lifetime and acceptance gates. Build 22 remains CPU-only.

## M3-05 / build 22 — temporal controls, Particle color and native Angle

Owner confirms build 21 Origin birth behavior. Build 22 integrates historical
PPS, samples all existing birth controls at actual birth times, and integrates
Force over lived time for ordinary/Auxiliary systems. Particle Color uses four
modes and a saved 2–8-stop gradient. Emitter rotations use native AE Angle controls
with turns/degrees, a dial and keyframes. Fresh Emitter/Particle effects required.
See [parameter time audit](temporal-parameter-audit.md) and ADR 0025 for the full
control matrix, precision, bounds and remaining reference/AE gates.

1,420 scoped C++ checks and focused generated-expression/startup/gradient JS
checks pass; all five May 2023 SDK Release /MT AEXs and paired Core DLL build.
No AE session operated. Owner AE 2023.5.0 Build 52 qualification remains open.
Build 22 is 32790/0x8016; native identity 3/layout metadata 7; main IDs/manifest,
Render.hpp and C ABI 2 unchanged. The CEP ready marker remains animation-19.



## M3-03 / build 21 — emitter Origin birth history

Build 20 owner playback renders but reveals current Origin moving all survivors.
Pre-render now samples owned Origin XY/Z at birth times and carries a transient
immutable history record to Core, shared across Auxiliary parent evaluations.
Ordinary velocity/force displacement stays relative to each birth position.
Wide-time dependency declarations match PiPL/runtime; history never persists in
project data or a process-global cache. IDs, Render.hpp and C ABI 2 stay stable.
921 scoped checks and the paired SDK build pass. Owner trajectory/interpolation,
cache invalidation after past-key edits and reopen need AE qualification.
Other animated clocks/birth controls and Force integration are separate work.

## P-02J / build 20 — SmartFX delivered parameter count

Owner build-19 opening error reports binding stream -1. Sampling no longer treats
PF_InData.num_params as the registered stream count; SmartFX passes no params[].
Actual checkout/checkin callbacks validate bindings and main render globals.
Count-zero full graph/CPU frame checks reproduce rejection before the fix and pass
afterward; 812 scoped checks pass. Diagnostics distinguish record/context/stream
failures and include delivered count. Host attribution remains an inference until
owner playback qualification. CEP animation-19 token and IDs/ABI stay unchanged.

## P-02J / build 19 — animation binding follow-up

Owner reports Origin XY keys produce black output and the panel rejects its
gateway. Paired animation-19 readiness fixes the confirmed token mismatch.
Bindings use documented effect/param methods, reject missing UUIDs instead of
zeroing values, and validate evaluated values/enabled state before publication.
Rollback restores underlying values as well as expressions. UI/render failures
identify the binding stream. The old callable-host-object assumption is a
hypothesis for the owner black output; AE playback acceptance remains open.
798 scoped adapter/camera/registration checks plus actual CPU frame regressions,
generated-expression tests and real JSX handshake tests pass. See ADR 0023.

## P-02J / build 18 — native keyframes

Owner confirms build 17 Effect Controls edits take effect. Public node controls
now permit keyframes/user expressions. Main manifest 22 appends 512 derived numeric
dependencies; UI commits bind native UUID/property/components, render samples its
own PF inputs at each requested time. Optional record 0x8002 holds typed mappings.
No render-thread AEGP reads, Core ABI or node layout change. ADR 0023 documents
capacity, fresh-development-effect qualification and current-frame kernel semantics.
AE stopwatches/interpolation, CEP-closed rendering and undo/reopen need owner tests.

## P-02I / build 17

Build 16 owner acceptance failed at delivery: its generic request was not acknowledged.
Native node USER_CHANGED_PARAM now directly calls a shared compiler/publisher compiled
inside every node AEX. It reads sibling node records with the node module's AEGP ID,
substitutes its accepted callback value by UUID/index, and saves/verifies/restores
the main renderer graph and integer receipts through AEGP streams. No cross-effect
generic call, transport payload or main edit selector remains. NativeGraphCommit.*
owns shared publication/snapshot helpers; NativeNodeGraph accepts an explicit caller
ID. Main CEP commits retain the same helpers, and render uses immutable saved graphs.
Independent node controls, main 21/layout 6/node schemas/Core ABI 2 remain unchanged.
Actual AE native edits, undo/redo and reopen still require owner qualification.

## P-02I / build 16

Native node USER_CHANGED_PARAM sends an acknowledged COMPLETELY_GENERAL request
to the renderer, carrying its accepted value and UUID. The native compiler applies
that value only to the matching node/index; other controls remain AE-owned streams.
The renderer publishes graph/revision/checksum/source with AEGP_SetStreamValue,
checks exact saved bytes and restores old values on failure. No synthetic callback
array is treated as persistence. Generic callback context is not assumed populated:
private request v2 borrows the node's suite/handle callbacks and layer/renderer refs,
with numeric source geometry/time. Only eight main control streams are read for
compilation. Publication/readback distinguishes scalar receipts from the arbitrary
graph. Receipt integers are float-exact and union reads require matching stream
types. Borrowed host objects never enter Core or persistent state. Build 15 owner
qualification failed with error 516 across controls; build 16 AE acceptance is open.
Rendering still consumes immutable saved graphs;
main 21/native layout 6/Core ABI 2 and node schemas are unchanged. See ADR 0022.

## P-02H / build 14

Fresh Output cap is 1000000 (maximum 2000000); low-population evaluation reserves
actual survivors, and Auxiliary does not reserve the full cap at startup. Force
schema 2/native layout 6 exposes scalar Gravity/random, Wind X/Y/Z,
Spin/frequency/resist/delay and Air Density, with a saved percentage curve.
Piecewise linear wind forcing integrates analytically under total branch drag;
random attenuation is salted by Force and Emitter UUIDs. Spin and its velocity
are explicit deterministic displacement equations. Reference numeric parity is
not inferred from names. Native effects remain separate and node controls flat.

Main manifest 21 appends streams 90..97 for Time Remapping and Preview. Output
schema 3 saves these globals; pre-render checks out their live animated values
and makes an immutable graph snapshot. The remapped clock is applied once before
recursive Auxiliary sampling. Preview filters stable identities after simulation,
preserving parents and full live counts. Core ABI 2/Render.hpp stay unchanged.
Fresh development effects required; see ADR 0021 and the main/Force comparison.

## P-02G / build 13

Native node edit callbacks dispatch by runtime parameter index. PF_ParamDef's
setup-only uu.id union cannot identify edits: during USER_CHANGED_PARAM it holds
change flags. UUID values and the batch guard still validate the owning node.
The renderer continues to consume immutable graph snapshots; no render-time
sibling reads are introduced. CEP node clicks select the UUID-owned AE effect
(Output selects the renderer), with coalesced transient requests and no undo group.
Unchanged node manifests skip redundant stream writes.

Core ABI 2 carries a numeric camera snapshot. The adapter captures SDK matrices
and the Core projects camera-facing sprites, clips behind-camera particles and
sorts visible sprites by depth. Main PiPL/runtime flags include camera dependency.
Layer inverse mapping avoids applying AE's later transform twice. See ADRs 0003,
0005, 0012 and 0020 for coordinates, absent-camera behavior and open host gates.

Emitter schema 5/native layout 5 adds Auxiliary mode and optional parent input 2.
Auxiliary reuses StarfieldEmitter.aex and the common motion kernel. Each child
samples parent position/velocity/appearance at birth and survives independently
after parent death. Chance, parent-life window and inheritance are percentages.
Output retains one cap; recursion/work are bounded and cancellable. CEP offers
Auxiliary creation, preserves stable native records, and omits an inaccurate
simple live-count estimate for Auxiliary. No old-project migration is provided.
Fresh development effects are required. SDK/scoped tests do not qualify AE behavior.

## P-02F / build 12

Particle input accepts distinct direct Emitters. The registry has no
emitter-specific merge prohibition; the edit planner preserves existing wires.
Each (Emitter, Particle) pair has one birth sequence, with shared Particle
properties and forces planned once. Emitter UUID/birth identity, deterministic
modulo child assignment and Output's global cap remain intact. Native records
still store up to four outgoing edges per node; all current inputs accept fan-in.
The CEP population counter sums actual per-emitter branch survivors before
applying the global cap. Emitter copies preserve mapped outgoing connections.

Native layout 4/identity 3 and main manifest 20 align implemented controls with
observed names: Life (Seconds), Size (Pixels), Origin XY/Z, Speed, Speed Random,
Angle X/Y/Z and percentage Opacity. Emitter schema 4 adds key 22 for independent
random percent; C ABI and graph envelope stay fixed. Life caps at 10000,
defaults to 2 and uses explicit 0.1 CEP steps. Base size/opacity never rewrite
percentage curve knots. Duplicate summary/record lookup functions are separated.

The renderer skips fully transparent sprites and has no default coverage-count
cutoff; explicit RenderLimits remain supported. Scan-row cancellation, graph,
population, ROI and storage bounds remain. Large visible sprites still cost
their full rasterization work; output is neither truncated nor approximated.
No development migration; use fresh effects. Source/SDK gates are distinct
from owner AE acceptance. See ADRs 0005, 0015, 0016 and 0019.

Current target: After Effects 2023 on Windows x64, built with the supplied May 2023 SDK. Owner direction on 2026-09-27 defers newer-host adaptation and qualification. See [the detailed roadmap](roadmap.md) for the support matrix and milestone gates.

## Goal and boundary

P-02E/build 11 uses flat native parameter layout 3 and main manifest 19: controls
appear directly beneath their effect headers, with no redundant outer topics.
Surviving disk IDs stay separate from compacted stream indices; arbitrary
callback disk ID 31 remains fixed. Gateway native-node-sync-11 and the node
direct-edit compiler share the new main commit index. Delete/Backspace/context
menu actions exclude fixed Output and use the existing native record transaction.
Core/graph schemas are unchanged; no development migration is provided.

G-06/build 10 evaluates multiple Emitters through their own UUID-ordered Particle
children and merges actual live births under Output's single cap. Internal
particle identity pairs Emitter UUID with local birth slot. Selected particles
write directly into a bounded final buffer; no per-emitter population allocation
or final sort. Host cancellation returns an interrupt without a dialog message.
CEP wire hits pass through the empty node container and connection previews snap
to compatible ports within 22 screen pixels. Independent saved AE node effects,
render snapshots, graph schemas and C ABI remain unchanged. SDK compilation
passes; owner qualification of these fixes is pending.

Native node parameter identity revision 2 (build 9) uses explicit disk IDs in
1..9999, defined in `schema/node-parameters.json` and shared `NodeRecord.hpp`.
Registration and supervised edit lookup use the same identities; compile-time
checks cover uniqueness, range and stream match-name length. Build 11 compacts
stream indices while retaining these numeric identities and graph schemas.
Unreleased FourCC IDs are not migrated; create fresh development effects.

This repository is the clean implementation of a node-based particle effect for After Effects. Existing reverse-engineering notes and binaries live in the parent directory and are reference material only. They are not linked into the build and must not be copied into the new implementation. Use observed user-facing behavior to define independent requirements; implement the algorithms and data structures anew.

The native effect is the product boundary. The AE SDK adapter translates selectors, parameters, pixel worlds, and host suites into a small host-independent C++ API. The core does not include AE headers, retain host pointers, access the filesystem implicitly, or call UI APIs.

## Layers

```text
After Effects
  ├─ StarfieldParticle.aex       main render effect, graph persistence,
  │                              SmartFX snapshots, host pixels and DLL loader
  ├─ StarfieldEmitter.aex        internal, menu-hidden Emitter node controls
  ├─ StarfieldParticleNode.aex   internal, menu-hidden Particle node controls
  ├─ StarfieldAppearance.aex    internal, menu-hidden Appearance node controls
  ├─ StarfieldForce.aex          internal, menu-hidden Force node controls
  └─ StarfieldCore.dll           graph evaluation, deterministic simulation,
                                 CPU renderer and later backends
```

The node AEX modules hold independent AE parameter streams; they do not render
particles. The existing `StarfieldParticle.aex` remains the only renderer and owns
the compiled graph snapshot. The independent node effects own saved authoring
records. The graph's fixed visible Output terminal is the
panel representation of that main effect; no separate Output AEX is built.
Node AEX modules set `PF_OutFlag_I_AM_OBSOLETE` so they are not offered as
standalone effects in AE's Effects menu. The CEP bridge still adds them by stable
match name to persist independent node values; AE 2023 must confirm that this
scripted creation path works while the modules are hidden from the menu.

Build 6 corrects the internal node 32-bpc contract reported by the owner: node
modules implement SmartFX pre-render/render pass-through and advertise SmartFX
together with float awareness (`flags2=0x00001400`). They forward the input ROI,
copy through the host world suite, and check in successful input checkouts on
every failure path. Parameter schema remains revision 18. Four actual-node
fake-host runs pass 276 checks; AE Particle creation/32-bpc qualification is open.

The main renderer has only one structural parameter group, Output. Hidden
bootstrap values use ordinary invisible controls; no invisible group boundary
participates in the ECW hierarchy (development schema 17). Installation exposes
the complete `dist` bundle through one `Plug-ins/Starfield` junction, with the
hot Core runtime in a real `dist/StarfieldRuntime` subdirectory.

Keep UI and preset compatibility as separate adapter modules. The AE 2023 dockable CEP panel uses the versioned ExtendScript bridge in ADR 0009. A graph transaction creates/removes the corresponding node AEX instances, writes their values, and commits the canonical graph snapshot to the main effect in one undo group. The Output terminal and its global controls remain on the main effect. Direct node-control edits have a supervised graph-sync source path, but callback delivery, cache/render response, undo and save/reopen remain AE 2023 qualification gates. The panel never shares C++ object layouts with the effect, and render code never queries sibling effects or panel state.

ADR 0012 defines the runtime C ABI and versioned development DLLs. The AE
adapter still owns graph serialization and control-to-graph construction because
those operations participate in AE parameter persistence; evaluation and
rasterization run in the selected DLL generation. Each pre-render pins that
generation until its matching render and result release finish. Its content
identity is mixed into the SmartFX cache key.

## Render contract

1. SmartFX pre-render checks out input metadata for layer bounds/reference geometry and time-varying values, determines the output/ROI, and constructs an immutable `RenderRequest`. Particle rendering does not consume input pixels; the core composites into a premultiplied staging buffer and encodes the requested output alpha mode. The current AE adapter candidate requests straight-alpha output pending focused AE 2023 confirmation (ADR 0005).
2. The core validates all external values, evaluates the graph deterministically for the requested rational time, and returns an owned staging buffer or a typed error.
3. The adapter copies pixels to the host output while checkouts are valid, then releases every checkout on every exit path.

Particle state must be reproducible from graph parameters, seed, and absolute time. Do not advance a process-global simulation by “one frame”; AE can request frames out of order, repeatedly, or concurrently. Random streams should be derived from stable particle IDs and the effect seed. Frame time remains rational through the adapter and is converted once at the simulation boundary.

Each Particle node owns its lifetime and retires only the particle slots assigned
to its branch. Emitter schema 3 has no lifetime or Max Particles parameter; Particle
schema 2 requires its own lifetime. Output schema 2 stores one global Max Particles
cap in the main effect's graph snapshot and keeps stable particle-ID ordering across
branches. Particle Size is a base diameter
in full-resolution layer pixels (bounded at 100000 px). Size Over Life and Opacity
Over Life each use a fixed 0–100% curve that multiplies the corresponding base
value. Box and Sphere emitter dimensions are direct full-resolution layer pixels converted through frame
height and pixel aspect; the Disc uses its dedicated layer-height diameter control.

## State and concurrency

- Global setup owns immutable plug-in metadata and acquired suite references. Global teardown releases them.
- The graph is a versioned serialized arbitrary parameter owned by AE; never mutate it while rendering. Sequence data remains unused in the current graph design.
- Render requests and graph snapshots are immutable. No mutable global/static render state.
- Advertise threaded rendering only after every selector and every dependency is audited for concurrent calls. No host suite or checkout calls while holding a lock.
- Shared expensive derived data belongs in AE Compute Cache with a complete content key, not in mutable sequence data. Cache keys include schema version, graph revision, time, quality, dimensions, format, and every checked-out input dependency.
- CPU is the correctness reference. GPU is optional and must produce a defined fallback when the required device/backend is unavailable.

## Compatibility rules

- Parameter IDs and effect match name are persistent project-file identifiers. Never renumber/reuse a released parameter ID.
- Node IDs, graph edge IDs, AE parameter IDs, and UI widget IDs are separate identity domains. The graph schema owns node/edge identity and migrations; AE parameter IDs map UI controls into that graph but never become graph IDs.
- Store a schema version in flattened sequence data. Deserialization is bounded, validates lengths, and migrates known older versions; unknown versions fail safely with a readable message.
- Keep display labels, UI layout, and preset import/export independent from stable parameter IDs.
- Describe output time dependence accurately to AE so its frame cache cannot return stale particle frames.
- Convert host pixel formats at the boundary. Respect rowbytes, channel order, alpha mode, pixel aspect, downsample, ROI, and 8/16/32-bit worlds.
- Export only the SDK-required C entry points (`PluginDataEntryFunction2` and `EffectMain`). Hide other symbols to avoid collisions with AE and other plug-ins.

## Source layout

- `include/starfield/core/Settings.hpp`: stable internal IDs, manifest bounds, and validated settings types.
- `include/starfield/core/AgeCurve.hpp`: bounded normalized-age curves and their nested versioned graph payload.
- `include/starfield/core/Graph.hpp`: host-independent node, port, edge, parameter, registry, and graph-validation contracts.
- `include/starfield/core/SequenceCodec.hpp`: bounded schema-1 graph serialization and parsing with CRC-32.
- `include/starfield/core/Error.hpp`: typed error codes and the `Result` primitive shared by the core.
- `include/starfield/core/Time.hpp`: normalized signed rational time with checked arithmetic.
- `include/starfield/core/Render.hpp`: host-independent frame/request/output/backend contract and the cancellation interface.
- `include/starfield/core/ParticleSimulation.hpp`: deterministic point-emitter evaluation for one absolute time.
- `include/starfield/core/CpuRenderer.hpp`: the CPU reference backend and its bounded-work limits.
- `src/core/`: implementations of the above plus bounded, finite-value settings validation.
- `include/starfield/core/PluginApi.h` and `src/core/PluginApi.cpp`: fixed-width
  render/inspect C ABI with opaque result ownership and generation-local release.
- `src/core/GraphConstruction.cpp`: graph creation shared by the adapter and DLL;
  `GraphEvaluation.cpp` stays in the DLL for frequent algorithm changes.
- `ae_plugin/CoreLoader.*`: versioned DLL selection, ABI validation and leases.
- `tests/core_tests.cpp`: host-independent self-tests for the M2 contracts.
- `ae_plugin/`: native SDK adapter, PiPL resource, Windows MSBuild project, and reproducible PiPL build scripts (see `ae_plugin/README.md`).
- `docs/parameter-mapping.md`: schema row → AE control → core field table and the emission rules.
- `docs/compatibility-matrix.md`: behavior inventory and independently derived acceptance criteria.
- `docs/adr/`: decisions that affect saved projects or rendering semantics.

The CMake build compiles only the portable core. The AE module is built by the Windows MSBuild project against the local May 2023 SDK by default. M2 implements SmartFX transport and 8/16/32-bpc CPU rendering; the owner reports all bit depths and time consistency pass in AE 2023. On 2026-09-28, AE 2023.5.0 Build 52 visually confirmed the updated adapter renders particles over transparency. G-03/G-05 source includes explicit Particle branches, deterministic emitter-slot partitioning, and branch-local force accumulation (ADRs 0007/0015); all portable core translation units compile and link, while regression and host checks remain open. G-04 persists a graph snapshot in an AE arbitrary-data parameter (ADR 0008). MFR, GPU, and Compute Cache remain disabled.

## SDK guidance used

- [Entry point](https://docs.yuelili.com/en/ae-plugin/effect-basics/entry-point/) and [command selectors](https://docs.yuelili.com/en/ae-plugin/effect-basics/command-selectors/) define the AE boundary and lifecycle dispatch.
- [SmartFX](https://docs.yuelili.com/en/ae-plugin/smartfx/smartfx/) supplies pre-render/render separation and explicit input dependencies.
- [Multi-Frame Rendering](https://docs.yuelili.com/en/ae-plugin/effect-details/multi-frame-rendering-in-ae/) documents thread-safety requirements and Compute Cache guidance.
- [Parameters](https://docs.yuelili.com/en/ae-plugin/effect-basics/parameters/) and [PF_ParamDef](https://docs.yuelili.com/en/ae-plugin/effect-basics/pf_paramdef/) establish stable parameter IDs and selector-scoped values.
- [PiPL resources](https://docs.yuelili.com/en/ae-plugin/intro/pipl-resources/) and [symbol exporting](https://docs.yuelili.com/en/ae-plugin/intro/symbol-export/) guide registration and minimal exports.

## Delivery sequence

1. **Complete in code:** M0/M1 contracts and the AE 2023 Windows x64 shell.
2. **Complete in code; host gate open:** M2 SmartFX transport, ROI, pixel-format adapters, and deterministic CPU rendering. See M2-06 in `compatibility-matrix.md`.
3. **Implemented in core and controls:** M3-01 seeded emitters, M3-01B direct-pixel Box/Sphere dimensions, and M3-02 gravity, drag, and age curves. P-02C adds bounded Size/Opacity polylines. M3-06 makes Particle own lifetime and makes curve interpolation selection non-destructive; a render reproduced dark translucent particles over blue, so AE currently requests straight-alpha output pending focused host confirmation. AE curve, lifetime, and compositing checks remain open. G-05 provides Particle branches and parallel force merges, with focused core and adapter regression coverage; AE qualification remains open.
4. **Graph authoring architecture:** P-02D uses independent hidden Emitter, Particle, Appearance and Force effects as the authoring source. Each owns its ordinary values, UUID, connections and position. Output stays on the main renderer. CEP reads these records directly and writes them under guarded transactions; numeric commit 43 builds the render snapshot, with receipt 44, revision 41 and checksum halves 90/91. All expression snapshots/mailboxes are removed. Revision-18 build 5 is deployed, 756 adapter checks and targeted CEP checks pass. The owner confirms build 4 fixed selection crashes; actual node operations, render response, undo and reopen await owner testing. Rendering never queries sibling effects.
5. **Remaining Alpha work:** qualify P-02D's native node-effect flow and graph lifecycle in AE before claiming P-02B/P-02C host completion; qualify G-05 behavior, define animation/history, and prioritize depth/projection, source types, and presets from observed behavior.
6. Keep MFR, Compute Cache, and GPU work behind separate concurrency/performance evidence and host-specific decisions.

## Current scope

H-01 separates the AE adapter from the runtime core and adds manual development
reload. The May 2023 SDK builds both binaries; 6,196 core checks, 395 adapter
checks and the loader harness pass. The `/MT` split pair is installed in AE
2023.5.0 Build 52. Full/Quarter hot reload, missing-DLL fallback, 8/16/32-bpc
visual rendering and save/close/reopen have host evidence. Three Full-resolution
8-bpc render-queue frames have exact decoded RGBA parity with the prior
monolithic binary; an in-flight AE render switch and broader pixel parity
remain open. The
prior monolith is backed up for rollback. Half/Third point mapping, split-build
copy/undo, shapes and basic gravity/size changes also have AE observations. See
`compatibility-matrix.md`.

M0/M1, M2 rendering, M3 core behavior, G-01–G-06 and CEP graph authoring source are present. Independent node effects own authoring data; the main effect owns Output and a compiled render graph. The gateway reads ordinary records without expressions or CUSTOM_VALUE scripting. Ready marker at index 87 prevents deleted nodes from being recreated; refresh prunes dangling links and re-keys raw duplicates. Build 10 addresses normal cancellation, multiple emitters and wire snapping; build 11 adds canvas deletion and flat Effect Controls. Exact node-operation acceptance, render response, multi-emitter behavior, undo and save/reopen remain owner gates. Automatic bootstrap with CEP closed, animation/history, MFR and GPU remain open.
