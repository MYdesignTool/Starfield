# 分批工程治理提案

本轮仅产出审计；下面GOV编号是待owner确认的任务提案，尚未写入现有backlog或修改业务。M3-20修复和其他在途实现保持原所有权。结论依据见 [TECH_DEBT.md](TECH_DEBT.md)，实测基线见 [BASELINE.md](BASELINE.md)。

## 最优先的三个任务

| 次序 / 任务 | 工作与拟拥有文件 | 风险与验收 |
| --- | --- | --- |
| 1：GOV-01，可信构建与回归入口 | CMakeLists、RunAll/RunPanel及专用runner清单、model_transport测试契约、结果manifest；修遗漏resource input，明确current/legacy ABI，纳入现有专用fixtures和完整DOM；不改生产算法 | **低至中**：错误放宽ABI断言、误称skip为pass、覆盖旧证据。验收：clean Core DLL/20项当前CTest构建可运行；明确未注册专用suite；统一offline清单无静默遗漏；旧catalog/ABI/遗漏source负面对照必失败；所有生产source hash不变 |
| 2：GOV-02，图事务回归闭环 | 现有native lifecycle/backup/exclusion tests、JSX DOM fixtures、手工AE场景/日志索引；本批只测试与证据，不改gateway/native transaction行为 | **中**：mock不能复制真实AE时序、过度推导根因。验收：ordinary与整图路径逐阶段fault injection、guard/receipt/rollback/refs不变量通过；当前kind/注册输入有来源；owner在明确AE2023 build和候选hash下复现/确认开放问题，否则gate仍open |
| 3：GOV-03，参数契约与Codex上下文收敛 | schema只读审计；实际注册catalog与JS/JSX/codec对照tests、Assert-PanelContract；文档当前入口/模块阅读包。先做验证/索引，不改ID、wire、ABI、默认值 | **低至中**：把stream/disk/key混同，生成新“真源”或复制旧checkpoint。验收：八kind、三Motion模式、Turbulence映射、Speed单位及整个snapshot读写一致；不合法record继续拒绝；当前入口不内嵌历史计数/版本，任务阅读包准确指到owner和最小回归 |

这三个任务可分别审阅、合入、撤回；先修证据链，再决定业务重构。GOV-02的host步骤保持owner-led，未获新宿主授权时只准备/收集，不能为了关闭任务自动启动AE。

## 后续批次及依赖

| 批次 | 依赖 / 范围 | 可验证边界 / 停止条件 |
| --- | --- | --- |
| GOV-04，增量构建与测时 | GOV-01；现有Core静态library/CTest或MSBuild，build wrappers/results；逐步复用相同宏/flags的test对象 | 对照clean/no-op/单cpp/header，分compile/run；保留 `/MT`、SDK/PiPL、NodeKind隔离与adapter指纹。未改善或key不完整即撤回；不先并行共享output的runners |
| GOV-05，宿主图桥职责拆分 | GOV-02/03；先独立提案NativeNodeGraph的纯record codec/Output可达性与host读取/alias事务；每次只拆一个内部边界 | 参数ID/wire/generic protocol/guard时序/失败诊断不变，production调用点及failure fixtures对照；ABI或Render变更须单独owner任务、ADR和迁移，不在此顺手修改 |
| GOV-06，求值planning与shared helpers | GOV-01/03；先建立静态/temporal等价条件及独立oracle，再减少ordered processor planning的双路径改动 | 固定seed/身份/预算/取消，Auxiliary出生、edge顺序、CPU/GPU/Model affine、旧snapshot字节一致或明确容差；不合并有意不同的积分模型。任何行为差异先定位、另开bug，不以重构吸收 |
| GOV-07，缓存/资源与诊断 | GOV-02/04；缓存owner/identity/生命周期表和多实例诊断，再按证据细化API | copied UUID+不同PF_State不串用；cancel/death/reload释放测试；same-generation结果释放保持。没有竞争/泄漏证据不盲目消灭global或引入新scheduler |

每批提出精确owned-file list后再开始；有业务文件所有权交叉时先与M3卡协调。禁止一次混入参数新增、语言迁移、GPU重写或MFR启用。

## Codex上下文方案

当前目录边界可用：Core类型与Render contract独立、AE对象集中adapter、CEP已有planner/transaction模块。困难在于“模块名”并不等于修改边界：删除走Model事务、主AEX编译Core图源码、节点同工程多kind、静态/temporal行为分支。无需复制整仓库到长prompt，采用下面四个短入口。

1. **唯一当前入口**：保留 `docs/current-state.md` 为候选/部署/gates事实源。未来把其中当前摘要与历史移交分开；AGENTS只链接它和关键授权，不固化native54等易过期版本。架构文档只讲职责，历史构建叙述链接history，不删除历史证据。
2. **模块阅读包**：每个模块README或一页索引包含职责、owned路径、入口函数、输入/输出owner、调用上/下游、不可变契约、最多5条关键回归命令、相关ADR。优先Graph/temporal、native graph bridge、CEP transactions、render/resources四包；复杂的NativeNodeGraph按职责阅读，不先物理拆文件。
3. **任务卡**：一个ID一次。开工读取AGENTS →当前摘要 →所属模块包 →relevant ADR →card owned files →git diff（含新文件）。卡记录预期不变量、最小验证、开放host gate；跨层修改明确列JS/JSX/native/Core/schema影响矩阵。不以branch名字选择当前功能。
4. **结束回执**：candidate source manifest/命令/实际执行suite/失败与skip/开放问题/下一步/回滚位置，链接artifact，不在五份文档和AGENTS复制checks/hash。临时dump和会话记录只放artifacts；长期文档只留可复现结论。

契约owner索引至少覆盖：main参数 `schema/parameters.json`；node磁盘ID `schema/node-parameters.json`；Core值 `Settings.hpp`；Render边界 `Render.hpp`；C ABI `PluginApi.h`；sequence/graph/snapshot各codec及ADR；native stream layout/绑定record；ES3 factory `tools/candidates/native_record_contract.js`。索引说明disk ID、stream index、graph key、popup/core enum、单位换算之间的关系，不复制数值表形成第二权威源。

可持续性验收：模拟“修改Motion模式/删除路径/纯Noise kernel”三张任务，依据入口找到owner、调用边界和必须运行的suite；未读历史也能明确当前候选，遇到旧方案由撤回标记阻止误用。新增节点必须同时更新契约矩阵/测试manifest/模块索引；不要求每次扩写长报告。

## 小批次、回滚与发布纪律

- 每批开始先保留dirty/untracked source manifest与原文件；候选必须包含未提交/未跟踪实现，不能从HEAD重建后宣称等同。后续若隔离工作区，先确定如何完整携带owner的当前工作，不自动使用clean/reset。
- 分开文档/测试/构建/纯内部重构changesets；单批不能既改预期行为又重组代码。先有旧行为的可执行fixture和负面对照，再动内部边界。
- 验收记录按源hash锁定；模块共享源改动走full paired build，CoreOnly继续服从指纹。ID/matchName/version packing/schema/ABI改变单独写ADR与migration/compatibility计划。
- 回滚只撤该批增量或在隔离候选切回保留前态；已提交批次用可审阅revert，不覆盖别人的dirty文件，不执行git reset/clean。未提交批次按开始manifest恢复**仅该批owned文件**，出现外部改动先停查。
- 每批源码回滚后重跑对应最小gate并核对hash；准备前后候选而不改dist。任何部署要保留完整native/Core/CEP配对和既有Restore-TestBuild一条rollback；启动/停止AE及其他宿主动作仍需ADR0011单独授权。本轮无发布。

## 本轮完成与下一轮门槛

本轮五份文档、原始失败/成功日志、源身份/依赖/计时基线已经建立。CMake/过期测试/聚合覆盖缺口按事实保留；实际AE失败没有由离线通过来关闭。下一轮等待owner选择GOV-01/02/03及精确范围，再建立正式backlog卡与逐批验收；不自动开始任何业务重构。
