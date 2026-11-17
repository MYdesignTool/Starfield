# MFR离线复核与Owner实机交接 — 2026-11-16

按Owner最新安排，本轮仅离线测试和整理交接。没有启动/退出AE、操作工程、部署132或修改宿主设置；native/panel实现与冻结132构建输入保持，没有重新构建AEX或修改公开ID/ABI/schema。

当前安装129/124仍关闭MFR；冻结132/124只在独立实验配置中启用九效果MFR声明。正式AE2023资格没有完成。手工步骤见 [实机清单](ae2023-owner-test-checklist-20261116.md)，反馈见 [结果模板](ae2023-test-results-template.md)。

## 本轮实际执行的离线测试

证据根目录：`artifacts/mfr-offline-handoff-20261116/`。下面是实际测试程序结果，不把不同计数单位混加成“总通过数”。

| 范围 | 结果 | 证据 |
| --- | --- | --- |
| 全部面板回归 `node tests/RunPanelTests.cjs` | 75套，0失败；之前受沙箱限制的retirement Junction测试也已通过。本次允许的Junction仅在仓库内隔离夹具中创建 | `panel.log`、`panel-results.json`、`panel-logs/` |
| GPU生产适配器严格分析 `RunGpuConcurrencyTests.ps1 -Run -Analyze` | /analyze /W4 /WX，24,043项，0失败；四线程400渲染＋400透传，以及重入/取消/分配/驱动故障 | `gpu-analysis.log` |
| 同一GPU适配器ASan `-Run -Sanitize` | 24,043项，0失败 | `gpu-asan.log` |
| 真实驱动 `RunCoreTests.ps1 -GpuConcurrent` | 477项，0失败；CUDA/OpenCL各96帧渲染＋96透传＋96取消。同驱动串行/并行输出字节一致，CPU对照沿用既有0.0002容差 | `gpu-driver.log` |
| 发射历史缓存 `RunNativeTemporalCacheTests.ps1 -Run -Sanitize` | 131项，0失败；含四线程800乱序帧的积分/出生时间对照、请求隔离和旧发布拒绝 | `cache-asan.log` |
| 原生绑定 `RunCloudNativeSyncTests.ps1 -Run -Bindings -Sanitize` | 48,165项，0失败；Camera12项，0失败 | `native-bindings-asan.log` |
| 冻结实际Core DLL `RunCoreDllConcurrencyTests.ps1 -Run` | 新增2,286项，0失败；129/132实际DLL的36组串行配对、四线程400帧、24取消、RGBA8/16/32F。像素/区域/步长/格式/inspect一致，结果跨其他调用仍有效，并由所属generation释放后再卸载 | `core-dll.log` |
| 实际冻结AEX声明 `RunMfrBinaryFlagsTests.ps1` | 开启132与关闭131配置各333项，0失败；比较九个实际AEX的GLOBAL_SETUP和内嵌PiPL，不依赖头文件文本推算 | `binary-enabled.log`、`binary-disabled.log` |
| 生产资格日志头 `RunMfrJournalTests.ps1 -Run` | 新增独立四线程测试：194完整记录/97配对区间，四渲染线程、48外层渲染、48嵌套GPU异常展开、一次pre-render；ID唯一、结束配对和实际区间重叠通过 | `journal.log`、`journal-tests/records.jsonl` |
| 宿主smoke脚本离线夹具、证据检查器 | 真正的JSX脚本以严格ES3解析并在无全局JSON的模拟宿主执行；CPU/CUDA矩阵、已有工程拒绝、失败清理通过。Python两组测试验证像素差异、错误进程/时间窗、目录越界、缺失配对和pre-render假并行拒绝 | `harness.log`、`checker.log` |

实际Core DLL测试使用本项目UX-16自主生成的四个图输入，逐一校验固定SHA256；不是父目录反编译实现或旧插件资源。输入副本与测试源码哈希随本轮证据保存。该项不测试AE的动态热切换；M17仍需专用实机测包。

资格日志测试使用独立的 `journal-tests/records.jsonl`，没有把离线进程记录写入真实AE资格日志，也不把它当作AE并行证据。实际AEX仅在离线选择器夹具中查询GLOBAL_SETUP，没有启动宿主。

前轮May2023 SDK132完整NoPublish构建证据仍为 `artifacts/mfr-01-20261116/sdk132-mfr-build.log`。本轮复核223个构建输入与冻结副本匹配，复用该构建，没有把它写成本轮重编译或宿主通过。前轮沙箱/测试容差失败记录保留，本轮结果不回写前轮冻结时的测试计数。

## 交付状态

- 132候选manifest保持 `A370317C83DD1A581AE1B000A2DFEC7A95BF8BF767AA853344CEA92574E088EE`，`experimentalOnly=true`、`hostQualified=false`、`deployed=false`。
- 当前安装11二进制、29面板源、Core selector与冻结129/124复核；本轮完整哈希结果见 `audit.json`。
- 18项常规AE项目交Owner手工执行；另单列需专用包的Core热切换门。所有实机结果初始化为未测，清单带切换132和恢复原配对命令。
- A01/A02可先在已安装129/124验证；MFR项不能在129上判断。PNG smoke不能替代完整16/32位/HDR、资源族、UI取消及保存重开资格。
- 原先请求代理启动/脚本自动退出的方案已被当前Owner手工安排取代；本轮不再请求启动AE。
