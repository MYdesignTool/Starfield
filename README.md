# Starfield Core

独立实现一套面向现代 After Effects 的粒子效果。当前阶段先建立稳定的核心边界和数据契约；旧插件样本与逆向分析材料保留在父目录，不进入本工程构建。

## 当前进度

- M0/M1 contracts and the native shell are in place. M2 adds SmartFX transport, an 8/16/32-bpc CPU sprite renderer, deterministic time evaluation, and source compositing.
- M3-01 adds seeded Point/Box/Sphere/Disc birth distributions, emitter position, per-particle velocity spread, and deterministic random streams.
- M3-02 completes the current Alpha chain `发射器 → 力场 → 外观 → 输出`: closed-form gravity/linear-drag integration and linear age curves for size, opacity and color. Gravity X/Y/Z (17-19), Linear Drag (20), Color Start/End (21/22), Size End (23) and Opacity End (24) are registered supervised controls whose defaults reproduce the previous look.
- G-01 through G-04 provide typed graph validation/serialization/evaluation, the four-stage chain, and an AE arbitrary-data parameter path with a supervised edit surface (Node Graph edits rewrite the canonical graph in the same user-change transaction).
- P-02 adds the dockable CEP node editor in `cep_panel/` (ADR 0009 protocol v1); Spark/Snow/Floating Light are documented in [交付示例](docs/examples.md) and shipped as panel presets.
- Core self-tests pass (6,184 assertions) and the adapter suite passes (287 assertions); the Windows x64 plug-in builds with the May 2023 SDK. Current work targets AE 2023 only. The 24-parameter build, persistence/undo and the panel still need host qualification; evidence is tracked in [行为清单](docs/compatibility-matrix.md).
- This is no longer a flat white sprite slice, but it is not Stardust parity: the output is still 2D discs (Z does not affect projection), the chain topology is fixed, and texture sources, motion blur, mesh/volume rendering, and file-based presets remain unfinished. AE 2023 host qualification remains open. See the [current feature audit](docs/current-feature-audit.md).

## 当前架构

- 纯 C++ 核心不依赖 AE SDK；宿主指针和 suite 只出现在 `ae_plugin/` 适配层。
- 渲染以不可变参数快照和绝对时间求值为基础，便于乱序帧请求与后续 MFR。
- `Settings` 验证器在进入渲染前限制资源上界并替换非有限值。
- 图模型以独立 UUID 和强类型端口/参数 key 表示；AE 任意数据参数路径已接入，SmartFX 预渲染创建不可变节点快照。
- SmartFX、序列化、Compute Cache、CPU 参考渲染器和 GPU 后端的边界见 [架构说明](docs/architecture.md)。
- 行为覆盖按独立验收场景推进，见 [行为清单](docs/compatibility-matrix.md)。
- 当前构建和验收仅针对 AE 2023，暂不进行新版本适配；分阶段交付见 [开发路线图](docs/roadmap.md)。
- AE 2023 曾确认较早的 M2 构建可加载并渲染；当前 24 参数构建（21 个控件 + 图数据/来源/捕捉）与 `cep_panel/` 面板均待宿主复测。面板到宿主网关的调用已修好（网关入口点原先在文件内是私有的，脚本调用直接报错），已安装副本需要按面板说明更新。低分辨率下的发射器原点仍被报告为偏移，定位需要你把 Full 与 Quarter 两次 `Options` 读数发回（步骤见[参数映射](docs/parameter-mapping.md)）。功能证据与 agent 任务拆分见[参考功能目录](docs/reference-inventory.md)和[任务清单](docs/agent-backlog.md)。

## 面板与交付示例

- 可停靠节点面板：`cep_panel/`（CEP，AE 2023），显示固定的 `发射器 → 力场 → 外观 → 输出` 链、连线与可编辑参数；协议、安装与更新步骤见 [面板说明](cep_panel/README.md) 和 [ADR 0009](docs/adr/0009-cep-panel-bridge.md)。已安装副本是**拷贝**而非链接，源码改动后按面板说明里的 `robocopy` 覆盖并重开面板即可（无需重启 AE）。
- 三个交付示例（火花 / 飘雪 / 漂浮光点）：精确参数配方见 [交付示例](docs/examples.md)，面板的 `Example` 下拉可直接套用；不装面板也可以照表手填。
- 宿主验收清单（含面板与示例）在 [行为清单](docs/compatibility-matrix.md) 的 M2-06 小节。

## 构建

用 CMake 构建不依赖 AE 的核心库。Windows AE 插件使用锁定的 MSVC v145 和本地 May 2023 SDK；从仓库根目录运行 `powershell -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1`，默认产物写入 `artifacts/plugin/2023/x64/Release/`。显式传入 `-SdkPath 'AdobeSDK\May2023_AfterEffectsSDK' -ArtifactLabel 2023` 得到同一目标。详细工具链和构建状态见 [构建矩阵](docs/build-matrix.md)。

## 许可

本项目采用 MIT 许可，全文见 [LICENSE](LICENSE)。

Adobe After Effects SDK 不随本仓库分发，仅作为本地构建输入使用；仓库中不包含 SDK 头文件、示例源码、PiPL 工具或任何生成产物。引入第三方依赖前必须记录其版本、许可与对 AE 进程内稳定性的影响（见 [CONTRIBUTING.md](CONTRIBUTING.md)）。
