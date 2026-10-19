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

首版已通过已连接的 GitHub 接口提交至独立分支 `codex/web-01-github-pages`，见 [草稿 PR #1](https://github.com/MYdesignTool/Starfield/pull/1)。未合入 main、未上线。没有推送本地 P-05/P-06/P-07/GOV 在途插件修改；远端 README/任务卡修改从远端 main 分别追加官网入口与本卡，保留各自已有内容。本地 checkout 分支及预先存在的修改保持。

初次只读远端检查显示仓库公开、默认分支 main、Pages 尚未启用。本机 Git 连接受失效代理阻挡；浏览器未登录 GitHub，无法代为更改 Pages Source。首次上线需 owner 登录后选择 GitHub Actions。实现、静态验证与实际发布结果分别记录，不能把预期地址当上线证据。

本地静态验证与脚本语法通过。`artifacts/website/` 保存静态检查及临时 VM 交互 smoke 记录；覆盖动画暂停/恢复、系统减少动态效果、隐藏页面/移出视口停止、像素比上限、菜单/Escape/锚点关闭和无 Canvas 回退。这些是离线验证。远端 [Actions run 39284989514](https://github.com/MYdesignTool/Starfield/actions/runs/39284989514) 已完成且 conclusion=success；PR 的 deploy 按设计跳过，不代表 Pages 上线。后续提交的 CI 仍需分别查看。

Owner 于 2026-10-20 明确批准运行 `node artifacts/website/preview.cjs --serve` 并在检查后停止。临时服务仅监听 `127.0.0.1:4173`，内容仅 `docs/site/`，使用 `/Starfield/` 子路径。沙箱内服务最初不能被浏览器访问；经同命令沙箱外执行后，HTTP 200 与应用内浏览器访问成功。没有修改代理、注册表、系统防火墙或 AE。完成后已 Ctrl+C 停止；只读核验该端口监听数为 0。

实际应用内 Chromium 检查了 1440×1000 桌面、390×844 手机和 320×740 窄屏。标题/按钮/节点图/功能区无横向溢出；手机菜单展开、Escape 关闭并返回焦点、FAQ 展开、动画暂停/恢复通过，控制台 warn/error 为空。按预览调整了粒子可见度。桌面首页、功能区和手机工作流截图保存于 `artifacts/website/{desktop-home,desktop-features,mobile-workflow}.png`。系统减少动态效果和隐藏/离屏状态仍仅有 VM 检查，不补记为浏览器实测。截图接口曾有短暂同步超时，恢复后重新核对并保存最终截图。

## 回退

官网更改可在 GitHub 对官网专属提交执行 Revert，再由 main 的同一工作流重新部署。该操作无需回退插件、SDK 或 AE 安装。停用 Pages 是独立的远端操作，仅在 owner 要求时执行。
