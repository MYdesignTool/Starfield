# 有证据的技术债务

按影响排序，不以文件大小、命名或测试检查数判定缺陷。S1影响当前资格/可靠回归；S2增加跨层回归或阻碍可靠构建；S3维护/诊断成本。证据等级见 [BASELINE.md](BASELINE.md)。

2026-10-19复核并实施GOV-01A小修。2026-10-13的原始审计和失败证据仍保留；不能把历史版本或历史故障直接当作当前缺陷。当前安装身份只见 [current-state.md](../current-state.md)，本轮未部署。任务范围见 [GOV-01A](../agent-backlog.md)。架构级重做须先获得owner批准。

GOV-01A证据位于 `artifacts/gov-01a-20261019/`：其全新CMake DLL构建成功，CTest原为19/20通过。Owner随后批准GOV-01B按最小范围修复分支汇合；新的独立构建及完整CTest20/20通过，证据位于 `artifacts/gov-01b-20261019/`。GOV-01A历史记录还包括54个前端suite通过、七项离线聚合中六项通过及ModelGraph单独复测通过；原失败结果保留。GOV-01B的七项SDK/fake-host离线回归本轮聚合全部通过，并另验证预算及取消。不是完整AE宿主或全部离线测试资格声明。

## 1. S1：原生事务生命周期复杂，历史宿主故障需按当前候选复核

**已证实**：普通发布仍跨共享gateway、StarfieldHost、NativeGraphCommit和NativeBindingTransaction；删除另走ModelTransactionHost、EffectGraphBackup、JSX transport及final binding refresh。renderer/source guard、Host exclusion、receipt和物理effect顺序共同维持正确性。

**已证实（历史缺口，已有修复）**：ADR0044记录漏SUPERVISE、结构改变后复用Property、恢复graph前删除新增源等问题。当前Motion Type已带SUPERVISE；ordinary rollback先退休新增源、恢复旧图和绑定、再移除，失败保持guard。2026-10-19的contract、Motion往返与结构失效离线检查通过，不证明所有AE时序一致。

**待验证**：ADR0044的native86 Motion回退/身份删除报告属于历史；当前状态已有后续owner接受，也明确未报告专项不算逐项通过。当前同症状是否仍存在须绑定候选hash及AE2023 build复核，不能继续声称“仍列于current-state第3行”，也不能仅靠mock关闭宿主门。事务拆分、guard/timeout协议变更须单独审批。

## 2. S1：聚合覆盖及注册catalog来源仍需补齐，已修复点名漏项与旧ABI测试

**已修复（GOV-01A）**：`tests/RunAllTests.ps1` 的All/Cpp新增MotionNativeBinding、TurbulenceGraph、ModelGraph、ModelTransport、MaterialNativeBinding、AliasHandoff和DeletionRenderLifecycle；全部显式带 `-Run`，Alias先于依赖其七kind图的Deletion执行。可用 `-Group NativeRegression` 单独执行这七项。没有把报告模式的退出0记成实际通过。

**已修复（GOV-01A）**：`model_transport_tests.cpp` 区分current ABI9及合法ABI8/7协商，分别校验旧前缀布局；实际构造精确ABI8/7分配，保留旧尾部不可读取、毒化新字段、GPU准备及同generation释放检查。新接入夹具同时修复了Transform的旧ABI4/非法snapshot5断言、硬编码出生ID，以及Material播放节点未连接Output的过期假设。没有删除对应的兼容性、非法版本/端口或身份一致性检查。

**仍存在**：这七项并不穷尽全部专用runner；RunPanel默认DOM没有开启全部feature/结构失效组合。`RunModelPanelAudit.ps1` 会重建catalog，但RunPanel不会；motion roundtrip仍默认读取硬编码历史catalog。本次只读审计中，同源DOM使用根旧catalog时报缺Motion Max Delay，换到既有followup catalog后18组合通过；这些catalog本次未重建，不能推断native100注册已全部验证。旧审计的46 suites是历史数，不是稳定覆盖指标。

后续补明确的suite/scope清单、catalog来源hash/新鲜度和skip/fail规则。历史负面对照见 `artifacts/engineering-audit-20261013/`；本次只读复核见 `artifacts/tech-debt-review-20261019-7b108a21/`。不把局部通过称为“All当前功能通过”。

## 3. S2：CMake缺失输入已修复，构建清单分叉仍在

**已修复（GOV-01A）**：CMake核心源清单补入 `src/core/MotionLightPathResource.cpp`，与MSBuild和RunCoreTests既有输入一致。修复前独立DLL确实报LNK2019/LNK1120，修复后全新DLL及全部CTest目标构建成功；不再描述为“当前便携Core不能链接”。

**仍存在**：独立MSBuild工程、CMake及专用cl配方各维护source lists。直接cl runner仍重编整组输入，NodeEffect按8种NodeKind隔离对象，主AEX/DLL均编译共享图实现；完整BuildWindows仍顺序调多个目标。复用源码不是重复业务实现，NodeKind宏不同的对象不能直接混用。

本次还观察到本机中文MSVC诊断与Ninja include前缀不配对时，头文件修改未触发旧构建目录重编；最终验证改用仅限编译子进程的 `VSLANG=1033` 和全新 `cmake-final/`，不能用先前增量结果证明新头文件通过。没有修改宿主环境变量；可复现命令在证据目录。

**疑似/待验证**：统一输入和可复用host-independent目标可能缩短反馈，但本次没有完整native clean/no-op/单源/header对照，不能引用旧26秒测量推导收益。构建体系重做、对象复用须另行提案，保留 `/MT`、PiPL和adapter指纹门。

## 4. S2：跨语言参数契约仍有多份手工映射

**已证实**：同一Motion模式经过schema、MotionLayout、NodeEffects注册、NativeNodeGraph解码、Graph registry、JS defaults/ports、JSX writer和record validator。Core mode0/1/2对应native popup2/1/3；Speed key4 radians/s与key17 percent共享一个原生流。disk ID、stream index和graph key不能互换。

**历史回归已修复**：CEP87 planner拒绝Look At2导致whole-graph读取失败，见ADR0044。本次精确factory/generation gate、record contract和Motion三模式/速度往返通过，不能宣称当前两份factory不同。

**仍需验证**：factory gate不能独自覆盖全部C++/schema注册范围、颜色维度、enum及单位换算。先扩展实际catalog→JS/JSX/codec往返矩阵；生成全部映射或引入新契约源属于须另行批准的设计变更。

## 5. S2：分支汇合已作局部修复；图桥与双路径仍耦合

**已修复（GOV-01B，owner已批准）**：原先静态与temporal在上游数量不为1时停止回溯，导致同Particle并行Force汇合返回空population。现使用共用的私有祖先收集函数，仅在汇合时收集唯一节点，复用现有Transform/Motion上下文校验；temporal保留真实DAG边。共享Force只应用一次，独立Output入口仍保留独立population及身份。原“不同入口直接到Output也应拒绝”的旧测试已按现行独立输出规则修正，真正的modifier内歧义汇合继续严格拒绝。

回归包含解析数值、共享祖先、Force-only、插入顺序、Motion/Turbulence公共后缀、不同Particle/Transform/有序链拒绝、Auxiliary及多层菱形图；原失败场景和完整CTest均通过。额外探针确认线性与汇合的预算耗尽返回work_limit_exceeded，汇合中取消返回cancelled。未拆模块或改积分、ID/ABI/schema、Render.hpp及宿主事务，见ADR0032补记。本轮没有部署或AE实机资格声明。

**局部行为修复（GOV-01A）**：新增gate暴露粒子空帧仍采样Model/Material，违反ADR0038。已在现有规划中限制负时间/零配额的粒子元数据capture，保留直接Model/Material输出的现有采样。测试涵盖空帧不触发失败sampler、直接输出零粒子配额仍采样动画及错误传播。没有改积分策略、Render.hpp、ID、wire、ABI或模块边界，见ADR0038补记。

**结构性维护债仍在**：NativeNodeGraph承担record读取、host参数解码、compile、binding install、采样及metadata；GraphEvaluation文本include多个私有求值实现。已有共享数学helper及部分Motion static/temporal、Turbulence edge-order测试，不能称为完全重复或完全没有差分覆盖。除上述明确汇合场景，本轮没有证据声称两条路径普遍数值不一致。图桥职责拆分、planning重做或合并积分策略须先审批。

## 6. S3：全局状态与所有权的诊断粒度不足

**已证实**：CoreLoader current/mutex、Host command/scan、ModelTransactionHost单session、NativeTemporalCache proof/prefix和SmartRender最近timing仍是模块全局状态。`SmartRender.hpp` 的last timing缺target身份，多实例时不能由最近读数归因到某节点。当前职责说明见 [architecture.md](../architecture.md)，原审计所有权表属于历史。

**待验证**：复制层同UUID/不同PF_State、重开、取消和host death的隔离矩阵。已有状态比较、缓存上限、UI线程约束及RAII；本次没有竞争、泄漏、悬空SDK引用或跨CRT释放证据。不以移除所有global为目标；缓存/调度架构调整先审批。

## 7. S3：旧checkpoint大部分已清理，剩余历史说明需区分语境

**已处理**：AGENTS Current checkpoint已只指向current-state及web_panel；architecture先给职责/所有权，旧部署与未发布叙述已归档。原文“仍写native54/panel54”“职责表在229行后”“当前86/89”等描述已过期，不再作为当前缺陷。

**仍存在**：schema的migrationPolicy仍串列早期ABI3..5迁移记录，需区分历史与当前规则，但这不是ABI实现错误。本批不改schema契约；当前安装版本继续由current-state单点维护，不在此复制哈希/部署计数。技术债本身也须标明历史观察日期，避免恢复已撤回方案。

## 8. S3：runner已有耗时，阶段测量及源码身份仍不足

**已修复（GOV-01A）**：RunAllTests和RunPanelTests每个suite结果新增 `durationMs`，保留原有passed/exit/log等字段及失败收集行为。

**仍存在**：生产runner结果没有绑定源码身份；缺少统一compile/run阶段、AE启动/提交/首次帧、cold/warm的可比计时。SmartRender已有controls/history/scene/total，journal已有被动记录，Host提交仍扫描项目。重复expression验证、Motion probe和temporal checkout只是性能候选，没有本次AE测量就不能给优化百分比。阶段化诊断和性能改造须按证据及范围另立任务。

## 未证实、不得作为重构理由的结论

历史有限扫描未发现project-owned quoted include环或Core反向包含adapter；这不是全仓库永久无环证明。Host↔Effect回调和Core sampler是显式协议，不能自动认定越层。资源释放已有generation/RAII边界；没有当前证据就不宣称泄漏、全局模拟污染或GPU/宿主支持。小修授权不等于批准图事务、参数映射、求值或缓存架构重做。