# AE2023 行为验收

## P-04 AEGP/WebView2 gate — 2026-10-19

Latest: owner accepts panel100 retest, then reports a modal-script error during
AE exit before retirement. Native98 addresses host readiness and scoped error
reporting with463 offline checks. Actual quit acceptance is reopened; keep CEP
registered and defer main merge. See current-state.md for the installed pairing.

Owner now confirms no visible faults in the native node canvas. Presets fail
on panel99 at Particle8 published-record verification; supplied age bytes are
binary32 rounding of .1/.8. Panel100 is installed with unchanged native97/Core
bytes after AE0; offline semantic/rollback cases pass, but actual Add/Replace,
Undo and reopen qualification is still open. Keep legacy CEP enabled.

Baseline c43e293/native96/CEP99. Candidate native97 retains all existing author
transactions and node/render contracts. Offline transport/build evidence cannot
close these native-container gates. Legacy CEP remains available until all pass.

| Native-container behavior | Required AE23.5x52 evidence |
| --- | --- |
| Docking/lifecycle | Open from Window/menu launcher, dock/float, resize, tab hide/show, workspace restore, close/reopen, AE exit |
| Input | Chinese IME/text, prompt, focus to ECW, canvas drag/zoom/multi-selection, Delete/Ctrl+D and Undo/Redo with text focus distinguished |
| All current node kinds | Emitter/Auxiliary/Particle/Force/Transform/Motion/Turbulence/Model/Material/Output; create/select/connect/disconnect/duplicate/delete and rollback |
| Native authoring | Same parameter values, curves, gradient, animation, light prefix and geometry/reference behavior as baseline |
| Presets/resources | Separate modeless window, built-in/custom import/export, Add/Replace, render settings, large paged Model files and native file dialogs |
| Project behavior | One undo per edit, redo, save/reopen, duplicate/copy/paste, pinned-target identity and project-switch conflict rejection |
| Failure/headless | Missing runtime/COM failure is visible without startup failure, browser failure/reopen, expired request cannot write, no UI in suppressed/render-only mode |
| CEP retirement | Verify installed native hash and all above results; retain exact extension Junction for one-step restore; no shared registry/cache changes |

编译和 fake-host/Core 测试不等于 AE 实际行为通过。当前安装版本见 [current-state.md](current-state.md)；旧观察保留在 [有日期的原始记录](history/2026-10-07/compatibility-matrix.md)。

## 已记录观察的边界

AE2023.5.0 Build52 的早期候选已有 owner 观察：透明粒子、8/16/32-bpc、Full/Half/Third/Quarter 坐标、时间一致性、基本形状/重力/大小、保存/重开、复制/粘贴与撤销/重做、自动 CEP 发现。
H-01 的 Full/Quarter 手动热重载、缺失/坏选择文件 fallback，以及与旧单体三个 Full/8-bpc render-queue 帧的 decoded RGBA 对比有记录。
这些证据属于记录中对应的候选和场景，不自动覆盖后来所有曲线、GPU、Motion Blur、Transform 或所有工程。native45 曲线操作已有 owner 定性接受；性能没有可推广的基准数字。

## 剩余 gates

| 行为 | 需要的证据 |
| --- | --- |
| 当前原生/CEP 图编辑 | 添加/删除/连线/多选、原生 ECW 改值、读回一致、冲突回滚与单次 undo |
| 项目持久化 | graph bytes/曲线/渐变/旋转、原生动画、资源、重开、复制及迁移策略 |
| panel54 palette | 实际外观/坐标、取消、拖动与目标切换 |
| CEP 性能 | idle/seek、多 Emitter、Add/Replace 的光标与延迟 |
| Motion Blur | 所有模式、camera disregard、各色深、短 lifetime、反向时间与成本 |
| Transform | 已部署的 ABI6/panel54 候选；Null 动画/父级/剪切/反射/奇异尺度、ordered Force/Auxiliary 与 shutter |
| GPU | 实际 AE 支持的设备/backend、失败 fallback、CPU 像素对照与资源/取消 |
| Core reload | 真正渲染进行中的切换、重复切换、取消、反向请求、扩大像素场景 |
| SDK 线程边界 | Camera/Layer suite 各 selector 与线程允许范围；禁用 MFR 不代替审计 |

每条记录至少包含 host family/build、bundle 哈希、步骤、实际输出和失败证据。selection crash 的早期记录保留用于追溯；未经复现不能把静态猜测写成当前根因。Options 的 geometry/history 是进程全局最近读数，可能来自另一实例；文本中的硬编码 Build31 不是可靠的当前二进制身份。
宿主操作与部署按 ADR0011。整理期间没有新 AE 验收，以上开放 gates 均保留。
