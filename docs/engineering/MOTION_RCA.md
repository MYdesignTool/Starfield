# MOTION-RCA：模式回退与 Motion/Force 删除身份故障

调查日期：2026-10-14（Asia/Shanghai）。任务：MOTION-RCA，读取 M3-20 的历史与在途候选；本轮仅取证和文档，不接续实现。

**结论：目前无法确认两个 AE 故障的实际根因，也无法定位当前失败操作的第一个错误边界。** 已确认 native86/CEP89 之后仍有 owner 失败报告；已找到可解释现象的具体代码边界，但缺少一次失败操作内、具有相同 target/UUID/事务身份的作者值、保存流、图 ARB 和回调轨迹。不能从离线通过、截图或旧缺陷直接宣布根因。

优先判断为：模式故障首先查作者值与原生保存流不一致；删除身份故障首先查结构变化后的 PF 回调 binding；事务失败/恢复可能把二者连接起来。三个假设及区分实验见第 5、6 节。**不建议批准下一轮猜测式修复；先批准最小观测实验。**

**取证窗口内发生了外部版本切换**：调查开始为86/89；另一并行工作于13:13:00+08发布87/90并改写源码/现状文档。本调查没有执行该发布。结束只读核对87的11/11 native/Core与90的27/27 panel匹配新收据；但没有87/90实际AE失败或成功证据。下文根因分析的失败基准是最后已报告失败的86/89，不能将它表述为“87/90仍可复现”，也不能因新版本离线通过宣布故障解决。

## 1. 调查范围、版本与证据身份

### 1.1 已读取的材料

- 五份审计：[BASELINE](BASELINE.md)、[ARCHITECTURE](ARCHITECTURE.md)、[TECH_DEBT](TECH_DEBT.md)、[TEST_STRATEGY](TEST_STRATEGY.md)、[REFACTOR_PLAN](REFACTOR_PLAN.md)。其中宿主故障结论来自 owner/现状记录，并非审计实际在 AE 重现。
- `docs/architecture.md`、`docs/roadmap.md`、`docs/agent-backlog.md` 的 M3-17/18/20 卡、`docs/current-state.md`、`docs/testing.md`；相关 ADR 0022、0023、0039、0040、0043、0044、0045，以及宿主授权 ADR0011。审计治理提案未执行。
- Git 历史、部署收据、native86 保留源码、当前脏工作树、CEP89 与 isolated CEP90、现有 journal、owner saved-project 提取记录、相关 native/DOM 测试实现和日志。

证据等级：**源码事实**只证明所述代码存在；**宿主观察**指有来源的 owner/既有 AE 记录；**离线证据**不能证明 AE 的通知/保存/引用生命周期；**假设**必须通过实验区分。本轮不运行修复测试矩阵，不修改夹具来获得通过。

### 1.2 现场版本与当前源码必须分开

| 对象 | 本轮核对结果 | 限制 |
| --- | --- | --- |
| 最后已知失败配对 / 调查开始部署 | native86 / packed32854 / Core ABI9 / CEP89，2026-10-13 23:02:59+08 的 `artifacts/m3-20-motion-author-20261013/deploy-after.json` | 部署收据不是某次失败操作的 graph receipt |
| 开始本地部署目录 | 当时 `dist/` 的 11/11 native/Core SHA-256 与上述收据 afterHash 一致；CEP 源码 27/27 与 candidate.json 的 CEP89 清单一致 | 本轮没有重新核对 Adobe/CEP Junction；磁盘哈希不能证明历史内存镜像；后被外部发布替换 |
| AE 进程 | 本轮 2026-10-14 12:59:08+08 只读查询 `AfterFX`/`AfterFX_64` 得到空列表 | 没有启动、停止或调度 AE；不因此执行部署 |
| 当前 checkout | HEAD `e02ad2eef96b2974f0969c4197b76c6ef4afb581`；大量未提交/未跟踪内容；原生版本头为 native87 / packed32855 | HEAD 不能代表调查源码；本轮开始采集 547 个现存 Git/未忽略文件的哈希 |
| CEP90 | isolated `candidate-panel/` 包含 intent guard4..6 与 `valueAtTime(0,true)`；开始根目录为CEP89，结束外部发布后为CEP90 | 开始native87源与根目录CEP89不是配对候选；结束配对另核，不能混用两个时间点 |
| 并发编辑 | 中段发现3个源码/测试文件改变与SDK日志写入；13:15:56核对为初始547文件中516不变、31改变（含CEP、测试、状态文档） | 本轮没有写入、回退或停止这些工作；完整差异见source-after-verification，不声称当前工作树全程冻结 |
| 结束部署 | 新 `m3-20-motion-native-author-20261014/deploy-after.json`：native87 / packed32855 / ABI9 / CEP90；本轮只读匹配11/27。新source清单547中540匹配，7个差异仅为部署后文档更新 | 由外部工作发布；本调查无87/90宿主qualification证据。清单核对不能替代失败操作轨迹 |

本调查只新增本文件及 `artifacts/motion-rca-20261014/`。该目录保存 `source-before.json`、`git-before.txt`、`source-vs86.json`、`source-mid-changes.json`、`dist-pair.json`、`cep89-pair.json`、`host-readonly.json`、`evidence-files.json`、journal 快照及摘要。结束比较存 `source-after-verification.json`，新配对核对存 `pair87-end.json`、`source-vs87-end.json`。未清理工作区、修改业务/契约、重建或部署；外部发布另列，未归为本调查动作。

native86 相关源码来自 `artifacts/m3-20-motion-author-20261013/source/`，本轮逐文件核对其与 candidate.json 清单一致。下文对失败版本的路径/行号指**这份 native86/CEP89 保留源码**。开始CEP89、NativeNodeGraph、EffectGraphBackup与根目录同名文件相同；随后CEP变为90，查旧gateway必须用保留source。涉及native87时另行注明，避免混读。

| 文件 | native86/CEP89 源 SHA-256 |
| --- | --- |
| `ae_plugin/NodeGraphSync.cpp` | `E10367ED2369B98E3C4DFA14D37B2E9D78034BB745C1108E7B575F41858BFBAC` |
| `ae_plugin/NativeGraphCommit.cpp` | `AD939D6675D615E3BB7A42724ED072E4A8BEE4EEEC2D66C571438BCF90C1279F` |
| `ae_plugin/NodeEffects.cpp` | `40CF45E9658680603D493121B47E7846871B73654689352A36541F70FC9554B5` |
| `ae_plugin/MotionLayout.hpp` | `9C5689E69366826D9ADEC2273A00FF48D006E011284399E5864C2886DCA50167` |
| `ae_plugin/NativeNodeGraph.cpp` | `1B3735D06F0811362563A52EE41F266566426B9BB813F53DCE92EA5620F7E15C` |
| `ae_plugin/EffectGraphBackup.cpp` | `3AB75CAB835A210885E6413D8FBF533BAB32A31225E57D939428915486652F28` |
| `cep_panel/jsx/starfield_gateway.jsx` | `E0FD7EECACCA66C2F4AB40DC170DC5AF60A1BC7ECAF284A9F59137741AAAE0B7` |

二进制现场基准：Motion `1E6C6437041F2AA9B7510020AD96C21A62A8A2DA075842E25CB0CB3FFD7A75DF`；Force `7A49084292F56C9D5365DD620DE4BCA2ED9193BEB6AA1970E2EDD1FBEA337C63`；主 AEX `7CFCFD95E8A364DDCFB9A4AAA944920D5800EAA575F35C66D240C7AF59EAAF40`；Host `78B454C274B23F84A55695EEE67ADC07191C9E4745E2F2481511208C94DE1678`；Core `A074A756EF00B765753855FBF6CF3795B970CB52185C7C64E1813FE91775D349`。完整配对与当前源码哈希见取证 JSON。

## 2. 两个故障的可复现定义及证据状态

### F1：Motion 模式回退

**判定定义**：对同一个 live Motion UUID，以 CEP 或原生 Effects Controls 请求 Circle（Core1/native popup1），随后 native popup、graph ARB、CEP fresh record 中至少一个成为 Look At（Core2/native popup3），且用户没有再次选择 Look At。必须同时记录事务成功/失败：失败后恢复原 Look At 与“成功提交了错误模式”是不同故障。

契约固定：Circle Core1 ↔ popup1；Light Path Core0 ↔ popup2；Look At Core2 ↔ popup3；disk1701、stream index1、graph key1 不能混用（MotionLayout:5–12、18–25）。默认注册为 popup1；CEP Motion 默认 key1=1。当前未发现默认值或这三项 enum 映射反向的源码证据。

| 已有观察 | 来源 / 状态 | 尚缺条件 |
| --- | --- | --- |
| 新 Motion 变 Look At；Circle 在 CEP、原生两端回到 Look At；expected1/observed2 | `current-state.md:16`、M3-20 卡、ADR0044/0045；native86/CEP89 后 owner 继续报告。**宿主失败仍开放** | 对应 UUID、请求/回执原文、是立即还是下一次刷新/编辑/预览后回退、graph ARB 实值、失败事务是否恢复旧模式 |
| 早一轮 expected1/observed0 | native84/CEP86 后的 owner 记录 | 不能将其与现在的 1→2 合并为同一映射 bug |
| 保存图内两 Motion 已为 Core0，binding mode 常量为 native2 | `m3-20-followup-20261013/owner-evidence.json`；UUID `5f8c811198044cd9b94731022cae5a3b`、`da1d19ad2d174a7ba69d493c62191986`；项目 SHA `356faa2cd88d8a5ccfbb61d10f6d9c93c6188c88f047d1a160616f86baab1b29` | 只证明保存时两种表示一致地为 Light Path；没有当时的作者请求，不知道第一次改变的操作，也不是 native86 1→2 的新证据 |

本轮未自行在 AE 复现。精确发生率、最小场景、动画/灯光/既有工程依赖、AE 当前确切 build 均未锁定。历史材料有 AE2023.5.0 Build52，但不能自动填入这次失败的宿主版本。结束时外部发布87/90，其故障是否存在未知。第 7 节是**待执行复现协议**，不是已经验证的稳定 recipe。

### F2：删除时 Motion/Force stream 身份错误

**判定定义**：对固定 target 的 Motion/Force 单删或批删，AE 弹出明确包含“实际 Motion／预期 Force”的身份校验错误。保存原文、触发操作、删除 UUID、effect parade 前后顺序；分别记录图提交是否成功、源是否物理移除、是否进入 rollback、预览是否失败。弹框不等于删除必然未提交。

- owner 在 native84/CEP86 后报告批删失败、单删有时成功但弹框；native86/CEP89 后仍报告 Motion/Force 身份对话框（M3-20 卡、ADR0045、current-state:3–29）。**当前未关闭**。
- 对话框只给出实际/预期 kind，无法识别是哪一个 PF selector、哪条 AEGP stream 调用、哪个 UUID 或何种 guard 阶段。现有文字没有建立删除 Motion、删除 Force、删除两者的分别发生率，也没有原始截图文件与日志行的共同事务身份。
- M3-17 的 stream144/258/268/error516 是旧绑定不可用故障；M3-20 的 orphan stream240 是断开的 Turbulence。它们与当前 kind mismatch 不能直接等同。

两项优先审计问题均与**当前继续报告的症状**吻合，但本轮只能确认“仍有 owner 报告”，不能确认稳定复现条件或实际因果已经固定。

## 3. 历史修复与回归关系

下面只整理与这两个故障相关的修复序列。提交/收据确定“改了什么”；为何后续仍失败，除明确的源码/负面对照外均标为未确认。未提交的 M3-20 必须按保留 source manifest 识别，不能假装有对应 release commit。

| 轮次 | 当时想解决什么 / 改动 | 后续结果与解释边界 |
| --- | --- | --- |
| ADR0022 build17（早期基础） | 原生 edit 的 generic delivery 失败；改为节点直接编译，按 UUID/index 用 delivered 值覆盖源读数并保存 renderer | 当前仍沿用此基础。覆盖 graph 不自动证明宿主已保存源控件；不是已证明的 Motion 回归起点 |
| native68/CEP71 | 混合节点 parameter matchName 错误；alias 改用唯一名称 literal 引用并验证全 UUID，停止异类 effect 枚举表达式 | 旧宿主确切缓存原因未证实。当前 literal/full UUID 路径保留；不能用早期表达式枚举 bug 解释这次 UI 对话框 |
| native69/CEP72，ADR0040 | CEP 假定 setValue 会送监督通知；改 nonce + 同步 Host command + saved graph/receipt 校验 | 消除已知协议依赖；该 receipt 仍认证原生编译结果，未携带用户 Motion 期望值 |
| native72/CEP76、73/77 | Circle 独立作者、随后 ordered Force；binding9、Motion kind7；首阶段还没有三模式完整作者 | 后期三模式扩展了读写跨度/validator；没有证据证明 Circle 初版的宿主模式问题已关闭后又重新引入 |
| native74/CEP78，`41777fc` | 删除源早于出版图；改 full backup、source guard3 退休，出版后才物理删 | owner 仍见参数值错误；后续确认 alias mutation 本身仍有窗口。宿主调度触发点未捕获 |
| native75/CEP79，`82c460f` | 消除旧 graph 活着时 sentinel/禁用表达式窗口；finite handoff，出版后退休 alias，rollback 先表达式后 base | owner 仍见 stream268；离线 mutation hooks 未覆盖完整 duplicate renderer 生命周期 |
| native76/CEP80，`f7f9a05` | 原始/disabled backup renderer 在中间状态读取 alias；renderer guard1/2 checkout 直接 cancel | owner 仍见 guard0 stream258。只在 guard 内取消不足以证明 guard 释放后的 dependencies 正确 |
| native77/CEP81，`2532799`、`097a3ca`、冻结 `352562d` | 物理删源/backup 后，用最终 parade fresh refs 重绑；退休 alias 做真实文本 transition；CEP 立即投影 | owner 接受响应速度，仍拒绝删除行为。journal 有 6 次 unavailable stream258；AE 缓存解释仍为假设 |
| native78/CEP81，`78b7c79` | 假设 graph/alias generation 混合；增加 CRC-stamped alias/checkout cancellation | owner 添加节点 8s timeout；15 条 PF 记录 graph/receipt CRC 相同，未支持原假设。不可把这轮视为确定根因修复 |
| native79/CEP82，`2b541c5` | read timeout 不立即取消事务；failure full restore 后增加 rebind finalizer；加 UI phase journal | owner 仍不能添加；`cc16d74`（2026-10-13 00:21+08）明确撤回 78/79、恢复 77/81。旧日志/提案保留，不能在本次顺手恢复方案 |
| native80/CEP83 | 三模式作者；修 native reader 拒绝 popup3；Light Path appended fields142→190、binding10 并保留 reader9默认迁移 | 这些是明确源码契约缺口。高节点删除警告延期，未宣布宿主删除修好；与当前 1→2 是否同源无证据 |
| native81–83/CEP84–85 | Turbulence 默认单位及 sparse binding | 相邻功能发布，未关闭 Motion/删除；不可用它们的 passing counts 判断 Motion |
| native84/CEP86，ADR0043 | owner 项目 disconnected Turbulence 已删但 alias仍存；Output祖先范围、explicit orphan retirement、append reuse | orphan 数据不一致已证实，owner确认速度改善。留下随机 active binding/删除/模式报告；孤立源问题不能解释全部模式/身份故障 |
| native85/CEP87，ADR0044 | DRAW/source supervision 越事务边界；加 exclusion query、guard3、defer metadata；修 Motion Speed 把 record 转 Number 得 NaN、两种 Speed 表示及 curve容差 | Speed50负面对照证实旧 writer 错误，但没有复现 1→0 根因。随后遇到 independent reader bug，而非已证实同一故障“交替” |
| native85/CEP88 | CEP87 planner embedded factory 不接受 Core2；同步两份 factory，读取失败保留同 target confirmed canvas、fresh receipt gate、统一 cache generation | 旧87 whole-graph reader确实复现 unsupported Motion mode；当前两份契约已一致。该 bug会拒读，不会主动保存 Look At2；不认定它是当前模式回退根因 |
| native86/CEP89 | Motion Type/After Path 加 SUPERVISE；selector 强制写、UUID重取；guard 内 verify mode；ordinary rollback 先恢复 graph/bindings 再删 appended additions，失败保 guard | owner仍见 1→2、Motion/Force dialog。缺 notification 是源码缺口，不能解释“已补 notification 仍失败”。DOM fresh getter/mock使用的状态权威与 AE 不一定相同 |
| native87/CEP90，ADR0045 | native author intent、AEGP 源 mode保存/验证、完整 callback count/fresh kind+UUID、扩大 exclusion | 调查开始在途、取证中改变并由外部工作于13:13发布；仍没有新pair的AE实测。离线/build日志不构成根因或现场修复证明，本轮未继续实现 |

历史上的“后续出现回退”包含至少三种不同事情：owner明确要求撤回 78/79；CEP87新 reader 缺陷导致 whole-graph 拒读；native86 后原宿主问题仍未关闭。没有同 target/UUID 的失败前后轨迹，不能断言每一轮都是同一个 bug 被修好又重现。旧修复针对不同窗口、测试替代的状态过于一致，是现有证据支持的解释；究竟哪个窗口被 AE 触发仍未知。

## 4. 聚焦的关键状态流

### 4.1 状态权威与不变量

| 状态 | 权威 / 校验 | 本次应记录什么 |
| --- | --- | --- |
| CEP intent/draft | graph_edits + graph_transactions；画布先投影不代表保存 | target token、requestId、node UUID、期望 Core mode、pending/返回错误 |
| 源作者参数 | JSX Property、PF delivered params、AEGP saved stream 三种读取入口 | popup值、delivered count/index/type、全8 UUID words、actual installed kind、源guard |
| renderer graph | graph ARB + binding record；revision、CRC高低、control source | ARB读取后独立解码的 mode/UUID、binding mode常量、revision、CRC；不能用 CEP snapshot 代替 |
| receipt | commit nonce与saved receipt；原生 publication末尾写receipt并读回 | 正负 nonce、publication stage、失败/rollback/cleanup outcome；receipt不是作者参数原始请求 |
| aliases/binding | literal source name+UUID；animated字段按slot checkout | slot/stream、源kind+UUID、enabled、finite/resolved、publication/retirement/final refresh阶段 |
| guards/exclusion | renderer0正常/1编辑/2备份；source0正常/1作者/2备份/3退休；Host另有scope | delivered与saved guard区分，exclusion entry/exit，真实物理删/重排边界；4..6仅87/90，不属于86失败基准 |

Motion Type不可动画；在现有 binding 中属于常量（历史 owner-evidence 的 mode slot65535即无alias）。Force积分、history或Motion几何不会在逐帧 render 中写回 popup/图 mode。因此“Force求值把 Circle改成Look At”目前没有写路径支持；真正要查的是作者/保存/出版/恢复。

### 4.2 CEP 编辑与普通图事务

`panel.js:883` 附近编辑提交 → `graph_transactions.js:193` 以当前 target/revision/recordStamp 规划 → ordinary `submitGraph`；涉及 remove 时切 full transaction。

CEP89 ordinary 顺序（gateway:2272–2372）：

1. fresh target和nativeSnapshot检查 base revision/recordStamp；ordinary拒绝remove。这里还没有业务写入。
2. undo + renderer guard1；ensure effects、UUID、边、作者参数。新增 effect使 indexed refs失效，重新解析renderer。
3. `authorNativeNode`（1391–1409、1639–1647）：源guard1下显式selector setValue；finally源guard0；清Property cache，以UUID重取并验证 `.value`。**此时没有把期望 Motion mode作为独立native请求字段送给compiler。**
4. `triggerGraphCarrier`（2000–2039）：commit nonce写入 + 同步 `app.executeCommand`；Host扫描唯一 pending guarded renderer；generic进入主 AEX。
5. `NativeGraphCommit`：读旧graph → compile native sources → scratch snapshot/revision/CRC → install binding → scalars/graph ARB/最后receipt →读回 bytes/scalars → retire aliases → accept。
6. UUID fresh nativeSnapshot + `verifyAuthoredMotionModes`在guard内检查；catch恢复原作者/graph/bindings后删新增源；finally释放renderer guard（失败恢复则保持）。返回结果后CEP采用receipt并后台刷新。

**重要观测边界**：`nativeSnapshot`（1959–1982）读取numeric receipt与 `readNativeNodes(layer)`，其 node值来自源控件；它没有直接解码保存 graph ARB。`verifyAuthoredMotionModes`验证的是fresh作者记录，不是ARB mode逐字节内容。`NativeGraphCommit`保存验证的expected graph由native compiler自身生成。因此 native保存getter与script getter若不同，native可正确保存“错误的作者解释”，两边各自校验仍不提供跨权威一致证明。此可能性尚无本次 AE 失败实测。

### 4.3 原生控件编辑

`NodeEffects` SUPERVISE → `NodeGraphSync::sync_node_graph_parameter` →本地 `NativeEdit(kind,index,uuid,delivered value)` → direct `commit_native_graph_edit`（ADR0022）；不经CEP manifest或Host graph command。

native86（保留 NodeGraphSync:325–444）：先查询Host exclusion、看delivered source guard；取得 `AEGP_GetNewEffectForEffect(in_data->effect_ref)`，**先打开源guard stream，后定位renderer并查saved renderer guard**；随后从params读取UUID/capture value。没有完整 `num_params == parameter_count(kind)` gate。

compiler（NativeNodeGraph:197–205）以 `edited_effect + parameter_index` 的delivered值覆盖本轮源读取；Motion mapping（914–918）生成graph。native86 commit把结果保存到renderer，却没有显式AEGP保存源Motion Type；成功后仅对Particle activation额外标CHANGED_VALUE，未对Motion selector做同样操作（NodeGraphSync:425–440）。源控件最后如何保存依赖AE完成该用户编辑。

这形成可检验窗口：delivered Circle1编译为graph1；若源saved仍为popup3，下一次不覆盖mode的编译会读到LookAt2。是否真的发生，以及是延迟、被拒、旧delivery还是另一次事务恢复，现有journal均没有证据。native87在途代码针对这个窗口增加作者保存和回调保护，但不能反向作为native86宿主原因证明。

### 4.4 删除事务及失败恢复

`graph_transactions.js` → `model_graph_transactions.js` queue/poll → JSX transport → resident ModelTransactionHost持Host exclusion → `apply_model_graph_transaction`（ModelGraphTransaction:114–144）。非Model删除也走此路径。

正常顺序：backup.prepare把原renderer置guard1，复制整个受支持effect图并为备份建立private identity/guard2 → JSX prepare ensure所需节点，待删源guard3、保持物理存活 → assets（无Model时可空） → JSX commit verify prepared作者记录、写作者与renderer、nonce显式publish、verify published records → remove退休源 → backup.discard删除备份 → native finalize以最终parade fresh refs重绑 → release renderer guard →结果通知/CEP确认。

这里source删除和backup删除都会改变宿主effect对象/槽位；disabled备份也可能收到PF回调。编译/backup find会按kind+UUID重新查找，但**native86源监督入口仍先使用借用PF effect binding开guard stream**。renderer render取消、DRAW defer、source入口初次exclusion query覆盖不同selector/不同时间；Host scope结束后延迟到达的旧callback并不自动被覆盖。

失败路径另查：

- ordinary catch采用native86新增顺序：退休实际新增源 →恢复旧graph/binding →物理删新增源；恢复失败保持guard1。没有证据表明当前又恢复为“先删后恢复”。
- full `backup.restore`（EffectGraphBackup:181–208）删live originals，reorder备份，恢复原name/UUID/flags，逐源guard0、renderer最后guard0；**没有调用finalize重新绑定恢复出来的实例**。这是被撤回native79曾处理的风险，当前仍保留native77路线；不是本轮发现它“重新变回”的提交证据。
- `discard`（174–179）即使delete_backups/finalize报cleanup错误仍调用release_guard；结果可为committed+cleanupError（transport:225起）。所以“已commit”与“最终依赖已确认/guard还保有”不能划等号。

以上是具体源码行为，不证明当前弹框发生在restore或cleanup失败支路。必须先取得进入该支路的实际outcome。

## 5. 三个优先根因假设

### H1：作者值、源保存值与graph出版值不在同一时点/权威

**优先级：F1最高；F2关联较弱。** Circle请求可能被script暂态/缓存getter确认，但native编译读到旧LookAt；或者native callback只将delivered Circle保存进graph，源popup并未持久化，下一轮编译恢复旧mode。

- 支持：第4.2、4.3的两条实际读写链；native86缺独立native author intent，缺显式源selector保存；snapshot验证不是ARB直接读回；owner两端均回退使纯CEP显示问题不够解释。旧saved模式0与binding popup2一致，说明至少某一历史阶段错误值已持久化，而不只画布标签。
- 反证/限制：SUPERVISE和native映射均已修；CEP89有强制setValue及UUID重取；AE正常应保存用户原生edit。没有证据证明 `.value` 与AEGP getter在该失败中不同，更没有证明AE拒存selector。旧0证据不能证明现在2。若请求先失败并rollback到旧LookAt，H1不是首错。
- 最小验证：E1一次原生切换与一次CEP切换，分阶段并排记录delivered/script值、native saved popup、compiler core mode、ARB mode，之后只改一个非mode字段；依据**第一次不等**决定是入口、持久化还是后续重编译。

### H2：结构变化后旧PF回调binding被按错误kind布局读取

**优先级：F2最高；也可能间接造成F1失败恢复。** 删除、备份复制/重排/恢复使一个原Force binding对应现Motion；native86源callback在fresh kind+UUID校验前开guard stream，或对不完整delivery使用本kind索引。

- 支持：实际对话框的kind对；native86 NodeGraphSync:336、357–361、379在stream打开前没有fresh actual kind+UUID/完整count；源guard读取早于renderer guard判断；full backup引起physical parade变化；native87现有fixture刻意将Motion PF binding指向formerForce slot，说明覆盖模型可表达这一风险。
- 反证/限制：native86已有Host exclusion query，Model owner scope内正常会defer；saved sourceguard非零也defer；native编译/alias/backup有kind/full UUID验证。没有selector/调用栈或对话框时的scope值，不能说一定是NodeGraphSync；可能来自其他UI、ARB或宿主内部旧引用。native87 fake绑定不等于AE实测。
- 最小验证：E2按具体delete/cleanup/restore阶段关联每个callback入口和首个SDK调用；对照有无Force、Force/Motion parade先后、ECW可见/收起。若first异常发生在source guard stream之前且expected/actualkind不同，才支持此边界；若回调全部defer且异常出现在finalize，则转H3。

### H3：失败恢复/清理完成状态早于依赖重建，随后回调或重编译采用恢复状态

**优先级：第三；偏向F2及F1的“失败后回退”，不能用作全部成功编辑的解释。** full restore替换了对象却无fresh rebind；discard失败仍放guard；或ordinary请求先因身份/绑定错误失败，正常rollback旧mode被用户观察为模式回退。

- 支持：第4.4的保留源码行为；旧76/77已观察guard0 binding unavailable；78/79撤回后restore rebind不在active路径；CEP错误catch会重写旧作者node，若旧mode为2自然回到2。这里可建立两症状的同事务关联，而无需假设enum错误。
- 反证/限制：正常成功full delete已有final refresh，ordinary rollback顺序已修且失败保guard；现有日志没有当前failure/cleanup outcome，也没有证明1→2前发生rollback。若干净工程单原生切换、无delete/restore也失败，H3不能单独解释F1。旧stream258错误与当前kind对话框不同。
- 最小验证：E3在现有offline fixture中只注入finalize失败/prepare或commit失败，观察实际guard及fresh binding；随后在owner失败操作中核对是否走同支路。必须区分“源码存在风险”与“本次AE根因”。本轮不实现fault injection或rebind补丁。

**共享性判断**：H1与H2可能共享源callback的freshness/通知时序；H2→commit失败→ordinary rollback可以把删除身份异常与mode2复现连接起来；H3涉及同一guard/transaction生命周期。也完全允许F1为独立selector持久化问题、F2为旧PF binding问题。没有共同requestId/UUID/阶段轨迹，当前不判定共享根因。

## 6. 最小区分实验与诊断插桩提案（未实施）

### 6.1 先定义通过/反驳标准

| 实验 | 最小输入与独立观察 | 区分结果 |
| --- | --- | --- |
| E1 作者/保存三方对照 | 固定一个UUID；原生LookAt→Circle，再只改Speed；另一个干净副本CEP同序列。记录请求结果、popup、saved mode、ARB，不靠CEP snapshot替代ARB | native edit后graph1/source3：持久化H1；script1/native3：跨getter H1；全部1直到一条旧callback后变2：H2候选；先失败/rollback再2：查H2/H3；仅CEP标签2而源/ARB1：转panel refresh假设 |
| E2 删除首个访问 | 同样小图，先有/无Force各一次；有Force时固定parade顺序，并分别单删Force、单删Motion、批删二者。记录删除/backup/reorder/restore边界及selector | structural mutation后callback入口就wrong kind支持H2；只有cleanup/restore后异常支持H3；无Force仍报expectedForce应查宿主旧绑定/另一target，不推导参数ID相撞 |
| E3 条件失败路径 | isolated offline source，强制finalize返回错误、另强制prepare/commit失败；观察guard、full restore对象身份与fresh binding。保留成功支路对照 | 可直接验证H3的不变量缺口；只有owner实测进入同分支才建立因果。不能凭fixture复现命名“AE已复现” |

第一批建议E1的一原生/一CEP操作，再E2一个单删Force触发Motion槽位移动的场景；只有观察到failure/restore才扩到E3。无需先跑全部模式排列、延长timeout、关闭严格校验或发布native87。负结果不能证明无故障，记录尝试数、确切版本和未触发即可。

### 6.2 最小插桩位置与字段

现有journal不够提供E1/E2的跨边界证据。下列为**诊断候选提案**，不在本轮加代码，不新增参数/guard协议。先以结束已部署87/90人工确认是否仍复现；若继续失败，在owner指定完整pair上只加观测。若必须比较86/89，应由owner明确批准paired回退/诊断范围，本轮不恢复它。87新增作者写入会改变H1的窗口，因此87成功不能单独证明86根因；发布仍须配对、保留rollback。

| 插入位置（native86基准） | 最小记录 | 预期证据 |
| --- | --- | --- |
| gateway作者1391/1409、1640/1645；ordinary2325/2335及catch2340起 | requestId、target、UUID、wantedCore/popup、script before/after、源guard释放、成功/失败/rollback；不永久保存Property对象 | `.value`是否确认错误权威，回退是否实际rollback |
| NodeGraphSync:325入口、357开borrowed ref前、361 guard访问前、371定位renderer后、385capture后 | process/monotonic eventSeq、selector、module kind、PF ref仅短时标识、num_params/index、完整delivery时UUID和popup、已知exclusion；首SDK调用label及结果 | 抓首错前的callback身份/guard。logging不能为诊断再次打开可疑PF binding |
| NativeNodeGraph已完成actual matchName/UUID解析后、914 mode读取后；NativeGraphCommit编译/graph写/receipt/verify/rollback | 使用已有读结果：actualkind+UUID+parade index、nativepopup、compiledCore、revision/CRC/nonce、binding mode常量、ARB已读取bytes的mode/摘要、stage/error | 区分compiler读错与保存/后续改错；不只记录compiler自行生成的expected bytes |
| transport prepare/commit/remove与ModelGraphTransaction result；EffectGraphBackup duplicate/delete/reorder/restore/discard；Host scope出入 | transactionId/target、removed UUID、mutation before/after、guard、scope token、finalize/restore/cleanup结果、actualkind+UUID的已存在inventory读值 | 哪次structural变动先使引用失效；guard是否在未确认恢复时释放 |

按一次失败操作采集，日志写到checkout `artifacts/`，有界、可关闭、不得改变返回值/重试/控制流。使用wall UTC时间和进程内eventSeq关联，composition time另列。不得额外打开已疑似失效的effect ref来“验证它”；先记录已有上下文，fresh inventory仅复用原路径已经执行的读取。不要用新native author写入或guard4..6“顺便修复”观测对象。

ARB检查尽量利用已有publication读回handle；若必须额外读取，限定一次明确诊断点，记录observer effect风险，不每帧扫描。无extra SDK调用的轻量日志优先。原生日志与JSX日志通过nonce/transactionId/target/UUID关联；仅PF指针或graphCRC不够。无需registry、环境变量、Adobe缓存、自动AE启动或调度脚本。

## 7. 用户在 AE 内执行的具体最小测试（等待批准，未执行）

先记录AE“关于”完整版本/build、工程是否原历史副本、panel显示版本与当前使用的pair；结束时磁盘pair为87/90，应先确认该pair的表现，结果与86/89分开。用**新建临时工程或历史工程副本**，保留原工程。诊断候选未批准/未部署前可做肉眼定位，但不能把它当作三方值已采集。无需重置AE/缓存，不自动恢复旧pair。以下灯光不参与模式identity测试，不需要先制作Light Path动画。

### A：模式（先两次操作）

1. 建一个solid加Starfield，通过CEP建立最小 `Emitter → Particle → Motion → Output`，保持paused和固定当前时间。记新增Motion默认值及effect列表顺序，不先改其他属性。
2. 在原生ECW选择LookAt；等待操作/刷新结束后选择Circle。记录是否立即回退、CEP响应是否成功或红色错误、两个UI显示值；发生异常就保留完整报错。
3. 只改同Motion的Speed一个小量（例如100→90），观察是切Circle时回退还是这次非mode编辑后回退。不要连续快速切模式，否则难找首错。
4. 在独立同样小图上，经CEP执行LookAt→Circle，再只改Speed。记录与原生路径差异。每步记墙钟时间；有诊断日志时由UUID关联，不用“第几个节点”关联。
5. 若失败立刻停止该副本编辑，另存为新的证据工程；若都不失败，报告0/尝试数，再用历史副本同序列一次。保存/重开只能作为随后独立实验，避免把它混进初次模式失败。

验收不只看画布：请求status、nativepopup、ARB mode、fresh snapshot应分别一致。无诊断候选时ARB未知必须注明，不猜测。

### B：删除（先一个单删，后必要对照）

1. 新建第二个临时小图。先创建Force，再创建Motion，使物理effect列表明确包含Force在Motion前；将图连接成 `Emitter → Particle → Motion → Force → Output`。记录连线顺序和effect列表顺序，二者不能混为同一个顺序。
2. 保持paused、固定时间，ECW显示这层；从CEP单删Force（预期Motion物理槽位前移）。只做这一操作，记录完整actual/expected文字、CEP成功/失败/完成warning、Force是否真的消失、Motionmode是否同时变。等待结果，不立刻Undo。
3. 保存失败副本与日志；诊断结果未明确前不连做修复。若需对照，在新的相同副本先单删Motion，另一副本批删Force+Motion；不要在已异常图上比较三种delete。
4. 仍未触发时，做一份无Force的小图单删Motion；再做ECW收起的同样副本操作。这样区分Force旧binding/ECW回调依赖。只在必要时用历史工程副本复试，不默认高节点数就是必要条件。

一次失败后所需材料：操作步骤和墙钟时间、AE完整build、确切pair、完整弹框、transaction返回结果/diagnostics、证据工程副本、该时间段journal。无需执行旧自动acceptance脚本；本次不要求用户手动运行会写宿主状态的诊断脚本。

## 8. 现有日志能定位到哪里，还缺什么

### 8.1 journal实际内容与来源

取证快照按build分组；原文件可能含离线fixture与AE记录，不能仅凭文件名认定是宿主日志。

| journal | 本轮快照 | 可用判断 |
| --- | --- | --- |
| native84 | 2,206行：PID46552 2,198行（2146成功/52取消）；另两PID各4行 | 与旧owner-evidence的2,198帧吻合；没有非cancel render错，不排除UI故障 |
| native85 | 3,388行：PID15264/18384，分别2547/841；仅成功/取消 | 缺各次操作/进程归属的共同receipt，不能拿正常帧说明mode/del没错 |
| native86 | 16,593行：既有current-state标识的AE PID42164为8863（8834成功/29取消）；PID40604为7722（97成功/7624取消/1个516）；另两PID各4行 | 7722/7624/97及516原文与 `regression-audit/deletion-lifecycle.log` 完整吻合，是故意unguarded missing-source负面对照，不能认定AE故障；AE那批未记录非cancel render错误 |
| native87 | 15,452行：两PID各7722同样pattern、另两PID各4行 | 采集时与在途deletion-lifecycle日志吻合；其后外部发布由另一份收据证明。此journal快照不是87的AE修复证据 |

native86负面对照 message明确是stream98、node `00000000000000000000000000000001`、parameter1/component0、516；它不是owner的Motion/Force actual/expected原文。`HAP-log.txt`只一行output plugin exit，与这两个故障没有可关联状态。

当前GraphRenderJournal只写build/PID/PF ref/composition time/guard/result/message，没有wall timestamp、target、UUID、revision、nonce、exclusion、selector、作者mode或ARB实值。`time/scale`是合成时间，会倒退/重复，不能当作事件墙钟。PF effect指针也会随对象替换改变，不能当UUID。render成功不意味着UI callbacks没有弹框。

native79历史版本曾写 `UI begin/bindings/published/result`，随方案撤回不在native86 active源码。历史generation记录反而没有支持78假设。当前部署/backup receipt记录的是文件配对，保存工程提取记录的是最终状态；两者均无法重建当前单次失败的中间边界。JSX transport已有stage/outcome诊断，但本轮没有找到与owner当前失败相配的原始响应轨迹。

### 8.2 是否重新引入旧问题

| 旧缺陷 | 当前检查 | 结论 |
| --- | --- | --- |
| popup3 native reject / CEP87 Core2 reader reject | native mapping1/2/3及record0..2；current CEP89与部署源hash一致 | 没有静态证据显示重新引入；87旧unsupported-mode bug不作本次根因 |
| Motion Speed Number(record)→NaN、key4/17冲突 | writer用authored scalar mode；native单stream产生两种Speed表示 | 旧writer缺陷已改，不足以解释当前selector回退 |
| selector没有SUPERVISE | native86 setup_motion:Type和AfterPath均带SUPERVISE、仍nonanimated | 已补；“没通知”不能不加证据再次认定完整根因 |
| ordinary先物理删新增源再恢复 | CEP89 restore顺序/failed guard保留仍在 | 未发现重引入 |
| full restore无fresh rebind；cleanup失败仍guard释放 | 保留native86与current EffectGraphBackup哈希一致，源码仍有上述行为 | 风险存在；79处理曾明确撤回，不能包装为新回归或据此自动恢复79方案 |
| borrowed PF source binding / delivery count不足 | 失败基准native86有风险访问；native87已在外部工作中改变并发布 | 尚无宿主触发或87资格证据；不能把新测试通过当作86根因已经被验证 |

### 8.3 能确认和不能确认的边界

能确认：旧CEP87首次拒读在planner的Core2 validator；旧orphan故障源数据是missing disconnected Turbulence；旧alias/backup/guard窗口有源码与fixture证据。这些是**已定位的历史边界**。

最后有失败报告的86/89，F1只能把优先范围缩到“作者→源保存/原生编译→graph出版→随后重编译或恢复”；F2只能缩到“physical parade变化→某个callback/stream访问或cleanup/restore”。没有足够数据确定第一条错值/错kind访问。结束已部署87/90是否仍失败是新门槛；两版本不可混记。尚缺：

1. 一次native86/CEP89失败的完整target、UUID、作者请求、transactionId/nonce与成功/失败outcome。
2. 同次操作delivered/JSX getter/native saved popup/独立ARB mode的阶段化对照。
3. dialog首发selector与调用边界、actualkind+UUID、完整delivery count、source/renderer guard及Host exclusion。
4. physical removal/backup cleanup/restore/finalize的事件顺序与错误码；effect parade前后identity。
5. 当前失败所用AE确切build、最小图与尝试数，而非借用历史host或fixture证据。

**调查在此停止。** 本轮不修改业务代码、不追加兼容patch、不改参数/ABI/schema、不给宿主故障判通过，也不自行开展上述插桩、实验、候选部署或AE操作。等待owner批准一个具体观测实验或在证据明确后批准修复。
