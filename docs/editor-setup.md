# 编辑器配置

打开 tools/Starfield.code-workspace 或直接打开 newStardust；仅打开父 forensic 工作区不会应用本工程的 C++ 配置。
项目配置使用 C++20、MSVC x64、当前 Core 和本地 May2023 SDK。原生 variant 定义必须与正在查看的 Emitter/Particle/Force/Transform 代码一致；Appearance 已退役。PluginApi.h 是当前 ABI 的唯一声明，不从旧检查点复制 ABI2。

IntelliSense/browse 缓存位于忽略的 artifacts/editor，不扫描父目录旧源码或 deployed binaries。tools/Clean-Workspace.ps1 可报告并清掉此工作区的再生成缓存，不改用户 profile 缓存/全局编辑器设置。
诊断持续时由 owner 执行 C/C++: Reset IntelliSense Database，再 Developer: Reload Window。用 Go to Definition 确认 SfCoreRenderRequest 指向本 checkout 的 PluginApi.h。

实际编译/include provenance 的入口：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1 -NativeSync -TraceIncludes
```

测试响应文件使用仓库 cwd 下的相对路径，避免非 ASCII 路径进入 CL 参数；不再通过 subst 映射盘符。原生 PiPL 构建仍由 BuildWindows.ps1 管理。编辑器提示和编译日志分别判断，历史 editor 修复不是当前 ABI 验收。
