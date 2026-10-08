# Starfield

面向 After Effects 2023 的独立粒子插件实现。Core 不依赖 Adobe SDK；AE 适配层负责宿主参数、图持久化、SmartFX 和像素转换，CEP 提供节点画布、属性编辑和预设管理。

## 当前状态

源码包含确定性发射、Particle/Force/Auxiliary 链、相机与深度投影、程序形状、曲线/渐变、原生动画采样、CPU/GPU 路径和 Motion Blur。Appearance 已移除。Transform 的 Core、原生与 CEP 候选已实现并随 native54/panel54 配对部署；AE 2023 行为验收仍开放。

源码候选与已部署包是不同检查点。版本、安装哈希、测试结果和未完成的宿主验收统一记录在 [当前状态](docs/current-state.md)。不要从旧文档的测试计数或构建成功推断当前 AE 行为通过。

## 构建与测试

从仓库根目录运行：

```powershell
node tests/RunPanelTests.cjs
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunCoreTests.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunAllTests.ps1
```

测试选择和日志位置见 [测试说明](docs/testing.md)。纯 Core 可由 CMake 构建；AE 原生目标使用本地 May 2023 SDK 和 MSVC v145：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ae_plugin/BuildWindows.ps1 -NoDistPublish -NoRuntimePublish
```

上面的命令只构建候选。发布、安装与回滚规则见 [构建说明](docs/build-matrix.md) 和 [ADR 0011](docs/adr/0011-host-authorization.md)。

## 项目入口

- [文档索引](docs/README.md)、[架构](docs/architecture.md)、[路线图](docs/roadmap.md)、[任务卡](docs/agent-backlog.md)
- [AE 适配层](ae_plugin/README.md)、[CEP 面板](cep_panel/README.md)、[编辑器配置](docs/editor-setup.md)
- [参数映射](docs/parameter-mapping.md)、[行为验收](docs/compatibility-matrix.md)、[参考行为清单](docs/reference-inventory.md)

临时文件、测试夹具、日志和编译输出均放在忽略的 artifacts/。清理工具默认只列清单；显式 -Clean 才删除可再生成缓存，保留部署包、回滚备份和证据，见构建说明。

本项目采用 [MIT 许可](LICENSE)。Adobe SDK 是本地构建输入，不随仓库分发；父目录旧实现与逆向资料仅作参考，不参与构建。
