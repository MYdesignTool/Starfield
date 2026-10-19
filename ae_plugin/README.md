# AE 适配层

当前配对和实际验证见 [当前状态](../docs/current-state.md)；构建/发布见 [构建说明](../docs/build-matrix.md)。

| 模块 | 职责 |
| --- | --- |
| StarfieldHost、WebPanelHost/Protocol | AEGP/WebView2 面板、受限本地内容、有界 UI 排队与安全脚本执行 |
| EffectMain、PluginFlags/Version、PiPL | selector、能力宣告与打包身份 |
| Parameters、GraphParameter、NativeGraphCommit | 参数、图持久化、提交及回滚 |
| NodeEffects、各 layout、NodeGraphSync | 独立原生作者、动画绑定及事务 |
| SmartRender、NativeTemporalCache、历史/资源捕获 | 不可变预渲染快照、历史和宿主资源 |
| Camera、MotionBlur、WorldBridge、GpuRender | 宿主几何、曝光、像素和设备协商 |
| CoreLoader | 同 generation 的 Core lease 与结果生命周期 |

前端位置为 web_panel/，不依赖 CEP 注册。AE 类型、suite 和句柄只停在适配层；C ABI 不传 C++/AE 对象，/MT 模块内配对分配/释放。Output 为主 renderer，Auxiliary 是 Emitter 的结构模式。Appearance 和旧浮动编辑器已退役。GPU F32 按设备/帧协商；MFR 和 Compute Cache 关闭。

从根目录执行 BuildWindows.ps1 -NoDistPublish -NoRuntimePublish 构建候选；本地 SDK 和所有输出不入 Git。主 AEX 同样包含构图和冻结快照需要的共享 Core 源码，不能把所有求值描述为 DLL 独占。
