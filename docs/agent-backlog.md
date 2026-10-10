# 当前任务卡

2026-10-11 当前活动卡重新为M3-17：owner反馈63/65仍state=executing。64/66候选消除完成通知Array原型依赖，保留执行/回滚后的数字终态，250ms仅重发同一收据；不丢失失败/cleanup/undo信息，不覆盖在途写入或重跑executor。actual JSX双VM复现旧拒收、新字符串完成，但实机根因仍假设。仅修Model，Motion/MNT保持，完整冻结构建及新配对待发布。

旧任务卡和精确历史所有权见 [归档](history/2026-10-07/agent-backlog.md)；产品进度见 [路线图](roadmap.md)。

2026-10-11 M3-17停滞修正版已部署，等待owner验收并按要求暂停。完整冻结4988627源码/May2023八目标terminal exit0，00:28+08 AE process0后发布native63/CoreABI8/CEP65；26hash、selector/runtime及paired rollback report通过。空诊断句柄处理、只读Request/Claim、executor前Begin、未写入阶段无进展恢复与executing/applying保留已接入。备份m3-17-native63-panel65-host-receipt-20261011恢复62/64。实际AE默认添加/OBJ/预设/撤销重开仍开放，Motion及MNT保持，完整目标未完成。

## MNT-01 — 工作区整理（2026-10-07）

owner 授权：清理过时文件、文档、测试与编译产物，修正文档错误，暂不修改项目主要代码。
拥有：README/AGENTS/CONTRIBUTING、docs 当前入口与 history、模块 README、编辑器配置、测试夹具和运行器、清理工具以及 artifacts 内可再生成产物。
保留：正在开发的 Transform 主要源码/schema/版本改动、dist 当前安装包、SDK/GPU 构建依赖、disabled 回滚备份、prepared 候选、crash/reference 证据和登记 Git worktree。
不包含：主要 Core/AE/CEP 行为实现、身份/ID/ABI 迁移、部署、AE 操作或宿主状态修改。
完成条件：当前/历史文档分离、坏链接与失效说明修正、旧测试断言同步并运行、清理路径/链接检查与删除清单保留，报告真实失败和环境限制。

## M3-11 — Transform（继续开发；本次整理不承担实现）

owner 的十四项参考控件：Inherit Motion（Null）、Anchor XY/Z、Position XYZ、Rotation XYZ、Scale XYZ、Particles Scale/Opacity。
拥有：Settings/ParticleTransform、图构建与求值、仿射基底/CPU/GPU/快照传输、原生 Transform/Null 作者与采样、CEP inspector/gateway/preset、schema、配对构建与 focused tests、ADR0032。
Render.hpp/Core ABI 变化须先更新 ADR0032。现有 kind/key/磁盘 ID 保持；不暴露无效控件，不将纯数值测试称为 host parity。

native54/panel54 与 Core ABI6 已配对部署；Transform 的 Core、原生、CEP authoring 候选和数值 Null 资源绑定已实现。disk1401..1414、synthetic binding fields15..26 与上限由 ADR0032 定义。工作树版本标记已推进到 build55，但未提交的业务改动不属于 MNT-01。
尚待：实际 AE Null/父级/剪切/反射/奇异尺度、Force suffix/Auxiliary、motion blur、取消与有界工作、undo/reopen 验收。宿主 qualification 保持开放。

## 已有实现的开放验收

| ID | 剩余 gate | 文件所有权边界 |
| --- | --- | --- |
| P-02L | panel54 拖放/外观/撤销 | node_palette/panel、样式、focused palette tests、ADR0009 |
| P-02K | 实际 idle/seek/Add/Replace 延迟 | panel/gateway/preset 性能与对应 fake-host tests |
| P-03 | 预设 Add/Replace、资源、银行参数、撤销与重开 | preset manager、native launcher、gateway、ADR0029 |
| M3-07/08/09 | 原生曲线/渐变/旋转的完整 host 验收 | editor/native Particle/Force、schema 与 ADR0027/28/30 |
| M3-10 | AE shutter、camera、格式与生命周期 | MotionBlur/SmartRender/history/CPU/GPU、ADR0031 |
| H-01 | 在途 Core 切换、取消和广泛像素一致性 | CoreLoader/C ABI/SmartRender/build/deploy、ADR0012 |

其他参考行为族、MFR/实机 GPU 和安装迁移在安排实现前建立独立任务卡；保留历史卡中的约束与尚未关闭的验收。没有主动并行 agent 授权时不分派子 agent。

## M3-12 — Particle Transfer Mode 与节点效果定位（2026-10-08）

owner 要求：Normal/Add/Screen/Stencil 粒子间叠加；点击 CEP 节点后展开并滚动到原生效果。
拥有：Particle Core 类型/graph/求值/快照/CPU/GPU，Particle native 控件/记录/绑定，StarfieldHost UI reveal 命令、CEP graph inspector/gateway/preset 映射，node schema、Core ABI/version、配对发布、ADR0033。
依赖：M3-11 当前 native51/CEP51；owner 本次已确认上次删除和相对 Null 引用修复无问题。广泛 undo/reopen gate 保留。
迁移：ADR0033 先行；追加 disk232/stream519，保留现有 streams/IDs，graph key30 optional/Normal，private binding v3/snapshot5/Core ABI5。
完成条件：全链路实现与构建，AE 关闭后配对发布并核对哈希/回滚；本次不新增或运行实现测试。实机行为由 owner 验收。

## M3-13 — Particle Texture / Layer 采样（2026-10-08）

2026-10-09 owner确认当前版本节点添加已修复，关闭合法Texture引用画布的添加gate；当前配对为native60/CEP61，十八安装哈希复查与收据一致。背面及采样/阶段/持久化gate仍开放；M3-17继续活动实现。

M3-13 修复已发布并等待 owner 对 CEP60 添加/背面实机反馈；不再同时修改本卡实现。当前活动卡切换至 M3-16。native58/CEP58 与默认方向 Core 维护已发布；owner 确认 Comp 2 可选择/显示，随后报告背面无效、选择标签空白和已有 Texture 引用时添加节点报 parameter31（无引用画布正常）。native59 改为原生行内 PF_LAYER，显示已由 owner 确认；CEP60 明确范围分支与细化诊断已发布。该维护未混入 M3-16 与 MNT，编译/最小测试不关闭宿主添加、正反面、八种采样/持久化及 Source/Masks/Effects gate。见 ADR0034。

2026-10-09 08:33 +08:00 fresh no-AE 后发布cb1048d/native59/CEP59；七个 native/Core、十一项 CEP、selector 与 exact58配对备份/恢复 report 核对。收据 artifacts/m3-13-native59-deploy-{before,after}.json。等待 owner 的选择标签/背面/已有 Texture 添加实机证据，完整 goal 继续。

owner 已确认选择器显示，添加仍拒绝合法 ID44/type3/kind number。CEP60 候选将图层/枚举/开关范围改成明确分支，保留严格校验并记录 reason/typeKind/max；旧嵌套表达式的引擎解释仅为假设。80项 Texture CEP、完整 ID44 事务/预设/失败回滚及启动通过；native59保持不变。实机添加及背面仍开放，本卡继续。

08:40 +08:00 fresh no-AE 后使用 Deploy-TestBuild -KeepNative 发布6f7b1bc/CEP60；native59/ABI7与selector不变，18文件及 exact59/59配对回滚核对。收据 artifacts/m3-13-panel60-deploy-{before,after}.json，等待 owner 实机回复。
拥有：Settings/ParticleTexture、Render.hpp/C ABI/快照、图求值/CPU/GPU fallback、AE texture 作者与 SmartFX checkout、CEP/资源/预设、schema/version、focused texture tests、ADR0034。
依赖：已部署 native52/CEP52、ABI5；参考字段和八种采样菜单已核对。Freeze Frame 经 owner 明确选择为粒子出生时的图层画面。
迁移：ADR0034 先行；Particle optional keys31..36，shape3，snapshot6/Core ABI6；新增 native/main 控件只追加，适配器布局在更改前补充 ADR。
完成条件：八种采样、正反面、颜色使用、比例/透视全链路实现，必要最小测试与 AE2023 构建、闭宿主配对发布；真实 AE 图层/undo/reopen 验收。其余 Face/Model/Cloud/Path/Shadow 等按后续卡推进，完整目标保持开放。

## M3-14 — 主效果名称/顺序/入口（2026-10-08）

owner 完整 goal 的主效果排布要求。M3-13 Source 里程碑 native53/CEP53 已部署；实际 AE 与 Masks/Effects gate 保留。
拥有：主 Parameters/MotionBlur/NativeGraphCommit 索引映射、PresetsUI/MainLauncher、主 schema、CEP 主全局查找、版本/构建指纹、focused main tests、ADR0035、配对发布。
迁移：仅重排 main610..625，保留所有磁盘 ID/默认值/type、逻辑 parameter id、其余物理索引及 ABI6；具体映射由 ADR0035 先行规定。
完成条件：已有主分组按参考相对顺序排列、开关/Panel/Presets 入口、必要最小检查、May2023 构建与闭宿主配对发布；旧工程动画/撤销/重开及数字索引表达式由 AE2023 实机验收。完整目标及剩余粒子行为族继续开放。

## M3-15 — Particle Cloud 圆群（2026-10-08）

M3-15 源码/配对发布完成，真实 Cloud 与 Texture gate 等待 owner；本轮产生作者代码、测试、May2023 构建、native57/CEP57/ABI7 发布与推送，属于 progress。完整目标 active，其余 Particle 行为仍开放。
拥有：Settings/ParticleCloud、ParticleInstance、graph keys/registry/construction/evaluation/history、MotionBlur、SpriteGeometry/CPU/GPU scene/kernel/driver、PluginApi、版本与构建指纹、native Particle 控件/绑定/记录、CEP inspector/gateway/preset、node schema、focused Cloud tests、ADR0036 与配对发布。
证据：owner 的 Density0/66/100/200/1000 截图；Circles10、Aspect150 不变，Density0 重合为圆，Density 增大使成员中心散布，最大1000。确切随机分布与 Aspect 轴向缩放仍是独立实现假设，须实机比对。
迁移：ADR0036 先行；optional keys37..39、shared Cloud style、snapshot7/Core ABI7，旧图/快照保留固定五圆；追加 native 控件，不改既有 IDs/序列 schema。Core 数值里程碑之后完成作者控件再配对发布，不提前暴露无效 UI。
作者里程碑：build57/CEP57 候选完成 Cloud topic528、Circles529、Aspect530、Density531、end532 与 hidden constant activation533，保留旧效果；private binding5 兼容旧 v1..4。原生绑定/相机7647、实际 Particle 回调277、控件/Texture selector276、Cloud CEP49、Texture CEP47 与完整 gateway Cloud Add/Replace/rollback 检查通过，May2023 构建通过。native57/CEP57/ABI7 已闭宿主成对发布，七个 native 与十一项 CEP 哈希、runtime selector、一键 exact56 配对回滚报告通过；真实 AE gate 仍开放；其余共享 MNT 改动保留并排除。
完成条件：Circles/Aspect/Density 全链路可用、圆群统一透明度/运动/寿命/身份、稳定种子、CPU/GPU 和 shutter/crop/预算兼容、必要最小测试、May2023 构建、闭宿主配对发布与回滚；AE2023 外观/动画/undo/reopen 验收。其余 Particle 行为族与原完整目标继续开放。

## M3-16 — Particle Shift Seed / Birth Chance（2026-10-08）

2026-10-09 当前活动卡为 M3-16：图求值3348已提交73b7ad8；作者90b7a2b完成绑定8441+相机12、回调344、注册269，Birth51/Cloud49/Texture80、完整事务和启动。完整May2023 /MT通过，09:14 +08:00 fresh无AE后已配对部署native60/packed32828/CEP61；18安装+18备份及selector独立核对。候选 artifacts/prepared/m3-16-native60-panel61 与exact59/60回滚保留。当前继续真实AE gate；M3-13节点添加/背面反馈到达时优先处理。
拥有：Settings/ParticleBirth/Random、graph optional keys/registry/construction/evaluation/history、候选扫描预算和 branch identity、native Particle 控件/记录/绑定、CEP inspector/gateway/presets、node schema、构建/version、focused birth tests、ADR0037 与未来配对发布。Render.hpp、ABI7 与 snapshot7 计划保持；需要变更时先修订 ADR。
参考：owner 参数库存默认 Shift Seed0/Birth Chance100；官方指南要求偏移同一发射源的 seed、按出生概率筛选其粒子。范围边界与精确 RNG 仍需参考确认。先完成纯核心确定性策略，随后接入静态/历史/Auxiliary 求值及 native/CEP 作者；未接入前不暴露控件、不声明完成。
迁移：optional keys40/41；缺字段保留旧分支行为，明确作者字段启用完整发射源分支。新 native 控件追加且通过独立 hidden activation 保留旧工程。已有 purpose1..20/IDs 不重用，出生筛选新增 purpose21。
完成条件：偏移影响发射运动和全部随机属性；概率0/100/中间值、动画按出生采样、父子发射、共享 cap/work/cancel、稳定身份与 shutter 匹配；focused tests、AE2023 构建、闭宿主发布、回滚、owner 可见行为和 undo/reopen gate。完整目标仍 active。

## M3-17 — Particle Model 几何与资源（2026-10-09）

2026-10-10 Model receiving停滞：owner只读实机报告确认p0-c1-l29/AE23.5x52/main、command5061、Host entry function、receiving且余270秒。空诊断句柄fixture复现丢弃SDK成功回复；原生三个通道读取字符串、Model只读Request/Claim及executor前Begin保护整个backup/rollback，十五秒无进展恢复仅限未写入阶段，忙碌显示state、poll250ms。native63/packed32831/CoreABI8/CEP65候选完整构建/部署继续；不改变持久ID/ABI/schema/Shape六项，不推进暂停的Motion。模拟回归不认定此实机问题已修复。

2026-10-10 21:43+08修复发布：7fda058冻结完整May2023 /MT八目标实际exit0，fresh AE count0后部署native62/packed32830/CoreABI8/CEP64；26安装hash、selector/runtime、实际paired rollback report通过，备份m3-17-native62-panel64-root-id-20261010恢复61/63。owner已确认CEP63加载、p0-c1-l29身份，root0修正版真实AE添加/OBJ/资源预设/undo/reopen待验收。owner要求今日部署后暂停目标，完整目标不记complete；Motion今后按可操作阶段分批交付，不要求三个作者一起发布，数值层无作者入口不作为可验收模式。

2026-10-10当前卡切回M3-17：owner确认CEP63能加载、Model Add失败，只读AE23.5x52报告p0-c1-l29证实root ID0。CEP及Model事务/asset Host/OBJ checkpoint接受非负root，comp/layer及实际SDK重验保持；native62/packed32830/CEP64候选，schema/ABI不变，完整冻结SDK及配对部署继续。标准/ASAN事务9027364/9027464、资产149165、导入20628和CEP356/DOM35/export588/作者74/Shape92/ES3 18/只读报告11、新旧dummy配对61/60通过。安装61/63保留，真实AE Model gate开放；Motion在途/完整目标不缩减。

2026-10-10 14:27+08 owner关闭AE、fresh process0后，CEP修正源0e5cbc5发布native61/CEP63。通过既有Deploy-TestBuild KeepNative保留八native/Core/selector，十八CEP/26hash和paired Restore report通过；备份m3-17-native61-panel63-es3-20261010可恢复61/62，旧60/61回滚继续保留。byte→byteIndex修正已安装，实际AE63加载及Model/OBJ/预设/撤销重开仍待owner，不关宿主gate。发布工具将nativeSourceCommit与CEP sourceCommit分列，避免热面板提交被当作新native构建。

2026-10-10 owner报告CEP62保留字加载错误；ES3静态解析复现Model三处byte变量。CEP63隔离候选改byteIndex、统一generation，三个JSX ES3检查18/旧modal入口拒绝90及Model/Particle/Texture/Cloud/Birth/gateway/startup回归通过。新增默认report的panel-only配对工具调用既有Deploy-TestBuild KeepNative，dummy发布恢复62项通过。真实AE资格保持开放；owner关闭AE后发布native61/CEP63并保留61/62回滚。剩余Shape/Use Model(s)/实际Model gate及完整目标不缩减。

2026-10-10 13:59 +08，owner授权测试版已发布native61/packed32829/CoreABI8/CEP62，源8d9b50e完整冻结May2023 /MT八目标实际exit0；26文件hash/selector/runtime与paired rollback report通过，before/after及日志artifacts/m3-17-native61-*.json/.log。新Host/Shape/Model作者/网格资产/renderer/CEP整笔资源同版发布，没有独立半成品Model。备份m3-17-native61-panel62-model-20261010回滚60/61；AE2023实际gate保持开放，Face禁用、Use Model(s)待参考。后续候选baseCEP62可重现；SDK全Model构建禁止直接partial dist并自动包含Model。13:57:30 PID30420已确认不存在，无Stop-Process。继续Particle/命名排序及Motion/Turbulence的完整目标。

2026-10-10 owner额度恢复并要求测试版：准备native61/packed32829/CoreABI8/CEP62完整八native/Core+十八CEP，gateway62及cache/manifest升级；隔离Model/Particle/Texture/Cloud/Birth回归通过。默认report的Model配对发布工具、IncludeModel/记录推导rollback/部分未复制helper恢复及热Core的Model匹配核对接入；实际工具dummy模拟60检查通过。准备冻结完整SDK后fresh AE check再按授权发布，保留配对60/61回滚与实际AE gate；不扩充Shape或Use Model(s)。

2026-10-10本阶段已提交推送dae9621；其后冻结命令因automatic approval review用量上限未执行（提示13:30）。完整SDK构建也没有启动，不存在新session或冻结产物证据；不能沿用旧构建证明新模块。owner已获告知并有继续问题待回复；未绕过审核、部署或停止PID30420。完整目标active，恢复后先冻结/双NoPublish八目标构建及配对记录，再继续Model作者/实际AE gate。

2026-10-10预设文件模态候选接入resident Host排队/claim/原生chooser/关闭后ID-stage-expiry重验和UTF16 hex路径。CEP仅短evalScript、显式有界poll，terminal receipt retained，cancel/expiry不写文件、丢失回复不重放。Host标准/ASAN各1596、chooser各242、实际隔离文件583及Shape/Model/Texture/Cloud/Birth回归通过；Prepare重现native-preset-modal-panel62-v2。完整冻结SDK构建继续，未部署或增加AE资格；初次测试PID30420停止授权等待owner，其他实现继续。Use Model(s)独立参考/全Model配对/实际AE gate开放，完整目标active。

2026-10-10 e5bf287 Shape入口已推送/冻结，完整May2023 /MT八目标双NoPublish实际exit0，11:21十八安装文件/selector/Core匹配60/61，证据m3-17-model-shape-{build,installed}-hashes.json及build-exit.txt。下一实现项为CEP预设文件窗口的resident Host排队/原生chooser/关闭后plain ID重验与UTF16路径传递（ADR0038 proposed）；保持原文件发布/rollback语义，补cancel/expiry/SDK时序fixture再冻结构建。Use Model(s)完整菜单单独请求owner。未部署，完整目标active。

2026-10-10 Shape入口实现候选：原生disk213六项/Model6-Core4映射与数值回放接通；ECP/隔离CEP Face禁用，native5拒绝，旧四值保持。实际注册/事件标准及ASAN112、实际图编译/Model标准及ASAN9245（另控件452）、CEP选择/gateway/默认cube Add/Replace92及共享作者/资源/Texture/Cloud/Birth回归通过。Prepare重现m3-17-model-shapes-panel62-v3，不改live CEP；完整SDK冻结继续。Face、Use Model(s)参考、真实ECP drag/动画/撤销重开及全Model配对仍开放，完整目标active。

2026-10-10 owner限定完整Shape仅Circle、Rectangle、Cloud、Texture、Face、Model，顺序固定；禁止推断其他模式。参考与ADR0038/schema已更新；Path/Shadow是Properties。native Face5/Model6为未发布作者计划，内部Model4保持，禁止日后重释保存值。Use Model(s)仍待参考。d1fa6e4冻结May2023 /MT八目标及NoRuntimePublish末尾标记已记录，原会话退出码无法恢复，报告null；10:46十八安装哈希/selector/Core匹配60/61。没有部署候选或关闭AE gate；下一步完整Particle Model作者选择映射及配对验收。

2026-10-10继续：2cbc74a已推送/冻结完整八目标双NoPublish SDK build通过，十八安装哈希及selector保持60/61。后续源码接入resident Host独占UI token与已加载effect client，覆盖已知原生脚本/模态范围和两个idle通道；独立DLL/Windows消息泵标准/ASAN470、实际导出148867及事务8886869/8887089回归通过。OBJ数字/完整网格自有快照在chooser后及提交前重新验证，导入/快照各20284通过，真实AE时序/撤销仍开放，SDK冻结证据见上方记录。Particle Shape六种顺序已由owner确认，Use Model(s)仍待参考。完整目标active，继续Model完整作者/配对验收，未部署。

2026-10-10 owner恢复任务：候选ModelTransactionHost接入独立菜单排队/有界idle拉取/整笔执行器；全部metadata及64MiB预检先于payload页，UI目标在两个回调前重新定位。标准/ASAN传输检查通过（计数8885968/8886148含逐字节和调用，executor为fake）。隔离CEP上传/queue/result和真实prepare/commit回调、preset Add/Replace资源UUID重写/停放mesh及复制显式导出已接入；protocol/planner271、真实gateway DOM35、作者74/预设332/export588及旧Texture/Cloud/Birth/完整gateway回归通过。tracked候选helper与patch由Prepare重现到m3-17-model-transactions-panel62-v2；live CEP不变，完整新SDK冻结待记录。当前目标active；接下来Particle Model菜单及Modal/idle/OBJ导入状态一致性部署前验收、完整配对发布，实际AE gate开放。

2026-10-10最新收尾：按owner要求暂停，保留完整目标及所有在途改动。源码10a1b6e已推送并冻结完整May2023 x64 Release /MT八目标构建通过（IncludeModelCandidate、双NoPublish），实际Host编译完整备份/事务执行器；标准/ASAN各636209、共用备份458562证据保留。日志及八输出/十八安装哈希artifacts/m3-17-model-graph-transaction-*，08:50 +08:00安装0不匹配，native60/CEP61、selector不变。执行器尚无真实Host/CEP调用入口，未部署或关闭AE gate。下次接续有界Host上传/排队、真实CEP prepare/commit及Add/Replace/duplicate资源UUID映射，然后Particle Model菜单、配对验证、Modal/idle与导入revision一致性部署前验收；之后继续Particle、节点命名/排序、Motion三模式和Turbulence。详见current-state.md最新交接。

隔离preset version3保存/导入：实际Save Current采集有界SFMG1，codec核对owner UUID/revision/bounds及parked mesh，显式32768字符分页文件传输与rename失败恢复；330项实际候选codec/client/gateway/UI检查和旧preset断言通过，Model/Texture/Cloud/Birth/完整gateway回归通过。新的文件IO使用内存fixture；原生整笔Model Add/Replace/duplicate待接入，当前明确拒绝mesh apply，未部署。SFMW源码969dcf1冻结完整May2023 /MT八目标通过，十八安装哈希0不匹配，安装60/61保持；不是Model发布或AE资格。

SFMW精确资产写入seam已接入Model generic入口，UUID/Source/revision/guard前置核对，十项Mesh/bounds/revision/Source/guard读回与失败恢复；标准/ASAN原生模块各7535通过，另控件452。调用者负责outer undo和整笔graph/mirror事务，尚未接入完整Host导入/预设应用，未暴露菜单。3c10b5d UI-idle导出桥完整冻结May2023 /MT八目标通过；十八安装哈希0不匹配，60/61保持。下一步有界预设保存/导入与整体Add/Replace/duplicate恢复，再完成Particle菜单。

UI-idle导出传输候选接入Host会话模块和隔离gateway：命令仅排队、固定script entry points、有界32KiB页，普通panel polling不导出mesh。标准/ASAN Host147289、gateway/client联动576、SFMG1资产/client373通过；作者74和旧Texture/Cloud/Birth/完整事务回归通过。Prepare工具与tracked候选helper/patch重现，无live修改；精确资产导入、完整预设IO/应用rollback、Model菜单继续。此前9a441a8 seam的完整冻结SDK八目标通过，十八安装哈希0不匹配；新Host桥独立冻结SDK证据见上，实际AE gate仍开放。安装60/61不变，goal保持active。

原生preset网格export seam实现：private SFMX/version1走Model generic selector，固定数字+借用sink/cancel回调传有界SFMG1，无项目写入。身份/修订/Source/守卫/边界一致性、render-only拒绝、取消/host错误/损坏及parked mesh导出由实际Model模块夹具覆盖，标准/ASAN各3483通过。异步Host UI idle传输与精确资产导入/整体preset回滚继续；完整SDK候选待冻结，不增加宿主资格或暴露Model菜单。

源码857818b冻结完整May2023 /MT八目标通过（IncludeModelCandidate/双NoPublish），Model101/完整导入路线编译；十八安装文件哈希0不匹配，native60/CEP61与runtime不变。日志artifacts/m3-17-model-author-build.log和m3-17-model-author-{build,installed}-hashes.json。阶段实现、focused tests、推送和冻结构建属于progress；网格预设/复制/恢复与Model菜单继续，实际AE gate未关闭。

作者边界追加候选：streams95..100/disks1519..1524，总101，metadata19..94和binding7 synthetic bounds19..24保持。导入十项保存/读回/逆序恢复；标准/ASAN原生模块各3040、导入事务各1559通过。隔离CEP作者74项、旧Texture80/Cloud49/Birth51及完整gateway事务通过；Model拖放/模型端口/普通参数记录/parked OBJ切换已接入候选patch。工具Prepare-ModelPanelCandidate只在artifacts复制和应用候选，不改live CEP。完整SDK冻结检查待记录；资源预设传输/复制/恢复及Particle菜单继续，未部署、实际AE gate开放。

当前活动实现卡切换为M3-17；M3-16已部署并等待owner宿主验收，M3-13添加/背面报错反馈优先。完整目标不缩减。

2026-10-10恢复进展：ModelGraphTransaction完整原生执行器在一个SDK undo组内组合备份、prepare、私有SFMW和commit；全部数值资产预检先于项目变更，失败/取消/回调异常整笔恢复，发布后的cleanup/undo独立诊断。标准/ASAN各636209、共用Backup458562检查通过，m3-17-model-graph-transaction-*.log；callbacks/generic是fake，不关闭Host/CEP/AE gate。接入Host工程/fingerprint，完整冻结SDK证据见本卡最新收尾；下一步Host资产上传/排队与真实CEP prepare/commit、Model菜单及配对验证，安装60/61保持。

2026-10-10 owner要求阶段收尾并暂停：完整EffectGraphBackup候选和guard2库存隔离已实现，标准/ASAN helper各458562项、Host147760、实际Model7699（控件452）、隔离gateway588/作者74/预设330与旧preset断言通过。保留失败诊断，日志artifacts/m3-17-effect-graph-backup-*；Prepare可重现隔离候选，不改live CEP。尚未将helper接入整笔Host资产/图事务，本阶段完整SDK冻结和实际AE验证待续；native60/CEP61保持。下次继续Host资产导入/完整Add/Replace/duplicate、Particle Model菜单、配对构建及部署前Modal/idle与导入revision一致性gate（ADR0038，owner静态假设未复现），然后继续Particle、各节点命名/排序和Motion/Turbulence；不缩减目标。
拥有：ModelGeometry/OBJ数值输入、三角形场景与CPU渲染、Settings/Render/graph/history/snapshot/C ABI的明确迁移、原生Model资源作者/Particle类型、CEP/预设、schema/version/build、focused mesh tests、ADR0038。不能重用既有shape0..3、diskID或matchName；外部资源读入在AE/UI适配器，Core只有数值。
MotionBlur.hpp的Model style验证/组索引迁移/线性矩阵采样属于M3-17渲染契约，保留旧形状的快门行为；语义先行记录于ADR0038。
首个里程碑：单位立方体、有界OBJ多边形/索引/属性、正确三角化、typed拒绝及取消。随后接入深度/裁剪/合成和完整作者链路，未实现渲染前不暴露Model菜单、不部署纯解析器候选。Face依赖OBJ发射器、Path依赖路径发射器，分别保留后续卡；Model图来源和Use Model(s)菜单待owner参考。
2026-10-09几何里程碑：Settings数值类型、ModelGeometry操作及有界OBJ、单位cube已实现；独立MSVC /MT fixture4141检查、0失败，日志 artifacts/m3-17-model-geometry-tests.log。继续投影/近裁剪/深度/合成与资源、作者接入；现有安装60/61不变。
2026-10-09三角形里程碑：ModelScene显式矩阵投影、近面/正W/ROI裁剪、透视深度/UV、每模型四采样遮挡与一次覆盖合成，四种transfer/HDR通过6406项MSVC /MT检查。ROI/反射/shear/downsample、horizon/预算/取消/分配失败均覆盖；合成只扫描触及像素。CMake与Core vcxproj接入数值源；粒子pose/资源、wire迁移、作者仍未实现，不暴露菜单或部署此独立里程碑。
源码5d17802已推送，并在 artifacts/prepared/m3-17-triangle-5d17802/source 冻结后完整May2023 /MT构建通过（双NoPublish）；日志 artifacts/m3-17-triangle-native-build.log。安装18项保持native60/CEP61收据一致。下一步：明确Model资源/pose/wire迁移后接入粒子、CPU与作者；完整goal仍active，菜单参考与实际AE gate开放。
完成条件：默认cube和Model资源能作为粒子显示，3D旋转/缩放/反射/近裁剪/深度/叠加/相机/快照/预算/shutter有效；原生/CEP/预设、配对构建/发布和真实AE2023持久化验收。
2026-10-09资源里程碑：SFMG1有界网格codec和粒子pose通过2076项；RenderRequest数值资源/C ABI8尾部传输通过30项标准及ASAN检查，保留精确ABI7前缀。三角形6406复查通过。追加源列表/CMake/Core工程；未接入图/shape4/snapshot8/原生/CEP，未发布或关闭Model gate。迁移方案先行写入ADR0038；现有安装ABI7不变。
资源源码8220f25已推送并冻结完整May2023 /MT构建通过（双NoPublish），十八安装哈希保持native60/CEP61。
2026-10-09粒子数值里程碑：shared Model groups/shape4/snapshot8/CPU真实像素通过2695项标准及ASAN；mesh lease一次验证复用，同组一次opacity、帧输入tri/sample预算和typed GPU CPU-fallback。旧snapshot3..7保持；Texture4322、资源2076、三角形6406及ABI前缀ASAN30复查通过。live图/采样与原生/CEP作者仍待接入，不暴露菜单，不部署此数值里程碑；所有实际Model AE gate继续。
snapshot/CPU源码e12517d冻结完整May2023 /MT全目标构建通过（双NoPublish），main AEX网格/资源依赖与adapter fingerprint同步；十八安装哈希保持native60/CEP61，日志 artifacts/m3-17-model-particles-native-build.log。继续live Model图/采样与完整作者，goal保持active。
2026-10-09 live数值里程碑：Model schema1/metadata端口、多源共享组、完整依赖与粒子流分离、稳定Particle UUID分区、Auxiliary组/资源域、当前帧采样及Linear shutter组迁移/矩阵插值接入。标准/ASAN各444项检查，旧Texture4322/Birth3348/Model CPU2695回归通过；原生/CEP/预设作者仍待完成，现有安装60/61不变。
源码67fc063冻结完整May2023 /MT全目标构建通过（双NoPublish），十八安装哈希匹配native60/CEP61；日志artifacts/m3-17-model-graph-native-build.log。下一步原生网格持久化/资源捕获和作者。
ModelGeometryParameter持久化helper标准/ASAN各4915项fake-host检查通过（artifacts/m3-17-model-parameter-{tests,asan}-current.log）；main AEX工程/fingerprint接入。SFMG1只保存数值mesh、UUID/revision归作者，全部ARB selectors在注册前实现；未注册新Model参数/菜单，作者与资源捕获继续。
源码a1751b9冻结完整May2023 /MT全目标构建通过（双NoPublish），十八安装哈希匹配native60/CEP61；日志artifacts/m3-17-model-parameter-native-build.log。下一步原生Model/资源捕获、Particle Shape及隔离CEP完整作者；Particle优先，Motion首批三模式在其后。

M3-17可编辑作者候选：可选schema1 keys4..13保存origin/rotation/percent scale/Flip/Center/Normalize/bounds/source，兼容旧matrix元数据且拒绝冲突；标准/ASAN各559图检查，资源2076/任意参数4915回归通过。ModelControls用真实May2023参数定义注册18项，独立Model disk1501..1518先行写入ADR并做共享唯一性检查；捕获只返回拥有的数值mesh/pose，OBJ准备新句柄不修改原控件。标准/ASAN各452项通过。源码c65d5f7已推送，冻结完整May2023 /MT七目标通过（双NoPublish）；十八安装哈希匹配native60/CEP61。Model kind5/AEX/binding/资源镜像、按钮提交撤销及隔离CEP作者尚未启用，不发布半成品菜单；下一步完整原生Model节点/资源提交与捕获。

M3-17原生模块/binding7候选：独立kind5 Model AEX注册18项作者控件/metadata，总95；UI复制/解码mesh并记录bounds，Model -> Particle input3，渲染回放只读14路pose别名及Source/revision/bounds常量。binding7仅含Model时写入，旧v1..6保持；标准/ASAN各2809项检查通过，日志artifacts/m3-17-model-native-binding-{tests,asan}.log。IncludeModelCandidate强制双NoPublish/full build，默认部署列表保持。Import提交撤销、主mesh镜像/SmartFX与完整CEP仍待接入，不发布无效菜单。

源码7f5c7d8冻结完整May2023 /MT八目标（含Model AEX/PiPL、显式IncludeModelCandidate/双NoPublish）通过；初次SDK路径与ModelResources缺ParticleTransform链接修正后重冻。旧绑定8441/相机12回归通过，十八安装哈希仍匹配native60/CEP61。日志artifacts/m3-17-model-native-build.log与m3-17-model-legacy-binding-regression.log。继续资源镜像/导入事务/SmartFX及完整CEP；未部署候选。

Model renderer mirror helper实现SFMR1 UUID/revision/CRC+SFMG1，以及全部ARB callbacks/独立所有权/empty slot和256槽注册helper；标准/ASAN各8021项通过（含复用452控件项），日志artifacts/m3-17-model-mirror-{tests,asan}.log。main新镜像IDs及manifest30追加方案由ADR0038先行规定，当前manifest29/755注册保持；helper编译入口已纳入main/node工程。下一步主资源安装/回滚与SmartFX数值捕获、OBJ按钮事务及完整CEP，不部署helper独立候选。

镜像持久化源码0d868eb冻结完整May2023 /MT八目标通过（显式IncludeModelCandidate/双NoPublish），ModelMirrorParameter实际进入main/node候选；十八安装哈希保持native60/CEP61。日志artifacts/m3-17-model-mirror-build.log；继续主资源事务/回滚与SmartFX捕获，未调用注册helper、未部署。

Model资源桥候选已接入manifest30/main1012与SFMR1 selector入口、NativeBindingTransaction资源提交/验证/回滚、SmartFX活动资源及快门去重捕获。标准/ASAN夹具各922项、原生绑定2809、旧绑定8697/相机12通过；日志artifacts/m3-17-model-resource-bridge-{tests,asan}.log和m3-17-model-resource-{native-binding-tests,legacy-regression}.log。OBJ导入事务、Particle类型/隔离CEP完整作者仍待接入，当前安装60/61保持。下一步冻结完整SDK构建后继续作者，不发布资源桥独立候选。

资源桥源码eae10ae冻结完整May2023 /MT八目标通过（显式IncludeModelCandidate/双NoPublish），日志artifacts/m3-17-model-resource-bridge-build.log；十八安装哈希保持native60/CEP61。继续OBJ按钮事务/完整CEP，不部署此桥候选。

原生OBJ按钮/导入事务候选接入：先有界读入/验证，再在guard94下提交Mesh3、Revision4递增、Source1=OBJ2，读回并通过原生图事务发布，失败精确恢复/诊断恢复失败。标准/ASAN各1019检查、原生模块/binding2809通过；日志artifacts/m3-17-model-import-transaction-{tests,asan}.log、m3-17-model-import-native-binding-tests.log。对话框/真实undo/reopen gate开放，完整SDK构建待冻结；下一步Particle Model菜单/隔离CEP及预设，当前安装60/61不变。

导入源码da76183冻结完整May2023 /MT八目标通过（显式IncludeModelCandidate/双NoPublish），实际Windows对话框/按钮路由编译；日志artifacts/m3-17-model-import-build.log。十八安装哈希保持native60/CEP61，未部署Model；继续Particle菜单/隔离CEP和预设完整作者，Motion首批在粒子阶段之后。

## M3-18 — Motion 首批（计算层开发中）

2026-10-10 有序frame源码8bd5a5c已推送/git archive冻结；IncludeModelCandidate/NoDistPublish/NoRuntimePublish完整May2023 x64 Release /MT八目标会话81513实际terminal exit0，motion-frame-sdk-build.log/exit.txt/build-hashes.json及sdk-terminal.json保存八新输出。冻结Circle1842/Look At1377/Path8432再检actual exit0；新26安装hash/selector/runtime核对匹配61/63。没有Motion作者部署，下一步有序Force/外部资源依赖/动画几何导数/参考政策及三模式作者，完整目标active。

2026-10-10 有序Motion/Transform数值候选：逐粒子Q*A*B*Euler及snapshot10迁移先记录ADR0039，普通/历史/Auxiliary按图顺序处理朝向、位移和后置Transform，支持反序Motion与交错/串联Transform。标准/ASAN4141、Circle1842/Look At1377/Path8432、Transform594/Model graph559/Model particle2695实际exit0，motion-frame-*日志保留；前置/无Motion旧分支保持，MNT改动未纳入。下一步本阶段提交冻结完整SDK，然后有序Force、外部资源/动画几何导数、参考政策与三模式作者，安装61/63保持，完整目标active。

2026-10-10 mode0源码cf64261已推送/git archive冻结；双NoPublish完整May2023 x64 Release /MT八目标会话18162实际terminal exit0，新Main/Core路径CPP及八hash/terminal证据记录于motion-path-graph-sdk-*。构建后26安装hash/selector/runtime匹配61/63，motion-path-graph-installed-hashes.json。下阶段外部Light依赖/GUID、完整有序frame/动画几何导数与参考单位/延迟/目标政策，随后三模式作者/配对发布/AE2023，完整目标active。

2026-10-10 mode0普通/历史Core graph/Auxiliary候选完成SFMP1数字点、delay捕获于birth、动画speed/curve随机积分及最终基底切线pose；32MiB累计capacity及20M工作量请求预算跨Auxiliary，当前几何/历史clock lease及4096淘汰验证。标准/ASAN8288、Circle1772/Look At1372/travel1216/Transform594/Model559/Model粒子2695均实际exit0，motion-path-graph-*及path-graph-*-regression日志保留。新共享源码提交/冻结完整SDK继续；外部依赖/GUID、完整有序frame/动画几何导数与公开参考政策及三模式作者仍需完成，不发布数学入口或关闭完整目标。

Light Path接入GraphEvaluation后，既有直接编译该CPP的测试运行器需要追加MotionGeometry/MotionPathTravel链接输入。M3-18仅拥有这些依赖行，保留MNT-01的其余夹具/运行器改动；RunCoreTests的两行追加独立入索引，不提交其他在途diff。当前没有另一live agent的文件分配。

2026-10-10 travel源码874f852已推送/git archive冻结；双NoPublish完整May2023 /MT八目标会话69978实际exit0，新CPP编译及八输出hash记录于m3-18-motion-path-travel-sdk-build.log/exit.txt/build-hashes.json；26安装hash/selector/runtime匹配61/63，travel-installed-hashes.json。下一步实际mode0 graph/history/Auxiliary、全请求资源/工作预算及外部来源/有序frame，随后三模式作者/配对发布/AE2023；不能由独立travel helper或SDK通过关闭本卡，完整目标active。

2026-10-10 Light Path travel子阶段：Circle共用固定曲线clock提取/稳定延迟区间积分；独立有界路径travel贡献相对位移和瞬时速度，明确speed/delay/random数字，切线pose保留既有分布/Euler/affine剪切镜像；无分配乱序查询、capacity存储计量及失败原子性。标准/ASAN1216、Circle1766/Look At1366均exit0，m3-18-motion-path-travel-*与path-clock-*-regression.log；Core工程/CMake/fingerprint纳入新输入，冻结完整SDK继续。参考末端/Delay Random=0对照已请求，暂不推测公开转换或分布；实际graph mode0/history/Auxiliary、全局预算/资源依赖/有序frame和三模式作者仍为本卡要求，mode0仍typed reject，不部署计算层或缩减目标。

2026-10-10 Look At源码da17da8已推送/git archive冻结；双NoPublish完整May2023 /MT八目标实际terminal exit0，审批服务首次额度失败后的日志/exit/八新产物hash已恢复核对，m3-18-motion-look-at-sdk-{build.log,build-exit.txt,build-hashes.json,terminal.json}保留。新26安装hash/selector/runtime匹配native61/CEP63，m3-18-motion-look-at-installed-hashes.json；没有重启构建或沿用旧Circle产物。继续Light Path/完整有序frame/资源/参考政策/三模式作者；未部署Motion，完整目标active。

2026-10-10 Look At/姿态子阶段：mode2及goal7/forward8接入实际普通/历史graph，当前时间goal/Over Life最短弧；逐粒子四元数保持Euler/shared affine剪切镜像，CPU/GPU场景准备/Model共同消费，Linear shutter SLERP。ADR0039先定义snapshot9/232B迁移，单位pose保留旧3..8/C ABI8/sequence1/AE IDs，必须新源码完整SDK配对。标准/ASAN1366、Circle1766、Transform594/Model graph559/Model snapshot2695通过，m3-18-motion-look-at-*及look-at-*-regression.log；相机Circle/Cloud保留完整姿态轴，冻结SDK继续。安装61/63不变，没有作者入口。串联Look At/Circle→Look At可用，反序/后置Force/Transform与不同链合并明确拒绝；完整有序frame、Light Path及公开目标/单位政策仍是本卡完成要求，不缩减三模式目标。

2026-10-10 Circle graph子阶段：append-only Motion schema1/本地key1..6、显式rad/s、曲线精确antiderivative、历史采样/metadata proof和有界不可变lease、随机purpose22。实际普通/历史graph、Force/Transform前置、串联Circle、Auxiliary出生/瞬时速度、codec/snapshot和CPU像素接入；后置Force/Transform及不同链合并明确拒绝。标准/ASAN1764、Transform594/Model559回归通过，m3-18-motion-circle-graph-*与circle-*-regression.log。源码05c37e2已推送/冻结，双NoPublish完整May2023 /MT八目标实际exit0，m3-18-motion-circle-sdk-build.log/exit.txt/hash报告及26安装hash/selector复查保留，安装61/63保持。尚无作者/AE disk ID/ABI变化或部署；Light Path/Look At、完整frame/动画轴原点导数与参考公开单位仍为本卡要求，不以Circle子阶段缩减首批三模式或完整目标。

2026-10-10 Point capture阶段：新增AE只读适配器，按明确layer ID/局部点/PF时间验证comp归属/身份，完整矩阵转到效果层canonical frame；同源矩阵一次查询，成功且suite释放后整笔替换数字输出，错误/取消保留原输出。真实May2023 helper、fake SDK标准及ASAN各2365通过，m3-18-motion-point-capture-{tests,asan}.log；main工程及adapter输入指纹更新。本层不决定Starting With/anchor/Light Path构造/Look At目标，未改图或ABI、未暴露节点。新源4c9c50c已推送/冻结，双NoPublish完整May2023 /MT八目标实际exit0，新helper实际编译；m3-18-motion-points-sdk-build.log/输出hash及26安装hash/selector核对保留，native61/CEP63保持。下一阶段值/graph/数值资源快照、历史/shutter、依赖GUID/AE缓存与ABI迁移先行，参考未确认项保留，不新增Light flag或非首批选项。

2026-10-10 独立MotionGeometry实现有界圆周旋转、不可变B-spline距离/切线查询和加权最短弧朝向；ADR0039区分数学定义与尚未确认的参考策略。单独MSVC /MT标准及ASAN各4759检查通过，日志artifacts/m3-18-motion-geometry-{tests,asan}.log，含极小向量、大坐标下小路径、解析弧长、重复/转折/端点、全部路径分配及复制赋值失败、取消、无分配乱序查询及四线程读取。初次夹具nan命名冲突和ASAN运行器环境/链接选项诊断已修正，不修改用户环境。Core工程及CMake纳入源码；尚无Motion graph/作者入口、资源或ABI迁移，不部署无入口计算层。源码3d6d718已推送/冻结，双NoPublish的完整May2023 /MT八目标实际exit0，m3-18-motion-geometry-sdk-build.log及八输出hash记录；构建后26安装hash/selector/Core仍匹配native61/CEP63。实际AE/Light Path路径构造、Circle单位/半径、Look At目标筛选仍开放。

2026-10-09 owner截图限定Light Path、Circle、Look At，复杂Motion按阶段执行；参考清单docs/reference-motion-phases.md。拥有：独立Motion数值类型/采样与粒子路径/朝向、graph/history/资源/渲染的明确契约、独立原生节点及CEP/预设/schema/build/focused tests/新ADR。只追加自有身份/IDs；不复用Transform或Force身份，不注册无实际行为的选项。动画/Light资源采样边界和Circle半径/Look At目标语义先记录证据与独立方程，再接入作者。完成条件：三模式实际行为、共享曲线编辑/持久化、配对构建/发布及AE2023验收。

## M3-19 — Turbulence（待Motion首批后执行）

owner目标包含独立湍流节点；参考dump的标签/默认值可用，但未列完整枚举。拥有：独立噪声数值、影响属性及坐标/时间/随机性契约、graph/history、原生/CEP/预设/schema/build/focused tests/新ADR。不把普通Force的Gravity/Wind模块冒充Turbulence，不复制旧实现。完成条件：确定性有界噪声/实际属性变化、动画/曲线/多路径、完整作者/持久化与AE2023验收。
