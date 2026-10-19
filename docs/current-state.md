# 当前状态 — native99 / panel100 / Core ABI9 — 2026-10-19

## 源码、安装与验收

前端目录已从 cep_panel/ 改为 **web_panel/**。当前 UI 是 AEGP Panel + WebView2 的节点画布和独立预设窗口；参数、动画、曲线/渐变在 AE Effect Controls 编辑。不要重建 cep_panel/、恢复浮动属性编辑器/Appearance，或自动重新安装 CEP。旧 Install.ps1 已改为默认只报告。

Owner 在 AE2023.5.0 Build52 接受节点画布、panel100 预设修复和 native98 的保存确认/取消退出复测，并明确要求本轮目录迁移、重建、旧注册退役、文档审查及回滚包保留整理。native99 是源目录/资源定位迁移和过时 fallback 提示清理；panel generation100、节点行为、参数 IDs/schema、动画、Core 源码/ABI9 和渲染边界保持。schema 仅修改 banner 资产位置。

最终 May2023 x64 Release BuildWindows 通过（两个 NoPublish 开关），随后在只读 AE0 检查后按 standing authorization 发布。既有单个 Plug-ins/Starfield -> dist Junction 保持。11 个 native/Core 的候选/安装/旧备份哈希、26 个前端当前/旧快照及 selector 核验通过。

- Installed Host SHA256：`09AC364A461A22C1AE0FB79E933BD84D67252AE97E0D48E9A73E9A4BFBED4675`
- Installed Core SHA256：`418E18F07EF5374CCCDECAA59B52CFFBC4F8C150B0AEA10E7879CE1D5FC9AC13`
- Selector：`StarfieldCore-418E18F07EF5374C.dll`
- 安装/回滚证据：artifacts/webview2/native99-deploy-{before,after}.json、native99-deploy.log。

Core 源码未变，但完整重建的二进制哈希不同，不能宣称与旧版 DLL 逐字一致。目录迁移后没有由 agent 启动 AE；native99 的实际启动/硬件/专项矩阵没有新增实测记录。owner 版本接受不是对未报告的每项 IME、headless、性能或节点排列的逐项通过声明。

## 旧 CEP Windows 注册与 main

**旧 CEP 注册已移出扩展扫描目录。** 仅保留本项目自己的恢复入口；没有修改其他扩展、注册表或共享缓存。

2026-10-19 经 owner 要求重新发起 UAC 后成功。实测注册状态：扫描目录内 true → false，保留目录 false → true。证据为 artifacts/webview2/elevated-cep-retirement-result.json、elevated-cep-retirement.log 及本轮 cep-registration-*.json。

旧注册：`C:\Program Files (x86)\Common Files\Adobe\CEP\extensions\cep_panel`。保留位置：`C:\Program Files (x86)\Common Files\Adobe\CEP\starfield-legacy-cep-disabled`。退役工具只接受本 checkout 的当前或历史源码 Junction，支持已失效的历史目标；不访问别的扩展。

**已执行成功的退役命令（记录用途，不要重复执行）：**

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Set-LegacyCepPanel.ps1 -Action Disable -QualificationReport artifacts/webview2/owner-retirement-acceptance.json
```

对应的一步恢复（AE 关闭，需相应 Windows 权限，只有 owner 明确要求恢复 CEP 才执行）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Set-LegacyCepPanel.ps1 -Action Restore
```

恢复会将历史 Junction 的指向重新绑定到 web_panel。验收记录保留真实 owner 原句及 accepted native98 / authorized native99 rename 的边界，不捏造全矩阵通过。已快进合入本地 main；尚未推送远端。

## 测试及回滚

53 个前端 suite、464 项实际 WebPanelHost/fake-AE 检查通过（含从真实测试 EXE 位置发现新目录），11 个脚本 scope 无失败；旧 Model 两项还单独以 -Run 执行，部署62项/制备24项通过。制备夹具不再误用旧 recipe81 作为当前版本，并补齐正式契约检查依赖。最近五版保留夹具验证默认报告、按部署时间排序及不跟随 Junction。日志位于 artifacts/webview2/native99-*.log 与 artifacts/panel-tests、all-tests。

最近回退基线为 native98/panel100，备份名 `p04-native99-web-panel-20261019`，配对 Restore 预检通过。一步回退（AE 关闭）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p04-native99-web-panel-20261019' -Restore
```

该基线的原生加载路径仍为 cep_panel，Restore 会验证/保留当前源码后恢复旧内容和目录名；在 Git 中可见。不要直接复制旧 AEX 或调用与当前哈希不配对的历史恢复命令。CEP 注册的停用/恢复仍是独立动作。

## 文档及旧备份清理

13 份清理前入口的原文见 [归档](history/2026-10-19/pre-web-panel/README.md)。活跃架构/任务/路线/测试文档已移除重复旧安装、Active/暂停交接和旧 passing counts；ADRs 与参考报告中的日期属于各自证据时间。工程分析草稿及其他预先存在的无关改动没有混入本次提交。

已按 owner 授权删除 **113 个旧部署回滚包**，释放 814,872,257 字节（约 777.1 MiB）；删除目录和 deployment 元数据保留在 artifacts/rollback-retention。仅保留本 checkout 最近5个实际部署备份：

- p04-native99-web-panel-20261019
- p04-native98-modal-exit-20261019
- p04-native97-panel100-preset-curves-20261019
- p04-native97-webview2-20261019
- m3-20-lightpath-inventory-diagnostic-native96-20261019

未匹配本 checkout bundle 的记录、没有 deployment 清单的保留材料、prepared、SDK、在用 Core/runtime、故障日志及注册 worktree 未删除。旧历史命令可能指向已删除备份，不再是恢复入口。当前身份以本页、实际收据及 Git 为准。
