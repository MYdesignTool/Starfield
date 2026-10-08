# 当前工程状态

核对日期：2026-10-08。当前 Cloud 作者候选为 native57 / packed32825 / Core ABI7 / CEP57；尚未发布。安装基线为 native56 / CEP56 / ABI6，来源 a086b9fe098e7f9b9553b02395f9379ff1e18f64。Core 数值里程碑 f5b0844 已推送，作者候选在本轮完成。MNT-01 共享改动保留并排除于实现提交。

## 源码与安装

| 项目 | 源码候选 | 当前安装 |
| --- | --- | --- |
| 原生版本 | build57，packed32825 | native56，packed32824 |
| Core ABI | 7 | 6 |
| CEP | panel57（隔离候选） | panel56（live Junction） |
| Particle | total534；Cloud 追加528..533；binding5 | total528；Texture520..527；binding4 |
| 主效果 | manifest29，count755；命名/排序 ADR0035 | 同契约 |
| 节点 | Emitter、Auxiliary、Particle、Force、Transform、固定 Output | 同节点族 |

Appearance 已退出当前注册表、构建与部署。MFR/Compute Cache 未启用。GPU F32 逐设备/逐帧协商；实际驱动的数值检查不等于 AE 宿主资格。

## Cloud 候选

Cloud 是一个逻辑粒子的圆群。Circles/Aspect/Density 默认10/150/66，Density0..1000；0 重合为一个圆，1000 增大成员中心散布。Circles1..1000、Aspect1..1000 是待参考证据确认的独立范围。确切随机分布和缩放仍需外观比对。

optional keys37..39、snapshot7/ABI7；旧图缺少 Cloud keys 时保留固定五圆。原生 hidden constant activation533/disk242 默认0；明确 Shape=Cloud 或修改 Cloud 参数才激活。新 CEP/Core 节点写入显式默认。既有 IDs、match names、node schema7/envelope1/序列契约保留。详见 ADR0036。

## 本轮验证

- 原生绑定7635 + 相机12，0失败；实际 Particle Cloud 回调277，0失败；实际控件注册/Texture selector276，0失败。
- Cloud CEP49、Texture CEP47 检查通过；完整 gateway/coordinator 的 Cloud 提交、失败回滚、预设 Add/Replace 通过。
- 完整 May2023 /MT 构建通过：artifacts/m3-15-cloud-authoring-build.log。
- Core 未变更于 f5b0844 的数值里程碑；其2441项 Cloud 核心和1116项私有 CUDA/OpenCL 驱动证据见 ADR0036。作者接入不替代真实 AE gate。
- 具体运行命令见 docs/testing.md；日志 artifacts/m3-15-cloud-native-sync.log、m3-15-cloud-callback-tests.log、m3-15-texture-cloud-selector-tests.log、m3-15-cloud-gateway-transactions.log。

## 当前安装证据与回滚

native56/CEP56 配对收据：artifacts/m3-13-native56-deploy-after.json。七个 native/Core 和十一项 CEP 文件已按收据只读核对。Runtime selector：StarfieldCore-037D48F4411A16E8.dll.

| dist 文件 | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | C119154D35C77D7954901F2E75A91398FE2BEA59A6325BCF2BB5EB03C631DF1D |
| StarfieldEmitter.aex | 8E8C3D2246C49D244EBE4DB3C27473C29EEABFF0232F0E7E1FD354836E230805 |
| StarfieldParticleNode.aex | 7491F39C381B686046CC93D0AB095D0903EB36BCBF406E14BF082A74B72B528E |
| StarfieldForce.aex | DC293664508BB6E25B27A89A6901C3DC6A8AAFDF6F71C0F4AFE6938D41255672 |
| StarfieldTransform.aex | 91E8642D2A87A809684273299BAADF1919FEA94C40C6A5F7E676C49AE0E7CE78 |
| StarfieldHost.aex | 9BA5A6E5BEAE8FC4E07B4AF0897FA972659607CBE6D752A11FE9ABCDAE8E0475 |
| StarfieldCore.dll | 037D48F4411A16E812A970BFEA6DDD604F1EF4CD9CA0ED4901F3F2FF848BDD6E |

native56 的 exact native55/CEP55 备份：artifacts/disabled/m3-13-native56-panel56-script-inventory-20261008。新的57成对发布将另保留 exact56；发布前需 fresh no-AE 检查，按 ADR0011 和 tools/Deploy-TestBuild.ps1 执行。进程起停、注册表/缓存/全局 host switches 均不在该授权中。

## 开放 gate

- owner 的 native55 Texture menu 仍只显示 None；native56 owner-pinned public layer inventory 已部署，实际新结果待确认。
- native57/CEP57 Cloud 外观、动画、预设、撤销、保存重开及实际 AE GPU/shutter 行为待 owner 验收。既有宿主观察仅覆盖记录的 AE2023.5.0 Build52，不从编译扩展支持版本。
- Face/Model、Path/Shadow、Shift Seed/Birth Chance、Texture Source/Masks/Effects stage 等剩余 Particle 行为仍开放；PTF 按 owner 决定等待 Physics。
