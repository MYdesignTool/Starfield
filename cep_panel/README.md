# Starfield node editor panel (CEP)

Dockable After Effects 2023 panel for the current Alpha chain
`emitter -> force -> appearance -> output`. It implements protocol v1 of
[ADR 0009](../docs/adr/0009-cep-panel-bridge.md): the panel renders the fixed chain,
reads and writes the effect's **supervised ordinary parameters** through a namespaced
ExtendScript gateway, and never touches `Node Graph Data` (the arbitrary-data
parameter) or any host-private state.

## Files

| Path | Role |
|---|---|
| `CSXS/manifest.xml` | CEP 11 manifest; host `AEFT [23.0, 99.9]`; panel entry `index.html` |
| `index.html`, `css/panel.css`, `js/panel.js` | Panel UI and protocol client |
| `jsx/starfield_gateway.jsx` | `SFLD_getState` / `SFLD_setParameters` gateway (public AE scripting DOM only) |

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

   AE 2023 ships CEP 11, so that is the key that matters; check `CSXS.12` too if the panel does not
   appear. On this machine it is already `1` and other panels rely on it staying that way. `Install.ps1`
   prints the value it finds, read-only, and leaves it alone.

3. Put this folder where CEP scans for extensions. Both roots work; the owner's current install is the
   system-wide one, as a plain copy:

   | Root | Path | Needs admin |
   |---|---|---|
   | user | `%APPDATA%\Adobe\CEP\extensions\<name>` | no |
   | system | `C:\Program Files (x86)\Common Files\Adobe\CEP\extensions\<name>` | yes |

   The folder name is the extension's identity in the menu; `cep_panel` and
   `org.starfieldfx.panel` (the bundle id) both work. Two copies under two names would appear twice.

4. Restart After Effects. Open the panel from **Window > Extensions**, entry **Starfield Node
   Editor**.

### Updating an installed copy

A copy does not follow the repository, and the panel is plain HTML/JS/JSX, so an update is a file copy
and a panel reload — After Effects itself does not have to restart. The panel evaluates
`jsx/starfield_gateway.jsx` by itself on the first host call, so the JSX does not need the app-start
ScriptPath pass either.

```
robocopy "<repo>\cep_panel" "C:\Program Files (x86)\Common Files\Adobe\CEP\extensions\cep_panel" /MIR
```

Then close and reopen the panel window (`Window > Extensions > Starfield Node Editor`). If the JSX ever
looks stale, the panel reload is enough; a full restart is only needed when the manifest itself changed.

A junction avoids the copy step entirely when the repository can be the source of truth:

```
rmdir "C:\Program Files (x86)\Common Files\Adobe\CEP\extensions\cep_panel"
mklink /J "C:\Program Files (x86)\Common Files\Adobe\CEP\extensions\cep_panel" "<repo>\cep_panel"
```

## Troubleshooting

| Panel shows | Cause | What to do |
|---|---|---|
| `bad_response: … unreadable data: EvalScript error.` | The gateway threw before it could answer. The first host run hit this because the entry points were private to the file's IIFE, so `SFLD_getState(...)` was a `ReferenceError`. | Fixed: the gateway publishes `SFLD_getState`/`SFLD_setParameters`/`SFLD_ready` on the ExtendScript global object and the panel loads it by path if the host has not. Update the installed copy (above) and reload the panel. |
| `bad_response: … unreadable data: undefined` | The gateway is not loaded in this session. | Same as above; the panel's self-loading path covers it. If it persists, confirm `jsx/starfield_gateway.jsx` exists in the installed copy. |
| `gateway_missing` | Neither the ScriptPath pass nor the self-loading path produced the gateway. | Check the installed copy for `jsx/starfield_gateway.jsx`, then reload the panel. |
| `no_target` | No selected layer carries the effect. | Select exactly one layer carrying Starfield Particle and press **Refresh**. |
| `missing_parameter` | The installed `.aex` build and the panel's binding table disagree (a parameter was renamed or removed). | Rebuild/install the current `.aex`; the bindings list the names the gateway resolves. |

## Use

1. Put `StarfieldParticle.aex` in the AE plug-ins folder (see the repository
   `README.md`) and apply the effect to a layer.
2. Select exactly one layer that carries the effect, then press **Refresh** in the
   panel. The panel shows the emitter, force, appearance, and output stages with their
   parameters and connections.
3. Edit a value: the panel validates it, writes it through the gateway in one undo
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

Panel -> gateway requests are JSON strings:

```json
{ "protocol": "org.starfieldfx.panel", "version": 1, "requestId": "r1",
  "operation": "getState | setParameters", "target": {}, "baseRevision": "<token>",
  "changes": [{ "key": "gravity_y", "value": -2.6 }] }
```

The gateway rejects the whole set on an unsupported version, an ambiguous or missing
target, an unknown binding, an out-of-range or non-finite value, more than 32 changes,
a payload over 64 KiB, a stale `baseRevision`, or an animated parameter. Error codes:
`no_project`, `no_active_comp`, `no_target`, `ambiguous_target`, `no_effect`,
`missing_parameter`, `unknown_binding`, `invalid_value`, `stale_state`,
`animated_parameter`, `host_write_failed`, `host_error`, `invalid_request`,
`unknown_operation`, `no_host`, `bad_response`, `host_timeout`.

## Qualification status (not yet verified on a host)

The panel is **code complete and unqualified**. Before it can be called working, an
AE 2023 pass must record:

1. AE build number, OS, and the `.aex` build; the panel appears under
   **Window > Extensions**.
2. Effect parameters resolve **by name** (the footer shows `Lookup: name`; `index`
   means the fallback ran).
3. Hidden/standard parameter streams can be read and written by the gateway.
4. A panel edit updates the composition and one undo/redo restores and reapplies it.
5. A panel edit in `Node Graph` mode rewrites the stored graph (Options readout shows
   the changed value).
6. Save/reopen and effect duplication preserve the values; a stale `baseRevision`
   edit is rejected with `stale_state`.

Record results in [docs/compatibility-matrix.md](../docs/compatibility-matrix.md).
Until that is done, treat the panel as a preview, not a supported feature.
