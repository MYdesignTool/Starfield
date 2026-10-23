# 官网维护与 GitHub Pages

WEB-01 / 2026-10-21。源码位于 `docs/site/`；中文静态站；官网不依赖框架，使用文档采用本地固定版本 Docsify，无字体服务、分析脚本或构建工具。部署决策见 [ADR0047](adr/0047-public-website.md)。

## 用户使用文档（2026-10-23）

新增 docs/site/guide/：基于 Docsify 4.13.1，7 个分区、35 个用户页面、全部 10 类节点参考、搜索、章节目录、前后篇与缺失页回退。具体说明暂不填充，页面明确标注待完善。共享官网玻璃视觉；来源、维护及验证见 [文档站说明](website-guide.md)。同一 Pages 目录发布，仍只更新原 PR、暂不上线。预览 session 25762 保留供体验。

## 当前视觉与内容

四角星与 STARFIELD 字标保留，视觉与 owner 自有 Ae-ToolKit 官网统一：深蓝/蓝紫光晕、悬浮玻璃导航、半透明圆角卡片、描边高光和蓝色胶囊按钮。首页保留原蓝白星尘/香槟金图片，加入缓慢漂移、轻微缩放与光感呼吸，统一控制原 12 个星光。参考仓库与动效精确范围见 [视觉实现记录](website-style.md)；素材、完整提示词、尺寸与哈希见 [主图生成记录](website-artwork.md)。原预设主图及插件前端未修改。

- `docs/site/index.html`：产品介绍、工作流、功能、入门与 FAQ。
- `docs/site/styles.css`：响应式布局、图片融合、CSS 星光与减少动态效果样式。
- `docs/site/brand.css`：产品家族视觉、玻璃表面、图片缓动与响应式适配。
- `docs/site/glass.js`：按原站历史修复移植的连续玻璃反光与可见区域边缘折射，空闲时停止帧调度。
- `docs/site/site.js`：移动菜单、主视觉暂停/恢复、离屏和页面可见性管理。主图不使用 Canvas 绘制或逐帧脚本循环。
- `docs/site/demo/`：复用插件画布 UI 的隔离演示，支持添加、拖动、连接/断开与删除、撤销、重置。内存适配器替代宿主提交，不生成粒子；复用与维护见 [节点画布说明](website-canvas.md)。不影响主图星光。
- `docs/site/assets/hero-particles.jpg`：1254×1254 概念主图，391,687 bytes。脚本不可用时仍显示图片与静止星光。
- `docs/site/assets/mark.svg`：本站四角星标识。
- `docs/site/404.html`：独立错误页面，不依赖相对资源，深层失效链接仍可返回官网。
- `.github/workflows/pages.yml`：验证与部署，只上传 `docs/site/`。

概念图片、星光、功能插画及 UI 画布不调用 Core，也不代表 AE 输出或性能；图注和 FAQ 已标注。节点画布复用自有前端代码，宿主桥、事务和文件操作均不加载。保持开发中、Windows x64 / AE2023 的范围说明。Owner 于 2026-10-23 指定首屏主按钮为“前往下载”，直接链接 GitHub Releases，即使尚无发布也保留入口；次按钮为“使用文档”，进入 guide/index.html，顶部入口保留。页面不宣称已有安装包，FAQ 保留开发阶段说明；官网发布不更改插件的验证与安装状态。

## 验证与预览

仓库根目录运行 `node tests/website-check.cjs`，检查资源和锚点、ARIA 引用、脚本语法、项目子路径、发布目录以及无 SDK/二进制/符号链接混入。不安装依赖或启动本地服务器。

当前玻璃/微动效版本验证通过：本轮加入文档前为 19 个公开文件、24 个本地资源引用、684,302 bytes；文档版大小以 website-check 当前输出为准，单文件保持小于 500,000 bytes。脚本语法与共享画布检查通过。主视觉控制、菜单、减少动态效果、隐藏页面等 VM 检查与本轮实际浏览器证据见 [视觉实现记录](website-style.md)。画布模糊修复将背景滤镜移到 iframe 后方的装饰层，并排除其祖先的 SVG 折射；桌面/手机清晰度及嵌入画布添加、断开、重连已复核。旧 `interaction-smoke.cjs` 针对已移除的主图 Canvas，不适用于当前主图。画布证据见 [画布说明](website-canvas.md)。

实际应用内 Chromium 核验默认桌面、390×844 手机图片裁切、320×740 窄屏无横向溢出、图片加载、12 个星光的运行状态、暂停/恢复、手机菜单与 Escape；控制台 warn/error 为空。截图保存于 `artifacts/website/image-hero-desktop.png`、`image-hero-mobile.png`。系统减少动态效果、页面隐藏和离屏恢复仍以 VM 证据为准。

Owner 已明确批准 `node artifacts/website/preview.cjs --serve`，仅监听 `127.0.0.1:4173`、仅提供 `docs/site/`、使用 `/Starfield/` 子路径。图片版初次检查后已停止；owner 随后要求打开预览供体验，目前保留运行。停止只需 Ctrl+C（当前 exec session 58360）；没有修改代理、注册表、防火墙或 AE。

后续发布前仍应检查桌面和窄屏布局、手机菜单与 Escape、FAQ、星光暂停/播放、系统减少动态效果及外链。静态检查不能替代实际浏览器检查。

## 发布与状态

仓库 `MYdesignTool/Starfield`，分支 `codex/web-01-github-pages`，见 [PR #1](https://github.com/MYdesignTool/Starfield/pull/1)。仅更新代码和 PR，未合入 main、未上线。没有推送本地其他在途插件修改；远端任务卡仅同步 WEB-01 段落。

Owner 要求先完成代码与 PR、暂不上线，随后告知已将 Pages Source 设为 GitHub Actions，并要求先检查视觉。配置完成不视为发布授权；不得自行合并 main 或触发生产部署。历史色彩版本的 CI run 39415341022 已成功；图片版提交的 CI 以其自身 Actions 结果为准。

得到发布指令后：

1. 将官网 PR 合入 `main`。
2. 由 main 推送触发工作流，或在 **Actions → Starfield website → Run workflow** 选择 `main`。PR 只检查与打包，不部署。
3. 确认 `validate`、`deploy` 成功，再访问工作流输出地址检查资源与交互。预期项目地址 `https://mydesigntool.github.io/Starfield/` 不能单独作为上线证据。

工作流依照 [GitHub 官方 Pages 指南](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages)，使用 Pages artifact 与 OIDC 部署。仅 deploy 作业具有 pages/id-token 写权限；不会构建、安装或上传插件。工程文档与生成记录不进入 Pages artifact。

## 回退

官网更改可在 GitHub 对官网专属提交执行 Revert，再由 main 的同一工作流重新部署。无需回退插件、SDK 或 AE 安装。停用 Pages 是独立远端操作，仅在 owner 要求时执行。
