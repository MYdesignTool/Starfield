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

This extension must never be able to affect anyone else's panel. Two things it touches are
host-wide, so both are handled carefully:

- The manifest carries **no `CEFCommandLine` block**: CEP appends those switches to the shared
  CEF command line, which changes how *every* CEP panel in the host starts.
- `Install.ps1` writes the per-user `PlayerDebugMode` key, which is what lets an unsigned
  extension load at all. `Install.ps1 -Uninstall` removes it again.

To get a clean host back, in this order:

1. Quit After Effects completely.
2. `powershell -ExecutionPolicy Bypass -File cep_panel\Install.ps1 -Uninstall`
   (removes the extension link and clears `PlayerDebugMode` for CSXS.11 and CSXS.12).
3. Start After Effects and check that the other extension panels open again.
4. If they do, reinstall **one step at a time** to find what the host dislikes:
   `Install.ps1 -RegistryOnly` (registers the debug key, installs nothing), restart, test;
   then `Install.ps1 -PanelOnly` (links the extension, touches no registry), restart, test.
   Whichever step breaks the other panels is the one to report.
5. Keep `%TEMP%\cep_cache\` and `%LOCALAPPDATA%\Temp\Adobe\CEP*` logs from the failing run:
   CEP records why an extension failed to initialise.

## Install for development

1. Close the previous panel in After Effects (if open).
2. Enable unsigned extensions (once):

   ```
   reg add "HKCU\Software\Adobe\CSXS.11" /v PlayerDebugMode /t REG_SZ /d 1 /f
   ```

   AE 2023 ships CEP 11; if the panel does not appear, repeat the key for `CSXS.12`.

3. Link or copy this folder into the user CEP extensions directory:

   ```
   %APPDATA%\Adobe\CEP\extensions\org.starfieldfx.panel
   ```

   The directory name must equal the bundle id. A directory junction avoids copying:

   ```
   mklink /J "%APPDATA%\Adobe\CEP\extensions\org.starfieldfx.panel" "<repo>\cep_panel"
   ```

4. Restart After Effects. Open the panel from **Window > Extensions > Starfield Node
   Editor**.

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
