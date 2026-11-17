# Starfield 工程基线

本轮只建立基线、审计和规划，属于 MNT-01 的文档治理边界；不关闭原清理任务，不承担 M3-20 修复。证据采集始于 2026-10-13。路径均相对 `newStardust/`，`:行号` 指本轮源码位置。

证据等级：**已证实**＝当前源码/本轮可复现结果，或明确注明来源的既有记录；**疑似**＝有调用链支持的风险推断；**待验证**＝需要额外实验或 AE 实机证据。历史结果不自动证明当前候选通过。

## 版本与工作树

| 项目 | 基线 |
| --- | --- |
| 分支 / HEAD | `codex/m3-07-particle-gradient` / `e02ad2eef96b2974f0969c4197b76c6ef4afb581` |
| HEAD 描述 | `feat(turbulence): add bounded deterministic noise kernel`，2026-10-13 09:02:31 +08:00 |
| 初始 Git 状态 | 140 个已跟踪修改、1 个删除、21 个未跟踪项；无暂存修改。覆盖 AE、Core、CEP、schema、测试与文档，HEAD 不能代表当前源码 |
| 源码版本 | `ae_plugin/PluginVersion.h:10` build86 / packed32854；`include/starfield/core/PluginApi.h:13` ABI9；面板 CEP89，契约检查通过 |
| 本地 bundle | 本轮只读核对 `dist/` 的11个 native/Core 文件与部署收据全部一致；27个 CEP **源码**文件也与收据一致；所选 runtime DLL 哈希一致 |
| 部署收据 | `artifacts/m3-20-motion-author-20261013/deploy-after.json`；未重新检查 Adobe 安装 Junction 或真实加载进程 |
| 宿主范围 | AE2023；既有行为记录包含 AE2023.5.0 Build52，不能推广为当前全部功能已通过 |

取证目录：`artifacts/engineering-audit-20261013/`。`git-before.txt`、`diff-before.txt`、`source-before.json` 保存初始状态和540个现存 Git 文件的 SHA-256；`pair-check.json` 保存本地 bundle/CEP 源码核对结果。原有删除、未跟踪文件与未提交实现均保留。本轮只新增五份审计文档和该 ignored 取证目录；未修改构建脚本、测试或业务代码，未部署、启动/停止 AE、改注册表或 Adobe 缓存。

**取证窗口限制（已证实）**：结束核对时538/540个初始文件哈希不变；`tests/RunCloudNativeSyncTests.ps1`、`tests/native_sync_tests.cpp` 在窗口内发生变化，本轮没有写入它们，变更来源未进一步调查，也未回退。差异见 `source-after-verification.json`。Git状态条目相比初始仅多 `docs/engineering/`，但已脏文件继续编辑不会增加状态条目；因此不能声称“整个工作树被冻结”。本轮通过的Core/JS源、CMake失败相关源和构建清单不在这两项中；未来native sync回归应绑定新hash重新验证。

## 构建入口与约束

| 入口 | 实际职责 / 当前状态 |
| --- | --- |
| `ae_plugin/BuildWindows.ps1` | May2023 SDK，x64 Release，MSBuild v145，原生模块 `/MT`；Core → 主 AEX →8种节点 AEX → Host，11次顺序 MSBuild 调用，分别带 `/m`（267–313行） |
| `ae_plugin/BuildPiPL.ps1` | SDK PiPL 预处理/转换，核对 identity、flags、packed version；含精确输入及受限规范化缓存恢复，不能把缓存命中当作 SDK 工具恢复 |
| `CMakeLists.txt` | 宿主无关静态 Core、DLL 和20个 CTest 注册项；**当前构建失败**，见下表 |
| `tests/RunCoreTests.ps1` 与专用 runners | 直接 `cl @rsp` 编译/链接再运行；不需要启动 AE，部分需要 SDK headers / fake suites |
| Core 热更新 | `CoreOnly` 指纹覆盖共享 AEX 输入并检查安装配对；共享图实现变动常会要求完整配对，不能绕过检查（BuildWindows 118–242行，ADR0012） |

项目规定的原生入口：

```powershell
powershell -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1
# 隔离候选需明确 NoDistPublish / NoRuntimePublish；执行前检查其环境、GPU/PiPL和subst行为。
powershell -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -SdkPath 'AdobeSDK\May2023_AfterEffectsSDK' -ArtifactLabel 2023 -NoDistPublish -NoRuntimePublish
```

本轮未运行该完整脚本：它包含临时 `subst` 映射及默认发布路径；审计范围不包含宿主动作。既有 `native86-build.log` 记录完整构建成功，但 `/v:minimal` 没有足够阶段计时。本轮用已有 Core 测试配方在全新取证目录编译，不触碰原候选、中间产物或回滚备份。

## 本轮实际验证

| 检查 | 结果 / 耗时 | 可复现证据 |
| --- | --- | --- |
| 全部 JS suites | 46/46通过，串行8.219秒；Node v24.18.0 | `panel-results.json`、各 `*_tests.js.log`；与 RunPanelTests 相同发现/child Acorn选项；`preset_tests.js` 单独 cwd 到取证目录，保留原 `artifacts/preset-catalog.hex` |
| 完整 DOM 矩阵 | 18/18通过，16.455秒；fresh/reused × commit失效 × current/3类legacy，另加旧API与commit中reload；Material/Motion/删除开启 | `dom-current-catalog-results.json`；实际 gateway + 模拟原生receipt/backup；使用已保留的 followup catalogs，**本轮未重建原生注册目录** |
| catalog负面对照 | 使用 `artifacts/` 根目录旧catalog时18例均失败：Motion缺少 `Max Delay (Seconds)`；换到 followup catalog后通过 | `dom-results.json` 与 `dom-0-0-current.log`；证明输入不配对，不证明业务回归 |
| Core 配方 | 编译成功，11,895 checks / 0 failures；编译25.999秒，执行0.426 / 0.045 / 0.040秒 | `core/compile.rsp` 从 RunCoreTests 的25个源文件和现有flags派生，只改变输出目录；`core-times.json`、`core-run-*.log` |
| CMake clean build | configure成功；全量失败，19.118秒到失败，**不是完整编译耗时** | `cmake-build.cmd`、`cmake-configure.log`、`cmake-build.log`：model_transport_tests 21/22行 ABI8静态断言失败 |
| CMake 独立链接 | DLL 与 Motion path graph target 都失败 | `cmake-link-probe.cmd/.log`：Light Path resource LNK2019/LNK1120；CMake清单遗漏实现，MSBuild包含该文件 |
| CEP契约 | 通过，只读 | `powershell -NoProfile -ExecutionPolicy Bypass -File tools/Assert-PanelContract.ps1` |

测试命令/环境写在保留的 `.cmd`、`.rsp`、结果JSON及 [TEST_STRATEGY.md](TEST_STRATEGY.md)。失败证据不被成功重跑覆盖。CMake首次命令路径使用 `/` 导致 cmd 启动失败，保留在 `cmake-launch-failure.json`；19.118秒来自修正启动后的真实构建，未将执行器故障归因于代码。

## 测试现状及主要风险

- **已证实**：有63个 `*_tests.cpp`、46个 JS suites、55个 `Run*Tests.ps1`，但数量不是覆盖率。`RunAllTests.ps1:35` 只列旧 RunCoreTests scopes/4类节点，未调用 Motion/Turbulence/Model/Material专用 runners 或完整DOM矩阵。`Scripts` 发现 `*_tests.ps1` 不会发现 `RunMotion*Tests.ps1`。不能把 All通过描述为全部当前功能通过。
- **已证实（既有记录）**：`docs/testing.md:3` 保留 native sync、UI exclusion、删除生命周期通过证据；本轮没有重新执行这些原生SDK fixtures、ASAN或GPU硬件测试，不能把历史计数列为本轮通过。
- **已证实（owner报告及现状文档）**：`docs/current-state.md:3` 仍开放 Motion模式回退、Motion/Force身份删除对话框；准确根因及native86实机修复效果**待验证**。
- **疑似**：多层事务、重复映射和双求值路径增加联动修改成本，详见 [TECH_DEBT.md](TECH_DEBT.md)；未发现足够证据宣称当前存在内存泄漏、Core依赖AE或源码include环。

## 效率边界

1. **已证实**：本轮Core热执行中位数0.045秒，对照编译25.999秒。现有直接cl配方每次重编其source list；优先建立可复用编译目标，不能只并发执行更多完整配方。
2. **已证实**：NodeEffect工程33个compile inputs由8种NodeKind各自构建；主AEX与DLL都编译GraphEvaluation。共享源码不是重复业务实现，但带来重复编译/配对成本。11次MSBuild顺序调用不等于内部单线程；收益需要clean/no-op/单文件/公共header四类对照。
3. **疑似**：`StarfieldHost.cpp:120` 每次同步图命令遍历项目item/layer/effect（上限65536），再native编译/绑定/验证；大工程提交延迟可能随工程规模增长，未测AE耗时。
4. **待验证**：AE启动、插件/CEP发现、首次预览、seek、增删、保存重开各阶段没有本轮计时。利用已有SmartRender计时、graph journal和owner手工时间点，分开cold/warm、AE缓存与Core成本；不自动启动AE。

治理批次和回滚条件见 [REFACTOR_PLAN.md](REFACTOR_PLAN.md)。
