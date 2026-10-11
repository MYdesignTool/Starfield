# Model 提交链路整体排查 — 2026-10-11

## M3-17 native68 candidate — literal sources and triangle spans (2026-10-11)

Owner reports native67/CEP71 still produces Particle/Emitter stream matchName
mismatch on Model addition, and Model sample visit cap exceeded on OBJ import.
The expression host's reuse of a call site across heterogeneous effects remains
a hypothesis; no AE root cause or fix is claimed from mock tests. native68 emits
one literal Effect(name) reference per alias after a fresh native kind/full-UUID
inventory. Every UTF-16 name unit is escaped. Source identity still checks all
eight immutable UUID words before numeric control access; errors/sentinels retain
rollback. No expression enumerates the effect parade. Transaction duplicates
receive unique private names and original names are restored on rollback.
Missing/ambiguous author names reject before alias writes; this deliberately
replaces the old assumption that duplicate display names were irrelevant.
Custom author renames rebind at the next native graph commit; host automatic
rename rewriting and old-project migration remain AE acceptance gates.

Read-only owner OBJ (2858 positions/5600 triangles) reproduces the old cap at
size133, angle30, Center enabled: 68,952,940 bounding-box candidates. Scanline
half-plane spans preserve final edge/top-left/depth tests and reduce charged
work to16,889,395 including span calculations, succeeding under the unchanged
64,000,000 group/512,000,000 frame caps. Twelve owner-mesh poses were exercised;
this is an offline render workload result, not a claim about the user's exact
frame or AE performance. Independent whole-ROI oracle compares fractional,
thin, reflected, clipped-ROI and overlapping triangles, coverage/depth/source
identity and exact work-budget boundary. Row/span work is charged even if empty,
preflight runs before depth allocation and cancellation remains bounded.

Evidence so far: scene150946 standard/ASAN, Model particle2695 standard,
whole-graph transaction810845 standard/ASAN, mixed native9048/camera12/Transform60
ASAN, actual generated mixed JS643/Transform168 and animation/gateway checks,
dummy67/71→68/71 full deploy/rollback61. Initial test harness visibility/stage and
backup-name expectations were corrected; ASAN scene exe now runs inside the
vcvars process to retain its runtime search path. Failure logs are retained.
Build68/packed32836/CoreABI8/CEP71; persistent IDs/matchNames/schemas unchanged.
Full frozen SDK/panel verification and deployment pending. Installed67/71
retained until release receipts say otherwise. No AE process started or script
dispatched; owner chose manual testing. Motion/MNT work preserved; push pending.


## native67部署及证据

2026-10-11 14:48+08 native67/packed32835/CoreABI8/CEP71已部署。源码816eb83fc0da5a9757cbf8c1e483bcfb32104380经git archive冻结，完整May2023 x64 Release /MT八目标会话75911实际exit0；冻结ASAN native8802/camera12/Transform60、真实生成JS混合644/Transform168及整套PanelAudit均exit0，dummy66/71→67/71发布与整套恢复61通过。fresh AE0后经Deploy-ModelTestBuild→Deploy-TestBuild -IncludeModel发布；26配对安装hash、全部27已测CEP文件和selector/runtime独立核对通过。首次Emitter Type绑定覆盖Effect计数缺失/0/999/throw和八UUID不匹配；删除native66未经证实的计数契约假设，改为固定UUID参数名称查找，未找到源与非有限值分开诊断且均回滚。实机stream98失败是否消失仍等待owner自行复测；没有启动AE或执行验收脚本，不能视为客户发布资格。IDs/schema/ABI8保持，Motion/MNT改动保留；Git push仍待已有独立授权回复，部署交接后按owner既有要求暂停。

原生完整构建及面板/ASAN冻结结果、26配对加27完整CEP哈希和整套回滚报告见current-state.md。首次回归中的native66重建表达式fixture误替换了初始化match位置，修正后独立反例通过，旧失败日志保留；全套Transform表达式不再复用旧生成数据，运行器每次重新编译生成。

## 新建效果重复失败与测试契约缺口

2026-10-11 M3-17继续修复owner新建图层首次添加效果失败（stream98/parameter1/verify finite bound value/516）。native66增加的Effect.numProperties布局计数没有AE契约证据，旧JS桩从生成表达式反推正确计数，漏掉缺失/不同/抛错计数。native67候选改用Effect.param("Node UUID 0"…"Node UUID 7")固定常量名称，八字全部匹配后才访问源节点数字参数；主效果自身、其他renderer、不同kind及同名不同UUID均不会打开其动画别名。完整UUID唯一性仍由原生编译保证，source求值错误不吞掉，sentinel与非有限值分别报verify source binding resolved和verify finite bound value，失败仍完整恢复表达式。无IDs/matchName/schema/ABI8迁移，CEP71保持。首个Emitter Type真实生成表达式覆盖计数缺失/0/999/throw，native66 gate在同一夹具返回sentinel，新表达式返回1；不是AE实机根因或验收声明。原生/ASAN及冻结完整构建继续；owner选择自行测试，本轮无AE启动或脚本调度。

旧native66按计数避开跨kind访问的测试结论仅适用于原桩，不能证明宿主绑定有效；以本节命名查找和独立计数反例替代其实现假设。

2026-10-11 14:27+08 native66/packed32834/CoreABI8/CEP71已部署。源码186cbf3e1a99f989019f040fcb74846ff342213f git archive冻结，May2023 x64 Release /MT完整八目标会话57534实际exit0，冻结mixed-ASAN/native8796/camera12/JS644及整套PanelAudit实际exit0。完整OBJ/Core4145标准-ASAN、控件453/导入20632标准-ASAN、Model模块9434、dummy65/71→66/71整套回滚61通过；small172/owner271自动验收自检通过。fresh AE0后Deploy-ModelTestBuild→Deploy-TestBuild -IncludeModel发布；26配对安装hash及全部27冻结面板字节、selector/runtime核对通过。owner选择自行测试并稍后反馈，本轮未启动或向AE调度验收。OBJ误拒绝已离线确认修复；Model/Transform内部错误根因及速度改善未据夹具关闭。 备份m3-17-native66-panel71-rounded-obj-20261011恢复65/71；receipt/checker见current-state.md。

native65/CEP71于13:38+08部署：源码9b77643冻结27文件逐字节匹配并重跑关键回归，fresh AE0，26hash/selector/runtime和paired Restore通过；native9200c6b/packed32833/ABI8保持，SDK未重建。baseline71重现27安装面板字节。备份m3-17-native65-panel71-record-contract-20261011恢复65/70。首次授权启动已执行但无entry/result；owner正在自行测试，后续现有实例调度未获授权，保持离线排查。

## Model编辑及真实OBJ反馈 — 2026-10-11

owner确认Model可添加但慢，编辑时报Model/Transform stream matchName内部校验失败；OBJ时报invalid/oversized。后者已用owner只读文件 `D:\Project\Code\test\test.obj` 离线复现：385660字节，第6680行quad normalized non-planarity3.2092e-8，整体quad最大5.5953e-7，原1e-8阈值误拒绝小数导出误差。候选1e-6容差保留原XYZ、不扩大容量限制、继续拒绝明显扭曲/退化/自交。完整文件修后2858positions/5600triangles，编码SFMG1为384480字节；Core几何4145、控件453及导入20632标准/ASAN通过。错误消息包含静态解析原因与行号，失败仍不写控件。

表达式扫描原先先在任意effect上打开数字UUID参数，再检查name/UUID；Model100、Transform90、Emitter110等真实布局不同。候选在param(index)之前检查完整布局计数，后续name/八UUID字不变。实际mixed Model/Transform native edit及生成表达式执行通过：native8796/camera12标准及ASAN，JS混合644/既有动画通过。该证据证明候选避免跨kind数字子流访问，**不能证明AE内部校验错误已消失**，仍为宿主根因假设。ASAN聚合fixture首次默认栈溢出，测试exe单独增栈16MiB后通过；生产DLL标志不变。

新入口 `tests/RunMixedNodeBindingTests.ps1 -Run [-Sanitize]` 补齐Model链接依赖并运行实际生成JS；旧RunCoreTests的NativeSync范围缺少Model helper链接，本轮失败日志保留，未混入其他工作区测试修改。验收生成器可接收artifacts内的实际SFMG1输出，同一8例覆盖真实模型资源/复制/预设/删除。分页逐页核对，并记录prepare、queue/commit、readback及总耗时；计时含250ms轮询/调度，不能作为native单函数耗时。小mesh172、owner mesh271模拟/ES3检查通过；12页资源回读准备完成，没有真实AE结果或性能改善声明。

native66/packed32834/CoreABI8/CEP71完整源码候选正在冻结构建，未部署。原生参数IDs、effect match names、schema及C ABI保持；只修改开发build号。owner仍在使用65/71，继续原工程测试；不自动启动、关闭、写入其AE或安装路径。

M3-17。CEP70出现另一 UUID 的 schema7/3 错误，owner要求停止逐个报错验收。范围包括 Model/普通图提交、预设、资产传输、脚本生命周期、备份回滚和发布。AE为什么返回3、新UUID的节点类型均未证实。

## 确认缺口及 CEP71 修正

| 位置 | 已核对缺口 | 修正 |
| --- | --- | --- |
| browser / Model transport | 浏览器投影五类布局，最终提交仅处理Particle | 同一 ES3 factory 嵌入 browser/gateway，全部五类共用 API规则 |
| nativeSnapshot | schema版本另有嵌套选择表达式 | 同一 type/version 表；没有证据认定AE解析表达式错误 |
| native callback 后 | receipt/ordinary submit/sync/Model snapshot 可能复用旧引用 | plain token 重新解析 renderer、receipt和snapshot |
| script boundary | Array instanceof 依赖调用方prototype | 中性Array tag，覆盖向量/曲线/资产bounds跨realm |
| diagnostics | UUID无法确定type或实际contract | 逐字段错误增加type/build/contract，信息有界 |
| 旧DOM夹具 | 共用宽泛参数表，只测试Particle旧label | actual PF_Cmd_PARAMS_SETUP 导出五模块属性，按真实matchName/disk/index组装DOM |

投影前检查有界整数label、所有必需/已知可选key、唯一canonical key、精确wire type及有限有界值。raw node/schema/stamp不重写，不补必需字段，不放宽用户值、布局或拓扑比较；prepared raw label仍严格。disk IDs、持久化graph/sequence schemas、Core ABI8、native65字节保持。

## 离线回归证据

入口 `tests/RunModelPanelAudit.ps1 -Run -PanelRoot <artifacts/prepared/.../cep_panel>`，默认report。日志 `artifacts/m3-17-full-panel-audit-run5.log`，详细目录 `artifacts/m3-17-panel-audit-84328947399c42dcb915a63a375eab46`。

- 五类factory/browser/gateway、各label、逐字段坏wire、重复/未知key与跨realm：458。
- actual planner/client/JSX混合图、各类prepared/published失败和模拟完整恢复、重试/取消/重载：2340。
- actual gateway/实际注册表18 DOM模式：每组44，旧API重载45。覆盖两个Emitter/两个Particle/Force/Transform/Model、正背面Texture IDs44/66、两种wrapper身份、发布失效、排序和legacy label、提交中gateway重载。
- actual注册：Emitter86/Particle91/Force82/Transform102；Model controls452/binding9433。补齐旧Shape event测试stub、六形状及Transform binding bound断言；native实现未改。
- 页面36、asset600含foreign bounds、author74、preset583、普通gateway全套、三完整JSX ES3 18。
- actual native backup/SFMG1 executor标准/ASAN各636209；Host标准9593769/ASAN9594294；实际Windows message pump exclusion各470；asset/import标准和ASAN也通过。SDK调用仍为替身。
- nested Prepare19、native65/CEP70→CEP71发布/整套恢复dummy64。

首次catalog运行发现旧测试链接/断言缺口及Windows PowerShell UTF16读入问题，失败日志保留，修正后整套通过。通过数不能代替AE资格。

## 自动AE验收及剩余gate

`tools/prepare_model_acceptance.cjs --prepare <isolated panel> <artifact output>` 使用真实browser planner、preset/mesh codec生成8例：默认Add、混合图/已有Texture、默认复制、SFMG1资源/pose恢复、资源复制、资源预设Add/Replace、删除Model。

`tools/Run-ModelAcceptance.jsx` 执行真实native idle事务、等待retained completion、重读snapshot，并逐字节native export回读资源。只创建临时文件夹/合成/solid，结束删除自身项目项并恢复合成选择，不保存工程；执行中超时保留测试合成，不能拆除native正在持有的对象。结果仅写artifacts。单独运行默认report。生成/CRC及引用重写、成功/失败/忙碌/超时清理和两个JSX ES3自检135通过，仍为替身。

`tools/Run-ModelAcceptance.ps1 -Run -PreparedName model-acceptance-panel71` 启动指定AfterFX.exe，短时抑制脚本错误弹窗后恢复，保留AE运行；ADR0011要求单次明确process-start授权。当前尚无实际AE自动结果。撤销/保存重开、OBJ文件窗口、像素/时间行为、模态时序及GPU硬件gate仍开放，禁止称为可向客户发布。

冻结标准 mixed fixture 增加名称回滚用例后，其聚合 main 在默认1MiB测试栈溢出（实际exit -1073741571）；旧 cmd 的 if errorlevel 1 未捕获负数退出码，后续JS缺文件才拒绝。runner现对标准/ASAN均使用测试exe专属16MiB栈并检查所有非零退出码。生产DLL栈/标志不变。第一冻结89e8507保留失败日志，不作为发布候选；修正版将重新冻结和全量构建。
