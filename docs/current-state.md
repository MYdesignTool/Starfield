# 当前工程状态

核对日期：2026-10-10。owner要求开始新的工作，完整目标恢复active。当前安装 native60 / packed32828 / Core ABI7 / CEP61，冻结部署源码90b7a2bfe982eab1c11d3592a928ca84e12ab1ec；M3-16作者/构建/部署完成，实际AE gate开放。当前M3-17已接入候选Host分块资产上传/排队与隔离CEP整笔prepare/commit；Particle Model菜单和部署前Modal/idle验收继续，未部署候选。owner已确认当前版本节点添加问题修复；背面采样及其余Texture gate保留。所有在途及MNT-01改动保留。

owner最新顺序：优先完善Particle；Motion首批限定Light Path、Circle、Look At，随后推进Turbulence。Motion截图及参考库存记录于docs/reference-motion-phases.md；其余Motion模式不属于首批交付。

## 源码与安装

### 恢复开发：Model整笔传输候选（2026-10-10）

独立Host命令只排队，idle以最多八项工作/25ms slice拉取plain ASCII描述和32KiB网格页；全部描述符、UUID及总64MiB预检先于payload读取，随后调用已有ModelGraphTransaction完整备份/单undo/SFMW执行器。Host在prepare/commit前重新定位project/comp/layer；取消检查是纯数值，不在拥有effect ref时执行脚本。标准及ASAN传输夹具通过（逐字节与调用计数分别8885968/8886148，0失败；executor为fake），artifacts/m3-17-model-transaction-host-{tests,asan}.log；首轮夹具SDK时间类型错误已修正并保留诊断。

隔离CEP接入有界upload/queue/read/release和真实gateway回调：网格恢复前只物化Model身份/layout/link，commit再写作者参数、验证原生图收据和作者记录；失败由原生整笔恢复，提交后cleanup/undo/通知问题保留committed并显示完成问题，不自动重放。预设Add/Replace重写Model资源UUID并携带停放网格；复制仅在需要新mesh时显式收集源资产。实际候选protocol/planner271检查、真实gateway DOM回调35检查通过（SFMW/外层native为模拟），日志m3-17-model-graph-{transport,gateway-dom}-tests.log。Model作者74、预设文件332、export588、Texture80/Cloud49/Birth51及完整gateway回归通过，m3-17-model-transaction-*.log；三项测试夹具API/typed array/异步队列问题已修正，失败诊断保留。

候选由Prepare工具重现于artifacts/prepared/m3-17-model-transactions-panel62-v2/cep_panel，新增tracked私有client/transport helpers及author patch，live CEP保持。此阶段冻结SDK构建待记录；此前10a1b6e的构建不代表新Host传输已经编译。后续完成Particle Model菜单/模型选择和Model部署前脚本/模态范围互斥、OBJ导入返回后的目标/作者/revision一致性验收，再完整配对发布。真实AE撤销/表达式身份/重开及传输时序仍开放；完整目标保持。

### 最新收尾交接（2026-10-10）

- 源码：10a1b6ee2cfb585d1e675c278a95bbca00fa461f已推送；完整备份与ModelGraphTransaction执行器、精确SFMW写入、隔离预设文件候选保留。标准/ASAN执行器各636209检查、共用备份458562检查通过；callbacks/generic为夹具，不代表真实AE应用成功。
- 构建：上述提交git archive冻结于artifacts/prepared/m3-17-model-graph-transaction-10a1b6e/source；IncludeModelCandidate、NoDistPublish、NoRuntimePublish的May2023 x64 Release /MT八目标全部产出，日志终止于NoRuntimePublish标记。日志artifacts/m3-17-model-graph-transaction-build.log，八输出哈希artifacts/m3-17-model-graph-transaction-build-hashes.json；未部署候选。
- 安装：2026-10-10 08:50 +08:00重新核对七native/Core、十一CEP，共十八项均匹配native60/CEP61收据，runtime selector及所选Core哈希匹配；报告artifacts/m3-17-model-graph-transaction-installed-hashes.json。下方既有备份和单步回滚保留。
- 风险：step_model_asset_host未包含在已部署native60中；PresetsUI、ModelImportUI、EditorPresetPicker的Modal/idle重入与导入后revision一致性为未复现的静态假设，按owner要求保留为Model部署前验收项，详见ADR0038和testing.md。
- 下次接续：Host有界资产上传/排队及真实CEP prepare/commit，完成Model Add/Replace/duplicate整笔事务与资源UUID映射；再完成Particle Model菜单、完整配对验证及上述部署前验收。随后继续Particle剩余选项、节点命名/排序、Motion的Light Path/Circle/Look At和Turbulence。当前暂停不缩减完整目标。

2026-10-10恢复M3-17：原生ModelGraphTransaction执行器已接入Host工程输入，先核对全部UUID/修订/边界/单mesh8MiB与总64MiB，再解码全部SFMG1，完成预检后才开始单个SDK undo组。完整备份覆盖prepare、逐Model UUID重新定位/私有SFMW和commit；任一阶段失败/取消/回调异常恢复整笔备份。资产自身rollback、整笔rollback、提交后cleanup和EndUndoGroup错误独立记录，后两者保留committed=true避免重复应用。标准/ASAN各636209项检查通过（含共用备份夹具/调用资源计数），共用备份458562回归通过，日志artifacts/m3-17-model-graph-transaction-*.log。prepare/commit和generic为fake callbacks，实际executor/backup/SFMG1参与测试；首轮夹具固定时间假设已修正，诊断保留。Host上传/排队、CEP prepare/commit回调和Model菜单仍待接通，本执行器没有独立用户入口；本阶段完整SDK冻结构建证据见上方最新收尾交接，未部署，安装60/61保持。

2026-10-10阶段收尾（owner要求暂停）：M3-17新增完整EffectGraphBackup helper，使用AEGP_DuplicateEffect保存主效果/节点的完整网格、动画、表达式与参数；guard2和临时UUID隔离备份，恢复名称/顺序/flags并最后释放主效果guard。删除、排序、身份和flags均读回核对；原生图编译、renderer定位、Model导出及隔离CEP库存排除备份。标准/ASAN helper各458562项fake-host检查，Host147760、实际Model模块7699（另控件452）、隔离gateway588/作者74/预设330与旧preset断言通过，日志artifacts/m3-17-effect-graph-backup-*；两次夹具编译错误和一次Model专用定位过滤遗漏已修正，失败日志保留。候选由Prepare工具重现到artifacts/prepared/m3-17-model-backup-panel62-fixed/cep_panel；live CEP和native60/CEP61保持。helper尚未接入整笔Host图事务，本阶段未做完整SDK冻结构建或AE宿主验证，不能视为Model预设应用已经完成。

恢复工作顺序：先完成Host资产导入和整笔Add/Replace/duplicate事务，再完成Particle的Model菜单/模型选择、配对SDK构建；Model部署前须处理并验收下面的脚本/模态范围重入和导入状态一致性gate。随后继续各节点Stardust命名/排序、剩余Particle类型与选项，以及Motion首批三模式和Turbulence，完整目标保留。当前所有在途源改动及MNT-01改动均保留。

2026-10-10只读部署核对：dist/StarfieldHost.aex的SHA256为8F17C43E7D2FA6658DBF2F732DF25CE069654A0249CAA4B62713B0DD84BFFFCF，与native60部署收据及90b7a2b冻结bundle完全一致；AE插件Junction仍指向dist。冻结StarfieldHost.cpp没有step_model_asset_host，该idle接入由后续3c10b5d加入，仅在未部署的Model候选中。owner提出的PresetsUI脚本模态重入、OBJ文件对话框、编辑器预设对话框及导入返回后author/revision一致性疑虑均列为Model部署前验收项（ADR0038），属于静态假设，未复现；本次仅核对和记录。

隔离预设文件阶段实现version3 Model资产codec、Save Current实际网格收集及Import分页读取；v1/v2保持兼容。独立32768字符页/128MiB+256KiB文本限额不改变普通请求上限；用户选择文件后临时文件读回验证、rename发布与失败恢复，清理失败保留路径。真实候选codec/client/gateway/按钮共330项检查和未修改的旧preset断言通过，日志artifacts/m3-17-model-preset-file-tests.log；Model74/export576/Texture80/Cloud49/Birth51及完整gateway事务回归通过，m3-17-model-preset-files-*.log。新File行为全部使用内存夹具，没有实际磁盘/AE资格。候选可由Prepare重现；完整Model Add/Replace仍在项目变更前明确拒绝，原生整笔事务/duplicate和菜单继续，未部署。

SFMW源码969dcf1已推送并git archive冻结于artifacts/prepared/m3-17-model-asset-write-969dcf1/source；显式IncludeModelCandidate/双NoPublish完整May2023 x64 Release /MT八目标通过，实际ModelAssetWrite编译。日志artifacts/m3-17-model-asset-write-build.log，八输出/十八安装哈希m3-17-model-asset-write-{build,installed}-hashes.json；0安装不匹配，native60/CEP61及runtime selector保持。此freeze不包含之后的隔离预设文件改动。

精确Model资产写入候选接入private SFMW/version1与实际Model generic分支：按预期UUID/Source/revision/guard核对后恢复Mesh、六项bounds及精确revision/Source；支持parked mesh和revision0默认cube。每次写入读回，部分失败、silent setter、取消均恢复原值，恢复失败独立诊断；不发布图、不建立独立undo、不写pose/UUID。标准/ASAN实际原生模块各7535检查通过（另复用控件452），日志artifacts/m3-17-model-asset-write-{tests,asan}.log；首次SDK非const setter/fixture编译及同Source silent期望错误已修正，失败日志保留。完整Host导入/整笔预设事务仍待接入，此seam不独立暴露；冻结SDK证据见上，安装60/61保持。

UI-idle桥源码3c10b5da15d01b96c505eb4909f595764eea6713已git archive冻结于artifacts/prepared/m3-17-model-asset-host-3c10b5d/source；显式IncludeModelCandidate/双NoPublish完整May2023 x64 Release /MT八目标通过，实际Host新模块编译。日志artifacts/m3-17-model-asset-host-build.log，八输出/十八安装哈希m3-17-model-asset-host-{build,installed}-hashes.json；0安装不匹配。此freeze早于SFMW写入候选，不包含后者，实际AE上下文/时序gate仍开放。

Model资源导出异步候选：Host菜单仅排队，UI idle在进入线程/重入守卫下核对project root/comp/layer/唯一renderer/Model UUID，再调用私有SFMX；每次最多八个32KiB数值页，不参与常规CEP轮询。隔离gateway保留有界plain session，核对revision/Source/guard/bounds，取消、过期和stale丢弃。标准/ASAN Host各147289检查，gateway联动576、SFMG1客户端/资源校验373通过；Model作者74、旧Texture80/Cloud49/Birth51及完整gateway事务回归通过。日志artifacts/m3-17-model-asset-host-*.log和m3-17-model-assets-tests.log。tools/candidates/model_assets.js与更新后的author patch由Prepare工具重现到artifacts/prepared/m3-17-model-assets-panel62-reproduced/cep_panel，未写live CEP。精确资产导入、完整预设文件IO/应用回滚和Model菜单继续；此Host完整冻结SDK证据见上；实际AE上下文/时序尚未验收。

此前导出seam源码9a441a8已git archive冻结于artifacts/prepared/m3-17-model-asset-export-9a441a8/source，显式IncludeModelCandidate/双NoPublish完整May2023 x64 Release /MT八目标通过，±0.5默认新增警告已消除。日志artifacts/m3-17-model-asset-export-build.log，八输出/十八安装哈希m3-17-model-asset-export-{build,installed}-hashes.json；0安装不匹配，native60/CEP61和runtime选择不变。此freeze不含上段新Host传输，不用它宣称新Host构建已通过。

Model数值网格导出seam接入原生Model PF_Cmd_COMPLETELY_GENERAL：private SFMX/version1、有界SFMG1 sink，校验UUID/Source/revision/guard/bounds并用模块自己的PF上下文复制网格，host值释放后调用sink。无项目写入、句柄或分配器跨模块传输；常规CEP轮询不调用它。标准/ASAN原生模块各3483项通过（含既有452控件项），日志artifacts/m3-17-model-asset-export-{tests,asan}.log；首次漏fixture plugin-id stub的链接失败及平面minZ>maxZ的错误期望已修复，失败日志保留。新源纳入node工程/fingerprint，±0.5默认改为精确float literal消除新增编译警告。完整冻结SDK/host idle及CEP异步桥、导入/回滚尚待完成，安装60/61不变。

作者边界/隔离CEP阶段源码857818b已推送；git archive冻结于artifacts/prepared/m3-17-model-author-857818b/source，显式IncludeModelCandidate/双NoPublish完整May2023 x64 Release /MT八目标通过。实际Model101参数及导入、主资源捕获编译；日志artifacts/m3-17-model-author-build.log，八输出/十八安装哈希m3-17-model-author-{build,installed}-hashes.json、0安装不匹配。构建有default double转PF_FpShort警告（边界默认±0.5可精确表示）；不增加AE宿主资格。当前native60/CEP61及runtime选择保持，继续网格预设资产传输。

Model作者边界候选追加六项常量stream95..100/disk1519..1524，总101；导入与失败恢复同时处理Mesh/Revision/Source/bounds。UI编译核对SFMG1与边界，六项physical controls不写入binding7，回放仍用synthetic19..24。标准/ASAN原生模块各3040项，导入事务各1559项通过（均含复用452控件项）；日志artifacts/m3-17-model-author-bounds-{native-tests,native-asan,import-tests,import-asan}.log。首次检查发现physical95误记录为binding字段，已修复并补六项边界不一致拒绝检查；完整冻结SDK检查待下阶段记录。

隔离CEP Model作者支持默认cube、Model输出1/Particle模型输入3、属性向量分量、原生普通参数读取和保留parked mesh的Cube/OBJ切换。74项作者检查、Texture80/Cloud49/Birth51与完整既有gateway事务回归通过；候选位于artifacts/prepared/m3-17-model-panel62-reproduced/cep_panel，未替换安装面板。tools/Prepare-ModelPanelCandidate.ps1默认报告，-Prepare在artifacts内复制已核对的CEP61基线并应用tools/candidates/model-panel-author.patch；不写live CEP。基线包含已有未提交面板维护，脚本以规范化文本哈希严格核对，其他checkout须先取得相同基线。下一步完成网格本体的有界预设导出/导入、复制/回滚及Particle Model菜单；现有native60/CEP61保持。

导入源码da76183c82f9584367561e2742f0ac76c411e397冻结于artifacts/prepared/m3-17-model-import-da76183/source，显式IncludeModelCandidate/双NoPublish完整May2023 x64 Release /MT八目标通过，包含ModelImportUI的Windows对话框与实际NodeEffects按钮路由。日志artifacts/m3-17-model-import-build.log，八输出/十八安装哈希m3-17-model-import-{build,installed}-hashes.json，安装native60/CEP61、0不匹配。此候选未部署，继续Particle Model菜单/隔离CEP和预设。

原生OBJ导入候选接入Import OBJ按钮：AE所属Windows对话框、有界文件读入、网格准备、guard94下Mesh/Revision/Source写入与读回、图提交和失败恢复。标准/ASAN导入事务夹具各1019项通过（含复用452控件项），包括每项部分写失败、silent setter、图提交失败和恢复失败诊断；原生模块/binding2809复查通过。日志artifacts/m3-17-model-import-transaction-{tests,asan}.log和m3-17-model-import-native-binding-tests.log。完整SDK证据见上条；文件对话框/实际撤销和重开尚未在AE验收。下一步Particle Model菜单、隔离CEP/预设作者；安装native60/CEP61保持。

资源桥源码eae10aecb11f0a50dae8d1a3605d6750dc6eccaa冻结于artifacts/prepared/m3-17-model-resource-bridge-eae10ae/source，显式IncludeModelCandidate/双NoPublish完整May2023 x64 Release /MT八目标通过。日志artifacts/m3-17-model-resource-bridge-build.log，八输出哈希m3-17-model-resource-bridge-build-hashes.json；十八安装哈希保持native60/CEP61、0不匹配（m3-17-model-resource-bridge-installed-hashes.json）。主1012参数/SFMR1路由/SmartFX入口实际编译，OBJ按钮及完整CEP作者继续，未部署Model候选。

资源桥阶段已接入manifest30主效果注册1012参数和SFMR1回调路由。NativeBindingTransaction安装、读回校验与失败回滚Model镜像；SmartFX只捕获实际Model粒子及快门样本使用的资源，复制为自有ABI8数值数组并混入GUID。标准/ASAN资源桥夹具各922项通过（含复用452控件检查），原生绑定2809、旧绑定8697及相机12通过。日志artifacts/m3-17-model-resource-bridge-{tests,asan}.log、m3-17-model-resource-native-binding-tests.log、m3-17-model-resource-legacy-regression.log；冻结SDK构建证据见上条。以下helper构建叙述为之前阶段证据。

镜像持久化源码0d868eb35fa00ba50619460722cc9b9f504cd1bb冻结于artifacts/prepared/m3-17-model-mirror-0d868eb/source；显式IncludeModelCandidate/双NoPublish完整May2023 x64 Release /MT八目标通过，main及五种node模块实际编译ModelMirrorParameter。日志artifacts/m3-17-model-mirror-build.log，八输出哈希artifacts/m3-17-model-mirror-build-hashes.json；十八安装哈希仍符合native60/CEP61，报告artifacts/m3-17-model-mirror-installed-hashes.json。注册helper未调用、未部署；下一步NativeBindingTransaction资源提交/回滚与SmartFX捕获，随后OBJ按钮/完整CEP作者。

最新资源候选增加ModelMirrorParameter：SFMR1以UUID/revision及CRC封装有界SFMG1，empty slot独立保存；所有ARB selectors、独立句柄所有权、注册失败回收和取消已实现。标准/ASAN各8021项检查通过（总计含复用ModelControls452），日志artifacts/m3-17-model-mirror-{tests,asan}.log。helper预留main755..1010/disk1900..2155和count1011/disk2200，当前main manifest29/755参数保持，尚未调用注册helper；完整Model镜像安装/rollback、SmartFX数值捕获及OBJ按钮事务仍待接入。helper已纳入main及node工程/fingerprint，未部署候选。

原生模块/binding7源码7f5c7d8已冻结于artifacts/prepared/m3-17-model-native-7f5c7d8/source。`BuildWindows.ps1 -IncludeModelCandidate -NoDistPublish -NoRuntimePublish`完成May2023 x64 Release /MT八个目标，包含实际StarfieldModel.aex；日志artifacts/m3-17-model-native-build.log，八输出哈希artifacts/m3-17-model-native-build-hashes.json。初次SDK路径与随后漏链接ParticleTransform.cpp均已修正后重新冻结；失败日志保留。旧绑定8441及相机12回归通过（artifacts/m3-17-model-legacy-binding-regression.log，独立artifacts脚本追加mesh helper，不改共享runner）。十八安装哈希仍符合native60/CEP61，报告artifacts/m3-17-model-native-installed-hashes.json。没有Model部署；继续OBJ导入/资源镜像及SmartFX、完整CEP。

最新候选接入独立Model AEX（kind5、matchName org.starfieldfx.node.model），实际注册18项作者控件及metadata，总参数95。private binding7只为含Model的图写入；14路pose动画别名及六项数值bounds、Source/revision常量保持宿主安全的回放。UI编译在AEGP值有效期复制/解码OBJ，Model输出映射Particle input3。标准/ASAN各2809检查通过，包含实际Model模块注册/ARB回调、绑定版本/坏字段拒绝、数值动画checkout/checkin、UI句柄所有权及parked mesh隔离。日志artifacts/m3-17-model-native-binding-{tests,asan}.log。此候选仅显式IncludeModelCandidate双NoPublish构建；Import按钮事务、主mesh镜像/SmartFX捕获及完整CEP尚未接入，当前安装60/61保持。

可编辑作者源码c65d5f79728ae2340b3cd9dbfa37b5cbb46e2f4f已推送并冻结于artifacts/prepared/m3-17-model-controls-c65d5f7/source。完整May2023 x64 Release /MT七个目标通过（双NoPublish），main AEX实际编译ModelControls.cpp；日志artifacts/m3-17-model-controls-native-build.log，输出哈希artifacts/m3-17-model-controls-build-hashes.json。构建后十八安装文件哈希仍匹配native60/CEP61，报告artifacts/m3-17-model-controls-installed-hashes.json。候选Model控件注册函数没有入口调用，kind5/binding/资源镜像及CEP尚未启用，下一步接入完整原生Model节点与资源提交/捕获。

ModelControls候选按ADR0038注册真实May2023控件定义，Source/Import OBJ/任意mesh/revision与偏移、角度、缩放、Flip/Center/Normalize共18项；新Model效果独立disk1501..1518已预留并纳入共享唯一性检查。控件读取转换为拥有的数值网格/pose及schema1可编辑keys4..13，不丢失作者值；OBJ可先准备新句柄，不修改原控件。Model kind5、AEX、菜单及binding迁移仍未启用；UI文件对话框/提交撤销、主效果资源镜像、SmartFX捕获和隔离CEP全链路继续。不是可用AE Model发布。

Model可编辑pose采用独立的percent scale、XYZ Euler、per-axis Flip、Center/Normalize方程；标准/ASAN各559项图检查，资源2076与ARB4915回归通过。原生控件标准/ASAN各452项检查通过，覆盖实际SDK定义/失败注册句柄所有权、原生值捕获、图往返及live求值、parked mesh隔离、OBJ候选失败和损坏拒绝。日志artifacts/m3-17-model-author-graph-tests-current.log、m3-17-model-author-graph-asan.log、m3-17-model-controls-{tests-current,asan}.log。首次控件编译缺SDK常量头及误用fixture成员名，修正后通过；没有真实AE持久化证据。

持久化源码a1751b9b761340e07f2b0003088e83d685c9bc88冻结于artifacts/prepared/m3-17-model-parameter-a1751b9/source，完整May2023 x64 Release /MT全部目标通过（双NoPublish）。main AEX实际编译ModelGeometryParameter.cpp，未注册Model作者；日志artifacts/m3-17-model-parameter-native-build.log。十八安装文件哈希仍匹配native60/CEP61，报告artifacts/m3-17-model-parameter-installed-hashes.json。下一步原生Model节点/资源捕获、Particle Shape菜单及隔离CEP完整作者。

Model网格原生持久化helper已实现：SFMG1有界ARB、独立句柄复制、保存/恢复、比较、离散插值、文本往返、取消/分配/损坏拒绝；标准及ASAN各4915项fake-host检查通过。句柄解锁后才解码与轮询取消。main AEX工程及adapter fingerprint纳入ModelGeometryParameter；未分配/注册Model控件IDs或效果，资源捕获/原生/CEP完整作者仍待接入。

live图源码67fc06376729c45991a40122dc8130506d5e4806冻结于artifacts/prepared/m3-17-model-graph-67fc063/source，完整May2023 x64 Release /MT全目标构建通过（双NoPublish），日志artifacts/m3-17-model-graph-native-build.log。adapter fingerprint纳入ModelEvaluation.hpp；十八安装文件哈希仍匹配native60/CEP61，报告artifacts/m3-17-model-graph-installed-hashes.json。候选未部署，下一步Model网格原生持久化与完整作者。

Model live图支持默认cube、多Model共享组、稳定Particle分区、Auxiliary独立组、当前帧动画元数据、Force/Transform路径及Linear/Subframe采样。标准及ASAN各444项检查通过；旧Texture4322、Birth3348及Model CPU2695回归通过。此前新增fixture误用PortId和不存在的RenderRequest成员，修正后才执行通过。原生Model资源捕获与原生/CEP/预设作者仍未接入，不发布纯数值候选；现有native60/CEP61保持。

snapshot/CPU源码e12517d99a1545694a02c99182fde7e893b0f736冻结于 artifacts/prepared/m3-17-model-particles-e12517d/source，完整May2023 x64 Release /MT全部目标通过（双NoPublish），日志 artifacts/m3-17-model-particles-native-build.log。main AEX接入snapshot数值验证的ModelGeometry/Resources，adapter fingerprint同步纳入。构建后十八安装哈希仍匹配native60/CEP61，报告 artifacts/m3-17-model-particles-installed-hashes.json。该数值候选未部署；下一步为live Model图/采样和完整作者。

当前Model候选在显式evaluated snapshot中支持shape4、snapshot8模型组和CPU实际像素；默认cube/导入多边形、三维pose、近裁剪、ROI/PAR/downsample、四种transfer及primitive共同排序均通过2695项标准与ASAN检查。一次验证的数值mesh lease供各粒子复用；同组成员先合并覆盖再应用一次opacity，frame共享输入三角形/采样预算。live图/原生/CEP作者未接入，未发布菜单或安装。现有Texture4322与资源2076/三角形6406回归通过；ABI8精确ABI7前缀ASAN30也通过。

资源源码8220f25d1cc95b1aa5700fb962b8e137d8adf418已推送，冻结于 artifacts/prepared/m3-17-resources-8220f25/source，完整May2023 /MT全目标构建通过（双NoPublish），日志 artifacts/m3-17-resources-native-build.log。构建后十八安装哈希匹配native60/CEP61，报告 artifacts/m3-17-resources-installed-hashes.json。本freeze早于snapshot/CPU里程碑；共享ABI与ParticleInstance变化均要求完整配对构建。

M3-17三角形源码里程碑5d178022aaf239e65426192b8d757103f2e01cd8已推送；其完整git archive冻结于 artifacts/prepared/m3-17-triangle-5d17802/source。May2023 x64 Release /MT全目标构建通过，包含ModelGeometry.cpp/ModelScene.cpp；命令使用-NoDistPublish -NoRuntimePublish，日志 artifacts/m3-17-triangle-native-build.log。构建后安装18项哈希再次符合native60/CEP61收据。该候选未接入资源/粒子作者，不发布纯数值Model里程碑，不从编译关闭AE gate。

owner 已确认 native59 原生选择器可以显示；旧CEP59添加曾报 key31/type3/value44/kind number。CEP60维护使用明确范围分支，CEP61保留严格校验及 reason/typeKind/max诊断。2026-10-09 owner确认当前版本“无法添加节点”已修复；关闭该添加gate，背面/八种时间采样/阶段/持久化仍开放。不能将旧嵌套条件的引擎解释假设当作已确诊的根因。当前18项安装哈希与收据一致；ID44完整gateway/Texture80复查通过。

native60/CEP61实现 Shift Seed/Birth Chance，streams534/535/536、disk243/244/245、binding6，Particle total537；graph7/envelope1/snapshot7/Core ABI7保留。原生激活默认0保存旧分支，明确出生编辑才启用；新CEP节点写显式defaults0/100。完整源码冻结于 artifacts/prepared/m3-16-native60-panel61/source，CEP候选隔离至发布。必要检查：图求值3348；绑定8441+相机12、回调344、注册269；Birth51/Cloud49/Texture80、完整gateway/启动全部通过。日志 artifacts/m3-16-*。

候选 native59/packed32827/CEP59：普通 PF_LAYER、PF_PUI_NONE；既有 disk233/234、streams521/522、类型/绑定/schema/ABI7 保留。AE 管理选择和显示名称，正反面均经标准 USER_CHANGED；真实 AE 行为待验收。候选基于65c7245，位于 artifacts/prepared/m3-13-native59-panel59/source；控件266、绑定7773+相机12、Core Texture4322/CEP47、完整 gateway/启动及 May2023 /MT 构建通过。完整 gateway 的已有正反面引用添加/失败回滚通过，没有复现 parameter31；保留校验并扩展诊断。Source/Masks/Effects stage 的资源镜像契约仍开放。

native58 修复原生提交临时 PF 上下文缺失 effect_ref 的 owner 传递。原生7773+相机12检查、隔离 CEP 纹理47、完整事务与启动通过；May2023 /MT 构建通过。部署前 fresh process check 无 AE，七个 native/Core、十一项 CEP、runtime selector 与 exact57 配对回滚核对。候选 artifacts/prepared/m3-13-native58-panel58 保留，日志 m3-13-native58-*。

默认方向 Core 维护已于22:56 +08:00发布：冻结7861a2b源码，仅加入fe6498afbdc378cc0c91546429d11f63ae28df16的 SpriteGeometry Texture 相机轴/绕序修正；四角贴图在修复前第114项失败，修复后4322项与 Cloud2441项通过。CoreOnly /MT 构建通过，全部原 native58 adapter 输入字节哈希匹配。fresh process check 无 AE，通过 Deploy-TestBuild 安装；六个 AEX 与十一项 CEP 哈希不变，七个 native/Core、十一项 CEP、selector、回滚旧文件均已独立核对。实际方向待 owner 验收。候选位于 artifacts/prepared/m3-13-native58-texture-orientation/source；没有纳入未完成的 M3-16。

| 项目 | 源码候选 | 当前安装 |
| --- | --- | --- |
| 原生版本 | build60，packed32828；出生控制追加 | native60，packed32828 |
| Core ABI | 8（Model资源候选；接受ABI7前缀） | 7 |
| CEP | panel61 | panel61（既有 live Junction） |
| Particle | total537；Birth 追加534..536；binding6 | 同契约 |
| 主效果 | manifest30，count1012；Model资源镜像追加 | manifest29，count755；命名/排序 ADR0035 |
| 节点 | Emitter、Auxiliary、Particle、Force、Transform、Model候选、固定 Output | Model尚未部署；其余同节点族 |

Appearance 已退出当前注册表、构建与部署。MFR/Compute Cache 未启用。GPU F32 逐设备/逐帧协商；实际驱动的数值检查不等于 AE 宿主资格。

## Cloud 实现

Cloud 是一个逻辑粒子的圆群。Circles/Aspect/Density 默认10/150/66，Density0..1000；0 重合为一个圆，1000 增大成员中心散布。Circles1..1000、Aspect1..1000 是待参考证据确认的独立范围。确切随机分布和缩放仍需外观比对。

optional keys37..39、snapshot7/ABI7；旧图缺少 Cloud keys 时保留固定五圆。原生 hidden constant activation533/disk242 默认0；明确 Shape=Cloud 或修改 Cloud 参数才激活。新 CEP/Core 节点写入显式默认。既有 IDs、match names、node schema7/envelope1/序列契约保留。详见 ADR0036。

## Cloud 作者阶段既有证据

- 原生绑定7635 + 相机12，0失败；实际 Particle Cloud 回调277，0失败；实际控件注册/Texture selector276，0失败。
- Cloud CEP49、Texture CEP47 检查通过；完整 gateway/coordinator 的 Cloud 提交、失败回滚、预设 Add/Replace 通过。
- 完整 May2023 /MT 构建通过：artifacts/m3-15-cloud-authoring-build.log。
- Core 未变更于 f5b0844 的数值里程碑；其2441项 Cloud 核心和1116项私有 CUDA/OpenCL 驱动证据见 ADR0036。作者接入不替代真实 AE gate。
- 具体运行命令见 docs/testing.md；日志 artifacts/m3-15-cloud-native-sync.log、m3-15-cloud-callback-tests.log、m3-15-texture-cloud-selector-tests.log、m3-15-cloud-gateway-transactions.log。

## 当前安装证据与回滚

当前 native60/CEP61 收据：artifacts/m3-16-native60-deploy-before.json、artifacts/m3-16-native60-deploy-after.json；发布时刻2026-10-09T09:14:44.7668774+08:00。fresh无AE后经 tools/Deploy-TestBuild.ps1 与既有Junction发布完整配对；七个native/Core、十一项CEP、runtime selector和exact59/60回滚全部独立核对。冻结候选七项与CEP十一项哈希位于 native-bundle/candidate.json；adapter指纹来自同一冻结构建。当前 selector：StarfieldCore-777AC32B51AF471B.dll。历史58/59/CEP60收据、候选及所有备份保留。

| dist 文件 | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | E2FF723FD7DFF36BB27D435B88E4C455C151CF051C30157C449AD9B535CC0544 |
| StarfieldEmitter.aex | 3D8FFF07EACF8AAACC83022D65BFCB3287D04C50ABE1BC1D4683D2FCB937CAD0 |
| StarfieldParticleNode.aex | 706E7D4DA880B99C6DED08819EAAF070E243B4940F0E11D24A5F5C49C5515E96 |
| StarfieldForce.aex | D21ADF248C77F7C3951458C8580E1D059C353C8BD9C7B500B44E83F6F7166677 |
| StarfieldTransform.aex | 760517FB20C50AE8A340C6FE0801E1F2EA9B83DB70827F68005F29CB4047E281 |
| StarfieldHost.aex | 8F17C43E7D2FA6658DBF2F732DF25CE069654A0249CAA4B62713B0DD84BFFFCF |
| StarfieldCore.dll | 777AC32B51AF471B3DC22246B0C24D45AAC299327E44D19BE32A01AECB4D71AB |

本次 exact native59/CEP60 备份：artifacts/disabled/m3-16-native60-panel61-birth-controls-20261009。回滚报告：artifacts/m3-16-native60-rollback-report.log；发布日志：artifacts/m3-16-native60-deploy-wrapper.log。

AE 关闭后的单步回滚（仓库根目录）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'm3-16-native60-panel61-birth-controls-20261009' -Restore
```

未更改进程起停、注册表、Adobe 缓存、环境开关或 Junction。完整目标按owner最新要求恢复active；未从源码/驱动测试声明新增 AE 宿主资格。

## 开放 gate

- M3-17独立Model候选不在installed60/61中。snapshot8/shape4/live图接入CPU；可编辑pose图559及SDK控件452项标准/ASAN检查通过，2695项CPU既有回归保留。原生Model入口/资源绑定/CEP作者、材质/法线/透明网格交叉排序仍开放。完整Model与Shape、Use Model(s)参考菜单待参考；不能将数值像素/SDK helper证据当作可用AE Model粒子作者。

- M3-16 已完成配对构建/部署。Shift Seed对整个源运动/随机外观的影响、Chance0/100/动画、多分支、Auxiliary、undo/reopen仍需AE2023 owner验收；源码数值证据不关闭宿主gate。M3-13反馈到达时优先处理。

- owner 确认 native58 Comp 2 可选择并显示，native59 原生选择器可显示；旧CEP59添加曾拒绝合法 ID44。CEP61包含CEP60明确范围分支；当前节点添加已由owner实机确认修复。背面、八种时间采样、撤销/保存重开及 Source/Masks/Effects gate 继续。
- native57/CEP57 Cloud 外观、动画、预设、撤销、保存重开及实际 AE GPU/shutter 行为待 owner 验收。既有宿主观察仅覆盖记录的 AE2023.5.0 Build52，不从编译扩展支持版本。
- Face/Model、Path/Shadow、Texture Source/Masks/Effects stage等剩余 Particle行为仍开放；出生控制实现的host gate保留，PTF按owner决定等待Physics。
