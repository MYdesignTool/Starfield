# 官网维护与 GitHub Pages

WEB-01 / 2026-10-20。源码位于 `docs/site/`；中文静态站，无运行时依赖、字体服务、分析脚本或构建工具。部署决策见 [ADR0047](adr/0047-public-website.md)。

## 当前视觉与内容

四角星标识、疏朗大写 STARFIELD 字标、冷黑背景、银白文字与蓝青交互色。首页使用参考插件预设主图风格生成的蓝白星尘与香槟金光丝图片，叠加 12 个错开周期的微小星光；图片本身保持静止。素材、完整提示词、尺寸与哈希见 [官网主图生成记录](website-artwork.md)。原预设主图及插件前端未修改。

- `docs/site/index.html`：产品介绍、工作流、功能、入门与 FAQ。
- `docs/site/styles.css`：响应式布局、图片融合、CSS 星光与减少动态效果样式。
- `docs/site/site.js`：移动菜单、闪烁暂停/恢复、离屏和页面可见性管理。无 Canvas 绘制、旋转星轨或逐帧脚本循环。
- `docs/site/assets/hero-particles.jpg`：1254×1254 概念主图，391,687 bytes。脚本不可用时仍显示图片与静止星光。
- `docs/site/assets/mark.svg`：本站四角星标识。
- `docs/site/404.html`：独立错误页面，不依赖相对资源，深层失效链接仍可返回官网。
- `.github/workflows/pages.yml`：验证与部署，只上传 `docs/site/`。

概念图片、星光与功能插画不调用 Core，也不代表 AE 截图、输出或性能；图注和 FAQ 已标注。保持开发中、Windows x64 / AE2023 的范围说明。没有正式安装包时不添加虚假下载入口；官网发布不更改插件的验证与安装状态。

## 验证与预览

仓库根目录运行 `node tests/website-check.cjs`，检查资源和锚点、ARIA 引用、脚本语法、项目子路径、发布目录以及无 SDK/二进制/符号链接混入。不安装依赖或启动本地服务器。

当前图片版验证通过：8 个公开文件、12 个本地资源引用，总计 432,380 bytes，单文件保持小于 500,000 bytes。脚本语法通过。忽略目录内 `artifacts/website/image-hero-smoke.cjs` 验证手动暂停/恢复、初始与动态减少动画、隐藏页面、离屏重入、缺失 IntersectionObserver 和菜单/Escape/锚点关闭；这些为 VM 检查，不冒充浏览器实测。旧 `interaction-smoke.cjs` 针对已移除的 Canvas，不再适用于当前主图。

实际应用内 Chromium 核验默认桌面、390×844 手机图片裁切、320×740 窄屏无横向溢出、图片加载、12 个星光的运行状态、暂停/恢复、手机菜单与 Escape；控制台 warn/error 为空。截图保存于 `artifacts/website/image-hero-desktop.png`、`image-hero-mobile.png`。系统减少动态效果、页面隐藏和离屏恢复仍以 VM 证据为准。

Owner 已明确批准 `node artifacts/website/preview.cjs --serve`，仅监听 `127.0.0.1:4173`、仅提供 `docs/site/`、使用 `/Starfield/` 子路径。图片版检查后已 Ctrl+C 停止，只读核验端口监听数为 0。没有修改代理、注册表、防火墙或 AE。撤回这次临时预览只需 Ctrl+C；当前已完成。

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
