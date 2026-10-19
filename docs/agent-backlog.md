# 当前任务卡

## P-05 — 整理原生面板菜单（已部署，等待 AE 复测）

Owner 要求窗口菜单只保留两个正式面板入口，内部作者命令隐藏，手动图修复移入面板 Diagnostics。拥有 StarfieldHost/WebPanelHost、会话命令绑定、共享 JSX 及候选传输、前端 generation、对应测试/构建/配对部署与当前文档。约束：命令处理逻辑、节点、参数身份、持久化、Core ABI9 和渲染保持。SDK 命令分配/注册与可见菜单分离；每次安全 idle 脚本执行前提供当前会话 ID，禁止依赖隐藏菜单的名称查找。native100/panel101 已构建、离线通过并配对部署，位于 codex/p05-panel-menu-cleanup；实际安装、回滚与待 owner 复测范围见 current-state.md。离线通过不代表 AE 菜单/预设实测通过；本轮尚未并入 main。

## P-04 — AEGP Panel + WebView2 已完成

旧 CEP 注册已在 owner 要求重发 UAC 后移出扫描目录；web_panel 迁移及已验证检查点已合入本地 main。源码/安装配对、精确注册路径、证据和恢复命令见 [current-state.md](current-state.md)。本卡不再存在待执行的管理员清理或 main 合并步骤。

Owner 已接受 native98/panel100 的节点画布、预设及模态保存退出复测，并要求退役旧 CEP、合入本地 main、将共享前端改为 web_panel、整理过时文档和仅保留最近几个回滚版。本次采用最近 5 个实际部署备份策略。

本次拥有范围：WebPanelHost/Protocol 与 Host 集成、前端传输和当前预设验证修复、路径迁移涉及的原生资源/构建/测试/部署/回滚工具、.vscode/、活跃文档、旧 CEP 精确注册退役及备份保留工具。后续配对及状态只在 [current-state.md](current-state.md) 维护。

约束：不改变节点行为、参数身份、schema、动画、Core ABI 或渲染；不恢复旧属性编辑器/Appearance。UI/JSX 保留 panel100 generation。native99 只为目录迁移重新构建。退役只操作本项目已核对 Junction；Windows 注册表、其他扩展与共享缓存不改动。

验收与授权分别记录：owner 的版本接受是 release decision；未逐项报告的 IME、硬件、headless、性能和所有节点排列不补记为实际通过。旧 CEP 退役需 Windows 管理员权限；实际未成功时不能标记完成。保持默认报告、显式行动和一条恢复命令。

## 后续任务

无默认的新功能实现指派。Motion 前缀修正后的实际轨迹、硬件/性能等专项证据按 [路线图](roadmap.md) 与 [行为验收](compatibility-matrix.md) 安排。MNT-01 的旧范围不再限制 owner 本轮明确授权的文档及旧备份整理。

旧文档中的多张重复 Active M3-20、旧暂停指令和逐版本交接已移入 [历史任务卡](history/2026-10-19/pre-web-panel/docs/agent-backlog.md)。它们是证据，不是当前 agent 的执行指令。保留预先存在的工程分析草稿、日志和无关工作树改动，不混入本次提交。

## GOV-01A — 技术债小修（实施中）

Owner 于2026-10-19授权直接修复小问题；架构级重做须另行批准。本卡仅修 CMake 遗漏的既有 Core 输入、Model ABI9/current 与 ABI8/7 兼容测试、聚合入口中已点名的七项离线 runner、runner 耗时及过时技术债描述。拥有 CMakeLists.txt、tests/model_transport_tests.cpp、tests/RunModelTransportTests.ps1、tests/RunAllTests.ps1、tests/RunPanelTests.cjs、docs/engineering/TECH_DEBT.md；本卡在此追加，不覆盖在途 P-05。

不修改生产算法、SDK、参数/ABI/schema、宿主事务、dist 或安装状态。不重构构建体系、参数映射、求值/缓存边界。验证 clean CMake DLL/CTest、Model ABI 精确旧前缀、已接入的离线 runner 与面板 runner；完整注册 catalog/DOM 聚合和源码身份清单仍属于后续工作，不用局部通过关闭宿主门。证据保存 artifacts/gov-01a-20261019/。
