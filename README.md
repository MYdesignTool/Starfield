# Starfield Core

独立实现一套面向现代 After Effects 的粒子效果。当前阶段先建立稳定的核心边界和数据契约；旧插件样本与逆向分析材料保留在父目录，不进入本工程构建。

## 当前进度

- M0/M1 contracts and the native shell are in place. M2 adds SmartFX transport, an 8/16/32-bpc CPU sprite renderer, deterministic time evaluation, and source compositing.
- M3-01 adds seeded Point/Box/Sphere/Disc birth distributions, emitter position, per-particle velocity spread, and deterministic random streams.
- G-01/G-02 provide typed graph validation plus bounded, versioned sequence serialization. The codec is core-only: graphs are not persisted in AE or used by rendering yet.
- Core self-tests pass (3,828 checks), and the Windows x64 plug-in builds against both the May 2023 and AE 26.5 SDKs. The most recent M3-01 control layout has not yet been re-tested in AE; host evidence is tracked in [行为清单](docs/compatibility-matrix.md).
- This remains a render slice, not Stardust parity. The current look is white 2D sprites with constant size/opacity; forces, age curves, color/texture sources, depth, mesh/volume rendering, presets, graph persistence/evaluation, and the dockable editor remain unfinished. See the [current feature audit](docs/current-feature-audit.md).

## 当前架构

- 纯 C++ 核心不依赖 AE SDK；宿主指针和 suite 只出现在 `ae_plugin/` 适配层。
- 渲染以不可变参数快照和绝对时间求值为基础，便于乱序帧请求与后续 MFR。
- `Settings` 验证器在进入渲染前限制资源上界并替换非有限值。
- 图模型以独立 UUID 和强类型端口/参数 key 表示；核心验证完成，但序列持久化和渲染求值尚未接入。
- SmartFX、序列化、Compute Cache、CPU 参考渲染器和 GPU 后端的边界见 [架构说明](docs/architecture.md)。
- 行为覆盖按独立验收场景推进，见 [行为清单](docs/compatibility-matrix.md)。
- 目标宿主为 AE 2023 及之后版本，分阶段交付与 SDK/工具链版本策略见 [开发路线图](docs/roadmap.md)。
- AE 2023 曾确认较早的 M2 构建可加载并渲染；当前 13 控件的 M3-01 构建待复测。功能证据与 agent 任务拆分见[参考功能目录](docs/reference-inventory.md)和[任务清单](docs/agent-backlog.md)。

## 构建

用 CMake 构建不依赖 AE 的核心库。Windows AE 插件使用当前锁定的 MSVC v145 工具集与本地 Adobe SDK；从仓库根目录运行 `powershell -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1`，产物写入 `artifacts/plugin/x64/Release/`。单独构建 2023 SDK 对照版本时，传入 `-SdkPath 'AdobeSDK\May2023_AfterEffectsSDK' -ArtifactLabel 2023`，避免覆盖主产物。详细工具链版本和构建状态见 [构建矩阵](docs/build-matrix.md)。

## 许可

本项目采用 MIT 许可，全文见 [LICENSE](LICENSE)。

Adobe After Effects SDK 不随本仓库分发，仅作为本地构建输入使用；仓库中不包含 SDK 头文件、示例源码、PiPL 工具或任何生成产物。引入第三方依赖前必须记录其版本、许可与对 AE 进程内稳定性的影响（见 [CONTRIBUTING.md](CONTRIBUTING.md)）。
