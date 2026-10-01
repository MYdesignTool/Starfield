# ADR 0011: host changes require explicit, per-action authorization

- Status: accepted (owner instruction, 2026-09-27; bounded deployment authorization added 2026-10-02).
- Applies to every agent working on this repository.

## Standing Starfield deployment authorization — 2026-10-02

The owner explicitly instructed: "部署吧，之后如果你看到ae没有在运行都可以直接部署，不用再问我".
This authorizes routine Starfield development deployments whenever a read-only check immediately
before deployment finds neither `AfterFX` nor `AfterFX_64` running. No repeated approval request
is required for this case. The deployment script also refuses installation while AE is running.

Use `tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName '<unique-backup-name>' -Install`
through the existing single `Plug-ins\Starfield` Junction to this checkout's `dist/`.
Keep the replaced files and deployment manifest under `artifacts/disabled/`, verify installed
hashes, record the before/after state, and provide the same command with `-Rollback` as the
one-step undo (AE closed). Prefer Core hot updates when an AEX replacement is unnecessary.

This authorization covers the Starfield development bundle. Starting/stopping processes,
registry or host-wide switches, Adobe caches, per-user folders, CEP installation, other
extensions, and unrelated host files continue to require their own explicit authorization.

## Rule

Nothing outside this repository is modified without the owner's **explicit authorization for that
specific action**. Listing the commands and waiting for approval is the minimum; "it is probably
safe", "it is reversible" or "the user asked for a working panel" are not authorization.
The standing Starfield deployment authorization above is the owner's explicit exception.

Host-level changes are exactly the cases that caused harm on 2026-09-27:

- Windows registry keys (for example `HKCU\Software\Adobe\CSXS.*\PlayerDebugMode`)
- Adobe per-user folders (for example `%APPDATA%\Adobe\CEP`) and caches
- the After Effects application and plug-in folders (adding, moving or removing `.aex` files)
- environment variables that host processes read
- starting or stopping processes (for example stray `CEPHtmlEngine.exe`)
- anything under `Program Files`, `Common Files\Adobe`, or the user's profile

## Required procedure for any host change

1. **List it before doing it**: the exact commands, the paths, and what the change depends on.
2. **Get approval** for that list, except deployments covered by the standing authorization above.
   A general "go ahead" from an earlier turn does not authorize other host changes.
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

## Rule for the repository's own scripts

Owner instruction, 2026-09-28: *"写命令可以，但绝对不可以在命令里面改什么奇怪的注册表."*

- **No script in this repository writes the registry**, in any direction, for any reason. Reading a
  value for a report is allowed and must be labelled read-only.
- The scripts that used to do it were fixed: `cep_panel/Install.ps1` no longer sets or clears
  `PlayerDebugMode` (the switch that did it is gone), and `tools/cleanup_host_traces.ps1` is a
  report by default whose every action needs an explicit switch.
- Host-wide switches in general are the owner's to set by hand. A script that sets one for them is how
  the owner's other panels stopped loading in the first place.
- Scripts here default to reporting. When they do act, they take one explicit switch per action, they
  name the exact file they touch, and they keep what they replace (`artifacts/disabled/`) rather than
  deleting it.

## Consequence for this project's work

Panel and host-integration work stays **frozen** until the owner asks for it. Plugin work inside the
repository (core, adapter, schema, docs, tests) continues normally and needs no host authorization,
because it changes no host state. When host qualification is needed, the agent writes the checklist and
lets the owner run it. Installing or updating the plug-in and the panel is authorized per action, with
the exact commands listed first, except Starfield deployments covered by the standing authorization
above; the install scripts resolve their own paths and print what they replaced.
