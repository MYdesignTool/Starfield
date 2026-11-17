# PERF-17 画布读取与按需完整作者快照

120的getPanelState在显示之前总会readNativeNodes，读取每个数值、梯度、完整曲线、Motion路径前缀、Model描述/边界，并可能枚举图层资源，最后在客户端序列化完整参数图。仅显示节点/连线/位置不需要这些数据。普通读取仍在AE主线程执行，16轮的轻选择探测没有消除这个完整读取成本。

121的graph panel请求canvasOnly：已初始化的Node Graph仅读UUID、类型、布局、最多4个出边及少量摘要（rate、size/opacity/shape、Aux标记等），Output仅max与位置。主效果guard须0。摘要不读曲线、梯度、模型资源/Bounds、全部数值或图层资源；未初始化图和legacy仍沿既有完整初始化路径。没有改变native128、Core、ABI9、公开参数或存盘schema。

轻快照明确标canvasOnly，没有graphHex或authoring recordStamp，只有显示用canvasStamp；客户端校验63作者、身份/边唯一性与存在性、坐标/几何/摘要有限值，然后生成带只读标记的视图图。不能序列化、不能调用GraphEdits.apply、不能进入GraphTransactions或EditQueue；布局helper保留只读标记。不会补默认参数来伪造可编辑作者图。自由语义错误边仍保留，由已有原生诊断定位。

节点单击打开AE参数直接用已读UUID；纯布局用原有独立坐标CAS队列保存，不需要完整作者值。真实结构意图最多64个，首次按需取一次完整且正常校验的getGraphSnapshot，连续意图共用读取；失败、target/scope/epoch不符或pagehide均不提交。完整快照接纳后才进入原有提交/guard/删除/回执流程。读取期间不重建活动手势，结束后保留期间产生的坐标变化，布局冲突仍走既有flush/fresh read保护。没有后台自动读取全参数来补齐只读画布；首个结构编辑因此仍可能等待完整作者读取。

首次使用的UI进度只显示Preparing edits。结构编辑的完整确认、旧绑定交接、原生删除保护均不减少；诊断用轻快照revision/CRC，不用它提交图。只读画布不估算基于完整粒子参数的Live count。状态轮询/15秒检查继续，但图检查也只读显示数据；必要的作者全读留给实际结构编辑。

实际JSX夹具：8个粒子、每个3条64点曲线，节点值读取3552→112（约31.7倍调用量缩减），写入/Undo/编译均0。旧generic fixture的扩展曲线disk别名只在本次夹具中补为当前注册ID，未改产品ID。实际panel测试覆盖首次卡片、单击参数/布局零全读、连续结构意图一次hydration、原参数保留、失败/异目标/pagehide/活动手势门。是离线调用量，不是AE耗时或宿主资格。

AE运行时不部署。候选121在artifacts下独立替换cache/gateway/manifest版本；工作源码暂保留120令牌，兼容已打开的120面板。freshAE0部署前核对阶段源码和冻结，才切换工作面板到121并经既有KeepNative工具发布；完整128/120回退从已安装冻结保存，另保留阶段源码。
