# Starfield

面向 After Effects 2023 的独立粒子插件。AEGP Panel + WebView2 提供节点画布和独立预设窗口；参数、曲线和渐变通过 AE Effect Controls 编辑。Core 不依赖 Adobe SDK。Appearance 和旧 CEP 容器已退出当前产品入口。

## 当前入口

源码前端目录为 `web_panel/`，不要重建 `cep_panel/` 或自动安装 CEP。当前源码/安装配对、验证边界、注册清理状态及回滚命令统一见 [当前状态](docs/current-state.md)。历史候选和旧 passing counts 不代表当前版本。

- [文档索引](docs/README.md)、[架构](docs/architecture.md)、[任务卡](docs/agent-backlog.md)、[路线图](docs/roadmap.md)
- [前端](web_panel/README.md)、[AE 适配层](ae_plugin/README.md)、[编辑器配置](docs/editor-setup.md)
- [构建与回滚](docs/build-matrix.md)、[测试](docs/testing.md)、[宿主证据](docs/compatibility-matrix.md)

## 构建

从仓库根目录执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/Prepare-WebView2Sdk.ps1 -Download
powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -NoDistPublish -NoRuntimePublish
node tests/RunPanelTests.cjs
```

使用本地 May2023 SDK、MSVC v145 和固定的 WebView2 SDK；以上构建不发布。临时文件、测试夹具、输出和备份位于忽略的 `artifacts/`。SDK、旧实现及构建输出不随 MIT 源码分发。部署和一切宿主改动遵循 [ADR0011](docs/adr/0011-host-authorization.md)。
