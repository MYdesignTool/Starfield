# 官网维护与 GitHub Pages

WEB-01 / 2026-10-20。源码位于 `docs/site/`；中文静态站，无运行时依赖、字体服务、分析脚本或构建工具。部署决策见 [ADR0047](adr/0047-public-website.md)。

## 内容与修改

- `docs/site/index.html`：产品介绍、工作流、功能、入门与 FAQ。
- `docs/site/styles.css`：响应式布局与减少动态效果样式。
- `docs/site/site.js`：移动菜单、独立 Canvas 粒子插画和暂停/页面可见性管理。
- `docs/site/assets/mark.svg`：本站原创星形标识。
- `.github/workflows/pages.yml`：验证与部署，只上传 `docs/site/`。

网页概念动画与插画不调用 Core，也不代表 AE 截图、输出或性能。保持开发中、Windows x64 / AE2023 的范围说明。没有正式安装包时不添加虚假下载入口；官网发布不更改插件的验证与安装状态。

## 验证与预览

仓库根目录运行 `node tests/website-check.cjs`，检查资源和锚点、ARIA 引用、脚本语法、项目子路径、发布目录以及无 SDK/二进制/符号链接混入。检查不安装依赖、不启动 AE 或本地服务器。

可手动用浏览器打开 `docs/site/index.html`。发布前检查桌面和窄屏布局、手机菜单与 Escape、FAQ、动画暂停/播放、系统减少动态效果及外链。静态检查不能替代实际浏览器检查。

## 首次启用

仓库：`MYdesignTool/Starfield`。预期项目地址：`https://mydesigntool.github.io/Starfield/`，仅在部署成功后才视为已上线。

1. 将官网文件、验证脚本和工作流提交到 `main`，或通过 PR 合入。
2. 仓库 **Settings → Pages → Build and deployment → Source → GitHub Actions**。保留现有访问权限；不需要公开额外文件或配置 Secret。
3. 在 **Actions → Starfield website → Run workflow** 选择 `main`。之后修改官网并推送 main 时自动部署；PR 只检查与打包，不部署。
4. 确认 `validate`、`deploy` 成功，再访问部署输出地址验证资源与交互。

工作流依照 [GitHub 官方 Pages 指南](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages)，使用 Pages artifact 与 OIDC 部署。仅 deploy 作业具有 pages/id-token 写权限；不会构建、安装或上传插件。

## 发布状态

初次只读远端检查显示仓库公开、默认分支 main、Pages 尚未启用。本机 Git 连接受失效代理阻挡；浏览器未登录 GitHub。实现、静态验证与实际发布结果分别记录，不能把预期地址当上线证据。

## 回退

官网更改可在 GitHub 对官网专属提交执行 Revert，再由 main 的同一工作流重新部署。该操作无需回退插件、SDK 或 AE 安装。停用 Pages 是独立的远端操作，仅在 owner 要求时执行。
