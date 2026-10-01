# Starfield Core

独立实现一套面向现代 After Effects 的粒子效果。当前阶段先建立稳定的核心边界和数据契约；旧插件样本与逆向分析材料保留在父目录，不进入本工程构建。

## 当前进度

- M0/M1 contracts and the native shell are in place. M2 adds SmartFX transport, an 8/16/32-bpc CPU sprite renderer, deterministic time evaluation, and transparent particle output that can composite over lower layers.
- M3-01 adds seeded Point/Box/Sphere/Disc birth distributions, emitter position, per-particle velocity spread, and deterministic random streams.
- M3-02 completes the current Alpha chain `发射器 → 力场 → 外观 → 输出`: closed-form gravity/linear-drag integration and linear age curves for size, opacity and color. Gravity X/Y/Z (17-19), Linear Drag (20), Color Start/End (21/22), Size End (23) and Opacity End (24) are registered supervised controls whose defaults reproduce the previous look.
- G-01 through G-04 provide typed graph validation/serialization/evaluation, the four-stage chain, and an AE arbitrary-data parameter path with a supervised edit surface (Node Graph edits rewrite the canonical graph in the same user-change transaction).
- P-02 adds the dockable CEP panel in `cep_panel/` (ADR 0009 protocol v1); Spark/Snow/Floating Light are documented in [交付示例](docs/examples.md) and shipped as panel presets. The panel is a grouped parameter form; the visible node canvas is still open as P-02A.
- H-01 (ADR 0012) splits the AE adapter from a reloadable `StarfieldCore.dll` behind a versioned C ABI, with content-addressed generations and an explicit Options reload. AE 2023.5.0 Build 52 confirmed Full/Quarter hot reload without restarting AE, missing-DLL fallback, visual 8/16/32-bpc output, transparency, and save/close/reopen. In-flight render switching and exact pixel comparison against the previous monolithic build remain open.
- Core self-tests pass (6,196 checks), the adapter fake-host suite passes (395 checks), and the panel gateway/startup fake-host regressions pass; the Windows x64 plug-in builds with the May 2023 SDK. Current work targets AE 2023 only. Host evidence covers controls, lifecycle, save/reopen, copy/paste, undo/redo, 8/16/32 bpc, Full/Half/Third/Quarter preview and time consistency; open gates include graph-byte persistence, render queue, cancellation, and in-flight core switching. Evidence is tracked in [行为清单](docs/compatibility-matrix.md).
- This is no longer a flat white sprite slice, but it is not Stardust parity: the output is still 2D discs (Z does not affect projection), the chain topology is fixed, and texture sources, motion blur, mesh/volume rendering, and file-based presets remain unfinished. See the [current feature audit](docs/current-feature-audit.md).

## 当前架构

- 纯 C++ 核心不依赖 AE SDK；宿主指针和 suite 只出现在 `ae_plugin/` 适配层。
- 渲染以不可变参数快照和绝对时间求值为基础，便于乱序帧请求与后续 MFR。
- `Settings` 验证器在进入渲染前限制资源上界并替换非有限值。
- 图模型以独立 UUID 和强类型端口/参数 key 表示；AE 任意数据参数路径已接入，SmartFX 预渲染创建不可变节点快照。
- SmartFX、序列化、Compute Cache、CPU 参考渲染器和 GPU 后端的边界见 [架构说明](docs/architecture.md)。
- 行为覆盖按独立验收场景推进，见 [行为清单](docs/compatibility-matrix.md)。
- 当前构建和验收仅针对 AE 2023，暂不进行新版本适配；分阶段交付见 [开发路线图](docs/roadmap.md)。
- AE 2023.5.0 Build 52 已加载 H-01 拆分构建：点击 `Options` 手动重载核心后 Full/Quarter 预览更新、缺失核心时保留上一代、8/16/32-bpc 与透明输出、存盘/关闭/重开均已通过；CEP 面板在选中工程与图层后自动出结果，无需 Refresh。低分辨率发射器原点偏移已按预览缩放修复并有宿主读数。仍待办：AE 渲染进行中切换核心、与旧单体构建的逐像素对比、Node Graph 持久化、渲染队列与取消。功能证据与 agent 任务拆分见[参考功能目录](docs/reference-inventory.md)和[任务清单](docs/agent-backlog.md)。

## 面板与交付示例

- 可停靠 CEP 节点面板：`cep_panel/`（AE 2023）提供节点画布、属性窗口、连线与多选操作。节点增删与独立效果同步仍在宿主验收中；当前阻塞为选中效果图层时崩溃。协议与安装步骤见 [面板说明](cep_panel/README.md)。前端源码通过现有 CEP Junction 加载。
- 三个交付示例（火花 / 飘雪 / 漂浮光点）：精确参数配方见 [交付示例](docs/examples.md)，面板的 `Example` 下拉可直接套用；不装面板也可以照表手填。
- 宿主验收清单（含面板与示例）在 [行为清单](docs/compatibility-matrix.md) 的 M2-06 小节。

## 构建

用 CMake 构建不依赖 AE 的核心库。Windows AE 插件使用锁定的 MSVC v145 和本地 May 2023 SDK；从仓库根目录运行 `powershell -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1`，默认产物写入 `artifacts/plugin/2023/x64/Release/`。显式传入 `-SdkPath 'AdobeSDK\May2023_AfterEffectsSDK' -ArtifactLabel 2023` 得到同一目标。详细工具链和构建状态见 [构建矩阵](docs/build-matrix.md)。

开发安装使用一个文件夹链接，完整构建发布五个 AEX 和配套 Core；热更新只发布版本化 Core：

```
AE Plug-ins\Starfield -> 项目\dist
dist\StarfieldRuntime\current.txt -> 同目录版本化 Core DLL
powershell -ExecutionPolicy Bypass -File tools\Deploy-TestBuild.ps1 -PluginDir <AE Plug-ins> # 默认只读
powershell -ExecutionPolicy Bypass -File tools\Deploy-TestBuild.ps1 -Install -PluginDir <AE Plug-ins> -BackupName <本次备份名>
powershell -ExecutionPolicy Bypass -File tools\Deploy-TestBuild.ps1 -Rollback -PluginDir <AE Plug-ins> -BackupName <本次备份名>
```

`-CoreOnly` 只更新 `dist\StarfieldRuntime\` 的版本化 DLL 与原子选择文件，不改 AEX。完整构建在 AE 运行时禁止发布 AEX；只构建候选时使用 `-NoRuntimePublish -NoDistPublish`。部署会归档插件根目录中旧的 Starfield 文件和旧运行时链接，保留其他插件。备份位于 `artifacts\disabled\`，替换 AEX 与回滚前必须关闭 AE。旧单体及散文件安装脚本已停用。

## 许可

本项目采用 MIT 许可，全文见 [LICENSE](LICENSE)。

Adobe After Effects SDK 不随本仓库分发，仅作为本地构建输入使用；仓库中不包含 SDK 头文件、示例源码、PiPL 工具或任何生成产物。引入第三方依赖前必须记录其版本、许可与对 AE 进程内稳定性的影响（见 [CONTRIBUTING.md](CONTRIBUTING.md)）。
