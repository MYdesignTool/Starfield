# Current feature audit

Review baseline: `070c33e` plus G-04 host arbitrary parameter bridge, 2026-09-27.

## What the implementation currently does

- The AE adapter uses SmartFX and converts host parameters/worlds into a host-independent CPU render request.
- The core evaluates a deterministic birth/lifetime schedule at an absolute time, supports seeded Point/Box/Sphere/Disc birth positions and per-particle velocity spread, integrates gravity and linear drag in closed form, and renders soft-edged 2D discs with per-particle color, size and opacity over the optional input.
- G-01/G-02/G-03 define the graph schema, bounded codec and validation, and CPU evaluation of the full single-emitter chain emitter -> force -> appearance -> output (stage order enforced, single emitter and single appearance stage). G-04 adds an AE arbitrary-data graph parameter, render-time snapshots and explicit legacy-control capture; every bound control is supervised so a Node Graph edit rewrites the canonical graph in the same user-change transaction.
- P-02 adds the dockable CEP panel (`cep_panel/`, ADR 0009 protocol v1) that displays the fixed chain and edits the supervised parameters. Dynamic graph editing (create/delete/rewire nodes) and further node kernels are not implemented.

## Why the current result feels far from Stardust

The M2 target was a render vertical slice, not a feature-parity release. M3-01 added emitter distributions and M3-02 added gravity, drag, color and the size/opacity age curves, so a trail can now fall, slow down, warm toward its end color, and shrink with age. The picture is still a flat 2D disc — Z does not affect projection, depth, or occlusion — and there are no textures, layer sources, particles-from-layers, meshes, materials, lights, volumes, motion blur, or file-based presets. The chain is fixed: nodes cannot be created, deleted, reordered, or rewired, and one force plus one appearance stage is the whole topology. The controls remain one flat list (twenty-one of them) rather than an organized UI. This explains the gap without treating a successful build as a parity result.

The current compositing default (particles over the optional source) is an explicit project choice in ADR 0005, not yet confirmed to match the reference. Treat differences from the old effect as open behavior questions until a repeatable AE reference case records them.

## Verification gaps

- The 24-parameter build has not been loaded in AE 2023. Existing host evidence covers an earlier eight-control revision only; the exact AE build is also unrecorded. Save/reopen, effect copy, old-project default selection and undo/redo remain unverified in the host.
- AE playback, Full/Half/Quarter preview, 8/16/32-bpc, cancellation, and project lifecycle remain unqualified for the current build.
- The point-control normalization shim is still provisional until the current AE 2023 Options readout records the delivered origin values.
- G-01 through G-04 have core/native code and fake-host coverage. Host persistence and undo claims remain unqualified until exercised in AE 2023. Capturing controls stores current-time constants; it does not transform historical animation into node tracks.
- The CEP panel and its protocol are unqualified: name-based parameter lookup, scripted writes to the supervised streams, undo grouping, and the Node Graph rewrite path all need the AE 2023 host pass recorded in `cep_panel/README.md` and `docs/compatibility-matrix.md`.
- The three delivery examples are documented recipes and panel presets; none has been rendered in the host yet.

## Next product steps

1. Install the current effect in AE 2023; exercise fresh add, build-1 project load, graph selection, explicit capture, gravity/drag/color/curve edits, save/reopen, duplication and undo/redo.
2. Qualify the CEP panel: name lookup, one-undo-group edits, stale-state rejection, and the Node Graph rewrite path (`cep_panel/README.md`).
3. Record the three delivery examples from `docs/examples.md` as still frames at t ≥ 1 s.
4. Grow the graph beyond a fixed chain: dynamic node creation, deletion, and rewiring as protocol v2, plus the next node kernels (spawn/aux particles, textures, depth) each tied to observed reference cases and rendered fixtures.
