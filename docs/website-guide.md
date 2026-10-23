# 使用文档站维护

WEB-01 / 2026-10-23。Owner 要求搭建面向普通用户的官方文档框架，细节暂不填充，明确要求复用开源框架。

## 结构与维护

采用 [Docsify 4.13.1](https://github.com/docsifyjs/docsify/tree/v4.13.1) 的 Markdown 渲染、hash 路由、侧栏和官方全文搜索插件。入口为 docs/site/guide/index.html，官网导航已加入入口。支持 /Starfield/guide/ 和 /Starfield/guide/index.html#/nodes/emitter；GitHub Pages 无需重写规则。

共有 35 个用户页面：首页 + 开始使用 5、工作界面 4、画布操作 5、节点参考 10、外观与动画 3、预设 3、帮助 4。Emitter、Auxiliary、Particle、Force、Transform、Motion、Turbulence、Model、Material、Output 均有独立入口。页面只保留阅读目的、待填小节和前后篇链接，不编造安装步骤、参数默认值、兼容承诺或行为细节。

_sidebar.md 是目录，README.md 是首页，_404.md 是内部缺失路由回退。config.js 配置 Docsify、搜索路径、页面标题和键盘适配；guide.css 在上游主题和官网共享 brand.css 上适配阅读布局。深蓝光场、四角星、悬浮导航、圆角玻璃保持统一；正文不模糊。手机选择章节或按 Escape 后收起目录。

直接编辑对应 Markdown，不运行脚手架。新增或更名页面时同步侧栏、前后篇链接、config.js 搜索路径和 tests/website-docs-check.cjs 页数。写完内容后移除占位提示；全站状态目前在 config.js 渲染 hook 中，正式写作阶段改为逐页状态。搜索索引仅缓存公开文档，使用独立 localStorage namespace，缓存一小时；结构大改时更新 namespace，不读取 AE 或用户工程。工程资料与用户指南分开维护。

## 上游资源

vendor/ 保存固定版本运行时、官方 search 插件、Vue 基础主题和 MIT 许可证。运行时不依赖 CDN 或远端字体。主题只删除 Google Fonts @import，其他定制在 guide.css。原始地址和当前 SHA256 见 vendor/NOTICE.txt。升级需核对版本、许可和哈希并重新验证，不用浮动版本。Docsify 需要 JavaScript；搜索引擎预渲染不在本轮范围。

## 验证与发布

website-docs-check 检查 35 个导航和搜索路由、10 类节点、78 条正文链接及草稿边界；website-check 允许公开 Markdown，继续检查资源、语法、大小和发布目录。Pages 增加此检查，目录仍仅 docs/site，PR 不部署。原画布和玻璃检查通过；未重新导出正在修改中的插件画布副本。

Chromium 检查桌面/390px 首页、Emitter 搜索和结果跳转、深链接刷新、章节标题、手机目录与 Escape、无横向溢出；warn/error 为空。截图为 artifacts/website/docs-{home,mobile}.png。沿用 owner 的 127.0.0.1:4173 预览授权，session 25762 按此前要求保留；Ctrl+C 停止。仍只更新原 PR，不合并或发布。
