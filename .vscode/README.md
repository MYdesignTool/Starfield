# Local editor configuration

Open `tools/Starfield.code-workspace` so `${workspaceFolder}` is this checkout,
not its parent reference workspace. C/C++ configurations explicitly reference
the locally installed MSVC compiler; update `env.starfieldCompiler` if that
toolchain is replaced. Adobe May2023 and the pinned WebView2 SDK must exist at
the same paths used by `ae_plugin/BuildWindows.ps1`.

Use **C/C++: Select a Configuration** for the file being edited:

- `WebView2 Host`: resident AEGP/WebView2 files and `WebPanelHost.hpp`.
- `OpenCL Kernel`: optional standalone kernel configuration. Every configuration,
  including the default `Win32`, now forces the editor-only OpenCL declarations
  so `ae_plugin/gpu/SpriteKernel.h` resolves `get_global_id` without switching.
  This header has no built-in implementation and is never included by runtime
  GPU builds.
- Existing `Win32`/`Particle`/`Transform`/`Force`: corresponding native effects.

The CPU kernel test supplies its own per-thread implementation in its `shader`
namespace; it coexists with the global editor declaration. A C/C++ IntelliSense configuration
does not change MSBuild, NVRTC or AE. Reload the window after configuration
changes if diagnostics are stale. The actual build remains the authority for
compiler errors; editor checks cannot qualify AE behavior.
