# 当前状态 — native100 / panel101 / Core ABI9 — 2026-10-19

## 当前源码、安装与验证边界

当前任务为 **P-05 菜单整理**。源码位于 `codex/p05-panel-menu-cleanup`；P-04 容器/目录迁移和 CEP 退役此前已合入本地 main。P-05 已完成实现、离线检查与配对部署，等待 owner 在 AE 中复测后再整合本轮变更；尚未推送远端。

当前 UI 是 AEGP Panel + WebView2，前端目录为 **web_panel/**。参数、动画、曲线和渐变通过 AE Effect Controls 编辑。不要重建 cep_panel/、恢复浮动属性编辑器/Appearance 或自动安装 CEP。

native100/panel101 将六个内部命令定义为 AEGP_Menu_NONE，Window 菜单仅注册 Particle Controls (WebView2) 和 Presets (WebView2)。原生安全 idle 在每次脚本调用前提供本会话命令 ID，gateway 验证后执行既有命令处理器；绑定存在但无效时拒绝，不转而查找别的菜单。无绑定的旧宿主保留名称查找回退。手动修复位于节点面板右上角 **Diagnostics → Repair Selected Graph**，仍要求有效面板、主线程、可用宿主与事务互斥。

节点行为、参数 IDs/schema、动画、Core 源码/ABI9 和渲染边界未修改。前端 generation101 用于清除旧 gateway/页面缓存；原生 PiPL/代码 build100，packed32868。完整重建改变了 DLL 哈希，不宣称 Core 二进制与旧版逐字相同。

May2023 SDK / x64 Release 完整 BuildWindows 通过，使用两个 NoPublish 开关。随后在只读 AE0 检查后按 ADR0011 standing authorization 发布；既有单个 Plug-ins/Starfield -> dist Junction 保持。11 个候选/安装/旧备份 native/Core 哈希、26 个当前/备份前端文件及 selector 已核验。

- Installed Host SHA256：`E7A29938362EFD967BB458B30929A89B7E52A505886E728D76FB31FE2B5F0F6F`
- Installed Core SHA256：`63A3F63511C5F6BF55D8E4CA06D1481D6136B5CF09BB7C974623E175CF48122B`
- Selector：`StarfieldCore-63A3F63511C5F6BF.dll`
- 证据：artifacts/webview2/native100-menu-build.log、native100-deploy-{before,after}.json、native100-deploy.log。

Owner 此前在 AE2023.5.0 Build52 接受画布、panel100 预设和 native98 保存确认/取消退出复测，并在截图中观察到原生菜单的 8 个入口。**native100/panel101 尚无实际 AE 复测记录**。需要确认 Window 只显示两个入口、选择节点定位参数、节点编辑/撤销、预设应用与文件导入导出、Model 工作流和保存/退出无回归，并确认 Diagnostics 中存在修复入口；正常工程无需为了验收而执行修复。agent 没有启动/停止 AE，也没有补记 IME、硬件、headless、性能等未报告场景为通过。

## 离线证据及回滚

54 个前端 suite、479 项实际 WebPanelHost/fake-AE 检查通过。覆盖隐藏命令定义及注册失败、每次脚本绑定、全部六项 ID 的校验/刷新/旧版回退、无可见菜单查找的节点提交与 Model/预设文件事务，以及诊断菜单的显式修复路由、模态/互斥/失效面板阻断。最后修改的模型资源导出和只读诊断报告分别补跑通过。测试不启动 AE 或 WebView2，不等同于真实 AE 的 Menu_NONE 调度证明。

日志为 artifacts/webview2/native100-menu-{host,panel}-tests.log 和 artifacts/panel-tests。此前 P-04 的脚本/回滚夹具证据保持历史范围，本轮没有将旧计数冒充重新运行。

最近回退基线为 **native99/panel100**，备份 `p05-native100-panel101-menu-20261019`。Restore 只读预检通过，恢复目录仍为 web_panel。一步回退（AE 关闭）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p05-native100-panel101-menu-20261019' -Restore
```

Restore 会先保留候选，再恢复已验证的原生/Core 与前端快照；源文件有后续改动时拒绝覆盖。不要单独复制旧 Host/AEX 或仅修改前端版本号。

## 旧 CEP Windows 注册

**旧 CEP 注册已移出扩展扫描目录，P-05 未改变注册状态。** 仅保留本项目自己的恢复入口；没有修改其他扩展、注册表或共享缓存。

扫描目录内 `C:\Program Files (x86)\Common Files\Adobe\CEP\extensions\cep_panel` 不存在。保留的本项目 Junction 位于 `C:\Program Files (x86)\Common Files\Adobe\CEP\starfield-legacy-cep-disabled`。历史目标失效属于已退役注册状态，不代表要重新创建 cep_panel 源码目录。

P-04 的管理员退役证据保留于 artifacts/webview2/elevated-cep-retirement-result.json、elevated-cep-retirement.log 和 cep-registration-*.json。owner-retirement-acceptance.json 是当时接受/授权的历史身份，不能改成 owner 接受了当前尚未实测的 native100。仅在 owner 明确要求恢复 CEP、AE 关闭且具备 Windows 权限时，才执行独立恢复：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Set-LegacyCepPanel.ps1 -Action Restore
```

## 文档与最近五版保留

过时入口的原文位于 [2026-10-19 归档](history/2026-10-19/pre-web-panel/README.md)。历史版本、passing counts 和冻结故障恢复脚本不再承担当前部署指令。预先存在的工程草稿、日志及无关工作树改动不混入 P-05。

按 owner 最近五版策略，本轮新增备份后移除最旧的 m3-20-lightpath-inventory-diagnostic-native96-20261019，释放11,594,599字节。此前113包清理证据仍保留；本轮删除前后记录在 artifacts/rollback-retention/20261019-210956938 和 native100-backup-retention.log。当前实际部署备份为：

- p05-native100-panel101-menu-20261019
- p04-native99-web-panel-20261019
- p04-native98-modal-exit-20261019
- p04-native97-panel100-preset-curves-20261019
- p04-native97-webview2-20261019

SDK、prepared、当前运行文件、事故/测试证据和注册 worktree 未因保留整理而删除。已删除旧包的历史命令不再是恢复入口。
