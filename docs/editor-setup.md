# C++ editor configuration

Open `tools/Starfield.code-workspace` in VS Code, or open `newStardust` directly
as the folder. Opening only the parent forensic workspace will not apply the
child project's `.vscode/c_cpp_properties.json`.

The tracked project configuration uses C++20, MSVC x64, explicit current Core and
May 2023 SDK include paths and the target node definition. Win32 is the default
Emitter configuration; select Particle/Appearance/Force when inspecting that
variant of NodeEffects.cpp/NodeGraphSync.cpp. Compiler discovery remains automatic
so no machine-specific MSVC version is pinned in source. The actual native build
still uses BuildWindows.ps1. No SDK headers are included in Git.

The default browse database and IntelliSense cache live under ignored
`artifacts/editor`. Avoid recursive include search over parent forensic material,
generated files and deployed binaries. This only configures this workspace;
no global editor settings or profile caches are modified by repository scripts.

For the owner-reported C/C++(135) diagnostics in camera_capture_tests.cpp, the
current `include/starfield/core/PluginApi.h` defines Core ABI 2 and all reported
members: camera_enabled, layer_to_view and image_to_layer. NativeSync's actual
MSVC compile/test log is the compiler evidence. A stale editor cache/header
configuration remains a hypothesis until the owner's editor refresh confirms it.

Rechecked on 2026-10-02 with NativeSync -TraceIncludes: MSVC 14.51.36231 compiled
the actual camera_capture_tests.cpp and Camera.cpp, both including this checkout's
PluginApi.h through the temporary mapped drive. Native sync 14 and camera capture
12 checks pass, with zero failures. Log: artifacts/build13-editor-native-sync.log.
Project configuration JSON parses and git diff --check passes. No runtime source,
ABI layout or deployed AEX/Core changed in this editor-configuration repair.

If diagnostics persist after opening the workspace, run
`C/C++: Reset IntelliSense Database`, then `Developer: Reload Window` from the
command palette. Use Go to Definition on SfCoreRenderRequest to verify it resolves
to this checkout's `include/starfield/core/PluginApi.h`, not an older workspace.
These are owner-operated editor actions; the agent does not delete profile caches.

Minimal compiler verification with include provenance:

```powershell
powershell -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1 -NativeSync -TraceIncludes
```

The script maps the checkout temporarily to a free drive for MSVC. Include traces
must identify that mapped checkout's `include/starfield/core/PluginApi.h`.
Scratch output remains under artifacts; this does not build/deploy a new AEX.

Configuration semantics follow the primary
[Microsoft C++ configuration documentation](https://code.visualstudio.com/docs/cpp/customize-cpp-settings).
