# 外部审查报告核实（2026-10-11）

对象：owner提供的 `D:/Starfield-项目审查报告-2026-10-11.md`。
报告中的建议是资料，不是清理、提交在途文件或操作宿主的授权。
核查起点：已部署native70/CEP73、冻结源码23bb594；工作树HEAD83c851a。
原有Motion/MNT修改保留，本次M3-17材质修复候选独立验证。

## 确认的优先问题

1. **P1-1 指纹漏项成立。** 解析三个AEX工程的ClCompile/ClInclude后，
   缺MotionGeometry.cpp、TransformNullUI.cpp、TransformLayerPicker.cpp，
   另有报告未列出的Host所编译的src/core/ModelScene.cpp。
   TransformNullUI.hpp/CurveEditorModel.hpp等传递依赖也需覆盖；
   只从ClCompile派生仍不充分。下一次CoreOnly前应修复。
   2026-10-12 M3-17已修复：BuildWindows合并三个AEX工程ClCompile/ClInclude，
   再覆盖adapter/Core所有本地h/hpp；冻结aa1ea34完整SDK通过，指纹确认包含
   上述六项漏项。native71已配对部署。聚合门和旧ABI断言仍是独立待办。
2. **P1-2 默认聚合门失败成立。** 实跑RunPanelTests.cjs：40套件、20失败。
   缺候选环境、缺--expose-internals及断言漂移均存在，main_order的30/29、
   palette缺Model/Material已复现。失败列表和原日志分别在
   artifacts/review-verification-20261011.json与artifacts/panel-tests。
3. **P1-3 ABI断言漂移成立。** model_transport_tests.cpp:21-22写死旧尺寸及ABI8；
   transform_transport_tests.cpp:252要求返回ABI4。应补当前ABI9验证，
   同时保留精确ABI7/8前缀兼容测试，不能简单把全部8替换成9。
4. **P1-6 干净克隆入口不完整成立。** 聚合runner/两个stub仍未跟踪，
   工作树RunCoreTests依赖它们。原有改动属于其他在途工作，需要独立任务
   核对所有权、依赖和冻结证据，不能盲目提交全部文件或删除日志。
5. **P1-4 当前部署日志重复成立，历史版本不全部是错误。** ADR0004限定M1 shell；
   ADR0032记载Transform首次ABI4迁移；ADR0029的implemented in build35是历史。
   应区分当前安装和决策生效时版本，不能把历史决策改写成今天的版本。

旧artifacts/all-tests/results.json不是native70专项发布门证据。专项冻结日志
另在artifacts/prepared/m3-17-native70-material。默认聚合门失败不能推出这些
独立命令未通过，也不能把专项通过称为全仓库全绿。

## 需要更正的结论

| 项 | 核实结果及证据 |
| --- | --- |
| P1-5 生成器漏更新Extension Version | 不成立：Prepare-ModelPanelCandidate.ps1已分别更新Bundle及两个Extension Version；实读CEP73匹配。部署工具此前漏校验Extension Version的子项成立，本次补XML检查及错代次拒绝测试。多处版本字面量是维护建议。 |
| P1-7 Model轮询/预设分页每次重载 | 不成立：panel.js中ModelGraph除begin之外和ModelAsset均可复用；preset_manager.js中PresetFile和Model轮询也可复用。普通图写入强制重载成立，保障同一host turn的跨页入口校验。未量测解析耗时，不能称为性能总根因。 |
| P1-8 不存在断言式宿主自动化 | 不成立：Run-ModelAcceptance.jsx含实际事务结果、资源回读、失败结果及临时项清理断言；ps1只是派发入口。缺实际AE成功结果、Check-NativeNodes未被文档发现成立。启动、派发、临时项目修改不属于只读操作。 |
| P2-24 CPU/GPU普通alpha<1公式不同 | 推导不成立：MotionBlur.cpp:227含original_alpha==0分支，straight是rgb/alpha；GPU乘adjusted/alpha再除adjusted，adjusted>0时同式。零alpha/零adjusted、舍入、格式与AE实际解释仍需专项像素验证，不能据此直接改公式。 |
| P2-26 每30ms全扫描且持锁比较proof | 部分不成立：StarfieldHost的30ms是idle建议间隔，next_scan限制全扫描500ms，另有64访问/4消息/250ms预算；NativeTemporalCache锁内复制、锁外比较。UI先取proof再比state可优化，耗时未测。 |
| P2-29 下溢为可达缺陷 | 未证明可达：session清零后descriptor只有length<=remaining才累加，保持total<=total_limit。显式防御检查有价值，报告未给出破坏不变量的入口。 |
| P2-31 PF_InData未填inter即错误 | 不成立于已查路径：context{}已明确置零，compiler通过AEGP普通stream读取；NativeAnimationPlan::sample另检查checkout/checkin。强制所有编译具备PF checkout可能重新破坏resident command。 |
| P2-34 PiPL没声明头依赖 | 不成立：Starfield.vcxproj:90-94已有Outputs和PluginFlags.h/PluginVersion.h AdditionalInputs。工具版本固定是另一个可移植性建议。 |
| P2-36 read_world全仓库无调用 | 不成立：tests/graph_parameter_tests.cpp有调用。生产当前不调用不等于死代码，不能删除测试需要的接口。 |

## 其余P2逐项处置

事实存在不等于已证用户故障；性能项没有宿主量测，不提前改实现。

| 项 | 核实 / 处置 |
| --- | --- |
| 1 | cleanup_host_traces确实调用不存在的Install-Plugin，是该卸载分支断链；当前Restore-TestBuild配对回滚不依赖它。 |
| 2 | 生成器确实写tracked editor_presets.js；重复生成可保持字节一致，不能断言每次弄脏。live Junction下生成仍应隔离。 |
| 3 | 默认私有路径存在；BuildWindows已有PythonPath/NvrtcPath/MSBuildPath覆盖参数，建议补自动发现。 |
| 4 | 无仓库CI成立，属工程化计划。 |
| 5 | 聚合Scripts不传-Run，报告型套件exit0，成立；应区分skipped并仅跑离线fixture。 |
| 6 | 聚合器未重置LASTEXITCODE/无catch，残值记录风险成立，未构造异常启动复现。 |
| 7 | 多个套件消费artifacts生成表达式/夹具成立；应完善生成顺序，不是所有输出都该跟踪。 |
| 8 | runner漏--expose-internals成立，两个MODULE_NOT_FOUND已复现。 |
| 9 | 实读34个runtime DLL及退役Appearance.pdb成立；保留策略须考虑已加载DLL和回滚引用，不执行清理。 |
| 10 | Clean-Workspace默认报告、-Clean删可再生成输出且无Restore成立；受MNT-01边界约束，不能把ADR0011宿主规则直接套成所有workspace清理违规。 |
| 11 | schema/Parameters两份数据存在；需补实际注册全目录比对，不能只凭无生成器名推断零覆盖。 |
| 12 | main_parameter_tests的755与Model mirror总数1012不符成立；不能把已部署注册改回旧布局。 |
| 13 | bootstrap槽1与launcher历史替换缺清晰标记成立；唯一性须区分逻辑ID、disk ID、当前stream，不宜删除已释放槽。 |
| 14 | CEP便携图64MiB与Core瞬时512MiB不同成立；AE保存/事务还有24KiB边界，不能简单扩大或统一所有上限。 |
| 15 | migrationPolicy散文含索引歧义成立；真实注册与ADR迁移记录应作为核对依据。 |
| 16 | 主效果显示名路径成立，本地化故障未复现；插件自注册名称不等于AE翻译名称。 |
| 17 | entries.push无列表去重/删除成立，同名可能合法；upsert/确认是产品选择，文件保存已有retained backup。 |
| 18 | 全局Delete处理成立，已有撤销及Output保护；额外确认是产品建议，owner未要求增加确认。 |
| 19 | manifest安装范围[23.0,99.9]成立，不等于各host已验证；资格范围仍仅AE2023。 |
| 20 | 重复JSON.stringify成立；adoptState有短路null保护，未证null崩溃路径。 |
| 21 | 拖拽重建path/监听器、几何读取存在，收益须量测后再做rAF/对象复用。 |
| 22 | 两套转义且panel未显式处理U+2028/U+2029成立，属需补ES3边界回归的风险。 |
| 23 | 请求262144字符与旧ADR64KiB不一致成立；UTF-16字符与UTF-8字节不同，需按传输种类分界。 |
| 25 | GPU callback逐次Acquire/Release存在，符合RAII；缓存suite需证明device/host生命周期，未证瓶颈。 |
| 27 | 多次checkout存在；AE无通用批量checkout接口，每次须对应checkin，耗时未测。 |
| 28 | arbitrary管道/setter重复存在，重构须保留各类型CRC/尺寸/所有权差异。 |
| 30 | 多个析构/释放不传播错误存在；报告故障及计数未注入验证，析构不可抛，应区分主错误与清理错误。 |
| 32 | 静态GPU能力与按device收紧同时存在，是协商机制，不凭此认定缺陷。 |
| 33 | 警告配置不同成立；SDK使WX不可用尚未证明。 |
| 35 | AEX/DLL共享部分Core代码存在，各有作者编译/数值渲染职责；需补指纹，不能简单去掉任一路。 |
| 37 | text/opaque缺更小单项上限存在，仍受全局约束；资源预算建议，未证越界。 |
| 38 | resize依赖前序同一不可变bytes的preflight存在，members已受限，冗余复查是防御建议。 |
| 39 | 窄化前有整体validate/尺寸检查，未证接受越界，新增assert可强化局部可读性。 |
| 40 | pow/map/重复计算存在，收益待量测；降低取消频率改变响应性，缓存不能只加graph_revision而漏动画依赖。 |
| 41 | 长TU/内部头存在，拆分属维护建议，不是功能缺陷。 |
| 42 | 快照结构读取重复存在，重构须保留旧3..10与11语义及截断检查。 |
| 43 | 重复计数存在；artifacts不跟踪是项目规则，应记录冻结源/重建命令，不把日志塞入源码。 |
| 44 | ADR体量/Status格式不统一存在，历史迁移证据不能覆盖删除。 |
| 45 | docs索引不完整存在，本次补本核实文档；全量整理属MNT。 |
| 46 | HAP-log未跟踪/未忽略存在，原文件保留待MNT。 |
| 47 | artifacts模式超过文档枚举存在；cleanup必须保留SDK、dist、prepared、disabled、worktree，不从过时表生成删除白名单。 |

## 本次材质修复的证据边界

owner的popup“0 out of range1..2”是真实AE反馈。旧writer已有value+1，
对合法默认0的Node模拟结果是1，尚未离线复现该弹窗的完整触发链，
不把推测写成已定位根因。候选改为显式0→1/1→2，拒绝超出0/1的Material
switch，检查native读回。旧通用uint32检查会放过4294967295，不应送入popup。
异常附实际尝试native value，便于定位宿主返回差异。

另一个确定缺陷已先失败后修复：graph RGB三项直接写AE，而AE要求RGBA四项。
真实注册目录模拟器现在强制popup范围/颜色维度。用默认材质进入HostPrepare
新建路径，再查独立颜色、三个百分比、AE Lights、已有材质、错误值不写入、
numeric key、旧wrapper及gateway重载。旧模拟器未施加这些AE约束，且直接
改了默认Color Source，遗漏实际新建路径。

本次已部署native70/CEP74补丁，native/Core保持原字节。发布事实见current-state。
专项离线通过不关闭AE灯光/视觉、undo/reopen宿主gate；不启动或调度AE。

补充核实：发布预检另发现recipe的文件hash已更新到CEP73而basePanel仍72。
这会导致候选声明错误基线，配对门已拒绝；本次将基线同步为73，并新增
Prepare阶段的源manifest/recipe一致性检查和“过期recipe不得创建候选”回归。
首次冻结审计缺忽略的GpuKernels.hpp，按冻结GPU源码重新生成NVRTC输入，
未修改生产源码或引用旧生成物后冒充新源码验证。

最终候选冻结于a3829ac，全部公开事务接口DOM矩阵、ES3、真实注册范围、
native70/ABI9及旧native61/ABI8配对回滚通过。新增fixture最初调用未公开的
api.read/api.write，已改走生产snapshot/ensure；失败归档保留，重新冻结并
独立核对486个tracked源码与27个面板文件。部署前fresh AE0，部署后全量
hash一致；这不改变上述“原popup完整触发链尚未复现”的证据边界。
