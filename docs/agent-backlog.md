# 当前任务卡

## WEB-01 — Starfield 官网与 GitHub Pages

Owner 于 2026-10-20 要求开始实现官网，并使用 GitHub Pages。拥有 docs/site/、.github/workflows/pages.yml、tests/website-check.cjs、tests/website-graph-check.cjs、tests/website-glass-check.cjs、tools/Build-WebsiteDemo.cjs、tools/website-demo/、docs/website.md、docs/website-artwork.md、docs/website-canvas.md、docs/website-style.md、ADR0047，以及 README/文档索引/本卡中的官网入口。采用独立静态页面，仅发布 docs/site/；不将 SDK、插件二进制、宿主前端、内部文档或 artifacts 打包入站点。不修改插件、参数/schema、渲染或现有部署。产品内容以当前架构与验证边界为准；网页粒子与节点图均为明确标注的独立演示，不冒充 AE 渲染或截图。不承诺公开安装包、其他宿主或未经验证的性能。验证静态资源、链接、脚本与 Pages 发布配置；远端发布和浏览器实测仅在实际成功后记录。

首版、响应式样式与动效已实现，本地静态检查、脚本语法及 VM 交互 smoke 通过；PR #1 位于 codex/web-01-github-pages。Owner 批准临时预览后，实际浏览器完成桌面/390/320 窄屏、菜单/Escape/FAQ/动画检查，临时服务已停止并核验监听数 0。远端 CI run39284989514 已成功，PR deploy 按设计跳过；后续提交单独检查。未上线，Pages 后续设置见下文。发布步骤与精确验证边界见 docs/website.md。本卡不关闭任何 AE 宿主门。

Owner 后续明确先完成代码/PR、暂不上线，并告知 Pages Source 已设为 GitHub Actions，要求先完成视觉检查。本轮补齐 SVG 轨道封面、404 和可读样式，视觉/资源复核后更新 PR；不合并 main 或发布。精确 CI 与浏览器证据统一见 docs/website.md。

Owner 再要求四角星标识并参考插件预设主图风格。官网改为银蓝四角星与大写字标、冷黑/银白/星蓝界面，粒子采用蓝白与少量香槟金光轨；保留原预设主图和插件前端。同步静态 SVG、favicon、404、功能图与动效，修正焦点伪选择器；桌面和390/320手机复核通过。本次仍只更新 PR，不上线。

Owner 认为颜色过素，进一步提高电蓝/青色饱和度与金色粒子比例，增加蓝青按钮/标题渐变和金/青/蓝紫功能插画。共享色彩规则已在桌面和手机确认生效；同步 SVG、favicon、404，更新同一 PR，仍不发布。

Owner 后续否决程序化旋转星轨，要求改用图片并叠加闪烁粒子。当前主图为内置 imagegen 参照预设主图生成的本地 JPEG，配 12 个错开周期的 CSS 星光；移除 Canvas 星轨与旧 orbit.svg。桌面/390 手机/320 窄屏、暂停恢复与菜单检查通过；减少动态效果、隐藏页面与离屏恢复经 VM 验证。图源与提示词见 docs/website-artwork.md。临时预览已停止并核验监听数 0；只更新原 PR，仍不上线。

Owner 再要求官网可操作节点 UI，并明确复用现有画布代码。复用五个纯前端模块、样式和 46 个 UI 函数，在 sandbox iframe 中以独立内存适配器替代宿主读写/提交。支持添加、拖动、框选、复制、连接/断开、缩放、删除、撤销和重置；不加载桥接、事务、轮询、参数/预设/文件或渲染模块。生成器默认报告，--write 仅更新官网副本；原 web_panel 不修改。共享编辑模块及隔离检查、桌面/390px 浏览器操作通过，精确证据见 docs/website-canvas.md。预览按 owner 要求保持运行供体验；仍只更新 PR，暂不上线。

Owner 于 2026-10-21 要求图片有微动效，并与自有 Ae-ToolKit 官网统一玻璃美学，明确允许读取自有仓库实现。新增 brand.css 产品家族视觉层，统一深蓝/蓝紫、悬浮玻璃导航、卡片、按钮、字重、画布外框与404。主图采用慢速漂移/轻微缩放/光感呼吸，与星光共用暂停、离屏/隐藏/减少动画控制；不旋转、不更改图片字节。桌面/390/320、暂停及离屏、画布添加撤销检查通过；来源与边界见 docs/website-style.md。仅更新官网 PR，预览继续供体验，暂不发布。

Owner 随后要求对照 Ae-ToolKit 当时的修改记录。已逐项检查连续反光、反射恢复、白边裁剪、窄屏整合与实时玻璃相关提交，移植最终反光/圆角法线折射逻辑，补充进入/快速转向/离开/重入等回归，并修正紧凑手机首屏的渐变字阴影。完整对照链接与实际验证见 docs/website-style.md；原站与插件文件均不修改。

Owner 报告嵌入画布被模糊后，将背景模糊移至 iframe 后方的独立装饰层，画布祖先不再携带 backdrop/SVG 折射。补充支持光学效果路径中的 iframe 排除回归；桌面/手机清晰度、实际嵌入添加/断开/重连、静态及共享画布检查通过。精确证据与自动化拖动限制见 docs/website-style.md；只更新同一 PR，预览继续运行，未发布。

## P-04 — AEGP Panel + WebView2 已完成

旧 CEP 注册已在 owner 要求重发 UAC 后移出扫描目录；web_panel 迁移及已验证检查点已合入本地 main。源码/安装配对、精确注册路径、证据和恢复命令见 [current-state.md](current-state.md)。本卡不再存在待执行的管理员清理或 main 合并步骤。

Owner 已接受 native98/panel100 的节点画布、预设及模态保存退出复测，并要求退役旧 CEP、合入本地 main、将共享前端改为 web_panel、整理过时文档和仅保留最近几个回滚版。本次采用最近 5 个实际部署备份策略。

本次拥有范围：WebPanelHost/Protocol 与 Host 集成、前端传输和当前预设验证修复、路径迁移涉及的原生资源/构建/测试/部署/回滚工具、.vscode/、活跃文档、旧 CEP 精确注册退役及备份保留工具。后续配对及状态只在 [current-state.md](current-state.md) 维护。

约束：不改变节点行为、参数身份、schema、动画、Core ABI 或渲染；不恢复旧属性编辑器/Appearance。UI/JSX 保留 panel100 generation。native99 只为目录迁移重新构建。退役只操作本项目已核对 Junction；Windows 注册表、其他扩展与共享缓存不改动。

验收与授权分别记录：owner 的版本接受是 release decision；未逐项报告的 IME、硬件、headless、性能和所有节点排列不补记为实际通过。旧 CEP 退役需 Windows 管理员权限；实际未成功时不能标记完成。保持默认报告、显式行动和一条恢复命令。

## 后续任务

无默认的新功能实现指派。Motion 前缀修正后的实际轨迹、硬件/性能等专项证据按 [路线图](roadmap.md) 与 [行为验收](compatibility-matrix.md) 安排。MNT-01 的旧范围不再限制 owner 本轮明确授权的文档及旧备份整理。

旧文档中的多张重复 Active M3-20、旧暂停指令和逐版本交接已移入 [历史任务卡](history/2026-10-19/pre-web-panel/docs/agent-backlog.md)。它们是证据，不是当前 agent 的执行指令。保留预先存在的工程分析草稿、日志和无关工作树改动，不混入本次提交。
