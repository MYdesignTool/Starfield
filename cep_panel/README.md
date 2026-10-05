# Starfield CEP panels

Native46/panel47 repairs Add's retained-node version projection. Only complete
current ordinary native-control records resolve to the current graph schema;
missing controls, unknown fields and future versions fail before any host write.
Imported graph presets retain strict schema validation. The inspector uses dark
8px scrollbars, vertical scrolling and wrapping curve/gradient controls.
Actual Add and narrow inspector appearance remain AE owner qualification gates.

Native46/panel46 adds main Motion Blur controls and preset render-setting roundtrip.
Comp Settings is the default; On uses custom Shutter Angle/Phase. Linear and
Subframe Sample render exposures on CPU/GPU. Nothing/Camera Motion are available;
PTF is deferred by the owner. Main27 appends616..625/disk1640..1649; native layouts
and ABI3 remain unchanged. Existing main effects acquire appended AE defaults.
Actual host qualification is recorded separately in docs/compatibility-matrix.

Up or Folders returns to the category grid; All presets shows every card. Example
selection is removed from the node editor. Both pages use build37 resource URLs
and gateway identity to avoid mixing script generations. Preset validation uses
the node planner's schema table; mismatch messages name the node and versions.
Restart AE and reopen both panels for the update. No authored schemas changed
from build36. Preset application and native title-icon removal need owner retest.

Build36 corrects the visible Modeless lifecycle and uses an untitled picture-only
entry. Restart AE to read the manifest version, then create a fresh main effect
(manifest26/disk1631). Emitter/Particle layouts are unchanged from build35.
The banner is packaged at1086x362. Actual content/no-title UI needs owner retest.

Click the original Presets picture in a fresh main effect, or Presets in the node
panel, to open Starfield Presets. Select a layer with Starfield before opening;
Refresh Target deliberately changes the pinned target. Browse categories, search,
select a preset, then Add or Replace. Add keeps existing nodes. Uncheck Apply
Render Settings to keep the current Output settings. AE undo restores graph edits.

Save Current and Import use explicit file dialogs for .sfldpreset files. My Presets
shows saves/imports in this manager session; reopen a saved file through Import.
The six built-in graphs use available features only. Card art is an illustration
preview, not an AE render. Fresh main/Emitter/Particle effects are required after
the development schema changes. Actual AE menu/Modeless qualification is pending.

## Historical build32 Particle Color Gradient

Accepted build31 is on main; development continues on
codex/m3-07-particle-gradient. Reopen CEP for native-gradient-32. Create a fresh
Particle effect/graph for the new Particle schema5 control layout.

In AE Effect Controls choose Particle Color > Color Over Life. The Color Gradient
bar supports adding and dragging stops, double-clicking to choose a color, and
Alt-click or Delete/Backspace to remove an interior stop. Flip, Copy/Paste and
presets apply the complete gradient. CEP has the same complete-gradient actions;
each editor keeps its own session clipboard. Endpoints stay at 0% and 100%.
Supported parameters are organized as Particle Properties, Over Life and Rotation.
See ADR0027 and docs/reference-parameter-map.md for remaining reference families
and the actual AE2023 qualification gate.

## Accepted build31 checkpoint

Appearance is removed by owner request. Emitter/Auxiliary, Particle, Force and
Output are the current nodes; Particle owns color/gradient, size/opacity and
Over Life. Recreate graphs containing Appearance. Remaining native disk IDs,
Particle schema4/base75, Emitter schema6/base31, Force schema2/base27 and CoreABI3
are paired; the unused Color End disk ID207 definition is retained.
Main layout35/36 now has Particle labels; main manifest24 and public IDs stay.
Reopen CEP for native-idle-31. Connection/layout edits preserve authored controls
and keys. StarfieldHost.aex adds bounded read-only initialization on active-comp
UI idle, requiring real AE2023 no-Options qualification. Older sections below are
historical checkpoints, not the current node inventory. See ADR0026.

# Build 24 / AE native GPU candidate

Reopen CEP and create a fresh main effect for main manifest 24 / C ABI 3.
Acceleration in main/Output defaults to GPU; CPU is selectable. AE must offer an
implemented CUDA/OpenCL device. Unsupported proposals and per-frame budgets can
choose CPU. Options shows the last main render path (process-global diagnostic).
No host project GPU setting is changed by the installer. Actual AE GPU rendering,
preview scales and render queue remain owner qualification gates.

# Starfield CEP panel

## Build 23 — reference Particle and independent Emitting timing

Close/reopen CEP for gateway native-gpu-24 after the paired AEX/Core
update. Recreate the development main/Emitter/Particle effects. Emitting is timing
(Default / Once / Sequenced / Randomized); creating Auxiliary sets a separate
source flag. Direction defaults Uniform. Particle exposes Life Random,
Circle/Rectangle/Cloud, Size Y, Feather, Up Axis, Orient To, Angle X/Y/Z,
Angle Random, spin Speed X/Y/Z / Speed Random and Limit to 2D in reference order.
Native angle/spin controls use AE turns/degrees and dials. Color modes and the
saved 2–8-stop CEP gradient remain independent of Size/Opacity over-life curves.
Main Output exposes Time Sampling: 30/60/120 Hz, default 30. Build 23 is CPU-only;
AE native GPU integration remains open in ADR 0026.

## Build 14 current controls

Reopen CEP for gateway native-node-animation-19 after installing the paired build.
The panel and JSX readiness tokens must match. The startup fixture now executes
the real gateway and checks stale-script reload with its actual readiness response.
Use fresh development effects (main manifest 21, native layout 6, Force schema 2,
Output schema 3). Force has scalar Gravity/random, separate Wind controls, Spin
and Air Density, plus an independent 0..100% Wind/Spin Over Life editor. Output
owns the million-particle fresh cap, Time Remapping and Preview chance. Example
setups also use that cap. The default is a limit, not an allocation or birth rate.
Renderer globals save/undo in the main effect and sample animated values at
pre-render. Preview keeps full simulation/live counts. Current ordinary Force
motion equations and remaining main controls are documented in ADR 0021 and
reference-main-force-comparison.md. Exact host/reference parity remains open.

## Build 13 current-node interaction and Auxiliary — 2026-10-02

Clicking a node selects its matching native effect by saved UUID; Output selects
the renderer. Selection requests coalesce and do not add undo entries. Native
parameter sync now uses runtime indices instead of the setup-only disk-ID union.
Existing supported control names/steps remain; Random Seed ends the source list.
CEP hides unused shape and inheritance controls. New Speed defaults to 100 pixels/s.

Add Auxiliary from the canvas menu, then connect:
`Emitter -> Particle -> Auxiliary -> Particle -> Output`.
For both parent and child particles to remain visible, also connect the first
Particle to Output. Auxiliary uses the same native Emitter effect, with Emitting
set to Auxiliary, and accepts several parent streams on its upper input. Set Emit
Chance and Emit Life Start/End to choose participating parents and their life
interval; Inherit Velocity/Size/Opacity/Color controls use percentages. Children
retain their birth state after the parent dies. Feedback cycles remain invalid.

Switching Emitting to Default in CEP removes parent wires in the same transaction.
Disconnect parents before changing that mode in native Effect Controls. Auxiliary
live count currently reports unavailable; a simple constant-rate estimate would
be incorrect. Speed Over Life/Inertia/Orient/time-sampling controls remain future
contracts. Active AE camera projection is implemented; actual AE camera/native
sync/selection/auxiliary behavior awaits owner testing. Non-square comp/source PAR
and extreme 3D layer transforms remain qualification gates.

Use fresh development effects for native layout 5/Emitter schema 5/Core ABI 2.
Close/reopen CEP for gateway native-node-sync-13. The paired bundle requires one
AEX deployment; subsequent compatible Core changes can use hot updates.
Build 13 is installed after source push 2751a56 and AE-absent checks; all six
installed/selected Core hashes and build-12 backups are verified. Host acceptance
is still pending. Use a fresh test layer/main effect and rebuild old test nodes.
[Deployment status and rollback](../docs/native-node-checkpoint.md).

## Build 12 shared Particle and numeric controls — 2026-10-02

Connect several Emitters to the same Particle top port. Connecting a new
Emitter preserves existing connections; force/output inputs also support fan-in.
Each node's native record holds up to four outgoing links. Emitter duplication
preserves mapped outgoing links; group copies use the copied Emitter.
The Output counter handles multiple emitter streams under its one global cap.

Life (Seconds) defaults to 2, caps at 10000 and drags in 0.1-second CEP steps.
Counts, seed, positions and dimensions use integer display/step 1. Size (Pixels)
uses 1-pixel steps, Speed 1 pixel/second, percentages/angles 1, Disc/Drag 0.01,
and Gravity 0.1. Shift is faster, Ctrl finer within displayed precision.
Type/Direction stay dropdowns. Origin XY and Origin Z are separate pixel edits
that preserve the other component. Opacity and Speed Random display 0..100%.
Changing base Size/Opacity leaves both Over Life percent curves untouched;
Speed Random retains its value at zero Speed. Both plot axes remain 0..100%.

Native layout 4/identity 3, main manifest 20 and Emitter schema 4 require fresh
development effects; gateway native-node-sync-12. Close/reopen CEP with the
new build. Default sprite coverage cutoff is removed and scan rows check
cancellation; large visible sprites can still render slowly. Explicit finite
RenderLimits fail without truncating output. SDK compilation and JavaScript
parsing do not establish actual AE acceptance. Build 12 is deployed after source
push cadab2a and AE-absent checks, with installed hashes/selected Core verified
and build 11 backed up. Owner acceptance remains pending.
[Deployment status and rollback](../docs/native-node-checkpoint.md).

## Build 11 canvas deletion and flat controls — 2026-10-02

Select editable nodes and press Delete/Backspace, or use Delete Selected in the
canvas context menu. Fixed Output is excluded from deletion/duplication, so a
mixed selection still acts on the editable nodes. Open menus pause polling;
marquee/context-menu interactions move keyboard focus back to the canvas.
Native record transactions remove the selected AE effects and incident links.

Native build 11 removes the redundant outer Output/Emitter/Particle/Appearance/
Force categories. Parameters appear directly under each effect header. Main
schema 19 and native layout 3 compact stream indices without development
migration; surviving disk IDs and Core remain unchanged. Gateway token is
`native-node-sync-11`; close/reopen CEP after loading the new AEXs. SDK build and
JS syntax checks pass. Build 11 is deployed with installed hashes verified;
owner AE acceptance remains pending.
[Deployment status and one-step rollback](../docs/native-node-checkpoint.md).

## Build 10 wire interaction update — 2026-10-02

Click a wire to disconnect it. The empty node container lets pointer events reach
SVG wires; a wire press pauses automatic refresh and commits once on release.
Keyboard Enter/Space activation remains available. Drag a port toward a compatible
opposite port: within 22 screen pixels the port highlights and the preview snaps;
release commits the connection. Invalid stages, self-links and cycles do not snap.
No AEX change is needed for these UI gestures. The existing installed CEP junction
points to this workspace; close/reopen the panel to load the changed sources.
Gateway token stays native-node-sync-6. Normal cancellation/multiple-emitter runtime
fixes require build 10's paired adapter/Core, now deployed through the existing
single Starfield Junction with installed hashes verified. Owner host acceptance
remains open. [Backup and one-step rollback](../docs/native-node-checkpoint.md).

## Current panel with native build 9 — 2026-10-02

Build 8 still failed with the same duplicate-matchname error. Build 9 replaces
out-of-range FourCC disk IDs with explicit IDs in 1..9999, shared by registration
and supervised lookup. Native stream indices and CEP sources are unchanged.
SDK build passes; installed hashes verified. Create a fresh Starfield effect;
old development IDs are not migrated. Particle creation awaits the owner's AE result.

Gateway token: `native-node-sync-6`. Errors from editing stay visible until manual
Refresh or a successful edit; failed default initialization waits for manual
Refresh. ResizeObserver work is deferred/coalesced so its delivery warning no
longer covers a native error. Close/reopen CEP to load the source changes.

The owner supplied AE's float/SmartFX contract error. **That fix requires the
build-6 or later node AEX**, not just CEP reload. Build 9 is deployed with hashes verified
through the existing single plugin junction. Emitter duplication works; Particle still awaits
actual AE confirmation. [Current checkpoint](../docs/native-node-checkpoint.md).

## Current CEP 5b update — 2026-10-01

The owner confirms Emitter duplication, but Particle default/addition still
fails. Errors now stay visible after background refresh and include creation
stage/match name/control details. A failed bootstrap waits for manual Refresh
before another attempt. Close/reopen CEP; no AE restart or AEX replacement is
needed. Capture one Particle addition failure to continue diagnosis. Current
gateway token: `native-node-sync-5b`; [checkpoint](../docs/native-node-checkpoint.md).

## CEP 5a hotfix after owner feedback — 2026-10-01

The owner reports build 5 creates native Emitter effects but repeatedly shows
`stale_graph`; copied effects do not appear in the canvas, and reopening CEP
loses the canvas. **Build 5 did not pass native node acceptance.**

The offending readonly `ensureNodeEffects` path compared a browser reconstruction
against the actual AE records and rejected reload itself. CEP 5a returns the
current Effect Parade records directly. Only mutation checks a host-produced
opaque authoring stamp, so decimal JSON/codec differences do not pretend that
the owner edited an effect. A targeted fixture reproduces rounded decimal JSON
and verifies a readonly reload performs no compile, copy/edit still works, and
a genuine intervening AE value change rejects before adding any effect.
The exact numerical mismatch in the owner's session was not captured; decimal
rounding is a reproduced hypothesis, not confirmed host evidence.

Numeric native payload CRCs remain diagnostic. A browser's rounded projection
cannot prove native byte equality; commit receipt, advancing native revision and
semantic node-record readback confirm a transaction. Additional supervised
callbacks may advance revision beyond exactly one. Failed graph reads keep the
last valid canvas and do not clear/re-show the error banner on every poll.

The gateway/loader token is `native-node-sync-5a`. This changes only workspace
CEP files through the existing extension junction: **no AEX replacement or AE
restart is needed**. Close/reopen the CEP panel. The actual effects are the source;
existing Emitter copies should become visible. If the Particle effect is absent,
add it from the node context menu and connect it. Partial first initialization now fills missing Emitter/Particle and initial links while ready=0; ready=1 deliberate deletion remains unchanged. Fresh-effect automatic bootstrap,
real render response, undo and reopen still require owner confirmation.


Current native build: revision-18 build 9, deployed 2026-10-02 through the
existing single `Plug-ins/Starfield -> dist` junction. Gateway token:
`native-node-sync-6`. The owner confirms build 4 fixed the selection crash.
Build 5 removes all expression access. Node effects own saved values, identity,
links and layout; Output and the compiled render graph remain on the main effect.
Actual add/copy/delete/render/undo acceptance awaits owner testing.

Dockable After Effects 2023 node editor for the top-down `Emitter -> Particle -> Force -> Output`
graph. The legacy AE Controls view uses protocol v1 of [ADR 0009](../docs/adr/0009-cep-panel-bridge.md).
Node Graph mode displays the graph compiled from separate hidden AE node-effect instances,
as specified by [ADR 0019](../docs/adr/0019-ae-native-node-effects.md); the inspector edits the selected node.
Cards are 110 × 54 pixels, use color by node type, and connect only from a lower output
port to an upper input port. Nodes can be placed freely; the
port direction remains top-in/bottom-out even when a downstream card is placed above
its source. Particle remains the node label for the particle
size, opacity, and color controls. Node labels and summaries are centered. Output
holds the Max Particles control and shows the live particle total for the comp's current
time; a lightweight read-only poll updates that total every 200 ms while auto-refresh is on.
Numeric values use AE-style scrub controls: drag left or right to adjust, Shift-drag
for faster changes, Ctrl-drag for finer changes, or click to type a value directly.
The panel formats and commits values at each control's precision (integer
particles-per-second and origin coordinates, configured precision for other floats)
and presents emitter type as a named dropdown.
The canvas supports a pinned target, top-down layout, project-saved node
positions, viewport-wide marquee/group movement, wheel zoom, unrestricted middle-button
canvas pan with a clipped viewport and no scrollbars, a bottom-left interactive minimap,
Alt-drag copy preview, Ctrl+D, port-to-port
connection gestures, and a graph context menu. Disconnect, insert, connect, add,
duplicate, and delete commands use a copy-on-write edit planner and a bounded,
revision-checked transaction coordinator. Dynamic card positions are stored in the graph's
project-owned optional layout record; the older eight layout streams remain only for the
four-card view.
The panel reads and writes **supervised render-value parameters** through a namespaced
ExtendScript gateway. In Node Graph mode it writes each node's values, links, identity, and
position into that node's own hidden AE effect instance. A numeric supervised trigger asks
the main renderer to enumerate those sibling effects on its edit callback, validate the
assembled graph, and replace its private render snapshot. No graph bytes travel through an
expression, and rendering never queries sibling effects. The fixed Output terminal is
backed by the main renderer and omitted from the node-effect manifest. The panel never
writes the arbitrary-data `Node Graph Data` property directly or accesses host-private
state. Effect Parade edits, callback/undo, save/reopen, and render response still require
qualification in AE 2023.

The gateway exposes `getGraphSnapshot`, `syncGraphSnapshot`, `ensureNodeEffects`,
and `submitGraph`. First synchronization directly creates Emitter → Particle →
Output using separate effect instances. Snapshot responses contain ordinary
native-node records. The CEP reconstructs their portable graph; numeric revision
41 and checksum halves 90/91 confirm the renderer compiled the same payload.
`submitGraph` compares revision and base records, updates node effects and Output
in one undo group, then raises numeric commit 43 and reads receipt 44.
No expression snapshot, expression mailbox or arbitrary-data scripting is used.

Close/reopen the CEP panel after deployment to load gateway `native-node-sync-5a`.
Use a fresh Starfield effect/layer for this development parameter schema.
756 adapter checks and the targeted gateway, transaction and startup suites pass.
The native-node gateway checks include add, duplicate, independent curves/values,
move, splice, connect/disconnect, native reorder/deletion, raw duplicate re-key,
rollback and an intentionally empty Output-only graph. These do not establish
real AE behavior; the owner is running the host test.

At startup, the panel requests the selected effect's state. If AE is still resolving the
project, selection, or ExtendScript gateway, it retries transient startup errors with a delay
that grows to a 5-second cap. Retries continue until state is found, then stop. **Refresh**
remains available for a deliberate re-query; it is not required for normal panel discovery.
After changing the panel's source files, close and reopen the CEP panel to load the new
JavaScript and ExtendScript gateway; the Refresh button only re-queries AE state.

## Files

| Path | Role |
|---|---|
| `CSXS/manifest.xml` | CEP 11 manifest; host `AEFT [23.0, 99.9]`; panel entry `index.html` |
| `index.html`, `css/panel.css`, `js/panel.js` | Panel UI and protocol client |
| `jsx/starfield_gateway.jsx` | `SFLD_getState`, `SFLD_getFrameStatus`, `SFLD_setParameters`, `SFLD_setNodeLayout`, and source-level graph carrier endpoints (public AE scripting DOM only) |

No third-party JavaScript is bundled. `js/panel.js` contains a ~10-line CEP bridge
shim around `window.__adobe_cep__.evalScript`; Adobe's full `CSInterface.js` can be
dropped in later without changing the protocol code.

## If other extension panels stop opening

This extension must never be able to affect anyone else's panel. Two things are host-wide,
and both are handled without ever writing host state:

- The manifest carries **no `CEFCommandLine` block**: CEP appends those switches to the shared
  CEF command line, which changes how *every* CEP panel in the host starts.
- `PlayerDebugMode` in `HKCU\Software\Adobe\CSXS.<n>` is what lets an unsigned extension load at
  all, and **every** other unsigned panel in the host depends on it. `Install.ps1` only links or
  unlinks the extension folder and never writes that key; the old `-RegistryOnly` switch that set
  and cleared it is gone. Clearing it once is exactly what made the owner's other panels stop
  opening, with no way back short of restoring the key by hand.

If panels go missing after an update:

1. Quit After Effects completely.
2. Run `powershell -ExecutionPolicy Bypass -File tools\cleanup_host_traces.ps1` — it reports by
   default and changes nothing unless you pass a switch. It prints the current `PlayerDebugMode`
   values (read-only).
3. If that value is not `1`, set it **by hand** (`reg query` to check, `reg add` only if you decide
   to). That is a deliberate host change, not a step in installing a panel.
4. Start After Effects. If the panels are still missing, run
   `Window > Workspace > Reset to Saved Layout` once; it rebuilds the panel layout without touching
   preferences or the registry.
5. Keep `%TEMP%\cep_cache\` and `%LOCALAPPDATA%\Temp\Adobe\CEP*` logs from the failing run: CEP
   records why an extension failed to initialise.

## Install for development

1. Close the previous panel in After Effects (if open).
2. Unsigned extensions must be enabled once. This is a **manual owner action**, not part of the
   install: the key is host-wide and every other unsigned panel depends on it, so no script in this
   repository sets it or clears it (ADR 0011).

   ```
   reg query "HKCU\Software\Adobe\CSXS.11" /v PlayerDebugMode
   ```

   AE 2023 ships CEP 11, so that is the primary key; check `CSXS.12` too if the panel does not appear.
   A read-only check on 2026-09-28 found no `PlayerDebugMode` key/value under either `CSXS.11` or
   `CSXS.12` on this machine. `Install.ps1` reports the value it finds and leaves it alone. If the
   owner decides to enable unsigned extensions, that host-wide setting must be changed manually;
   this project does not create or modify it.

3. Put this folder where CEP scans for extensions. Both roots work. On 2026-09-28, the owner
   authorized a Junction from the actual `cep_panel` source into the system-wide root below. AE
   lists and opens **Starfield Particle Controls** from that location. The Junction points to the
   repository source, so edits appear after reopening the panel:

   | Root | Path | Needs admin |
   |---|---|---|
   | user | `%APPDATA%\Adobe\CEP\extensions\<name>` | no |
   | system | `C:\Program Files (x86)\Common Files\Adobe\CEP\extensions\<name>` | yes |

   The folder name is the extension's identity in the menu; `cep_panel` and
   `org.starfieldfx.panel` (the bundle id) both work. Two copies under two names would appear twice.

4. Restart After Effects. Open the panel from **Window > Extensions**, entry
   **Starfield Particle Controls**.

### Updating the development installation

The current authorized installation is a Junction to the repository's `cep_panel/`.
Edits to HTML, JS, CSS or JSX are visible after closing and reopening **Starfield
Particle Controls**; the panel loads `jsx/starfield_gateway.jsx` on its first host
call. A manifest change requires an AE restart. A separate copied installation
would need its files copied again before reopening the panel. Changing or
replacing either installation path is an ADR 0011 host action with its own
authorization.

## Troubleshooting

| Panel shows | Cause | What to do |
|---|---|---|
| `bad_response: … unreadable data: EvalScript error.` | The gateway threw before it could answer. The first host run hit this because the entry points were private to the file's IIFE, so `SFLD_getState(...)` was a `ReferenceError`. | Fixed: the gateway publishes `SFLD_getState`/`SFLD_setParameters`/`SFLD_ready` on the ExtendScript global object and the panel loads it by path if the host has not. Update the installed copy (above) and reload the panel. |
| `bad_response: … unreadable data: undefined` | The gateway is not loaded in this session. | Same as above; the panel's self-loading path covers it. If it persists, confirm `jsx/starfield_gateway.jsx` exists in the installed copy. |
| `gateway_missing` | Neither the ScriptPath pass nor the self-loading path produced the gateway. The panel appends the JSX loader result, such as `missing_file` or `load_error`, when available. | Check that exact file in the active extension folder. The active copy is shown by `getSystemPath("extension")`; reload the panel after updating it. |
| `no_target` | No selected layer carries the effect. | Select exactly one layer carrying Starfield Particle; the panel retries automatically. **Refresh** is an optional manual re-query. |
| `missing_parameter` | The installed `.aex` build and the panel's binding table disagree (a parameter was renamed or removed). | Rebuild/install the current `.aex`; the bindings list the names the gateway resolves. |

## Use

1. Put `StarfieldParticle.aex` in the AE plug-ins folder (see the repository
   `README.md`) and apply the effect to a layer.
2. Select exactly one layer that carries the effect. The panel discovers it automatically
    and shows the effect's current graph. In Node Graph mode, add, connect/reconnect, disconnect,
    splice, duplicate, delete, move, and supported parameter edits submit graph transactions.
    Dynamic node positions travel with the project in the graph layout record; the protocol-v1
    four-card view uses the revision-7 layout streams. Pan and zoom are transient view state. Drag
    empty canvas anywhere in the viewport to marquee-select, move the middle mouse button to pan, and
    scroll to zoom at the cursor. Use the bottom-left minimap to inspect the whole graph and click or
    drag it to navigate. Alt-drag previews a duplicate; Ctrl+D and the
    right-click menu expose graph operations. AE 2023 cannot script-read the graph's
   `CUSTOM_VALUE`; node values live on separate hidden Emitter, Particle, Appearance, and
   Force effect instances, and a numeric callback compiles them into the renderer snapshot.
   Callback delivery, one-step undo, save/reopen, and render response still need qualification
   with the matching AEX. Click a node to open its floating properties window. The synchronization
   contract is documented in [ADR 0019](../docs/adr/0019-ae-native-node-effects.md). The checked
   **Refresh Automatically** option re-reads the selected AE target while the panel is visible;
   **Refresh** remains available for an immediate manual read.
3. Edit a value in the floating inspector: the panel validates it, writes it through the gateway in one undo
   group, and the composition updates.
4. **Example** presets: `Spark`, `Snow`, `Floating Light`, and `Reset Defaults` fill in
   the same values documented in [docs/examples.md](../docs/examples.md). They leave
   `Emitter Origin` untouched on purpose (its host unit is still an open question,
   backlog D-05).

`Control Source` is reported as `AE Controls` or `Node Graph`:

- `AE Controls` — the panel's writes drive the render directly.
- `Node Graph` — the effect rewrites its canonical graph from the delivered values
  during `PF_Cmd_USER_CHANGED_PARAM`, so the panel's writes still reach the render.

## Protocol v1 summary

Panel -> gateway requests are JSON strings. A `getState` request may omit target
and revision; a `setParameters` request must echo both from its latest `getState`
response:

```json
{ "protocol": "org.starfieldfx.panel", "version": 1, "requestId": "r2",
  "operation": "setParameters", "target": { "token": "<target-token>" },
  "baseRevision": "<revision>", "changes": [{ "key": "gravity_y", "value": -2.6 }] }
```

The gateway rejects the whole set on an unsupported version, an ambiguous or missing
target, a missing target token or `baseRevision`, an unknown binding, an out-of-range
or non-finite value, more than 32 changes, a payload over 64 KiB, a stale token or
`baseRevision`, or an animated parameter. Error codes:
`no_project`, `no_active_comp`, `no_target`, `ambiguous_target`, `no_effect`,
`missing_parameter`, `unknown_binding`, `invalid_value`, `stale_state`,
`animated_parameter`, `host_write_failed`, `host_error`, `invalid_request`,
`unknown_operation`, `no_host`, `bad_response`, `host_timeout`.

## Qualification status (partial AE 2023 host pass, 2026-09-28)

AE 2023.5.0 Build 52 lists the panel under **Window > Extensions** and opens the repository page
through the Junction. After opening the test project and selecting the layer with Starfield
Particle, the form populated automatically without clicking **Refresh**. The initial `No target`
state was caused by the panel polling before AE had a selected project target; startup retry now
waits for that transient condition to clear. No host preference or registry value was changed.
The owner then observed `Lookup: name` in the footer, a panel `Size` edit updating
the AE frame, and host undo restoring the picture. Undo initially left the panel
displaying the prior value; the client now re-reads on focus, and the owner reports
that values update again. These observations were made on the earlier grouped form.
The draggable canvas, floating inspector, pinned target, marquee/group movement, and zoom
are implemented in source, but this UI revision has not yet been viewed in the owner's AE
dock. The graph remains fixed: node creation, deletion, reconnection, wire disconnect, and
edge insertion require the graph-backed transaction contract in P-02B; protocol v1 only
edits the supervised parameter streams.
Remaining qualification:

1. Redo, panel focus refresh after other AE edits, and undo grouping across a batch.
2. A panel edit in `Node Graph` mode rewrites the stored graph bytes.
3. Save/reopen and effect duplication preserve panel-authored values; a stale
   `baseRevision` edit is rejected with `stale_state` in AE.

Record results in [docs/compatibility-matrix.md](../docs/compatibility-matrix.md).
Until that is done, treat the panel as a preview, not a supported feature.
