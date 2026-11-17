# 分层回归与效率策略

现有测试基础值得保留；首要问题是输入来源、覆盖清单和适用边界。本轮实测和日志见 [BASELINE.md](BASELINE.md)，以下自动化都是后续提案，不新增依赖或业务改动。

## 测试层与执行边界

| 层 | 当前可利用的入口 | 自动化与关键断言 | AE依赖 |
| --- | --- | --- | --- |
| L0静态契约/构建输入 | Assert-PanelContract、schema、vcxproj/CMake、record-contract/ES3 tests | ID/flags/generation一致，source-list覆盖，测试manifest无遗漏、catalog来源hash | 无；注册layout验证需要本地SDK编译 |
| L1纯Core | RunCoreTests、Motion/Turbulence/Model/Texture/Transform专用runners；修复后的CTest | seed/绝对时间、逆序请求、codec旧版本、非法bytes/预算/取消、顺序处理、独立像素/几何oracle | 无AE；纯Core可脱离SDK |
| L2真实adapter + fake suites | AliasHandoff、NativeBinding、GraphCommitHost、EffectGraphBackup、DeletionRenderLifecycle、Texture/Light captures | failure injection后graph/bindings/资源回到原状，callback后owned refs=0，active missing严格拒绝，guarded render取消 | 编译需SDK headers，运行不需AE；UI exclusion需要Windows DLL/message window |
| L3真实JS/JSX + DOM | RunPanelTests；RunModelPanelAudit的完整矩阵 | indexed ref失效、UUID重取、全部模式/单位、stale receipt、partial commit/reload/取消；mock只替代host接口 | Node/VM，无AE；必须匹配真实native catalog |
| L4完整native产物 | BuildWindows隔离候选、既有publication/deploy/rollback dummy tests | 10 AEX + Core / PiPL / `/MT` / ABI /指纹/配对/selector；真实DLL generation隔离测试 | 编译需本地SDK/PiPL/GPU工具；不需启动AE或部署 |
| L5 AE2023 qualification | tools/Run-ModelAcceptance、已有手工compatibility checklist与被动journal | 新建/加载/增删/ECW/undo/重开、缓存、suite线程、8/16/32bpc、GPU硬件、在途reload | 必须实际AE和明确宿主操作授权；本轮保持owner-led，不自动运行 |

checks数含夹具/资源计数，不是独立场景数，也不是代码覆盖率。本轮未跑覆盖率或ASAN，不生成虚构coverage百分比。静态推论与host观察必须独立标记。

## 回归优先顺序

| 优先 | 路径与理由 | 先建立的可靠gate / 验收 |
| --- | --- | --- |
| P0 | 测试发现、ABI和build清单；否则局部绿会掩盖缺口 | 明确当前/legacy ABI断言；故意遗漏Motion/Turbulence runner或resource source必须让gate失败；所有suite按层标明executed/skip reason |
| P0 | ordinary /整图增删/恢复/发布/guard；已有实际host失败 | 保留native lifecycle/backup/exclusion fixtures和DOM失效矩阵；逐阶段注入错误，验证先恢复依赖后删除、restore失败不放guard、无意外missing源、所有owned refs释放 |
| P1 | 模式/单位/record读写跨语言；曾导致Look At读取回归 | 实际八kind SDK注册catalog +三Motion模式 +Turbulence0/3/4 +百分比/radians/curve float容差；覆盖writer→native snapshot→planner→整个图，而非只有单控件 |
| P1 | 静态/temporal/Auxiliary/ordered processors | 固定seed/绝对时钟、乱序帧，Force/Transform/Motion/Turbulence排列，branch/资源/Output祖先；独立有限差分/几何oracle；Emit Once及Lifetime Random触发另一执行路径 |
| P1 | 保存/codec/资源/GUID/generation | 旧wire fixture不从当前serializer现场生成；截断/错误stride/NaN/失效引用；同generation释放；内容变化改变cache identity；取消和配对fallback |
| P2 | pixels/CPU-GPU、性能 | CPU ROI/PAR/alpha/HDR/深度/预算像素对照；模拟GPU scene只证明数值准备，硬件/AE对照仍单列L5；后续建立分阶段性能基线 |

静态与temporal不是处处应相等：先列声明等价的条件、时间离散精度和allowance；有积分/历史语义区别时用各自独立oracle。禁止把测试期望从被测实现复制出来，或为了“全绿”删strict rejection/legacy checks。

## 现有命令与本轮复现

常用入口（固定output会重写旧日志；后续runner需支持run目录，本轮用新取证目录保存输出）：

```powershell
node tests/RunPanelTests.cjs
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Assert-PanelContract.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunAliasHandoffTests.ps1 -Run
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunDeletionRenderLifecycleTests.ps1 -Run
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelUiExclusionTests.ps1 -Run
```

本轮Core编译复现：保留的 `artifacts/engineering-audit-20261013/core/compile.cmd`调用vcvars和cl，响应文件从当前RunCoreTests source list派生；再运行同目录 `tests.exe`。CMake复现：`cmake-build.cmd`配置Ninja Release/j4；`cmake-link-probe.cmd`单独构建DLL/Motion target。通过 `node tools/Run-WithBuildEnvironment.cjs cmd.exe /d /c <保留的反斜杠cmd路径>`运行，规范化只作用于child环境；避免覆盖取证日志，重测时用新run目录。

完整DOM组合见 `dom-current-catalog-results.json`，child环境为：

```text
STARFIELD_PANEL_ROOT=<当前checkout>/cep_panel
STARFIELD_NODE_CATALOG_DIR=<当前checkout>/artifacts/m3-20-followup-20261013
STARFIELD_DOM_MATERIAL=1, MOTION=1, DELETE_RENDER=1, FULL_GRAPH=1, REORDER_CONNECTIONS=1
STARFIELD_DOM_STABLE_EFFECTS=0/1 × INVALIDATE_COMMIT=0/1 × LEGACY_KIND=""/emitter/particle/force
另两例：STARFIELD_DOM_STALE_GATEWAY_API=1；STARFIELD_DOM_RELOAD_ON_COMMIT=1
node --expose-internals tests/model_graph_gateway_dom_tests.js
```

上述缩写后的每个开关都带 `STARFIELD_DOM_` 前缀。本轮使用**保留catalog**，不宣称当前注册flags重建通过。后续权威入口应先用已有实际SDK注册fixtures生成当前catalog，记录源码/工具链hash，再跑DOM；旧目录拒绝/单独标记compatibility fixture。现有RunModelPanelAudit已重建部分catalog，但固定文件名、旧fallback及最新kind覆盖仍需整理。

## 执行与结果管理

- 用小JSON或PowerShell表建立suite manifest：name、层、source roots、runner/参数、SDK/Windows/GPU要求、kind/features、时长。支持快速相关集与完整离线集；未映射新增suite必须告警。当前All不得继续以名称暗示全覆盖。
- 每次run使用 `artifacts/<任务ID>/<runID>/`；结果包括HEAD+dirty source manifest、candidate/panel/catalog hashes、SDK/toolset/Node、命令与参数、开始/结束、build/run分别耗时、exit、executed/skipped/failed、log。无需大型框架。
- L0+受影响L1/L3作为每次修改的快反馈；L2在事务/adapter变更时强制；完整离线manifest +配对build在候选发布前运行。ASAN用于资源/生命周期改变；已有通过检查不无故反复扩大重跑。
- 拆开“compile”与“run”，复用相同flags/macro/source依赖的对象；NodeKind、ASAN、ABI和runtime必须在cache key内。先修可用CMake/CTest或复用现有MSBuild，禁止另建第四套业务source list。
- 确认输出目录隔离后才并发独立suite；当前多个配方共享artifact/catalog文件，不应直接并发全量runner。CPU预算、失败收集及资源高峰先测后调。

## 性能与AE验证的证据标准

本机已有基线：JS46 suites 8.219s；完整DOM18例16.455s；Core clean配方编译25.999s，warm执行中位0.045s。每项后续至少测相同输入的clean、no-op、单cpp、公共header；记录build/run分段及一次性工具初始化，不能把失败到中止19.118s当作CMake全量基准。

AE时间记录分启动→插件发现→CEP ready→首个snapshot→首帧→warm seek→增删→undo→保存/重开。样本固定工程规模、active/parked节点、light/path keys、OBJ大小、色深和分辨率；保留p50/p95与取消/错误数。优先使用SmartRender现有controls/history/scene计时和journal，再考虑增加最小诊断；最近全局读数必须与目标/时间对齐。

L5回执必须写host完整build、候选/资源hash、步骤、graph/receipt/UUID/guard、实际现象与日志。撤销/重开/表达式缓存、属性监督、灯光外部变更失效、suite允许线程、硬件GPU、正在render时换DLL都不能由offline fixture关闭。下一阶段只准备可审阅用例；ADR0011及owner禁止自动AE验收的约束保持有效。
