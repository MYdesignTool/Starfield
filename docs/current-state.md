# 当前工程状态

核对日期：2026-10-08。工作树的原生版本标记为 build55 / packed32823，Core ABI 为6；dist 当前安装记录为 native54 / CEP54 / ABI6。工作树有未提交的 M3 业务改动，本页不把它们记作已部署版本。

## 源码与部署

| 项目 | 源码候选 | 当前开发安装记录 |
| --- | --- | --- |
| 原生版本 | build55，packed32823 | native54，packed32822 |
| Core ABI | 6 | 6 |
| CEP | panel54 | panel54 |
| 节点 | Emitter、Auxiliary、Particle、Force、Transform、Output | Emitter、Auxiliary、Particle、Force、Transform、固定 Output |
| Transform | Core/原生/CEP 候选在工作树 | 随 native54 成对部署；AE 行为验收仍开放 |

Appearance 已按 owner 要求退出当前注册表、构建与部署。旧效果/工程是否迁移遵循对应 ADR；整理工作不引入迁移，也不更改身份、ID、ABI 或主要实现。

GPU F32 能力在原生 PiPL/runtime 中声明，逐设备/逐帧协商并保留 CPU 路径。MFR/Compute Cache 仍未启用。已存在 GPU 源码和软件场景测试不代表任何实际 GPU 或 AE 构建已经通过完整验收。

## 安装证据

native54/CEP54 配对记录位于 artifacts/m3-14-native54-deploy-after.json。只读 SHA-256 核对确认 dist 中六个 AEX 与 StarfieldCore.dll 和收据一致；StarfieldRuntime/current.txt 指向 StarfieldCore-037D48F4411A16E8.dll。本次工作区整理没有部署或触碰 dist、AE、CEP 用户目录、进程或注册表。

| dist 文件 | SHA-256 |
| --- | --- |
| StarfieldParticle.aex | 88F94BCC7CBD786843B6802F04792AA93CD78058A070B6C038C9F9EDB8089365 |
| StarfieldEmitter.aex | D52DBCEF5215008A438633CA95AB347C481B86A3401F307B716779EF3A8654EC |
| StarfieldParticleNode.aex | C48853C534E849F4D22D344705251B2C2715EE7CB2AFDF251685C6601186AFC0 |
| StarfieldForce.aex | 43D0735D226A851E964C89DA03700F99AA70443A2DDA25C4445CE77978D9D4F4 |
| StarfieldTransform.aex | B8D5255F70B55B3F601DF1F295F9D07BC241101BC0A449AC836FD5AA2E65B971 |
| StarfieldHost.aex | BA70AFBE274E90A74A2978F3903C26C3021D143394ED42CCD11D74ABD0B4A87E |
| StarfieldCore.dll | 037D48F4411A16E812A970BFEA6DDD604F1EF4CD9CA0ED4901F3F2FF848BDD6E |

native54/CEP54 备份位于 artifacts/disabled/m3-14-native54-panel54-main-order-20261008；回滚信息见对应部署收据，仍按 ADR0011 操作。

## 本次验证

- 清理检查点（2026-10-07）：面板入口 RunPanelTests.cjs 的20个套件全部通过，日志 artifacts/panel-tests/。
- 主 Core：11,895 项检查、0 失败，日志 artifacts/workspace-cleanup-20261007/core.log。
- 清理检查点：18个 C++ 范围及6个发布/隔离部署 PowerShell 套件通过；GPU驱动、时间线和曲线编辑器重跑收据见 artifacts/workspace-cleanup-20261007/。这些结果不覆盖随后新增的业务改动。
- 原分析报告核验保留在 artifacts/report-verification-20261007/报告核验结论.md；其失败计数描述整理前的时点，不能当作整理后的测试结果。

宿主证据来自已记录的 AE 2023.5.0 Build52 owner 观察，详见 [行为验收](compatibility-matrix.md)。本次源测试不补足实际 AE、GPU、保存/重开、撤销、运动模糊或 Null 几何验收。
