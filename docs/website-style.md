# 官网视觉与主图微动效

WEB-01 / 2026-10-21。Owner 要求与自己的 [Ae-ToolKit 官网](https://mydesigntool.github.io/Ae-ToolKit/) 统一视觉，并明确允许参考自有仓库的玻璃实现。

## 参考与实现

已检查实际页面、owner 提供的深色截图，以及仓库 [assets/css/style.css](https://github.com/MYdesignTool/Ae-ToolKit/blob/main/assets/css/style.css)。本次读取的 Git blob 为 `259005b5ff0e5dbc24d1d9d0a56273bb8140699f`。

`docs/site/brand.css` 是 Starfield 的产品家族视觉层：沿用参考站的深蓝底色、蓝紫光晕、系统字体、1120px 内容宽度、24/26px 圆角、蓝色胶囊按钮与玻璃色板。`glass.js` 从 owner 仓库最终玻璃实现移植连续反光与圆角距离场边缘法线贴图；支持 SVG backdrop filter 时使用 10px 模糊/155% 饱和度及边缘位移，其他情况保留 24px CSS 模糊、高光和描边。指针反光只在交互与缓动收敛期间调度帧，停下后无后台循环；不追踪触摸。无脚本时仍有 CSS 玻璃基础表面。

导航固定悬浮在页面上方。功能卡片、工作流步骤、画布外框、入门区、FAQ 和 404 同步玻璃表面。演示 iframe 的外观由 `tools/website-demo/demo.css` 维护并显式导出；没有修改原插件 UI 或另一项目的仓库。保留 Starfield 四角星、粒子主图和开发中/AE2023 范围，不新增参考站的版本、下载或兼容性声明。

### 嵌入画布的清晰度修复

Owner 随后报告工作流画布整体被模糊。检查发现 `.graph-live` 直接携带 `blur(10px) saturate(155%) url("#glass-optics-…")`，iframe 本身没有滤镜。此前仅确认滤镜加载、画布可交互，未充分覆盖 iframe 合成后的视觉清晰度。

当前将 `.graph-live` 的 backdrop-filter 清空，24px 背景模糊仅放在其 `::before` 装饰层，位于 iframe 后方；保留半透明底色、圆角、阴影与一像素高光。`glass.js` 跳过包含 iframe 的表面，不为其祖先注册 SVG 折射。其他玻璃卡片继续使用光学效果。此边界避免嵌入内容进入祖先滤镜的合成路径，没有修改画布源码、缩放或交互。

新增支持 SVG 光学效果时的回归：普通卡片仍被观察和注册，包含 iframe 的卡片不被注册；CSS 检查保证背景模糊位于装饰层。实际应用内 Chromium 在 1112×680 检查清晰度、滚动重绘、添加节点、断开与 Enter 端口重连；390×844 检查清晰度与无横向溢出，warn/error 为空。截图为 `artifacts/website/glass-canvas-sharp.png` 和 `glass-canvas-sharp-mobile.png`。本轮自动化拖动被浏览器工具的小数 iframe 坐标限制拦截，因此不新增拖动实测结论；共享编辑模块与原有拖动证据仍见画布说明。

## 历史修复对照

Owner 指出需要参考原站的修改记录后，检查了提交历史、最终 main.js / visuals.js 和 docs/site-motion.md。初版仅提取颜色与基础模糊，遗漏了下列已修正细节；本轮在提交 PR 前替换为成熟逻辑：

- [3481e25：连续柔和反光](https://github.com/MYdesignTool/Ae-ToolKit/commit/3481e25c9d9dfa0b1960f1fcb245c46146d9bdd2)：进入、快速转向、离开和重入均从当前位置缓动；离开时淡出，每帧最多一次更新。
- [abc04dd：恢复玻璃反射](https://github.com/MYdesignTool/Ae-ToolKit/commit/abc04dd861c1a96a4eb4e4b5dd0435c40b559bd8)：恢复斜向反光带、随指针变化的边缘高光和圆角边缘折射。贴图最长边限制 512px，只在可见和实际尺寸变化时生成。
- [867ddbf：收敛白边外溢与背景分割](https://github.com/MYdesignTool/Ae-ToolKit/commit/867ddbffb7ee3aeb95ae7d1758852ce607b70034)：边缘层限定在一像素圆角描边内，光场统一放在页面背景，避免 section 交界处色块断层。
- [6c0cc4e：窄屏首屏整合](https://github.com/MYdesignTool/Ae-ToolKit/commit/6c0cc4e9da96b7bb6800f084a7a6cf91acb641d7)：手机主图改为文字后的低透明度装饰层，不单独占据一整屏，前景按钮保持可操作。保留 Starfield 的暂停与概念图说明。
- [a79b78f：玻璃保持实时](https://github.com/MYdesignTool/Ae-ToolKit/commit/a79b78fa415836a18ef041b064d21f640569f967)：本页不引入包裹玻璃的快照/模糊过渡。语言切换和工具箱 Logo 的 GPU 渲染不在本任务范围。

另外在手机实测中修正了渐变字继承深色 text-shadow 后变暗的问题：标题使用外部 drop-shadow，字面不继承遮挡渐变的阴影。

## 微动效

保留原 JPEG 字节与来源，只通过 CSS 让图片在 22 秒周期内缓慢漂移（水平约 −6～10px、垂直约 −7～4px）并在 1.015～1.055 倍之间缩放；没有旋转。叠加 14 秒周期的轻微蓝紫光感变化，原 12 个错开明灭的星光继续保留。图片边缘以渐隐遮罩融入背景。

暂停按钮统一控制图片、光感层和星光。手动暂停不因重新进入视口自动取消；离屏、隐藏页面和系统减少动态效果均停止动画。无脚本时图片、光感和星光静止；系统减少动态效果时不显示播放按钮。仅运行时为图片设置 transform 的 will-change；图片不用逐帧脚本或视频资源。

## 验证

- `node tests/website-check.cjs`、`node tests/website-graph-check.cjs`、`node tests/website-glass-check.cjs`、`node tools/Build-WebsiteDemo.cjs` 通过。官网复制产物 0 差异；不重建插件。玻璃回归覆盖连续进入/转向/离开/重入、帧合并、空闲停止、不拦截触摸、减少动态效果、页面隐藏及 CSS 光学回退。
- `artifacts/website/image-hero-smoke.cjs` 的 VM 检查通过：手动暂停/恢复、初始与动态减少动画、页面隐藏、离屏重入、缺失 observer 与菜单关闭。CSS 减少动画规则继续覆盖全部动画。
- 实际 Chromium 检查桌面首页、玻璃卡片和指针反光，header/card 的 SVG backdrop filter 已加载且描边 mask-composite=exclude；图片/光感/星光均在运行，暂停后全部 paused；滚到工作流后主图实际 paused。390px 紧凑首屏、固定菜单/Escape、玻璃画布添加与撤销、320px 无横向溢出通过，控制台 warn/error 为空。系统减少动态效果和页面隐藏仍按 VM 证据记录，不补记为浏览器模拟实测。
- 截图位于 `artifacts/website/glass-{home,features,mobile-home,mobile-workflow}.png`。预览按 owner 要求保持在 `127.0.0.1:4173`，仅更新原 PR，未发布。
