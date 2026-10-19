# 测试入口

当前结论与安装身份见 [current-state.md](current-state.md)。旧版逐次测试流水已完整移入 [历史测试记录](history/2026-10-19/pre-web-panel/docs/testing.md)，不沿用旧 passing counts 作为当前结果。

## 当前命令

从仓库根目录运行。Node runner 自动发现全部 *_tests.js，并保留每个 suite 的日志：

```powershell
node tests/RunPanelTests.cjs
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunWebPanelHostTests.ps1 -Run
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1 -HostBootstrap
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunAllTests.ps1 -Group Scripts
```

前端默认 web_panel/；STARFIELD_PANEL_ROOT 可指向 artifacts 下的隔离候选。suite 失败继续收集，最终非零退出；符号链接权限或缺少驱动/工具与实现失败分别记录，不跳过失败项宣称通过。

## 本次验证范围

- WebPanelHost 生产队列的 fake-AE 检查覆盖来源、期限、关闭/重放/FIFO、模态禁用/取消、quiet-error 恢复及从真实测试 EXE 位置发现 web_panel，不启动 AE 或 WebView2。
- 面板 suite 覆盖既有节点、预设/图事务及容器适配；Particle curve 浮点归一化仍严格校验头部、布局、真实数值差异和完整回滚。
- PowerShell 夹具覆盖部署配对、历史/当前清单前缀、目录恢复、后续修改拒绝，以及最近五版保留和不跟随归档 Junction。
- 实际 AE2023 的 owner 接受与专项未报告项见 [行为证据](compatibility-matrix.md)。native99 路径迁移的离线检查不冒充实际 AE 启动记录。

本轮日志保留在 artifacts/webview2/：native99-web-panel-*.log、backup-retention-tests.log；所有 suite 详细输出在 artifacts/panel-tests 和 artifacts/all-tests。SDK 完整构建使用 BuildWindows.ps1 的两个 NoPublish 开关，输出在 artifacts/plugin、core-dll。当前最终计数从实际结果文件读取并写入当前状态，不在多个文档复制。

编辑器 get_global_id 属于 OpenCL 内建符号的解析环境。所有配置默认强制加载 .vscode/OpenClIntelliSense.h；它不参与运行构建。C17/C++20 独立内核及 CPU shader fixture 的语法检查见 kernel-editor-default-tests.log。用户配置说明见 [editor-setup.md](editor-setup.md)。
