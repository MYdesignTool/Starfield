# 构建与产物管理

当前源码/安装版本和哈希见 [current-state.md](current-state.md)，带日期的构建/部署过程见 [历史记录](history/2026-10-07/build-matrix.md)。

## 工具链与候选

Windows x64 Release 使用 MSVC v145、本地 AdobeSDK/May2023_AfterEffectsSDK、artifact label2023；AE 范围为2023。
原生 AEX/Core 使用 /MT，避免 AE2023 自带旧 MSVC runtime 干扰。SDK 头、样例、PiPL 工具与生成输出不进入 Git。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -NoDistPublish -NoRuntimePublish
# 显式默认 SDK/产物标签：
powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -SdkPath 'AdobeSDK\May2023_AfterEffectsSDK' -ArtifactLabel 2023 -NoDistPublish -NoRuntimePublish
```

默认 BuildWindows 会尝试发布，候选检查必须显式使用以上两个 NoPublish 开关。PiPL flags/version 与运行时代码保持一致；BuildPiPL 校验并生成资源。主要代码变化按需完整重建，不能依赖旧资源或时间戳推断二进制兼容。
GPU 本地构建依赖放在 artifacts/gpu-build，Prepare-GpuBuildInputs.ps1 和 Build-GpuKernels.py 维护它；清理保留这些依赖和生成输入。

## 发布、安装和回滚

完整候选包含 Particle、Emitter、ParticleNode、Force、Transform、Host 六个 AEX 和 Core。dist 是经单个 AE Plug-ins/Starfield Junction 使用的安装 bundle，不能作为一般垃圾目录删除。
CoreOnly 不改 AEX，但共享源/ABI 指纹变化会拒绝该路径；runtime 发布还检查安装的 AEX 与对应候选配对。当前 native54/panel54 安装使用 ABI6；更改 ABI 时必须成对重建和发布。

Deploy-TestBuild.ps1 默认报告，显式 -Install 才行动；Restore-TestBuild.ps1 默认检查配对回滚，显式 -Restore 才行动。BackupName 必须是本次唯一名字，备份保留在 artifacts/disabled。
按 ADR0011 的 standing authorization，紧邻部署的只读进程检查确认 AfterFX/AfterFX_64 未运行后可使用既有 Junction 部署 Starfield；其他宿主改动仍需逐项授权。部署保留/核对前后哈希与一条回滚命令；任何脚本不写注册表。

## 目录与清理

| 路径 | 内容 | 整理策略 |
| --- | --- | --- |
| artifacts/plugin/2023/x64/Release | 当前原生候选、符号、配对指纹 | 保留当前 AEX/PDB/指纹；去除退役 Appearance 输出 |
| artifacts/core-dll/2023/x64/Release | Core 候选、符号 | 保留当前 DLL/PDB |
| artifacts/plugin/2023/obj、core-dll/2023/obj、editor | 编译/IntelliSense 缓存 | 可再生成 |
| artifacts/*-tests、all-tests、panel-tests | 编译测试、日志、隔离夹具 | 编译中间产物可清，日志记录保留 |
| artifacts/disabled、prepared | 部署备份及待发布/回滚来源 | 保留 |
| artifacts/runtime | 旧 runtime；备份 Junction 仍可能引用 | 保留，不能按“旧目录”直接删除 |
| artifacts/worktrees | 已登记的 Git worktree | 保留，用 Git/worktree 流程处理 |
| artifacts/crash、reference、reports、diagnostics | 故障与行为证据 | 保留 |
| artifacts/history/20261007 | 旧的一次性脚本、日志、收据 | 归档并保留原路径清单 |
| artifacts/report-verification-20261007 | 报告核验与探针 | 保留源/结果，清掉可再编译的二进制 |
| artifacts/workspace-cleanup-20261007 | 本次清单、验证与文档恢复包 | 保留 |

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Clean-Workspace.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Clean-Workspace.ps1 -Clean
```

清理默认报告；-Clean 只接受明确目录和扩展名，验证绝对路径在 artifacts 内，拒绝 reparse point/链接和正在运行的编译器。保留 dist、SDK、备份、GPU 依赖与 worktree；删除库存写在 workspace-cleanup 目录。
禁止使用 git clean -fd 清理共享工作树：共享工作树中的 Transform 集成源码和测试属于另一项正在开发的任务。测试入口见 [testing.md](testing.md)。
