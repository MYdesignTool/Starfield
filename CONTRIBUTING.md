# Contributing

This repository uses Git for staged development. Keep each independent task on its own branch or worktree, and use the task ID from `docs/agent-backlog.md` in the branch and commit subject.

## Before editing

1. Read `AGENTS.md` and the assigned task card.
2. Check dependencies and owned files in the backlog; coordinate instead of editing another task's files.
3. Treat reverse-engineering notes as evidence about behavior, not as source code or a design to copy line by line.

## Contracts

- Preserve stable parameter IDs, the effect match name, and the current version contract.
- Keep host-specific types in `ae_plugin/`; keep the core independent of the AE SDK.
- Changes to time, pixel, alpha, sequence, threading, or version semantics must update the matching ADR and task acceptance criteria.
- Do not add a runtime dependency without documenting why it is needed, which exact version is selected, its license metadata, and its effect on AE process-wide stability.

## Build and host evidence

- Build the AE module with `ae_plugin/BuildWindows.ps1`; local SDK files and all generated files are excluded by `.gitignore`.
- CMake builds only the host-independent core.
- Record host load/render/lifecycle evidence separately from compile results. AE 2023 confirmed load, controls, and rendering on an earlier M2 parameter revision; the current 13-control M3-01 build and lifecycle still need host qualification.

## Review

Every change should name its task ID, summarize the contract affected, and list build or host evidence actually collected. Do not mark a roadmap gate complete based only on code being present.

## License

This project is released under the MIT license; the full text is in `LICENSE`. Contributions are accepted under the same terms (inbound = outbound), so do not contribute code you cannot license that way.

The Adobe After Effects SDK is a local build input and is not redistributed here. Do not commit SDK headers, sample sources, PiPL binaries, plug-in binaries, or generated build artifacts. Any new runtime dependency must still record its exact version and license as required by the contracts section above.
