# 构建、部署与回滚

当前安装、候选、哈希和证据只记录在 [current-state.md](current-state.md)。历史逐版本构建流水见 [归档](history/2026-10-19/pre-web-panel/docs/build-matrix.md)。

## 构建候选

Windows x64 Release，MSVC v145、本地 AdobeSDK/May2023_AfterEffectsSDK，artifact label2023，原生/Core 使用 /MT。SDK 和生成文件不入 Git。WebView2 使用固定 SDK 和静态 loader，不在本任务中安装 Evergreen Runtime。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Prepare-WebView2Sdk.ps1 -Download
powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -NoDistPublish -NoRuntimePublish
```

原生内容发现和资源输入使用 web_panel/。PiPL 与运行时版本/能力必须一致；BuildPiPL 仅接受经严格输入与身份/flags/version 核验的缓存转换。CoreOnly 受共享输入指纹及安装配对约束，不可用来发布尚未安装的新 adapter。

## 发布及恢复

完整当前节点包由 10 个 AEX 和 Core 组成。AE0 下，按 ADR0011 的 standing authorization 使用 Deploy-TestBuild.ps1、既有单个 Plug-ins/Starfield -> dist Junction；完整包包含 Model/Material/Motion/Turbulence 的全部开关。只改前端时可用 -KeepNative，仍须保留配对快照。面板源码由 web_panel/ 直接加载，不依赖 CEP 注册。

Deploy-TestBuild 默认报告，-Install 明确行动；Restore-TestBuild 默认核验，-Restore 才恢复。后者接受历史 cep_panel/ 与当前 web_panel/ 清单前缀；迁移备份通过 restoreSourceDirectory 恢复旧原生程序所需的目录名。源文件有后续修改、未知路径或哈希不符时拒绝覆盖。旧的 Model/Panel 专用 helper 保留其受限历史 baseline 契约，不能用旧 baseline 宣称当前版本可发布。

旧 CEP 注册由 Set-LegacyCepPanel.ps1 单独管理，默认报告；不自动重装 CEP。其 Windows 精确路径和管理员要求见当前状态。任何脚本不写注册表，其他宿主变动仍需逐项授权。

## 产物及保留

| 路径 | 策略 |
| --- | --- |
| artifacts/plugin、core-dll、gpu-build、webview2/dependencies | 保留当前候选、符号、指纹及本地依赖 |
| artifacts/*-tests、panel-tests、all-tests | 测试夹具和日志；保留验证证据 |
| artifacts/disabled | owner 授权仅保留最近 5 个本 checkout 的实际 deployment.json 备份；清理前复制清单、记录删除目录/字节，不跟随 Junction |
| artifacts/rollback-retention | 旧回滚包清理前后的清单与 deployment 元数据；删除的旧二进制不可恢复 |
| artifacts/prepared、runtime、crash、reports、diagnostics、worktrees | 本轮备份清理不触碰；不要从“旧目录”推断可删 |
| dist、AdobeSDK | 安装或构建输入，禁止当作缓存删除 |

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Prune-DeploymentBackups.ps1
# 只有明确授权且核对报告后才执行：
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Prune-DeploymentBackups.ps1 -Keep 5 -Prune
```

不要使用 git clean/reset 清理共享工作树。源测试不按年龄删除；注册工作树走对应 Git 流程。后续清理不能从本次授权推断可以删除 SDK、prepared 候选、事故证据或在用 Core。
