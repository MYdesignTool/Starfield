# Starfield CEP 面板

当前源/安装为 panel54：节点画布、属性窗口、curve/gradient 编辑器和独立预设窗口。版本/原生配对见 [当前状态](../docs/current-state.md)，早期交接保留在 [归档](../docs/history/2026-10-07/cep-panel.md)。

## 行为与协议

左侧 palette 默认展开，可拖入 Emitter/Auxiliary/Particle/Force/Transform；Output 固定。Transform 的 Null 资源按层 ID 解析，数值预设支持 None；AE 中的 Null 几何、动画、撤销和重开仍需宿主验收。拖动预览不访问 host，目标变化/Escape/失去捕获取消；画布 pan/zoom、负坐标和多选由现有守卫事务处理。
主效果隐藏 ARB 不可直接经 AE2023 ExtendScript 读取/写入。当前桥使用 UUID 标识的普通原生效果流、数值 commit/revision/checksum 收据；ADR0013 的 expression request mailbox 已拒绝，不能恢复。
动画 numeric alias 的表达式服务于参数采样，不是旧图请求 mailbox。原生 curve banks 通过精确磁盘 matchName 查找，topic 不当作脚本容器。

Graph 事务验证 target、base revision/record stamp、完整 native schema，并读回确认；编辑器 bank 常量与 animated 参数有不同策略。当前交互锁/回滚与动参提示的差异仍需要宿主验收。
后台 pulse 完成后再排下一次：活动500ms，三次不变后2s；变更/焦点/Refresh 与名义15s audit 做完整检查。Live count 有相应延迟，实际光标/延迟必须在 AE 量测。
预设 Add/Replace 使用一次即时规划快照并由 host 独立验证；用户文件读写只在明确保存/加载操作发生。各模式与参考边界见 ADR0029/0030。

## 安装与验证

CEP 前端目前通过既有 source Junction 加载；改动这些源码可能被正在打开的 panel 读取。当前整理只修改测试与 README，保留运行源码。
Install.ps1 默认行为及用户目录操作遵循 ADR0011，面板安装不包含原生 standing deployment 的授权。仓库脚本不写 PlayerDebugMode 或任何注册表值；host-wide switches 由 owner 手动决定。
manifest 的 AEFT23.0..99.9 是加载 eligibility，不是所有 host 已验证支持；owner 范围只有 AE2023。

```powershell
node tests/RunPanelTests.cjs
```

所有 *_tests.js 自动发现、逐个运行、非零失败退出并保留日志。真实 AE 拖放、原生改值、preset/undo/reopen 与性能 gates 见 [行为验收](../docs/compatibility-matrix.md)。
