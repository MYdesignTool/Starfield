# 当前工程状态

核对日期：2026-10-08。native58 / packed32826 / Core ABI7 / CEP58 已成对部署，源码7861a2ba63cff245a0840539a3933da10b8077c0已推送。发布时间2026-10-08T22:33:57.4339046+08:00。owner 确认 Comp 2 选择及纹理显示正常，发现默认镜像；当前 M3-13 优先修复方向。M3-16 图求值工作树与 MNT-01 共享改动保留，排除于维护发布。

## 源码与安装

native58 修复原生提交临时 PF 上下文缺失 effect_ref 的 owner 传递。原生7773+相机12检查、隔离 CEP 纹理47、完整事务与启动通过；May2023 /MT 构建通过。部署前 fresh process check 无 AE，七个 native/Core、十一项 CEP、runtime selector 与 exact57 配对回滚核对。候选 artifacts/prepared/m3-13-native58-panel58 保留，日志 m3-13-native58-*。

当前方向候选：冻结7861a2b源码，仅加入 SpriteGeometry 的 Texture 相机轴/绕序修正；四角贴图在修复前第114项失败，修复后4322项与 Cloud2441项通过。CoreOnly /MT 构建通过，全部原 native58 adapter 输入字节哈希匹配。六个 AEX 与 CEP 不变，待 fresh no-AE 发布。候选位于 artifacts/prepared/m3-13-native58-texture-orientation/source，Core SHA256 B509D97495EEDF7BF23A2AC254419CEF00B69356C78AD5838D31DAA48C58F5D5；没有纳入未完成的 M3-16。

| 项目 | 源码候选 | 当前安装 |
| --- | --- | --- |
| 原生版本 | build58，packed32826；Core 方向维护候选 | native58，packed32826 |
| Core ABI | 7 | 7 |
| CEP | panel58 | panel58（既有 live Junction） |
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

native58/CEP58 配对收据：artifacts/m3-13-native58-deploy-before.json、m3-13-native58-deploy-after.json。发布前 fresh process checks 均无 AfterFX/AfterFX_64；使用 tools/Deploy-TestBuild.ps1 和既有 native/CEP Junction。七个 native/Core、十一项 CEP、runtime selector 与一键配对恢复 report 全部核对。Runtime selector：StarfieldCore-7840E5B1298D129B.dll.

| dist 文件 | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | 7DE33B342FA9FE408F55E76A533EB6F8C2057F6CA9C619FACB6CF931D8E0DC53 |
| StarfieldEmitter.aex | 15E9B7E0A72CEDDEB0BF2EDCE43298B17C1C04C3F986D783C2B5FF62A9D6CF32 |
| StarfieldParticleNode.aex | F8E18F08498AECC1B422B334D8BC4385333AD530DF897A699FAD9B9224980613 |
| StarfieldForce.aex | 0A7A94FE1D541B2555BD54A711B0E1E75A19F4B4B610C3C576E8F865953BE796 |
| StarfieldTransform.aex | E1D5EE3986EA059BE7D6E06577D012E67004FD8210D7576ACBF7882E11FFA85F |
| StarfieldHost.aex | CC45D0A1DED203420531ED7AE29B262452BD8A85DA3FEE06B3CC7FD95D3FDF75 |
| StarfieldCore.dll | 7840E5B1298D129B98C8ABA3B5025DC82E765BE79C66E3FDEE786CA5016BBF54 |

exact native57/CEP57 备份：artifacts/disabled/m3-13-native58-panel58-texture-owner-20261008。既有历史备份保留。回滚报告：artifacts/m3-13-native58-rollback-report.log；发布日志：artifacts/m3-13-native58-deploy-wrapper.log。

AE 关闭后的单步回滚（仓库根目录）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'm3-13-native58-panel58-texture-owner-20261008' -Restore
```

未更改进程起停、注册表、Adobe 缓存、环境开关或 Junction。完整目标继续 active；未从源码/驱动测试声明新增 AE 宿主资格。

## 开放 gate

- M3-16 纯核心 Shift Seed/Birth Chance 策略已提交f83e20f；独立40985检查、0失败。图求值/历史/辅助发射/预算/身份工作树保留，作者未完成，不暴露 UI、不发布到 Core；ADR0037 规定后续完整迁移。当前暂缓以处理 M3-13 方向反馈。

- owner 确认 native58 Comp 2 可选择并正常显示纹理；默认镜像修复待发布/实际方向验收。该观察不关闭八种时间采样、背面、撤销/保存重开及 Source/Masks/Effects gate。
- native57/CEP57 Cloud 外观、动画、预设、撤销、保存重开及实际 AE GPU/shutter 行为待 owner 验收。既有宿主观察仅覆盖记录的 AE2023.5.0 Build52，不从编译扩展支持版本。
- Face/Model、Path/Shadow、Shift Seed/Birth Chance、Texture Source/Masks/Effects stage 等剩余 Particle 行为仍开放；PTF 按 owner 决定等待 Physics。
