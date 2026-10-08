# 当前工程状态

核对日期：2026-10-08。native57 / packed32825 / Core ABI7 / CEP57 已成对部署，源码 f60910adf9b331d9bdb06a3fb09053a1f7511ee1 已推送。发布时间 2026-10-08T21:04:06.6364006+08:00。Core 数值里程碑 f5b0844、原生/CEP 作者里程碑 f60910a；实际 AE 验收仍开放。MNT-01 共享改动保留并排除于实现提交。

## 源码与安装

| 项目 | 源码候选 | 当前安装 |
| --- | --- | --- |
| 原生版本 | build57，packed32825 | native57，packed32825 |
| Core ABI | 7 | 7 |
| CEP | panel57 | panel57（既有 live Junction） |
| Particle | total534；Cloud 追加528..533；binding5 | 同契约 |
| 主效果 | manifest29，count755；命名/排序 ADR0035 | 同契约 |
| 节点 | Emitter、Auxiliary、Particle、Force、Transform、固定 Output | 同节点族 |

Appearance 已退出当前注册表、构建与部署。MFR/Compute Cache 未启用。GPU F32 逐设备/逐帧协商；实际驱动的数值检查不等于 AE 宿主资格。

## Cloud 实现

Cloud 是一个逻辑粒子的圆群。Circles/Aspect/Density 默认10/150/66，Density0..1000；0 重合为一个圆，1000 增大成员中心散布。Circles1..1000、Aspect1..1000 是待参考证据确认的独立范围。确切随机分布和缩放仍需外观比对。

optional keys37..39、snapshot7/ABI7；旧图缺少 Cloud keys 时保留固定五圆。原生 hidden constant activation533/disk242 默认0；明确 Shape=Cloud 或修改 Cloud 参数才激活。新 CEP/Core 节点写入显式默认。既有 IDs、match names、node schema7/envelope1/序列契约保留。详见 ADR0036。

## 本轮验证

- 原生绑定7635 + 相机12，0失败；实际 Particle Cloud 回调277，0失败；实际控件注册/Texture selector276，0失败。
- Cloud CEP49、Texture CEP47 检查通过；完整 gateway/coordinator 的 Cloud 提交、失败回滚、预设 Add/Replace 通过。
- 完整 May2023 /MT 构建通过：artifacts/m3-15-cloud-authoring-build.log。
- Core 未变更于 f5b0844 的数值里程碑；其2441项 Cloud 核心和1116项私有 CUDA/OpenCL 驱动证据见 ADR0036。作者接入不替代真实 AE gate。
- 具体运行命令见 docs/testing.md；日志 artifacts/m3-15-cloud-native-sync.log、m3-15-cloud-callback-tests.log、m3-15-texture-cloud-selector-tests.log、m3-15-cloud-gateway-transactions.log。

## 当前安装证据与回滚

native57/CEP57 配对收据：artifacts/m3-15-native57-deploy-before.json、m3-15-native57-deploy-after.json。发布前两次 fresh process check 均无 AfterFX/AfterFX_64；使用 tools/Deploy-TestBuild.ps1 和既有 native/CEP Junction。七个 native/Core、十一项 CEP、runtime selector 与一键配对恢复 report 全部核对。Runtime selector：StarfieldCore-7840E5B1298D129B.dll.

| dist 文件 | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | C0AB3C7D307D2684400161600A2058CB39F2F520A8CC38CFB685B56BAC5B82C4 |
| StarfieldEmitter.aex | 201BEE49E7E02541B49B8E800131653C9D3FF44A6B8B32DC35851B8FC9322F4C |
| StarfieldParticleNode.aex | 3EB809A2EC86414AAE3EA923AA2436D6FFC22FF761DF2EEF660EE0546C8EE2D9 |
| StarfieldForce.aex | 420FFC1F9556DF01D6405EF3D167E838202D875A9C8AFD32EE6C3EB370AD5C0F |
| StarfieldTransform.aex | 477554F195AA9525794A3D45D310BEF92D9C3C4B79C3E3F99FC6233B809BFD19 |
| StarfieldHost.aex | 21C44C9E8DE55CA2DE04F1E3969D1C62AA2F7F036C2EAF4A188B0752FF11A56D |
| StarfieldCore.dll | 7840E5B1298D129B98C8ABA3B5025DC82E765BE79C66E3FDEE786CA5016BBF54 |

exact native56/CEP56 备份：artifacts/disabled/m3-15-native57-panel57-cloud-20261008。原先56的配对备份保留。回滚报告：artifacts/m3-15-native57-rollback-report.log；发布日志：artifacts/m3-15-native57-deploy-wrapper.log。

AE 关闭后的单步回滚（仓库根目录）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Restore-TestBuild.ps1 -PluginDir 'D:\Software\Adobe\Adobe After Effects 2023\Support Files\Plug-ins' -BackupName 'm3-15-native57-panel57-cloud-20261008' -Restore
```

未更改进程起停、注册表、Adobe 缓存、环境开关或 Junction。完整目标继续 active；未从源码/驱动测试声明新增 AE 宿主资格。

## 开放 gate

- owner 的 native55 Texture menu 仍只显示 None；native56 owner-pinned public layer inventory 已部署，实际新结果待确认。
- native57/CEP57 Cloud 外观、动画、预设、撤销、保存重开及实际 AE GPU/shutter 行为待 owner 验收。既有宿主观察仅覆盖记录的 AE2023.5.0 Build52，不从编译扩展支持版本。
- Face/Model、Path/Shadow、Shift Seed/Birth Chance、Texture Source/Masks/Effects stage 等剩余 Particle 行为仍开放；PTF 按 owner 决定等待 Physics。
