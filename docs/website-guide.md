# 使用文档站维护

WEB-01 / 2026-10-23。Owner 先要求面向普通用户的开源文档框架，随后要求开始完善稳定概念，保留可能变化的操作细节。

## 结构与维护

采用 [Docsify 4.13.1](https://github.com/docsifyjs/docsify/tree/v4.13.1) 的 Markdown 渲染、hash 路由、侧栏和官方全文搜索插件。入口为 docs/site/guide/index.html，官网导航已加入入口。支持 /Starfield/guide/ 和 /Starfield/guide/index.html#/nodes/emitter；GitHub Pages 无需重写规则。

共有 35 个用户页面：首页 + 开始使用 5、工作界面 4、画布操作 5、节点参考 10、外观与动画 3、预设 3、帮助 4。当前 33 页有基础概念、阅读思路与使用边界，安装和 Auxiliary 操作两页保留草稿状态。全部 10 类节点均有入口；Auxiliary 仅补充父/子粒子基本定位，不固定新回连的操作步骤。

基础说明覆盖三种界面的职责、最小粒子链、粒子/模型端口、发射与寿命/数量上限、基础值与生命曲线、合成时间与粒子年龄、Transform 分布缩放/粒子大小、预设添加/替换/工程备份、排查与反馈。按钮位置、快捷键、逐项参数、默认值及安装流程继续延后，不编造兼容保证。

_sidebar.md 是目录，README.md 是首页，_404.md 是内部缺失路由回退。config.js 配置 Docsify、搜索路径、页面标题和键盘适配；guide.css 在上游主题和官网共享 brand.css 上适配阅读布局。深蓝光场、四角星、悬浮导航、圆角玻璃保持统一；正文不模糊。手机选择章节或按 Escape 后收起目录。

直接编辑对应 Markdown，不运行脚手架。新增或更名页面时同步侧栏、前后篇链接、config.js 搜索路径和 tests/website-docs-check.cjs 页数。config.js 的 pageStatus 按路由区分 foundation/draft；界面显示“基础说明 · 操作细节持续更新”或“章节完善中”，不将基础说明标为完整教程。搜索 namespace 已更新，旧草稿索引不会覆盖本轮内容；仅缓存公开文档一小时，不读取 AE 或用户工程。

## 内容依据与限制

本轮只读核对当前 architecture/roadmap/current-state、Settings.hpp、graph_edits.js 的连接校验及 presets.html，并参考 ADR0015/0016/0021/0029/0032/0034/0039/0041/0042。旧 ADR 的 CEP/Appearance 操作不带入用户文档，当前界面以 AEGP + WebView2 和 AE 效果控件为准。AE 动画术语参考 after-effects 技能的 animation-basics 资料，自行表述，不复制 SDK、反编译内容或内部数据结构。

重要范围：不公开 P-08 尚待实际使用确认的回连细节；Motion 只介绍三种模式的意图，不提供未验证的来源/单位/插值配方；Turbulence 说明当前 Noise 不会单独改变渲染；Model/Material 使用当前新建连接方向，只描述基础材质。文字基于当前实现契约，不构成新的 AE 宿主、设备或性能验收。

## 上游资源

vendor/ 保存固定版本运行时、官方 search 插件、Vue 基础主题和 MIT 许可证。运行时不依赖 CDN 或远端字体。主题只删除 Google Fonts @import，其他定制在 guide.css。原始地址和当前 SHA256 见 vendor/NOTICE.txt。升级需核对版本、许可和哈希并重新验证，不用浮动版本。Docsify 需要 JavaScript；搜索引擎预渲染不在本轮范围。

## 验证与发布

website-docs-check 检查 35 个导航和搜索路由、33 基础/2 草稿状态、10 类节点与 143 条正文链接；website-check 继续检查资源、语法、大小和发布目录。目录仍仅 docs/site，PR 不部署；未重新导出正在修改中的插件画布副本。

Chromium 检查桌面/390px 首页、Emitter 搜索和结果跳转、深链接刷新、章节标题、手机目录与 Escape、无横向溢出；warn/error 为空。截图为 artifacts/website/docs-{home,mobile}.png。沿用 owner 的 127.0.0.1:4173 预览授权，session 25762 按此前要求保留；Ctrl+C 停止。仍只更新原 PR，不合并或发布。

基础正文版本另核对桌面“认识 Starfield”的表格、搜索“生命进度”并跳转动画小节、390px 正文与术语表无溢出、安装页显示草稿标签；warn/error 为空。增加暗色表格样式与 Docsify topMargin:130，给固定导航留出定位空间。截图为 artifacts/website/docs-foundations{,-mobile}.png。该浏览器检查不等于 AE 操作或节点渲染验收。
