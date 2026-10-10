# 测试入口

2026-10-10 `powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunMotionPathGraphTests.ps1 -Run`及追加`-Sanitize`，MSVC x64 /MT标准/ASAN各8288、实际exit0，artifacts/m3-18-motion-path-graph-{tests,asan}.log及exit.txt。实际mode0普通/历史graph/Auxiliary、SFMP1/6160B最大点包/every truncation/非法header/数字先验证再分配、动画speed/random/出生delay、四curve及metadata、30/60/120Hz partial interval、4096 clock cache淘汰后的lease、最终Euler/affine/reflection切线及串联Path/下游Look At、实际CPU像素/GPU准备/Linear与subframe、普通和历史32MiB累计capacity与20M曲线工作量边界、逐取消/全部分配失败和并发。候选明确数字单位/delay/clamp，不认定Reference作者、AE2023回调或GPU硬件。

相邻必要回归：MotionCircleGraph1772、MotionLookAtGraph1372、MotionPathTravel1216、RunCoreTests -TransformGraph594、ModelGraph559及ModelParticle2695，均实际exit0；artifacts/m3-18-path-graph-*-regression.log和各exit.txt。GraphEvaluation新CPP依赖已补到直接编译运行器，RunCoreTests原MNT内容保留，仅两条Motion source行归本卡。完整冻结SDK与部署前后证据仍分别记录，安装61/63保持。

2026-10-10 `powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunMotionPathTravelTests.ps1 -Run`及追加`-Sanitize`，MSVC x64 /MT标准/ASAN各1216、实际exit0，artifacts/m3-18-motion-path-travel-{tests,asan}.log与各exit.txt。实际共用曲线时钟和路径travel源码：四种插值分段独立quadrature、Circle时钟原值、life1e6最后1e-4秒稳定积分、明确delay/random sample、发射offset/身份/样式保留、反向/Limit To2D/Euler/shear/reflection、signed zero/退化路径、越界与缺basis失败原子性、全部compile/assignment分配失败、取消、1000乱序无分配及四线程只读。仅明确数字路径/延迟，不测Reference控件换算/末端政策/实际graph mode0/AE；全图总资源/工作预算须在接入时完成。

共享Clock提取后的必要相邻回归：`tests/RunMotionCircleGraphTests.ps1 -Run`1766、`tests/RunMotionLookAtGraphTests.ps1 -Run`1366，均实际exit0；artifacts/m3-18-path-clock-{circle,look-at}-regression.log及exit.txt。新Core源码/CMake/fingerprint接入；未部署Motion。

源码874f852c266ad82aac5d9072cf41a0a5ed360c03已推送/git archive冻结于artifacts/prepared/m3-18-motion-path-travel-874f852/source；完整May2023 x64 Release /MT八目标会话69978实际terminal exit0，显式IncludeModelCandidate及双NoPublish。artifacts/m3-18-motion-path-travel-sdk-build.log/exit.txt/build-hashes.json：Core实际编译MotionPathTravel.cpp，共享Circle新Clock进入Main/Core，八新产物分别读取实际SHA256。m3-18-motion-path-travel-installed-hashes.json确认26安装hash及selector/runtime匹配native61/CEP63。没有mode0图/原生/CEP入口或部署，编译不认定公开参考转换、实际AE或GPU资格。

2026-10-10 `powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunMotionLookAtGraphTests.ps1 -Run`及追加`-Sanitize`，MSVC x64 /MT标准与ASAN各1366检查通过，artifacts/m3-18-motion-look-at-{tests,asan}.log。实际普通/历史graph的数字goal/forward、当前时间采样与Over Life、零权重/重合/退化/反向、Euler/shear/reflection、串联Look At及Circle前置；5001个不同朝向无共享基底；CPU实际矩形像素、GPU场景实际inverse axes、Model矩阵Gram和中心、旧snapshot3..8精确往返/所有共享表迁移9、每个9截断/stride/reserved/坏q、SLERP与全部取消/分配失败。早期1298/1345/1355检查日志是扩展夹具前记录，最终记录1366；GPU准备不代表硬件资格，数字goal不代表参考目标筛选或AE。

源码da17da843a898c5108f9641cf38e2d59879763af已推送/git archive冻结，完整May2023 x64 Release /MT双NoPublish会话67021实际terminal exit0。审核服务首次额度失败拒绝后续读取，恢复后已读回wrapper日志/exit0并检查全部八新产物SHA256；artifacts/m3-18-motion-look-at-sdk-{build.log,build-exit.txt,build-hashes.json,terminal.json}。新26安装hash/Core selector/所选runtime均匹配native61/CEP63，m3-18-motion-look-at-installed-hashes.json。没有重启已结束构建、部署无作者入口的Motion，或由SDK认定AE/GPU硬件资格。Main/Core实际编入新姿态/header与snapshot9共享代码，旧Circle SDK证据不代替这次完整构建。

必要相邻回归：`tests/RunMotionCircleGraphTests.ps1 -Run`1766、`tests/RunCoreTests.ps1 -TransformGraph`594、`tests/RunModelGraphTests.ps1 -Run`559及`tests/RunModelParticleTests.ps1 -Run`2695通过，artifacts/m3-18-look-at-{circle,transform,model,model-particles}-regression.log，后三项实际exit0另存exit.txt；Circle夹具改为检查mode2缺goal/forward而非声称该模式未实现。扩展相机覆盖发现候选Circle/Cloud billboard分支丢掉pose三维轴，修正后最终标准/ASAN1366、实际exit0，各有exit.txt；相机夹具首次project函数名错误诊断保留于motion-look-at-camera-fixture-initial{,-asan}.log，修正为实际project_sprite后通过。snapshot9/共享渲染输入需冻结新源码进行完整May2023配对构建，之前Circle SDK构建不覆盖本次新代码；安装native61/CEP63保持。

2026-10-10 `powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunMotionCircleGraphTests.ps1 -Run`及追加`-Sanitize`，MSVC x64 /MT标准与ASAN各1764检查通过，artifacts/m3-18-motion-circle-graph-{tests,asan}.log。实际普通/历史Core graph及CPU renderer：曲线Linear/Draw/Hold/Bezier积分、独立quadrature/速度差分、animated rate与metadata exact clock、随机/逆序/串联、Force/Transform前置及未支持后置明确拒绝、Auxiliary出生位置/轨道速度继承、graph和snapshot往返、实际像素、Linear chord/Subframe arc、全部取消/分配失败及四线程请求。历史验证allocation_failed误报已修正。初始夹具错误Result bool/Force schema及诊断保留；不代表公开Speed单位/未知菜单、Light Path/Look At或AE2023资格。

必要相邻回归：`tests/RunCoreTests.ps1 -TransformGraph`594、`tests/RunModelGraphTests.ps1 -Run`559通过；日志m3-18-circle-{transform,model}-regression.log。MotionCircle是固定存储header evaluator，既有graph fixture source lists仍可编译；未改MNT在途测试脚本。

源码05c37e202b667a532c4a79e32457b5c559960754的git archive冻结源码执行`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -IncludeModelCandidate -NoDistPublish -NoRuntimePublish`，May2023 x64 Release /MT八目标实际exit0；artifacts/m3-18-motion-circle-sdk-build.log、build-exit.txt、build-hashes.json。GraphEvaluation的新Circle/header和Sampler接口实际编入Main/Core/shared候选；26安装hash/selector/versioned Core与native61/CEP63收据一致，m3-18-motion-circle-installed-hashes.json；没有部署未完成的Motion作者或认定AE资格。

2026-10-10 `powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunMotionPointCaptureTests.ps1 -Run`及追加`-Sanitize`，May2023/MSVC x64 /MT标准和ASAN各2365检查通过，artifacts/m3-18-motion-point-capture-{tests,asan}.log。真实MotionPointCapture、既有affine及Geometry代码，SDK函数为夹具，未操作AE。验证非零comp时间偏移/倍率/负时间/逆序采样、row/column矩阵、完整剪切反射/效果层逆变换/指定局部点、PAR/Z原点、同源及owner矩阵一次采样、稳定ID/comp匹配、全SDK读取错误、Acquire成功空suite/缺函数、Release错误、取消含释放后取消、输出原样保留、分配失败与256请求上限。SDK签名与矩阵定义按本地May2023头文件核对；作者/筛选/曲线/资源ABI及selector-thread和实际parented/animated AE行为仍开放。

新源4c9c50c781cb1b053320088c186e21193822e5ef推送后git archive冻结于artifacts/prepared/m3-18-motion-points-4c9c50c/source，`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -IncludeModelCandidate -NoDistPublish -NoRuntimePublish`完整May2023 x64 Release /MT八目标实际exit0；main实际编译MotionPointCapture，adapter fingerprint纳入新输入。日志artifacts/m3-18-motion-points-sdk-build.log、sdk-build-exit.txt和sdk-build-hashes.json。构建后26安装hash/selector及所选Core仍匹配native61/CEP63，m3-18-motion-points-installed-hashes.json。未部署未调用的seam，不代表实际Light/Null、shader/shutter、依赖缓存或AE qualification。

2026-10-10 Motion计算源码3d6d71898011732be648924b013cad427865ec8c已推送并git archive冻结于artifacts/prepared/m3-18-motion-geometry-3d6d718/source。`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -IncludeModelCandidate -NoDistPublish -NoRuntimePublish`完整May2023 x64 Release /MT八目标实际exit0，日志artifacts/m3-18-motion-geometry-sdk-build.log、sdk-build-exit.txt和sdk-build-hashes.json；Core工程实际编译新MotionGeometry。26安装文件、selector及所选Core构建后再次匹配native61/CEP63，m3-18-motion-geometry-installed-hashes.json。此计算层无Motion入口、未部署，不代表参考轨迹或实际AE2023验收。

2026-10-10 M3-18：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunMotionGeometryTests.ps1 -Run`及追加`-Sanitize`，MSVC x64 /MT标准与ASAN各4759检查通过，日志artifacts/m3-18-motion-geometry-{tests,asan}.log。实际MotionGeometry参与，覆盖零角/轴/正反旋转、极小多分量方向/正交矩阵、二次解析长度/三次独立Simpson长度、多knot span、重复点/静止切线、端点、1e9平移、数值边界、预算、取消、每次路径分配失败、1000乱序无分配查询及四线程只读。仅数值几何，未接入或验证Motion控件、灯光资源、Circle Speed单位、参考轨迹或AE。初次夹具quiet_nan命名冲突诊断保留于initial-tests.log；ASAN首次测试exit -1073741515源于离开vcvars子进程后找不到运行库，改在同一build子进程运行，未写用户PATH；/link多行响应文件选项警告修正为同一行，原诊断asan-runner-warning.log保留。完整SDK构建证据见上；安装native61/CEP63保持。

CEP63安装后复查：live `node --expose-internals tests/extendscript_syntax_tests.js`18、实际gateway旧入口90通过，m3-17-panel63-installed-{es3,legacy}-tests.log；Prepare empty delta从base63重现postrelease-panel63。发布工具增加nativeSourceCommit独立字段后，dummy配对发布/回滚再跑63检查通过，m3-17-panel63-deployment-final-tests.log；该额外字段只补来源身份，不改变实际已部署的26字节哈希。最终源码与安装对照见m3-17-panel63-postcommit-hashes.json。

2026-10-10 14:27+08 CEP63已部署：owner已关闭，fresh process0记录m3-17-panel63-predeploy-processes.json。Deploy-PanelTestBuild实际调用Deploy-TestBuild KeepNative，八native/Core/selector保持；26hash和十八CEP paired Restore默认report通过，m3-17-panel63-deploy-wrapper.log及m3-17-native61-panel63-es3-20261010-{before,after}.json。native冻结8d9b50e与CEP修正0e5cbc5分别记录，不重构建native、不调用AE、registry或cache操作。一步撤回61/62见current-state最新记录。live CEP语法与最终源码哈希复查另记；真实AE63加载/Model/OBJ/预设/撤销资格仍开放。

2026-10-10 CEP63语法修正候选：STARFIELD_PANEL_ROOT=artifacts/prepared/m3-17-native61-panel63-es3/cep_panel，`node --expose-internals tests/extendscript_syntax_tests.js`18检查/三个完整JSX通过；Node自带Acorn8.16.0以ecmaVersion3/allowReserved=never解析，无新增依赖或vendor。原CEP62的byte变量可静态复现保留字解析失败；owner已报告AE加载错误，修正后的真实AE执行待测。现代Node VM通过不等于ExtendScript兼容。`node tests/legacy_preset_file_tests.js`90检查，真实gateway两次重载及旧请求/坏generation/越界payload无modal/IO；旧catalog fixture专注codec/事务，文件写入在新文件fixture覆盖。

同候选Model文件583/asset gateway588/graph transport271/DOM35/作者74/Shape92/Texture80/Cloud49/Birth51、完整panel native gateway及startup通过，日志artifacts/m3-17-panel63-*.log。`powershell -NoProfile -ExecutionPolicy Bypass -File tests/panel_test_build_deployment_tests.ps1 -Run`62检查通过，artifact dummy native/CEP/Junction调用实际Deploy-PanelTestBuild/Deploy-TestBuild/Restore；验证报告无写、ES3错误/混合generation拒绝、保留Model八native、十八CEP安装/恢复/selector。fixture内仅模拟Get-Process查询，无实际进程操作或Adobe写入。初次Restore wrapper把中文绝对路径嵌入无BOM脚本造成乱码，改用PSScriptRoot及Stop并复查通过；首个诊断保留。首次ES3夹具allowReserved=false仍允许属性名default，改为never后通过；错误Shape测试文件名按实际model_particle_shape_tests.js重跑92通过。发布前默认report核对当前26hash成功，日志m3-17-panel63-deploy-report.log；实际部署另记。

2026-10-10源8d9b50e43b75562cfe4eda7501b9eb566c9f9744的冻结May2023 x64 Release /MT八目标实际exit0，命令`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -IncludeModelCandidate -NoDistPublish -NoRuntimePublish`，日志artifacts/m3-17-native61-model-build.log、build-exit.txt及artifacts/prepared/m3-17-native61-model-8d9b50e/native-bundle/candidate.json八native/十八CEP哈希。13:59 +08按owner要求部署native61/CoreABI8/CEP62；fresh AE process0，26源/安装hash、selector/versioned Core与paired rollback report通过。before/after：artifacts/m3-17-native61-deploy-{before,after}.json；新Host编译证据有效，仍不能当作实际AE脚本/原生窗口/OBJ/预设/undo/reopen资格。

Owner测试建议：① Particle Shape=Model在没有连接Model时显示默认cube；② 拖入Model并连接其output到Particle Model input，导入一个静态OBJ，核对origin/rotation/scale；③ 保存包含OBJ的预设，Add/Replace、复制及保存重开，网格和身份正常；④ 文件窗口取消及完成后继续编辑，撤销重做正常。六Shape只按参考、Face灰色不可用；实际Use Model(s)选项不推测。备份与单步回滚见current-state最新部署记录，完整目标未完成。

Prepare更新已发布CEP62 baseline并支持empty delta，重现artifacts/prepared/m3-17-postrelease-panel62；所有在途cleanup文件保持。PID30420于13:57:30只读确认不存在，不再待停止，记录artifacts/m3-17-native-preset-test-process-check.json。

2026-10-10 owner确认额度恢复并要求测试版，准备native61/CEP62完整配对。`powershell -NoProfile -ExecutionPolicy Bypass -File tests/model_deployment_tests.ps1 -Run`：60检查通过，实际Deploy-ModelTestBuild/Deploy-TestBuild/Restore-TestBuild在artifacts/deploy-test的dummy binaries/CEP与模拟Junction执行；覆盖read-only report、缺Model拒绝且无安装副作用、八native/十八CEP安装hash、已安装Model拒绝partial替换、缺失未复制新helper时paired rollback、旧七native/十三原有CEP还原、新Model/五helper消失和旧selector。日志artifacts/m3-17-native61-deployment-tests.log；首次预期失败被PowerShell Stop处理的夹具问题修复，首个fixture日志保留。没有对真实Adobe路径做测试写入。

STARFIELD_PANEL_ROOT=artifacts/prepared/m3-17-native61-panel62-test/cep_panel：gatewayBuild统一native-presets-62；Shape92/作者74/graph transport271/DOM35/文件583/export588/Texture80/Cloud49/Birth51全部通过，m3-17-native61-*-tests.log。完整SDK冻结构建继续，不把既有native60构建或fake部署当作实际AE资格。

源码dae9621已提交推送，但其后冻结命令因automatic approval review用量上限未执行（提示13:30）；后续完整SDK构建调用也未执行。没有新build session/退出码/产物，不把e5bf287历史构建当作新模块验证。下面的Host/chooser最小测试证据有效，完整May2023八目标仍待冻结编译；额度恢复后继续，不绕过审批。

2026-10-10 M3-17原生预设文件窗口候选：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunPresetFileHostTests.ps1 -Run`及`-Sanitize`各1596；actual Host、fake SDK/chooser与同线程message-only Windows pump，验证script不重入/SDK引用在窗口前全释放、ID claim/关闭后验证、suite/script/memory失败、stop/worker/重入、UTF16路径和丢失完成ack不重放。`tests/RunPresetFileChooserTests.ps1 -Run`及`-Sanitize`各242：actual Win32 wrapper替换系统API，验证NOCHANGEDIR、类型/单选/default extension、后缀/覆盖/取消/路径错误；未显示真实文件窗口或进行磁盘IO。日志artifacts/m3-17-native-preset-{host,chooser}-{tests,asan}.log；首轮夹具类型/unsigned/全局命名和UTF16源编码诊断修复。

`STARFIELD_PANEL_ROOT=artifacts/prepared/m3-17-native-preset-modal-panel62-v2/cep_panel`执行`node tests/model_preset_file_tests.js`：583 actual codec/client/gateway/UI检查通过，FS/native选择器均fake；脚本File dialogs/confirm一旦调用就失败，验证队列/claim/poll/retained terminal、迟到或丢失ack、release/expiry/identity变动无IO、原临时文件验证/备份/rename及rollback。Shape92、作者74/transport271/DOM35/export588/Texture80/Cloud49/Birth51回归通过；日志m3-17-native-preset-*-tests.log，tracked patch重现隔离CEP且live保持。初次UI夹具固定ID造成无界等待的PID30420按AGENTS停止授权仍待owner；后续drain最多1000步。完整新SDK冻结及实际AE2023模态/文件系统/整笔Model资源资格待验收，不能由这些检查关闭gate。

2026-10-10 e5bf28759dc4744d67e66cc7f2f1e2a4cd69d4a5的git archive冻结源码完整May2023 x64 Release /MT八目标实际exit0，命令`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -IncludeModelCandidate -NoDistPublish -NoRuntimePublish`；artifacts/m3-17-model-shape-build.log、build-exit.txt和build-hashes.json。实际编译新的Shape UI/picker到五种node AEX，并编译Main的显式数值转换；此证据不代替实际Win32菜单/Drawbot/AE拖动资格。11:21 +08:00十八安装文件、selector及所选Core与native60/CEP61收据全部匹配，m3-17-model-shape-installed-hashes.json。没有发布候选。

2026-10-10 Particle Shape候选：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunParticleShapeUiTests.ps1 -Run`及`-Sanitize`各112检查通过；实际NodeEffects Particle参数注册/EffectMain事件及ParticleShapeUI/转换/NativeEdit校验，独立resident token DLL，picker与publication为fake。覆盖六项顺序/Face禁用数据、Model6-Core4双向映射、invalid/fractional/Face拒绝、没有Host/占用/guard/取消/相同选择、不在DO_CLICK写入、DRAG只发布一次、过期值/失败完整参数恢复。没有显示真实Win32菜单或Drawbot/AE窗口。日志artifacts/m3-17-particle-shape-ui-{tests,asan}.log；首次PF矩形成员顺序、disk_ids命名空间及GPU stub缺失修复，失败日志保留。

`tests/RunModelNativeBindingTests.ps1 -Run`及ASAN：实际NativeNodeGraph/Model模块各9245、复用ModelControls452通过，加入native1/2/3/4/6到Core0/1/2/3/4、native0/5/7拒绝和所有SDK引用释放。日志artifacts/m3-17-particle-shape-native-{tests,asan}.log。

`STARFIELD_PANEL_ROOT=artifacts/prepared/m3-17-model-shapes-panel62-v3/cep_panel`运行`node tests/model_particle_shape_tests.js`：92项通过，真实隔离view、popup DOM构造和gateway（host controls fake），覆盖六项/Face禁用、Model6显示/Core4往返、native5拒绝、预检错误wire值无写入、codec与默认cube预设Add/Replace。CEP fixture最初误用不存在的disk别名，修复并保留m3-17-model-particle-shape-fixture-failed.log。共享Model作者74、graph transport271、gateway DOM35、preset file332、asset gateway588、Texture80、Cloud49、Birth51通过，m3-17-shape-*-tests.log。Prepare通过tracked delta重现新候选，patch空白context规范化后仍可应用。未部署；真实ECP drag/keyboard/timeline/动画/undo/reopen/渲染需要全Model配对后的AE2023验收。

2026-10-10 d1fa6e44f42015acd5274bf89ac2053e5fda8460已冻结于artifacts/prepared/m3-17-model-ui-protection-d1fa6e4/source；`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -IncludeModelCandidate -NoDistPublish -NoRuntimePublish`完整May2023 x64 Release /MT八目标产出，末尾NoRuntimePublish标记存在，无编译错误。日志artifacts/m3-17-model-ui-protection-build.log、八产物哈希m3-17-model-ui-protection-build-hashes.json；恢复上下文后终端会话已关闭，退出码无法重取，报告exitCode=null且列完成依据，不补造exit0。10:46 +08:00十八安装文件、selector及所选Core均匹配native60/CEP61；m3-17-model-ui-protection-installed-hashes.json。没有部署Model或实际AE资格。之后的六种Shape参考记录只是文档/元数据，不增加菜单实现。

2026-10-10 Model原生UI保护：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelUiExclusionTests.ps1 -Run`与追加`-Sanitize`各470检查通过；实际resident状态/已加载模块解析/token与独立DLL、message-only Windows同线程消息泵参与，覆盖缺失/不匹配Host、模态期间重复重入、错误token/旧token/非UI线程释放、异常与stop恢复。没有调用真实AE脚本/文件窗口。`tests/RunModelAssetHostTests.ps1 -Run`及ASAN各148867、`tests/RunModelTransactionHostTests.ps1 -Run`及ASAN8886869/8887089通过，覆盖实际idle模块范围占用/排队恢复/脚本内拒绝重入与既有资源验证；计数包含字节与调用，执行器fake。日志artifacts/m3-17-model-ui-exclusion-{tests,asan,export,export-asan,transaction,transaction-asan}.log。首轮C导出属性声明不一致诊断保留于linkage-failed.log。

`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelImportTransactionTests.ps1 -Run`和ASAN各20284检查通过（实际ImportTransaction/ImportCheckpoint/ARB helper，fake SDK，复用Model控件452）。覆盖全部98数字作者/metadata/bounds流变化、同revision不同mesh、project/comp/layer/UUID/time/flags、相等分数时间、suite与每一流读取失败、callback Source/revision过期、无undo/写入拒绝、正常导入及既有rollback；快照没有SDK引用或锁跨窗口。日志artifacts/m3-17-model-import-checkpoint-{tests,asan}.log，SDK名称/头文件首次诊断保留于sdk-type-failed.log。此源码冻结SDK构建证据见上方记录，真实AE模态及导入回调仍需验收。

2cbc74a Host传输冻结完整May2023 x64 Release /MT八目标exit0，双NoPublish；artifacts/m3-17-model-transaction-host-build.log及八输出/十八安装哈希m3-17-model-transaction-host-{build,installed}-hashes.json。10:13 +08:00安装与native60/CEP61收据0不匹配，selector/所选Core匹配；本证据不含之后UI保护/导入快照源码。

2026-10-10 M3-17恢复Host/CEP整笔传输：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelTransactionHostTests.ps1 -Run`和追加`-Sanitize`，May2023/MSVC /MT，8885968/8886148计数、0失败（包含逐字节核对和时限引起的idle调用次数变化，executor是fake）。覆盖所有描述/总64MiB在page复制前预检、单8MiB/63资产/64 IDs、顺序/长度/字节合法性、script handles/锁/suite释放、目标重新定位、线程/直接重入、取消/停止、prepare/commit失败及committed cleanup诊断；不是Modal/idle跨模块互斥或AE资格。日志artifacts/m3-17-model-transaction-host-{tests,asan}.log；首次夹具A_TimeMode误用已修正，time-type-failed日志保留。

Prepare生成m3-17-model-transactions-panel62-v2/cep_panel后，进程STARFIELD_PANEL_ROOT指向该隔离目录：`node tests/model_graph_transport_tests.js`271检查、`node tests/model_graph_gateway_dom_tests.js`35检查通过。实际client/helper/Model validators/graph planner以及真实gateway物化与提交回调参与；DOM、SFMW、外层备份/undo/原生收据为模拟。覆盖跨页、停放mesh、Add/Replace UUID映射、导出复制、guard2备份/索引组失效、恢复前暂缓参数、stale/坏页/超限拒绝、取消迟到回调清理、队列ack丢失、已发布但通知丢失、commit拒绝和作者读回。日志artifacts/m3-17-model-graph-{transport,gateway-dom}-tests.log，fixture-api/typed-array-fixture/scheduler-fixture失败诊断保留；不能从这些夹具关闭AE rollback/undo/expression/模态资格。

Model作者74、portable preset332、export gateway588、Texture80、Cloud49、Birth51以及完整panel_native_node_gateway_tests回归通过，日志artifacts/m3-17-model-transaction-<test>.js.log。候选Panel与Preset脚本node --check通过；不运行其他维护任务测试。完整新Host冻结SDK待记录，安装native60/CEP61保持。

2026-10-10阶段收尾：源码10a1b6ee2cfb585d1e675c278a95bbca00fa461f已推送，git archive冻结于artifacts/prepared/m3-17-model-graph-transaction-10a1b6e/source，执行`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -IncludeModelCandidate -NoDistPublish -NoRuntimePublish`。May2023 x64 Release /MT八目标全部产出，日志artifacts/m3-17-model-graph-transaction-build.log以NoRuntimePublish完成标记结束；实际Host编译ModelGraphTransaction及EffectGraphBackup。原进程会话已关闭，收尾以八个目标日志/文件及最终标记核对，不重新启动构建。八输出哈希artifacts/m3-17-model-graph-transaction-build-hashes.json；2026-10-10 08:50 +08:00十八安装哈希0不匹配、selector/runtime Core保持，artifacts/m3-17-model-graph-transaction-installed-hashes.json。本次未新增测试或部署；上述标准/ASAN证据仍有效，完整Host/CEP路由、AE2023模态/undo/持久化资格开放。按owner要求暂停，下次从current-state.md最新收尾交接继续。

2026-10-10 M3-17整笔Model执行器：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelGraphTransactionTests.ps1 -Run`及追加`-Sanitize`，MSVC /MT/May2023，标准/ASAN各636209检查、0失败（包含共用备份夹具和资源调用计数）。实际ModelGraphTransaction/EffectGraphBackup/SFMG1执行，prepare/commit/generic为fake callbacks，不能证明CEP或实际AE路由；覆盖多资产提交、default Cube reset、各阶段错误/异常/取消、第二资产失败恢复第一资产、精确动画/网格恢复、guard2/UUID重新定位、重复/不匹配/忙目标、预检坏CRC/修订/边界/身份、总64MiB拒绝在解引用前、silent revision、局部/全局恢复错误区分、发布后的cleanup/undo错误保留committed=true及undo/ref平衡。日志artifacts/m3-17-model-graph-transaction-{tests,asan}.log；首次fixture只允许时间0/1修正为同时接受事务图层时间12/24，失败日志保留。共用Backup单独458562检查回归通过。完整SDK冻结证据见上条；Host transport/CEP回调和实际AE模态/undo/持久化验收待续。

2026-10-10 M3-17备份helper：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunEffectGraphBackupTests.ps1 -Run`与追加`-Sanitize`，实际EffectGraphBackup.cpp/May2023 SDK，标准/ASAN各458562项检查、0失败（包含夹具调用/资源计数）。覆盖完整ARB/关键帧/表达式/Unicode名称、两种duplicate插入位置、第三方效果身份/顺序、精确失败恢复/RAII、已发布状态的备份清理失败、逐项元数据错误/静默写入、duplicate返回错误且带ref、静默delete/reorder拒绝、UUID冲突与Output身份排除，回调间无owned ref/value/handle/lock。日志artifacts/m3-17-effect-graph-backup-{tests,asan}.log；首次夹具与std::ref冲突修正，失败日志保留。单独Host导出147760检查、原生Model模块7699（控件452），隔离gateway588/作者74/预设330与旧preset断言通过，m3-17-effect-graph-backup-*；新增原生夹具字段名错误和Model专用定位漏过滤已修正。候选由Prepare重现。尚未接入整个Host图事务、未做本阶段完整SDK冻结构建；NodeGraphSync新renderer过滤的实机行为、完整事务/undo/表达式身份及下述模态gate仍开放。

## Model 模态/idle 部署前验收项（2026-10-10，未执行）

owner静态审查提出的风险尚未复现。已只读确认native60安装Host哈希匹配90b7a2b冻结bundle；step_model_asset_host的idle接入来自未部署的3c10b5d。因此这些验收项属于M3-17 Model候选，详细范围见ADR0038；本次没有运行测试或修改运行时代码。

- PresetsUI.cpp:53..55：在Model导出排队/活动时分别覆盖app.executeCommand和alert的模态入口，确认第一次AEGP_ExecuteScript返回前不会从idle发起第二次脚本调用；覆盖取消、错误和延期恢复。
- ModelImportUI.cpp:24与EditorPresetPicker.cpp:133：对话框消息循环中不得执行Model脚本；关闭/取消后安全恢复待处理工作。
- OBJ导入单独检查：对话框返回后重新确认目标和当前UUID/Source/revision/guard及相关作者状态；旧callback params或期间的状态变化不能导致过期revision提交、丢失更新或无法定位的间歇性失败。覆盖安全拒绝/一致提交和完整失败恢复。
- 同一UI线程与idle自身running标志只证明部分保护，不能作为上述脚本/模态范围互斥的验收证据。fake-host和SDK构建不能替代实际AE2023模态路径观察。

2026-10-09 portable Model preset：Prepare生成隔离候选后，将进程STARFIELD_PANEL_ROOT设为其cep_panel，执行`node tests/model_preset_file_tests.js`，330项、0失败，并将未修改的tests/preset_tests.js断言重定向至候选依赖通过。覆盖v1/2/3、active/parked/缺失/坏mesh、图身份/修订、超过256KiB文件、页序/限额/取消/过期/重载、UTF-16边界、覆盖原文件、部分/静默文件失败与rename/恢复/清理诊断，实际preset_manager的Save Current/Import与生成evalScript也执行。所有新增文件行为为内存fixture，没有磁盘或AE资格；日志artifacts/m3-17-model-preset-file-tests.log。首次UI fixture缺window.prompt已修复，失败日志m3-17-model-preset-file-ui-fixture-failed.log保留。

候选Model author74、export gateway576、Texture80、Cloud49、Birth51和完整native gateway事务通过，日志artifacts/m3-17-model-preset-files-<test>.log。tools/candidates中独立helper由Prepare复制，新增三项基线仍以规范化SHA256严格核对，live CEP保持。下一步Model整体Add/Replace/duplicate资产恢复；mesh apply当前明确拒绝，未从文件codec宣称完整预设应用。

969dcf1 SFMW候选冻结于artifacts/prepared/m3-17-model-asset-write-969dcf1/source，显式IncludeModelCandidate/双NoPublish完整May2023 x64 Release /MT八目标通过。日志/八输出/十八安装对照为artifacts/m3-17-model-asset-write-{build.log,build-hashes.json,installed-hashes.json}；0安装不匹配，native60/CEP61保持。此freeze早于隔离预设文件阶段。

2026-10-09 SFMW精确资产恢复：`powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunModelNativeBindingTests.ps1 -Run`及追加`-Sanitize`，各7535项、0失败，另复用控件452。日志artifacts/m3-17-model-asset-write-{tests,asan}.log。实际Model generic入口覆盖精确修订、parked/默认cube、参数/身份/边界/CRC拒绝、取消、每项部分写失败/silent setter/读回失败、最终guard释放失败和恢复失败诊断，以及全部值/句柄/流/suite回收。失败日志model-asset-write-{compile,same-source-fixture}-failed.log保留；同Source setter本来无可观察变化，fixture改为实际Source变化后验证拒绝，没有放宽断言。全preset/Host导入及真实AE gate仍待完成。

源码3c10b5d UI-idle export bridge独立冻结May2023 x64 Release /MT八目标通过，显式IncludeModelCandidate/双NoPublish；日志artifacts/m3-17-model-asset-host-build.log，八输出/十八安装哈希m3-17-model-asset-host-{build,installed}-hashes.json。0安装不匹配，native60/CEP61保持；此freeze不包含之后SFMW写入候选。

2026-10-09 Model UI-idle资源桥：`tests/RunModelAssetHostTests.ps1 -Run`及`-Sanitize`各147289通过；实际Host传输源、fake SDK suites覆盖空闲无script调用、非UI线程拒绝、project/comp/layer/唯一renderer/UUID、suite缺失、AEGP result/error句柄释放、页拒绝/取消、8MiB/256页、不完整generic ack/payload及错误。日志artifacts/m3-17-model-asset-host-{tests,asan}.log。首次漏include路径和fixture类型/标准库ADL命名冲突修复，失败日志model-asset-host-{include-failed,compile-failed}.log保留。Core共享参数头加入Host include，不新增Core链接依赖。

Prepare工具已重现隔离候选artifacts/prepared/m3-17-model-assets-panel62-reproduced/cep_panel。设置测试进程STARFIELD_PANEL_ROOT后，model_asset_gateway_tests.js576、model_panel_author_tests.js74、Texture80/Cloud49/Birth51、完整panel_native_node_gateway_tests.js通过；日志artifacts/m3-17-model-asset-host-<test>.log。前者使用实际gateway及真实客户端，核对gateway重载保留plain data、guard/stale/Source/parked OBJ/越界页/超时/释放，不读取CUSTOM_VALUE。最初缺renderer数字propertyIndex的fixture失败记录在gateway-{first-failed,carrier-failed}.log。`node tests/model_assets_tests.js`373通过，包括SFMG1头/CRC/数值/索引/退化三角形/引用点bounds、65536顶点分页、资产图映射、客户端取消/过期；日志m3-17-model-assets-tests.log。最初UInt8Array把256截断成0的错误fixture改为普通array，失败日志m3-17-model-assets-fixture-byte-failed.log保留。完整预设IO/导入恢复及实际AE gate未关闭。

此前9a441a8导出seam完整May2023 x64 Release /MT八目标冻结构建通过，命令BuildWindows.ps1显式IncludeModelCandidate/双NoPublish；日志m3-17-model-asset-export-build.log，八输出与十八安装哈希m3-17-model-asset-export-{build,installed}-hashes.json。native60/CEP61未变。此构建早于新UI-idle桥，后者独立冻结SDK证据见上；fake-host不能替代AE2023资格。

2026-10-09 Model asset export：RunModelNativeBindingTests.ps1标准/ASAN各3483通过（含既有452控件项）。日志artifacts/m3-17-model-asset-export-{tests,asan}.log。实际NodeEffects generic分支验证请求版本/类型、UUID/Source/revision/guard/bounds、revision0无资产、Cube下parked OBJ、无host stream/lock进入sink、SFMG1持有副本、render-only拒绝、开始及复制时取消、坏ARB/host失败/sink拒绝、零payload错误响应。首次缺fixture plugin-id stub的链接失败日志model-asset-export-link-failed.log；flat mesh minZ变大先违反bounds再进入stale，修正fixture精确error期待，失败日志model-asset-export-{bounds-expectation-failed,asan-bounds-expectation-failed}.log保留。最后标准/ASAN编译没有新增警告；完整SDK与真实AE generic上下文、CEP桥/预设持久化待验收。

2026-10-09 Model作者候选完整SDK：857818b的git archive冻结于artifacts/prepared/m3-17-model-author-857818b/source，`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -IncludeModelCandidate -NoDistPublish -NoRuntimePublish`，May2023 x64 Release /MT八目标通过。实际Model101及十项导入事务编译；default double→PF_FpShort警告保留（±0.5精确表示）。日志artifacts/m3-17-model-author-build.log，八输出哈希m3-17-model-author-build-hashes.json；十八安装对照m3-17-model-author-installed-hashes.json、0不匹配，未部署。隔离CEP按下面候选命令独立验证，不从SDK编译宣称AE支持。

2026-10-09 Model作者边界追加：RunModelNativeBindingTests.ps1标准/ASAN各3040项，RunModelImportTransactionTests.ps1标准/ASAN各1559项通过；日志artifacts/m3-17-model-author-bounds-{native-tests,native-asan,import-tests,import-asan}.log。物理95..100只核对导入边界，六项stale值拒绝，binding7 synthetic19..24保持。首次绑定检查因误记录95失败，修复后重新标准/ASAN通过。

隔离CEP候选最小命令：`powershell -NoProfile -ExecutionPolicy Bypass -File tools/Prepare-ModelPanelCandidate.ps1 -Prepare -DestinationName m3-17-model-panel62-reproduced`；目录已存在则选择新名字，不覆写。将进程内`STARFIELD_PANEL_ROOT`设为该目录下cep_panel，再运行`node tests/model_panel_author_tests.js`（74检查）。候选patch及基线哈希保存在tools/candidates，不写安装面板。Texture80、Cloud49、particle_birth_panel51与panel_native_node_gateway_tests的完整事务夹具也通过该候选；日志artifacts/m3-17-model-author-<test-file>.log。首次误写birth_panel_tests.js文件名导致MODULE_NOT_FOUND，按实际particle_birth_panel_tests.js重跑通过；这不是被跳过的实现失败。网格预设传输、完整菜单和实际AE gate仍待完成。

2026-10-09 Model原生OBJ完整SDK构建：源码da76183c82f9584367561e2742f0ac76c411e397冻结于artifacts/prepared/m3-17-model-import-da76183/source，`powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -IncludeModelCandidate -NoDistPublish -NoRuntimePublish`，May2023 x64 Release /MT八目标通过。ModelImportUI Windows对话框和实际NodeEffects路由编译；日志artifacts/m3-17-model-import-build.log，八输出/十八安装哈希m3-17-model-import-{build,installed}-hashes.json。当前native60/CEP61未变；菜单/完整CEP/预设及实际AE资格仍开放。

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
