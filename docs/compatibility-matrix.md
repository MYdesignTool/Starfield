# Behavior inventory

Use this file to turn observed behavior into requirements before implementing each feature. Do not infer undocumented internal algorithms from binary details. Record a repeatable user-visible scenario, expected result, reference version, and status for each row.

## Host qualification checkpoint

| Check | Host | Status | Evidence / next step |
|---|---|---|---|
| M1 plug-in discovery and load | AE 2023, exact build not recorded | User-confirmed pass | Effect loads; verify render pass-through, add/remove, save/reopen, duplicate, and undo/redo |
| M1 discovery and load | Current AE 26.x | Not checked | Record exact host build before claiming support |

| Area | Independent acceptance case | Status |
|---|---|---|
| Emitter | Point/box/sphere/disc emitters produce deterministic positions from seed and absolute time | Not started |
| Particle lifecycle | Birth rate, lifetime, age, and particle cap behave consistently at arbitrary frame order | Not started |
| Forces | Each force has isolated enable/disable and stable parameter semantics | Not started |
| Nodes | Graph connections validate cycles, missing inputs, and invalid references without crashing | Not started |
| Rendering | Alpha, premultiplication, color depth, rowbytes, ROI, and downsample are explicit | Not started |
| Persistence | Save/reopen and schema migration preserve settings and graph identity | Not started |
| Concurrency | Repeated concurrent renders return identical pixels and never mutate shared state | Not started |
| Presets | Import/export validates version and rejects malformed or oversized data | Not started |
| Panel | UI state synchronizes through a versioned protocol and tolerates disconnect/restart | Not started |

Suggested columns when a case is ready: `Case ID`, `Reference behavior`, `Input project/settings`, `Expected output`, `AE version`, `Evidence`, `Implementation`, `Status`.
