# 当前工程状态

核对日期：2026-10-09。09:14 +08:00已部署 native60 / packed32828 / Core ABI7 / CEP61，冻结源码90b7a2bfe982eab1c11d3592a928ca84e12ab1ec已推送。M3-16作者/构建/部署完成，实际AE gate开放；当前活动实现卡M3-17 Model：几何4141、三角形6406、资源/pose2076及ABI8传输30项（含ASAN）通过，继续图/快照/粒子/作者接入，未暴露菜单或部署。owner已确认当前版本节点添加问题修复；背面采样及其余Texture gate保留。MNT-01 改动保留。

## 源码与安装

当前Model资源候选新增SFMG1 codec、唯一ID/64MiB总资源预检、粒子pose和RenderRequest自有数值资源。Core ABI8追加网格数组，并保留精确ABI7请求前缀，不读取其不存在的尾部；30项标准及ASAN检查通过。资源/pose2076和更新矩阵上限后的三角形6406复查通过，日志 artifacts/m3-17-model-*-current.log。图/shape4/snapshot8/native/CEP仍未接入；共享ABI变化要求完整配对构建，当前安装仍ABI7。

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
| 主效果 | manifest29，count755；命名/排序 ADR0035 | 同契约 |
| 节点 | Emitter、Auxiliary、Particle、Force、Transform、固定 Output | 同节点族 |

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

未更改进程起停、注册表、Adobe 缓存、环境开关或 Junction。完整目标继续 active；未从源码/驱动测试声明新增 AE 宿主资格。

## 开放 gate

- M3-17独立Model候选不在installed60/61中。ModelScene通过显式model-to-layer矩阵，处理近面/齐次图像/ROI裁剪、透视正确深度/UV、每模型四采样遮挡、一次覆盖率与四种叠加/HDR合成；6406检查通过。SFMG1资源/粒子pose2076及ABI8传输30（含ASAN）已接入数值契约；图/shape4/快照/作者、材质/法线/透明网格交叉排序仍开放。完整Model与Shape、Use Model(s)参考菜单待参考；不能将数值资源证据当作可用Model粒子类型。

- M3-16 已完成配对构建/部署。Shift Seed对整个源运动/随机外观的影响、Chance0/100/动画、多分支、Auxiliary、undo/reopen仍需AE2023 owner验收；源码数值证据不关闭宿主gate。M3-13反馈到达时优先处理。

- owner 确认 native58 Comp 2 可选择并显示，native59 原生选择器可显示；旧CEP59添加曾拒绝合法 ID44。CEP61包含CEP60明确范围分支；当前节点添加已由owner实机确认修复。背面、八种时间采样、撤销/保存重开及 Source/Masks/Effects gate 继续。
- native57/CEP57 Cloud 外观、动画、预设、撤销、保存重开及实际 AE GPU/shutter 行为待 owner 验收。既有宿主观察仅覆盖记录的 AE2023.5.0 Build52，不从编译扩展支持版本。
- Face/Model、Path/Shadow、Texture Source/Masks/Effects stage等剩余 Particle行为仍开放；出生控制实现的host gate保留，PTF按owner决定等待Physics。
