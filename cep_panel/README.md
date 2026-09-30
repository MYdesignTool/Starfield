# Starfield CEP panel

Dockable After Effects 2023 node editor for the top-down `Emitter -> Particle -> Force -> Output`
graph. The legacy AE Controls view uses protocol v1 of [ADR 0009](../docs/adr/0009-cep-panel-bridge.md).
Node Graph mode projects the canonical graph through the bounded expression carrier in
[ADR 0013](../docs/adr/0013-script-visible-graph-snapshot.md); the inspector edits the selected node.
Cards are 110 × 54 pixels, use color by node type, and connect only from a lower output
port to an upper input port. Reverse or overlapping connections are rejected, and the
drag preview is only drawn downward. Particle remains the node label for the particle
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
ExtendScript gateway. In Node Graph mode, it gets the canonical graph snapshot, plans a
revision-checked edit, creates/removes the matching Emitter, Particle, Appearance, or Force
AEX instances, writes their values, and submits the replacement graph through the main
renderer effect's supervised expression carrier in one AE undo group. The fixed Output
terminal is backed by the main renderer and is omitted from the node-effect manifest. The
panel never writes the arbitrary-data `Node Graph Data` property directly or accesses
host-private state. Editing a node AEX's controls directly in Effect Controls does not yet
recompile the graph. Fake-host tests cover graph planning, manifests, two independent native
Emitter instances, dimensions, identity streams, deletion, gateway startup, and projection;
Effect Parade edits, callback/undo, save/reopen, and render response remain unqualified in
AE 2023.

The gateway exposes `getGraphSnapshot`, `syncGraphSnapshot`, `ensureNodeEffects`, and
`submitGraph`. Graph reads and commits use ADR 0013's hidden expression carrier;
`ensureNodeEffects` materializes missing editable node instances from the project graph.
`submitGraph` accepts bounded schema-1 graph bytes as lowercase hex, reconciles native node
instances and values, then asks the main effect to validate and persist the graph. This
integration passes fake-host tests and builds against the May 2023 SDK, but has not yet
passed its AE 2023 host qualification gates below.

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
| `gateway_missing` | Neither the ScriptPath pass nor the self-loading path produced the gateway. | Check the installed copy for `jsx/starfield_gateway.jsx`, then reload the panel. |
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
   `CUSTOM_VALUE`, so the panel uses ADR 0013's ordinary expression carrier instead. Source
   integration is present, but the supervised callback and one-step undo/save-reopen behavior
   still need qualification with the matching AEX. P-02D's Emitter, Particle, and Force AEX
   modules currently build as pass-through prototypes; the graph actions are not yet wired to
   create/remove those AE effects or compile their values into the renderer. Click a node to open its floating
   properties window. The proposed carrier and its qualification gate are documented in
   [ADR 0013](../docs/adr/0013-script-visible-graph-snapshot.md). The checked
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
