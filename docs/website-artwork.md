# 官网主图生成记录

WEB-01 / 2026-10-20。使用内置 `imagegen` 工具生成，不使用 CLI 或外部图床。

- 风格参考：`web_panel/assets/starfield-presets-banner.png`，仅作为生成参考，原文件未修改。其来源见 [预设主图记录](preset-banner-provenance.md)。SHA256：`313999EC1D0C67CD28B73FB0C13530BDBA4F5A3A2B22F6298FF6CAB2725ECD3A`。
- 原始 PNG：`artifacts/website/imagegen-hero-original.png`，1254×1254，2,677,805 bytes。SHA256：`2B5B486AA3A0860CA32C66D4E607B3E389D2B05438924D51E77557196FEB3165`。原始生成件留在工具输出目录，仓库 artifacts 副本不提交。
- 发布素材：`docs/site/assets/hero-particles.jpg`，1254×1254，391,687 bytes。只做 JPEG quality 88 编码，未裁切或改变构图。SHA256：`AB8A7E650AE177D40EFBAD20E2C9DCF35FD10AE519BC56E244B939235AF9480B`。
- 网页通过 CSS 适配裁切与边缘融合，叠加 12 个错开周期的微小闪烁点；图片本身不旋转。此图为概念营销插画，不是插件渲染或 AE 截图。页面图注与 FAQ 均已标注。

## 完整提示词

```text
Use case: ads-marketing. Asset type: one finished raster hero artwork for the right-hand visual area of the Starfield particle-plugin website. Image 1 is a STYLE REFERENCE, not an edit target: closely match its beautiful, realistic silver/ice-blue particle dust, luminous champagne-gold filaments, tiny white highlights, cinematic depth of field and sophisticated black backdrop. Generate a new square 1:1 image, around 2048 x 2048 pixels. Subject: an organic three-dimensional vortex of flowing particle ribbons, like a richly detailed macro photograph of light sculpture. Asymmetric, intricate, fluid overlapping strands spiral into a small brilliant golden-white core around 60% across and 48% down; broad delicate blue-white streams sweep toward the lower left and upper right. Many tiny sparkling particles, several softly defocused foreground bokeh lights, elegant wispy filaments, realistic depth, high dynamic range, beautiful golden light contrasting cool ice blue, luminous but refined. The leftmost edge and corners fade into near-black charcoal #0a0f14 so the image can blend into a dark page. Fill most of the frame with the detailed particle artwork rather than broad empty space. Match the REFERENCE IMAGE's restrained natural photographic color and detailed light ribbons, rather than simplistic neon shapes. Absolutely NO text, letters, wordmarks, logos, interface, borders, diagrams, perfect geometric ellipses, SVG-style outlines, cartoon dots, or watermark. This is conceptual marketing artwork, not an After Effects screenshot.
```

提示词请求约 2048 方图；工具实际返回 1254 方图，发布素材保持工具实际尺寸。
