# Starfield 架构

Owner于2026-11-17确认native136/panel124的Over Life生效、连续编辑与曲线闪回原问题已解决，作为INT-01同步main及下一轮开发的源码基线。以下逐版说明保留实施时状态；不扩大为完整MFR或其他宿主资格。

STB-21的136/124仅将成功曲线提交改为排队局部重绘，清除PF_EO_UPDATE_NOW，并让原调用内预览覆盖失效通知重入；拖动及失败恢复仍立即画。Owner曾确认135手势正确但闪回仍在，136的事件返回顺序模型通过离线回归后，现已确认原问题解决。既有拥有值提交、参数/ABI与Core数学保持，见ADR0023/current-state。

STB-21的135/124在134有效提交上补连续插入点手势、仅发布调用期间的数值显示预览与局部重绘。预览按context/UUID/bank隔离，token析构清理，重入DRAW不读混合参数来画旧形；不成为作者/渲染缓存，不跨调用遮挡Undo。Core数学/ABI、拥有值提交、ID/schema与MFR配置不变，Owner已确认135编辑体验正确，但闪回仍在；136待实测，见ADR0023及current-state。

STB-21的134/124以固定拥有值请求提交自定义曲线/渐变，独立于普通监督回调的params数组数量门；不伪造num_params，重新核对宿主source kind/UUID/guard与renderer guard。局部拖动/松手一次发布及失败恢复保留，普通监督回调仍有原数量门。公开ID/布局/schema、Core数学/ABI与Render.hpp不变，MFR继续实验配置。Owner已确认134的Over Life生效，135编辑体验已确认，闪回继续由136验证，见ADR0023、engineering/particle-overlife-20261117.md和current-state。

MFR-01已完成历史前缀请求隔离及GPU请求内核隔离；GPU设备锁不再跨宿主回调，OpenCL参数不再共享。132独立资格构建的九个效果AEX同步设置MFR bit27并记录真实帧区间；已按Owner要求在freshAE0通过单Junction部署132/124，129/124回退完整，默认构建仍关闭MFR。实际宿主与资源族资格尚未完成，由Owner手工验收，见engineering/mfr-readiness-20261116.md、engineering/ae2023-owner-test-checklist-20261116.md及ADR0050。

STB-20由gateway私有ES3 JSON承担所有宿主协议编解码，Model/预设传输共享，两个页面外层加载失败返回自带字符串编码。全程不依赖或修改宿主全局JSON，避免冷启动未被其他脚本预热时连错误回复都失败。解析不执行文本，原有请求/事务与原生/Core保持；见ADR0046。

UX-16把新效果首帧与首次原生节点发布对齐：新默认为Node Graph，旧工程回填值保持legacy；仅在ready0/revision0/guard0与精确出厂ARB证明下生成无宿主写入的三节点数值投影。首次空图发布沿用图内身份，已有图保留严格绑定。成功初始化的画布不再使用之前的legacy显示信封。见[ADR0049](adr/0049-new-effect-initial-render.md)，真实AE连续性待验。

## 数据流

PERF-17将画布显示与完整作者快照拆开：canvasOnly只读身份/连接/位置/摘要，不构造可提交默认参数；纯布局/AE参数选择直接可用，结构意图共用按需完整读取后才进入原有事务。只读标记贯穿序列化/编辑门，不改native/Core/ABI/schema。[画布读取契约](engineering/canvas-read-20261111.md)。

PERF-16将选择发现与作者加载分开：200ms完成式轻探测，稳定后才完整读，状态2秒/完整审计15秒保持；选择诊断延后800ms，新选择取消旧诊断。采用selectedLayers数组避免全合成枚举，普通focus只唤醒探测。编辑仍需新鲜完整快照，不改native/Core/事务。[性能边界](engineering/selection-performance-20261109.md)。

UX-15面板发现将普通无选择与真实错误分离：无有效选择时只读复查上次画布目标，自动刷新决定是否跟随新选择；后续调用明确绑定画布目标。空提示位于视口层，自动刷新仅会话状态且启动勾选。保持既有token/scope、原生发布和删除契约；见[画布目标说明](engineering/canvas-target-20261109.md)。

UX-14 panel118 诊断呈现修正：无效边仅在来源节点显示红色错误/描边；Model 缺少几何时沿用默认立方体并显示黄色警告；安全文本暗色浮窗展示详情。只读派生，不增加 AE 通信或改变128执行投影/事务。见[节点诊断说明](engineering/node-diagnostics-20261109.md)，安装状态以 current-state 为准。

UX-13源候选128/117将作者图与可执行图区分：错误连线持久保留，由共用Core拓扑规划器生成安全执行子集与私有警告记录；UI显示黄三角。拓扑意图本地预览、合并且只有一个宿主写入，已有节点的连线/删除批次走带旧行CAS的原生delta，新增/资源保持既有受控事务。布局仍独立；guard、旧绑定交接、物理删除后的依赖刷新与未知结果停止策略保留。完整边界、诊断读回成本和回退兼容性见[UX-13说明](engineering/editor-topology-20261104.md)，部署以current-state为准。

PERF-10（126/115）将纯画布布局独立为本地绘制与合并后的普通坐标元数据保存，不进入图发布/Model资源/绑定事务。真实结构编辑保留一次权威发布确认，直接采用成功快照；时间pulse与结构刷新分离。原生126删除生命周期与Core保持不变。具体写入、冲突、Undo与失败恢复边界见[编辑性能说明](engineering/editor-performance-20261031.md)及ADR0048，部署与资格以current-state为准。

```text
AEGP Panel + WebView2 (web_panel)
  -> 有界消息队列 -> 安全 UI idle -> 既有 JSX/native 作者事务
  -> 原生节点参数、UUID/revision/checksum 与主效果图持久化
  -> SmartFX 不可变采样/历史/资源快照
  -> Core generation 的 C ABI / CPU 或 GPU 数值场景
  -> AE 输出世界
```

| 模块 | 职责 |
| --- | --- |
| StarfieldHost.aex | AEGP 面板、WebView2 生命周期、UI 排队、Model/预设文件服务及互斥 |
| StarfieldParticle.aex | 主渲染效果、ARB 图持久化、SmartFX、Core 加载和像素转换 |
| 各节点 AEX | 隐藏菜单的原生作者参数、持久化/动画绑定，SmartFX 透传输入 |
| StarfieldCore.dll | 宿主无关图求值、确定性模拟、CPU 栅格化和 GPU 场景准备 |
| web_panel/ | 画布/预设、选择/拓扑、守卫事务、自适应检查及传输适配 |

Output 无独立 AEX；Auxiliary 使用 Emitter 的结构模式。参数、曲线和渐变使用 AE Effect Controls；旧浮动编辑器及 Appearance 不恢复。

## 所有权与执行

Settings.hpp 拥有 Core 值类型，Render.hpp 是宿主无关渲染边界。AE suite、句柄、世界指针停在 ae_plugin/。主 AEX 也编译构图和冻结快照所需的共享 Core 代码，不能把所有求值描述为 DLL 独占。C ABI 不传 C++/AE 对象，分配与释放在同一 generation/CRT 内完成。

WebView2 回调只保存有界自有数据；只允许本地映射的两个页面发送请求。UI idle 检查线程、重入、插件互斥、有效且启用的 AE 主 HWND 和脚本可用性。模态期间保持 FIFO 与原截止时间，过期或旧页面请求不执行；单次 ExecuteScript 的 quiet-error 状态成对恢复，不持久修改全局偏好。关闭、导航和重放保护见 ADR0046。

Window 菜单只提供两个面板入口。内部命令用 AEGP_Menu_NONE 定义并保留处理器，每次面板脚本执行前注入本会话的六项命令 ID；共享 gateway 按版本和整数范围验证后调用，不依赖可见菜单名称。绑定存在但缺失/无效时拒绝，只有旧宿主完全没有绑定时才使用历史名称查找。图修复由节点面板原生 Diagnostics 菜单显式触发，继续走既有互斥和修复处理器。

图与节点身份、磁盘参数 ID、schema、动画、读回/回滚及 Core ABI 不因容器迁移而变化。按绝对时间与稳定身份求值，图/历史/几何及 Core lease 在 render 完成前保持有效。Core 输入为数值和拥有明确生命周期的资源；文件和宿主操作在适配层。

普通删除通过有界原生增量进入常驻Host：只保存受影响连接，保留旧作者源到主效果一次发布ACK，再移除源并刷新绑定；不经过Model全图备份。失败补偿与提交后收尾分别报告，未知回执不自动重试。其余编辑和复杂资源保持既有事务；ECW入口子集与外部删除资格边界见ADR0048。

SmartFX 尊重 rowbytes、PAR、downsample、ROI 与 AE 8/16/32-bpc 语义。GPU 按设备/帧协商；MFR 和 Compute Cache 仍关闭，不从编译推断真实设备或新版宿主资格。格式、Motion/Transform/Model 的具体契约以 Settings/Render/PluginApi/schema 和对应 ADR 为准，不复制历史 ABI/snapshot 数字。

## 当前与历史

当前配对与证据仅见 [current-state.md](current-state.md)。原先混入的部署流水、旧安装版本和未发布候选叙述已移入 [2026-10-19 归档](history/2026-10-19/pre-web-panel/docs/architecture.md)，不能据此恢复旧 CEP 或旧包。变更范围与授权以 [任务卡](agent-backlog.md) 和 ADR0011 为准。

PERF12原生参数编辑可在同帧及严格身份/SDK状态证明下，复用非目标纯数值节点的原始参数记录；目标节点、元数据/连接、资源、Core验证和发布ACK保持原路径。原子生命周期generation及exact accepted ARB约束失效，缓存只存拥有值/PF_State，不存宿主句柄。能力不足回完整编译。最终退休以兼容126前缀的交替禁用标记减少冗余写，并读回精确handoff；详见ADR0048与当前候选资格。
