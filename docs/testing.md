# 测试入口

INT-01验收记录（2026-11-17）：Owner确认136/124当前原问题已解决，STB-21关闭；覆盖此前反馈的三条Over Life生效、首次加点连续编辑与松手曲线闪回。具体AE2023 Build未补充，完整MFR、Undo/save/reopen全矩阵仍按各自门记录。以下逐版离线数据和部署证据保留历史范围。本次main同步不重建或部署，独立检查日志位于`artifacts/main-sync-20261117/`。

INT-01本次回归：Core11,895、编辑器4,078、真实Over Life链路4,529及panel124合同通过；面板75套中74通过，旧CEP退役夹具因Junction EPERM及Windows PowerShell子进程4294967295未完成，不称全过。详见[同步记录](engineering/main-sync-20261117.md)。

STB-21重绘顺序/136（2026-11-17）：Owner确认135编辑正确但闪回仍在。新增事件返回后UPDATE_NOW先收到旧参数、正常排队绘制收到采纳值，以及失效通知自身重入的独立模型。冻结135为4,135检查9失败；`RunCoreTests.ps1 -GradientEditor`普通/ASan各4,078通过。真实OverLife链路普通/ASan各4,529、Particle注册642、实际冻结135/136 DLL2,286、实际AEX333与May2023 SDK136通过。原分析C6237/C6326/C6262保留，未称整体通过。证据`artifacts/stb-21flash-20261117/`；136/124已freshAE0部署、2,189哈希核验及135回退保留，AE真实松手显示仍待Owner。

STB-21交互/135（2026-11-17）：Owner确认134生效，补首次插入即拖动、重入DRAW旧/部分参数组、局部刷新、失败后恢复绘制与Undo覆盖层清理。最终同源编辑夹具冻结134为3,900/22失败；`RunCoreTests.ps1 -GradientEditor`普通/ASan各3,955通过。真实OverLife链路普通/ASan各4,529、Particle注册642、实际冻结134/135 DLL2,286、实际AEX333及完整SDK135通过。原有分析C6237/C6326/C6262仍在，未报整体通过。所有日志/命令、旧源码、134现场日志只读快照与8次source服务计时在`artifacts/stb-21ui-20261117/`；135/124已freshAE0部署，现场端到端响应不由19.436–20.820ms原生服务样本代替。

STB-21续修/134（2026-11-17）：Owner确认133仍失败，故补真实20个preset catalog×3曲线×delivered 0/1/完整数量矩阵，旧133矩阵243失败；`RunCloudNativeSyncTests.ps1 -Run -OverLife`普通/ASan最终各4,529通过（额外含拥有值请求的10项保护检查）。选择器用受控替身，实际编辑/提交保留生产实现；未运行Win32选择器或AE。`RunCoreTests.ps1 -GradientEditor`普通/ASan各3,708（含共享Motion/Turbulence），Particle注册642；原生绑定ASan48,165/Camera12、冻结133/134实际DLL2,286、实际AEX333和完整SDK134通过。分析仍有原有C6237/C6326/C6262，未报整体静态通过。日志、原133源码、矩阵测试快照、命令和2,189项部署前后核验在`artifacts/stb-21r-20261117/`，134/124已freshAE0部署，真实原步骤待Owner。API边界见ADR0023。

STB-21/2026-11-17：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1 -GradientEditor`普通与独立ASan各3,731通过；`tests/RunCloudNativeSyncTests.ps1 -Run -OverLife`及`-Sanitize`各1,099，通过真实UI→native commit→常量参数组采样→Core粒子size/opacity/rotation，旧源码同夹具6失败。OverLife已接入RunAll的All/Cpp/NativeRegression。原生`-Run -Bindings -Sanitize`48,165与Camera12通过；冻结132/133实际DLL对照2,286、九效果实际AEX PiPL/runtime333与完整SDK133通过。严格/analyze /WX保留C6237/C6326和渐变bitmap C6262（54,644字节），修前/修后同现，不报整体静态通过。日志/命令与冻结/部署前后收据在`artifacts/stb-21-20261117/`；详细范围见[Over Life说明](engineering/particle-overlife-20261117.md)。133/124已freshAE0部署，真实AE曲线及延迟仍需Owner复测，警告消失不等同MFR全部资格。

132实机交付2026-11-16：Owner确认前两项原问题已测无误后要求部署；freshAE0完整部署132/124，前559/后570共1,129次哈希0不符，223输入及11安装二进制匹配冻结132、29panel124保持；129/124备份11文件与selector核验。未重建/启动AE，不新增宿主通过结论。实际备份保留五份按report-first完成；部署证据artifacts/mfr-deploy-20261116，当前状态/清单已更新，Owner从M01开始。

Owner手工交接2026-11-16：最新证据 `artifacts/mfr-offline-handoff-20261116/`。`RunPanelTests.cjs`75套全部通过（含之前受沙箱限制的retirement Junction项）；GPU strict/ASan各24,043、`RunCoreTests.ps1 -GpuConcurrent`477（CUDA/OpenCL各96渲染/透传/取消）、原生绑定ASan48,165/Camera12、历史缓存ASan131通过。新增 `RunCoreDllConcurrencyTests.ps1 -Run` 对冻结129/132实际DLL2,286检查通过，36串行配对/400并行帧/24取消/三精度与所属generation释放；四个UX-16自主生成图输入固定哈希。新增 `RunMfrJournalTests.ps1 -Run` 实际生产日志头194记录/97区间并发/嵌套GPU异常释放通过，独立日志不写AE资格日志。冻结AEX开/关各333项；ES3宿主脚本矩阵和证据反例通过。没有AE启动/部署/实现变更；实机事项见engineering/ae2023-owner-test-checklist-20261116.md，未测试项不冒称通过。

MFR-01第二阶段2026-11-16：`RunGpuConcurrencyTests.ps1 -Run -Analyze`和`-Sanitize`各24,043项通过，含4线程400渲染+400透传、双向宿主回调重入、取消、部分分配/上传/内核参数/enqueue/sync故障与释放；冻结130旧GPU源码被同一测试拒绝。`RunCoreTests.ps1 -GpuConcurrent`477项通过，真实CUDA/OpenCL各96帧渲染+透传+取消，与同驱动串行逐字节一致、CPU沿用既有0.0002容差。首轮新测试错误地设2e-6容差，诊断为2.2..2.8e-6浮点差，失败日志保留；修正为既有标准并新增严格串行字节对照，而非修改Core/GPU数学。May2023 SDK131和隔离MFR132完整NoPublish构建通过；`RunMfrBinaryFlagsTests.ps1 -Run`及131 `-Disabled -PackedVersion 32899`各333项实际AEX内嵌PiPL/GLOBAL_SETUP检查通过。脚本ES3/真实宿主脚本离线矩阵、证据检查器反例通过；部署/回滚隔离测试在允许仓库Junction后通过，初次沙箱拒绝保留。实际AE尚未运行；PNG smoke不代表完整精度或资源族资格。

MFR-01 2026-11-16（前置候选，未启用MFR/未部署）：`RunNativeTemporalCacheTests.ps1 -Run -Analyze`与`-Sanitize`各131检查通过，包含4线程800乱序帧与未缓存积分/出生时间精确对照、请求隔离/旧完成拒绝/失效；旧代码新增场景失败日志保留。`RunCloudNativeSyncTests.ps1 -Run -Bindings -Sanitize`48,165/Camera12通过；May2023完整SDK130以NoRuntimePublish/NoDistPublish/RenderJournal构建通过，初次PiPL9020保留。没有真实AE或GPU/MFR资格，不把这些检查写成MFR支持；记录artifacts/mfr-01-20261116及准入矩阵。

STB-20 2026-11-16：冻结129/124 v2，`node tests/RunPanelTests.cjs`为73通过、1旧CEP模拟Junction测试受沙箱EPERM阻止。新增`host_json_startup_tests.js`在无/不可信全局JSON的实际gateway/两页面生成脚本中通过6,152项检查，含UTF-8往返、严格语法/深度/循环/原型键、加载失败信息及全局不变；Model事务2,636、预设文件583在JSON缺失上下文通过；完整JSX以ES3/旧引擎运算符检查通过。v1代理码UTF-8反例、两项旧全局故障注入夹具失败均保留。无native/Core重建或AE实测，证据artifacts/stb-20-20261116及current-state。

UX-16 2026-11-16：`RunCoreTests.ps1 -Adapter`通过6,264检查；`RunCloudNativeSyncTests.ps1 -Run -Bindings`及`-Sanitize`各48,165/Camera12通过，含实际编译器与首次渲染投影54组合的编码粒子逐字节对照。GraphParameter.cpp/Parameters.cpp严格/analyze通过；May2023完整BuildWindows的NoPublish/RenderJournal构建通过，初次PiPL9020保留。`node tests/RunPanelTests.cjs`冻结129/123为72通过、1项旧CEP模拟Junction/PowerShell环境失败，未冒称全过；新panel_initialization_view回归覆盖首次确认绘制/位置/状态/暂停/已有模式，gateway覆盖新效果身份连续。实际两Core DLL36组CPU区域/步长/格式/像素对照通过。日志、冻结/部署/完整回退见current-state与artifacts/ux-16-20261116；没有AE现场资格。

PERF-17 2026-11-11：最终冻结128/121 v2全72组及版本合同通过；`canvas_gateway_performance_tests.js`执行实际JSX 8粒子/64点曲线对照，3552→112节点值读、0写；`panel_canvas_loading_tests.js`执行实际面板的只读首屏、布局/参数选择、按需一次完整读取、多意图/原参数、busy/失败/异目标/pagehide/活动手势门。v1 busy反例与最初AE运行阻断的旧CEP测试保留，关闭后补跑及v2全量通过。日志、731哈希核验与配对回退链见 `artifacts/perf-17-20261111/`，没有真实AE耗时测量。

PERF-16 2026-11-09：冻结128/120 v1通过全70套件及版本合同。`panel_selection_performance_tests.js`执行实际panel的轻探测/连续选择合并/focus/慢读/延后诊断/暂停/空目标/状态采样，最多一个宿主读；`panel_target_resolution_tests.js`在5万图层的实际JSX夹具验证selectedLayers不枚举、不读renderer或写宿主。日志、686哈希核验与128119回退见 `artifacts/perf-16-20261109/`；没有真实AE耗时测量。

UX-15 2026-11-09：冻结128/119 v2通过 `node tests/RunPanelTests.cjs` 全69组及版本合同；`panel_target_follow_tests.js`执行实际面板事件/晚回执，`panel_target_resolution_tests.js`执行实际JSX目标解析。无目标普通状态与host_not_ready连接重试分别验证；v1跨工程同ID晚到响应反例与v2修复证据保留。日志、安装684哈希核验与128118回退见 `artifacts/ux-15-20261109/`，未进行实际AE操作。

UX-14 2026-11-09：冻结128/118通过 `node tests/RunPanelTests.cjs` 全67组，`tools/Assert-PanelContract.ps1`通过；完整日志与当前安装证据见 `artifacts/ux-14-20261109/`。`node_diagnostics_tests.js`验证本地分级/几何推导，`panel_node_warning_tests.js`执行实际面板DOM/异步诊断，`panel_topology_responsiveness_tests.js`包含无队列目标切换清理。AE运行时旧CEP回退套件会按保护拒绝，须AE关闭后重跑，不能绕过宿主保护。未运行真实AE冒烟。

当前结论与安装身份见 [current-state.md](current-state.md)。旧版逐次测试流水已完整移入 [历史测试记录](history/2026-10-19/pre-web-panel/docs/testing.md)，不沿用旧 passing counts 作为当前结果。

## 当前命令

UX-13（离线，不启动AE）：`node tests/RunPanelTests.cjs`发现全部面板套件（含333/500ms虚拟输入、真实面板失败/拖动/布局成员/黄警告/晃动重放）；`powershell -ExecutionPolicy Bypass -File tests/RunGraphTopologyTests.ps1 -Run`及`-Sanitize`验证真实Core预检/投影、数值硬失败与63作者/252边上限；`tests/RunNativeGraphEditTests.ps1 -Run -Sanitize`验证生产delta/Clear/CAS/等量替换/增长尾部补偿；`tests/RunWebPanelHostTests.ps1 -Run -Sanitize`、`tests/RunNativeClearHostTests.ps1 -Run -Analyze`验证真实Host入口与消息边界。`tests/RunGraphDiagnosticsSelectorTests.ps1 -Run`加载实际主AEX、用只读fake May2023 suites与poisoned params验证私有诊断selector；`tests/RunNativeSourceSelectorTests.ps1 -Run`、`tests/RunNativeSelectorAuditTests.ps1 -Run`加载实际八节点模块。`tests/RunCoreTests.ps1 -NativeSync`补齐既有Model/Light依赖后运行实际发布器/绑定回归。实际日志、独立审校、失败证据及最终冻结身份见 artifacts/ux-13-20261104；不可由这些命令推断真实AE无崩溃/吞吐/Undo/save/reopen资格。

PERF-11只读分析：`node artifacts/perf-11-20261031/prepare-budget.cjs`后从checkout执行`artifacts\perf-11-20261031\compile.cmd`完整编译实际生产发布器/绑定源及计数夹具，原生产品文件不改；最终2844检查0失败，含既有fixture基础checks。5操作计数见native-budget.json；新独立重放一致并补有限空槽值0对照，不叠加重复2844作新覆盖。67输入冻结核验，判定仅分析通过、尚非可实施合同；无AE运行/毫秒资格/部署。原始初次生成夹具签名错误及修后日志都在artifacts/perf-11-20261031，产品38hash仍与126/115一致。

PERF-10/126115：`node tests/RunPanelTests.cjs`最终56套通过，交付冻结副本同样56套；`node artifacts/perf-10-20261031/measure-read-budget.cjs`用同一生产gateway/DOM对照259→164标量读取，不报告AE毫秒。全新v6对冻结实际queue/gateway/panel/bridge等六入口和七个历史独立探针共13进程exit0、41唯一输入hash一致，补充pan异常恢复限制见其报告。v1-v5旧失败及候选保留，不把基础与独立项数叠加。freshAE0 KeepNative部署后1217hash验证及126114回退；无新SDK/AE运行，不改变原生126/69Core资格。最终日志与审校在artifacts/perf-10-20261031/{frozen-v3-final.log,reviewer-v6/review.md}，工程边界见[编辑性能说明](engineering/editor-performance-20261031.md)。

STB19/126114候选：旧125对象在纠正Force->Output的受控不可用源顺序下20660/1预期失败（missing-clear-before-active-force.log），不是实际AE执行。原七类fixture曾把Force停放而漏测，原始证据保留。最终`RunNativeGraphEditTests.ps1 -Run -Analyze`/`-Sanitize`各160255，`RunNativeClearHostTests.ps1`分析/ASan各4508（真实Host+执行器、fakeSDK/GUI）、既有Host各768、真实publisher普通/ASan Native27084+Camera12、55panel114与完整May2023 SDK126 NoPublish/RenderJournal通过。新请求SFC1/SFC2只借固定操作内数值，EffectMain限UI/same odd epoch，普通路径不放宽。新独立实现审校166输入当前/冻结一致，真实Host+执行器158及实际126 AEX准入19独立断言、冻结palette通过。2026-10-30 freshAE0部署126114并1338hash/125113完整回退核验；不能以离线证明实际Clear/Undo/重开或性能。证据artifacts/stb-19-20261030。

STB18/125113已部署：Owner确认125112删除仍失败，要求恢复单击编辑。最终`node tests/RunPanelTests.cjs`55套零失败，含实际palette的单击、两click+dblclick一次、A在途/B双击不丢队列、修饰选择取消旧意图及新增量删除路由。全新独立冻结实际panel33断言及palette通过、8当前输入hash一致；原生125十一文件逐字复用，未做新SDK编译。2026-10-30 freshAE0部署，1174hash验证与125112回退保留，UI回退不能当原错误修复。日志/证据artifacts/stb-18-20261030。

STB17/125112候选：`RunNativeGraphEditTests.ps1 -Run -Analyze`及`-Sanitize`各116366检查零失败；真实删除执行器覆盖八类原生效果、多选、正ACK前源存活、写失败补偿、提交后错误、未知回执、whole EFFECT/STREAMREF与collection释放、target路径和已识别选择后的通知ID失败。`RunWebPanelHostTests.ps1 -Run -Analyze`及`-Sanitize`最终各768零失败，包含当前非root原生焦点和Edit文本/祖先Clear在capture/latch前透传。ASan runner仍有既有/PDB命令行警告，运行结果通过；前态夹具DestroyWindow空值分析警告及修后日志均保留。55panel112套、既有backup717555/Model945479/真实publisher Native24025+Camera12通过；最终May2023 SDK `-NoRuntimePublish -NoDistPublish -RenderJournal`成功，日志sdk125-final-v4-build.log。全新v4独立冻结21输入复核一致，生产执行器基础156634及独立53断言、Host路由18断言及冻结前端通过，无新可复现阻断。2026-10-30 freshAE0冻结部署125112，1187hash核对安装/冻结/审校输入与124111回退，Core逐字不变；不操作AE/工程，实际宿主门开放。证据artifacts/stb-17-20261029。

STB17调用量：reference-static/run-backup-probe.cmd链接实际EffectGraphBackup.cpp与既有假SDK；N=32旧prepare+discard有22492标量读/5016效果查询/33复制，新执行器编排872/39/0，均不含真实publisher工作，非AE耗时或端到端提速比。原生增量删除只迁移普通删除；其他编辑/复杂资源及一般外部删除生命周期仍开放，实际删除、Undo/重开和性能保持Owner验收门。

STB16/124111：55panel111套通过，首位真实panel旧编辑排队失败保留，修后新独立65/原palette/8输入一致。native11文件逐字复用124，没有新SDK构建；freshAE0部署并1164hash/124110回退核验。Owner已明确授权单击选择、双击编辑；原生效果直接删除及所有节点现场问题另卡继续，不宣称已修复。证据artifacts/stb-16-20261029。

STB15/124110：123现场PID7640实际45行记录Clear Delete1/gate3且local=GUI=foreground，原始53行另8条为父graph fake PID30276，已分离不改写。graph_commit_host_tests现显式NoJournal。旧控制器GotFocus/root原生焦点真实predicate726/1后修正，最终WebHost /analyze /WX与ASan各749、router46、graph56、55panel110及最终SDK124成功；全新独立Host792（含冻结基础、不与父叠加）/11输入一致，无新增明显阻断。中间744/SDK及fixture命名/C6387失败保留。冻结213adapter/69Core/3build/232validation/11binary/26panel，freshAE0部署并1167哈希/123110回退核验，实际事件顺序/Force/性能门开放。证据artifacts/stb-15-20261029。

STB14/123110：实际旧StarfieldHostEntry在Web初始化stub下注册6个CommandHook，14检查1失败；新`RunHostCommandRouterTests.ps1 -Run -Analyze`/`-Sanitize`各46，`RunWebPanelHostTests.ps1`分析/ASan各724，graph command56、HostBootstrap14、55panel110与SDK123 NoPublish/RenderJournal通过。新独立实际entry107（含46）、actualWeb730（含724）、10/10源hash一致，无新明显阻断；不将模拟计数等同AE覆盖语义。先定义全部会话ID后仅一次最终ALL注册，分发原内部handler及panel/Clear，失败bindings0。冻结213adapter/69Core/3build/232validation/11binary/26panel并freshAE0发布，1166hash与122110回退核验。现场Force/删除/性能门仍开放；证据artifacts/stb-14-20261029。

STB13/122110：真实旧predicate跨GUI队列null/stale焦点705/1失败后，局部GetGUIThreadInfo及诊断修正版Host /analyze /WX和ASan各720、55panel110零失败、完整SDK122 NoPublish/RenderJournal通过。新独立782（含720基础，不与780中间轮累加）及9/9源hash一致，覆盖foreground/active/root/queryfail/菜单/选择返回变更/Clear消费及1100重复上限1000，未发现新明显阻断。26panel/69Core源及DLL与121一致；freshAE0部署并1161哈希/121110回退核验，原Force实际AE门继续开放。日志/收据/审校在artifacts/stb-13-20261029。

STB12/121110：父真实Host分析/ASan各703，完整55suite最终零失败，SDK121 NoPublish/RenderJournal通过；独立第二位Host712、bridge/文本/实际panel46及9输入哈希一致，无新明显阻断。首审校Ctrl+D过期误删缺口局部修后交新审校；升110漏preset_manager版本的2失败已保留且修正配对。nativeDelete实际panel测试Force走完整事务1/普通提交0，pending/text/Output及拒绝保留原图通过。冻结213adapter/69Core/3build/230validation/11binary/26panel，23:41 freshAE0部署并核验1161哈希与120109回退。未操作AE/工程，实际钩子/快速按键/文本长按/Force原故障及性能仍开放。原始日志和审校见artifacts/stb-12-20261028。

STB11/120109：`RunWebPanelHostTests.ps1 -Run -Analyze`及`-Sanitize`最终各672通过，受控GetFocus/foreground/input查询+真实message-only HWND覆盖18场景，不移动用户真实焦点；第三位新独立focus699 /analyze /WX、实际panel/bridge metadata54、gateway802、journal3086/514记录通过且冻结16输入一致。父trace/fullgraph/Motion/delete-render/真实catalog的首次sync错误DOM502、55panel109及最终SDK120 NoPublish/RenderJournal通过。前两位审校前态654/1后台焦点、665/1 reparent反例保留；父首焦点夹具错用非数字请求ID和测试DestroyWindow可能null告警只修夹具，旧日志保留。119实测仍失败，267条日志15次516/零transaction未证明实际路径，120未作AE验收。22:57 freshAE0配对发布并1,168哈希验证/119108回退，证据artifacts/stb-11-20261028。

STB10/119108：旧实际worker读取guard0且缺Force的入口23,675项1失败（native-before.log），修后`RunCloudNativeSyncTests.ps1 -Run -Bindings`及`-Sanitize`各24,025、Camera各12通过；`RunModelUiExclusionTests.ps1 -Run`真实跨DLL1,212；`RunModelTransactionHostTests.ps1 -Run -Analyze` /WX9,664,745通过。新独立22,015并发与9,735,256实际Host变体覆盖停止、异常通知、错误token及普通UI不改epoch，冻结9/9审校输入一致。SDK119 NoPublish/RenderJournal、55panel108通过；Core69源和DLL与118一致。16:57 freshAE0部署119108并最终1,161哈希核验118107回退；实际AE删除/旧guard恢复/性能尚未验证。所有命令/日志与审校在artifacts/stb-10-20261028，当前身份见current-state。

STB09/2026-10-28离线候选：`STARFIELD_PANEL_ROOT=web_panel`及`STARFIELD_EXTENDSCRIPT_CONDITIONAL_MODEL=1`下以`node --expose-internals tests/model_graph_transport_tests.js`实际2,636项通过；VM不提供Array.indexOf，覆盖过期owned拒绝/迟到回执/终态和传输态替换，以及Turbulence合法与非有限/非法枚举。`extendscript_syntax_tests.js`52项覆盖当前3个JSX与未分组混合逻辑；`native_record_contract_tests.js`710项在缺indexOf环境验证权威工厂和两嵌入一致。父任务实际catalog/fullgraph/Motion/delete-render/trace/shared-effects旧逻辑DOM520通过（第二个Array修正前）；独立逻辑审校111,530、另一新Array审校4,249通过（包含原1,727，不重复相加）；最终`node tests/RunPanelTests.cjs`55suite零失败，见panel-final-paired-suites.log。原源码及两次旧失败、首次共享工厂不同步失败均保留；无AE操作/新SDK/新部署，Force现场故障仍开放。

STB08/118107：不操作AE。118原生产.obj离线桥接945,481检查、175阶段日志通过；实际保存前态graph-4/Force8b272c经过当前前端为full transaction1/ordinary0，宿主尚未执行。Panel107完整55suite、Model2530、`STARFIELD_DOM_TRACE=1`+实际catalog/fullgraph/Motion/delete-render下新DOM495通过；修后新独立探针605，父任务条件模型/共享wrapper重放630通过。独立v1曾用505项复现跨target、跨project、坏数据容量三缺口，v2按saved path+target/8失败/32事件/有界reader修正；未保存身份标historyOnly。v2审校605后曾额度中断，恢复后核对当前107并完成报告，630保持父任务重放归属。所有证据stb-08-20261027保留。复用118原生11文件/69Core源，不另报SDK或AE通过；freshAE0部署118107并核验118106回滚，已有锁/原Force故障与性能仍开放。

STB07/118：实际旧Parameters链接完整checkout_render_graph夹具复现Host active+PF guard0仍读取缺失Force。新增入口与返回前UI查询后，最终ASan Native23,601/Camera12通过（增量重编实际Parameters与测试，保留首次完整ASan编译及旧失败日志）。`RunModelGraphTransactionTests.ps1 -Run -Sanitize`945,479通过。全新独立审校23,797/Camera12覆盖迟到checkout/checkin、成功/错误、非法guard/失败、worker/legacy/null输出；两轮各10000跨DLL日志完整解析，转义边界/当前日志头/analyze /W4 /WX通过。实际完整事务源/analyze /WX仍有既有C6262（11769,360/新69,376bytes），不记整体通过。SDK118 `-NoRuntimePublish -NoDistPublish -RenderJournal`最终成功，初始sandbox PiPL9020及随后Windows宏冲突失败日志保留；修正NOMINMAX后完整重建。实际117/118 DLL36scene/像素一致。冻结/AE0配对部署、117/106备份与恢复预检见stb-07-20261027及current-state；原故障/guard和性能未验收，诊断I/O不能当性能测量。

117/106部署补记：2026-10-26 20:53 +08:00 owner要求部署，fresh AE0下完整发布、11dist/Junction二进制、26panel、新Core selector/版本DLL及114/104配对回滚核验，Restore报告通过。artifacts/stb-06-20261026/deployment-verification.json为部署收据，validation.json为此前离线资格。未启动/关闭AE或运行宿主脚本，真实删除/Undo/重开/性能等待owner测试；一步恢复见current-state。

STB06第三轮独立审校：实际113/116 DLL合法负Seed Shift有序Force拒绝由新审校者复现；共享MotionFrame修正出生序号参考后旧实现1869checks复现同一bounds错误。局部修复后Core23/23、`RunMotionFrameTests.ps1 -Run -Sanitize`9,799、无NDEBUG的Core /analyze /WX通过；新审校者独立实际helper1,058检查含1,024随机uint32及非法边界通过。实际117 DLL36scene/render成功（18旧合法拒绝修复、18原scene/像素字节不变），另6legacyDLL字节一致，均无AE/设备。SDK117/213adapter/69Core/2构建/36验证/11二进制/26panel106冻结，Core新哈希见current-state，dist114/110回滚保留。三轮修复各用新审校者复核，报告和前后失败日志stb04/05/06；无部署/现场资格。

STB04/05独立审校：冻结115/105旧client+真实JSX复现发布后超时/取消误报成功、丢失回执及过期接管；修复后的Model协议2530/完整panel55suite、当前catalog Motion删除DOM156/181通过；新审校者独立异步次序未发现该范围新明显问题。另位原生审校用实际115代码复现复制/清理双故障guard0，修复后`RunModelGraphTransactionTests.ps1 -Run -Sanitize`945,479、`RunEffectGraphBackupTests.ps1 -Run -Sanitize`最终日志、实际备份源/analyze /WX通过；全新审校者独立实际实现24组合通过。完整SDK116 NoPublish及213adapter/69Core/2构建/34验证/11二进制/26panel106冻结核验，dist114及110回滚保留，无AE/GUI/部署。证据artifacts/stb-04-20261026及stb-05-20261026，真实现场故障/一秒资格不从离线推导。

STB-03/115候选：`RunCloudNativeSyncTests.ps1 -Run -Bindings`及`-Sanitize`各22,759/Camera12，含用户Motion→Force→Output删除的旧帧handoff/新图/最终刷新；这个夹具在114本身通过，不能宣称现场故障已复现。`RunModelGraphTransactionTests.ps1 -Run -Sanitize`934,562、`RunEffectGraphBackupTests.ps1 -Run -Sanitize`706,638检查通过；最终固定名称偏移由合并事务ASan再次覆盖。实际EffectGraphBackup.cpp /analyze /WX通过。旧114备份新断言664527失败及旧JSX saved0/display1误拒绝保留。panel55suite与`RunModelPanelAudit.ps1 -Run -PanelRoot artifacts/prepared/stb03-native115-panel105-final/web_panel`完整实际注册/包装器/删除审计通过；Model9,584/Material185/Motion812，额外当前catalog Motion/删除DOM156/181。旧catalog缺字段、旧fixture缺UI回调/按钮预期失败保留，未改产品节点契约。完整SDK115及最终qualified候选/213adapter/69Core/2构建/34验证输入冻结；dist114不变，working panel105源码不等于已部署完整115。只读现场脚本需先批准，未操作AE/工程/进程；最终证据artifacts/stb-03-20261026/validation.json。

114部署补记：owner要求部署后于2026-10-26 14:45 +08:00以fresh AE0发布114/104；11二进制在dist/Junction、26panel、Core selector/版本DLL及110配对备份核验，Restore报告通过。部署收据artifacts/perf-09-20261026/deployment-verification.json；offline validation.json保留部署前状态。未启动/关闭AE或运行宿主测试，待owner测试。五版实际部署备份保留已报告后执行，当前一步回滚见current-state。

PERF-09/114候选：`RunCorePreparationStorageTests.ps1 -Run -Mode Before`冻结113为1,724项；`-Run -Mode After`及`-Run -Mode After -Sanitize`各1,680项，含静态/ordered temporal逐处分配故障。19场景38完整图/粒子文件、每份27,247,959bytes在三方逐字节一致。Core CMake/CTest23/23，actual native21,359/Camera12，Model桥ASan997累计通过。artifacts/perf-09-20261026/analyze-core.ps1对四拥有cpp执行无NDEBUG的/analyze /WX，原五处C28020/C6262闭环；stack-report保留独立/O2/MT prolog，不能当最终LTCG或宿主测量。完整SDK114及实际旧/新Core DLL六scene/RGBA32F字节对照通过。213adapter/完整Core源头/2构建/20验证输入冻结，Core新哈希见current-state；安装110/104未变，无宿主或设备动作。普通与ASan时长不可混比，简单图固定scratch内存略增，未取得真实AE一秒资格。最终日志/哈希在本轮validation.json，初次失败日志保留。

PERF-08/113候选：`RunRenderScopeTests.ps1 -Run -Sanitize`实际pre_render/codec、工作图/完整布局/类型拒绝/活动资源/OOM共24项；`RunTextureAdapterTests.ps1 -Run -Sanitize`真实纹理准备/staging142项；`RunModelResourceBridgeTests.ps1 -Run -Sanitize`997累计检查（含其控件基础检查）。原生`RunCloudNativeSyncTests.ps1 -Run -Bindings`21,359/Camera12。四个实际adapter由artifacts/perf-08-20261026/analyze-adapters.ps1执行/analyze /WX通过，完整SDK113/212输入/18验证源冻结；实际安装110/104未变。独立编译冻结112的SmartRender原代码，五项夹具精确复现两项停放Motion错误，before源码/fixture/日志保留。宽范围纹理`-Analyze`仍因未修改Core的两处C28020和65KB栈C6262失败，不能写成全项目静态通过；详见本轮static-followups.md。所有普通/初次链接、夹具、ASan环境失败日志保留，当前资格以ASan最终日志/validation.json为准，无AE或真实设备操作。

PERF-07/112候选：`RunNativeTemporalCacheTests.ps1 -Run -Analyze`与`-Run -Sanitize`各124项，`-Run -Benchmark`共3,724项，36组有/无完整收据对照（两遍，每组25样本）。稳定512输入查询/比较512/512→1/1；全局失效/失败保留单项及旧有效回退。`RunCloudNativeSyncTests.ps1 -Run -Bindings`及`-Run -Bindings -Sanitize`各21,359/Camera12；当前普通EXE运行`--missing-auxiliary artifacts/stb-01-20261025/project-evidence/graph-5.sfgr`为21,364/Camera12。覆盖外层完整捕获状态前后变化/失败及原有完整图编码、cold/lazy/动态/缺失绑定。实际NativeNodeGraph /analyze /WX和SDK112通过。检查次数因减少PF回调而下降，不能比较数字大小判断覆盖多少。全部日志与初次夹具/脚本错误保留于artifacts/perf-07-20261026；独立112冻结、当前dist110/104未变，无AE操作或端到端性能资格。

STB-02/111候选：`RunWebPanelHostTests.ps1 -Run -Analyze`、`-Run -Sanitize`各509项；`RunModelTransactionHostTests.ps1 -Run -Analyze`9,664,560项，`-Run -Sanitize`9,665,054项。所有分析保留/WX。`RunModelAssetHostTests.ps1 -Run`150,891、`RunPresetFileHostTests.ps1 -Run`1,842项。四个实际客户端覆盖大容量空诊断；非空/SDK状态/内存失败仍拒绝，面板覆盖有界UTF-8错误文本与截断。假SDK/message-only测试不启动AE或WebView2，不提供宿主资格。旧110 check61失败日志保留，不能把原错误预期当正确契约。SDK111/211输入和完整冻结通过，当前安装仍110；本轮未进行电脑操作，详情artifacts/stb-02-20261026/report.md与validation.json。

PERF-06/native110：`RunCloudNativeSyncTests.ps1 -Run -Bindings`和`-Run -Bindings -Sanitize`最终各23,976/Camera12；当前EXE对旧保存工程`--missing-auxiliary artifacts/stb-01-20261025/project-evidence/graph-5.sfgr`为23,981/Camera12。覆盖认证数值跨帧/回调复用、完整图字节一致、无payload/NaN/sentinel/UUID错配回退、读取中途依赖变化或失败不发布旧warm值、缺失绑定仍报错。该夹具不是今日已变化的工程，勿混同。

实际NativeNodeGraph /analyze /WX通过（源码保留实际失败与修正日志）；曲线边界/analyze258,807项，新`starfield_age_curve_bounds_tests`纳入标准CMake/CTest，最终22/22。证明缓存分析71。SDK110及211输入核验、完整11个native/Core/26前端104冻结/AE0发布/109回退报告通过。独立宿主工具安全/CRC113项仅模拟宿主，不能当AE渲染；首次真实AE因工具ES3保留字native失败后已修正。只读owner-host-probe在AE23.5x52完成，网关可加载、绑定存在、未选图层为no_target。尚无完整宿主帧计时/一秒资格。证据artifacts/perf-06-20261025，当前安装与动作授权以current-state为准。

PERF-05/native109：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunNativeTemporalCacheTests.ps1 -Run`，可加`-Analyze`或`-Sanitize`，各71项；`-Benchmark`含最终两遍/每case25样本的18组证明微基准，共1871项。覆盖SDK次序/旧有效回退/每stream一次state、重入/并发、publication/prefix存活与内存失败。旧108缓存同夹具复现分配失败损失prefix；同一原生fixture编译三个before源精确复现三项旧常量/Life残留失败，新109通过。不得把微基准当作AE端到端性能。

`RunCloudNativeSyncTests.ps1 -Run -Bindings`及`-Run -Bindings -Sanitize`最终各21,818/Camera12通过；EXE增加`--missing-auxiliary artifacts/stb-01-20261025/project-evidence/graph-5.sfgr`为21,823/Camera12，保留真实stream208缺失Auxiliary严格验证。完整May2023 SDK109 NoPublish构建通过、211输入与冻结一致，Core DLL未改；11二进制/26前端104配对AE0部署与108回退预检通过。最终日志/原始基准/指纹见artifacts/perf-05-20261025/validation.json。108实际修复协调器未改，以下为其既有验证；真实AE修复/出图/等待时间门仍开放。

STB-01/native108：`RunGraphRepairTests.ps1 -Run`、`-Run -Analyze`、`-Run -Sanitize`分别覆盖真实修复协调器和每个SDK故障点，各24,530项通过；-Analyze保留/WX，不关闭告警。新增guard0完成回执/缺失native目标清理、有效边ID/顺序、完整备份、source记录清尾/guard及待处理请求拒绝。修正旧fixture256项小于Motion metadata的越界，使用实际parameter_count与流边界检查。磁盘ID校验容量/UTF-8前缀移位分析告警通过显式边界/宽中间值消除，ID/编码不变。

`RunCloudNativeSyncTests.ps1 -Run -Bindings`21,468/Camera12通过。其EXE可额外运行`--missing-auxiliary artifacts/stb-01-20261025/project-evidence/graph-5.sfgr`，读取只读抽取且CRC有效的实际当前工程图，最终21,473/Camera12通过：精确复现stream208/node7f86fd/parameter1/component0，去掉实际缺失Auxiliary及两条连接后严格采样剩余源且不读取缺失slot。原始.aep哈希不变。`RunMotionAuthorTransactionTests.ps1 -Run`3,108、`RunModelControlsTests.ps1 -Run`453及`paired_panel_rollback_tests.ps1`通过，后者新增报告模式提前拒绝candidate-panel冲突。完整May2023 SDK108及211项指纹、108/104冻结/AE0配对发布/106回滚预检通过。真实AE显式修复/出图/Undo/重开与一秒目标仍未验证；日志哈希见artifacts/stb-01-20261025/validation.json，安装身份见current-state.md。

PERF-04/native107：实际`RunCloudNativeSyncTests.ps1 -Run -Bindings`最终21,468项原生/12项Camera通过；新增lazy预热后的连续idle/DRAW去重、每tick一次整体state查询、回调引用变化、publication清除与重捕获。新Release CMake/CTest21/21，`starfield_temporal_preparation_tests`725项/720组完整编码输出一致；测试进程可设`STARFIELD_TEST_PREP_BENCHMARK=1`取得3万/5万、三帧中位数的同算法对照，最终785项通过。复杂Force/Transform/Turbulence/Motion/Wind-Spin、动态与未证明输入、不同/亚帧/跳帧/倒序、取消与身份防御均覆盖。

`RunCoreTests.ps1 -Gpu`已补齐当前资源适配源文件，scene view包含Cloud circles，cache-key断言包含既有灯光/相机键；最终真实CUDA/OpenCL475项通过，不据此宣称AE宿主资格。完整May2023 SDK两个NoPublish开关通过，211项当前构建指纹核验；新AE0下107/104部署和106/104回退预检完成。初次新fixture键/类型错误、旧GPU runner链接/夹具失败均保留。精确日志哈希/测试源身份见artifacts/perf-04-20261025/validation.json，源/候选/安装及备份边界见current-state.md；没有运行真实AE性能对照。

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

PERF-03 / panel104：最终 `node tests/RunPanelTests.cjs` 自动发现55个suite，全部通过。新增panel_addition_queue_tests的205项检查覆盖稳定ID、混合节点默认值、64项上限/16项批次、串行写入、取消计时器、旧目标/重复/错误回执、超时/冲突/cleanup警告停止与原子失败。完整panel_palette_tests通过真实面板+palette验证11次连续拖放仅两次提交、自己回执不取消进行中的拖放、无逐笔getPanelState和匹配回执不重建画布。panel_native_node_gateway_tests补充实际JSX/fake-AE的多次追加Property失效、按批Undo和中途创建失败完整回滚；不代表AE或原生compiler资格。

本轮未改原生/Core源码，211项既有构建输入与11个106二进制指纹复核一致，未为前端改动重复构建/运行Core渲染测试。26个前端统一generation104并冻结，AE0下KeepNative配对发布及103回滚预检通过。所有suite日志、测试/前端源码指纹和前后部署收据保存在artifacts/perf-03-20261025/；模拟回执延迟的提交次数结果不转换为真实AE耗时或FPS。

PERF-02像素补查：在上述数值对照之外，核验原始粒子合成顺序一致，并对28组实际CPU输出比较，8-bit最大通道差为0。日志numeric-pixel-check.log；不替代真实GPU/AE像素资格。

PERF-02 / native106：`RunCloudNativeSyncTests.ps1 -Run -Bindings`先在旧准备源码复现停放Force及常量重力/阻力两项慢路，最终21,580项原生同步与12项Camera检查通过。覆盖实际capture入口、无历史checkout/冻结编码、Wind各轴/Spin/复杂节点/非Default发射/随机Life拒绝、并行Force拒绝与证明失效后的真实历史回退，并对照粒子身份/位置/速度/寿命/大小/透明度/颜色/旋转。`STARFIELD_TEST_PREP_BENCHMARK=1`只给隔离测试进程增加3万/5万粒子三次测量与数量校验，最终22,384项原生同步通过；修复前对照只替换已保存的EmitterHistoryCapture源码，预期两个慢路断言失败，Camera仍通过。

PERF-02独立数值矩阵首轮在并行Force出现差异，保留numeric-matrix.log；快路据此收紧为线性链。最终504个场景、10,944个粒子比较通过，最大绝对差异1.384e-12；覆盖30/60/120Hz、一/二级串行Force、重力随机0/37/100、阻力0/1e-11/0.4/50、负时刻/零/亚帧及跳帧。当前Core新CMake构建、完整CTest20/20通过。完整SDK两个NoPublish开关构建与211项输入指纹核对通过；部署及回滚预检边界见current-state.md。所有原始成功/失败日志、可复现probe源码与最终计时位于artifacts/perf-02-20261024/，未运行真实AE或GPU硬件性能资格测试。

P-09/105：104截图确认真实选择栏消失后，将全AEX审计从“无CONTROL”收紧为与正常控件一致的PF_PUI_NONE/零尺寸，同时覆盖标题区和控制区的DRAW/CLICK/DRAG。修正前104出现240项中的6项失败，105相同240项全部通过；共享节点注册/透传和原生Shape、Transform按钮事务回归通过。完整May2023 SDK候选NoPublish构建及211个当前输入指纹核对通过。10月24日新AE0检查下完整配对部署105/103，哈希与Restore预检通过；AE实际显示尚未验证。精确结果及对104原结论的更正见[native-controls-audit.md](native-controls-audit.md)，证据artifacts/p09r-native-titles-20261023/。以下P-09/104是此前离线流水，不再当作当前显示通过。


P-09：新增 `RunNativeSelectorAuditTests.ps1 -Run` 在隔离EXE中加载八类实际候选节点AEX，逐项核验23个普通选择器的注册和DRAW/CLICK/DRAG事件委托。Shape及Transform原生回调、Create Null空/部分交付恢复与创建失败回滚、纹理选择、真实图同步/Camera、native record及Transform面板回归通过，完整SDK构建使用两个NoPublish开关。随后Owner关闭AE并授权部署，新AE0检查下配对发布104/103并核验哈希/回滚。精确结果、初次失败边界和命令见[native-controls-audit.md](native-controls-audit.md)，日志artifacts/p09-native-controls-20261023/。不据此关闭真实AE宿主门。

P-08R：`RunCoreTests.ps1 -NodeEffects -NodeKind Emitter`的新register_ui断言在102源码复现缺失注册（emitter-before.log）；103四类NodeEffects夹具通过，增加实际回调注册/错误传播和DRAW事件空context、null draw/supplier/surface、suite释放检查。完整BuildWindows NoPublish候选通过；先回退101，再在AE0下发布103并核验完整安装/备份/前端/selector。原生UI缺陷已修正，但选中图层的实际AE崩溃复测仍开放。证据在artifacts/p08-crash-20261023/，不重复运行未改Core的旧计数作为本轮新结果。

P-08：完整May2023 SDK候选使用`BuildWindows.ps1 -NoRuntimePublish -NoDistPublish`；`RunCoreTests.ps1 -CurrentNodes`覆盖共享Particle直接回连与单代静态/历史求值；`RunCloudNativeSyncTests.ps1 -Run -Bindings`覆盖Auxiliary显隐和同步守卫；四种NodeEffects夹具、真实gateway命名及generation契约通过。面板聚合保留原始结果，CEP退役夹具在AE运行时被守卫阻止，不能称全通过。定位线的真实AE绘制/滚动/选择刷新仍待实测。精确计数、安装状态和日志见current-state.md与artifacts/p08-auxiliary-20261023/。

P-08发布轮：AE0后单独补跑`node tests/webview_retirement_tests.js`通过（retirement-ae0.log），此前53个通过suite未重复计数。原聚合失败记录保留。冻结候选与安装、旧备份/前端配对和Core selector核验通过，Restore仅运行报告预检；没有执行真实恢复或将部署成功记作AE实测。

P-07 native101/panel102：`RunCoreTests.ps1 -NodeEffects -NodeKind Emitter`及Particle/Force/Transform验证共享dispatcher；实际显示与守卫回归使用 `powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCloudNativeSyncTests.ps1 -Run -Bindings`。另通过当前节点面板与native record契约检查，以及完整May2023 SDK构建。计数、配对部署哈希和回滚状态仅见current-state.md，日志位于artifacts/p07-emitter-ui-20261019/。初次通用 `RunCoreTests.ps1 -NativeSync` 缺既有ModelMirror/ModelGeometry/Motion/Light链接输入而失败，日志原样保留；此次没有重构该runner，也没有把它写成通过。真实AE的两种节点选项、预设/复制、撤销和重开仍待owner验证。

P-06 panel102：离线事务回归复现非空 Motion Starting With 下 moveNodes 提交后 `invalid_graph_snapshot: Maximum call stack size exceeded`；修复后空/单字符/多字符/中文前缀的位置回执通过，前缀变化仍拒绝。最终54个面板 suite 全通过，独立运行日志保留在 artifacts/p06-snapshot-20261019/。未运行 AE 宿主测试，待实际重开面板验证拖动、位置持久化及撤销/重做。

- WebPanelHost 生产队列的 fake-AE 检查覆盖来源、期限、关闭/重放/FIFO、模态禁用/取消、quiet-error 恢复及从真实测试 EXE 位置发现 web_panel，不启动 AE 或 WebView2。
- 面板 suite 覆盖既有节点、预设/图事务及容器适配；Particle curve 浮点归一化仍严格校验头部、布局、真实数值差异和完整回滚。
- PowerShell 夹具覆盖部署配对、历史/当前清单前缀、目录恢复、后续修改拒绝，以及最近五版保留和不跟随归档 Junction。
- P-05 覆盖实际隐藏命令注册、会话 ID 校验/刷新、无可见菜单查找的节点/Model/预设文件事务和 Diagnostics 修复菜单；诊断报告保持只读。实际 AE2023 的 owner 接受与专项未报告项见 [行为证据](compatibility-matrix.md)，不从离线通过推断菜单或隐藏命令在实际宿主已验证。

P-05 日志保留在 artifacts/webview2/native100-menu-*.log，所有前端 suite 详细输出在 artifacts/panel-tests。此前 native99-web-panel-*.log、backup-retention-tests.log 和 artifacts/all-tests 属于 P-04 历史验证。SDK 完整构建使用 BuildWindows.ps1 的两个 NoPublish 开关，输出在 artifacts/plugin、core-dll。当前最终计数从实际结果文件读取并写入当前状态，不在多个文档复制。

编辑器 get_global_id 属于 OpenCL 内建符号的解析环境。所有配置默认强制加载 .vscode/OpenClIntelliSense.h；它不参与运行构建。C17/C++20 独立内核及 CPU shader fixture 的语法检查见 kernel-editor-default-tests.log。用户配置说明见 [editor-setup.md](editor-setup.md)。


## PERF12交付验证（2026-11-04，127/116）

主证据目录artifacts/perf-12-20261031。SDK127稳定构建应使用BuildWindows.ps1 -NoRuntimePublish -NoDistPublish -RenderJournal，219 adapter inputs与当前hash一致后才冻结。Core69源/DLL与126逐字一致。v2拒绝报告和实际AEX Force/Turbulence失败控制保留，修后必须新v3审校。

可重跑：RunCloudNativeSyncTests.ps1 -Run -Bindings -Sanitize（实际publisher/原始记录复用/旧代退休矩阵），RunNativeSourceSelectorTests.ps1 -Run（直接加载实际8 node AEX，以params=null/num_params=0测试私有selector，SDK/隔离Host需准备），RunModelUiExclusionTests.ps1 -Run（含有界telemetry环），RunNativeClearHostTests.ps1 -Run -Analyze，RunNativeGraphEditTests.ps1 -Run -Sanitize，RunWebPanelHostTests.ps1 -Run -Sanitize，RunEffectGraphBackupTests.ps1 -Run -Sanitize，node tests/RunPanelTests.cjs。各脚本在tests/下；不启动AE。离线计数不等于AE耗时/Undo重开资格。

最终交付证据：artifacts/perf-12-20261031/validation-summary.json（8项最终日志hash）、sdk127-v3-build.log、stable-build-verification.json、frozen-panel-final.log（57/0）、bindings-final-asan-v4.log（48007/0+Camera12）、actual-selector-final-v2.log（88/0）、ui-exclusion-v3.log（1285/0）、clear-host-v3-analysis.log（4508/0）、native-delete-final-v3-asan.log（160255/0）、backup-final-asan.log（717555/0）、adapter-final-v3.log（6251/0）、budget-final-v3.log（调用量，不是AE时间）。独立最终reviewer-v4 PASS/2868 hash/实际64和88入口及111显式反例重放通过；v2拒绝和v3未闭合报告历史保留，独立panel52/57的5前置缺失不冒称全过。部署与126/115回退最终1901哈希见final-audit.json。预部署47条journal已完整归档并标注非AE资格，没有删除原内容或与后续现场日志混用。
