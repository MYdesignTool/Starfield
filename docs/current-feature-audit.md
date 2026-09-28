# Current feature audit

Review baseline: working tree on 2026-09-28, including the preview-coordinate, transparent-output, and CEP startup-discovery fixes.

## What the implementation currently does

- The AE adapter uses SmartFX, reads input metadata for geometry, and converts host parameters into a host-independent CPU render request. It does not read or composite the input pixel world.
- The core evaluates a deterministic birth/lifetime schedule at an absolute time, supports seeded Point/Box/Sphere/Disc birth positions and per-particle velocity spread, integrates gravity and linear drag in closed form, and renders soft-edged 2D discs with per-particle color, size and opacity over transparent black.
- G-01/G-02/G-03 define the graph schema, bounded codec and validation, and CPU evaluation of the full single-emitter chain emitter -> force -> appearance -> output (stage order enforced, single emitter and single appearance stage). G-04 adds an AE arbitrary-data graph parameter, render-time snapshots and explicit legacy-control capture; every bound control is supervised so a Node Graph edit rewrites the canonical graph in the same user-change transaction.
- P-02 adds a dockable CEP panel (`cep_panel/`, ADR 0009 protocol v1) that edits supervised AE parameters. It renders the fixed chain as four bordered stage cards (`Emitter`, `Force`, `Appearance`, `Output`) stacked vertically, separated by a text connector (`↓ particles`), with one numeric field per bound parameter — no ports, no drawn edges, no pan or zoom. The owner's 2026-09-28 verdict on that surface is "the node graph is not implemented yet", so the fixed-topology canvas stays open as P-02A; dynamic graph editing and further node kernels are also not implemented.

## Why the current result feels far from Stardust

The M2 target was a render vertical slice, not a feature-parity release. M3-01 added emitter distributions and M3-02 added gravity, drag, color and the size/opacity age curves, so a trail can now fall, slow down, warm toward its end color, and shrink with age. The picture is still a flat 2D disc — Z does not affect projection, depth, or occlusion — and there are no textures, layer sources, particles-from-layers, meshes, materials, lights, volumes, motion blur, or file-based presets. The graph chain is fixed: nodes cannot be created, deleted, reordered, or rewired, and one force plus one appearance stage is the whole topology. Its 21 render controls are grouped in four AE topics; the current CEP surface is a parameter form with no visible graph canvas. This explains the gap without treating a successful build as a parity result.

The output contract is particle-only RGBA over transparency (ADR 0005), following the owner's AE feedback. AE 2023.5.0 Build 52 visually confirmed the rebuilt binary renders particles over the transparency grid. The adapter clears the host output world before copying the renderer's sparse particle buffer, preventing any source pixels pre-seeded by AE from remaining visible.

## Verification gaps

- AE 2023.5.0 Build 52 Full/Half/Third/Quarter readouts confirm preview-scaled centre normalization. Alpha-only output and all bit depths were visually confirmed on candidate `D22D43BAD15C5173867907369B2EF3293A3FD601C308158665A1F3FD0AB0816B`, and again on the H-01 split pair. The split build also has save/reopen, effect copy/undo, lower-layer compositing, shape distinctions, gravity and size-change observations. Exact graph-byte persistence remains open.
- The CEP parameter form populated automatically after the project and target effect layer were selected; no manual Refresh was used. If no target is selected, the panel retries transient discovery errors with a delay capped at 5 seconds until it finds one.
- The installed pair is `StarfieldParticle.aex` `B7362B01AC0E935D8AD596A70D61690DA4D586EEEC3E939328BA1BDC420069D5` plus a pinned `StarfieldCore.dll` `A4F104B5858DE5938F87B93D4B59FF89A5E324CD238DFDB3AD67B31327CD2545`; the development junction selects `StarfieldCore-095219764514FFCA.dll` (`095219764514FFCA1A5C3CFD36D78E6C8368ECF32C6C17576A8C26F2FA584564`). Core (6,196) and adapter (395) checks pass; panel gateway/startup fake-host checks pass. All of this was re-verified against the working tree on 2026-09-28.
- G-01 through G-04 have core/native code and fake-host coverage. AE Controls save/reopen, effect copy and undo/redo have host evidence; Node Graph-mode persistence and build-1 migration remain unqualified. Capturing controls stores current-time constants; it does not transform historical animation into node tracks.
- The CEP panel protocol is implemented and now has host-independent fake-host checks for animation protection and rollback. Name-based parameter lookup, scripted writes to supervised streams, undo grouping, and the Node Graph rewrite path still need AE 2023 qualification recorded in `cep_panel/README.md` and `docs/compatibility-matrix.md`.
- The three delivery examples are documented recipes and panel presets; none has been rendered in the host yet.

## Next product steps

1. Finish the H-01 host gates on the installed split pair: switch generations while an AE render is actually in flight, compare exact frames against the previous monolithic build, then exercise repeated switches, reverse-time requests, render queue and cancellation. The `C0830F…` "no main window" build is obsolete — the split pair is installed and loaded.
2. Continue CEP qualification for name lookup, one-undo-group edits, stale-state rejection, and the Node Graph rewrite path (`cep_panel/README.md`). Automatic discovery has already passed in AE without a Refresh click.
3. Record the three delivery examples from `docs/examples.md` as still frames at t ≥ 1 s.
4. Grow the graph beyond a fixed chain: dynamic node creation, deletion, and rewiring as protocol v2, plus the next node kernels (spawn/aux particles, textures, depth) each tied to observed reference cases and rendered fixtures.
