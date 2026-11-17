# 当前任务卡

## INT-01 — 验收基线同步main与下一轮开发（Owner 2026-11-17）

Owner已确认STB-21当前问题解决，授权同步本地及远端main，并从相同基线创建`codex/next-development`。本卡拥有验收/集成文档、累计插件源码的提交边界与Git分支引用；不改业务实现、版本、SDK、部署或宿主状态。保留全部Transform/Particle等既有改动，核对136冻结输入并运行集成回归；WEB-01及Pages工作流继续独立保留、不上线。备份与同步证据在`artifacts/main-sync-20261117/`，完成情况以实际Git引用为准。

## STB-21 — Particle Over Life 生效与拖动响应（Owner 2026-11-17）

状态：已完成，Owner于2026-11-17确认当前问题解决。结合此前反馈，覆盖三条Over Life生效、首次加点连续拖动和曲线松手闪回。以下为实施及逐版验证历史；Undo/save/reopen全矩阵、具体AE Build和完整MFR资格未由本次确认关闭。

最新136/124已freshAE0部署并获Owner确认解决原问题；成功提交取消事件返回后强制立即重绘，局部失效通知放入原显示保护，拖动/失败恢复立即画。冻结135最终顺序夹具4,135/9失败，136普通/ASan各4,078，实际UI/native链路各4,529、Particle642、实际135/136 DLL2,286、AEX333与SDK通过；前后2,189哈希及135回退核验。模型不代替AE内部顺序测量，现场结论来自Owner本次确认。

Owner曾确认135编辑体验正确但曲线框松手闪回仍在；136保留手势/拥有值提交/调用内预览，修复成功提交仍携带PF_EO_UPDATE_NOW导致事件返回后抢先重绘的时序。补宿主先画旧参数、随后采纳PF改值的队列回归，不用跨调用旧值覆盖层遮挡Undo。136原问题现已获Owner确认，135调用内通过本身仍不等于该现场结论。

135阶段历史状态：135/124已freshAE0部署，134回退及前后2,189哈希核验；旧134最终编辑夹具3,900/22失败，135普通/ASan各3,955通过，真实UI/native/预设链路各4,529、Particle注册642、实际134/135 DLL2,286与AEX333/SDK通过。源码仅UI/版本/诊断构建配置变化，134提交边界及Core/ABI保持。135真实连续加点、松手回显与Undo/save/reopen仍待Owner；既有静态告警不记整体通过，详情见current-state。

Owner已确认134的Over Life生效；继续修复新点需二次点击、松手闪回旧曲线及等待刷新。本阶段只拥有ParticleGradientUI.cpp、相应真实UI回归、版本/SDK配对与当前文档；保留134拥有值提交边界、所有Core/原生事务/其他在途改动。新点接入同一拖动，发布期间使用有界、仅该调用存活的显示预览，取消/失败/Undo和上下文关闭保持严格；不把19–20ms原生服务日志当作端到端渲染耗时。

134阶段历史状态：134/124已freshAE0完整部署，133/124回退与前后2,189次哈希0不符。20个真实catalog预设×三曲线×三delivered数量离线矩阵，旧133复现243失败；新入口普通/ASan各4,529、共享编辑器各3,708、Particle注册642、原生绑定ASan48,165/Camera12、实际133/134 DLL2,286与AEX333/完整SDK通过。独立拥有值参数组API不改num_params，保留宿主身份/guard与普通回调数组门；未开Win32选择器/操作AE，现场唯一根因及134实际效果仍待Owner。补充拥有NodeEffects.hpp私有声明与必要的测试同步替身、实际preset catalog输入核验。下文133状态保留为历史。

Owner随后确认133仍无效，Over Life预设Apply报SF-T202/AE516；本卡继续开放。扩展明确的自定义参数组提交入口与实际预设catalog测试，保留普通监督回调的数组数量门，使用固定拥有值请求并由宿主重新核对source kind/UUID/guard，禁止伪造num_params或跳过事务保护。133仅捕获未发布，不能记为原故障修复完成。

状态：133/124已freshAE0完整部署；冻结v1与前后2,181次哈希0不符、132/124回退核验。普通/ASan编辑器各3,731、真实UI/native/采样/Core各1,099、原生绑定ASan48,165/Camera12、实际132/133 Core并发2,286与AEX声明333通过，SDK构建完成。旧guard夹具6失败/逐移动发布反例保留，不能称Owner现场根因已重现；实际曲线/Undo/重开/延迟待Owner复测。严格分析的三项既有警告在前后均复现，未伪称通过。当前配对、证据及回退见current-state与engineering/particle-overlife-20261117.md。

Owner确认132的多帧渲染警告消失，报告Size/Opacity与Rotation Over Life无效、拖曲线卡顿。本卡拥有ParticleGradientUI.cpp、NodeGraphSync.cpp/.hpp中曲线提交边界、必要的NativeNodeGraph曲线转换、PluginVersion.h、相应实际编辑/绑定/渲染回归和当前文档。先验证三条曲线从PF事件到发布及渲染的完整链路；拖动使用拥有值的本地草稿，松手一次发布，保留取消/外部修改/失败回滚与身份隔离。保留MFR实验配置及全部既有改动，不改公开IDs、布局、schema、Core ABI或Render.hpp。不启动/退出AE；如需部署，沿用freshAE0常设授权与实验配对门。Owner反馈只证明警告消失，不扩大为完整MFR资格。

## MFR-01 — AE2023多帧渲染准入与线程边界（Owner 2026-11-16）

Owner最新安排：已确认前两项原问题无误，明确要求部署132供其亲自测试。18:20 freshAE0经既有Deploy-TestBuild完整部署132/124实验包，前后1,129次哈希0不符；129/124一步回退备份已核验。代理仍不启动/退出AE。沿用冻结132，不重建或修改实现/公共契约；当前拥有部署证据、对应文档和Owner结果交接，真实MFR资格待Owner反馈。此前代理启动宿主的提议不再执行。

第二阶段边界扩展：PluginFlags.h/NodeEffectFlags.h、EffectMain.cpp/NodeEffects.cpp、MfrQualification.hpp、Starfield.vcxproj/NodeEffect.vcxproj、BuildWindows.ps1/BuildPiPL.ps1，以及Deploy-TestBuild.ps1的显式构建标签选择和资格测试脚本。隔离NoPublish的132实验候选在Owner明确部署要求后已激活，默认构建仍关闭MFR；不具备通用AE支持声明，不擅自启动宿主试验。132以开始/结束记录验证真实回调重叠，不能用不同线程ID替代同时执行证据。公开IDs/ABI/schema不变。

第二阶段已实现并验证GPU请求隔离；131前置包与132启用MFR的隔离实验包均已冻结。离线证据：面板75套全过；GPU strict/ASan各24,043、真实GPU477、原生绑定ASan48,165/Camera12、历史缓存ASan131、实际Core DLL并发2,286、开/关配置实际AEX各333及生产日志194记录/97区间通过。当前已部署132/124，不重建；129/124回退保留。Owner按engineering/ae2023-owner-test-checklist-20261116.md从M01开始；A01/A02原问题已确认，Core热切换仍待专用包。完整资源族、精度和热切换资格未完成，本卡保持开放。

状态：离线测试与手工交接完成，实机资格待Owner。前序拥有NativeTemporalCache.cpp/.hpp、GpuRender.cpp、PluginVersion.h和对应测试，已完成请求隔离；不重写其他GPU/Transform功能。宿主资源快照实现若需改变须另行明确边界。不得仅设置标志消除警告；PF/AEGP线程契约和实际AE2023资格完成前不声明正式MFR支持。不改Core算法、公开ID、ABI/schema/Render.hpp，未启动/停止AE。最新证据见engineering/mfr-offline-handoff-20261116.md；历史130/131/132冻结和失败证据保留。

## STB-20 — 工程启动缺少全局JSON（Owner 2026-11-16）

状态：修正随129/124 v2部署，当前沿用panel124与native132实验配对。Owner最新确认原冷启动JSON问题已测无误；不再要求重测该故障，未提供具体AE Build或Model/预设分步结果。原冻结73套件通过、1旧CEP Junction环境失败为历史；后续完整离线75套已通过。新增无JSON真实脚本/UTF-8回归6,152项通过，v1代理码反例与原始失败保留。拥有gateway私有JSON编解码、model/preset JSX使用其编解码、panel/preset外层错误返回、面板版本配对与对应测试/ADR0046/当前文档。不写全局JSON、不依赖其他扩展初始化，不以重试/吞错替代修复；保留解析校验、真实错误、事务与原生/Core/参数/ABI/schema。代理未启动/停止或操作AE。

## UX-16 — 新增效果首次画布直接采用初始化结果（Owner 2026-11-16）

状态：129/123已freshAE0完整部署，原128/122画布修正与当前原生修复都保留配对备份；可一次回退到任务前128/121。54组实际编译前后粒子数据、36组实际Core CPU像素对照、原生普通/ASan及adapter/严格分析/SDK通过；面板72通过、1旧CEP环境失败未标通过。实际AE首帧连续性仍需Owner复测，详见current-state。拥有panel首次接纳/版本配对/测试/文档、Parameters.cpp与GraphParameter.cpp/.hpp启动投影、gateway首次节点身份、PluginVersion.h、schema/parameters.json默认值元数据及ADR0049。新效果从首帧使用与初始三节点一致的参数及图内身份；仅精确出厂ARB匹配才启用投影，已有作者图不被空receipt替换。保留旧工程缺失参数回填值、初始化事务、失败停止/手动重试、目标跟随及只读画布门。公开ID/图序列schema/Core ABI/Render.hpp不变，未启动或操作AE。原128/122仅为画布修复，不能单独称为合成预览问题闭环。

## PERF-17 — 画布数据轻读取与按需编辑快照（Owner 2026-11-11）

状态：128/121 v2已freshAE0经KeepNative部署，最终72套件零失败、731哈希核验与到128120的配对回退链通过；v1 busy准入反例已修、旧证据保留。8节点值读取3552→112仅为离线计数，实际AE画布延迟待Owner验收，详见current-state。只读标记的必要配套还拥有graph_edits.js、graph_layout.js。

拥有gateway.jsx/native_graph_snapshot.js/panel.js/graph_codec.js/graph_transactions.js/edit_queue.js及版本配对、相关测试/文档。画布仅读身份、类型、位置、连接和摘要；不读完整数值、曲线、资源/图层库存，不构造可提交的默认参数图。只读canvasOnly快照不能序列化或进入提交队列；结构意图合并等待一次完整新鲜快照后走原有事务，纯布局/单击AE参数保持直接可用。保留目标/scope/取消/布局冲突与删除保护，不改native/Core/公开ID/ABI/schema。不启动AE，freshAE0可KeepNative部署并保留完整回退，离线计数不宣称AE时间。

## PERF-16 — 图层选择发现与安静期加载（Owner 2026-11-09）

状态：128/120 freshAE0通过KeepNative部署，冻结v1；70套件零失败、版本合同和686哈希核对通过，完整128119回退保留。5万层选择解析/实际panel合并与慢读单在途计数通过，不等于真实AE交互资格。Owner仍需确认切层速度与阻断体感，见current-state。

拥有panel.js、gateway.jsx只读发现路径、面板版本配对与对应测试/文档。检查2秒空闲轮询、全作者快照/原生诊断的AE主线程成本；增加轻量选择探测，短暂选择合并，仅稳定目标完整读取，保持单个在途轮询，诊断延后到安静期。优先用AE selectedLayers数组，不全合成扫描找选择。普通读不改变AE选择/焦点，不缓存旧作者当可编辑新快照，不改native/Core/事务/公开ID/ABI/schema。不启动或操作AE，freshAE0可KeepNative交付完整回退；操作计数不当作真实AE耗时。

## UX-15 — 空画布与会话目标跟随（Owner 2026-11-09）

状态：128/119 freshAE0通过KeepNative部署，冻结v2；69套件零失败，版本合同与684哈希核对通过，128118完整回退保留。取消跟随后晚到的跨工程同ID响应反例已修，旧冻结/失败保留。代码与离线验证完成，实际AE交互仍为Owner门，见current-state。

拥有 panel.js、gateway.jsx 的只读面板目标解析、index.html/面板CSS/版本配对、相应测试/文档。无有效目标为普通空状态，提示固定在视口中心且不随缩放；没有有效选择时复查上次查看目标，仍存在于当前工程才保留。自动刷新关闭后不随新选择切换，手动Refresh允许切换；启动强制恢复勾选，不写工程或偏好。明确绑定画布目标的后续调用，防止旧画布编辑落入新选择；不改native/Core/事务提交、公开ID、ABI/schema。不启动/停止AE，freshAE0后可KeepNative部署并保留完整回退。

## UX-14 — 节点诊断归属、严重程度与悬停浮窗（Owner 2026-11-09）

状态：128/118已freshAE0通过KeepNative部署；冻结v1，67面板套件零失败，版本合同/安装/完整128117回退哈希验证完成。只改面板，Core/native保持；实际AE诊断与交互仍待Owner确认。证据与回退见current-state及artifacts/ux-14-20261109。

拥有 web_panel/js/graph_view.js、panel.js、css/panel.css 及面板版本配对文件、相应前端测试和当前文档。只改变诊断呈现：无效连线的原生 SF-G 错误仅标记来源节点，不沿祖先扩散；缺少模型且使用既有默认立方体的 Particle 显示 SF-W101 黄色警告；错误使用红色标识/描边；安全文本暗色悬停/键盘浮窗。诊断派生只读作者图及既有原生结果，不增加 AE 调用，不改变 Core、执行投影、提交/绑定/删除契约、公开 ID/ABI/schema。不启动/停止/操作 AE；AE 运行时仅准备候选。

## UX-13 — 自由连线、删除/晃动桥接、快速编辑与可定位诊断（Owner 2026-11-04）

状态：128/117已freshAE0部署，最终冻结v5；v3实现与v5交付独立审校PASS，66面板套件/普通与ASan/实际AEX离线检查通过。代码/静态准备完成，真实AE2023混合操作、时延、Undo/save/reopen仍为Owner验收门，不能由离线测试关闭。

Owner已确认127启动旧工程经显式修复解决；要求删除/摘出节点后上下游保持连接，拖动快速晃动两三次摘出，所有现有节点输出/输入可连，语义错误保留线并在节点旁黄三角/hover详情。新增静态与离线随机2–3Hz编辑压力回归，区分真实AE冒烟资格；用稳定插件诊断编号/明确文字补足通用AE516。拥有web_panel图编辑/诊断/手势/队列/视图CSS与native snapshot/transport、ae_plugin NativeGraphEdit/Host/projection/private headers/Parameters诊断与必要构建/版本、相应tests/ADR0048/架构/当前文档，以及Core GraphEvaluation.cpp/.hpp中只读、无模拟/无宿主的拓扑规划校验入口（复用生产规划器，不改普通求值数学）。Core ABI/Render.hpp/公开ID/schema不改；author connection bank仍既有目标UUID+edgeUUID、固定端口推导，不增隐藏参数。Core只接收合法可运行投影，编辑图由既有native manifest恢复，错误边保留在作者数据。

先独立设计审校（ADR0048），特别验证草稿/可执行投影、旧代删除交接、元数据增长补偿、CAS/未知回执、资源链和多选桥接；实现后由新的独立审校重放实际入口。禁止以PF错误号替换自定义任意AE返回值，516继续作为宿主状态附带，稳定SF-*码识别具体类别。每秒2–3次指令到达的离线随机/故障注入不等于真实AE吞吐或无崩溃保证。不启动/操作AE/工程，保留127/116配对和单击编辑；AE0后仅按常设部署授权交付完整配对及回退。

## PERF-12 — 原生编辑性能实施（Owner批准，保留126删除保护）

Owner已批准PERF11分析方向并要求避免回归。此卡拥有NativeNodeGraph退休绑定转换、新私有退休标记头、NativeGraphCommit/NodeGraphSync/NodeEffects中经设计审校确认的节点增量及可选能力探测、必要的EffectMain私有查询/生命周期、Host队列唤醒/阶段计量、web_panel删除反馈与版本配对、对应实际入口测试、ADR/当前文档及artifacts/perf-12-20261031。按阶段实施，不启动/操作AE，Core/公开ID/schema/Render.hpp/单击编辑保持。

首个可独立验证阶段不猜never-owned、不跳过历史零值槽：保留全体live与当前126退休集合的刷新，将force退休的value;→空双写改为两种交替的、禁用且无源依赖的本插件自有标记之一，保证一次不同文本转换；保留有限handoff/状态检查、失败回滚/guard。仅对原已禁用且精确数值相同的退休槽省掉重复数值/禁用写入。标记沿用126识别的native前缀，旧版本仍能识别并清理；不改图/序列存储schema。先独立设计审校，再实际旧代/零handoff/Undo记录/失败/partial/silent写/重新复用测试；禁止将调用量差当AE毫秒资格。

节点增量必须有可执行的作者新鲜度证明，不能只凭saved revision/CRC跳过未读作者；缓存只保存值/PF_State等可安全复制数据，不留AE句柄，未知能力/状态/拓扑/资源/动画/geometry/Undo变化须回到既有完整路径。每个落地阶段交新的独立实现审校；通过后可按freshAE0常设部署授权发布一致配对与一步回退，SDK构建不冒称AE验收。

## PERF-11 — 参数增量发布与删除长延迟分析

Owner在2026-10-31要求重点分析按变化节点/参数提交，反馈126/115删除无错误但体感5–10秒。本卡先拥有现有生产代码/现场journal的只读分析、artifacts/perf-11-20261031下冻结证据与实际SDK夹具调用计数、增量契约/分阶段定位说明；保持126/115产品与安装不变，不启动或操作AE。重点区分已有NativeEdit增量输入与全图编译/绑定/缓存失效，删除的UI队列/前置捕获/发布/物理删除/最终绑定/后读；SDK调用计数不等于AE毫秒。若提出产品改动，先明确命中前提、读取/写入集合、动画/Undo/资源/拓扑失效与失败契约，再由全新独立设计审校验证；不直接删除126修复依赖的旧代交接/新绑定检查，不改公开ID/schema/Core ABI/Render.hpp。保留所有既有Transform/粒子与部署改动。

## PERF-10 — 编辑确认与纯布局保存（Owner确认126/114删除已修复，性能优化）

Owner在2026-10-31确认当前删除问题已修复，要求保持此稳定基线并尽量减少不必要的提交/AE确认，明确指出纯节点画布位置不应反复通信。此卡拥有web_panel/js/panel.js、轻量布局队列新模块、gateway.jsx的坐标元数据写入/Graph模式状态与pulse、必要的webview_host.js历史通知、版本配对HTML/manifest及真实前端/gateway测试、相关ADR/文档。原生126 Clear/绑定/guard/快照证明、Core数学/ABI/Render.hpp/公开ID/schema均不修改，不启动或操作AE/工程。

纯拖动本地绘制，停手合并仅变化坐标、一次小写入保存到既有原生layout参数，不走完整图/Model/资源/别名提交，也不为保存重读全图。目标path/UUID、常量layout字段、旧坐标冲突/guard和失败补偿仍必要。layout写入后只把需要完整manifest的后续结构规划标记为需新快照；native删除仍沿已验证126入口。成功结构回执直接更新UI，不立即重复getPanelState；pulse将帧时间与结构stamp分开，Graph模式状态不读取退休legacy参数/曲线。拖动绘制按帧合并，手势结束先flush。先以真实入口记录现有调用量，完成回归和全新独立审校后可按freshAE0常设授权部署仅前端配对，原生十一文件逐字复用126并保留126114回退。用户确认只覆盖删除症状，不能扩大为Undo/所有宿主行为资格。

## STB-19 — 原生Clear与已删除源的生命周期（126/114已部署，现场门开放）

Owner要求系统分析并尝试解决125仍失败的所有节点删除。先从已冻结现场586记录和实际生产Clear/发布器/渲染入口建立可复现反例；不可继续仅把真实root焦点等同ECW或WebView。此卡拥有必要的NativeGraphEdit/WebPanelHost、原生图发布/绑定/EffectMain的私有删除生命周期边界、UiExclusion及对应真实入口测试、版本配对与相关ADR/当前文档。Core数学/ABI/Render.hpp/公开参数/ID/schema和单击编辑不改。任何新边界先写具体读写/撤销/回滚契约并交独立审校，修复后再由全新审校验证；不自动Undo、不清未知guard、不以零/任意旧值代替有效绑定，不启动或操作AE/工程。现场成功和性能不由离线fixture代替。

已实现观察式同步AE Clear：完整旧active数值捕获/前置guard，AE自己选择目标、inner bypass一次，plain ID/path/base/census确认实际selected UUID消失后prune/SFC2，旧代handoff与严格新绑定分离，失败准确留guard/阶段状态。补STREAM/关键帧/叶STREAMREF观察但不猜删参数；标准文本和DOM图路由保留。旧Force测试实际未接Output，已改正并保留旧125同类516反例20660/1预期失败。父native分析/ASan160255、组合Host4508、既有Host768、publisher27084+12、55panel114、完整SDK126通过；全新审校166输入一致、Host158及实际AEX准入19独立断言通过。freshAE0部署126114并1338hash/125113回退核验，Core不变。实际AE递归/Undo/重开、第三方与一般外部删除仍需资格，不自动修既有坏图。证据stb-19-20261030，回退见current-state。

## STB-18 — Owner恢复单击编辑（125/113已部署）

Owner确认125/112删除错误仍在，并撤回双击编辑偏好，要求恢复单击。此卡仅拥有panel.js节点点击/编辑、index提示、panel113配对元数据、实际palette回归及当前文档/工件；原生125/Core/删除执行器不修改。普通单击立即选择并打开AE Effect Controls，双击序列只执行第一次click，保留Enter、修饰键多选、拖动抑制、pending和最新选择合并/旧意图取消。跑实际panel/55suite，交全新独立审校；若freshAE0，经既有Deploy-TestBuild/Junction部署复用125原生与113面板，保留125112一致回退。不得把交互恢复当删除故障修复。只读保存现有125journal供后续外部删除生命周期定位，不启动/操作AE或工程。

已完成：55suite通过；父补A在途/B双击detail2不取消首击意图，全新独立审校冻结当前8输入、实际panel33断言与palette通过，未发现新增阻断。2026-10-30 freshAE0部署125113，1174hash及125112回退验证，十一原生/Core文件逐字复用125，不冒称新SDK。只裁剪旧stb13实际部署备份，保留最近五次及全部候选证据。原删除故障仍未解决，现有125现场journal只读保存并摘要586记录/7次render516，不能从其缺少新执行器阶段诊断推断所有入口。

## STB-17 — 原生增量删除（125/112已部署，宿主门开放）

Owner指出所有节点/面板及ECW直接删除均报错，并要求参考Stardust二进制、减少过重提交验证。只读八个原始AEX，脱敏suite名称与候选引用支持共享原生服务方向，不证明其无验证。实际生产EffectGraphBackup假SDK计数：32节点prepare+discard有22492次标量读取/5016次效果查询，复制33效果；删除也关闭全批别名保留快路。报告/可复现工具/哈希在artifacts/stb-17-20261029/reference-static，具体计数不是AE耗时。

初稿design.txt的全Model事务复用和revised-design.txt的自动Undo方案均已被替代；全新独立审校否决自动DoCommand(16)、只认EFFECT及Clear可覆盖全部删除的承诺。当前拟议契约见ADR0048：普通编辑统一原生入口，最小变化集合/局部补偿、源保留到一次严格发布；发布后删除/刷新失败如实报告已提交及cleanup_error，用户Undo组不作私有回滚日志。复杂资源事务保留全备份。外部删除还需明确旧代取消与派生图重建，不靠更多焦点猜测、未知锁清零或无效旧值替代。

参考核查阶段未修改产品；Owner随后明确批准按此方向实施，下述为当前候选范围。禁止将参考私有接口布局/代码作为规格，不操作AE/工程/真实焦点；实际删除、Undo/重开及性能对齐保持开放。

当前实施范围已由Owner批准：新增NativeGraphEdit.hpp/.cpp共享执行器、Host vcxproj、WebPanelHost/Protocol与webview_host.js/panel.js删除增量入口、gateway只增projectScope、125/112版本、真实生产执行器/路由/浏览器测试及共享fake parade fixture。普通删除发送target/revision/CRC/projectScope/最多63 UUID，Host只暂存连接元数据，源guard3保留到现有GraphCommitRequest正ACK，再降序物理删除/现有binding refresh，复用发布器一次确认，无整图效果备份/Model会话/资源往返/自动Undo。其他编辑和复杂资源仍用现有入口，绑定全局失效规则暂不改。

已识别ECW选择在UI exclusion内捕获，包含whole-effect STREAMREF（公开parent/root unique ID核对），collection持有的引用在修改前释放；busy在SDK读取前消费/drop。原生Clear仅正向SDK EFFECT菜单+相同当前非root native子窗/排除Web树才执行；模糊已识别本插件选择只capture/drop，不能凭旧菜单和负图焦点删残留选择。该门是安全的入口子集，root-only/混选/脚本/其他插件删除仍非全面生命周期资格。第一轮独立审校实际复现旧root-only误授权，已收紧，需新审校。

保守失败契约：原生发布器既有析构回滚不能逐项证明全部alias，所以调用generic后的失败只补偿作者元数据，保留renderer guard；未提交前自有元数据写失败可完整补偿释放。正ACK立即确立committed，之后getter/物理删除/刷新错误只作cleanup；缺ACK状态显式outcomeUnknown，不改未知回执。用户可主动Undo这一个组，不自动调用Undo或清未知锁。既有坏图仍需显式Diagnostics修复。

最终SDK125、删除执行器分析/ASan各116366、Host各768及55panel112套通过。独立v4审校当前21输入一致，生产执行器基础156634/独立53、实际Host路由18和冻结前端通过，无新可复现阻断；前三轮证据与已修反例保留，v3无最终报告不作最终资格。2026-10-30 freshAE0按既有授权冻结/部署125112，1187hash验证及124111完整回退，Core与124一致。只裁剪旧stb12实际备份，保留最近五次。实际AE删除/后续编辑/Undo重开及性能门开放，不冒称一般外部删除已覆盖。回退：powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-17-20261029/rollback.ps1 -Restore（AE关闭）。

## STB-16 — Owner选择单击选择/双击编辑（124/111已部署，外部删除另卡）

124仍有Clear gate3/controlFocus0（有效双订阅），旧source删除后同步拒绝。Owner明确采用单击仅图选择、双击编辑AE参数；补充所有节点及AE Effect Controls直接删也报错，不能以面板UI修正声称外部删除已解决。此卡只拥有panel.js节点选择/显式编辑事件、index提示/版本配对及对应panel测试/文档。保持多选、拖动、Delete完整事务、AE参数编辑入口与所有native/Core不变。单击/多选/普通拖动不发selectNodeEffect，双击或Enter才请求；完整两click+双击序列仅编辑一次，修饰/拖动/重复/写入busy边界不误触发。先实际旧panel夹具复现单击会调用宿主，再局部修正、全新独立审校/55suite、冻结124/111可配对既有124原生。外部效果删除作为后续独立card检查，不扩大当前owned-file后端范围；不操作AE/工程/真实焦点或清未知锁。

Owner明确批准单击只图选择、双击或Enter编辑AE。普通点击/多选无宿主请求，显式编辑仍沿原选择服务；新的图选择/拖动/框选/右键/删除取消未发送旧编辑意图。第一位审校实际真实panel复现A在途/B排队/C新选择后仍发B，修后交第二位全新审校65项及冻结palette通过，8输入hash一致，无新明显阻断；55panel111套零失败。旧失败与两轮审校保留。

2026-10-29T16:46:50.5748863+08:00 freshAE0经既有Deploy-TestBuild/Junction部署124/111，原生11文件逐字复用冻结124，不冒称新SDK编译。冻结213adapter/69Core/3build/232validation/11binary/26panel，最终1164哈希及124/110一致回退核验。当前安装124/111；实际图Delete、AE效果控件直接Delete及原始绑定故障仍未验收，不能把交互修正当所有删除问题已解决。

回退（AE关闭）：powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-16-20261029/rollback.ps1 -Restore

## STB-15 — 123已收到Delete但gate3拒绝：WebView控制器焦点（124/110已部署，现场删除门开放）

Owner123继续同sync555085拒绝。53日志中8条来自父graph-command fake测试PID30276，需明确排除；现场PID7640为45条（40render/5command），Clear observed确有Delete1、gate3、local=gui=foreground，约266ms后首render516。统一hook已抵达，但native窗口焦点门拒绝，不能再称无回调。拥有WebPanelHost.cpp仅控制器Got/LostFocus订阅/状态、图焦点谓词和选择恢复及现有诊断，tests/web_panel_host_tests.cpp/graph_commit_host_tests.cpp及必要runner/version/docs。控制器焦点是WebView2公开API，只有有效订阅、同controller生存期、当前前台root且native焦点正好是宿主root时才允许补足图child缺失；明确其他native子窗/菜单/后台/预设/查询失败仍拒绝。LostFocus/关闭/进程失败撤销状态并丢弃排队Delete；保留epoch/输入/截止/去重与严格绑定/guard/Core，不按鼠标悬停或旧点击猜测，不自动修旧图/清锁。先复现parent-native焦点+control-owned旧门拒绝，再加可测真实controller事件接线/失败/迟到/其他窗口边界；修后全新独立审校、完整SDK/配对回退。离线graph测试显式禁用journal，不污染现场文件。继续不操作AE/工程/真实焦点。

父最终WebHost /analyze /W4 /WX和ASan各749，router46/graph command56及55panel110套通过；旧726/1反例及中间744、fixture命名/非空告警日志保留，中间SDK不充作最终资格。最终May2023 SDK124 NoPublish/RenderJournal成功。全新独立实际生产Host分析792（含其冻结仓库基础，不与父749叠加），11/11当前/快照输入一致，覆盖实际订阅/部分失败/双view唯一owner/旧Lost不撤新owner/Clear正确view/root重parent/freshGot/worker/foreign/stopped/原ProcessFailed lambda/替换控制器及token回收等，无新增明显阻断。

2026-10-29T15:08:08.8353615+08:00 fresh AE0，按既有授权经Deploy-TestBuild/原Starfield Junction部署冻结stb15-native124-panel110-controller-focus（213adapter/69Core/3build/232validation/11binary/26panel）；最终1167哈希验证安装/当前/冻结/面板及123110一致回退。全部26panel、69Core源和Core DLL与123相同。Main A57A8B83F448ED7C0594135F0C1F591B0EE18B6238E8706668116640EC8ABE98，Host 98F0D046790BF40CAFBC481E669006174FAEBBA310F7D9C2DE27807CFFD2C72B。报告后仅裁剪旧stb10实际部署备份，保留最新五次，全部冻结候选和证据保留。

未操作AE/真实焦点/工程或清guard。123现场已证明Delete到达但gate3拒绝；124控制器Got/LostFocus的实际事件顺序、Force删除/后续编辑/Undo重开及准备性能仍需现场验收，不能宣称故障已解决。不以旧值替代缺失绑定，不静默修损坏图或解除未知锁。已分离现场PID7640的45行与父离线PID30276的8条旧记录，原始53行不改写；graph command fixture现显式NoJournal，重跑56项后首次部署124前诊断文件仍不存在。诊断I/O不用于性能资格。

AE关闭后从checkout一步恢复123/110（默认仅报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-15-20261029/rollback.ps1 -Restore
```

## STB-14 — 122注册成功但无Clear回调：单一AEGP命令分发（123/110已部署，现场Force门开放）

Owner确认122仍按Delete后报错；冻结81条日志，Clear=21/registration=0，但零Clear观察/forward/完整transaction，sync33357仍因缺失连接进入旧别名交接拒绝。不再继续扩大焦点推测。正式SDK要求每个AEGP只注册一个CommandHook并按command分发；当前WebPanelHost注册ALL/两个菜单、随后Host再注册六个内部hook，共九次。拥有StarfieldHost.cpp、NativeCommandBindings.hpp、WebPanelHost.hpp/.cpp的仅注册/分发边界、相关真实entry/host测试与runner、版本/文档。改为定义全部会话ID后只注册一次ALL路由，保留六内部handler、两个面板与Clear功能，未知命令不消费，already-handled/worker/停止无写入。WebPanelHost只定义菜单与暴露分发函数，最终注册结果供原有诊断。先以实际StarfieldHostEntry捕获旧注册数量反例，再单点分发；不把模拟的单hook覆盖行为当AE事实。严格别名/guard/Core/epoch/焦点/去重不改，不自动修旧图/清锁/操作AE工程。修后全新独立审校、完整SDK/回退。

修后全新独立审校实际entry/router /analyze /W4 /WX107（含46基础）、实际Web adapter730（含724基础）及10/10当前/快照输入一致，无新增明显阻断。覆盖最终ALL晚于定义、完整分发/拒绝不落入Web、无命令/注册suite、定义/enable/zeroID/最终注册失败、already/worker/stopped及实际adapter图/已有预设窗/Clear入口。父路由分析/ASan各46、WebHost分析/ASan各724、原graph command56/bootstrap14、55panel110套及完整May2023 SDK123 NoPublish/RenderJournal通过。实际旧entry14/1重复注册反例保留，不把计数当AE覆盖语义复现。

2026-10-29T14:39:27.8907338+08:00 fresh AE0按既有授权经Deploy-TestBuild/原Starfield Junction部署冻结stb14-native123-panel110-command-router（213adapter/69Core/3build/232validation/11binary/26panel）；最终1166哈希验证当前/冻结/安装与122110一致回退。全部26panel、69Core源和Core DLL与122相同。Main 23D5EBC9E745869CA98B9FF9F43D726F886599387AAFCF83C6E7870D4F3D855E，Host 1CCCDDC35E4671091CDE4E5DACB50A00B3CFD07C1D5F31E74EBA7D1E9F211209。报告优先裁剪一份旧stb08实际部署备份，保留最近五次，全部候选/原始证据保留。

未启动/停止AE、操作真实焦点、写工程或清guard。修正的是已查明的多CommandHook注册契约偏离；真实AE是否因此丢失Clear回调、Force删除/后续编辑/Undo重开及准备性能仍须现场验收。零旧值替代/未知锁自动解锁/损坏图静默重建，既有严格绑定和回滚不变；诊断I/O不用于性能资格。

AE关闭后从checkout一步恢复122/110（默认仅报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-14-20261029/rollback.ps1 -Restore
```

## STB-13 — 121转交缺失：跨GUI队列焦点判断（122/110已部署，原Force门开放）

Owner121继续相同Force/167/guard0/516；198条日志含公开Clear查找成功和257091/289963同步命令，零完整transaction/零Delete forwarding，后一次verify finite alias handoff拒绝。不据此断言实际键盘路径。正式Win32文档指出GetFocus只观察调用线程队列，WebView2输入窗口可能不在该队列；当前选择恢复及Clear门都用GetFocus，存在可离线复现的跨队列漏判。拥有WebPanelHost.cpp焦点观测/受限Clear诊断、现有host测试与runner、版本/必要docs。用GetGUIThreadInfo读取前台GUI队列且严格核对前台active/root、真实窗口/child，不连接线程输入或改系统状态。先保存旧源并复现local GetFocus空/旧值而前台图child真实焦点的反例，然后局部修正。命令钩子只存有界plain诊断，在idle记录是否注册/匹配/按键/焦点/消费；不在回调执行脚本/删除/文件I/O。保留所有epoch/短截止/DOM双路径去重/严格绑定/guard/Core，不自动修旧图或清锁。修后交新独立审校，SDK/配对冻结；继续不操作AE/工程。

修后全新独立审校/analyze /W4 /WX最终782项通过（含720基础；先前780不累加），精确旧predicate对照确认受控跨GUI漏判，9/9审校输入当前/快照一致，无新明显阻断。父Host分析/ASan各720、55套panel110零失败、完整May2023 SDK122 NoPublish/RenderJournal通过，前态705/1失败及所有审校证据保留。全部26panel文件、69Core源和Core DLL与121配对相同；不改严格别名/guard/ID/schema/ABI/数学。

2026-10-29T14:18:33.7541675+08:00 fresh AE0，按既有授权经Deploy-TestBuild及原Starfield Junction部署冻结stb13-native122-panel110-gui-focus（213adapter/69Core/3build/230validation/11binary/26panel）；最终1161哈希验证当前/冻结/安装及121110一致回退。Main 82BBFA8A6BA4F5A4E0BCE8D2BF48707DC38D1511A4BC8E223FBE9AE778FC86C6，Host 72E4E8A576FA60400FF7C086900CE3C555D48E35B7C7A49FB8C190A0896550FC。备份实际ID为stb13-native122-panel110-gui-focus-20261028（沿模板日期后缀，执行日期以上述收据为准）。报告后仅裁剪旧stb07实际部署备份，保留最近五次，所有冻结候选/证据保留。

未启动/停止AE、操作真实焦点或写工程/清guard。本轮修正有受控反例和Win32文档依据，但未证明现场WebView2/AE确属该队列布局，也不能宣称Force原故障已解决。Clear钩子次序/实际转交、快速按键/文本长按、原图恢复/后续编辑/Undo重开及准备性能继续开放。既有损坏图不自动改写，诊断journal含I/O成本不用于性能资格。

AE关闭后从checkout一步恢复121/110（默认只报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-13-20261029/rollback.ps1 -Restore
```

## STB-12 — 120首错已定位旧别名交接：宿主Delete命令归属（121/110已部署，真实AE门开放）

Owner120截图trace显示submitGraph nonce881849已接受，随后sync因manifest连接指向不存在节点而启动，nonce909342在原生animation binding/stream167/verify finite alias handoff拒绝。保存77条120日志、零完整transaction事件。此证据确定同步被旧不可读Force别名阻断，但不单凭零记录断言完整操作时序/焦点。拥有WebPanelHost.cpp/Protocol.hpp、webview_host.js与panel.js的仅图面板Delete路由、相关测试/runner与版本/docs/ADR；后端严格别名/guard/Core不改。不使用不可用旧值的零/缓存回退、不自动清未知锁。

按AE正式AEGP_Command_ALL/BeforeAE钩子处理经公开findMenuCommandId动态解析的Clear命令；只在当前图窗口真正持键盘焦点/前台、无修饰键的Delete/Backspace时消费，其他AE窗口/预设/命令/脚本不接管。钩子只排队有界UI通知，不执行AE脚本或删除效果；前端沿既有deleteSelection完整事务。通知绑定view epoch、随机page token与输入epoch，过期/导航/焦点或输入变化丢弃，不重放；普通DOM已处理的键不再重复提交，按住键不自动重试。文本字段交还浏览器本地编辑，不删除图节点。新UI元数据只存有界普通数据，维持序号/replay/来源校验；动态Clear查找仅安全idle/互斥下执行一次，不硬编码私有ID/不改全局键盘设置。修后新独立审校、完整SDK/回退，继续不操作AE/工程。

修后第二位全新审校真实Host /analyze /W4 /WX712（703基础+9新增）、独立消息/文本/真实panel handler与deleteSelection46项、9/9输入哈希一致，无新明显阻断。父Host分析/ASan各703、真实panel完整事务路由回归及最终55套panel110、完整May2023 SDK121 NoPublish/RenderJournal通过。升级时漏改preset_manager网关版本导致的2项失败已保留，修正配对后55套全部通过；不称该元数据修正由第二位独立审校执行。所有前态失败与审校证据保留。

2026-10-28T23:41:17.7778468+08:00 fresh AE0按既有授权经Deploy-TestBuild/现有Starfield Junction部署冻结stb12-native121-panel110-graph-clear（213adapter/69Core/3build/230validation/11native-Core/26panel）；最终1161哈希验证安装/冻结源/面板及120109一致回退。Core69源及DLL与120相同。Main A458AF51FE20B7A2AE4D8EA6C4F617B4BB98E607B02A4C9C352CD30A8B71B80A，Host 0DBC0FA90A6B86B25B3483C554E7E3439C2E343E1601D54F0CC065841FC3A29D。报告优先裁剪一份旧stb06实际部署备份，仅保留最新五次，所有冻结候选/证据保留。

未启动/停止AE、操作真实焦点、写工程或清guard。真实AE Clear钩子注册/次序、英文名称查找、Force删除/后续编辑/Undo重开及性能未验收，不能宣称原故障已修复。任意后续输入（包括快速松键）可能保守丢弃一次按键，需新按一次；文本收到原生通知后长按重复未资格。既有损坏图/未知锁仍需显式Diagnostics修复，不自动改写。诊断journal有I/O成本，不作为性能资格。

AE关闭后从checkout一步恢复120/109（默认只报告）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File artifacts/stb-12-20261028/rollback.ps1 -Restore
```

## STB-11 — 119删除后进入同步拒绝：选择焦点与首错链（120/109已部署，现场门开放）

Owner实测119仍Force8b272c/stream167/guard0/516，新增确切banner为node_effect_sync_failed / graph_commit_rejected，并确认只在Delete后出现。该banner来自syncGraphSnapshot而非完整Model删除结果，先保存119日志并查键盘是否绕过事务；不能把此推测当已证实现场根因。拥有WebPanelHost.cpp/WebPanelProtocol.hpp的仅节点选择保留焦点、webview_host.js/panel.js对应typed消息、gateway的首次sync错误链/repair reason、必要tests/runner、版本和当前docs/ADR。选择执行前如果graph仍有焦点，且执行中用户未输入/未切foreground/未关闭或导航，选择脚本返回后恢复graph焦点；普通读写、预设、用户已转去AE Effect Controls不得抢焦点。不拦截全局Delete/不装消息hook/不改AE设置，不新增作者写入或自动修复/清guard。先以受控Win32输入夹具复现焦点被脚本改变后未恢复，再局部修正，新审校/完整SDK候选；保留119/108。

本卡追加只读诊断拥有GraphRenderJournal.hpp、EffectMain.cpp及StarfieldHost.cpp同步graph-command边界：记录Host dispatch/SDK返回/ack/status与效果现成原生失败阶段，避免只有负nonce而没有首个拒绝原因；不改回执或提交/回滚结果。新同步错误首次就附原有有界trace，不要求另触发busy；busy只读不覆盖首错。两位新审校分别复现后台旧GetFocus、执行中reparent仍错误恢复，局部加入前后GA_ROOT和原HWND身份检查后交第三位全新审校；未将受控Win32焦点模型当AE现场根因。中间120构建不充作修后资格，最终结果如下，119实测门失败保持开放。

修后第三位全新审校focus699/metadata54/gateway802/journal3086（514记录）与冻结16/16输入通过；父host分析/ASan各672、首次同步DOM502、55suite、最终SDK120通过。22:57 freshAE0部署120109并核验1,168哈希及119108一致回退，69Core源/DLL不变。完整证据stb11/current-state，先前SDK中间构建与夹具错误均不充作最终通过。仅保留选择前已有图焦点的归属，不改变AE设置或清旧锁；实际Force首错仍须owner验证。Host dispatch的result0不可解释成已提交。

## STB-10 — 结构事务跨线程渲染快照屏障（119/108已部署，现场门开放）

Owner要求继续解决删除问题。118现有UI查询仅当前UI线程为true，无法保护独立render线程拿到的旧guard0，也不能识别checkout期间已经结束的结构修改。本卡拥有UiExclusionHost.hpp/.cpp、UiExclusionClient.hpp、ModelTransactionHost.cpp实际完整执行器边界、Parameters.cpp读取入口、GraphRenderJournal.hpp诊断字段、PluginVersion.h及其真实跨DLL/Host/native回归runner、相关ADR/当前文档。先用实际旧入口在工作线程复现，再为真正完整图结构执行增加独立原子epoch：奇数修改中，完成递增偶数；仅持有UI所有权的线程可开始/结束。跨线程只读，不新增SDK读写，不把普通idle/轮询/选节点/模态scope当结构修改。渲染前后epoch变化或处于修改中取消帧，事务外缺失绑定/非法guard继续报错，不清未知锁或替代值，不更改ID/schema/Core ABI/数值。覆盖修改中、过程中修改完毕、普通UI无屏障、失败/异常释放，修后新独立审校、实际SDK候选；不操作AE/UI/工程。

旧实际23,675/1失败后局部修复，native普通/ASan各24,025、Camera各12、跨DLL1,212、Host /analyze /WX9,664,745及55panel108通过；全新独立22,015并发/9,735,256Host异常变体与冻结9/9输入通过。SDK119成功，69Core源/DLL与118相同。16:57 freshAE0下按既有授权部署119/108，213adapter/3build/230validation/11binary/26panel冻结，最终1,161哈希/118107回退核验通过。新增source/epoch仅保护Parameters读取重叠，不涵盖事后旧快照/后续资源/外部原生删除，原现场门未闭环。记录current-state与stb10；无AE操作或清guard。STB09两处兼容修正同时随108发布。

## STB-09 — 公开社区旧引擎/删除生命周期核查（离线候选，Force原故障开放）

Owner于2026-10-28要求结合公开社区排查，尤其核对Motion历史同类ExtendScript语义差异。拥有web_panel/jsx/model_transaction_transport.jsx的会话占用条件、starfield_gateway.jsx唯一Array.indexOf依赖、tests/extendscript_conditionals.cjs、extendscript_syntax_tests.js及Model传输回归和必要当前文档/ADR。社区第一手报告提示混合逻辑优先级，扫描当前3个JSX仅发现一处未分组OR后AND；另Turbulence类型校验依赖ES3不提供的Array.indexOf，不能依赖其他面板注入polyfill。先保留旧实现并离线复现，再局部显式分组/严格数字比较，不能把模型当真实AE执行或首次Force绑定错误根因。补充旧逻辑模型、短路/括号边界及过期published/executing/applying拒绝接管、缺失indexOf的实际JSX回归；每次产品修正用新独立审校。保留118/107部署收据，不操作AE/工程/guard，不随意改变架构，不自动解锁。社区引用与新日志快照存artifacts/stb-09-20261028；其余删除引用及idle调用核查只读。

Array校验属于既有单一ES3工厂，扩展拥有tools/candidates/native_record_contract.js、web_panel/js/graph_edits.js对应嵌入及tests/native_record_contract_tests.js：同步相同严格比较，保持三份逐字一致和契约身份/版本。仅改gateway的初次55套出现一致性失败，原失败日志保留，不能绕过测试或称该轮全过；修正权威源和嵌入后重新执行。没有原生/Core/公开ID/磁盘schema/ABI变更。当前候选未新冻结/部署，不能把最近118/107安装收据等同当前working源码或内存配对。

最终两处修正各由不同新审校者复核：逻辑111,530，Array/共享工厂4,249（含原1,727）通过；父任务旧逻辑+缺方法Model2,636、缺方法共享工厂710、syntax52和最终55suite零失败。原始失败、研究链接和今天journal快照保留；旧锁不追溯，真正首次Force516/guard1仍开放。

## STB-08 — 118现场删除仍遗留锁：自动故障证据（118/107已部署，原故障开放）

Owner在118删除Force仍一次弹guard0/stream167/node8b272c483ca44731be4c59ddf92a4ffe/parameter1，Force似删除，p0-c88-l111保存guard1阻止后续操作。当前日志1115条含8次同UUID516、无transaction事件、全部uiTransaction=false；仅凭日志不能区分worker、焦点/键盘原生删除、错误Host配对或未经过执行器。先保存日志/当前保存测试工程并只读解析，核对真实事务路由与选择/键盘边界；不再把入口假设当完整根因。

本卡拥有必要的面板选择/键盘与WebPanelHost诊断、ModelTransactionHost/GraphRenderJournal中的只读调用路径证据、相关测试/runner及当前文档/ADR；任何修正需先实际复现并保留严格绑定/事务/回滚，禁止自动清未知guard或重建缺失作者。SDK/参数/ID/schema/Core ABI/数学与所有在途功能保持。先离线工作，不启动/操作AE或写工程；必要现场只读脚本先按ADR0011与owner最新电脑操作约束请求具体批准。

Owner拒绝重复电脑测试，继续纯离线。直接复用118生产.obj离线链接事务探针，945,481检查通过并产生175条阶段记录，不能由此推断AE现场已走同一路由。允许在本卡只改web_panel/jsx/{starfield_gateway,model_transaction_transport}.jsx、必要client generation与测试/当前文档：增加有界、无文件I/O的普通数据诊断，记录持锁/原生回执/Model失败收尾，在既有busy错误中附带同目标最后失败链。不可改变事务决定、命令次数/参数、回滚/解锁或吞原错误；诊断失败不得阻止原流程，轮询不得覆盖最初故障。不启动/操作AE，不再请求同一对照；脚本型测试副本保持未执行。

诊断v1三处归因/容量缺口由独立实际505项复现；v2按saved project path+target最多8份首次故障、32事件、12项trail和有界reader修正，新的独立605项通过，父任务条件模型重放630通过。未保存身份仅engineHistory，不声称当前锁因；旧锁不追溯/不解除。55suite/Model2530/DOM495通过，生产对象/实际前态路由证据保留。17:58 fresh AE0部署118/107并核验118/106回滚；原生11文件完全复用118，不编造新SDK/宿主资格。新独立审校605后曾额度中断，恢复后核对当前107逻辑并完成报告；630保持父任务执行归属。未操作AE/原图，原删除/准备延迟仍开放。

## STB-07 — 删除期间旧PF快照的渲染入口检查（118/106诊断版已部署，现场门开放）

Owner在117/106删除Force仍出现guard0/stream201/binding unavailable，随后p0-c88-l110的saved guard1阻止Particle删除。后者是首次失败后的拒绝，不能算Particle独立复现。拥有Parameters.cpp、GraphRenderJournal.hpp、ModelGraphTransaction.cpp中的仅诊断事件、必要私有UI查询调用点、tests/native_sync_tests.cpp及其runner、RunModelGraphTransactionTests.ps1、PluginVersion.h、ADR0040/0043和当前状态/测试文档；保留全部既有改动与117候选/部署。先用实际checkout_render_graph复现“Host已拥有事务、PF回调仍交付guard0”的重入路径，再做局部入口检查；不得用假host快照证明现场时序。

不修改Host排队/全局互斥架构、公开参数/ID/schema、Core ABI或数值，不自动清guard、接受坏绑定或替代缺失作者。无活动事务时缺失绑定必须继续严格失败。补充进入/退出、途中进入、回调/checkin及正常图回归；若需宿主证据，先准备具体只读动作再按owner最新电脑操作约束请求批准。本卡不授权操作AE、工程或启动/停止进程。

实际旧入口复现后增加现有UI查询；首轮新审校复现迟到scope及跨DLL日志损坏，局部修后换全新审校者验证23,797/Camera12、两轮10000条日志/转义边界通过。最终native ASan23,601/Camera12、完整事务ASan945,479、SDK118成功；Core源未改，实际DLL36scene/像素一致。额外严格事务分析暴露既有69KB备份栈C6262，明确保持开放。本轮12:54 fresh AE0下按既有授权部署118/106并核验117/106回滚，未操作AE/原图/guard；当前配对/诊断路径/边界见current-state。原故障及worker/已结束事务旧缓存不由此关闭。

## STB-06 — 有序Force内部32位种子误拒绝（117/106已部署，待owner测试）

部署补记：owner于本轮明确要求部署；2026-10-26 20:53 +08:00 fresh AE0后经既有工具完整发布117/106并核验11二进制/26panel/Core selector/Junction及114/104一致回滚预检。旧working106另留前态，回滚104来自冻结114配对。按五版政策报告并裁剪一份旧实际备份，全部候选与证据保留。未操作AE/工程/guard，等待owner测试；准确身份/一步回滚以current-state为准。

第五位独立审校Core后复现合法Emitter Seed0 + Particle Seed Shift -1 + Circle→Force→Output在113/当前DLL均失败，同图无Circle成功。OrderedForce.hpp把合法内部0xffffffff交给public seed≤2^31-1校验，seed_clamped notice被误作Force越界。ParticleBirth.hpp明确内部RNG全32位，Auxiliary混合也可产生高位。证据stb05/reviewer-core/result-complete.log。此卡只拥有src/core/OrderedForce.hpp、tests/motion_frame_tests.cpp及必要现有离线runner/CMake验证、PluginVersion/相关ADR和当前文档。只在内部Force物理量校验保留完整派生seed，公开作者限制与其余notice严格失败不变，禁止mask/clamp种子改变数值。

必须先用冻结旧源码/实际DLL复现，覆盖signed32边界、随机Force数值参考、静态/历史/Auxiliary、合法低位原结果一致和非法作者/Force仍拒绝；Core变化需重建并记录新哈希，完整SDK与全新修后审校者复核。无需架构改变，不改公开ID/磁盘schema/ABI/Render.hpp，不操作AE/GUI/部署。116/106及此前候选保留，真正现场缺失绑定与本缺陷不可直接画等号。

局部修复/离线完成：只在物理校验成功后恢复完整derived seed。旧实际DLL和新共享回归均复现合法拒绝；修后Core23/23、MotionFrame ASan9,799、Core严格分析/SDK117通过。新审校者独立1,058检查未发现本范围新明显问题。实际新DLL36case出图，其中18旧合法拒绝修复、18已有scene/像素字节一致，另6legacyDLL一致。新Core哈希见current-state；213adapter/69Core/2构建/36验证/11二进制/26panel106冻结。三轮修复均换新审校者复核，共6位独立审校参与，证据stb04/05/06；未部署/操作AE，现场门保留。

## STB-05 — 备份准备的双故障恢复屏障（116/106冻结，新独立审校通过）

STB04前端115/106已冻结并完成新审校者复核。第三位独立原生审校用实际115 EffectGraphBackup实现复现：第二次duplicate失败且备份delete失败，prepare/rollback均516却live guard0；partial-success duplicate再清理失败还残留同UUID的两个guard0源。证据artifacts/review-native-recovery-20261026。此卡只拥有EffectGraphBackup.cpp、必要EffectGraphBackup.hpp、tests/effect_graph_backup_tests.cpp与其两个离线runner、PluginVersion/当前文档/ADR0040。保留原错误和首个清理失败；只有全部准备失败清理成功才释放guard，不自动恢复未知状态。

补充双故障、静默delete失败、复制已发生但返回错误等实际代码回归；原单故障恢复与正常成功语义不变。完整SDK候选、严格分析/ASan及新的独立审校者必须复核。只改局部失败分支，不做架构变更，不操作AE/GUI/部署、Core/参数/ID/schema/ABI。前端106与所有115候选保留；完整现场根因仍未确认。

修复/离线完成：保留首个清理错误，所有局部/marked清理均成功才解锁。真实/静默、永久/一次性失败与partial duplicate覆盖；945,479完整事务ASan通过，新审校者独立实际代码24场景通过并未发现本范围新明显问题。目标/analyze /WX及完整SDK116 NoPublish通过，116/106独立冻结；原始复现与115各候选保留，当前哈希及精确资格见current-state/artifacts/stb-05-20261026。无AE或部署；另有全新审校者独立检查此前Core准备存储，结论另记录，不能由这里推断全项目或现场已无问题。

## STB-04 — 发布态与最终事务回执交错（115/106冻结，双独立审校通过）

继续owner的多轮离线静态审查目标。本卡拥有web_panel/js/model_graph_transactions.js、jsx/model_transaction_transport.jsx及必要API/generation声明、对应JS传输/生命周期/面板测试、ADR0040/0046与当前文档；以冻结115/105为基线。重点审查published状态之后原生commit确认丢失、仍可能发生回滚、最终通知丢失/延迟、cancel/timeout/release的组合。不能把script publication等同于原生事务最终committed，不丢弃尚待投递的最终回执，不自动重试未知结果或解除外来guard。

保留115全部原生修正与候选；不修改Core、公开参数/磁盘schema/ABI，不操作AE/电脑、不部署。用真实JSX/client和可控内存Host模拟确定次序，先复现再改。若只改前端，复用已核验115 native产物并同步前端generation，不能虚构新的原生SDK或宿主资格。完整结果语义/响应身份校验若发现缺口，须同卡记录并覆盖严格失败。

第一位独立审校复现published后取消误报committed、丢失回执目标及过期接管，确认原生确认丢失仍可rollback。最小修正后新审校者跑迟到begin/结果先于取消/取消后迟到结果，没有发现该范围新明显问题；未为假设伪造输入扩大改动。实际JSX/client2530断言、完整panel55套件、实际catalog Motion删除DOM156/181通过；115 native未变，复用其已资格SDK产物，独立115/106冻结。证据artifacts/stb-04-20261026。另一位原生独立审校发现prepare双故障会释放guard0，已有独立实际代码复现，交下一张卡；现场AE门仍开放。

## STB-03 — native114删除事务缺失绑定与遗留guard（115/105冻结，现场门开放）

Owner在AE23.5x52测试114：删除节点多次弹binding unavailable（guard0/stream167/node5729382497c94ec587687499d0d4f3de/parameter1/component0），节点似被删除，随后node_effect_sync_failed与guard1 busy，保存读回一致。不能从截图推断具体先后或已确定根因。本卡拥有EffectGraphBackup、ModelGraphTransaction/Host、NativeNodeGraph的必要事务绑定/回滚、JSX gateway/model_transaction_transport及客户端事务错误处理、相关离线回归/runner、PluginVersion及ADR0040/当前文档。先冻结114基线并用实际代码复现，保留全量节点身份/动画/图/别名/Undo与严格错误；不通过清零未知guard、零值/哨兵回退或隐藏弹窗掩盖损坏。

先离线检查并完成可审查修正；本次未授权操作当前AE/工程或启动/停止宿主。原114候选、110回滚及在途功能必须保留。宿主删除/Undo/重开和一秒目标需真实验证，不以模拟回调证明全部现象已修复。允许在本卡同步内部ModelGraphTransaction.hpp的回滚finalize说明、前端generation声明与完整前端回归；不改外部ABI/参数/磁盘schema。

完整面板注册审计使用的10月12日catalog已过期，需要当前实际节点注册重新生成。允许修正仅限该审计依赖的Model/Motion/Material注册夹具/runner，使其遵从已有UI注册、零count按钮/陈旧事件及已变参数布局契约；不得因此改变当前功能源码或公开ID。新增任何产品变更仍须独立证明与本卡范围复核。

离线完成：统一saved guard判断、失败恢复在最终parade上重新绑定后才释放、收尾失败保留guard并报告；旧JSX误拒绝/旧备份提前释放均有实际代码复现。原114用户Force拓扑夹具本身通过，现场完整根因未闭环。normal/ASan native各22,759/Camera12、事务ASan934,562、备份ASan706,638、目标分析/WX、panel55套件/全注册审计及额外DOM156/181通过；仅修正过期测试回调/按钮契约。完整SDK115/213输入与最终stb03-native115-panel105-qualified冻结；Core不变，dist114与110回滚保留，无AE/部署操作。已请求只读现场脚本批准，现场guard恢复/删除/Undo/重开和性能门继续，详情current-state与artifacts/stb-03-20261026。

## PERF-09 — Core栈与索引静态审计（114已部署，宿主门开放）

部署补记：owner随后明确要求部署；2026-10-26 14:45 +08:00新AE0检查后完整发布114/104，11二进制、26panel/Core selector和110备份/Restore报告核验通过。按既有五版政策报告并裁剪两份最旧实际部署备份，全部候选和证据保留。未操作AE/工程，待owner测试；当前身份与一步回滚见current-state。以下保留部署前离线任务记录。

PERF-08实现/113候选已冻结，宿主门保留。本卡拥有src/core/{GraphEvaluation,SequenceCodec,MotionGeometry,Settings}.cpp及其内部TemporalEvaluation.hpp（若实际构建同样暴露时间求值栈问题）、必要的对应测试/离线runner、PluginVersion.h及相关ADR/当前文档。以实际MSVC /analyze /WX和栈生成代码确认65KB来源，优先减少冗余大临时对象/重复初始化，避免逐粒子新增堆分配。CRC表与三次样条索引需结构性证明范围，不禁用告警，不用默认值掩盖无效输入。保留NDEBUG与非NDEBUG基线，不能通过改变宏隐藏告警。

保留所有数值公式/遍历与分支顺序、稳定身份、Auxiliary递归/预算/取消、原类型化失败、CRC与序列字节、Settings/Render.hpp和Core ABI9/参数ID/schema。覆盖旧113与新代码的完整数值/编码对照、Auxiliary及复杂路径、边界、失败/内存和分析；Core若变化必须重建并记录新哈希，不能沿用113。只做离线工作，不部署/操作AE或真实设备；一秒目标与连续操作验收仍开放。

静态审计还发现Emitter/Particle/Force的大optional数组均按全图节点数分配。允许在本卡按实际类型紧凑存储、预先精确reserve并保留节点索引指针视图；不能逐节点新建堆对象或跳过停放节点原语义校验，必须验证指针在完整请求/递归中的稳定性及分配故障。

本卡同时拥有include/starfield/core/Error.hpp中Result的内部构造：大值成功返回会经中间Storage再搬入成员，可直接原位构造成员。保留公共签名、variant成员布局、类型化错误和所有权/异常行为，不修改Render.hpp或Core ABI；以完整Core测试及旧113数值/移动/分配对照验证。实际SDK编译记录确认主AEX/Core均未定义NDEBUG，最终分析不得用额外宏掩盖时间求值两处栈告警。

为避免递归帧保留多份大reader/Settings临时，允许新增内部EvaluationStorage.hpp和构建输入，用一次有界堆frame承接原位factory结果；仅使用std::optional管理生命周期，禁止手写裸字节存储/悬空指针。普通/时间递归各自拥有frame，ordered Force的可复用scratch仅在求值请求内延迟创建，不得逐粒子分配。测试须覆盖不可移动值的直接构造、factory异常、旧新完整结果和分配故障。

实现/离线验证完成：五处原Core告警已修，四个拥有cpp无NDEBUG的/analyze /WX通过；独立生成代码栈80,448→25,616/55,864→10,008/14,496→2,272bytes。19旧/新normal/ASan完整编码场景一致；旧1,724/新各1,680项含静态及时间逐处分配失败、Core23/23、native21,359/Camera12、Model桥ASan997及实际SDK DLL六场景scene/像素字节对照通过。SDK114/213adapter输入、完整Core输入、2构建输入、20验证源、11二进制/26panel104冻结；Core新哈希以current-state为准。128Force累计分配字节降约56%，Auxiliary t36/24约23%；固定frame在简单场景略增内存，不承诺普遍下降。安装110/104只读核验未变，无部署/AE/设备操作；端到端目标继续开放。证据artifacts/perf-09-20261026。

## PERF-08 — Output范围的宿主资源准备（113已冻结，真实宿主门开放）

PERF-07已冻结112候选，真实宿主门保留。本卡只做离线工作，拥有GraphOutputScope.hpp、NativeNodeGraph.cpp中的既有范围collector、SmartRender.cpp、TextureResources/ModelRenderResources内部接口与实现、对应资源/SmartFX/原生测试和必要runner/build输入、PluginVersion.h、ADR0043及当前文档。保留112/111候选、Core/Render.hpp、全部参数/ID/schema和作者图；不操作AE、不部署。

检查实际pre_render在已有Output范围采样之后，仍把完整作者图交给Motion灯光/快门、纹理/材质准备，停放节点可能强制宿主资源请求和重复模拟。先用实际SmartFX入口离线复现；修正仅生成callback-local的Output祖先工作图（包括所有几何/材质/Auxiliary父引用边），不写回/修复作者图。完整输入仍保持结构校验；活动节点资源/损坏/缺失/取消继续严格失败，Core独立全图求值合同不变。

纹理与Model镜像槽位来自完整作者图，绝不能裁剪后重新编号。资源准备显式借用原完整布局，仅采样/分配Output消费的资源；保持完整镜像count/ID/revision/bounds校验及现有shutter/缓存键/回收。覆盖停放Motion缺灯光、停放贴图失效、镜像前置空洞、活动资源严格失败和Output数值/图结果对照；静态分析/ASan/SDK候选不代表AE一秒或连续操作验收。

113实现/离线/SDK冻结完成，未部署。冻结112实际pre_render五项夹具复现两项停放Motion失败；当前pre_render ASan24、真实纹理ASan142、Model桥ASan997累计（包含控件基础）、原生normal21,359/Camera12、四个实际adapter /analyze /WX通过。212生产/18验证输入、11二进制/26panel104独立冻结，Core未变、安装Junction/dist110/104核验。make_unique保留外层OOM码且故障注入无partial frame；旧纹理runner缺Model链接、528过期断言/ASan子进程环境、初次fixture/代码错误均保留证据。宽范围Core分析另暴露C28020两处和GraphEvaluation约65KB栈C6262，未关闭或算通过，需下一张独立Core卡；本卡不擅改Core。详情artifacts/perf-08-20261026/report.md与validation.json，goal仍未完成。

## PERF-07 — 完整依赖收据合并准备校验（112已冻结，真实宿主门开放）

STB-02实现与111候选已冻结，真实宿主门保留；owner要求先继续离线检查。本卡拥有NativeTemporalCache.{hpp,cpp}、NativeNodeGraph.cpp的UI证明发布、相关注释、PluginVersion.h、对应原生/缓存测试与runner、ADR0043及当前文档。不得操作AE或部署；不改Core、参数/ID/schema/Render.hpp、原生作者事务和111候选。

只允许在所有活跃alias预热之后，以PF_ParamIndex_CHECK_ALL、空时间范围的两次匹配状态夹持完整证明捕获，给immutable publication附加完整依赖收据。每项source恒定/曲线资格、单项状态夹持和数值读取继续保留。渲染按UUID/stream选择；完整收据匹配可合并该publication的重复查询，收据缺失/变化/失败则保留逐项严格校验和旧有效候选回退。既有无完整收据路径的查询/结果顺序不变；不得以graph内容、effect_ref相等、UI调度stamp或磁盘值代替宿主依赖证明。

离线覆盖512输入的查询次数、动态payload、跨回调、部分依赖变化、全局查询失败、捕获中变化、重入/淘汰、并发和内存失败，并比较实际adapter完整图编码。静态分析、ASan及SDK构建通过只说明候选条件；AE一秒目标和复杂度斜率仍需另行批准实测。

实现/离线/SDK112冻结完成，未部署：稳定512输入查询/比较512/512→1/1，保留严格回退。缓存/analyze(/WX)与ASan各124项，含基准3,724；actual adapter normal/ASan各21,359/Camera12、旧缺失Auxiliary夹具21,364/Camera12；实际NativeNodeGraph /analyze /WX通过。211生产输入/12验证源/11二进制/26panel104独立冻结，Core未变，dist110/104只读核验。首轮测试顺序改变旧快照有效性导致4断言失败，已把新捕获故障用例移到原快照用例之后；保留失败证据，不修改产品行为迎合夹具。报告和收据artifacts/perf-07-20261026；无AE计时或参考对齐声明。

## STB-02 — 脚本诊断分类与可定位失败（111已冻结，待批准宿主验证）

PERF-06实现/离线/110发布已完成，保留全部宿主开放门。本卡拥有ScriptDiagnostic.hpp、WebPanelHost.cpp、PluginVersion.h、对应Model/预设/面板Host离线测试及必要runner、ADR0046与当前文档。当前真实AE只读探针可直接加载gateway104且返回no_target；面板host_script_failed原因未确认，不把静态推断写成真实根因。

当前分类器把diagnostic容量>8192视为失败，即使其首字符NUL。按SDK字符串契约修正这项容量误判，保留SDK非零返回、非空文本和内存/suite失败拒绝。补齐大容量空缓冲区与非空错误、失败/锁配对回归，并让面板报告有界的原始AE错误码/诊断，不只返回通用Refresh提示。不隐藏真实错误，不改项目、渲染/参数/ID/schema/Core ABI9或后台事务/队列/quiet-error契约。

优先/WX静态分析和离线测试、完整SDK候选；当前110继续安装，AE运行时不部署。遵守owner最新约束：非必要不操作电脑，必要操作前说明具体动作并请求批准，不复用此前UI授权。原工程备份和所有候选/失败日志保留。

旧110原代码在更新后的纯内存夹具check61复现容量误判；修正后四个客户端通过。面板/analyze(/WX)及ASan各509项；Model事务/analyze9,664,560项、ASan9,665,054项；资产150,891与预设1,842项。保留首轮测试换行转义断言错误和隐藏测试窗口未显式检查NULL的/analyze记录；按协议/所有权修正，不关闭告警。完整May2023 SDK111/211当前输入、11二进制/26panel104/8验证源冻结，Core与110相同、当前安装110未变。实际AE根因和性能目标仍开放，须遵守新电脑操作批准约束。

## PERF-06 — 已认证常量数值复用与宿主准备测量（110已部署，宿主门开放）

本卡拥有NativeTemporalCache.hpp、NativeNodeGraph.cpp、PluginVersion.h、相关原生/证明缓存测试、必要宿主测量工具与ADR0043/当前文档；另拥有真实NativeNodeGraph /analyze暴露的AgeCurve.hpp索引边界及必要曲线/Core回归，不改变有效曲线公式/编码。109已部署；继续整体性能/稳定性目标，不将证明微基准等同于AE一秒目标。调查当前帧别名checkout仍逐项执行八段UUID表达式：可由UI在宿主state前后夹持、实际alias求值且验证有限值/类型/启用状态的恒定数值，随原生证明持有；每次读取仍按UUID/stream及整个宿主依赖状态验证。动态/选层矩阵/未知或失效证明、数值获取失败/内存不足必须走原checkout，不读取保存记录中的旧数值。只读PF_State缓存契约与真实AE资格分开。

不改表达式身份校验、公开/磁盘ID、schema/Core ABI9/Render.hpp；不移除缺失源错误、不自动修复启动工程。覆盖冷加载/回调拷贝、动画/表达式/数值与依赖失效、错类型/非有限/不可用值、取消及故障回退；优先静态分析/ASan/实际原生夹具和完整SDK。宿主测量脚本须具体可审查、只操作checkout内临时项目，启动AE仍需ADR0011单独授权。所有在途修改/109候选与回退保留。

实现/离线/发布完成：认证数值复用、按kind字段库、曲线显式边界；normal/ASan原生各23,976/Camera12、旧保存工程夹具23,981/Camera12、实际NativeNodeGraph /analyze /WX、曲线分析258,807、标准Core22/22通过。CMake纳入曲线边界测试。SDK110/211输入/11二进制/26panel104冻结并在AE0下发布，109配对回退预检通过。真实AE23.5x52只读探针确认网关可加载/会话绑定存在，当前Comp5未选图层返回no_target；此前面板host_script_failed的具体原因未确认。独立工具ES3保留字已改，首次失败快照保留；没有完成15帧/一秒延迟资格。具体收据以current-state及artifacts/perf-06-20261025为准。

Owner最新约束：非必要不操作电脑，如必须操作，先说明动作并请求批准。已允许编辑testproject.aep的权限保留，但后续AE窗口、脚本执行、启动/停止均先请求批准；代码/离线验证继续。裁剪旧备份被AE运行守卫拒绝，无删除，暂有六份，待获准关闭或owner关闭AE后再执行既有五版政策。

## PERF-05 — 原生证明快照与准备查找规模（109已部署，AE待测）

整体性能/稳定性goal继续，STB-01修复已部署但真实AE恢复待owner验证。本卡拥有NativeTemporalCache.cpp、NativeNodeGraph.cpp、EmitterHistoryCapture.cpp、PluginVersion、独立证明缓存测试/runner、必要原生回归、ADR0043与当前文档；证据artifacts/perf-05-20261025/。先测当前证明选择随active输入/32份publication增长的分配、复制、CPU和SDK调用；按不可变自有快照缩短锁范围、索引化重复查找，并去掉逐粒子的证明扫描。不改宿主依赖校验、最新有效证明优先/旧有效回退、Output范围、参数ID/schema/Core ABI9/Render.hpp，不用回调引用或相等帧值代替PF_State比较，不改启动/修复行为。

单线程/重入/并发发布、状态失败、clear/evict、prefix lease与bad_alloc必须保留安全回退，历史结果/PF调用范围保持。优先MSVC静态分析和地址检查，再实际原生与完整May2023 SDK验证；不能把证明缓存微基准当成AE一秒目标或连续操作无错误资格。所有已有修改/108/104回滚材料保留；仅fresh AE0按standing授权配对发布，不启动/停止AE或改工程/环境。

实现/离线验证/部署完成：不可变catalog/索引/锁外释放、请求内Life/rate索引与分配失败回退；新增先清理旧常量证书/raw字段的失败安全性，旧108同夹具复现三项残留失败，新109通过。证明71正常/分析/ASan、1871含基准，原生normal/ASan21,818/Camera12、保存图21,823/Camera12通过；512输入/32publication证明选择约2.16→0.060ms（无rate keys）或5.63→0.065ms（1024-key profiles），不代表AE总准备。完整SDK/211输入、11个native/Core/26个panel104冻结，AE0发布109/104、108回退预检及五版备份保留完成。当前哈希/命令以current-state为准；STB-01实际修复和整体性能/稳定性goal继续开放。

## STB-01 — native107启动active alias不可用（108已部署，待显式修复验证）

只读核验此前相关D:/Project/Code/test/testproject.aep的当前保存数据已匹配截图真实UUID **7f86fd58ba9f494e80725f518bcf16ec**（初次截图转录少一位，以下旧描述作废）。保存图含该Auxiliary及Particle双向连接、binding slot110；同层原生Emitter仅a638bf/25226c/c13ba2，cdat UUID与名称一致，7f86fd源效果不存在。源消失的操作/版本尚未确认。新增拥有GraphRepair.cpp、必要的连接修复辅助/故障测试、Restore-TestBuild.ps1及对应隔离测试：显式Diagnostics修复允许idle guard0且成功回执的陈旧图，先完整静音备份/禁用原渲染器，再仅清理指向已缺失native UUID的连接并严格重编译/绑定；不自动改启动工程、不重造缺失源、不使用虚构参数。修复保留单Undo和失败guard，待实际宿主确认。恢复工具报告必须覆盖保留路径冲突，不能再报告“可恢复”而实际拒绝。

Owner报告当前107打开工程/启动后不能正确出图，截图为guard0、stream208、node7f86fd58ba9f494e80725f518bcf16ec、parameter1/component0的binding unavailable。优先处理，暂停新的复杂度优化；现有107源码及候选/测试全部保留。此前107的Core/离线回归不能证明当前工程启动可用。本卡拥有原生绑定/加载/预热相关NativeNodeGraph、NativeTemporalUI/Cache、Parameters/GraphCarrier/EffectMain/Host必要最小修正、GraphRepair、NodeRecord/MotionPrefix的必要静态分析修正、PluginVersion及对应原生/故障/重开/静态分析/ASan回归、ADR0043/0040和当前文档；证据artifacts/stb-01-20261025/。先只读比对当前安装/107和106变更、捕获启动次序/绑定身份根因；不从截图宣称已确定某种旧工程或107变更根因，不用零值/旧值绕过活跃source严格校验。

AE0时可按standing授权回退保留106/104配对并验证，以隔离当前失败候选；回退本身不宣称修复。保留107失败候选/符号/安装证据和原始工程，不自动启动/停止AE或修改工程、缓存、注册表和环境。修正需故障回归与完整May2023 SDK候选、指纹/配对/回滚；真实AE启动/切帧与一秒目标仍需完整证据，不关闭整体goal。

STB-01实现/离线回归/发布完成：实际修复协调器normal/analyze/ASan各24,530项、native21,468/Camera12、保存工程stream208复现21,473/Camera12、Motion author3,108、Model453和隔离回滚通过；修正旧测试越界与头文件分析告警而不关闭warning。完整SDK108/211输入、11个native/Core/26个未改前端冻结，新AE0下配对发布108/104并核验106/104回滚；Restore预检与实际动作的候选保留冲突已补回归。当前需owner在原图层Diagnostics显式修复，不自动改工程；实际出图/Undo/重开与性能目标未验证，整体goal继续。当前哈希/命令/备份政策以current-state.md为准。

## PERF-04 — 跨帧常量编译复用与 UI 预热去重（改动保留在108，待AE实测）

Owner确认几乎所有节点在不改参数、渲染不同帧时仍有很长准备时间。本卡拥有GraphEvaluation.hpp的请求内采样能力、TemporalEvaluation.hpp、EmitterHistoryCapture与NativeNodeGraph、SmartRender计时/Diagnostics、NativeTemporalUI/NativeTemporalCache及其头文件、PluginVersion、对应Core/原生/GPU测试和必要的CMake/RunCoreTests输入修正、ADR0026/0043和当前文档；证据在artifacts/perf-04-20261025/。只将宿主已验证的全时常量暴露给请求内编译缓存，减少逐粒子节点复制、参数/曲线解析以及重复常量checkout；动态输入及外部资源仍按既有历史语义采样，不修改积分步长、模拟次序、Render.hpp、公开ID、磁盘schema或Core C ABI9。不以相等帧值推断恒定，不引入跨帧共享求值结果。

保留所有已有在途修改和panel104。先对照有/无常量能力的同一历史算法，覆盖复杂Force/Transform/Turbulence/Motion及动画/证明失效、取消和不同帧；补齐准备阶段计时，区分离线收益和真实AE延迟。Owner后续截图为C472ms/H7.6ms、UI最大557.4ms，体感5–8秒；此前将小数点误读成5574ms已明确更正，不能将不同回调计时相加解释总等待。本卡扩展拥有NativeTemporalUI.{cpp,hpp}与必要的原生元数据入口/测试，合并DRAW/idle/显式元数据成功捕获后的去重，移除至少三次全量预热；有效依赖变化/缺失证明仍重采，保留首轮lazy依赖预热。Options识别缺渲染资源的作者Light Path快照，避免将其直接作为出图错误。完整May2023 SDK候选构建；仅在新AE0检查通过后按standing授权部署配对并保存哈希/回滚，不启动/停止宿主或改环境。实际AE准备时长仍需owner复测。

实现/离线验证与107/104配对部署完成。Core21/21、720组编码结果一致、原生21,468项/Camera12项、独立CUDA/OpenCL475项通过；稳定UI每次仅一次整体state查询，跨回调引用仍去重，证明清除可恢复。3万/5万复杂图Core准备中位数降约37–56%，不等于AE总体收益。完整SDK与211项源指纹一致，新AE0下发布并核验11个二进制/26个未改前端/selector和106/104回滚，Restore报告通过；按五版策略报告后裁剪一个最旧实际备份。当前安装/哈希/一步恢复与未关闭的真实AE门见current-state.md；最终证据artifacts/perf-04-20261025/，未部署的中间候选也保留。

## PERF-03 — 连续节点添加与作者准备批处理（panel104已部署，待AE实测）

Owner确认渲染速度可接受，要求第二批集中解决连续添加节点的打断与出图前准备。本卡拥有web_panel/js/{panel,graph_transactions,graph_edits}.js、前端generation标记、对应面板/队列测试、ADR0040/0046及当前文档和artifacts/perf-03-20261025/。仅对已初始化图的独立addNode开放有界、稳定UUID的本地预览和单写者批提交；等待期间仍可继续添加，其他编辑维持互斥。同批一次Undo，逐批使用最新确认revision/stamp；失败/超时/警告停止未提交项，外部目标切换取消本地队列且旧回执不得覆盖新画布。保留宿主完整校验/守卫/读回/回滚，不并发宿主写入，不自动重试未知结果。

通过合并待添加项、复用确认回执、移除每笔添加后的立即全面刷新，减少重复作者构图/绑定检查/读取。Core模拟、native106快路、参数/ID/schema/ABI及原生代码不变；独立离线延迟与调用次数不当作AE主线程或宿主资格。AE0时才按standing授权配对发布前端，保留106/103快照和一步恢复，不启动/停止AE或修改外部环境。

实现与离线验证完成，最终55个suite通过；队列205项，完整palette的11次连续拖放仅两次串行提交，自己回执在下一次拖动中到达不取消手势，无逐笔全面读取。实际JSX/fake-AE新增按批创建/Property失效/Undo/中途失败回滚覆盖。26个前端104冻结，8文件变化；11个原生/Core及211项输入未变。AE0下KeepNative发布106/104并核验当前/冻结/Junction/103回滚配对及Restore预检。证据artifacts/perf-03-20261025/，回滚命令与五版保留状态见current-state.md。真实AE输入/注册/Undo/保存重开仍待owner复测，不从模拟延迟回调宣称已完全消除卡顿。

## PERF-02 — Output范围与常量重力/阻力准备快路（106已部署，待AE实测）

Owner要求先实施效果最明显的优化。本卡拥有ae_plugin/EmitterHistoryCapture.cpp、NativeNodeGraph.{cpp,hpp}、PluginVersion.h、tests/native_sync_tests.cpp、必要的独立性能夹具、ADR0026/0043及当前状态/测试/路线图文档。保留全部已有在途修改。先复现停放Force触发历史遍历，再按既有Output祖先范围判定静态资格；只允许已由宿主PF_State证明所有活跃输入恒定的单条线性Emitter→Particle→Force(s)→Output链、Default发射、无随机寿命、无Auxiliary和无Wind/Spin的场景复用既有解析求值。动画、Wind/Spin、Transform/Motion/Turbulence及其他复杂活跃节点保留历史路径，不从相同采样值推断常量。扩展数值矩阵发现并行Force路线不等价，已明确排除分支/汇合，不扩大到Core语义修正。

不改Core算法、Render.hpp、参数/磁盘ID、ABI或schema。覆盖真实adapter路径、证明缺失/失效、Force重力随机/阻力、并行Force、时序/取消与数值对照；完整May2023 SDK候选构建。AE运行时不部署，不启动/停止宿主；AE0时按standing授权配对部署并保存收据/哈希/回滚。本卡不声称已对齐Stardust，也不顺带重构编辑事务。

实现/离线验证完成：先复现两项旧慢路，最终实际原生同步21,580项、Camera12项及Core CTest20/20通过；504组已限定线性场景数值对照通过。修复前后同夹具“准备+Core求值”中位数3万270.2094→5.5118ms、5万456.1273→8.9083ms；非AE总帧耗时。完整SDK最终候选及211项输入一致，2026-10-25 AE0下发布106/103，11个文件与26个前端/回滚配对核验通过；原生105为最近回退。首次Restore报告缺前端快照已补齐并通过最终只读预检，原日志保留。部署/备份保留、命令与宿主门统一见current-state.md；本轮证据artifacts/perf-02-20261024/。

## PERF-01 — 编辑响应与渲染前准备性能审计（已完成，2026-10-24）

Owner报告连续添加节点需等待，几万粒子的体验明显落后参考插件；随后补充AE显示约41ms，要求重点综合分析渲染前准备。本卡仅拥有本任务卡与artifacts/perf-01-20261024/中的当前源码指纹、独立基准、原始结果和审计报告。只读追踪面板事务、宿主提交、SmartFX controls/history/resources/scene与核心求值，不修改产品源码、参数/ABI/schema、SDK、dist或宿主环境，保留全部在途修改。

区分源码事实、离线测量与实际AE假设。独立基准不代替AE2023端到端延迟或Stardust同场景测量；不从41ms截图推断未测阶段。后续优化按实测收益与契约风险拆卡，本卡不授权绕过守卫、回执、回滚或自动启动/停止AE。

当前Core全新Release隔离构建与162条分阶段测量完成，9组粒子身份/位置/速度对照通过；核验215个审计源码指纹。常量Force的3万粒子普通/历史路径约5.07/232.88ms，3万冻结结果约6MB，独立编码/图传输/解码合计约27.62ms；均非AE实际工程计时。报告、复现源码、原始CSV、构建日志与后续排序位于artifacts/perf-01-20261024/report.md。产品代码、安装与宿主未变；AE端到端及参考同场景门仍开放。

## P-09 — 全节点原生选择控件审计（105已部署，待AE复测）

Owner要求非必要不自绘、所有节点检查，后续截图确认104中Shape及Transform图层选择整项消失。当前修正沿用正常Particle Color/Up Axis/纹理层的原生注册，删除add_checked_parameter对首项参数的PF_PUI_TOPIC/尺寸注入及标题自绘路由。定位线暂停占用原生参数行，不新增自绘行或参数，Host reveal/会话定位保留。普通控件完全由AE绘制；专用曲线/渐变/文字UI保留。104参数追加迁移见ADR0032，本次105不再改ID/类型/索引/count/schema/ABI。

本卡拥有NodeEffects.cpp、NodeRecord.hpp、ParticleShape.hpp、TransformNullUI及相关头/构建输入、PluginVersion.h、node-parameters schema、原生/Shape/Null/纹理/全AEX审计测试与ADR0032/0038、native-controls-audit及当前文档。保留所有Auxiliary/Transform/官网和其他在途修改。首轮104普通控制区测试通过不代表完整原生行通过；严格AEX注册/标题区测试在104复现6项失败，105相同240项通过。共享注册/原生回调和完整SDK构建通过；精确计数/日志及初次失败保留在审计页与artifacts/p09r-native-titles-20261023/。

Owner10月24日要求继续后，核验既有成功构建/针对性日志、211个当前输入哈希与实际八类AEX审计，在新AE0检查下按standing授权完整部署105/103并核验配对与Restore预检；前端103/Core不变。最新回滚保存104（该版显示失败，不能称为验收基线）；103稳定候选与前次备份保留。安装/rollback命令、最近五版保留及仍开放的AE宿主门见current-state.md。不得把离线成功当作选择框实际恢复。

## P-08R — 选中效果图层崩溃恢复（103已部署，待原步骤复测）

Owner于2026-10-23报告native102无法使用，选中含效果的图层即崩溃，提供SentryIO报告目录。先在AE0下配对恢复native101/panel102，保留dump、102二进制/PDB和候选前端。异常为AfterFXLib+0x193d9ab处空指针读取；不从原始栈扫描推断已证明的插件调用栈。源码发现新增PF_PUI_TOPIC的Emitter/Force/Material未注册Effect UI（Model需检查其作者注册），这是确定的SDK契约缺陷；是否即本次根因仍待宿主复测。本卡拥有NodeEffects.cpp/PluginVersion.h、必要定位绘制防御、注册回归及当前ADR/状态/测试文档，不改Core、参数身份或共享外观求值。先增加实际register_ui回调断言，复现缺失后最小修正，再完整构建；不得把编译通过当作崩溃已经消失。

已确认Model同样缺注册，补齐上述四种setup，既有其他节点注册保留。新增实际回调断言在修复前失败，修正后四类节点注册/空绘制引用回归及完整构建通过。AE0下发布native103/panel103并核验完整配对/回滚，仍以native101/panel102为最近回退。dump源未改、没有启动/停止AE；原工程图层选择复测仍待owner报告，不能关闭崩溃门。精确证据与命令见current-state.md。

## P-08 — Auxiliary 身份与共享 Particle 外观回连（已部署，待 AE 实测）

Owner 于2026-10-23提供截图并确认 Particle ↔ Auxiliary 表示主粒子产生一代子粒子、复用同一 Particle 外观，不是递归繁殖。本卡拥有原生 Emitter 显示、gateway 创建命名、graph_edits/graph_view、Core Graph 校验与静态/时间 Auxiliary 父图求值、对应测试、ADR0026与当前文档。仅允许直接互连的 Particle→Auxiliary 父引用打破拓扑依赖；父图排除该 Auxiliary 的输出，维持单代、预算、取消、稳定身份及既有参数/ABI。其他循环继续拒绝。保留官网及其他在途修改。

Owner追加参考选项截图与点击节点定位线需求。本卡同时拥有 NodeLocatorUI.hpp、StarfieldHost.cpp、NodeEffects及flags/PiPL版本/构建输入和前端generation。在首个可见参数标题绘制蓝青定位线，通过既有Reveal命令记录会话UUID/layer，不新增工程参数；Output现有自绘不在本次变化中。参考中未实现的控件不加空选项。宿主实测、折叠/重开与定位宽度不能由编译结论代替。

Owner随后确认关闭AE并要求发布。AE0预检、冻结候选一致性、完整native102/panel103配对部署和安装/备份哈希验证通过，最近回退native101/panel102；此前受进程守卫阻止的退役夹具补跑通过。实际AE效果仍待owner复测；部署收据、回滚命令和最近五版保留证据统一见current-state.md。

## P-07 — Emitter / Auxiliary 原生选项区分（已部署，待 AE 复测）

Owner 于 2026-10-19 指出两者原生选项完全相同，要求参考 Stardust 区分 Auxiliary 专属继承等设置。本卡拥有 ae_plugin/NodeEffects.{cpp,hpp}、NodeGraphSync.cpp、PluginVersion.h、NodeEffectFlags.h、NodeEmitterPiPL.r、NodeEffect.vcxproj、对应原生节点/fake-host/GPU 链接 stub 测试与本卡、ADR0026、当前状态/测试文档。保留 P-05/P-06、GOV-01A/B 及其他在途修改。

保持共享 Emitter AEX、参数 IDs/顺序/类型、源标记、图/schema、Core ABI9 与渲染算法。仅在普通 Emitter 隐藏 Emit Chance、Emit Life Start/End、Inherit Velocity/Size/Opacity/Color；Auxiliary 显示这七项。共用发射设置继续保留，不新增未实现的参考功能。动态隐藏沿用 AEGP HIDDEN；同步守卫必须在预设/事务写入期间暴露条件控件，结束后恢复。验证正常/辅助模式、守卫开关、重复刷新和资源释放，完整 AE2023 SDK 候选构建；AE 实测单独记录，部署遵守 ADR0011。

native101/panel102完整构建与离线回归通过；owner关闭AE并明确要求部署后，AE0预检、完整配对安装、哈希及回滚预检通过。最近回退为native100/panel102；证据与实际宿主待测项见current-state.md。按既有最近五版策略报告后仅裁剪一个最旧实际部署备份；不混入其他任务的源修改或替其关闭宿主门。

## P-06 — 拖动后的字符串快照校验溢出

Owner 于 2026-10-19 报告拖动节点后 `invalid_graph_snapshot: Maximum call stack size exceeded`，授权检查与必要小修。本卡拥有 web_panel/js/graph_transactions.js、tests/panel_graph_transaction_tests.js、相关前端缓存标记与当前文档；保留 P-05 与 GOV-01A 在途改动。检查确认客户端回执比较器把非空字符串当数组递归：Motion Starting With 非空时，即使宿主已提交位置，也会在回执确认阶段报错。修复须保持字符串精确且区分大小写，保留数值容差、宿主校验与现有提交流程；不改参数、schema、ABI 或渲染。离线回归覆盖空/单字符/多字符/中文前缀下的位置提交及真正的字符串不一致拒绝。AE 运行时不部署；实际宿主复测单独记录。

## P-05 — 整理原生面板菜单（已部署，等待 AE 复测）

Owner 要求窗口菜单只保留两个正式面板入口，内部作者命令隐藏，手动图修复移入面板 Diagnostics。拥有 StarfieldHost/WebPanelHost、会话命令绑定、共享 JSX 及候选传输、前端 generation、对应测试/构建/配对部署与当前文档。约束：命令处理逻辑、节点、参数身份、持久化、Core ABI9 和渲染保持。SDK 命令分配/注册与可见菜单分离；每次安全 idle 脚本执行前提供当前会话 ID，禁止依赖隐藏菜单的名称查找。native100/panel101 已构建、离线通过并配对部署，位于 codex/p05-panel-menu-cleanup；实际安装、回滚与待 owner 复测范围见 current-state.md。离线通过不代表 AE 菜单/预设实测通过；本轮尚未并入 main。

## P-04 — AEGP Panel + WebView2 已完成

旧 CEP 注册已在 owner 要求重发 UAC 后移出扫描目录；web_panel 迁移及已验证检查点已合入本地 main。源码/安装配对、精确注册路径、证据和恢复命令见 [current-state.md](current-state.md)。本卡不再存在待执行的管理员清理或 main 合并步骤。

Owner 已接受 native98/panel100 的节点画布、预设及模态保存退出复测，并要求退役旧 CEP、合入本地 main、将共享前端改为 web_panel、整理过时文档和仅保留最近几个回滚版。本次采用最近 5 个实际部署备份策略。

本次拥有范围：WebPanelHost/Protocol 与 Host 集成、前端传输和当前预设验证修复、路径迁移涉及的原生资源/构建/测试/部署/回滚工具、.vscode/、活跃文档、旧 CEP 精确注册退役及备份保留工具。后续配对及状态只在 [current-state.md](current-state.md) 维护。

约束：不改变节点行为、参数身份、schema、动画、Core ABI 或渲染；不恢复旧属性编辑器/Appearance。UI/JSX 保留 panel100 generation。native99 只为目录迁移重新构建。退役只操作本项目已核对 Junction；Windows 注册表、其他扩展与共享缓存不改动。

验收与授权分别记录：owner 的版本接受是 release decision；未逐项报告的 IME、硬件、headless、性能和所有节点排列不补记为实际通过。旧 CEP 退役需 Windows 管理员权限；实际未成功时不能标记完成。保持默认报告、显式行动和一条恢复命令。

## 后续任务

无默认的新功能实现指派。Motion 前缀修正后的实际轨迹、硬件/性能等专项证据按 [路线图](roadmap.md) 与 [行为验收](compatibility-matrix.md) 安排。MNT-01 的旧范围不再限制 owner 本轮明确授权的文档及旧备份整理。

旧文档中的多张重复 Active M3-20、旧暂停指令和逐版本交接已移入 [历史任务卡](history/2026-10-19/pre-web-panel/docs/agent-backlog.md)。它们是证据，不是当前 agent 的执行指令。保留预先存在的工程分析草稿、日志和无关工作树改动，不混入本次提交。

## GOV-01A — 技术债小修已实施；分支汇合已转GOV-01B

Owner 于2026-10-19授权直接修复小问题；架构级重做须另行批准。本卡仅修 CMake 遗漏的既有 Core 输入、Model ABI9/current 与 ABI8/7 兼容测试、聚合入口中已点名的七项离线 runner、runner 耗时及过时技术债描述。拥有 CMakeLists.txt、tests/model_transport_tests.cpp、tests/RunModelTransportTests.ps1、tests/RunAllTests.ps1、tests/RunPanelTests.cjs、tests/transform_transport_tests.cpp、tests/transform_graph_tests.cpp、tests/material_native_binding_tests.cpp、tests/model_graph_tests.cpp、src/core/TemporalEvaluation.hpp、docs/adr/0038-particle-model-geometry.md、docs/engineering/TECH_DEBT.md；本卡在此追加，不覆盖在途 P-05。

仅恢复已由 ADR0038 规定的粒子空帧不采样 Model/Material 规则，并保留 ADR0041 直接输出的现有采样；修正新接入夹具中的旧快照/出生身份和停放节点预期。不改变积分算法、SDK、参数/ABI/schema、宿主事务、dist 或安装状态。不重构构建体系、参数映射、求值/缓存边界。验证 clean CMake DLL/CTest、Model ABI 精确旧前缀、已接入的离线 runner 与面板 runner；完整注册 catalog/DOM 聚合和源码身份清单仍属于后续工作，不用局部通过关闭宿主门。证据保存 artifacts/gov-01a-20261019/。

小修验证见 docs/engineering/TECH_DEBT.md 和 artifacts/gov-01a-20261019/。该批发现的分支汇合失败及原提案保留为历史证据；owner后续已批准最小范围修复，另由GOV-01B实施，不将新结果回写为原GOV-01A已通过。

## GOV-01B — 最小范围修复并行Force汇合（已完成，未部署）

Owner批准按最小影响范围修复GOV-01A已复现的汇合问题。拥有src/core/GraphEvaluation.cpp、src/core/TemporalEvaluation.hpp、tests/transform_graph_tests.cpp、tests/turbulence_graph_tests.cpp、docs/adr/0032-transform-node.md、docs/engineering/TECH_DEBT.md及本卡；保留GOV-01A和P-06既有修改。

仅补齐单个Output入口下的同Particle祖先路线收集，复用既有Transform/Motion顺序校验；独立Output fan-out不合并，共享Force只执行一次，含糊链路返回invalid_request。线性路线保持原路径。既有积分、参数/ABI/schema、Render.hpp、宿主事务和安装保持。验证旧失败场景、共享祖先/插入顺序/不同Particle或Transform/Motion/Turbulence链拒绝、Auxiliary、预算/取消和完整CTest；证据在artifacts/gov-01b-20261019/。本卡不授权额外模块重组或发布。

GOV-01B已通过全新核心DLL构建、完整CTest、七项SDK/fake-host离线回归及预算/取消探针。共享收集器只对汇合启用唯一祖先集合；线性路线、积分及既有上下文校验保留。回执与本轮独立差异见 artifacts/gov-01b-20261019/receipt.md、review.diff。未修改或部署P-06前端，未启动AE，不据离线结果关闭实际宿主门。
