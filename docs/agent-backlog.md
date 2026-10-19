# 当前任务卡

## P-04 — AEGP Panel + WebView2 已完成

旧 CEP 注册已在 owner 要求重发 UAC 后移出扫描目录；web_panel 迁移及已验证检查点已合入本地 main。源码/安装配对、精确注册路径、证据和恢复命令见 [current-state.md](current-state.md)。本卡不再存在待执行的管理员清理或 main 合并步骤。

Owner 已接受 native98/panel100 的节点画布、预设及模态保存退出复测，并要求退役旧 CEP、合入本地 main、将共享前端改为 web_panel、整理过时文档和仅保留最近几个回滚版。本次采用最近 5 个实际部署备份策略。

本次拥有范围：WebPanelHost/Protocol 与 Host 集成、前端传输和当前预设验证修复、路径迁移涉及的原生资源/构建/测试/部署/回滚工具、.vscode/、活跃文档、旧 CEP 精确注册退役及备份保留工具。后续配对及状态只在 [current-state.md](current-state.md) 维护。

约束：不改变节点行为、参数身份、schema、动画、Core ABI 或渲染；不恢复旧属性编辑器/Appearance。UI/JSX 保留 panel100 generation。native99 只为目录迁移重新构建。退役只操作本项目已核对 Junction；Windows 注册表、其他扩展与共享缓存不改动。

验收与授权分别记录：owner 的版本接受是 release decision；未逐项报告的 IME、硬件、headless、性能和所有节点排列不补记为实际通过。旧 CEP 退役需 Windows 管理员权限；实际未成功时不能标记完成。保持默认报告、显式行动和一条恢复命令。

## 后续任务

无默认的新功能实现指派。Motion 前缀修正后的实际轨迹、硬件/性能等专项证据按 [路线图](roadmap.md) 与 [行为验收](compatibility-matrix.md) 安排。MNT-01 的旧范围不再限制 owner 本轮明确授权的文档及旧备份整理。

旧文档中的多张重复 Active M3-20、旧暂停指令和逐版本交接已移入 [历史任务卡](history/2026-10-19/pre-web-panel/docs/agent-backlog.md)。它们是证据，不是当前 agent 的执行指令。保留预先存在的工程分析草稿、日志和无关工作树改动，不混入本次提交。
