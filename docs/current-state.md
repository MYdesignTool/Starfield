# 当前工程状态

核对日期：2026-10-09。native59 / packed32827 / Core ABI7 / CEP59 已于08:33 +08:00成对部署，源码cb1048d15ff205b9f7a541ee7931c1460070b75d已推送。M3-13 唯一活动：按 owner 要求采用原生行内 Layer/Dark Side 选择器，并排查已有 Texture 引用时节点添加报 parameter31（无引用画布正常）。最小测试通过，宿主报错尚未复现，不能宣称已修复；本版包含精确类型/值诊断。M3-16 与 MNT-01 共享改动保留并排除于维护发布。

## 源码与安装

owner 已确认 native59 原生选择器可以显示；添加仍报 key31/type3/value44/kind number。该值合法，尚未确定拒绝原因。CEP60 候选使用明确分支选择图层/枚举/开关范围，保留严格校验并增加 reason/typeKind/max；不能将旧嵌套条件在 ExtendScript 中的解释假设当作确诊。CEP80检查、实机ID44完整 gateway参数重写/添加/失败回滚/纹理预设 Add/Replace与启动通过。仅 CEP 维护，native59/Core ABI7 保留。

候选 native59/packed32827/CEP59：普通 PF_LAYER、PF_PUI_NONE；既有 disk233/234、streams521/522、类型/绑定/schema/ABI7 保留。AE 管理选择和显示名称，正反面均经标准 USER_CHANGED；真实 AE 行为待验收。候选基于65c7245，位于 artifacts/prepared/m3-13-native59-panel59/source；控件266、绑定7773+相机12、Core Texture4322/CEP47、完整 gateway/启动及 May2023 /MT 构建通过。完整 gateway 的已有正反面引用添加/失败回滚通过，没有复现 parameter31；保留校验并扩展诊断。Source/Masks/Effects stage 的资源镜像契约仍开放。

native58 修复原生提交临时 PF 上下文缺失 effect_ref 的 owner 传递。原生7773+相机12检查、隔离 CEP 纹理47、完整事务与启动通过；May2023 /MT 构建通过。部署前 fresh process check 无 AE，七个 native/Core、十一项 CEP、runtime selector 与 exact57 配对回滚核对。候选 artifacts/prepared/m3-13-native58-panel58 保留，日志 m3-13-native58-*。

默认方向 Core 维护已于22:56 +08:00发布：冻结7861a2b源码，仅加入fe6498afbdc378cc0c91546429d11f63ae28df16的 SpriteGeometry Texture 相机轴/绕序修正；四角贴图在修复前第114项失败，修复后4322项与 Cloud2441项通过。CoreOnly /MT 构建通过，全部原 native58 adapter 输入字节哈希匹配。fresh process check 无 AE，通过 Deploy-TestBuild 安装；六个 AEX 与十一项 CEP 哈希不变，七个 native/Core、十一项 CEP、selector、回滚旧文件均已独立核对。实际方向待 owner 验收。候选位于 artifacts/prepared/m3-13-native58-texture-orientation/source；没有纳入未完成的 M3-16。

| 项目 | 源码候选 | 当前安装 |
| --- | --- | --- |
| 原生版本 | build59，packed32827；普通 Texture layer widgets | native59，packed32827 |
| Core ABI | 7 | 7 |
| CEP | panel60 | panel59（既有 live Junction） |
| Particle | total534；Cloud 追加528..533；binding5 | 同契约 |
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

当前 native59/CEP59 收据：artifacts/m3-13-native59-deploy-before.json、artifacts/m3-13-native59-deploy-after.json。发布前 fresh process checks 无 AfterFX/AfterFX_64；使用 tools/Deploy-TestBuild.ps1 和既有 native/CEP Junction。七个 native/Core、十一项 CEP、runtime selector、配对恢复只读 report 与 exact58旧文件全部核对。冻结 source 基于65c7245加本次修复；未混入 M3-16。原构建输出也保存于候选 baseline-build-output。当前 runtime selector：StarfieldCore-37EBF72166E6B2B6.dll。历史58初次/方向维护收据与备份保留。

| dist 文件 | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | 27FCB9E354CC6508EE2CB1DC7405C8493203B68088478861BD60F4377EF2F1AE |
| StarfieldEmitter.aex | 8747E3AC3E6E912E12568C01486E3B36AE48EB57FCF91B329A2D9158353A1BFF |
| StarfieldParticleNode.aex | 15A470C95A8BC11B5558344E7805503B590A48E6316719DF8935C6C3BBDB40C9 |
| StarfieldForce.aex | CC9BB4F6FFDF6DC9B9E6ADD36EB48C97E5F24DB6CAE40A16007DC61F4A831419 |
| StarfieldTransform.aex | F2B83F251D507499D0FD459F278862DBF7B4C56F19FF94C53E5D2E0C2049BCB5 |
| StarfieldHost.aex | 3F358ACD0AED7D62F48F5430D5DE07DCE327F1FBEE2EE562F977BBDD5DE5B14D |
| StarfieldCore.dll | 37EBF72166E6B2B64A4050084BF52E4CF695E0B06454C75CE79B77354BB93F7B |

本次 exact native58/CEP58（方向修复 CoreB509）备份：artifacts/disabled/m3-13-native59-panel59-native-layer-picker-20261009。原方向维护/57及历史备份保留。回滚报告：artifacts/m3-13-native59-rollback-report.log；发布日志：artifacts/m3-13-native59-deploy-wrapper.log。

AE 关闭后的单步回滚（仓库根目录）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'm3-13-native59-panel59-native-layer-picker-20261009' -Restore
```

未更改进程起停、注册表、Adobe 缓存、环境开关或 Junction。完整目标继续 active；未从源码/驱动测试声明新增 AE 宿主资格。

## 开放 gate

- M3-16 纯核心 Shift Seed/Birth Chance 策略已提交f83e20f；独立40985检查、0失败。图求值/历史/辅助发射/预算/身份工作树保留，作者未完成，不暴露 UI、不发布到 Core；ADR0037 规定后续完整迁移。当前暂缓以处理 M3-13 方向反馈。

- owner 确认 native58 Comp 2 可选择并显示；报告背面无效、空白标签、已有 Texture 引用时添加报 parameter31。native59/CEP59 原生行内选择器及精确诊断已发布，真实选择/正反面/添加待回复。完整事务测试未复现宿主参数31，保持开放；八种时间采样、撤销/保存重开及 Source/Masks/Effects gate 继续。
- native57/CEP57 Cloud 外观、动画、预设、撤销、保存重开及实际 AE GPU/shutter 行为待 owner 验收。既有宿主观察仅覆盖记录的 AE2023.5.0 Build52，不从编译扩展支持版本。
- Face/Model、Path/Shadow、Shift Seed/Birth Chance、Texture Source/Masks/Effects stage 等剩余 Particle 行为仍开放；PTF 按 owner 决定等待 Physics。
