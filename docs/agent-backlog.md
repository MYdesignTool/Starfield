# 当前任务卡

旧任务卡和精确历史所有权见 [归档](history/2026-10-07/agent-backlog.md)；产品进度见 [路线图](roadmap.md)。

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

当前活动实现卡切换为M3-17；M3-16已部署并等待owner宿主验收，M3-13添加/背面报错反馈优先。完整目标不缩减。
拥有：ModelGeometry/OBJ数值输入、三角形场景与CPU渲染、Settings/Render/graph/history/snapshot/C ABI的明确迁移、原生Model资源作者/Particle类型、CEP/预设、schema/version/build、focused mesh tests、ADR0038。不能重用既有shape0..3、diskID或matchName；外部资源读入在AE/UI适配器，Core只有数值。
首个里程碑：单位立方体、有界OBJ多边形/索引/属性、正确三角化、typed拒绝及取消。随后接入深度/裁剪/合成和完整作者链路，未实现渲染前不暴露Model菜单、不部署纯解析器候选。Face依赖OBJ发射器、Path依赖路径发射器，分别保留后续卡；Model图来源和Use Model(s)菜单待owner参考。
2026-10-09几何里程碑：Settings数值类型、ModelGeometry操作及有界OBJ、单位cube已实现；独立MSVC /MT fixture4141检查、0失败，日志 artifacts/m3-17-model-geometry-tests.log。继续投影/近裁剪/深度/合成与资源、作者接入；现有安装60/61不变。
完成条件：默认cube和Model资源能作为粒子显示，3D旋转/缩放/反射/近裁剪/深度/叠加/相机/快照/预算/shutter有效；原生/CEP/预设、配对构建/发布和真实AE2023持久化验收。
