# 测试入口

2026-10-09 Model原生OBJ导入：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelImportTransactionTests.ps1 -Run`及追加`-Sanitize`，各1019项、0失败（含复用452控件项）。日志artifacts/m3-17-model-import-transaction-{tests,asan}.log。实际AEGP事务/host deep copies覆盖解析/取消前无写入、单组undo记录、递增revision/Source自动OBJ、Mesh/Revision/Source/guard逐项部分失败恢复、silent setter拒绝、图提交失败以及恢复失败独立诊断、修订号耗尽与守卫拒绝、句柄/值/流/suite释放。图提交使用fixture回调；UI文件对话框不由fixture驱动，仍需完整SDK编译和真实AE验收。原生模块/binding2809复查通过（m3-17-model-import-native-binding-tests.log）。首次编译误用SDK compare结构聚合赋值（首字段refcon），改为具名字段后通过；最终计数不沿用之前962项。

2026-10-09 Model资源桥完整SDK构建：源码eae10aecb11f0a50dae8d1a3605d6750dc6eccaa的git archive冻结于artifacts/prepared/m3-17-model-resource-bridge-eae10ae/source，`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -IncludeModelCandidate -NoDistPublish -NoRuntimePublish`，May2023 x64 Release /MT八目标通过。Main1012参数、SFMR1路由及SmartFX入口实际编译；日志artifacts/m3-17-model-resource-bridge-build.log，八输出/十八安装哈希分别m3-17-model-resource-bridge-{build,installed}-hashes.json。当前安装native60/CEP61未变；不从编译关闭AE资格或发布完整Model作者。

2026-10-09 Model资源桥：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelResourceBridgeTests.ps1 -Run`及追加`-Sanitize`，各922项、0失败（含复用ModelControls452）。覆盖真实镜像提交/读回、相同revision复用、尾部清理、部分写入失败与精确回滚、silent setter拒绝、PF checkout/checkin配对、损坏/取消、parked/负时间/零cap不捕获、shutter资源去重，以及释放全部fake宿主句柄后ABI8实际像素。日志artifacts/m3-17-model-resource-bridge-{tests,asan}.log。首次夹具遗漏optional Shape=Model而访问空vector、渲染结果struct_size和SequenceResult类型，修正后记录通过；检查中也修正了AEGP值先于流引用释放的顺序。原生绑定runner重新执行2809项通过（m3-17-model-resource-native-binding-tests.log）；独立artifacts/RunModelResourceLegacyRegression.ps1 -Run -Bindings追加必要Model源后，旧绑定8697/相机12通过（m3-17-model-resource-legacy-regression.log）。真实Main注册1012及SFMR1路由/SmartFX入口仍需完整SDK构建；没有Model AE验收或部署。

2026-10-09 Model mirror SDK构建：源码0d868eb35fa00ba50619460722cc9b9f504cd1bb冻结于artifacts/prepared/m3-17-model-mirror-0d868eb/source，`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -IncludeModelCandidate -NoDistPublish -NoRuntimePublish`完成May2023 x64 Release /MT八目标。main/node实际编译ModelMirrorParameter；日志artifacts/m3-17-model-mirror-build.log，八输出哈希m3-17-model-mirror-build-hashes.json。十八安装哈希匹配native60/CEP61，报告m3-17-model-mirror-installed-hashes.json。未调用main注册helper或部署，编译不关闭实际AE持久化gate。

2026-10-09 Model renderer mirror helper：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelMirrorTests.ps1 -Run`及追加`-Sanitize`，标准/ASAN各8021检查、0失败（总计含复用ModelControls452）。日志artifacts/m3-17-model-mirror-{tests,asan}.log。SFMR1空槽/有身份网格、每字节损坏和截断、UUID/revision/CRC、全部ARB selectors、deep copy/离散插值/保存/文本、256槽定义及注册边界失败、分配失败和解锁后取消均覆盖。Main注册调用/镜像事务/SmartFX仍待接入，不能以helper测试关闭AE undo/reopen gate。

2026-10-09 Model原生模块完整构建：7f5c7d8冻结于artifacts/prepared/m3-17-model-native-7f5c7d8/source，`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -IncludeModelCandidate -NoDistPublish -NoRuntimePublish`，May2023 x64 Release /MT全部八目标通过，包含实际Model AEX/PiPL。初次冻结SDK路径错误，随后ModelResources漏链接ParticleTransform.cpp，分别修正并重新冻结后通过；失败日志artifacts/m3-17-model-native-build-{sdk-path,link}-failed.log。成功日志/哈希/十八安装对照：artifacts/m3-17-model-native-{build.log,build-hashes.json,installed-hashes.json}，安装native60/CEP61、0不匹配。旧绑定8441及相机12回归通过，artifacts/RunModelLegacyBindingRegression.ps1沿用既有fixture并仅补mesh helper链接，未修改共享runner；日志artifacts/m3-17-model-legacy-binding-regression.log。候选未部署，不增加AE资格。

2026-10-09 Model原生模块/binding7：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelNativeBindingTests.ps1 -Run`及追加`-Sanitize`，各2809检查、0失败，另调用既有ModelControls452检查。日志artifacts/m3-17-model-native-binding-{tests,asan}.log。实际NodeEffects Model分支/95参数/ARB selector及flags、14别名checkout/checkin、v1..7兼容/Kind5版本限制、所有字段类型/截断/损坏/alias错误、UI OBJ深复制与边界、parked mesh以及Model -> Particle input3均覆盖。首次fixture遗漏Particle Random Limit的native默认1，错误落在stream303；补齐真实默认后通过。新增runner最初链接了无关MotionBlur源导致未满足camera/history依赖，移除该无关源后只构建必要范围。fake host不等于AE2023 UI/undo/reopen资格；资源作者完整前不部署。

2026-10-09 Model可编辑作者完整构建：源码c65d5f79728ae2340b3cd9dbfa37b5cbb46e2f4f冻结于artifacts/prepared/m3-17-model-controls-c65d5f7/source，`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -NoDistPublish -NoRuntimePublish`，May2023 x64 Release /MT全部七个目标通过，main AEX编译ModelControls.cpp。日志artifacts/m3-17-model-controls-native-build.log；七输出哈希artifacts/m3-17-model-controls-build-hashes.json，十八安装哈希artifacts/m3-17-model-controls-installed-hashes.json（native60/CEP61、0不匹配）。首次冻结archive的PowerShell参数构造错误未生成源码；修正后归档成功。默认沙盒拒绝SDK junction与构建临时drive映射，自动审批的workspace命令建立本地SDK引用并完成双NoPublish构建。没有Model部署或宿主资格新增。

2026-10-09 Model可编辑作者：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelGraphTests.ps1 -Run`以及追加`-Sanitize`均559项；标准最终复查artifacts/m3-17-model-author-graph-tests-current.log，ASAN日志artifacts/m3-17-model-author-graph-asan.log。Model资源2076与任意参数4915回归通过，日志artifacts/m3-17-model-author-{resource,parameter}-regression.log。新增pose保留Origin/Rotation/Scale/Flip/Center/Normalize，冲突matrix/坏bounds/source严格拒绝；数值方程仍需外观比对。

`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelControlsTests.ps1 -Run`及追加`-Sanitize`各通过452项、0失败，日志artifacts/m3-17-model-controls-{tests-current,asan}.log。local May2023 SDK实际PF定义、18项注册/每步失败清理、精确int32 revision、原生pose捕获、图往返/live Model求值、导入候选不修改原控件、损坏/取消/分配失败及parked mesh隔离均覆盖。第一次编译缺Param_Utils常量头和fixture误用graph API/Model组成员名，修正后通过；447项是扩展前计数，完整构建见上文。Helper在main AEX编译，不注册Model模块、资源binding或发布Particle菜单；fake host不等于AE2023持久化验收。

2026-10-09 Model持久化完整构建：a1751b9的git archive冻结于artifacts/prepared/m3-17-model-parameter-a1751b9/source，`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -NoDistPublish -NoRuntimePublish`，May2023 x64 Release /MT全部目标通过。main AEX编译ModelGeometryParameter；日志artifacts/m3-17-model-parameter-native-build.log。十八安装哈希仍匹配native60/CEP61（artifacts/m3-17-model-parameter-installed-hashes.json）；未注册/部署Model作者，不关闭AE gate。

2026-10-09 Model原生持久化helper：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelGeometryParameterTests.ps1 -Run`，及追加`-Sanitize`，各4915项、0失败。local May2023 SDK、fake handle callbacks；日志artifacts/m3-17-model-parameter-{tests,asan}-current.log。默认cube/导入OBJ的SFMG1句柄、全部ARB selectors、每字节损坏/截断拒绝、深复制、锁失败释放、解锁后取消、host/C++分配失败均覆盖。初次fixture误用ModelPosition.x；边界fixture给一个字符却声明允许的最大跨度，ASAN发现该夹具错误，修正为真正超上限长度并更正前缀长度9后通过。没有注册新Model控件或执行真实AE save/undo/reopen；不能关闭宿主gate。

2026-10-09 live Model完整构建：67fc063的git archive冻结于artifacts/prepared/m3-17-model-graph-67fc063/source，运行`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -NoDistPublish -NoRuntimePublish`，May2023 x64 Release /MT全部目标通过。日志artifacts/m3-17-model-graph-native-build.log；构建后十八安装哈希匹配native60/CEP61，报告artifacts/m3-17-model-graph-installed-hashes.json。没有发布候选、改写runtime selector或增加宿主资格。

2026-10-09 M3-17 live Model图：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelGraphTests.ps1 -Run`通过444项，日志artifacts/m3-17-model-graph-tests-current.log。覆盖多Model共享组/确定顺序、出生身份、当前帧/出生shape动画、parked/负时间/零cap、Auxiliary资源域、Force/Transform、C ABI实际像素、Linear shutter矩阵/索引/默认cube/资源变化/组预算及typed拒绝。新增fixture首次误用PortId和不存在的RenderRequest成员，修正后才通过。原生/CEP作者未接入，不从纯数值测试关闭Model宿主gate。

现有Texture4322（artifacts/m3-17-texture-live-model-regression.log）、Birth3348（artifacts/m3-17-birth-model-shutter-regression.log）及Model CPU2695（artifacts/m3-17-model-particle-live-graph-regression.log）回归通过。首次误写Birth runner名称未执行测试，随后使用正确的RunParticleBirthGraphTests.ps1执行并通过；失败日志已由实际运行日志替代。

同一Model图fixture追加`-Sanitize`执行MSVC AddressSanitizer，也通过444项；日志artifacts/m3-17-model-graph-asan-current.log。首次371/扩展440中间检查不作为最终候选计数。

2026-10-09 M3-17 snapshot/CPU完整构建：e12517d的git archive冻结于 artifacts/prepared/m3-17-model-particles-e12517d/source，`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -NoDistPublish -NoRuntimePublish`，May2023 x64 Release /MT全部目标通过。main AEX ModelGeometry/Resources与fingerprint接入；日志 artifacts/m3-17-model-particles-native-build.log。构建后十八安装哈希匹配native60/CEP61，报告 artifacts/m3-17-model-particles-installed-hashes.json。未接入live Model图/作者或部署；编译不关闭AE gate。

2026-10-09 M3-17 snapshot8/CPU：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelParticleTests.ps1 -Run`，及追加 `-Sanitize`，均2695项、0失败；日志 artifacts/m3-17-model-particle-{tests,asan}-current.log。默认cube/导入多边形真实像素、组内一次透明度、近面中心后方/ROI/PAR/downsample/Transform/primitive排序/四种transfer/8及16bpc/C ABI均覆盖。Model变长表全部截断、坏count/length/matrix/index、旧3..7、混合Cloud/Texture、资源缺失、取消/分配及全帧共享预算通过。首个fixture缺SequenceResult helper，首次数值fixture选边界像素作内部断言及漏填近裁剪相机identity homography；修正后才记录通过。ASAN linker/debug参数已移出响应文件，避免MSVC忽略PDB参数。

当前lease资源2076（artifacts/m3-17-model-resource-lease-current.log）、三角形6406（artifacts/m3-17-model-scene-lease-tests.log）、ABI前缀ASAN30（artifacts/m3-17-model-transport-snapshot8-asan.log）及既有Texture4322（artifacts/m3-17-texture-snapshot8-regression.log）复查通过。证据来自显式evaluated快照；live图/原生/CEP作者仍未接入，不能声明可用AE Model功能。

资源源码8220f25在 artifacts/prepared/m3-17-resources-8220f25/source 完整May2023 /MT构建通过（双NoPublish），日志 artifacts/m3-17-resources-native-build.log；十八安装哈希与native60/CEP61一致，报告 artifacts/m3-17-resources-installed-hashes.json。此freeze早于snapshot8/CPU改动。

2026-10-09 M3-17资源/pose：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelResourceTests.ps1 -Run`，MSVC /MT，2076项、0失败；日志 artifacts/m3-17-model-resource-tests-current.log。SFMG1每字节CRC/截断/头部/数值/索引拒绝、资源总预算/ID、分配/取消及Up Axis/Euler/2D/Transform/PAR/anchor/local pose均覆盖。

ABI8：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelTransportTests.ps1 -Run`，以及追加 `-Sanitize`，两者30项、0失败；日志 artifacts/m3-17-model-transport-{tests,asan}-current.log。ASAN运行精确ABI7字节分配，检查不访问新尾部；数组/总预算预检在数值解引用前执行。三角形6406当前复查通过，既有Texture4322在ABI8候选上回归通过（日志 artifacts/m3-17-texture-abi8-regression.log）。标准/ASAN仅测试数值传输，未接入Model graph/shape/snapshot/作者，不关闭AE gate；当前native60/ABI7/CEP61保持。

2026-10-09 M3-17完整构建：源码5d17802的git archive冻结于 artifacts/prepared/m3-17-triangle-5d17802/source，执行 `powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -NoDistPublish -NoRuntimePublish`，May2023 x64 Release /MT全部目标通过，包含新ModelGeometry/ModelScene。日志 artifacts/m3-17-triangle-native-build.log。构建后十八安装哈希仍符合native60/CEP61；本数值里程碑未发布或接入完整Model粒子/资源/作者，不从构建推断宿主资格。

2026-10-09 M3-17三角形：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelSceneTests.ps1 -Run`，MSVC /MT，6406检查、0失败；日志 artifacts/m3-17-model-scene-tests-current.log。覆盖透视深度/UV、近面/齐次horizon/ROI、每模型四采样去重/遮挡、反射/shear/downsample/PAR显式矩阵、四种transfer/HDR、预算预检、取消/分配失败/坏输入、仅触及目标像素及取消时丢弃部分staging。首次fixture误用了near_clip成员路径，随后取消阈值超过小cube实际轮询数；修正fixture后才记录通过。几何4141复查通过，日志 artifacts/m3-17-model-geometry-current-tests.log。纯数值API未接入粒子pose/资源/作者或wire，不能声明Model可在AE使用。

2026-10-09 Texture owner确认当前部署节点添加已修复。当前live CEP61运行texture_panel_tests.js（80）及panel_native_node_gateway_tests.js通过，日志 artifacts/m3-13-panel61-{texture,gateway}-recheck.log；七native与十一CEP安装哈希符合native60/CEP61收据。该owner反馈只关闭添加gate；背面及其他采样/阶段/持久化继续。

2026-10-09 M3-17几何：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelGeometryTests.ps1 -Run`，MSVC /MT，4141检查、0失败，日志 artifacts/m3-17-model-geometry-tests.log。覆盖cube闭合/绕序、OBJ四种角点索引/负索引、权重和UVW、凹多边形两种绕序/面积/稳定角点属性、引用边界、退化/相交/非平面拒绝、各项预算、取消和注入分配失败；平移与次正规有限尺度也通过。首次cube聚合初始化不符合MSVC，修正后才记录通过。测试只构建独立新几何代码，不修改安装或证明3D渲染/AE资格。

2026-10-09 M3-16完整构建：冻结90b7a2b源码 `artifacts/prepared/m3-16-native60-panel61/source` 中执行 `ae_plugin/BuildWindows.ps1 -NoDistPublish -NoRuntimePublish`，May2023 x64 Release /MT通过，日志 artifacts/m3-16-native60-build.log。09:14 +08:00无AE后发布native60/CEP61，十八个安装、十八个保存文件、前后selector独立核对，Restore只读report通过。收据 artifacts/m3-16-native60-deploy-{before,after}.json，构建/部署不替代实际AE2023新控件和Texture gate。

2026-10-09 M3-16 作者：`tests/RunCloudNativeSyncTests.ps1 -Run -Bindings`（8441+相机12）、同脚本 `-Run`（实际 Particle回调344）、`tests/RunTextureSelectorTests.ps1 -Run`（实际注册269）均0失败。覆盖整数seed极值/类型、chance端点/小数、原子activation/失败、绑定v1..5历史边界、动画值及独立常量chance证明。测试夹具的 CanVaryOverTime 已补充表达式资格，避免错误地把表达式声明为不能动画的流。日志 artifacts/m3-16-birth-{binding,callback,registration}-tests.log。

CEP候选测试进程设置 `STARFIELD_PANEL_ROOT=artifacts/prepared/m3-16-native60-panel61/cep_panel`：Node运行 particle_birth_panel_tests.js（51）、cloud_panel_tests.js（49）、texture_panel_tests.js（80）、panel_native_node_gateway_tests.js 和 panel_startup_tests.js，全部通过。覆盖 key40 的 signed32 wire、独立Cloud/Birth激活disk、旧图无字段、不改写投影、预设Add/Replace、整笔失败回滚及既有Texture引用拓扑编辑。日志 artifacts/m3-16-<测试名>.log。构建/真实AE2023资格单独记录。

2026-10-09 M3-16 图求值：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunParticleBirthGraphTests.ps1 -Run`，3348检查通过。覆盖种子对运动/随机颜色/寿命/尺寸/透明度/旋转的影响、旧分支迁移、概率子集与全局cap、出生时动画、Auxiliary、Force/Transform、shutter匹配、快照/图编解码及 work/cancel。日志 artifacts/m3-16-birth-graph-current-tests.log。此前新增测试的 Transform 键名错误已修正，不能将编译失败视为通过；native/CEP/真实 AE 接入仍开放。

2026-10-09 CEP60 范围分支维护：测试进程的 `STARFIELD_PANEL_ROOT` 指向 `artifacts/prepared/m3-13-native59-panel60/cep_panel`，Node 运行 texture_panel_tests.js（80通过）、panel_native_node_gateway_tests.js、panel_startup_tests.js。覆盖每项枚举/开关边界、坏类型/字符串/小数/负数/溢出拒绝；完整事务使用实机 ID44，覆盖已有引用参数重写、添加、失败回滚与纹理预设 Add/Replace。日志 artifacts/m3-13-panel60-{texture,gateway,startup}-tests.log。native59/Core 不变；这些浏览器/fake-host证据不能确认 ExtendScript 条件解释假设或关闭 owner 节点添加报错。

2026-10-09 native59/CEP59 行内 Texture 选择器：冻结候选 `artifacts/prepared/m3-13-native59-panel59/source` 中运行 `tests/RunTextureSelectorTests.ps1 -Run`（实际注册/EVENT/USER_CHANGED266，0失败）、`tests/RunCloudNativeSyncTests.ps1 -Run -Bindings`（7773+相机12，0失败）、`tests/RunParticleTextureTests.ps1 -Run`（4322通过）；Node 运行 texture_panel_tests.js（47）、panel_native_node_gateway_tests.js 和 panel_startup_tests.js 均通过。完整 gateway 覆盖不同正反面视频源引用后添加节点及失败回滚；尚未复现 owner 仅在已有 Texture 引用画布上的 parameter31 错误。候选保留严格校验并增加实际类型/值诊断。`ae_plugin/BuildWindows.ps1 -NoDistPublish -NoRuntimePublish` May2023 /MT 构建通过。日志在该候选 source/artifacts/m3-13-native59-*.log。AE2023 选择标签/背面/节点添加与阶段行为仍需真实宿主证据。

2026-10-08 Texture 默认方向维护：在冻结7861a2b候选 artifacts/prepared/m3-13-native58-texture-orientation/source 中运行 `powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunParticleTextureTests.ps1 -Run`（4322项，0失败）和 `tests/RunParticleCloudTests.ps1 -Run`（2441项，0失败）。四角贴图新增断言在修复前于第114项复现相机镜像；修复后覆盖行列方向、Z旋转、背面、Transform反射与锚点。该目录内 `ae_plugin/BuildWindows.ps1 -CoreOnly -NoDistPublish -NoRuntimePublish` 构建通过，全部 adapter 输入与 native58 原构建指纹精确匹配。日志为候选 artifacts/m3-13-texture-orientation-{before,after,cloud-regression,core-build}.log（before 在仓库 artifacts）。实际 AE2023 默认方向与其他采样/持久化仍需 owner 验收。

从仓库根目录运行。测试源码和夹具是行为覆盖，不按年龄删除；变更契约时同步断言，保留 malformed、rollback、identity、bounds 等保护。

2026-10-08 native58 Texture selection focused gate：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCloudNativeSyncTests.ps1 -Run -Bindings`（原生7773+相机12，0失败）。CEP 候选设置测试进程的 `STARFIELD_PANEL_ROOT=artifacts/prepared/m3-13-native58-panel58/cep_panel`，用 Node 执行 texture_panel_tests.js（47）、panel_native_node_gateway_tests.js 与 panel_startup_tests.js；三者通过。构建：`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -NoDistPublish -NoRuntimePublish`。日志 artifacts/m3-13-native58-*；owner 已确认 Comp 2 选择和纹理显示，实际方向/时间采样/undo/reopen 仍开放。

M3-16 纯出生策略：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunParticleBirthPolicyTests.ps1 -Run`，40985项、0失败；日志 artifacts/m3-16-birth-policy-tests.log。仅覆盖有限概率、signed32 seed wrap、稳定/嵌套筛选、反序及独立随机流；图求值/分支/历史/native/CEP/宿主接入仍未完成。

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunAllTests.ps1
# 可单独选择一组：
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunAllTests.ps1 -Group Panel
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunAllTests.ps1 -Group Cpp
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunAllTests.ps1 -Group Scripts
```

入口在失败后继续收集结果；任何失败最终非零退出。每组结果写入 artifacts/all-tests/results.json，运行另一组时此文件替换为该组结果；完整门禁使用默认 All。

| 范围 | 入口 | 内容/依赖 |
| --- | --- | --- |
| CEP | node tests/RunPanelTests.cjs | 自动发现全部 *_tests.js；Node.js；日志 artifacts/panel-tests |
| 主 Core | tests/RunCoreTests.ps1（无开关） | 纯 C++ 图/codec/模拟/渲染/ABI；MSVC |
| Focused Core | -CurrentNodes/-EmissionTimeline/-ParticleTransform/-TransformTransport/-TransformGraph/-TransformBinding | 节点、时间线、仿射/快照/图/表达式对应的数值覆盖 |
| SDK fake host | -Adapter/-RendererControls/-Gpu/-EmissionCache/-NativeSync/-HostBootstrap/-GradientEditor | 本地 May2023 SDK；Gpu/NodeEffects 还需本地 generated kernel 输入 |
| 原生节点 | -NodeEffects -NodeKind Emitter/Particle/Force/Transform | 四种当前候选；Appearance 已退役 |
| 发布/回滚 | *_tests.ps1（自动发现） | 假二进制和隔离 checkout/Junction，全部位于 artifacts，不修改当前安装 dist |
| Core loader 并发 | CoreLoaderTests.vcxproj / core_loader_tests.cpp | 独立运行，需要两个 Core DLL（兼容/不兼容）；不在 RunCoreTests 的单文件 scope 内 |

C++ runner 使用相对响应文件、C++20 /W4 /O2 /DNDEBUG /MT，不建立 subst drive。所有输出在 artifacts；release 下断言仍执行。Adapter 的 camera/history seam 明确拒绝 capture，该套件测试 registration/ARB/UI，真实 capture fake-host 在 NativeSync 中验证。
Core loader 的独立 executable 接收兼容 DLL 与不兼容 DLL 两个参数，把副本和选择文件写在自身 artifacts/loader-tests 输出旁；不能用未配对的安装变更来代替此检查。

## Cloud 最小范围（M3-15）

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunParticleCloudTests.ps1 -Run
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCloudGpuDriverTests.ps1 -Run
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCloudNativeSyncTests.ps1 -Run
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCloudNativeSyncTests.ps1 -Run -Bindings
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunTextureSelectorTests.ps1 -Run
# 发布前指向隔离候选，避免修改安装 CEP：
$env:STARFIELD_PANEL_ROOT=(Resolve-Path 'artifacts/prepared/m3-15-native57-panel57/cep_panel').Path
node tests/cloud_panel_tests.js
node tests/texture_panel_tests.js
node tests/panel_native_node_gateway_tests.js
```

Cloud native runner 默认只报告，加 -Run 才编译/执行；-Bindings 覆盖完整既有原生绑定夹具及 Cloud 迁移，默认范围覆盖实际 Particle USER_CHANGED 回调。驱动夹具使用私有 CUDA/OpenCL context；不启动 AE。原生/CEP/预设/动画的真实 AE2023 外观、撤销、保存重开 gate 仍需 owner 观察。最新日志和计数见 ADR0036 与 current-state，避免沿用历史 passing counts。

2026-10-07 整理修正 Emitter7/Particle7/Force3、插值数组元数据、editor_presets 依赖、Motion Blur fake controls、625 主索引、ARB disk ID 和回滚 bundle 清单。保留有效失败断言；真正的实现失败与 Junction/驱动/工具缺失分开记录，不能删除或跳过失败套件来宣称全绿。
本次最后结果见 [current-state.md](current-state.md) 与 artifacts/workspace-cleanup-20261007/清理记录.md。实际 AE、GPU 设备和 UI 验收继续按 compatibility-matrix，源测试不关闭宿主 gate。
