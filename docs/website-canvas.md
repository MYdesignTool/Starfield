# 官网节点画布

WEB-01 / owner 于 2026-10-20 要求将官网静态节点图换成可操作 UI，并明确优先复用已有画布。原宿主前端 `web_panel/` 仅读取，没有修改。

## 复用与边界

- `docs/site/demo/` 中的 `graph_codec.js`、`graph_layout.js`、`graph_edits.js`、`graph_view.js`、`node_palette.js` 与 `panel.css` 由当前自有前端按 LF 换行复制。没有第三方包、SDK 或旧插件资源。
- `panel-demo.js` 由 `panel.js` 的 46 个白名单函数与 `tools/website-demo/adapter.js` 生成。保留节点外观、绘制、框选、多选、拖动、Alt 拖动复制、Ctrl+D、连线吸附、点击断线、连线插入节点、平移、缩放及 minimap。仅替换节点 tooltip 和选中提示，将原 render 包装为键盘连接入口。
- 适配器直接使用真实 `StarfieldGraphEdits.apply` 和 `StarfieldGraphView.project`，图状态与最多 30 步撤销只保存在当前 iframe 内存；不写 localStorage，不导入导出，不加载宿主桥、JSX、事务协调器、轮询、预设或渲染模块。没有参数编辑或粒子输出。
- iframe 只有 `sandbox="allow-scripts"`，无同源、弹窗、导航或下载权限；CSP 禁止连接、表单与外部资源。正常滚轮保留给官网滚动，画布缩放使用按钮或 Ctrl/Command+滚轮。
- 官网适配增加工具栏添加、删除、撤销、重置、24 节点上限、尺寸变化适应、Enter/Space 选两端连接和状态提示。Output 与连接类型规则沿用真实编辑模块，不模拟任意连接。

## 维护

`node tools/Build-WebsiteDemo.cjs` 默认为只读检查并报告差异；显式 `--write` 才写入 `docs/site/demo/`。生成器仅读取 `web_panel`，不会修改原插件。调用前评审要导出的 UI 源版本；官网复制的是明确版本的快照，不在 CI 中自动从插件前端重建，以免夹带后续插件修改。

本轮 `panel.js` 按 LF 规范化后的 SHA256 为 `2fc78384805fe5f53a742dd17a921aaa7b477bc572917c65122fa2f5a6a45b94`。具体白名单见生成器；每次生成检查函数存在、脚本可解析，产物不得含宿主/网络调用。生成器与适配器只作为工程源，不进入 Pages artifact。

验证入口：`node tests/website-check.cjs`、`node tests/website-graph-check.cjs`。后者直接测试发布副本的真实图编辑模块：连接规则/拒绝循环、断开与重连、删除清理边、复制、位置更新、快照恢复、视图投影，以及入口无宿主传输。CI 运行这两个检查。

实际 Chromium 验证：嵌入画布添加与撤销；独立同源预览中的节点拖动、拖线吸附重连、左侧 palette 拖入、键盘断开/连接、删除、撤销与重置、Transform 拖到 Force→Output 线上自动插入及一次撤销恢复原线；390px 手机嵌入布局、添加/撤销、无页面横向溢出。CSP 沙箱内正常加载，无 warn/error。手机触摸设备未实测，不将鼠标/键盘模拟表述为真机触摸验收。截图位于 `artifacts/website/shared-canvas-{desktop,mobile}.png`。

原生插件与宿主事务不在本任务的修改或验证范围；该演示不形成新的 AE 资格证据。官网仍只更新 PR，暂不发布。临时预览按 owner 要求保持运行供查看。
