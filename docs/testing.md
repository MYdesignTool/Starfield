# 测试入口

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
