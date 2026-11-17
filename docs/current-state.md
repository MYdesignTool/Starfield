# 当前状态 — native136/panel124 Owner已验收，main同步基线 — 2026-11-17

## INT-01：已验收基线与下一轮开发

Owner于2026-11-17确认当前问题已解决，并要求同步本地及远端main、准备下一轮开发。STB-21按本轮反馈关闭：Size/Opacity/Rotation Over Life生效，新增点可连续拖动，曲线松手闪回已解决。具体AE2023 Build未补充；该确认不扩展为Undo/save/reopen全矩阵、端到端性能指标或完整MFR资格。

本次将当前136/124所依赖的累计插件源码、测试和工程文档形成可追溯提交，保留Transform/Particle等既有改动；同步main后从同一基线创建`codex/next-development`。不增加构建号、不重建或部署。WEB-01官网及Pages工作流继续保留为独立在途工作，不随本次插件基线推送；原“暂不上线”约束保持。522指纹核对0不符，Core11,895/编辑器4,078/Over Life4,529通过；面板74/75，退役夹具受执行环境限制未完成。范围见[同步记录](engineering/main-sync-20261117.md)，快照/检查证据位于`artifacts/main-sync-20261117/`，Git提交与远端引用为最终同步凭据。

## 当前部署136/124：成功提交排队重绘

Owner确认135编辑体验正确，但曲线框松手闪回仍未解决。发现成功提交仍由通用invalidate设置PF_EO_UPDATE_NOW，它按May2023 SDK在事件返回后触发立即绘制，已超出135调用内显示保护。136仅对成功提交取消此强制立即重绘：局部失效通知仍发出，并放在既有显示保护内以覆盖通知重入；随后让宿主按正常参数采纳/绘制队列处理。拖动、取消和失败恢复仍立即更新，FORCE_RERENDER及135手势/134提交保留，不增加延时定时器或跨调用覆盖层。此为独立重绘顺序模型的代码复现，不能据此声称已测量AE内部顺序或已关闭现场闪回门。

最终编辑器夹具对冻结135为4,135检查/9失败，136普通/ASan各4,078通过，覆盖提交中、局部失效通知重入、事件返回后的旧参数立即绘制、正常采纳后绘制，以及Undo/失败恢复。真实UI/native/预设/采样/Core链路普通/ASan各4,529、Particle注册642、冻结135/136实际Core DLL2,286、九效果PiPL/runtime333与完整May2023 SDK136通过。严格分析仍有原来的C6237/C6326和bitmap C6262 54,644字节，不记整体通过。Owner现已确认136解决曲线松手闪回及本轮原问题；此为实际操作反馈，不代表测量了AE内部事件顺序。

17:50（+08）freshAE0，经Deploy-TestBuild.ps1与现有单Junction安装136/124；前1,089/后1,100共2,189哈希0不符，223构建输入、11安装二进制、29前端、265测试源及135/124回退核验。冻结`artifacts/prepared/stb21flash-native136-panel124-v1`，manifest SHA256 `1A8E29CEA1D350A2420B65ECF2551D430E8383C840A701D287F4B88A3201F529`；Core SHA256 `16621C454B88637E280D05A083F633BA15CEEA92C2F39CAC4FFCF4BD7052E676`，selector `StarfieldCore-16621C454B88637E.dll`。Core源/ABI9/Render.hpp、公开参数/schema和panel124保持；MFR沿用既有实验配置，默认仍关闭。证据`artifacts/stb-21flash-20261117/`，未启动/退出AE。

备份`artifacts/disabled/stb21flash-native136-panel124-deferred-redraw-20261117`完整保留135/124。按五份策略report-first只裁剪旧实际备份`mfr01-native132-panel124-owner-test-20261116`，释放16,242,143字节，收据`artifacts/rollback-retention/20261117-175103991/`；候选/SDK/证据保留。一步回退135（先关闭AE，在checkout执行；会实际回退）：

```powershell
powershell -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'stb21flash-native136-panel124-deferred-redraw-20261117' -Rollback
```

## 上次部署135/124：连续加点与提交期间显示（历史，编辑已确认，闪回仍在）

Owner确认134的Over Life已生效，仍报告加点后需松手再次选点、拖动松手闪回旧图形后延迟更新。135让插入点直接获得当前手势的drag continuation，按下/移动不发布，松手一次发布；在同步发布调用内保存有界的曲线显示预览，重入DRAW即使收到旧或部分更新的参数组也画本次期望曲线。预览只在该调用内有效，不参与渲染/校验，结束或失败即清理，Undo/外部修改不被旧覆盖层遮挡。曲线提交只请求局部控件重绘，取消显式REFRESH_UI整ECW/Layer/Comp重绘，FORCE_RERENDER及134拥有值提交边界保留。三条曲线和共享编辑器行为均有离线回归；实际AE显示与延迟待Owner复测。

同一最终编辑器夹具对冻结134源码为3,900检查/22失败，135普通/ASan各3,955零失败，覆盖首次按下即拖新点、单次发布、旧/部分宿主参数的重入绘制、失败恢复与Undo。真实UI/native/预设/采样/Core链路普通/ASan各4,529、Particle实际注册642、冻结134/135实际Core DLL2,286、九效果PiPL/runtime333与完整May2023 SDK135通过。分析仍有原来的C6237/C6326和bitmap C6262（54,644字节），不称整体通过。只读保存134现场日志的8次曲线source服务为19.436–20.820ms；这不含完整AE渲染/回显，不用于宣称端到端耗时或帧率。

12:32（+08）freshAE0，经既有Deploy-TestBuild.ps1与单Junction安装135/124；前1,089/后1,100共2,189次哈希0不符，223输入、11二进制、29前端、265测试源和134/124备份核验。冻结`artifacts/prepared/stb21ui-native135-panel124-v1`，manifest SHA256 `AE4C3A546873EB173B76E2405FF458BFD67B4473846AED447567D129C61B48CB`；Core SHA256 `8B0B1DE4318CD0C70E29C53F455AF67BD1194DB45D4460FED9D19AF0FECA7D4A`，selector `StarfieldCore-8B0B1DE4318CD0C7.dll`。Core源、ABI9/Render.hpp、参数/布局/schema与panel124不变，MFR继续既有实验配置，默认仍关闭。证据`artifacts/stb-21ui-20261117/`，代理未启动/退出AE。

备份`artifacts/disabled/stb21ui-native135-panel124-curve-gesture-20261117`保留134/124。按五份政策report-first只裁剪旧实际备份`stb20-native129-panel124-json-startup-20261116`，释放17,783,085字节；收据`artifacts/rollback-retention/20261117-123306953/`，候选/SDK/证据保留。一步回退134（先由Owner关闭AE，在checkout执行；命令会实际回退）：

```powershell
powershell -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'stb21ui-native135-panel124-curve-gesture-20261117' -Rollback
```

## 上次部署134/124：Over Life预设提交入口（历史，Owner已确认生效）

Owner实测133仍无效，点击Over Life预设Apply出现SF-T202/AE516，故133没有关闭原故障。134把自定义编辑器的完整曲线/渐变值捕获为拥有值请求，通过独立入口提交，不再套用普通USER_CHANGED_PARAM的数组数量门，也不改写num_params；普通监督回调保持严格数量检查。提交时仍重新获取宿主source kind/UUID/guard与renderer guard，不放宽身份、互斥、完整参数组或发布回执。早期拒绝现在带SF-T203具体原因、kind、参数和delivered数量。133的本地拖动/松手一次提交保留。

旧133源码在20个真实catalog预设×3曲线×delivered 0/1/完整数量矩阵中复现243失败（含3项诊断差异）；新入口普通/ASan各4,529通过，含缺字段/UUID/跨类型/混组/宿主身份变化/已有事务/renderer guard拒绝。选择器以受控替身选择真实catalog，未实际打开Win32窗口或操作AE。编辑器普通/ASan各3,708、Particle实际注册642、原生绑定ASan48,165/Camera12、实际冻结133/134 Core DLL2,286、九效果PiPL/runtime333与May2023完整SDK134通过。严格分析仍只有修前已记录的C6237/C6326和bitmap C6262 54,644字节，不称整体静态通过。截图本身没有delivered数量；数量门是已复现的缺陷，尚不认定已证明现场唯一根因。

12:06（+08）freshAE0经既有Deploy-TestBuild.ps1与单Starfield Junction部署134/124，代理未启动/退出AE。前1,089/后1,100共2,189次哈希0不符；223构建输入、11安装二进制、29panel124、265测试源、实际preset catalog/schema与133/124回退核验。冻结`artifacts/prepared/stb21r-native134-panel124-v1`，manifest SHA256 `C46FD4EF3AC02629F23E1DFF713607649A8086142C5AB6B66E700F456477B435`。Core输入/ABI9/Render.hpp不变，新Core SHA256 `2FD7180673DC37115646181FF1C9A1F86AA24B062E72605D6EDB1DB16E54E6FE`，selector `StarfieldCore-2FD7180673DC3711.dll`，保留MFR实验配置；默认构建仍关闭MFR。实际AE预设Apply、拖动/Undo/save/reopen与完整MFR资格仍待Owner确认。

证据`artifacts/stb-21r-20261117/`；备份`artifacts/disabled/stb21r-native134-panel124-overlife-20261117`完整保留133/124。按五份政策report-first仅裁剪旧实际备份`ux16-native129-panel123-initial-render-20261116`，释放17,779,617字节，记录`artifacts/rollback-retention/20261117-120701410/`，未删除候选/SDK/证据。详见[本轮说明](engineering/particle-overlife-20261117.md)。

一步回退133/124（先由Owner关闭AE，在checkout执行；此命令会实际回退）：

```powershell
powershell -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'stb21r-native134-panel124-overlife-20261117' -Rollback
```

## 上次部署133/124：Particle Over Life提交与拖动（历史，Owner仍报告失败）

Owner报告132的多帧渲染警告已消失，但Size/Opacity/Rotation Over Life无效，拖曲线卡顿。133统一三条曲线所有插值模式的拥有值草稿：移动只更新局部UI，松手一次完整发布，取消/外部编辑/失败恢复原参数组与change flags；不移动不写。自定义参数组提交遇guard、部分上下文或目标不可用时必须报错，不能把未发布当成功。旧实现的guard路径在真实UI→native夹具中复现6项失败；这证明确定缺陷，尚未证明Owner现场就走该分支。详见[本轮说明](engineering/particle-overlife-20261117.md)与ADR0023。

离线：编辑器普通/ASan各3,731；实际UI→图发布→SmartFX式采样→Core粒子属性普通/ASan各1,099，旧源码同夹具6失败；原生绑定ASan48,165/Camera12；冻结132/133实际Core DLL 2,286（含400并行帧、三精度与取消）；九效果实际PiPL/runtime333均通过。May2023 SDK完整133构建通过，沿用132的显式MFR实验配置，默认构建仍关闭MFR。严格/analyze /WX的既有常量比较与渐变bitmap 54,644字节栈警告在修前/修后均复现，未称为全项目静态通过。实际AE曲线/Undo/save/reopen/延迟及完整MFR资格仍开放。

11:32（+08）最终freshAE0，经既有Deploy-TestBuild.ps1和单Starfield Junction完整部署133/124，未启动/退出AE。前1,085/后1,096次哈希共2,181次0不符，223构建输入、11安装二进制、29panel124、runtime selector与132/124备份核验。冻结候选 `artifacts/prepared/stb21-native133-panel124-v1`，manifest SHA256 `404610919617F88C0990ED8C5EE052C1318C27F099553289B6C054162795562F`；Core SHA256 `25B8360A1A596D357F4B0ED21D01EF66014A02627877C89A242463E2252581CF`，selector `StarfieldCore-25B8360A1A596D35.dll`。Core输入/ABI9与panel字节保持，Core新构建二进制另经实际DLL对照。证据 `artifacts/stb-21-20261117/`，候选manifest的deployed=false保留冻结时事实，实际安装以before/after收据为准。

11:28首次安装并核验后，代理核对回退命令时误执行实际Rollback，短暂恢复132；发现后freshAE0重新安装同一冻结133，11:32重新完成上述1,096项安装/回退核验。`rollback-report.log`是这次实际回退日志，不能当只读报告；`after-initial.json`、`deploy-final-action.log`和最终`after.json`保留全过程。没有AE运行/启动/退出。

备份 `artifacts/disabled/stb21-native133-panel124-overlife-final-20261117` 保留完整132/124。按既有五份保留政策分别先报告后裁剪实际备份 `perf17-native128-panel121-canvas-v2-20261111`、`ux16-native128-panel122-first-view-20261116`，共释放35,562,381字节，候选/SDK/证据未裁剪；记录 `artifacts/rollback-retention/20261117-112906869/` 与 `20261117-113237922/`。首次133安装备份也保留在最新五份中。

一步回退132/124（先由Owner关闭AE，从checkout执行）：

```powershell
powershell -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'stb21-native133-panel124-overlife-final-20261117' -Rollback
```

## 上次部署132/124，Owner进行MFR实测（历史）

Owner确认前两项原问题已在MFR需求前测过且无误，并明确要求部署新版本供其测试；按上下文将A01/A02记录为Owner确认（原文“a02和a02”，未补充具体AE Build/逐步骤材料）。不再将这两项列为本轮必重测，不把该确认扩展为132的MFR验收。

2026-11-16 18:20（+08）只读确认AfterFX/AfterFX_64均未运行，经既有Deploy-TestBuild.ps1及单一Starfield Junction完整部署native132/panel124实验配对。使用独立标签 `2023-native132-mfr-qualification`、显式 `-MfrQualificationTest` 与全部八种节点开关；不重编译，不启动/退出AE。11个安装二进制与冻结候选逐字一致，29面板源保持124。新Core selector为 `StarfieldCore-1A8B7281D8BE45FE.dll`，Core SHA256 `1A8B7281D8BE45FE23D3A723D7E4871CB996C25B62B3D357E0187CB178BABC27`。主效果及八种节点的MFR声明现在已激活，但仍是hostQualified=false的实验测试配对；默认构建/正式发布资格未改变。

冻结候选 `artifacts/prepared/mfr01-native132-panel124-host-qualification-v1`、manifest SHA256 `A370317C83DD1A581AE1B000A2DFEC7A95BF8BF767AA853344CEA92574E088EE` 保持原样。部署前559次、部署后570次哈希核对共1,129次，0不符；223原生输入、实际安装/Junction目标及129/124备份11文件/旧selector均核验。备份 `artifacts/disabled/mfr01-native132-panel124-owner-test-20261116` 已保留；panel未变，使用原Deploy工具的二进制回滚，不使用要求panel-snapshot的Restore工具。证据 `artifacts/mfr-deploy-20261116/` 的 `before.json`、`host-before.json`、`deploy-report.log`、`deploy-action.log`、`after.json`。

先报告后按既有保留政策仅裁剪旧实际部署备份 `perf17-native128-panel121-canvas-read-20261111`，释放17,772,061字节；保留最新五份实际部署备份，当前132的129/124回退仍完整。候选/SDK/测试证据未裁剪。记录 `artifacts/rollback-retention/20261116-182115993/`。尚未记录132的真实AE结果；Owner从[清单M01开始](engineering/ae2023-owner-test-checklist-20261116.md)，填[结果模板](engineering/ae2023-test-results-template.md)。

历史回退命令：该129/124备份已在136的五份保留策略中裁剪，以下命令仅保留历史、不可用于当前安装；当前一步回退见页首。

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/Deploy-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'mfr01-native132-panel124-owner-test-20261116' -Rollback
```

## 部署前的离线复核与Owner手工交接（历史记录）

Owner最新要求先离线测试，再由其亲自进行AE实机验收；代理启动/自动退出的提议不再执行。本轮75套面板测试全部通过，之前Junction沙箱阻断也已补过；GPU strict/ASan各24,043项、真实CUDA/OpenCL477项（各96帧渲染/透传/取消）、原生绑定ASan48,165/Camera12、历史缓存ASan131项均通过。新增实际冻结129/132 Core DLL多线程比较2,286项：36组串行配对、400并行帧、24取消、RGBA8/16/32F逐字节对照及所属generation释放通过；生产日志头独立四线程测试194条记录/97区间与异常展开配对通过。实际冻结AEX开/关声明各333通过，宿主脚本/证据检查器离线回归通过。

未改native/panel实现或重建132，未启动/退出AE，未部署或修改宿主设置。最新[离线报告](engineering/mfr-offline-handoff-20261116.md)、[实机清单](engineering/ae2023-owner-test-checklist-20261116.md)、[结果模板](engineering/ae2023-test-results-template.md)已整理：18项常规项目，另单列需专用包的Core热切换门。A01/A02可先用已安装129/124复测，MFR项目须切换到132实验包。所有实机项当前未测，正式MFR资格继续开放。

本轮已完成GPU线程改造：移除跨宿主回调的设备mutex，OpenCL内核参数改为请求自有，CUDA保持逐调用参数，原有上下文/上传/分块/同步后释放保持。严格分析与ASan各24,043项，400渲染+400透传、双向重入/取消/部分分配与驱动故障通过；冻结旧源码被同一并发测试拒绝。真实CUDA/OpenCL各96帧渲染+透传+取消，与同驱动串行逐字节一致，Core/GPU数学未变。首轮测试2e-6容差反例与诊断保留；按既有0.0002 CPU容差且另加严格串行字节对照后477项通过。

完整SDK131前置包已冻结 `artifacts/prepared/mfr01-native131-panel124-gpu-v1`，manifest SHA256 `597076BD2B0D17DDFD9400ADE40C127CC1FFE21122C5A93FD9B8FBC68B592EA9`。继续制作132实验资格构建：只有 `-MfrQualification -RenderJournal -NoRuntimePublish -NoDistPublish` 与独立标签才能开启九效果MFR标志；默认构建仍关闭。主效果与八种节点的实际GLOBAL_SETUP/内嵌PiPL各333项开/关配置通过，不能仅据此声明AE支持。资格日志记录开始/结束/线程/帧时间，只接受不同帧的实际render区间重叠。SDK132完整隔离构建通过，所有11二进制/223输入与测试脚本冻结于 `artifacts/prepared/mfr01-native132-panel124-host-qualification-v1`，manifest SHA256 `A370317C83DD1A581AE1B000A2DFEC7A95BF8BF767AA853344CEA92574E088EE`，明确experimentalOnly=true/hostQualified=false/deployed=false。

宿主脚本的真实ES3离线执行矩阵、无全局JSON、已有工程拒绝、失败清理及证据检查器反例通过；仓库内部署/回滚隔离测试在允许Junction后通过。尚未启动AE，也未激活实验构建。安装的11二进制、29面板源和runtime selector仍与129/124冻结逐字一致。此前代理自动测试的[备选方案](engineering/mfr-host-qualification-20261116.md)已由Owner手工清单取代；实际资源族与完整精度资格仍开放，不把PNG smoke当作全部MFR需求完成。

### 前一阶段130准备记录

Owner随后报告AE多帧渲染警告。安装129/124确实不声明MFR，主效果及8种隐藏节点均如此。130源码已将发射历史缓存改为请求自有副本、按版本发布，移除跨历史宿主回调持有的mutex；此为MFR前置准备，不是已支持MFR。GPU路径仍有跨宿主调用持锁；相机/灯光/Motion/贴图等AEGP资源路径的并发契约与实际AE2023资格尚未完成，不能仅改标志隐藏提示。[准入矩阵与下一步边界](engineering/mfr-readiness-20261116.md)、[ADR0050](adr/0050-multiframe-rendering.md)。

前置候选 `artifacts/prepared/mfr01-native130-panel124-prerequisite-v1`，manifest SHA256 `379E2848112212DD2391EBCF8F33AF1F3C7D0C16FAC682E058B755B085162ABE`，明确mfrEnabled=false/deployed=false。旧源码被新增并发场景拒绝；修后缓存131检查含四线程800乱序帧的积分/出生时间精确对照、/analyze /W4 /WX和ASan通过；实际原生绑定ASan48,165/Camera12通过，May2023完整SDK130 NoPublish/RenderJournal构建通过。初次PiPL9020与成功构建日志均保留。证据 `artifacts/mfr-01-20261116/`；没有AE实测，不据此宣称MFR实现完成。

本轮没有部署、启动/停止AE或修改宿主设置。安装的11二进制及29面板文件仍与冻结129/124逐字相同，runtime selector保持。原生130构建产物只是未部署前置候选，不能作为消除MFR警告的交付。下面仍为当前实际安装记录。

## 上次安装 — STB-20 129/124

2026-11-16T12:06:32.6091005+08:00 freshAE0，经既有Deploy-TestBuild.ps1 -KeepNative部署129/124。冻结v2 `artifacts/prepared/stb20-native129-panel124-v2`，manifest SHA256 `F0B7DD59E41EFEBAEAA1AF09A0A35826C7686553CB8638C9AB268505FEF1EEE3`。11个原生129/Core二进制、ABI9、runtime selector和单一Starfield Junction逐字保持；29面板源与129/123配对回退核验。

Owner打开工程截图报host_script_failed / JSON is undefined。离线将脚本上下文的全局JSON移除，旧网关在生成错误回复时直接抛异常；外层加载器catch也依赖同一个全局JSON，因而无法正常返回原错。panel124让gateway拥有私有ES3纯数据JSON编解码，Model/预设文件传输共享该私有对象；两个页面生成的AE加载失败回复自带字符串转义，不再依赖网关是否已加载。既不读取/创建/覆盖宿主全局JSON，也不修改其他扩展或以吞错/新增重试隐藏异常。[契约](adr/0046-aegp-webview2-panel.md)。原生渲染和上次UX-16修正保留。

最终冻结74组面板套件中73通过，唯一未通过为旧CEP退役模拟Junction套件的沙箱EPERM，未标为通过。新增host_json_startup_tests在缺失/不可信全局JSON、实际网关及两个页面生成脚本下通过6,152项检查：首次/缓存调用、真实加载错误、纯数据随机往返、严格语法/深度/循环/原型键拒绝、Unicode经过UTF-8仍完整。Model事务2,636项、预设文件583项在JSON缺失上下文通过；3个完整JSX以ES3解析并通过旧引擎运算符检查。没有重建native/Core或补记AE宿主通过。

首轮两个失败来自仍拦截旧全局序列化器的诊断/十进制模拟夹具，已将同一故障注入转到实际私有codec，原失败保留。冻结v1另被真实反例检出孤立UTF-16代理码经过UTF-8时变成替代字符；v2统一转义代理码并新增UTF-8往返检查，v1与失败证据均保留。

证据 `artifacts/stb-20-20261116/`：`before-reproduction.log`、`unicode-counterexample.json`、`panel-stage-results.json`、`panel-frozen-v2.log`、`panel-frozen-v2-results.json`、`deployment-verification.json`、`restore-report.log`。最终`final-audit.json`核对1,132次哈希、0不符，222个原生输入和11个二进制与前配对相同；Restore最终只读预检通过。先报告后保留最近五次实际部署备份，仅裁剪旧PERF-16备份，释放17,767,046字节，证据artifacts/rollback-retention/20261116-120800190，冻结/SDK/失败证据保留。未启动/停止AE、操作工程或改注册表/缓存；真实AE冷启动打开工程、图面板/预设/Model工作流仍待Owner复测。

一步回退129/123（AE关闭，从checkout执行；省略-Restore仅报告，候选保留）：

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'stb20-native129-panel124-json-startup-20261116' -Restore
```

# 上次部署记录 — UX-16 native129/panel123 — 2026-11-16

2026-11-16T11:07:30.2555505+08:00 freshAE0，经既有Deploy-TestBuild.ps1完整部署129/123，单一Starfield Junction保持。冻结 `artifacts/prepared/ux16-native129-panel123-v1`，manifest SHA256 `7AF55B594D627CED950407880AE0B8709DF448C99563608B751BDEA0C4AF9D3F`。11个二进制、29个面板源、Core selector及128/122完整回退核验；实际AE验证仍开放。

Owner最初截图显示旧四节点画布随后变成真实三节点。128/122画布修正已于同日10:40:29先行部署；Owner进一步明确合成预览也短暂呈现旧效果再被覆盖，因此该阶段不能视为问题闭环。原生129将新/重置效果的Control Source默认设为Node Graph，旧工程缺失参数仍填AE Controls，存盘值不批量迁移。首次ready0/revision0/guard0且ARB精确匹配出厂模板时，使用与初始原生节点一致的三节点数值投影；不先采样旧AE Controls链。首次空图初始化沿用同一图内UUID，防止随机粒子分布跳变。已有作者记录/非默认图绝不作为出厂图替换，guard/UI epoch及正常严格绑定路径保留。面板成功初始化后直接采用确认图，丢弃初始化前的旧模式/布局/帧数据。[契约与迁移边界](adr/0049-new-effect-initial-render.md)。公开ID、图序列编码、Core ABI9和Render.hpp不改；main参数清单元数据为31。

最终离线：实际native编译与启动投影在3高度×3粒子上限×6时间点的54个组合中，编码粒子身份/位置/外观逐字节相同。原生普通及ASan各48,165检查、Camera各12检查通过；adapter6,264检查通过；GraphParameter.cpp/Parameters.cpp以/analyze /W4 /WX通过；May2023完整SDK构建成功。Core源码未因本任务改动，但完整重建的DLL哈希变化，实际旧/新ABI9 DLL另做36组CPU场景，区域/步长/格式/像素精确一致；不视为GPU或AE资格。

冻结面板73组中72通过；唯一未通过为旧CEP退役的模拟Junction套件，首次被沙箱EPERM阻止，授权重放的PowerShell子进程又返回-1且无输出，未标记为通过。新默认、初次画布、首次固定身份、按需读取/编辑等面板回归通过。原生初次回归暴露旧fake checkout未返回保存的revision；夹具已补真实状态，早期17失败记录保留。另补出厂ARB精确匹配保护，避免仅凭空receipt替代作者图。SDK初次PiPL9020日志及后续成功构建均保留。

证据 `artifacts/ux-16-20261116/`：`native-deployment-verification.json`、`native-restore-report.log`、`panel-frozen-native-results.json`、`native-startup-parity-v2.log`、`native-startup-asan.log`、`adapter-final.log`、`analyze.log`、`sdk129-build-final.log`、`actual-core-comparison.log`。未启动/停止AE、操作工程或修改注册表/缓存/其他扩展。实际AE新增效果时首帧与初始化后的连续性、Undo/Redo、重置及保存重开仍需Owner复测，不能用离线检查关闭现场门。

完整撤回本任务两次部署到原128/121（AE关闭，从checkout执行；默认仅报告，`-Restore`才执行）：

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File artifacts/ux-16-20261116/rollback.ps1 -Restore
```

单次回退129/123到128/122也可用Restore-TestBuild.ps1与备份 `ux16-native129-panel123-initial-render-20261116`。最终 `final-audit.json` 核验1,100次哈希、0不符，222个原生构建输入与冻结/当前源码一致，完整双步回退链只读预检通过。最近五次实际部署备份按既有授权先报告再裁剪；仅移除旧UX-15/UX-14实际部署备份，释放35,524,259字节。候选/SDK/证据不裁剪，证据 `artifacts/rollback-retention/20261116-111018880` 及本任务retention日志。

# 上次部署记录 — PERF-17 native128/panel121 — 2026-11-11

最终2026-11-11T10:25:30.7630887+08:00 freshAE0，经既有Deploy-TestBuild.ps1 -KeepNative / Starfield Junction部署128/121，冻结v2 `artifacts/prepared/perf17-native128-panel121-v2`，manifest SHA256 `70C8E7660E4C0DC1886442C4F5B1E4C75BB6D4D070476B0A75178BAFB8F90F77`。11个native128/Core二进制、ABI9与runtime selector逐字保持；29面板源已核对。未启动/停止AE、操作工程或修改宿主设置。

图面板显示已初始化图时仅取canvasOnly身份/位置/连接/摘要，不读取完整参数、曲线、模型资源或图层库存，也不序列化默认参数图。显示数据没有graphHex/authoring recordStamp，序列化/GraphEdits/GraphTransactions/EditQueue都拒绝其提交；布局helper保留只读标记。单击AE参数及纯布局不全读；结构意图共用一次按需完整作者读取，目标/scope/epoch/失败/pagehide和活动手势仍受控，拿到完整快照才使用原有事务。首次结构编辑仍可能等待作者全读；未初始化/legacy图保留既有初始化路径。原生删除/绑定/guard、Core与schema不变。[读取契约](engineering/canvas-read-20261111.md)。

最终v2冻结72组面板套件零失败，版本合同通过；731次安装/冻结/阶段源码/回退链哈希核验，0不符。实际JSX夹具8粒子、各3条64点曲线：节点值读取3552→112，约31.7倍调用量缩减，写入/Undo/编译0；这不是AE耗时或速度倍数。实际panel覆盖轻首屏、单击与布局零全读、多意图一次完整读取、原参数保留、忙碌准入、失败/异目标/pagehide/活动手势。首轮71通过+旧CEP回退测试因AE运行受阻；Owner关闭后该项补跑通过。随后实际反例发现只读分支绕过busy准入，v2已修并重跑全72；v1与失败证据保留。

工作源码在AE运行期间保留120令牌，121候选独立冻结；freshAE0后才切换版本。两次实际安装均KeepNative，保留v2→v1→120完整配对回退链并验证中间哈希相符；工具默认报告，实际Restore前各自检查源/宿主。证据 `artifacts/perf-17-20261111/`：`panel-frozen-v2.log`、`panel-results-v2.json`、`busy-admission-counterexample.log`、`deployment-verification-v2.json`、`final-audit-v2.json`、`rollback-chain-report.log`。报告后只保留最近五次实际部署备份，裁剪证据 `artifacts/rollback-retention/20261111-102651437`；冻结/失败证据/SDK不裁剪。

Owner仍需实际AE验证画布加载、连续编辑/真实延迟/Undo/save/reopen，不从离线检查宣称宿主资格或Stardust速度。一步回退到128/120（AE关闭，从checkout执行；默认仅报告，自动执行两个已核验配对步骤）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/perf-17-20261111/rollback.ps1 -Restore
```

# 上次部署记录 — PERF-16 native128/panel120 — 2026-11-09

2026-11-09T17:01:35.9135632+08:00 freshAE0，经既有Deploy-TestBuild.ps1 -KeepNative / Starfield Junction部署panel120。native128/Core的11个二进制、ABI9与runtime selector逐字保持；29个面板源及完整128/119回退核验。冻结 `artifacts/prepared/perf16-native128-panel120-v1`，manifest SHA256 `1C46F333C25784F145692159D804F5828023FF6221BFA9FC3D6FA39BDAEE26C3`。未启动/停止AE、操作工程、改变AE选择/焦点或修改宿主设置。

选择发现改为200ms完成式轻探测，只返回目标token/scope，不读取renderer、图回执、作者参数或帧状态；优先selectedLayers数组而不是枚举全合成。稳定门120ms，通常第二次同一选择才完整加载，短暂选择合并。最多一个探测/状态/完整读在途，忙时不额外退2秒；focus只唤醒探测。稳定目标的状态2秒/完整审计15秒保留，采纳已读panelStamp避免首次状态重复全读。选择后的原生诊断延后800ms，新选择取消旧诊断；结构编辑即时诊断保持。[边界与证据](engineering/selection-performance-20261109.md)。

70组冻结面板套件全部通过，版本合同通过；最终686次哈希比较、0不符。实际JSX在5万图层夹具中不枚举comp.layer、不读renderer/作者值；实际panel用例中B/C/D连续选择只全读D，另外E慢读验证不重叠和200ms检查。总用例14次轻探测、1次普通状态、2次完整读、1次诊断、最多1个在途；这些是离线计数，不是AE毫秒或无卡顿保证。普通脚本源路径未发现主动SetFocus，不能据此否认Owner的阻断体感；真实主线程耗时与交互改善仍需Owner确认。

证据 `artifacts/perf-16-20261109/` 下 `panel-frozen-v1.log`、`panel-results-v1.json`、`deployment-verification.json`、`final-audit.json`、`restore-report.log`；PERF16首次回归的旧定时器夹具不符记录仍保留，更新为选择实际poll回调，未绕过产品保护。报告后按既有政策保留最近五次实际部署备份；裁剪证据 `artifacts/rollback-retention/20261109-170206260`，冻结/失败证据/SDK不裁剪。此前AE混合编辑/延迟/Undo/save/reopen门仍开放。

一步回退128/119（AE关闭，从checkout执行；默认仅报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/perf-16-20261109/rollback.ps1 -Restore
```

# 上次部署记录 — UX-15 native128/panel119 — 2026-11-09

2026-11-09T15:45:02.9118869+08:00 freshAE0，经既有Deploy-TestBuild.ps1 -KeepNative / Starfield Junction部署panel119。11个native128/Core二进制、ABI9和runtime selector逐字保持；29面板源与完整128/118回退均核验。冻结 `artifacts/prepared/ux15-native128-panel119-v2`，manifest SHA256 `479209B0969974946F971EDE5B3EA32B5EB9CD562F452F714A59724FCC2CC58F`。未启动/停止AE、操作工程或修改宿主设置。

普通no_target/no_project/no_active_comp/no_effect不再弹错误或连接退避提示；空画布显示固定22px居中提示，不随平移/缩放移动，隐藏小地图。没有有效选择时，只读复查仍有效的上一画布目标。自动刷新决定是否跟随新选择，默认每次面板加载勾选且不写工程/偏好；关闭后焦点读取仍绑定旧画布，手动Refresh可切换。晚到的新选择/不同工程响应不能越过暂停。其余调用明确绑定画布target token，保持native UUID/revision/CRC/projectScope/guard/提交和删除保护。[边界说明](engineering/canvas-target-20261109.md)。

最终冻结69组面板套件全部通过；版本合同通过。安装/冻结/回退最终684次哈希比较、0不符。真实反例：v1在取消自动刷新后，晚到的不同工程响应若复用AE ID仍会切换；已补工程scope判断并冻结v2，失败日志、旧冻结保留。69组覆盖实际panel+JSX fake入口、目标保持/删除、暂停/手动切换/默认值、晚回执、scope变化、空状态与真实错误区分以及此前图编辑回归；均非实际AE资格。

证据 `artifacts/ux-15-20261109/` 下 `panel-frozen-v2.log`、`panel-results-v2.json`、`late-scope-counterexample.log`、`deployment-verification.json`、`final-audit.json`、`restore-report.log`。Owner仍需真实AE检查无选择/自动刷新/空提示、此前混合编辑/延迟/Undo/save/reopen。报告后按既有政策只保留最近五次实际部署备份；裁剪证据 `artifacts/rollback-retention/20261109-154556593`，冻结/失败证据/SDK不裁剪。

一步回退128/118（AE关闭，从checkout执行；默认仅报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/ux-15-20261109/rollback.ps1 -Restore
```

# 上次部署记录 — UX-14 native128/panel118 — 2026-11-09

2026-11-09T14:36:03.0922509+08:00 fresh AE0，按常设授权经既有 Deploy-TestBuild.ps1 -KeepNative / Starfield Junction 发布 panel118。native128、11个二进制、Core ABI9及 runtime selector 均与128/117逐字一致；29个面板源、冻结及完整128/117回退哈希已验证。冻结 `artifacts/prepared/ux14-native128-panel118-v1`，manifest SHA256 `DD3356ADE9CBF7FF999629BFEB51258A4822A1C2FFE4DD4E0FA941A3F2F94760`。未启动/停止AE、操作工程或修改宿主设置。

诊断不再向正常上游或Output扩散：原生拒绝边仅标记来源节点，显示红色错误及红色描边；Model粒子缺少可用Model时显示黄色SF-W101警告，沿用既有默认立方体。详情改成可移入/选字/滚动的暗色浮窗，支持键盘与Escape，普通重绘不闪退。只读本地图派生，无新增宿主通信，不改提交、绑定、删除、执行投影、参数ID或schema。[实现边界](engineering/node-diagnostics-20261109.md)。

冻结面板67组套件全部通过，版本合同检查通过。覆盖Auxiliary上下游定位、红/黄级别、缺模型与多层Material几何、循环/错误端口、旧回执/目标变化、浮窗安全文本/边界/关闭/重绘和无新增host调用。早期全量测试因AE仍运行触发旧CEP回退测试保护，关闭AE后冻结重跑通过；抽取adoptState的旧夹具已补齐只读诊断依赖和目标切换清理断言。不是原生渲染或实际AE资格测试；Owner需验证AE内红框、悬停和实际默认立方体效果，之前128/117混合编辑/延迟/Undo/save/reopen门仍开放。

证据 `artifacts/ux-14-20261109/`：`panel-frozen-v1.log`、`panel-results-v1.json`、`deployment-verification.json`、`restore-report.log`、`final-audit.json`。按既有政策报告后保留最近五次实际部署备份，裁剪证据 `artifacts/rollback-retention/20261109-143634063`；冻结、失败证据和SDK不裁剪。

一步回退128/117（AE关闭，从checkout执行；默认只报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/ux-14-20261109/rollback.ps1 -Restore
```

# 上次部署记录 — UX-13 native128/panel117 — 2026-11-04

2026-11-04T23:23:08.6780823+08:00 fresh AE0，经既有 Deploy-TestBuild.ps1 / Starfield Junction 部署完整128/117与新Core DLL（公开C ABI9不变）。冻结 `artifacts/prepared/ux13-native128-panel117-v5`，manifest SHA256 `C37D7D1C02CC6AAB92AD26BB58A79D17F1D589905FEF1BFD678A883DCF6CF8E9`。11个二进制、29个面板源、Core selector均已按冻结hash验证；127/116完整回退保留，27个既有文件和2个新文件的回退归属经实际报告验证。未启动/关闭AE、操作工程或修改宿主设置。

实现删除/晃动摘出后的上下游桥接、自由作者连线与黄色节点警告、只写变化连接行的native delta、一个宿主写入的本地预览/合批队列、独立安静期布局保存、稳定SF错误码和折叠详情。保持单击AE参数编辑与127旧绑定交接/删除后刷新保护；公开参数ID/schema/Render.hpp不改。错误连线保留作者数据，Core只执行共用生产规划器验证的安全子集；不新增Auxiliary直接输出外观语义。边界/容量/回退草稿兼容性见[UX-13说明](engineering/editor-topology-20261104.md)。

独立实现v1/v2的真实反例、修复、旧冻结均保留：传输payload、未知回执后拖动解锁、Clear容量的物理删除前检查、新节点布局成员、拖动期间ACK，以及尺寸重放默认值。额外混合压力找出普通复制不必要的资源屏障，已只对实际带资源Model复制保留。v3全实现独立审校PASS；部署前清单条目遗漏被真实合同检查拦截，未替换宿主文件，修后v5交付审校PASS。v4审校中的路径分隔符指控已由审校者撤回（JSON转义误读），不是产品缺陷或实际修复。

最终66组面板套件零失败；Core投影普通/ASan7136、native编辑ASan336338、实际publisher47859+Camera12、Host/Clear分析4508、WebHost ASan771、router ASan46。实际AEX离线诊断43、源能力88、原生selector240；均使用fake SDK/上下文，不是AE宿主资格。三组独立虚拟种子在333/500ms输入间隔下各360个可准入操作，109/162个宿主批次，最多1个写入、无丢意图或准入/服务错误。纯Core最大63作者/252边普通检查约33ms，仅本机离线数值，不是AE准备时间。

Owner已确认此前旧工程启动问题经显式修复解决。当前128/117仍需Owner真实AE2023测试连续混合编辑、ECW删除、Undo/Redo、警告/保存重开及实际延迟；不宣称已达到Stardust或保证AE每秒2–3次无崩溃。Native请求有界等待30秒，避免误用8秒脚本等待判定Owner报告的5–10秒原生调用；未知结果不自动重试/Undo。

最终证据：`artifacts/ux-13-20261104/`下 `deployment-verification.json`、`final-audit.json`、`panel-final-v5.log`、`implementation-review-v3/review.md`、`implementation-review-v5/review.md`。按Owner政策报告后仅保留最近五次实际部署备份，其他冻结/失败证据/SDK不裁剪。

一步回退127/116（AE关闭，从checkout执行；错误作者草稿应先撤销/修好再降级；默认仅报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/ux-13-20261104/rollback.ps1 -Restore
```

# 上次部署记录 — 127/116原生编辑性能优化 — 2026-11-04

## Owner后续反馈：启动127时存档残留Emitter绑定（2026-11-04，后续已确认显式修复解决）

Owner用原testproject.aep启动报guard0/stream197/516，且未用127保存。只读原文件与journal精确匹配：Comp5/层111，存档Core图中的Output祖先Emitter7aa19e9f985249f2be5d8b1d2e2f319e占slot99/stream197，效果列表已无该源；disk1099表达式仍按其名称/完整UUID读取。原文件mtime2026-10-31，读取前后hash不变。当前直接原因是存档图/作者不一致，起初丢源的操作未证实，不能据此扩大127宿主资格或宣称所有127路径无回归。

现有Diagnostics > Repair Selected Graph可显式备份整层后按现存作者重建；不恢复已丢Emitter动画，不在render中自动修图。生产协调器/analyze探针24530/0；旧仓库fixture因缺126末参LNK2019，失败保留，仅artifacts内补stub重跑，产品/仓库测试/安装未变。未操作AE或工程，AE34968仍运行，Owner修复/保存/重开待验。证据artifacts/stb-20-20261104/{findings.md,failure-graphs.json,native-instance-summary.json,stream197-saved-chunks.json,repair-probe.log,read-only-verification.json}。

PERF12保留Owner已确认修复的126删除生命周期、单击编辑、Core/公开ID/schema/ABI。非目标Emitter/Particle/Force/Turbulence的数值参数，在同帧/geometry/PAR/项目/身份/epoch和SDK状态证明下复用；当前目标仍完整读，连接/身份/guard/资源、Core验证、原子完整快照及正ACK保持。能力不支持时静默回完整编译，不按单一旧revision猜作者未变。Particle旧新两贴图引用必须都0才复用。实现边界见[实施说明](engineering/native-edit-performance-implementation-20261101.md)及ADR0048。

删除最终依赖刷新保持126全部live/退休集合，交替禁用无源标记减少重复写入，并精确读回文本/状态/handoff，不按零值跳过槽。3作者79绑定生产夹具：重复Particle修改标量读241→196（另2次源generic查询，目标仍92）；最终force refresh表达式写1024→591、状态写591→158、数值写512→79，表达式读945→1378。调用量不等于AE耗时，不宣称5–10秒或Stardust目标已验收。删除先pending绘制，新增queue/捕获/发布/物理删除/最终刷新及query/full-node/compile/service计量，保留既有ACK/展示快照。

v2实际8AEX反例找出内部enum与图kind不同造成Force/Turbulence证明unsupported，以及设计门7缺分项计量；已显式逐项映射并补长期actual-selector/opaque SDK比较/故障原因与延迟计时测试，原失败源/AEX/日志保留。v3冻结和独立实验完成但报告未闭合；全新v4重新审阅/重编重放并最终PASS，2868比较零差异，当前/冻结712输入、SDK219输入、69Core源及DLL对126、prepared全包一致。v4自身实际8AEX64/88检查和111显式反例/ASan fixture48646+Camera12均零失败，实际selector228调用；不把v3的实验当其原创。v3独立panel52/57另5为隔离前置失败；父实际最终交付冻结panel57/57零失败，两者分别保留。

父最终publisher ASan48007+Camera12、Host+Clear分析4508、nativeDelete ASan160255、WebHost ASan768、Exclusion1285、完整效果备份ASan717555、隔离Adapter6251、Shape81以及最终May2023 SDK127构建通过。Core69源码及CoreDLL与126逐字一致。未启动/操作AE或工程，真实AE2023 generic PF context/状态语义、marker依赖失效、Undo/save/reopen及端到端性能仍为Owner门；编译/模拟suite不是宿主资格。

2026-11-04T11:53:49.8104571+08:00 freshAE0，经既有Deploy-TestBuild.ps1/Starfield Junction部署127/116，冻结artifacts/prepared/perf12-native127-panel116-incremental-v3（219adapter/69Core/3build/241validation/11native-Core/27panel）。最终1901哈希核验当前/冻结/安装/审校输入及126/115完整回退；Core selector不变。报告后只裁剪旧stb16实际部署备份，保留最近五次；全部候选/独立失败证据/SDK保留。预部署47条127 journal完整字节归档到本次predeployment-native127-journal.jsonl，明确排除AE性能资格，避免混入之后现场日志。

证据：artifacts/perf-12-20261031/{deployment-verification.json,final-audit.json,validation-summary.json,native-budget.json,reviewer-v4/review.md,journal-archive.json}。

一步回退126/115（AE关闭，从checkout执行；默认仅报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/perf-12-20261031/rollback.ps1 -Restore
```

# 当前状态 — 126/115编辑性能优化已部署，原生126保持 — 2026-10-31

## PERF-11：原生增量与删除延迟分析完成，产品未变

Owner反馈126/115删除未见错误但需5–10秒，希望按节点/参数增量提交。本轮只读分析/生产代码SDK夹具计数，未操作AE/工程、未部署；38个native/panel文件与已部署冻结配对一致。NativeEdit入口本已携带UUID/参数/value，但后端全读作者再构图。3作者79绑定夹具仅改Particle Size：GetNewStreamValue241次，含无关Emitter/Force119次，SetExpression0；最终force绑定刷新却有1024次SetExpression，因为512bank默认未用零/空/禁用槽也被归为退休槽。夹具433已知unused槽反事实排除后为158次；保留一个有限空槽为160次，独立0值控制也160次，证明不能按0判断未用。上述是调用量，不是现场AE耗时/全Delete预算，不能直接套到Owner工程或宣称提速比例。

父完整生产对象重编/最终探针2844检查0失败，新独立审校重放5case一致、另零值控制通过、67输入hash一致；结论PASS_ANALYSIS_ONLY/implementationReady=false。精确历史/当前/此次退休槽归属、inflight旧图保护和作者缓存新鲜度仍需可编码合同；保留126保护，不直接跳过零槽。现有journal只有完成记录，不能分解5–10秒；删除等回执和完整snapshot才更新画布，UI反馈也是独立优化点。下一轮优先解决bank误处理、原生目标节点/字段重建、同操作阶段计量和及时pending反馈。说明见[原生编辑性能分析](engineering/native-edit-performance-20261031.md)，证据artifacts/perf-11-20261031/{native-budget.json,baseline-verification.json,reviewer-v1/review.md}。原始失败编译日志与现场累计日志完整保留，不替代真实AE资格。

## PERF-10：布局元数据与重复确认优化

Owner于2026-10-31确认已部署126/114的删除故障已修复，要求保持此基线优化编辑性能；未新增具体AE build/Undo/重开覆盖清单，不扩大现场资格。

115面板把纯布局从完整图事务拆出：本地按帧绘制，停手600ms合并变化坐标写入既有layout参数，连续手势暂停自动保存；不编译/发布/复制效果/传Model资源，也不为坐标回执重读全图。保留path/target/UUID/空闲guard/常量字段/CAS和局部补偿。结构编辑采用已有成功快照，不立即重复getPanelState；成功快照播种pulse。时间变化独立更新帧状态并使下次manifest规划过期；Graph模式完整读取绕过退休legacy参数/曲线并合并frameStatus。native126删除只等待布局队列、仍保留原正回执及一次展示快照；单击编辑保持。

多轮独立反例曾发现Undo顺序、同hex重投影、部分失败/未知基线、大坐标容差及在途ACK越过手势暂停，均保留原证据并补修。最终全新v6冻结41唯一输入，13项入口/独立反例重放均exit0，当前/冻结hash一致；父最终实际冻结交付副本56套零失败。非阻断限制：未知布局ACK恰在中键平移期间返回，结束不立即读取，明确错误后可手动Refresh、焦点刷新或周期审计恢复；未观测自动重写，其他非节点手势不扩充资格。无AE操作/计时，不据VM或编译宣称Stardust性能对齐。相同2作者节点生产DOM对照完整state标量读取259→164（约-36.7%），作者139读不变，属于调用量而非AE毫秒。

2026-10-31T10:59:05.5039750+08:00 freshAE0，经既有Deploy-TestBuild.ps1 -KeepNative/原Starfield Junction登记部署126/115；11原生/Core文件逐字复用126，Core69源不变，无新SDK构建。最终候选artifacts/prepared/perf10-native126-panel115-editor-metadata-v3冻结216adapter/69Core/3build/238validation/11binary/27panel；最终1217hash核验当前/冻结/安装、41审校输入及126/114完整回退。先报告后仅裁剪旧stb15实际部署备份，保留最近五次及全部候选/反例/SDK。

证据：artifacts/perf-10-20261031/{deployment-verification.json,final-audit.json,frozen-v3-final.log,read-budget.json,reviewer-v6/review.md}；设计与必要/重复确认分类见[编辑性能说明](engineering/editor-performance-20261031.md)。真实AE输入到画面耗时、Undo/重开继续由Owner验证。

一步回退126/114（AE关闭，从checkout执行）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/perf-10-20261031/rollback.ps1 -Restore
```

## STB-19：系统分析与Clear生命周期126/114已部署

现场125的Clear gate3/root/controlFocus0后188ms出现guard0缺Force516，后续普通发布读旧active handoff又失败，日志不证明全部入口。实际125发布对象受控反例复现同类旧handoff516；原七类删除夹具遗漏Force->Output、实际停放Force，现补正确连接及断言。不能继续把它的旧通过当现场拓扑覆盖。

126/114候选让AE自己决定实际Clear目标：操作前捕获完整旧active slot/UUID有限值及base，在同一同步Undo/epoch内先保护renderer，释放引用后递归一次AE Clear，稳定ID/path重取；只对census证实的selected UUID消失收敛连接并一次发布。旧代交接用本操作前有效值，新源仍严格验证；no-op不猜删残留选择，失败保留自有guard并按阶段报告。观察式参数/关键帧选择不替代AE行为。单击编辑保持，Core/公开ID/schema/ABI不变。

父最终native分析/ASan160255、实际Host+执行器分析/ASan4508、既有Host768、实际publisher普通/ASan27084+Camera12、55panel114及完整SDK126通过。独立设计审校的两项要求已实施；全新实现审校冻结166唯一输入，实际Host+执行器158独立断言、实际126 AEX准入19断言及冻结palette通过，当前/冻结166hash一致，无新增可复现阻断。独立额外断言与父基础计数不累加。审校工件正反斜杠去重曾保留旧panelhash，已核对副本字节相同并纠正清单，历史说明保留。

2026-10-30T23:39:43.4275535+08:00 freshAE0，经既有Deploy-TestBuild/原Starfield Junction部署126/114，冻结216adapter/69Core/3build/237validation/11native-Core/26panel，最终1338hash核验安装/当前/冻结/166审校输入及125/113完整回退。Core69源和DLL与125一致，单击编辑保持。报告后仅裁剪旧stb14实际部署备份，保留最近五次及全部候选/证据。无AE/工程操作，真实递归Clear、嵌套Undo/重开、第三方hook及性能门保持开放，不能据离线结果宣称现场已修复。证据artifacts/stb-19-20261030/analysis.md、deployment-verification.json、final-audit.json及reviewer-v1/review.md。

一步回退125/113（AE关闭）：powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-19-20261030/rollback.ps1 -Restore。

## STB-18：Owner恢复单击编辑

Owner确认125/112删除仍报错，并要求恢复单击节点打开AE参数。125/113已恢复普通单击，保留Enter、修饰键多选/拖动/pending抑制及最新编辑合并。第二次click detail>1在取消旧意图前返回，避免A在途时B双击丢失首次点击的排队编辑。最终55套通过，全新独立实际冻结panel33断言/palette通过、8输入hash一致，无新可复现阻断。原生125/Core十一文件与125112逐字相同，未构建新SDK，不把UI恢复当删除修复。

2026-10-30T22:36:48.4953783+08:00 freshAE0，经既有Deploy-TestBuild/Starfield Junction部署125/113，1174哈希验证当前/冻结/安装/审校输入及125112一致回退。报告后只裁剪旧stb13实际备份，保留最近五次，候选及证据不动。重开面板加载113即可；没有启动或操作AE。证据artifacts/stb-18-20261030；一步回退（AE关闭）：powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-18-20261030/rollback.ps1 -Restore。

只读冻结既有125journal至artifacts/stb-18-20261030/owner-native125-journal.jsonl。现场PID32220的Clear仍有gate3/controlFocus0，随后guard0缺Force8b272c/stream167/516及自动同步finite handoff拒绝。日志不包含新增量执行器每个阶段，不能从缺少该类记录断言所有入口执行情况。原始删除故障仍开放；不要求Owner反复复现，不操作AE/工程或自动清guard。

## STB-17：原生增量删除125/112已部署

只读参考八AEX并保留脱敏PE/共有suite候选引用证据；不能证明Stardust无验证。实际生产备份代码以现有假May2023 SDK计数，32节点仅prepare+discard即22492标量读取/5016效果查询、复制33效果；尚不含图编译/渲染，不能当AE耗时。改造前删除统一走Model全事务、任意源删除关闭全批别名快路的成本已从源码确认。报告与复现命令见artifacts/stb-17-20261029/reference-static/analysis.md。

新独立设计审校否决自动Undo回滚和Clear可覆盖全部删除的初稿承诺；ADR0048收敛为原生增量编辑/发布边界、局部补偿与准确收尾状态，外部删除生命周期单独闭环。面板删除走小增量、源保留到一次正ACK、不复制整图效果、不走Model会话；ECW正向窗口门共享执行器，模糊归属只capture/drop，原生Edit/RichEdit/ComboBox及祖先焦点的Clear在capture/latch前留给文本编辑。已调用generic后的失败保守留guard，提交后的失败报告committed/cleanup，未知回执不重试；其余编辑/绑定失效规则/Core保持现有实现。

父最终原生删除分析/ASan各116366、Host分析/ASan各768、55panel112套、既有备份/Model/真实publisher回归及最终May2023 SDK125构建通过。第一、二位审校的真实反例保留并修正；v3探针通过但额度中断未形成最终报告，不作最终资格。第四位全新独立冻结21输入，执行器基础156634及独立53断言、实际Host路由18断言及冻结前端通过，当前21hash一致，无新可复现阻断；fake publisher/GUI不等于AE资格。

2026-10-30T15:38:19.4894422+08:00 freshAE0，按既有Deploy-TestBuild/Starfield Junction授权部署125/112。冻结215adapter/69Core/3build/235validation/11binary/26panel，最终1187哈希验证安装、冻结、21审校输入及124/111完整回退，Core DLL与124逐字相同。报告后只裁剪旧stb12实际部署备份，保留最新五次；所有候选和证据保留。证据artifacts/stb-17-20261029/deployment-verification.json、final-audit.json及reviewer-v4/review.md。

本批仅迁移普通删除；增加/连接/布局/复杂资源保持既有路径。没有启动或操作AE/工程；真实删除、连续编辑、Undo/重开和端到端准备耗时仍需Owner验收。混选、脚本/其他插件删除、自定义文本控件、unsaved全局generation仍非全面资格。已损坏图不静默修复，需现有显式Diagnostics或主动Undo恢复后再判断正常删除。

回退至124/111（AE关闭）：powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-17-20261029/rollback.ps1 -Restore

## STB-16：图选择与AE编辑分离

Owner明确批准单击只图选择、双击或Enter编辑AE。普通点击/多选无宿主请求，显式编辑仍沿原选择服务；新的图选择/拖动/框选/右键/删除取消未发送旧编辑意图。第一位审校实际真实panel复现A在途/B排队/C新选择后仍发B，修后交第二位全新审校65项及冻结palette通过，8输入hash一致，无新明显阻断；55panel111套零失败。旧失败与两轮审校保留。

2026-10-29T16:46:50.5748863+08:00 freshAE0经既有Deploy-TestBuild/Junction部署124/111，原生11文件逐字复用冻结124，不冒称新SDK编译。冻结213adapter/69Core/3build/232validation/11binary/26panel，最终1164哈希及124/110一致回退核验。当前安装124/111；实际图Delete、AE效果控件直接Delete及原始绑定故障仍未验收，不能把交互修正当所有删除问题已解决。

回退（AE关闭）：powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-16-20261029/rollback.ps1 -Restore

## STB-15：统一命令入口已抵达，原生root焦点未识别图owner

Owner123继续sync555085旧handoff拒绝。原始53行中8条graph-command来自父离线图提交fixture PID30276，需排除；现场PID7640为45行（40render/5command），明确记录Delete1/gate3、local=gui=foreground，约266ms后首render缺Force516。不能继续称“无Clear回调”：统一入口已生效，本次拒绝发生在native图键盘归属门。原始日志不改写，独立owner子集和来源分区存stb15。

124候选订阅WebView2公开controller Got/LostFocus，双订阅均成功才允许补足“native焦点正好是当前foreground root”的情况；保留有效GUI查询/同root/可见启用/非预设、显式其他native子窗仍拒绝。保存订阅owner COM身份及GotFocus时root，弱指针唯一逻辑owner防多面板误目标；Lost/关闭/进程失败撤销状态并丢pending，迟到/异线程/foreign sender不得授权。选择恢复用同谓词。保留全部epoch/输入/截止/互斥/去重/严格绑定/guard/Core，不按鼠标或旧点击猜测，不自动修旧图/清锁。graph_commit_host_tests显式NoJournal，防离线探针继续污染实际journal。

父最终WebHost /analyze /W4 /WX和ASan各749，router46/graph command56及55panel110套通过；旧726/1反例及中间744、fixture命名/非空告警日志保留，中间SDK不充作最终资格。最终May2023 SDK124 NoPublish/RenderJournal成功。全新独立实际生产Host分析792（含其冻结仓库基础，不与父749叠加），11/11当前/快照输入一致，覆盖实际订阅/部分失败/双view唯一owner/旧Lost不撤新owner/Clear正确view/root重parent/freshGot/worker/foreign/stopped/原ProcessFailed lambda/替换控制器及token回收等，无新增明显阻断。

2026-10-29T15:08:08.8353615+08:00 fresh AE0，按既有授权经Deploy-TestBuild/原Starfield Junction部署冻结stb15-native124-panel110-controller-focus（213adapter/69Core/3build/232validation/11binary/26panel）；最终1167哈希验证安装/当前/冻结/面板及123110一致回退。全部26panel、69Core源和Core DLL与123相同。Main A57A8B83F448ED7C0594135F0C1F591B0EE18B6238E8706668116640EC8ABE98，Host 98F0D046790BF40CAFBC481E669006174FAEBBA310F7D9C2DE27807CFFD2C72B。报告后仅裁剪旧stb10实际部署备份，保留最新五次，全部冻结候选和证据保留。

未操作AE/真实焦点/工程或清guard。123现场已证明Delete到达但gate3拒绝；124控制器Got/LostFocus的实际事件顺序、Force删除/后续编辑/Undo重开及准备性能仍需现场验收，不能宣称故障已解决。不以旧值替代缺失绑定，不静默修损坏图或解除未知锁。已分离现场PID7640的45行与父离线PID30276的8条旧记录，原始53行不改写；graph command fixture现显式NoJournal，重跑56项后首次部署124前诊断文件仍不存在。诊断I/O不用于性能资格。

AE关闭后从checkout一步恢复123/110（默认仅报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-15-20261029/rollback.ps1 -Restore
```

证据artifacts/stb-15-20261029；后文123及更早状态保留历史时点，不作当前现场通过结论。

## STB-14：122注册成功无Clear回调，完整初始化重复注册修正

Owner确认122仍再次按Delete后失败，trace只包含自动sync33357，缺失manifest节点后旧handoff拒绝。冻结81条122日志：77render/4graph-command，公开Clear21查找与registration0成功，但零Clear观察/forward/完整transaction。不能把122跨GUI反例当现场通过，也不把缺失回调直接当具体AE覆盖语义证明。

正式AEGP实现指南要求同一AEGP仅一个CommandHook；当前Web初始化先注册ALL及两菜单，Host再注册六内部handler。实际旧StarfieldHostEntry夹具（Web init stub）捕获六次注册，14项1失败；这证明调用数量，未模拟/冒称真实AE覆盖规则。123候选先定义八个会话命令ID，最后统一一次BeforeAE/ALL，按ID单点进入六现有handler或Web适配器的图/预设/Clear；未知/已处理/停止/worker不消费，注册失败不发布可执行bindings。Web初始化本身不再注册CommandHook。保留既有焦点/epoch/互斥/严格绑定/guard，无UI脚本或图写入新增。

修后全新独立审校实际entry/router /analyze /W4 /WX107（含46基础）、实际Web adapter730（含724基础）及10/10当前/快照输入一致，无新增明显阻断。覆盖最终ALL晚于定义、完整分发/拒绝不落入Web、无命令/注册suite、定义/enable/zeroID/最终注册失败、already/worker/stopped及实际adapter图/已有预设窗/Clear入口。父路由分析/ASan各46、WebHost分析/ASan各724、原graph command56/bootstrap14、55panel110套及完整May2023 SDK123 NoPublish/RenderJournal通过。实际旧entry14/1重复注册反例保留，不把计数当AE覆盖语义复现。

2026-10-29T14:39:27.8907338+08:00 fresh AE0按既有授权经Deploy-TestBuild/原Starfield Junction部署冻结stb14-native123-panel110-command-router（213adapter/69Core/3build/232validation/11binary/26panel）；最终1166哈希验证当前/冻结/安装与122110一致回退。全部26panel、69Core源和Core DLL与122相同。Main 23D5EBC9E745869CA98B9FF9F43D726F886599387AAFCF83C6E7870D4F3D855E，Host 1CCCDDC35E4671091CDE4E5DACB50A00B3CFD07C1D5F31E74EBA7D1E9F211209。报告优先裁剪一份旧stb08实际部署备份，保留最近五次，全部候选/原始证据保留。

未启动/停止AE、操作真实焦点、写工程或清guard。修正的是已查明的多CommandHook注册契约偏离；真实AE是否因此丢失Clear回调、Force删除/后续编辑/Undo重开及准备性能仍须现场验收。零旧值替代/未知锁自动解锁/损坏图静默重建，既有严格绑定和回滚不变；诊断I/O不用于性能资格。

AE关闭后从checkout一步恢复122/110（默认仅报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-14-20261029/rollback.ps1 -Restore
```

证据artifacts/stb-14-20261029；后文122及更早状态保留历史时点，不作现场通过结论。

## STB-13：121仍失败，跨GUI队列焦点缺口离线复现

Owner121新截图及110完整trace仍同Force8b272c/167/guard0/516。普通submitGraph257091已接受；随后manifest连接缺失节点导致sync289963在verify finite alias handoff拒绝。只读冻结198条121日志，Clear查找成功，零forward/零完整transaction事件；未据缺失记录断言真实物理删除路径。121现场门失败，前一轮离线结果不等同修复。

GetFocus只观察调用线程输入队列。原生图选择恢复与Clear门都依赖它；实际旧生产predicate在“本线程空/旧焦点，前台GUI队列图子窗持焦点”夹具705项1失败。局部改为GetGUIThreadInfo(0)且严格检查active/前台/root/真实HWND及无菜单/拖窗模式；选择返回后查询失败不得据此抢焦点。保留全部输入/页面/截止/互斥/双路径去重和严格别名/guard，不AttachThreadInput，不改系统设置或自动修旧图。增加有界plain命令门诊断，idle才写journal，记录注册返回和Clear所停门，不在钩子执行脚本/SDK写入或I/O。父修后Host分析/ASan各720、完整SDK122与新独立审校通过；panel110/Core保持不变。

修后全新独立审校/analyze /W4 /WX最终782项通过（含720基础；先前780不累加），精确旧predicate对照确认受控跨GUI漏判，9/9审校输入当前/快照一致，无新明显阻断。父Host分析/ASan各720、55套panel110零失败、完整May2023 SDK122 NoPublish/RenderJournal通过，前态705/1失败及所有审校证据保留。全部26panel文件、69Core源和Core DLL与121配对相同；不改严格别名/guard/ID/schema/ABI/数学。

2026-10-29T14:18:33.7541675+08:00 fresh AE0，按既有授权经Deploy-TestBuild及原Starfield Junction部署冻结stb13-native122-panel110-gui-focus（213adapter/69Core/3build/230validation/11binary/26panel）；最终1161哈希验证当前/冻结/安装及121110一致回退。Main 82BBFA8A6BA4F5A4E0BCE8D2BF48707DC38D1511A4BC8E223FBE9AE778FC86C6，Host 72E4E8A576FA60400FF7C086900CE3C555D48E35B7C7A49FB8C190A0896550FC。备份实际ID为stb13-native122-panel110-gui-focus-20261028（沿模板日期后缀，执行日期以上述收据为准）。报告后仅裁剪旧stb07实际部署备份，保留最近五次，所有冻结候选/证据保留。

未启动/停止AE、操作真实焦点或写工程/清guard。本轮修正有受控反例和Win32文档依据，但未证明现场WebView2/AE确属该队列布局，也不能宣称Force原故障已解决。Clear钩子次序/实际转交、快速按键/文本长按、原图恢复/后续编辑/Undo重开及准备性能继续开放。既有损坏图不自动改写，诊断journal含I/O成本不用于性能资格。

AE关闭后从checkout一步恢复121/110（默认只报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-13-20261029/rollback.ps1 -Restore
```

证据artifacts/stb-13-20261029；后文121及更早安装状态保留历史时点，不是当前现场通过结论。

## STB-12：120同步拒绝已定位，完整删除路径继续核查

Owner最新120截图仍报同Force8b272c/stream167/guard0/516。自动随错trace记录submitGraph881849已接受，随后因manifest连接指向不存在节点进入syncGraphSnapshot909342并被负nonce拒绝。只读保存77条120日志、零完整transaction事件；原生首拒绝明确为animation binding / verify finite alias handoff。不能把此前焦点保护、离线epoch测试或Host dispatch result0当Force已修复，120现场门失败。

STB12候选在正式AEGP_Command_ALL钩子接管仅图窗口真实焦点/前台、无修饰键的Delete/Backspace Clear，动态公开查找命令ID，绝不在钩子运行脚本/删效果。有界通知绑定页面/导航/输入epoch、原HWND/root/输入时间和短截止时间；busy/过期/后续输入丢弃，不延迟重放。前端仍沿deleteSelection完整事务，文本控件留浏览器本地编辑；其他AE窗口/预设/命令不改。第一位新审校发现Ctrl+D可能使旧通知误删新选择，局部失效规则修后交全新第二位审校。严格别名/guard/Core/参数schema不改，不自动修旧图或清锁，不操作AE/工程。

修后第二位全新审校真实Host /analyze /W4 /WX712（703基础+9新增）、独立消息/文本/真实panel handler与deleteSelection46项、9/9输入哈希一致，无新明显阻断。父Host分析/ASan各703、真实panel完整事务路由回归及最终55套panel110、完整May2023 SDK121 NoPublish/RenderJournal通过。升级时漏改preset_manager网关版本导致的2项失败已保留，修正配对后55套全部通过；不称该元数据修正由第二位独立审校执行。所有前态失败与审校证据保留。

2026-10-28T23:41:17.7778468+08:00 fresh AE0按既有授权经Deploy-TestBuild/现有Starfield Junction部署冻结stb12-native121-panel110-graph-clear（213adapter/69Core/3build/230validation/11native-Core/26panel）；最终1161哈希验证安装/冻结源/面板及120109一致回退。Core69源及DLL与120相同。Main A458AF51FE20B7A2AE4D8EA6C4F617B4BB98E607B02A4C9C352CD30A8B71B80A，Host 0DBC0FA90A6B86B25B3483C554E7E3439C2E343E1601D54F0CC065841FC3A29D。报告优先裁剪一份旧stb06实际部署备份，仅保留最新五次，所有冻结候选/证据保留。

未启动/停止AE、操作真实焦点、写工程或清guard。真实AE Clear钩子注册/次序、英文名称查找、Force删除/后续编辑/Undo重开及性能未验收，不能宣称原故障已修复。任意后续输入（包括快速松键）可能保守丢弃一次按键，需新按一次；文本收到原生通知后长按重复未资格。既有损坏图/未知锁仍需显式Diagnostics修复，不自动改写。诊断journal有I/O成本，不作为性能资格。

AE关闭后从checkout一步恢复120/109（默认只报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-12-20261028/rollback.ps1 -Restore
```

证据artifacts/stb-12-20261028；后文120及更早状态保留其历史时点，不是当前安装或现场通过结论。

## STB-11：119现场失败，120/109已部署

Owner在119继续报告Force8b272c/stream167/guard0/516，并给出node_effect_sync_failed / The effect rejected the graph transaction，确认按Delete后才出现。该具体banner来自syncGraphSnapshot的同步提交拒绝，并非完整Model事务结果；不据此断言此前没有其他请求。保存119日志267行（143成功、15条516、109取消），真实transaction事件0；不能用uiTransaction字段的字符串匹配计数。截图、SHA256和摘要存artifacts/stb-11-20261028。119现场门失败，不把此前离线epoch修正等同已解决Force故障。

当前候选明确标记节点选择请求evalScriptKeepGraphFocus，沿现有4字符串有界协议/FIFO/串号/epoch执行。仅当执行前graph HWND/子窗拥有焦点且所属顶层窗口为当前前台，在选择脚本返回或异常展开后仍是同一graph HWND、同前台根、同输入时间、未关闭/导航且窗口可见启用，才恢复graph HWND的键盘焦点，沿既有WM_SETFOCUS交还WebView2。普通脚本/预设/用户已转去ECW不恢复，不装全局键盘hook、不拦截AE Delete。选择后焦点转移导致原生AE删除仍是假设，未经真实AE证明，不能将受控焦点夹具当现场根因。

首次同步失败现在包含阶段及原有有界Last graph trace；未初始化快照保留有界repairReason，诊断String失败有兜底。busy尝试不追加/覆盖首错。显式RenderJournal版另记录同步graph-command的Host dispatch、SDK/ack/status和效果现成拒绝阶段，不改status/负nonce或回滚。dispatch result0只表示即将调用，不是提交成功；效果日志只接受当前graph-command消息前缀，避免误记旧render_msg。日志artifacts/graph-render-journal-120.jsonl；有诊断I/O成本，不用于性能资格。

第一位新审校实际654/1复现后台旧GetFocus错误恢复，第二位新审校实际665/1复现执行中reparent错误恢复，前后GA_ROOT与原HWND身份局部修后交第三位全新审校。最终独立focus699 /analyze /WX、panel/bridge metadata54、真实gateway802、journal3086/514并发记录通过，16/16当前/冻结输入一致，未发现此局部新明显问题。父最终host /analyze /WX与ASan各672、首同步错误DOM502、55panel109与最终May2023 SDK120 NoPublish/RenderJournal通过。各审校计数包含各自基础夹具，不跨轮累加；首两轮的前态失败日志保留。

2026-10-28 22:57+08 fresh AE0按既有授权经Deploy-TestBuild/现有Starfield Junction部署冻结stb11-native120-panel109-selection-focus（213adapter/69Core/3build/230validation/11binary/26panel），最终1,168哈希验证安装/冻结源/面板/119108一致回退；Core69源和DLL仍与119完全相同。Main A024F701F673A0609FBDAD037ABE476B31D0C22FD343B67C45E9C790A380776C，Host 5A12906EEEB55CB7CA98BE412B6A21F24E3D2D68000D7475E901AE91D5CCFD77。报告优先保留最近五次实际部署备份，仅裁剪旧perf09备份14,211,135bytes，全部候选/原始证据保留。未启动/停止AE、操作真实用户焦点、写工程或清guard；实际Delete/后续编辑/Undo重开/性能仍待owner验证。

AE关闭后从checkout一步回退119/108（默认仅报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-11-20261028/rollback.ps1 -Restore
```

## STB-10：119/108结构变更序号已部署

Owner要求解决删除问题，继续纯离线。实际旧Parameters入口在UI持锁、PF仍交付guard0、Force绑定不可用的夹具中，工作线程返回516而非取消：23,675检查1失败（Camera12通过），证据native-before.log。本轮只为完整Model图执行增加独立原子epoch：UI已拥有互斥且非source回调才能开始/结束，奇数修改中、完成变偶数；render线程仅只读。Parameters在合法guard之后检查修改中，并在checkout全部释放后比较前后序号；重叠读取取消帧并清瞬态消息，事务外缺失绑定/非法guard仍严格失败。普通idle、选节点、轮询、模态UI scope不增加序号，不扩大原UI查询，不自动清旧guard。

真实native普通/ASan各24,025与Camera各12通过；跨DLL1,212、Host目标/analyze /WX含成功/失败/停止/回执路径9,664,745通过；panel108完整55suite零失败。新独立审校实际22,015并发检查、9,735,256真实Host变体（含最终通知bad_alloc）通过，冻结9/9审校输入一致，无该局部新明显问题。未独立声称执行父native套件。完整May2023 SDK119 NoPublish/RenderJournal通过；Core69源及DLL均逐项与118相同，未重写数学/ID/schema/Core ABI9。108同时包含STB09显式逻辑分组及不依赖Array.indexOf的共享ES3工厂。

2026-10-28 16:57+08 fresh AE0，按既有授权经Deploy-TestBuild与现有Plug-ins/Starfield→dist Junction部署冻结artifacts/prepared/stb10-native119-panel108-structural-epoch：213adapter、69Core、3build、230validation、11native/Core、26panel；最终1,161项哈希核验当前/冻结源、dist/Junction、119/108面板及118/107回退通过。Core仍4E01ACC4D7509A9CBAE06A550876D8F26C5E90D3C2F0BD282FC3A10305FAA345，Host BA250BE0DE5B74D6D3273CFD54B0B09C9C762097C2A106B3509DFD05E22F99F6。部署/审校/审计见artifacts/stb-10-20261028。最初审计脚本未正确展开JSON数组，核对真实源无差异后只修artifact审计，旧失败日志保留。按报告优先五版策略仅裁剪perf06实际部署备份，所有候选/原始证据保留。

该保护只覆盖Parameters图checkout期间的完整结构事务；不覆盖事务结束后才进入的旧PF快照、随后资源checkout或绕过执行器的外部删除。全局短期epoch可保守取消其他层同时发生的checkout。没有AE现场操作/工程写入/启动停止进程；原Force首次516/遗留guard1、Undo/重开和准备性能仍待owner验证，不能凭离线通过关闭现场故障。诊断日志artifacts/graph-render-journal-119.jsonl，包含I/O成本，不用于性能资格。

AE关闭后从checkout一步回退118/107（默认仅报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-10-20261028/rollback.ps1 -Restore
```

## STB-09：社区旧引擎核查与局部候选 — 2026-10-28

STB09离线完成时部署收据为118/107；两处候选兼容修正随后已随119/108冻结部署，准确身份见上方STB10。Owner要求公开社区排查，没有执行新的AE操作；未启动/停止AE、改工程/guard或宿主配置。

社区2018年第一手报告同时记录三元关联和混合AND/OR优先级差异。当前三个JSX中唯一未分组OR后AND位于Model会话busy条件：按报告规则模型，五分钟后executing/applying/published可被新begin覆盖。旧真实transport用新回归复现，局部只加括号；迟到原生结果仍可回送，终态/未执行传输态生命周期不变。新独立审校111,530检查通过；这是报告规则下的兼容缺口，不是AE23.5x52执行证明，更不等同首次立即Force516根因。

另实际Turbulence key8的共享ES3工厂使用Array.indexOf，社区Adobe工程师/开发者指出该方法可能由其他面板注入，不能依赖。VM删除indexOf后旧gateway实际失败，改为0/3/4三个严格比较；权威tools/candidates源、browser与gateway嵌入同步，契约ID/版本不变，不注入全局polyfill。Model旧逻辑+缺方法回归2,636、共享工厂全八节点710、最终55suite零失败通过；初次未同步工厂导致的一致性失败保留，不冒称通过。第二位新独立审校4,249项通过，覆盖真实旧/新校验、权威源/嵌入及缺失/抛错indexOf；不与其包含的1,727重复相加。两处修正分别经不同的新审校者复核，报告及实际日志stb-09保留，无AE资格声明。

已有remove实现每次重取parade/effect并降序删除，完整事务已由AEGP idle执行，因此社区常见“重取引用/移到idle”的建议不能直接当作新修复。只读保存今天已有日志9,785行，新增PID29360的11次同Force8b272c/stream167/guard0/516仍无事务记录；不能据此判定线程/Host/具体阶段。调查、来源链接、前后实现及失败日志见artifacts/stb-09-20261028/research.md。原Force绑定丢失、遗留guard1和准备性能继续开放；本轮不自动解锁或请求重做电脑复现。

Owner补充只看到AE弹窗，无面板trace。继续只读核对更新于昨晚21:43的保存工程（旧13:04副本不覆盖）：五个图块CRC有效；排除EfdG定义后，一个实际图层无Force且库存无Force，另一个图层仍有8b272c Force且实际库存同UUID存在。不能当作当前内存状态、永久丢源或同层UUID冲突证据。阶段日志最早在preflight/undo后，之前的拒绝也可无transaction行。保存副本、哈希、RIFF/表达式提取及限制见stb09/current-saved-project与research；没有额外产品修正/AE操作/清guard。

## STB-08：118现场仍失败；实际删除路径待确认

Owner确认118由两次弹窗变一次，但Force删除后仍guard1，目标p0-c88-l111/AE23.5x52。截图native118/guard0/stream167/UUID8b272c483ca44731be4c59ddf92a4ffe/parameter1。当前日志快照1115条：440成功、8条同UUID516、667取消，全部uiTransaction=false，零transaction事件。不能从“零事件”直接断言哪个阶段失败、工作线程、Host错配或键盘原生删除；也不能把弹窗减少当完整修复。

已只读保存当前日志/测试工程及SHA256于artifacts/stb-08-20261027/owner-evidence。工程13:04:27的保存早于按日志mtime/tick推算约13:04:36的首次失败，完整图仍有Force，因此不能当作删除后库存。静态路由显示面板Delete应走完整Model执行器，选择节点会选择/原生reveal其AE效果，键盘焦点与实际运行模块需现场区分。进程33316在模块查询时已退出，没有本轮现场模块证据。

Owner随后拒绝重复电脑/AE对照，要求其他办法；启动器/两个测试副本只保留为未执行材料，不再请求或执行同一对照。没有启动/操作AE、写原工程或清guard。以下改用离线生产对象验证和自动随错携带证据，真实删除路径、首次缺失绑定及随后guard1仍开放。

直接复用118 SDK生产ModelGraphTransaction/EffectGraphBackup/Core对象，离线连接原有fake-suite夹具，945,481检查通过并输出175条事务记录（仅隔离探针自身DLL导入将日志定向到probe目录，不加载/修改AE或原日志）。证明这份对象代码能记录阶段，不能证明现场调用了它。实际保存前态graph-4/Force8b272c进入当前前端，离线确认为完整事务1次、普通submit0次；源文件字节保留，不能因此排除运行时缓存/键盘/其他路径。

Panel107在既有脚本引擎保留最多32条普通事件及8份首次失败，按已保存project.fsName+target隔离，捕获guard/原生nonce回执/Model prepare、commit、result；后续busy消息自动附Last graph trace，无需另取文件或额外操作。没有保存身份或路径不可读/过长时只标engineHistory，不当当前锁因；reader重建有限普通字段、限制响应32768字符，未知/坏数据或诊断getter/stringifier失败仅使证据不可用，不能改变原事务、作者/guard、命令次数或错误。轮询/重载/其他目标不覆盖首次失败。旧锁发生前未记录的原因不能追溯，未来AE现场根因仍未验收。

55前端suite/Model2530、新DOM495通过；首位独立审校实际505项复现v1跨target丢首次、跨project错归因和坏数据超长，局部v2修后新独立探针605通过（含110独立边界），父任务条件模型/共享wrapper重放630通过。v2审校首次605后因额度中断，随后恢复核对当前107逻辑并完成reviewer-v2/review.md；不追加测试，630明确归于父任务重放。所有旧失败与源码保留在artifacts/stb-08-20261027。

2026-10-27 17:58 +08:00 fresh AE0后按既有授权经Deploy-TestBuild/Junction发布冻结stb08-native118-panel107-inline-evidence；11原生/Core二进制逐字节复用118，未另称SDK构建。213adapter/69Core/2构建/36验证/26panel107冻结，安装/selector/118106回滚/Restore只读预检通过。部署收据artifacts/stb-08-20261027/deployment-verification.json。最近五次实际备份保留，报告后仅裁剪最旧perf05部署备份，所有候选/原始日志保留。AE关闭后，从checkout一步恢复118/106（默认只报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-08-20261027/rollback.ps1 -Restore
```

## STB-07：118/106诊断版部署与开放边界

Owner报告117仍在面板Delete删除Motion→Force→Output中的Force时弹binding unavailable（guard0/stream201/UUID3040462e814d4a7cad6d40e3cc407a7a/parameter1），随后p0-c88-l110的saved guard1拦住Particle删除。Particle这次拒绝不证明其独立删除也触发绑定故障。117现场未通过，不沿用此前离线审校结论宣称已修复。

实际checkout_render_graph旧实现复现：Host已持有结构事务，PF仍交付guard0旧快照并报告缺失Force。118在原guard校验后检查既有UI事务查询；新独立审校又复现graph checkout后才进入事务的窗口，现于返回前再次检查并丢弃完整/失败快照、清消息并取消帧。非法guard/checkout失败和事务外缺失源仍严格失败。不改Host互斥架构、公开ID/schema、Core ABI9/数学；查询仅覆盖当前UI线程，不声称覆盖worker或两次观察间已结束的事务/旧缓存，不自动解除既有guard1。

仅显式RenderJournal构建增加render线程/tick/当时UI状态和backup/prepare/commit/restore/discard的前后事件。旧CRT跨DLL追加实际10000次只剩9104行/1坏JSON，改FILE_APPEND_DATA与完整行单次WriteFile；新全新审校两轮各10000条均完整解析/唯一，255字节控制字符转义边界通过。日志位于artifacts/graph-render-journal-118.jsonl；I/O有额外成本，此诊断版不用于性能验收。未运行任何AE脚本或操作原故障图。

最终实际native ASan23,601/Camera12、完整事务ASan945,479通过；新独立入口探针23,797/Camera12通过。当前日志头/NOMINMAX/边界探针/analyze /W4 /WX通过；额外实际事务源分析仍因既有EffectGraphBackup栈C6262失败（117为69,360、当前69,376bytes，关闭日志也存在），不能说全项目静态分析通过。完整May2023 SDK118 NoPublish/RenderJournal成功；Core69源逐项未变，重建二进制哈希不同，实际117/118 DLL36个scene/像素逐字节一致。

2026-10-27 12:54 +08:00 fresh AE0后按既有部署授权，经Deploy-TestBuild与原Plug-ins/Starfield→dist Junction发布冻结artifacts/prepared/stb07-native118-panel106-diagnostic。213adapter/69Core/2构建/36验证输入、11二进制/26未改panel106冻结核验；dist/Junction、Core selector与117/106一致备份/Restore报告通过。Main SHA256 4E7DFE8A0D5523A579F2C97A9DE497D566B60D0533873D6629FD1DE6EE5A1D93，Host390F46E02D5BE043207A060D1B5B6C6523788B7942EFE269A6C4356A476304A7，Core4E01ACC4D7509A9CBAE06A550876D8F26C5E90D3C2F0BD282FC3A10305FAA345。证据artifacts/stb-07-20261027/{validation.json,deployment-verification.json,reviewer/review.md,reviewer-final/review.md}。按五版策略先报告再裁剪一份旧实际备份；所有候选保留。

未启动/关闭AE或写工程/guard。已有锁定图需要另行显式修复/检查，安装118不会清锁；应在新测试图复测同一Force拓扑并保留原故障图。现场首次错误、恢复失败/遗留guard、Undo/重开及5–8秒准备延迟仍开放。AE关闭后，从checkout一步恢复117/106（不加Restore只报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-07-20261027/rollback.ps1 -Restore
```

## 117/106部署收据（2026-10-26 20:53 +08:00）

Owner明确要求“部署当前版本，我来测试”。新只读AE0检查后，通过既有Deploy-TestBuild和Plug-ins/Starfield→dist Junction完整发布冻结117/106（全部Model/Material/Motion/Turbulence模块）。dist及Junction全部11二进制、新Core selector/版本DLL、26panel106与114备份哈希核验通过；Restore只读预检通过。未启动/关闭AE、运行宿主脚本或改工程/guard，现场删除/Undo/重开和性能仍待owner验收。

Main/Host/Core SHA256：E09E4B3861E45AEC9009B88AE00307ADE84BC9271CC59D5C3E814859290416FB、8EAC9308164CBE44DDAF5AFD78EA41AB0FDD084B3F28087CE0EFF99434DA26F2、C0A2C680FB90787C5442DEC201A8A279706AD109FAD2B4A19AA1CC89D68A8AD1；selector StarfieldCore-C0A2C680FB90787C.dll。部署收据artifacts/stb-06-20261026/deployment-verification.json，原validation.json保留部署前离线资格。

回滚保留一致的114/104：114二进制来自实际dist前态，panel104来自已核验的冻结114配对；部署前checkout实际panel106另存panel-before，不将它冒称旧104。回滚操作会保留当前106源码后恢复104。按既有五版策略报告后裁剪一份最旧实际部署备份，所有准备候选及证据保留。

AE关闭后，从checkout一步恢复114/104（不加Restore时仅报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-06-20261026/rollback.ps1 -Restore
```

## STB-06 内部种子：第三轮局部修复/新独立审校（117/106已部署，以下为离线证据）

第5位独立Core审校未发现114存储/生命周期改动的新明显缺陷，但实际113/116 DLL复现合法Seed0+Particle Shift-1+Circle→Force→Output被误拒绝；Auxiliary也可混合出高位seed。旧新同时拒绝不作为正确性通过。OrderedForce把derived uint32重新按public signed31上限校验，是既有缺陷。修正仅在物理Settings校验成功后恢复完整内部seed，不mask/clamp、不放宽作者限制或其他notice，不改架构/ID/schema/ABI/Render.hpp。12组边界+独立随机重力数值参考/静态历史/Auxiliary/非法作者和物理量拒绝覆盖；旧实现在1869checks复现后，Core23/23、MotionFrame ASan9,799和无NDEBUG的四Core cpp /analyze /WX通过。

第6位全新审校者独立实际helper1,058项（含1,024随机32位seed）通过，未发现修复及紧邻调用点的新明显问题。实际SDK117 DLL36场景均成功出scene/有限float像素：18先前合法拒绝的有序Force场景修复、18既有scene/像素逐字节不变；另6个既有DLL场景也逐字节一致。没有AE/GPU设备操作。此前两轮事务修复均在修后由不同新审校者复核通过；6位审校报告和实际复现均保留，不能据此声称全项目永无问题或现场AE故障已验收。

完整May2023 SDK117 NoPublish通过。最终artifacts/prepared/stb06-native117-panel106冻结213adapter/69Core/2构建/36验证源、11二进制/26panel106。MainE09E4B3861E45AEC9009B88AE00307ADE84BC9271CC59D5C3E814859290416FB、Host8EAC9308164CBE44DDAF5AFD78EA41AB0FDD084B3F28087CE0EFF99434DA26F2、Core新C0A2C680FB90787C5442DEC201A8A279706AD109FAD2B4A19AA1CC89D68A8AD1。收据artifacts/stb-06-20261026/validation.json；114安装/110回滚/Junction及所有此前候选保留。未部署/操作AE/工程/进程/自动清guard；working panel106源码不是完整117运行资格。真实缺失绑定/guard恢复、删除Undo重开及性能验收仍开放。

## STB-04/05 多轮独立审校（116/106候选，未部署）

Owner要求继续脱离AE的细致静态审查，每轮修复换新的子智能体审校，不做随意架构改动。STB04独立审校复现：JSX published后的原生确认仍可失败并rollback，但旧client取消/超时误报committed、release丢弃最终回执目标且过期begin可覆盖published。现只改既有状态判定：最终结果未知就报告unknown/pending，保留published至原生最终回执，最终负结果仍可覆盖临时publication。新审校者用独立异步次序复核未发现该范围新明显问题。实际JSX/client2530断言、panel55套件、Motion删除DOM156/181通过。原生115未变，独立115/106候选和两次审校报告artifacts/stb-04-20261026保留。

另一位独立原生审校复现prepare复制失败叠加清理失败仍guard0，部分成功复制还残留两个guard0同UUID源。STB05局部修正保留首个rollback_error，仅全部准备失败清理成功才release；后续marked-backup清理成功不能掩盖早先局部erase失败。正常成功/单故障、Core/ID/schema/ABI和整体架构不变。新的审校者独立编译实际实现，24种故障位置/部分成功/真或静默清理失败通过，没有发现修正及紧邻析构/调用者的新明显问题。最终组合事务ASan945,479、独立备份ASan717,555、目标源/analyze /WX、完整May2023 SDK116 NoPublish通过。

最终artifacts/prepared/stb05-native116-panel106冻结213 adapter/69完整Core/2构建/34验证输入、11二进制/26panel。Main4A173FB869FBB4BF9AF02B161CE4F5FDCFD6136D3C53D424DC75684660F59F92、Host05360E3386A7F10340A423FC1FC4AFB452D286D9554A9A60641E95A055F4928F，Core仍25CD4A0764AC734533634DF6EE27A7856DB4719033C5282DB38A5DE70430EA64。收据/原复现/独立审校见artifacts/stb-05-20261026。dist114及110回滚/Junction保留；working panel106源码不等于完整116已部署。未操作AE/GUI/工程/进程、未部署或自动清guard，真实故障根因/删除Undo重开/性能验收仍开放。

## STB-03 删除Force后缺失绑定/guard1（115/105离线候选，未部署）

Owner在AE23.5x52的114中按Delete删除Motion→Force→Output间Force，多次出现guard0/stream167/node5729382497c94ec587687499d0d4f3de/parameter1/component0的binding unavailable；Force似已删除，面板node_effect_sync_failed，随后p0-c88-l109的保存/named/indexed guard均1，编辑busy。截图不证明回调先后，不能宣称已找到全部现场根因。未操作当前AE/工程、未自动清guard或重试删除；已准备只读脚本并按ADR0011/owner电脑操作约束请求批准，尚无现场结果。

确认并离线修正三个事务漏洞：完整删除/Model及备份目标枚举改用saved valueAtTime(0,true)而非.value；全图失败恢复完成最终parade替换后，恢复renderer到owned1并调用现有native绑定刷新，验证后才释放0；backup cleanup/final refresh失败保持guard1并返回真实错误，不把坏绑定作为可渲染成功。前端generation同步105；不改公开参数/磁盘ID/schema、Core ABI9/Render.hpp或Core数学。备份临时名使用常量偏移消除原C28020，名称字节不变。

旧114实际备份代码复现收尾失败释放guard，新断言失败664527；旧JSX在saved0/display1误拒绝，新实现同夹具通过，并验证saved1/display0提交。真实原生Motion→Force→Output删除夹具在114即可通过，说明仍有宿主时序/依赖状态未覆盖；不将这些漏洞等同于现场根因。当前normal/ASan native各22,759/Camera12、实际全图事务ASan934,562、备份ASan706,638、目标备份源/analyze /WX、完整panel55套件和新注册catalog全审计通过。Model9,584/Material185/Motion812及额外Motion删除包装器156/181通过；旧catalog/UI回调/零count按钮测试过期仅修夹具，失败日志保留。

完整May2023 SDK115 NoPublish通过。最终冻结artifacts/prepared/stb03-native115-panel105-qualified：213 adapter/69完整Core/2构建/34验证输入、11二进制/26panel。Main61E5EB75A69BE07B75887C31187919A864548C9194229F5027C0D5C2C95E98EE、Host7AAD3BBCCFF19CCCEACCBA711F35C1404CA844848413420723B8AB33A48FA761；Core仍25CD4A0764AC734533634DF6EE27A7856DB4719033C5282DB38A5DE70430EA64。收据artifacts/stb-03-20261026/validation.json与report.md；dist114全部二进制/Junction及110回滚只读核验。checkout的web_panel现为105源码，Host可能重新加载checkout文件，未操作页面不能声称现有运行会话是完整115/105配对，也不能只凭gateway标签推断已部署。现场错误/guard恢复、删除Undo/重开与一秒目标仍开放。

## 114部署收据（2026-10-26 14:45 +08:00）

Owner明确要求“部署当前版本，我会测试”。新只读进程检查AE0后，通过tools/Deploy-TestBuild.ps1与既有Plug-ins/Starfield→dist Junction完整发布冻结114/104，含Model/Material/Motion/Turbulence。全部11二进制在dist及Junction路径、26未改panel104、Core selector/版本DLL及110配对备份哈希核验通过；Restore只读预检通过。未启动/关闭AE、执行宿主脚本或改测试工程/宿主设置，真实AE验收仍待owner。

Main/Host/Core SHA256分别为6428B53E6BD15582C092995B170BC866F6F79B56F8F09D853E784ADF53C991C4、821609F2D2FB534BE8731F72E8356800044496682E22FF0B6D9FB964C60C5411、25CD4A0764AC734533634DF6EE27A7856DB4719033C5282DB38A5DE70430EA64；selector为StarfieldCore-25CD4A0764AC7345.dll。部署/前态/回滚/备份裁剪收据见artifacts/perf-09-20261026。按既有策略报告后保留最新五次实际部署备份，所有准备候选及证据保留；下方离线记录是部署前历史。

最近回退为110/104；AE关闭后从checkout一步恢复（脚本默认仅报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/perf-09-20261026/rollback.ps1 -Restore
```

## PERF-09 源码候选114（已部署，以下为部署前离线证据）

继续遵从owner“先继续离线检查”。Core准备按实际Emitter/Particle/Force数量保存payload，稳定索引视图保留全部节点语义校验；移走已归属Settings、减少Result中间搬运及Auxiliary逐父粒子复制。普通/时间求值使用请求内、按递归深度分离的有界optional scratch，退出清空值，仅重用存储；ordered Force不逐粒子分配scratch。CRC/样条索引显式有界，无效输入仍类型化失败。参数ID/schema、Settings.hpp/Render.hpp、Core ABI9、数值公式/次序/身份/预算/取消不变。

旧113的五处实际Core告警均已处理：CRC/样条C28020及普通/时间/ordered Force三处C6262；四个拥有的Core cpp以实际SDK相同的无NDEBUG条件/analyze /WX通过，不代表全项目分析资格。独立/O2 /MT生成代码栈分配分别80,448→25,616、55,864→10,008、14,496→2,272bytes，非最终LTCG/AE测量。128Force夹具累计分配字节1,899,247→834,523，Auxiliary t36/24为2,035,046→1,561,008；简单图会增加少量固定帧存储，不能宣称所有场景均下降。

旧113/新normal/新ASan的19场景、38完整图/粒子文件、每实现27,247,959bytes逐字节一致。最终旧1,724/新normal与ASan各1,680项（含普通/时间逐处分配失败）、Core23/23、实际native21,359/Camera12、Model桥ASan997累计通过。完整May2023 SDK114通过；实际旧/新Core DLL六场景的数值scene及RGBA32F像素逐字节一致，不创建GPU设备。

独立冻结artifacts/prepared/perf09-native114-panel104：213 adapter输入、完整自有Core源/头文件、2构建输入、20验证源、11二进制/符号、26未改panel104。Core新SHA256为25CD4A0764AC734533634DF6EE27A7856DB4719033C5282DB38A5DE70430EA64，不能沿用113哈希。报告/指纹收据artifacts/perf-09-20261026。只读核验dist全部文件/Junction及panel仍110/104，111–113候选/全部备份保留。无AE/电脑操作/部署；原banner、测试工程出图、连续操作及一秒/Stardust目标仍开放。

## PERF-08 源码候选113（未部署/未在AE运行）

本轮仅离线工作。用冻结112的实际SmartRender原代码及五项内存夹具重编，复现两项停放Motion错误：未连接Output的Light Path/Look At仍请求灯光，宿主拒绝时阻断整帧准备。113共享原生Output祖先collector；完整作者输入先做descriptor/结构校验，再生成仅活跃节点/边的callback-local工作图，保留参数/顺序/optional records，不写作者图。Motion灯光/快门/历史、图传输、纹理/Model和材质灯光资格消费工作图，活动缺失/损坏资源仍严格失败。此缺陷并未被证明是当前真实AE banner或全部5–8秒等待的原因。

纹理/Model借用原完整布局，绝不按裁剪图重编号；真实资源测试确认renderer slot1与紧凑metadata slot0分离、Model完整count/UUID/revision/bounds及shutter校验保留。无活跃导入Model时不因停放Model重复模拟。实际pre_render ASan24、纹理ASan142、Model资源桥ASan997累计、原生normal21,359/Camera12通过；四个实际adapter源/analyze /WX及完整May2023 SDK113通过。128停放Motion仍交付三节点工作图且灯光请求0，编码与普通Output基线一致；不等于真实AE计时。

独立冻结artifacts/prepared/perf08-native113-panel104：212生产输入、18验证源、11二进制/符号、26未改panel104；Core C944…及ID/schema/Render.hpp/Core ABI9不变，112/111保留。只读核验实际Plug-ins/Starfield Junction与dist全部文件仍为110/104，未部署/操作AE或设备。报告和收据artifacts/perf-08-20261026。宽范围/analyze另发现未修改Core的SequenceCodec C28020、GraphEvaluation约65KB栈C6262、MotionGeometry C28020，日志与static-followups.md保留，未关闭/未算通过；下一轮需独立Core任务处理。真实AE出图、连续操作和一秒目标仍开放。

## PERF-07 源码候选112（未部署/未在AE运行）

Owner对具体AE验证请求回复“先继续离线检查”，本轮没有操作AE、运行宿主脚本或部署。112在111诊断修正之上合并稳定图的依赖校验：全部活跃alias预热之后，用包含引用图层的CHECK_ALL/all-time状态前后夹持完整UI证明捕获；只有这对状态相同，publication才带完整收据。恒定/曲线资格、单项状态对及实际数值认证保留。渲染按UUID/stream选择，完整收据匹配可共享一次查询/比较；缺失、变化或失败则逐项严格验证及旧有效回退。UI调度stamp不能充当该证明，动态输入不会因此成为常量。

离线512输入/32份publication的稳定案例从512次状态查询/512次比较降到1/1；不带曲线的证明选择微基准0.0620→0.0170ms，分配516→5次，非整个AE准备计时。普通与ASan实际adapter各21,359/Camera12通过，旧保存缺失Auxiliary夹具21,364/Camera12通过；缓存/analyze(/WX)及ASan各124项，含对照基准3,724项；实际NativeNodeGraph /analyze /WX及完整May2023 SDK112通过。覆盖完整图字节跨帧一致、外层捕获中途变化/失败、动态payload、跨回调/复制层、旧有效回退、重入淘汰、并发与分配失败。

独立冻结artifacts/prepared/perf07-native112-panel104：211生产输入、12验证源、11二进制、26未改panel104。与111相比仅6项生产输入变化（包括版本和注释）；Core仍C944…，ID/schema/Render.hpp/Core ABI9不变，111候选完整保留。核验dist全部11二进制及Host实际加载的web_panel仍是110/104配对。详情artifacts/perf-07-20261026/report.md与validation.json。完整状态查询本身在真实AE内部的成本尚未测量，不能宣称一秒、复杂度无关或Stardust对齐；真实banner原因及连续操作门仍开放。

## STB-02 源码候选111（已冻结，未部署/未在AE运行）

静态复核并用旧110源码离线复现：ScriptDiagnostic把容量>8192的空字符串判为错误。111改为按首字节判空，不分配/扫描容量；SDK非零执行状态、非空诊断及内存/suite失败仍严格拒绝。面板保留host_script_failed错误码，同时显示实际AE状态和最多1024bytes的诊断文本；UTF-8截断按显示文本处理，不能抛掉原错误或变成成功。队列/quiet-error/回执/事务/参数/ID/schema/Core ABI9均不变，Core DLL仍为110的C944…哈希。不宣称此静态缺陷已被证明是当前真实面板banner的原因。

面板实际代码离线/analyze(/WX)509项、ASan509项；Model事务分析9,664,560项、ASan9,665,054项、Model资产150,891、预设Host1,842项通过（大量检查来自既有分块payload夹具，不能当真实AE操作次数）。测试不启动AE/浏览器或操作用户窗口；message-only假宿主不可见。SDK111/211输入、11二进制/26未改panel104及8个验证输入冻结于artifacts/prepared/stb02-native111-panel104；只读核验当前dist仍是110/104。证据artifacts/stb-02-20261026，111源码改动不等于当前安装已变更。

Owner最新约束下，本轮未进行电脑操作；实际111诊断与一秒目标的宿主确认需先请求批准。110的一步回退和原工程副本仍见下节；不自动关闭AE来部署或裁剪。

另一个纯内存前后对照只让commit的成功响应附带大容量空诊断：旧实现已执行一次commit却发布0|5|516失败回执；111同样只执行一次，保留1|7|0成功回执。这证明条件下的错误确认会中断已执行操作，修正不只是改变提示；仍不证明用户工程实际遇到这种缓冲区形态。Owner随后要求先继续离线检查，未执行该次AE验证请求。

## PERF-06 已认证数值复用与字段库规模（110已部署）

UI在相同完整宿主依赖state前后夹持、实际求值并验证类型/启用/有限值后，随恒定证明保存自有数值。当前帧及历史hoist按UUID/stream和PF_State比较验证后复用，未知/失效/动态/无效快照保留严格checkout/checkin；不从disk值推断恒定。原生字段库改按各节点既有布局分配，Force不再复制Particle约26KB曲线库，消除解析/历史样本的大栈对象。AgeCurve二分搜索增加显式数组边界，既有有效输入公式/编码、所有公开/磁盘ID/schema/Render.hpp/Core ABI9不变；Core二进制因共享头变化重建，不能沿用109哈希。

最终normal/ASan原生各23,976项、Camera12通过；保存缺失Auxiliary夹具23,981/Camera12通过。实际NativeNodeGraph /analyze /WX通过；证明缓存分析71、曲线边界分析258,807、标准Core CTest22/22通过。当前帧完整图编码在三帧/不同回调引用下与原checkout一致；新增数值读取途中依赖变化/失败/NaN/unavailable、UUID错配和真实缺失绑定回退覆盖。不得据此宣称AE准备已低于一秒或已对齐参考插件。

完整May2023 SDK110/211输入及11个native/Core、26个未改panel104冻结在artifacts/prepared/perf06-native110-panel104。2026-10-26T00:43:39+08:00新AE0下发布110/104，安装/Junction/109备份/Core selector及Restore报告核验。Main/Host/Core SHA256：8D8F09FB3086A9E52A8A2193718D9DAFEA5279D4D4D2490710F752B2EBABE83C、220B34D875E49D48D1ED96AEF7ED6ACBDCBA93DDC3A66098C7AF6CA0C6A2D40C、C9446960F386E05ECE66D23FFFE615EB7B466FAB2852269966768877A804D712。

最近回退为 **109/104**，AE关闭后从checkout一步恢复：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'perf06-native110-panel104-certified-values-20261026' -Restore
```

Owner10月26日明确允许直接编辑D:/Project/Code/test/testproject.aep，说明它仅用于测试。当前工程SHA256已是274F5B4981F50E0B6E5D04CC7AD27A7B8789F6F6CCF8A5A3FCCE58BED00879A0，与25日取证27FE…不同；已保留并校验当前副本artifacts/perf-06-20261025/owner-project-before/testproject.aep，新的只读图证据单独保存，不能沿用旧工程状态。

真实AE验证尚在进行：命令行启动派发PID30772随后消失，未有脚本结果，不能算宿主通过；正常窗口启动确认AE进程与Untitled窗口存活。独立计时工具首次在AE因ES3保留字native编译失败，已修正工具而未改产品；冻结候选保留初版工具快照，后续工具源/日志须单独记录。Owner随后在AE打开当前testproject，观察到节点面板host_script_failed；正在读取具体脚本原因。未报告真实修复/出图/连续切帧/一秒延迟资格。以下109及以前为历史记录，当前身份以以上110收据为准。

后续只读owner-host-probe.json已在**AE23.5x52**完成：JSON可用、6项会话命令绑定存在、gateway104可直接加载，当前Comp5三层均未选中，getState正常返回no_target。这个结果不能证明AEGP面板桥或当前层出图成功。刷新点击被电脑操作工具的子窗口归属检查拒绝，没有完成刷新，不据此推断第二次请求仍失败。Owner最新要求非必要不操作电脑、必要时操作前请求批准，后续不沿用先前授权继续窗口操作。原工程截至探针未写入；原始副本保留。Prune依AE运行守卫拒绝，未删备份，当前六份，五版裁剪待AE关闭。

## PERF-05 证明快照与准备查找（109已部署）

109保留108的显式Diagnostics修复和全部此前准备优化。原生证明改为不可变自有publication/catalog快照，读请求不再复制最多32份完整证明/曲线；UUID/stream候选索引按既有次序合并，仍逐项PF_State比较、最新有效优先/旧有效回退、每stream一次状态查询。释放旧曲线/前缀放到锁外。常量证明与Life/rate查询建立请求内索引，内存不足回退原线性查找。修正重用plan在新准备失败时残留旧常量/raw Life的问题，先清理再分配/查询，失败后保持实际alias采样。参数ID/schema/Render.hpp/Core ABI9及panel104不变。

同一独立证明微基准512输入/32份publication，无rate keys时2.1614→0.0603ms、分配5,479,122→86,662bytes；每32项含1024-key profile时5.6324→0.0652ms、18,082,002→480,502bytes，均保留512次状态查询/比较。此为证明选择，不是整个准备或AE总延迟，不能据此宣称已解决5–8秒等待或达到一秒。

最终证明套件71、MSVC/analyze(/WX)71、ASan71通过；含基准1871项。实际原生normal/ASan各21,818项及Camera12通过，保存工程缺失Auxiliary复现21,823/Camera12通过。相同夹具编译旧108源码精确复现三项旧证明/旧Life残留失败，新109通过；旧缓存也复现publication分配失败损失前缀租约。完整May2023 SDK109构建/211输入一致，冻结11个native/Core、26个未改panel与符号/源码/最终测试；详细证据artifacts/perf-05-20261025/report.md及validation.json。

2026-10-25T17:00:55+08:00新AE0下经Deploy-TestBuild发布109/104，11个dist/Junction及108备份、26个前端和Core selector/Restore报告核验。Main/Host/Core SHA256为3217510DE1CEFF9FA320DAD34A7FFBA91E5B6A908AD805DB10C32E118D8CE150、F40C3D9920620AE1760468757A2C580262AAE5118FE7F263C8EA4EC2C38B7996、514F52C3C5DF0270B974A8559E391A7A8E62BE33F0AC2C9DD450263AA5D90C5B；Core selector仍为StarfieldCore-514F52C3C5DF0270.dll。未启动/停止AE或改原工程/宿主环境。

最近回退为 **native108/panel104**，AE关闭时从checkout一步恢复：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'perf05-native109-panel104-proof-cache-20261025' -Restore
```

最近五个实际部署备份：perf05-native109-panel104-proof-cache-20261025、stb01-native108-panel104-orphan-repair-20261025、perf04-native107-panel104-preparation-20261025、perf03-native106-panel104-additions-20261025、perf02-native106-panel103-prep-20261025。先报告后仅裁剪p09r-native105-panel103-native-titles-20261024，释放14,124,996bytes；prepared/SDK/历史失败证据保留。当前仍需owner在原图层Diagnostics > Repair Selected Graph显式修复缺失源引用，实际AE恢复/Undo/重开、连续操作和一秒目标未验证。以下108及以前内容为历史证据，当前安装以以上109收据为准。

## STB-01 guard0陈旧图恢复（108已部署，待owner显式修复）

Owner的107启动stream208错误已在此前相关testproject.aep的当前保存数据和真实adapter中复现：Auxiliary UUID7f86fd58ba9f494e80725f518bcf16ec仍在图/两条回连/slot110别名中，同层原生源库存却没有该UUID；三种现存Emitter的cdat身份与名称一致。只读工程SHA256为27FE820C5BDEA9E36C4892644537A285A91A6EC5C025C7A5CFF2A78DD3BC038F，调查后保持不变。源何时/为何消失尚未确认，不能断言是107引起；回退106也不能自行清理保存的不一致。

108扩展现有显式Diagnostics > Repair Selected Graph：允许ready1/guard0/完成回执的idle原图，也保留guard1/拒绝回执修复。仍要求主线程/互斥/唯一renderer及已知idle作者布局；先在单Undo内完整复制并静音备份、再重验/guard/禁用原renderer。只按实际native UUID库存移除不存在目标的原生连接、压紧并读回记录，保留有效边ID/顺序；然后沿既有disabled-rebuild路径重编图、严格验证所有新活跃绑定/回执/版本/flags/guard。失败保留备份与guard并报告阶段。不会重建缺失源，不用0/旧值替代无效粒子输入，也不在启动/轮询中自动写工程。参数ID/schema/Core ABI9、panel104不变；107准备优化源码保留在108。

**当前工程需要一次显式修复**：重开AE后选原始Starfield图层，在节点面板Diagnostics中点击Repair Selected Graph；修复会保留禁用全层备份，可一次Undo。实际修复/渲染、Undo/保存重开、连续操作及一秒准备目标仍待host证据，当前不宣称已恢复该工程出图或已达性能目标。

实际修复协调器normal、MSVC/analyze(/WX)、ASan各24,530项通过；实际原生21,468/Camera12及当前保存图复现21,473/Camera12通过；Motion author3,108、Model控件453通过。旧修复夹具256项小于Motion metadata已改为实际参数计数/检查边界；共享ID校验增加显式容量检查、UTF-8前缀移位在操作前拓宽，保留IDs/字节并消除分析告警，不关闭warning。Restore报告现在检查与动作相同的候选保留冲突，隔离回归通过，108备份不再预占candidate-panel。不是全项目或真实AE零错误资格。

完整May2023 SDK NoPublish构建与211项当前输入核验；冻结artifacts/prepared/stb01-native108-panel104/的11个native/Core、26个未改panel104与符号/源码。2026-10-25T16:08:25+08:00新AE0下发布108/104，11个dist/Junction/106备份、26个前端及Core selector和更新后Restore只读报告通过；不启动/停止AE、不改原工程/注册表/环境/共享缓存。详细证据artifacts/stb-01-20261025/的report.md、validation.json、deployment-verification.json。

Main/Host/Core SHA256分别为7972DCEE22E3E4D4C6007483D661DECDC48B1F3B1532DD28D2CAAC855D352C15、6D88627A2A518E8C73997E820E1697453C73DC6E4D5F459AE2D3F27D62A455A4、514F52C3C5DF0270B974A8559E391A7A8E62BE33F0AC2C9DD450263AA5D90C5B；selector为StarfieldCore-514F52C3C5DF0270.dll。最近回退为 **native106/panel104**，AE关闭后从checkout根目录一步恢复：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'stb01-native108-panel104-orphan-repair-20261025' -Restore
```

最近五个实际部署备份为stb01-native108-panel104-orphan-repair-20261025、perf04-native107-panel104-preparation-20261025（已执行过106回退）、perf03-native106-panel104-additions-20261025、perf02-native106-panel103-prep-20261025、p09r-native105-panel103-native-titles-20261024。按授权先报告后裁剪最旧p09-native104-panel103-native-controls-20261023，释放14,154,692字节；prepared/107失败证据/SDK/worktree保留。P-09/P-08R及未报告host门仍开放。

以下为107失败及106回退、先前配对的历史流水；当前安装以以上108/104收据为准。

## STB-01 启动active alias不可用（进行中）

Owner在107报告启动不能正确出图，截图为guard0、stream208、node7f86fd58ba9f494e80725f518bcf16ec、parameter1/component0的binding unavailable。这否定107在该工程中的启动可用性；此前离线检查和驱动结果不能当作宿主通过。安装与107冻结候选一致。只读检查此前相关testproject.aep的当前数据已匹配该UUID与stream：保存图含Auxiliary及回连，实际源效果库存中没有该UUID，原生Emitter的三个真实cdat UUID与名称一致；源消失的操作/版本未确认，不能把此状态直接归因于107优化。

新AE0检查后已按standing恢复并核验 **native106/panel104**：11个dist/Junction二进制、26个panel104与先前106/104冻结配对一致，Core selector恢复StarfieldCore-0014A659EA279FC3.dll。106是否恢复当前工程出图仍需owner确认。107源码/符号/失败候选和截图均保留，不启动/停止AE、不改工程或共享环境。证据artifacts/stb-01-20261025/rollback-verification.json及startup-error.png。

首次Restore拒绝执行，因为107发布时预先复制的candidate-panel与恢复工具保留路径冲突；当时安装未改变。核验这26个文件后将重复快照移到artifacts/stb-01-20261025/pre-retained-panel107，再执行Restore成功，原失败/最终日志均保留。108轮已修正预检覆盖，不把“报告通过”再当作真实恢复已验证。

以下107记录为已撤回的候选历史，不是当前安装或宿主验收。

## PERF-04 跨帧历史编译与 UI 预热（107已部署，待AE实测）

Owner确认面板操作改善，但不改参数渲染不同帧仍感到5–8秒准备。其Options截图为总483ms、C472/H7.6/S0.3ms、UI最大557.4ms；此前误读5574ms已明确更正。这些不同回调读数不能相加解释完整等待，当前仍未完成真实AE端到端归因。

107将宿主已证明恒定的Emitter/Particle/Force/Transform/Turbulence参数在一次历史求值请求内编译复用，完全恒定图省去第二组当前帧alias checkout；复杂图保留既有积分/分支/排序，动态/缺证明继续历史采样。合并DRAW/idle/Options预热状态，不再要求至少三次完整预热；以宿主全依赖状态比较跨回调复用，证明发布记录清除/淘汰仍重采。稳定idle只作一次整体state查询，移除计时器前逐alias证明扫描；渲染仍逐项比较证明，首轮lazy预热保留。Core C ABI9、Render.hpp、ID/schema及panel104不变。

准备诊断新增T图传输/R资源，UI区分last/max且计时覆盖idle依赖检查，Build不再硬编码31。Options对尚未附渲染资源的作者Light Path只提示live count需出图资源，避免错误按numeric points求值；不放宽实际渲染资源校验。Options仍有其已有绑定/预热行为，不是无副作用的纯读取工具。

最终Core CTest21/21、720组相同历史算法的编码结果逐字节一致、原生21,468项/Camera12项通过；新增验证稳定idle/DRAW一次整体查询、跨回调引用、证明清除后恢复、冷加载/重开/动画/线程与事务守卫。真实CUDA/OpenCL独立SmartFX/驱动475项通过；旧GPU runner缺已有资源源文件、Cloud view和light key断言已修正，保留失败流水。Core独立3万/5万复杂图准备中位数降约37–56%，不是AE总体延迟或Stardust对齐证据。

最终完整May2023 SDK NoPublish构建通过，211项输入与源码/冻结候选核验。冻结artifacts/prepared/perf04-native107-panel104-final/，11个二进制、26个未改panel104文件与符号/源快照保留；较早未部署的107候选仍保留。2026-10-25新AE0检查后按standing授权用Deploy-TestBuild完整发布，11个dist/Junction/106备份、26个前端与Core selector及Restore报告预检通过。没有启动/停止AE、改注册表/宿主环境/共享缓存。证据artifacts/perf-04-20261025/的report.md、validation.json、deployment-verification.json。

Main/Host/Core SHA256分别为D5DD750FAB9275E87963141DB7C4CB1D6BCD71E5D5DED0808E05087A2384C126、6B4C5E01D47987D899FE2F8C145F1E872B42FB9A05A7473C0A4A740024FBDEDB、514F52C3C5DF0270B974A8559E391A7A8E62BE33F0AC2C9DD450263AA5D90C5B；selector为StarfieldCore-514F52C3C5DF0270.dll。最近回退为 **native106/panel104**，AE关闭后在checkout根目录一步恢复：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'perf04-native107-panel104-preparation-20261025' -Restore
```

最近五个实际部署备份为perf04-native107-panel104-preparation-20261025、perf03-native106-panel104-additions-20261025、perf02-native106-panel103-prep-20261025、p09r-native105-panel103-native-titles-20261024、p09-native104-panel103-native-controls-20261023。先报告后只裁剪最旧p08r-native103-panel103-ui-register-20261023，释放14,078,654字节；候选/历史证据/SDK/worktree保留。AE2023连续切帧体感、首帧/重开/参数动画与参考同场景对照待owner复测；P-09/P-08R及未报告宿主门不关闭。

以下为先前配对的历史流水；当前安装以以上107/104收据为准。

## PERF-03 连续添加与作者准备批处理（panel104已部署，待AE实测）

Owner反馈渲染速度可接受，第二批要求集中解决连续添加的打断与出图前准备。已初始化图现在可以在上一笔添加等待回执时继续接收独立添加，稳定UUID/坐标立即预览；单写者使用最新确认revision/stamp分批提交，每批最多16个、最多64个未确认添加，同批一次Undo。自己的回执不会取消下一次palette拖放；外部图/尺寸/资源/目标改变仍使手势失效。其他拓扑编辑与Pin在添加期间保持互斥。

通过合并等待中的添加，减少重复作者构图、绑定检查和读回；每笔添加成功后不再立即getPanelState全量刷新，普通pulse在队列结束后对账，匹配回执不重建画布。失败/超时/警告停止后续项、保留已确认节点并刷新真实宿主结果，不自动重试未知写入；不绕过现有guard、完整校验、回执或回滚。Core数值准备/渲染算法、native106快路、参数/ID/schema/ABI不变；实际AE同步节点注册和主线程耗时仍需实测，不能从延迟回调夹具推断完全无卡顿。

最终55个面板suite全部通过，队列205项检查；完整面板模拟宿主延迟时，11次连续拖放立即显示，仅两次串行提交，回执在下一次拖动途中到达不取消手势，成功后没有逐笔全面读取。实际JSX gateway的fake-AE夹具覆盖多节点追加的Property失效/重新获取、每批一个Undo组，以及中途创建失败后移除本批已建效果、恢复原图和guard；不是C++ compiler或真实AE运行证据。

冻结artifacts/prepared/perf03-native106-panel104/，26个前端文件中8个变化，generation/cache/gateway统一104；11个原生/Core与211项原生输入完全未变，复用原106构建/PDB，不宣称新编译或新增宿主资格。2026-10-25 AE0下通过Deploy-TestBuild -KeepNative配对发布，保留既有单个Junction和Core selector；11个当前/Junction/旧备份哈希、26个104当前/冻结及103回滚源文件核验通过，Restore只读预检通过。收据与日志在artifacts/perf-03-20261025/，没有启动/停止AE、写注册表或改共享缓存。

最近回退为 **native106/panel103**，AE关闭时从checkout根目录一步恢复：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'perf03-native106-panel104-additions-20261025' -Restore
```

原生Main/Host/Core哈希仍分别为0E1BAF0356C47DF906147FEDE963B8443BC7485AF062D1937FCFF171CE77AB8D、CB4232370D3394B2D2A4D99D4F014160AF486822F203B43041A565031155513A、0014A659EA279FC38E1CAB1DE6B063CBFA6C3A631B4EB653AA9861AE003349AC，selector仍StarfieldCore-0014A659EA279FC3.dll。实际AE连续拖放、按批撤销/重做、目标切换、保存重开及真实延迟待owner确认；旧P-09/P-08R等宿主门不因此关闭。

按最新五版策略先报告后仅裁剪p08-native102-panel103-auxiliary-20261023实际部署备份，释放28,217,905字节；冻结102候选/失败证据、SDK和worktree保留。收据artifacts/rollback-retention/20261025-121444970/。当前实际部署备份：

- perf03-native106-panel104-additions-20261025
- perf02-native106-panel103-prep-20261025
- p09r-native105-panel103-native-titles-20261024
- p09-native104-panel103-native-controls-20261023
- p08r-native103-panel103-ui-register-20261023

以下106/103及更早配对/回滚描述均为各自当时的历史收据。

## PERF-02 渲染前准备优化（106已部署，待AE实测）

Owner要求先实施性能收益最明显的优化。本轮将静态资格改为既有Output祖先范围，停放Force不再迫使简单输出进入历史路径；对宿主PF_State已证明全部活跃输入恒定的单条Emitter→Particle→Force(s)→Output链开放已有解析求值，仅Default发射、无Auxiliary/随机Life/Wind/Spin。动画或证明缺失/失效继续历史采样。扩展数值对照发现并行Force路线不等价，因此分支/汇合明确保留原路径；未顺带更改Core语义。

实际adapter回归先复现两项慢路，最终原生同步21,580项、Camera12项通过。504组线性重力/阻力数值场景、10,944个粒子对照通过，最大绝对差异1.384e-12；新构建Core完整CTest20/20通过。相同fake-host夹具、相同其余对象，仅替换修复前/后准备源码的三次中位数：3万粒子“准备+Core求值”270.2094→5.5118ms，5万456.1273→8.9083ms；不含真实AE参数/表达式开销、像素绘制或整图传输，不能当作AE帧耗时或Stardust parity。单独准备阶段不再历史遍历/重复checkout/编码粒子，后续Core仍正常求值完整粒子量。

完整May2023 SDK / x64 Release候选两个NoPublish开关构建通过，211个adapter输入与当前源码一致。冻结候选artifacts/prepared/perf02-native106-panel103/含11个原生/Core、PDB、前端103、输入指纹及未提交源码身份；Core DLL与105相同，只有四个产品源码文件变化，不改参数/ABI/schema/Render.hpp或前端。UI连续添加队列/画布刷新未在此卡修改。

2026-10-25新只读确认AE0，核对安装仍为105/103及既有单个Junction后，按standing授权通过Deploy-TestBuild完整发布106/103。11个安装/冻结/Junction读取/旧105备份哈希、26个panel103当前/回滚文件及Core selector核验通过。首次配对Restore报告因尚未补panel-snapshot失败，原日志保留；随后从冻结105中核对与当前完全相同的26个103源文件，补齐配对快照，最终Restore只读预检通过。没有执行真实恢复或启动/停止AE，也未修改注册表、共享缓存或其他扩展。

- Main AEX SHA256：`0E1BAF0356C47DF906147FEDE963B8443BC7485AF062D1937FCFF171CE77AB8D`
- Host SHA256：`CB4232370D3394B2D2A4D99D4F014160AF486822F203B43041A565031155513A`
- Core SHA256：`0014A659EA279FC38E1CAB1DE6B063CBFA6C3A631B4EB653AA9861AE003349AC`
- Selector：`StarfieldCore-0014A659EA279FC3.dll`
- 证据及前后收据：artifacts/perf-02-20261024/；本轮跨午夜完成，报告日期2026-10-25。

最近回退为完整 **native105/panel103**。AE关闭时一步撤回106：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'perf02-native106-panel103-prep-20261025' -Restore
```

实际AE2023中用原工程确认冷/热首帧、参数动画、采样精度、Motion Blur及保存重开；原P-09原生选择、P-08R崩溃等宿主门仍开放。按最近五版策略先报告后只裁剪p07-native101-panel102-emitter-ui-20261019，释放14,058,686字节；证据retention{,-report}.log与artifacts/rollback-retention/20261025-001408241/。当前五个实际部署备份：

- perf02-native106-panel103-prep-20261025
- p09r-native105-panel103-native-titles-20261024
- p09-native104-panel103-native-controls-20261023
- p08r-native103-panel103-ui-register-20261023
- p08-native102-panel103-auxiliary-20261023

以下105及更早配对和回滚描述均为当时的历史收据。

## P-09 原生选择栏消失修正（105已部署，待AE复测）

Owner截图确认104的Shape和Transform图层选择实际消失。对照正常Particle Color/Up Axis/纹理图层，公共参数注册仍为定位线添加PF_PUI_TOPIC和尺寸，接管了包含原生行内值的标题区。105移除这层隐式接管及标题绘制路由，完全复用其他原生控件的注册方式；不另做选择栏。定位线暂停占用原生参数行，Host reveal/会话查询保留。没有参数ID/类型/顺序/数量、schema、Core ABI9或渲染改动。具体审计与验证边界见[native-controls-audit.md](native-controls-audit.md)。

严格八类实际AEX审计在104明确6项失败，105的相同240项检查全部通过；Shape/Transform按钮事务和共享原生注册/透传回归通过。完整May2023 SDK候选使用两个NoPublish开关构建通过，恢复后核验211个adapter输入哈希与当前源码一致。此前中断是自动审批服务用量限制，未执行被拒绝的日志读取/文档更新；本轮已继续完成。离线结果不代表AE真实显示已验证。

2026-10-24重新只读确认AfterFX/AfterFX_64数量0，按standing部署授权使用现有Deploy-TestBuild完整发布native105/panel103；11个原生/Core安装/冻结候选/旧备份/既有Junction读取哈希、26个前端文件及Core selector核验通过，Restore只读预检通过。前端仍为103且字节不变，Core DLL与104相同。冻结候选artifacts/prepared/p09r-native105-panel103/包含PDB、输入指纹和未提交源码身份；部署前后收据、日志及104失败审计在artifacts/p09r-native-titles-20261023/。

- ParticleNode SHA256：`E808AB8D11A1EF1B6B1CA2EFCF34E325987017535B2613BFD9B2044591E47BBF`
- Transform SHA256：`46E548EF9A7A68E327C606F83217E11F36641FB43B780CEC3D1E31C36959260E`
- Host SHA256：`826DFC26E67B158A004FE4615E542D6DFC042444510A63990C67D6FD428B32AC`
- Core SHA256：`0014A659EA279FC38E1CAB1DE6B063CBFA6C3A631B4EB653AA9861AE003349AC`
- Selector：`StarfieldCore-0014A659EA279FC3.dll`

最近回退备份保存部署前的 **native104/panel103**（已知该版有选择栏消失问题），AE关闭时一步撤回105：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p09r-native105-panel103-native-titles-20261024' -Restore
```

105实际AE的Shape/Null选择框、Create Null、撤销/重做和旧工程保存重开仍待owner确认；没有启动/停止AE、修改注册表、共享缓存或其他扩展。此前P-08R崩溃等未报告宿主门继续开放。104失败与103稳定候选/回滚证据均保留，不建议把104当作已验收版本。

按最近五版策略先报告再只裁剪p05-native100-panel101-menu-20261019；精确释放量及路径见retention{,-report}.log和artifacts/rollback-retention/本轮收据。当前实际部署备份为：

- p09r-native105-panel103-native-titles-20261024
- p09-native104-panel103-native-controls-20261023
- p08r-native103-panel103-ui-register-20261023
- p08-native102-panel103-auxiliary-20261023
- p07-native101-panel102-emitter-ui-20261019

以下104及更早发布收据是历史；其最近回退/当前备份表仅描述当时状态。


## P-09 native104全节点选择控件（历史：owner已确认选择栏消失）

Owner截图指出Particle Shape及Transform图层选择被自绘替换，并要求所有节点优先使用AE原生选择。native104恢复这两项默认PF_POPUP/PF_LAYER，Create Null改为独立原生按钮。八类实际候选AEX共23个普通选择控件的注册/事件审计通过，Auxiliary共享Emitter实现；Model/Material/Motion/Turbulence及Particle纹理选择原本已是原生。专用曲线/渐变、文字编辑和标题定位线保留。完整清单、迁移及验证见[native-controls-audit.md](native-controls-audit.md)。

Transform仅追加无数据按钮disk1415/runtime91，总num_params92；旧控件、UUID、guard、图schema及Core ABI9不变。Shape保留Model6，Face5位置使用原生不可选分隔项。两个NoPublish开关的May2023 SDK完整构建、八类AEX审计、Shape/Null/纹理/共享dispatcher、实际图同步及相关前端记录回归通过；具体结果与初次失败边界仅在审计页和artifacts/p09-native-controls-20261023/记录。未将离线成功记为AE实测。

最初AE仍运行所以仅构建。Owner随后明确“ae已退出，你可以直接部署”；新只读检查AE0后通过既有Deploy-TestBuild完整发布native104/panel103，11个原生/Core、26个前端当前/旧备份、单个Starfield Junction及Core selector哈希核验通过，Restore只读预检通过。前端源码字节与103相同，没有改generation；Core DLL与native103完全相同。冻结候选artifacts/prepared/p09-native104-panel103/保留PDB、输入指纹和包含未提交源码的构建身份；收据为artifacts/p09-native-controls-20261023/native104-deploy-{before,after}.json及deploy.log。

- ParticleNode SHA256：`0FFCCB6EBA5FFFF1F5426296EFA46C17670713D3ECEBD405A72927E60C4BF88F`
- Transform SHA256：`F815AF1F6E35FD32A8EAFC16330BF1D2B69578EAAD178E20F5371F12420C6C0A`
- Host SHA256：`B43D029A219823BEAA32B148C398C46628C54822951A6712055B511235260B9B`
- Core SHA256：`0014A659EA279FC38E1CAB1DE6B063CBFA6C3A631B4EB653AA9861AE003349AC`
- Selector：`StarfieldCore-0014A659EA279FC3.dll`

最近回退配对为 **native103/panel103**，AE关闭时一步恢复：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p09-native104-panel103-native-controls-20261023' -Restore
```

真实AE的控件布局、Shape/图层选择、Create Null、已有工程动画、撤销和保存重开仍待owner实测；P-08R原步骤崩溃复测等旧宿主门不因这次部署关闭。没有修改注册表、共享缓存、启动/停止进程或其他扩展。

按既有最近五版策略先报告再仅裁剪p04-native99-web-panel-20261019，释放14,056,359字节；证据retention{,-report}.log及artifacts/rollback-retention/的本轮收据。失败102证据、prepared、SDK和注册worktree均保留。当前实际部署备份：

- p09-native104-panel103-native-controls-20261023
- p08r-native103-panel103-ui-register-20261023
- p08-native102-panel103-auxiliary-20261023
- p07-native101-panel102-emitter-ui-20261019
- p05-native100-panel101-menu-20261019

以下P-08R及更早发布收据属于历史配对；其“最近回退”等描述只适用于当时版本。

## P-08R 选中效果图层崩溃（103已部署，待原步骤复测）

Owner报告native102在选中添加了效果的图层时崩溃。已只读复制最新Sentry报告353c795d-e977-4abf-a7a7-88a647eb8798.dmp，SHA256 `4F2D316DB0ECAE4216CD9A65899555BF68641FE22A3AB2A593F7B74628509028`。异常线程37868、0xc0000005读取地址0，异常地址AfterFXLib+0x193d9ab。报告包含native102项目模块；异常线程原始栈扫描没有形成已展开的插件调用栈，因此不宣称dump已证明根因。

AE0下已通过Restore-TestBuild配对恢复native101/panel102并核验全部原生哈希；失败103前端源码已保存在P-08备份的candidate-panel，native102二进制冻结候选和PDB均保留。没有修改原始Adobe报告、注册表或共享缓存，没有启动/停止进程。证据在artifacts/p08-crash-20261023/，恢复日志restore{,-preflight}.log。下文P-08安装记录现在是已撤回发布的历史收据，不能作为当前安装状态。

源码确认：Emitter/Force/Material/Model在102新增CUSTOM_UI/PUI_TOPIC后，没有注册Effect-window自绘UI。新增实际register_ui回调断言在修复前Emitter失败（emitter-before.log）；103补齐缺失注册并传播失败。定位绘制同时增加context/draw/supplier/surface空引用检查与文字能力检查，四类节点实际事件夹具覆盖这些空引用路径。此为确定的源码缺陷修正；选中图层崩溃是否消失仍需实际AE复测。Core求值和参数/ABI未改。

native103完整May2023 SDK / x64 Release构建通过，两个NoPublish开关；最终四类节点Emitter/Particle/Force/Transform分别94/99/90/110项、0失败，覆盖实际register_ui一次注册、注册错误传播、无context及SDK成功返回null draw/supplier/surface时不再调用下一级API、suite成对释放。未启动AE复现，不将测试通过视为宿主崩溃已消失。

重新只读确认AE0后，按standing授权将修复native103与冻结panel103完整部署，11个原生/Core安装与旧备份、26个前端103/回滚102文件、Junction读取及Core selector哈希核验通过；Restore只读预检通过。证据为artifacts/p08-crash-20261023/native103-deploy-{before,after}.json、deploy{,-report}.log，冻结候选位于artifacts/prepared/p08r-native103-panel103/。Core DLL哈希与失败102中的Core完全相同，本轮只修原生UI，保留Auxiliary有限回连与参数差异。无注册表、进程控制或共享缓存变化。

- Installed Emitter SHA256：`B3548129311CC390069E8621A192E589CC19A29EF49749FEB0AED80862D2B909`
- Installed Host SHA256：`57B6FAD8711E7CC189356BC63625338D3EE6C31EDC7AA4EAB51380D3D40793CB`
- Installed Core SHA256：`0014A659EA279FC38E1CAB1DE6B063CBFA6C3A631B4EB653AA9861AE003349AC`
- Selector：`StarfieldCore-0014A659EA279FC3.dll`

最近回退基线仍为 **native101/panel102**，使用新备份 `p08r-native103-panel103-ui-register-20261023`；AE关闭时一步恢复：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p08r-native103-panel103-ui-register-20261023' -Restore
```

待owner用原工程重现步骤复测：打开工程、选中含效果图层，再点选各节点显示定位线。实际结果未反馈前，本次崩溃门保持开放。失败102的dump/PDB/二进制及前端均保留，不建议恢复失败102。

## P-08 Auxiliary 身份、外观复用回连及定位线（102已撤回，历史收据）

Owner 提供实际截图，指出P-07仍显示Emitter名称/Point类型，并确认Particle ↔ Auxiliary的参考行为是一代子粒子复用同一Particle外观。此前P-07的离线通过没有完成此宿主验收，不能视为问题已经解决。当前源码改为新建Auxiliary原生命名；Auxiliary隐藏Type、Size X/Y/Z、Disc Size和形状Angle X/Y/Z，保留Origin、速度、方向/Orient与已有继承控制。静态和历史求值把直接互连的Particle→Auxiliary视为父引用，父图排除该Auxiliary自身输出；普通循环仍拒绝。保存原节点/边身份与参数，不新增虚拟Particle或修改Core ABI9。

Owner追加蓝青色定位线需求。native102的可编辑节点在首个可见参数标题区域绘制定位线，通过现有Reveal命令记录会话内UUID和Layer ID并查询，不写工程参数/撤销记录。Emitter定位改用始终可见的Emitting，避免隐藏Type后无法滚动。实际AE的标题区宽度、选中切换刷新和选择集合分支尚待验证；Output现有UI未改。参考中的Speed Over Life、Inertia、Life/Color Over Life继承等未实现控件没有被加为空选项。

May2023 SDK / x64 Release完整候选构建通过，PiPL/runtime版本102、packed32870，editable节点CUSTOM_UI声明同步；两个NoPublish开关保持。前端generation103须在完整配对部署后重开面板。Core当前节点463项、原生同步20,972项及Camera capture12项通过；Emitter/Particle/Force/Transform注册与透传87/92/83/103项通过。实际gateway新增Auxiliary命名回归、四种Emitting下显隐、静态/随机寿命历史求值与有限等价图、序列化回读通过。最后一轮面板54个suite中53通过；webview_retirement_tests因AE仍运行被既有进程守卫拒绝，不记为全绿，也未停止AE。证据在artifacts/p08-auxiliary-20261023/。

Core基础11,895项回归通过。完整11个原生/Core文件、adapter输入指纹与前端103已冻结在artifacts/prepared/p08-native102-panel103/，candidate.json记录当前HEAD及包含未提交源码的事实，不冒充干净提交产物。

最初构建时只读检查AfterFX PID3124运行，因此没有部署；installed-state.json是当时native101的核验记录。Owner随后明确关闭AE并要求发布，本轮重新确认AfterFX/AfterFX_64数量0，核验冻结候选与构建产物、前端一致后，通过Deploy-TestBuild.ps1发布完整native102/panel103。此前唯一被AE进程守卫阻止的webview_retirement_tests已在AE0下补跑通过，日志retirement-ae0.log；没有将旧聚合失败日志改为成功，也没有操作真实CEP注册。

11个原生/Core候选、dist、既有Junction读取及旧备份哈希，26个panel103当前文件与panel102回滚快照，以及Core选择器均核验通过。保留原单个Plug-ins/Starfield -> dist Junction；未启动/停止任何AE进程，未修改注册表或共享缓存。实际AE中的Auxiliary名称/参数、共享Particle回连渲染、蓝青线宽度与选择刷新、撤销/保存重开仍待owner实测。

- Installed Emitter SHA256：`5BAEB07F999D93EAA9876780F2BA96D2B27D888E7A526BA158735468167B3BC8`
- Installed Host SHA256：`4A9B16EF2D97ABC3C3E45A8C7600BFA9B03EB8C72621D0CC4DEE8124703434D9`
- Installed Core SHA256：`0014A659EA279FC38E1CAB1DE6B063CBFA6C3A631B4EB653AA9861AE003349AC`
- Selector：`StarfieldCore-0014A659EA279FC3.dll`
- 证据：artifacts/p08-auxiliary-20261023/native102-deploy-{before,after}.json、deploy{,-report}.log。

P-08当时回退基线为 **native101/panel102**，备份 `p08-native102-panel103-auxiliary-20261023`；此回退已在崩溃后执行，候选源码已保留，不能把以下已使用入口作为当前的一步恢复。当前入口见P-08R。历史命令：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p08-native102-panel103-auxiliary-20261023' -Restore
```

包含新回连语义的工程不能假定可由旧Core接受；回退前保留工程副本。配对恢复先保留103前端，再还原原生101与前端102，前端后续有修改时会拒绝覆盖。

## P-07 Emitter / Auxiliary 原生选项区分（已部署，待 AE 复测）

Owner 指出两者选项完全相同，并在本轮确认关闭 AE、要求部署。native101 保留共享 Emitter AEX，普通 Emitter 隐藏 Emit Chance、Emit Life Start/End、Inherit Velocity/Size/Opacity/Color 七项；Auxiliary 显示它们。共用发射参数继续保留。按既有 Auxiliary Source 区分，与 Emitting 的四种时间模式独立；无参数 ID、顺序、类型、schema 或 Core ABI 迁移。同步守卫在预设写入/回滚期间开放这七项，释放后恢复模式显示。Emitter 的 UPDATE_PARAMS_UI selector、PiPL/runtime SEND_UPDATE_PARAMS_UI 和构建校验映射同步更新。

May2023 SDK / x64 Release 完整 BuildWindows（两个 NoPublish 开关）通过。Emitter/Particle/Force/Transform 节点注册与透传分别87/92/83/103项，实际原生同步夹具20,796项及 Camera capture12项通过；native record契约710项和当前节点面板检查通过。覆盖四种 Emitting 下的源区分、真实监督守卫开关、备份/回滚守卫、重复刷新、无值写入、局部参数交付、可选 suite 缺失和资源释放。证据在 artifacts/p07-emitter-ui-20261019/。初次通用 NativeSync runner 因既有缺失链接输入失败，保留 native-sync-tests.log；成功结果来自已有完整 runner `RunCloudNativeSyncTests.ps1 -Run -Bindings`，未修改/掩盖该旧入口。初次构建的 PiPL 标志映射遗漏已补齐，最终日志为 native101-build-final.log。

最初检查 AE 仍运行，仅构建候选；owner 随后关闭，部署脚本重新只读确认 AE0 后通过 Deploy-TestBuild.ps1 完整发布，保留单个既有 Plug-ins/Starfield -> dist Junction。11个候选/安装/Junction读取/旧备份 native/Core哈希、26个 panel102当前/备份文件及 Core selector 已核验；Restore 只读预检通过。P-07本身只改原生显示；完整构建包含本 checkout 已有在途 Core 修改，不宣称与 native100 的 Core 二进制或算法逐字相同，也不据此补记 GOV 任务宿主验收。没有启动/停止 AE、修改注册表、CEP注册或共享缓存。

- Installed Emitter SHA256：`0DFA9D69E01A374D38A7D55907149584A09ECB531AE11D5D37F0C6A0288ADD78`
- Installed Host SHA256：`55D1C11756EE827275C6CDA1DA9C74CE8FAB12E5A4975B2AC6FB7909EB841553`
- Installed Core SHA256：`49110EF4522A32EE626467BB39D9D116089E8E11530A5BD103BD93C1F7424CFA`
- Selector：`StarfieldCore-49110EF4522A32EE.dll`
- 配对证据：artifacts/p07-emitter-ui-20261019/native101-deploy-{before,after}.json、native101-deploy.log。

P-07当时回退基线为 **native100/panel102**，备份 `p07-native101-panel102-emitter-ui-20261019`；当前最近基线见P-08。历史一步恢复（AE关闭）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'p07-native101-panel102-emitter-ui-20261019' -Restore
```

真实 AE2023 的控件初次显示、Auxiliary继承编辑、预设/复制、撤销/重做和保存重开尚待 owner 复测；部署成功不等于宿主资格通过。P-05菜单与P-06拖动修复的专项门仍保留。

## P-06 字符串快照校验小修

当前前端源码为 panel102。Owner 报告拖动节点后 `invalid_graph_snapshot: Maximum call stack size exceeded`，未观察到具体影响。离线事务夹具已复现完全相同的错误：图包含非空 Motion Starting With 字符串时，客户端回执比较器把字符串当数组，递归到单字符后无法终止。错误发生在宿主成功返回提交回执之后，解释了位置可能已保存却仍显示失败；尚未检查 owner 工程，不能据此断言该工程的持久化状态。

修复只让字符串精确、区分大小写比较，保留数值容差、原生提交/回读流程、节点 IDs/schema、Core ABI9 与渲染。缓存/manifest/gateway 标记同步至102。回归覆盖空、单字符、多字符和中文前缀的 moveNodes 提交、位置与前缀读回，以及前缀大小写/内容变化时拒绝回执。最终 54 个面板 suite 全通过，证据在 artifacts/p06-snapshot-20261019/{before,panel102-tests}.log；这些是离线检查，不是 AE 实测。

P-06当时只读检查发现 AfterFX 正在运行，没有执行 Deploy/Restore、重建或替换 AEX/Core。原生 WebPanelHost 直接映射 web_panel，源码102在面板重新创建/加载后生效。P-06当时没有部署备份；本页后续P-07已将panel102纳入完整配对快照。待 owner 确认非空 Starting With 图中拖动无报错、位置读回及撤销/重做；保存/重开工程仍待实际验证。

## P-05 部署基线与既有验证边界（历史配对）

以下保留P-05当时的部署基线，不是当前native101安装收据。源码位于 `codex/p05-panel-menu-cleanup`，现含P-06/P-07；P-04容器/目录迁移和CEP退役此前已合入本地main。P-05已完成实现、离线检查与配对部署，等待owner在AE中复测后再整合；尚未推送远端。

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

P-05当时的回退基线为 **native99/panel100**，备份 `p05-native100-panel101-menu-20261019`。此为更旧的回退入口，当前最近基线见本页P-07。历史一步回退（AE 关闭）：

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

按owner最近五版策略，P-08R发布后先报告，再仅移除最旧的p04-native98-modal-exit-20261019，释放14,053,240字节。证据在artifacts/rollback-retention/20261023-111810186及artifacts/p08-crash-20261023/retention{,-report}.log。失败102备份、dump/PDB、prepared与其他证据未删。此前清理证据保持历史范围。P-08R发布后当时的实际部署备份为（已由上方P-09更新）：

- p08r-native103-panel103-ui-register-20261023
- p08-native102-panel103-auxiliary-20261023
- p07-native101-panel102-emitter-ui-20261019
- p05-native100-panel101-menu-20261019
- p04-native99-web-panel-20261019

SDK、prepared、当前运行文件、事故/测试证据和注册 worktree 未因保留整理而删除。已删除旧包的历史命令不再是恢复入口。
