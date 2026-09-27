# Starfield Core

独立实现一套面向现代 After Effects 的粒子效果。当前阶段先建立稳定的核心边界和数据契约；旧插件样本与逆向分析材料保留在父目录，不进入本工程构建。

## 当前架构

- 纯 C++ 核心不依赖 AE SDK；宿主指针和 suite 只出现在 `ae_plugin/` 适配层。
- 渲染以不可变参数快照和绝对时间求值为基础，便于乱序帧请求与后续 MFR。
- `Settings` 验证器在进入渲染前限制资源上界并替换非有限值。
- SmartFX、序列化、Compute Cache、CPU 参考渲染器和 GPU 后端的边界见 [架构说明](docs/architecture.md)。
- 行为覆盖按独立验收场景推进，见 [行为清单](docs/compatibility-matrix.md)。
- 目标宿主为 AE 2023 及之后版本，分阶段交付与 SDK/工具链版本策略见 [开发路线图](docs/roadmap.md)。
- M1 空壳已由用户确认可在 AE 2023 加载；它还没有粒子控件或粒子渲染。功能证据与 agent 任务拆分见[参考功能目录](docs/reference-inventory.md)和[任务清单](docs/agent-backlog.md)。

## 构建

用 CMake 构建不依赖 AE 的核心库。Windows AE 插件使用当前锁定的 MSVC v145 工具集与本地 Adobe SDK；从仓库根目录运行 `powershell -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1`，产物写入 `artifacts/m1/x64/Release/`。单独构建 2023 SDK 对照版本时，传入 `-SdkPath 'AdobeSDK\May2023_AfterEffectsSDK' -ArtifactLabel 2023`，避免覆盖主产物。详细工具链版本和构建状态见 [构建矩阵](docs/build-matrix.md)。
