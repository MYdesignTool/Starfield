# ADR 0012: versioned runtime core for AE 2023 development

- Status: accepted; AE 2023 smoke qualification passed, in-flight render gate open.
- Date: 2026-09-28.

## Decision

P-02G/build 13 upgrades the pre-release Core ABI from 1 to 2 to carry numeric
camera geometry (ADR 0020). Old and new modules reject mismatched ABI versions.
Deploy the adapter, internal node modules and Core together once with AE closed;
subsequent ABI-2 Core-only iterations retain the hot generation mechanism.
No released project data migration or cross-ABI shim is introduced.

`StarfieldParticle.aex` remains the stable AE SDK adapter: PiPL, selectors, parameter
registration, arbitrary-data persistence, host checkouts and pixel-world copying
stay there. Particle evaluation, simulation and CPU rasterization move to a
separately built `StarfieldCore.dll`. The adapter calls the core through a
versioned C ABI containing fixed-width scalars, byte spans and opaque handles;
no AE SDK type, C++ object, exception or allocator ownership crosses it.
Both Windows modules statically link their own MSVC runtime. AE 2023.5 bundles
MSVCP140.dll 14.00.24210.0, while the current v145 toolset uses a newer STL;
the first host test of the /MD adapter crashed while locking its loader mutex,
before the core DLL loaded. /MT isolates the toolset's STL and CRT from AE's
app-local runtime. Keep allocation and release inside the same module, including
the opaque C ABI result handle; never transfer a CRT-owned pointer across the
boundary. Dependency inspection and the AE 2023 host smoke test passed after
this change; see `docs/compatibility-matrix.md` for the remaining gates.

Development builds publish uniquely named core DLLs and atomically update
`dist/StarfieldRuntime/current.txt`. Owner direction on 2026-10-01 replaces the
loose-file installation with one junction, `Plug-ins/Starfield -> dist`.
`StarfieldRuntime` is an ordinary directory inside the bundle, not another link.
The old root runtime junction and loose binaries are archived during deployment.
Core-only publication never replaces an AEX; full AEX publication requires AE closed.
Candidate builds use `-NoRuntimePublish -NoDistPublish` and deployment is explicit.
The Options command manually loads the selected core.
The manifest is one ASCII basename no longer than 100 bytes matching
`StarfieldCore-*.dll`, optionally followed by one line ending; any additional
line is rejected. Path separators and parent traversal are rejected. If
the manifest is absent, the loader tries a pinned `StarfieldCore.dll` next to
the AEX for a self-contained release. A malformed manifest is an error and
never silently selects another DLL.
The installed effect must continue using its previous generation if the new
file is missing, incompatible or fails to load. Every pre-render pins a core
generation through its matching render; a retired generation unloads only after
the last pin is released. The loader never overwrites a mapped DLL.

The selected generation identity is mixed into the SmartFX cache GUID during
pre-render. A successful manual reload requests a new render through the AE
2023 supported `PF_OutFlag_FORCE_RERENDER` path. Full and Quarter previews
updated after manual reload in AE 2023.5.0 Build 52 without a process restart.
The identity hashes the DLL path and contents once at load, so a fixed-name
release DLL also invalidates old cache entries after its contents change.

`BuildWindows.ps1 -CoreOnly` records no adapter output. It compares the full
build's SHA-256 fingerprint of AEX source and ABI inputs before building; if
any differ, it refuses the core-only path and requires a matching AEX rebuild
and installation. The full build publishes both binaries and matching PDBs.

The release bundle pins `StarfieldCore.dll` next to the `.aex` and does not
depend on a developer workspace. The match name, existing AE parameter IDs,
packed plug-in version and graph schema are unchanged by this split. A core
ABI mismatch reports a render error without destroying AE project data.

## Migration and rollback

H-01 changes the build and runtime transport but not serialized data. Keep the
last monolithic `.aex` as a rollback artifact until AE 2023 verifies save/reopen,
Full/Quarter, all bit depths, cancellation, repeated reload and bad-DLL
fallback. Host installation requires a separate ADR 0011 authorization with
exact paths, commands and one-step undo.
