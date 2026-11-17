# STB-21：Particle Over Life 与拖动提交

## Owner验收

2026-11-17，Owner确认“问题已解决”，要求当前版本同步main并准备新版本开发。结合本轮此前逐项反馈，native136/panel124的三条Over Life生效、首次加点连续拖动与松手曲线闪回原问题关闭。未补充精确AE2023 Build、完整Undo/save/reopen结果或MFR资格，不将该确认扩展至这些门。以下实施日志保留当时的待验状态，最终结论以本段为准。

## 136续修：事件返回后的重绘顺序

Owner已确认135编辑体验正确，但曲线框线条松手闪回仍在。135保护只在发布调用内，随后通用invalidate仍设置PF_EO_UPDATE_NOW；May2023 AE_EffectUI.h明确此位在事件返回后立即更新视图，因此上一版的调用内测试未覆盖这一段。

136成功提交仍调用PF_InvalidateRect，但清除UPDATE_NOW，且把该通知放入原预览作用域以覆盖其同步重入。随后直接返回，不再经过末尾的立即invalidate。拖动中仍立即显示，取消/失败也立即绘制恢复值，FORCE_RERENDER与参数change flags保持。没有新延时、循环重试或跨事件显示覆盖层，Undo/外部作者值仍直接可见。

时序夹具独立模型：提交内部DRAW、失效通知同步DRAW、事件返回后若UPDATE_NOW则用旧交付参数画、正常队列在采纳后画。真实绘制路径对冻结135为4,135检查9失败，新普通/ASan各4,078通过；模型不证明AE内部确实按某个固定顺序执行，现场136仍需原操作复测。原有UI/native/20预设/采样/Core链路普通/ASan各4,529、Particle注册642、实际冻结135/136 DLL2,286、九效果PiPL/runtime333及完整SDK136通过。原有C6237/C6326与bitmap C6262 54,644字节仍在，不记全项目静态通过。

17:50（+08）freshAE0经原工具与单Junction安装136/124；前1,089/后1,100共2,189哈希0不符，223构建输入、11二进制、29前端、265测试源和135回退核验。冻结`artifacts/prepared/stb21flash-native136-panel124-v1`，manifest `1A8E29CEA1D350A2420B65ECF2551D430E8383C840A701D287F4B88A3201F529`；Core hash `16621C454B88637E280D05A083F633BA15CEEA92C2F39CAC4FFCF4BD7052E676`，selector `StarfieldCore-16621C454B88637E.dll`。Core源/ABI、Render.hpp、参数/布局/schema、135手势与134提交边界、panel124及MFR实验配置保持。

日志/命令及修前UI在`artifacts/stb-21flash-20261117/`。备份`artifacts/disabled/stb21flash-native136-panel124-deferred-redraw-20261117`完整保留135/124，回退见current-state，本轮未执行Rollback。五份策略report-first只删除旧实际备份`mfr01-native132-panel124-owner-test-20261116`的16,242,143字节，证据`artifacts/rollback-retention/20261117-175103991/`，候选/SDK/证据未删，未启动/退出AE。

## 135交互续修：连续加点、提交期间显示与局部刷新

Owner确认134 Over Life生效，但新点需二次点选，松手闪回旧图后延迟刷新。135让新点立即绑定当前drag continuation，初次按下/移动仍为局部草稿，松手一次完整发布。发布期间用context/UUID/bank-keyed拥有值预览画期望曲线，旧或混合PF数据的重入DRAW不再闪回；token作用域结束即移除，不跨调用遮挡Undo/外部修改。该预览不参与Core或作者校验，重入编辑不启动第二次发布，失败仍恢复PF参数并绘制原曲线。成功提交保留FORCE_RERENDER，改用现有局部Invalidate，不再设置会重绘整个ECW/Layer/Comp的REFRESH_UI。

最终编辑器夹具冻结134为3,900项22失败，新普通/ASan各3,955零失败；覆盖三曲线首次加点拖动、旧/部分宿主bank重入、失败恢复与Undo。原有20预设实际编辑/native/采样/Core链路普通/ASan各4,529、Particle注册642、冻结134/135 DLL2,286（三精度/并发/取消）、实际九效果PiPL/runtime333及May2023 SDK135通过。严格分析仍有原有NodeGraphSync C6237/C6326与渐变bitmap C6262 54,644字节，未记整体通过。

证据`artifacts/stb-21ui-20261117/`包括旧UI、同一最终测试的before/after及ASan、SDK与安装核验。保存的134现场日志有8次曲线source服务19.436–20.820ms，compile0.548–0.832ms，只描述原生发布服务，未测AE输入到最终渲染/回显延迟。

12:32（+08）freshAE0安装135/124，前1,089/后1,100共2,189哈希0不符；223输入、11二进制、29前端、265测试源及134回退核验。冻结`artifacts/prepared/stb21ui-native135-panel124-v1`，manifest `AE4C3A546873EB173B76E2405FF458BFD67B4473846AED447567D129C61B48CB`；Core hash `8B0B1DE4318CD0C70E29C53F455AF67BD1194DB45D4460FED9D19AF0FECA7D4A`，selector `StarfieldCore-8B0B1DE4318CD0C7.dll`。Core源/ABI、公开参数与布局/schema、134拥有值提交边界和panel124保持；MFR仍实验配置，完整宿主资格开放。

备份`artifacts/disabled/stb21ui-native135-panel124-curve-gesture-20261117`保留完整134/124；准确回退见current-state，本轮未执行Rollback。五份策略先报告后仅删除一个旧实际部署备份17,783,085字节，证据`artifacts/rollback-retention/20261117-123306953/`。未启动/退出AE；实际135连续加点、松手回显、Undo/save/reopen及端到端延迟仍由Owner验收。

## 134续修：预设Apply的拥有值提交入口

Owner确认133仍无效，预设Apply弹SF-T202/AE516；下文133记录不能作为故障修复验收。134/124已于12:06（+08）freshAE0部署，实际AE效果待Owner。

133把PF自定义EVENT编辑送入普通USER_CHANGED_PARAM的数组数量检查。离线给完整曲线参数组、不同delivered计数可复现未发布；截图只有通用错误，没有实际计数，不能据此宣称已证明现场唯一根因。134新增私有sync_custom_node_graph_bank：编辑器先捕获固定拥有值（类型、count、全部曲线或渐变字段及UUID），请求不带params[]，不伪造num_params；普通监督回调的数组门保持。编译仍从当前宿主重新核对source kind/UUID/guard、renderer guard与事务互斥，未知或部分参数组先拒绝，无新fallback或自动重试。早期错误SF-T203给出具体阶段、kind、参数与delivered，失败仍恢复原PF数据。

| 本轮检查 | 结果 |
| --- | --- |
| 20真实catalog预设×3曲线×3delivered计数 | 原133矩阵243失败（240项拒绝/图不匹配、3项旧诊断差异）；新入口通过 |
| 真实编辑/原生发布/采样/Core链路，含上述预设矩阵及请求保护 | 普通/ASan最终各4,529；选择器为受控替身，不开Win32窗口 |
| 编辑器与共享Motion/Turbulence参数组 | 普通/ASan各3,708 |
| Particle实际注册 | 642通过 |
| 原生绑定ASan/Camera | 48,165 / 12通过 |
| 冻结133/134实际Core DLL | 2,286通过，36串行配对、400并行帧、24取消及三精度字节对照 |
| 实际九效果PiPL/runtime与完整May2023 SDK134 | 333及完整构建通过；版本32902，MFR实验配置保留 |
| 严格/analyze /WX | 原有C6237/C6326及bitmap C6262 54,644字节仍在；未发现新增诊断，不称全部通过 |

缺字段、空UUID、跨类型、混合参数组在宿主访问前拒绝；源UUID变化、已有事务和renderer guard也拒绝，revision和值保持。自定义入口不接受调用者的宿主句柄或回执，仅复制作者数据。普通监督回调仍按原数量检查。Render.hpp、Core数值输入/ABI9、公开ID、布局及schema不变。

证据`artifacts/stb-21r-20261117/`，修前源码与矩阵测试快照、修前/修后日志均保留。冻结`artifacts/prepared/stb21r-native134-panel124-v1`的manifest SHA256 `C46FD4EF3AC02629F23E1DFF713607649A8086142C5AB6B66E700F456477B435`，223构建输入/11二进制/29面板/265测试源及catalog/schema额外核对。部署前1,089/后1,100共2,189项0不符；新selector `StarfieldCore-2FD7180673DC3711.dll`，Core SHA256 `2FD7180673DC37115646181FF1C9A1F86AA24B062E72605D6EDB1DB16E54E6FE`。前端124与Core源不变，新DLL另行字节对照。

备份`artifacts/disabled/stb21r-native134-panel124-overlife-20261117`为完整133/124，准确回退命令见current-state。本轮没有执行Rollback。五份政策先报告后仅裁剪旧实际备份`ux16-native129-panel123-initial-render-20261116`，未删候选/SDK/证据，收据`artifacts/rollback-retention/20261117-120701410/`。代理未启动/退出AE；请Owner复测原预设Apply、拖动、Undo/save/reopen，完整MFR资格仍开放。

## 133阶段历史

Owner确认132的MFR警告消失，同时报告Size、Opacity、Rotation Over Life无效，拖曲线卡顿。当前133/124已部署；AE2023的具体Build、原曲线操作、Undo/save/reopen及体感延迟仍待Owner复测。

## 已确认的代码缺陷与修正

普通Linear/Hold/Bezier拖动逐事件写回参数并发布图；Draw已有拥有值草稿。三条曲线、四种插值现在共用草稿：按下并拖动只重绘局部曲线，最终last_time一次发布，宿主渲染在松手后更新。无移动不提交；取消/上下文关闭/外部修改丢弃草稿，迟到release不会启动新编辑；失败恢复完整PF参数组与原change flags。

自定义参数组的同步入口曾在guard/部分上下文/缺少目标时返回成功。编辑器因此可能让AE保存新参数组，而主效果图未发布。实际生产UI+native夹具在三曲线guard场景复现6项失败；现要求明确失败并恢复原参数组，无具体错误时显示SF-T202。普通监督参数回调仍按原协议延后。该夹具证明确定缺陷，未证明Owner现场走的是这个分支，不以此关闭宿主故障门。

不改变公开参数、ID、布局、序列schema、Core ABI9、Render.hpp或Core数值算法；MFR实验配置继续沿用，默认构建仍关闭。

## 离线证据

全部命令、日志、修前源与收据在`artifacts/stb-21-20261117/`。

| 检查 | 本轮结果 |
| --- | --- |
| GradientEditor普通/ASan | 各3,731通过；三曲线、四插值、局部草稿、无移动、取消、外部修改与失败回滚 |
| 真实UI→native commit→SmartFX式alias采样→Core粒子属性 | 普通/ASan各1,099通过；同一最终夹具修前6失败，三个guard分支未发布/未恢复 |
| 原生绑定ASan/Camera | 48,165 / 12通过 |
| 实际冻结132/133 Core DLL | 2,286通过，36串行配对、400并行帧、24取消，RGBA8/16/32F精确字节与generation释放 |
| 实际九效果AEX PiPL/runtime | 333通过，版本32901与MFR位一致 |
| May2023 SDK133完整隔离构建 | 通过，MfrQualification/RenderJournal/双NoPublish；PiPL工具9020沿既有受证缓存转换回退，实际二进制另行核对 |
| 严格/analyze /WX | 未通过：NodeGraphSync既有C6237/C6326常量分支、渐变bitmap既有C6262 54,644字节栈，修前/修后同现；不抑制警告、不声称全项目通过 |

常用回归：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1 -GradientEditor
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCloudNativeSyncTests.ps1 -Run -OverLife
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCloudNativeSyncTests.ps1 -Run -OverLife -Sanitize
```

RunAll的All/Cpp/NativeRegression已注册OverLife实际执行，默认报告不算通过。调用量下降仅证明移除了逐移动重编译，不给出未经AE测量的毫秒收益。

## 配对与回退

11:32（+08）最终freshAE0，经原Deploy-TestBuild工具与既有单Junction安装133/124。候选`artifacts/prepared/stb21-native133-panel124-v1`冻结223构建输入、11二进制、29前端和265测试源；manifest SHA256 `404610919617F88C0990ED8C5EE052C1318C27F099553289B6C054162795562F`。前1,085/后1,096次共2,181哈希0不符，安装/Junction/selector和132/124备份核验。前端字节与Core输入不变；新的Core构建字节以实际DLL对照覆盖。未启动、停止或操作AE。

当前selector `StarfieldCore-25B8360A1A596D35.dll`，Core SHA256 `25B8360A1A596D357F4B0ED21D01EF66014A02627877C89A242463E2252581CF`。备份`artifacts/disabled/stb21-native133-panel124-overlife-final-20261117`保留完整132/124；回退命令见current-state。保留最新五个实际部署备份，report-first分两轮仅移除两个旧实际备份，候选/SDK/证据均保留。

部署纠正记录：首次11:28安装通过后，代理核对回退时误执行了实际Rollback（日志`rollback-report.log`）；随后freshAE0以新备份名重新安装相同冻结133，11:32再核验1,096项0不符。`after-initial.json`保留首次收据，最终以`after.json`和`deploy-final-action.log`为准。两轮report-first保留最新五份，合计仅裁剪两个旧实际备份35,562,381字节；详细路径见current-state。期间AE均未运行，未启动/停止宿主。

Owner复测时在有存活且年龄不同的粒子帧拖动Size/Opacity/Rotation曲线：拖动时曲线应流畅跟手，松手后粒子更新；确认Undo、保存重开与不同曲线切换。具体AE Build/结果另记，不据本轮离线测试认定完整MFR支持。
