# P-02D native node checkpoint — 2026-10-01

## Implemented source

- Separate Emitter, Particle, Appearance and Force effect instances own values,
  UUID, outgoing connections, percentage curve banks and signed canvas positions.
  The main effect owns Output, its cap/position and the compiled render snapshot.
- Graph edits use ordinary node streams and numeric compile trigger 43. No request
  expression or parameter 90 remains. Snapshot 41 is a read-only expression mirror.
- Main guard 42 suppresses intermediate Output commits. Per-node guards suppress
  intermediate edits. Marker 89 distinguishes first creation from deleting all nodes.
- Failed transactions restore native effects, Output metadata and the compiled
  snapshot. Indexed-group references are reacquired after structural mutations.
- Semantic acknowledgement tolerates AE float/color quantization while checking
  node/edge identity, topology, values, curve knots and complete layout.
- Build-only child environments normalize duplicate Path/PATH names. Candidate
  builds can avoid publishing either dist or the runtime selector.

## Completed checks

The eight focused CEP suites passed: gateway, startup, native-node gateway,
graph transactions, codec, edits, view and layout. The final transaction/native-node
checks also passed after the snapshot exception fix. Adapter: **687 checks, zero
failures**. The full May 2023 SDK x64 Release build succeeded with five AEX modules
and the paired Core. Core algorithms were unchanged and the broader core suite was
not repeated. These results qualify source/build behavior, not AE host behavior.

## Actual AE evidence and failure

Target: **After Effects 2023.5.0 Build 52**, Windows x64.

The initial six-file revision-16 build-2 pair was deployed with hashes checked and
backup record `artifacts/disabled/p02d-native-records-20261001/deployment.json`.
CEP remains the existing junction to this checkout; no preferences, registry keys,
cache directories or additional junctions were modified.

Two CLI launches did not execute the JSX bridge. A File > Scripts > Run Script File
launch then wrote `script_started`, created a clean temporary composition and added
the main renderer. Its last breadcrumb was `renderer_added`; AE crashed while opening
the composition viewer, before gateway initialization and before node operations.
The owner also reported the crash. The test did not save the project.

The crash dump `c991e55b-7da9-4790-a054-7aa0f322affc` records access violation
`0xc0000005` in `AfterFXLib.dll+0x1931d36`, on main thread 29580. The exception stack
contains host/TDB frames and no direct Starfield return frame. Both the candidate
AEX and selected Core were loaded. This evidence does **not** isolate the cause or
exonerate the plug-in. The historical PowerShell dump parsers use incorrect packed
offset assumptions on this dump and cannot be relied upon for its exception fields.

Build 3 updates code and PiPL version together (`0x8003`) after the registered
parameter count changed, and clears group-end definitions rather than inheriting
slider flags/unions or an arbitrary-data handle. AE's log reports plug-in caching;
stale metadata is a hypothesis. These changes are defensive fixes, **not a confirmed
resolution** of the crash. No automatic repeated host runs are scheduled.

## Remaining acceptance

1. Establish safe fresh-effect apply/preview with the build-3 pair.
2. Bootstrap separate node effects; add/copy two Particle nodes with independent values.
3. Disconnect/reconnect/delete, including Output-only deletion, and rendered response.
4. Direct Effect Controls edits, deletion reconciliation and origin unit parity.
5. Undo/redo and save/reopen; menu-hidden add-by-match-name.

Automatic node creation without an open CEP panel is still open. Raw native Ctrl+D
duplicates retain a UUID and are detected as an identity conflict; automatic re-key
and import are not implemented. Multiple active emitters remain a core gate.
The current native record permits at most four outgoing connections per node.

## Deployment and undo

`tools/Deploy-TestBuild.ps1` reports by default. `-Install` archives prior paired
files and selector, copies candidates, verifies hashes and records one-step rollback.
It requires AE closed and the existing single runtime junction. It does not start
or stop processes and does not change system settings.

For the first deployed candidate, with AE closed:

```powershell
powershell -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p02d-native-records-20261001' -Rollback
```

Build 3 is now deployed with all six hashes verified. Its backup record is
`artifacts/disabled/p02d-build3-20261001/deployment.json`; current hashes and its
one-step rollback command are in the build matrix. AE was closed by the owner
before this deployment and has not been restarted by the agent. Create fresh effects for development parameter schemas;
older development projects are not migrated.
