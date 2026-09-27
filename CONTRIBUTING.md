# Contributing

This repository is being prepared for Git-based parallel work. Keep each agent assignment on its own branch or worktree once Git is initialized, and use the task ID from `docs/agent-backlog.md` in the branch and commit subject.

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
- Record host load/render/lifecycle evidence separately from compile results. The confirmed host result so far is that the M1 shell loads in AE 2023; the exact AE build and lifecycle behavior are still to be recorded.

## Review

Every change should name its task ID, summarize the contract affected, and list build or host evidence actually collected. Do not mark a roadmap gate complete based only on code being present.
