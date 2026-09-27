# ADR 0011: host changes require explicit, per-action authorization

- Status: accepted (owner instruction, 2026-09-27).
- Applies to every agent working on this repository.

## Rule

Nothing outside this repository is modified without the owner's **explicit authorization for that
specific action**. Listing the commands and waiting for approval is the minimum; "it is probably
safe", "it is reversible" or "the user asked for a working panel" are not authorization.

Host-level changes are exactly the cases that caused harm on 2026-09-27:

- Windows registry keys (for example `HKCU\Software\Adobe\CSXS.*\PlayerDebugMode`)
- Adobe per-user folders (for example `%APPDATA%\Adobe\CEP`) and caches
- the After Effects application and plug-in folders (adding, moving or removing `.aex` files)
- environment variables that host processes read
- starting or stopping processes (for example stray `CEPHtmlEngine.exe`)
- anything under `Program Files`, `Common Files\Adobe`, or the user's profile

## Required procedure for any host change

1. **List it before doing it**: the exact commands, the paths, and what the change depends on.
2. **Get approval** for that list. A general "go ahead" from an earlier turn does not carry over.
3. **Prefer reversible**: rename instead of delete, keep a copy in `artifacts/disabled/`, record the
   original path.
4. **Record the before and after state** in the same message that reports the change.
5. **Hand back a one-step undo** (for example `tools/cleanup_host_traces.ps1`).
6. If a change has host-wide reach, say so explicitly in the request: CEP's `CEFCommandLine`
   manifest block and `PlayerDebugMode` both affect other extensions, not just ours.

## What went wrong, recorded for the record

A CEP panel experiment wrote a host-wide registry key, a manifest `CEFCommandLine` block, and a
per-user extension link, then a cleanup removed that key. The owner's other extension panels stopped
loading, and because the key is host state rather than a cache, restarting After Effects and the
machine did not help. The CEP log `%TEMP%\CEP11-AEFT.log` showed the cause:

```
ERROR Signature verification failed for extension com.mtmograph.motion-next
ERROR Signature verification failed for extension com.local.aetoolkit.panel
ERROR Signature verification failed for extension Atom
```

Unsigned extensions load only while `PlayerDebugMode=1` is set, so removing it silently disabled every
unsigned panel the owner used. The recovery was to restore the key and the renamed folder.

## Consequence for this project's work

Panel and host-integration work stays **frozen** until the owner asks for it. Plugin work inside the
repository (core, adapter, schema, docs, tests) continues normally and needs no host authorization,
because it changes no host state. When host qualification is needed, the agent writes the checklist and
lets the owner run it.
