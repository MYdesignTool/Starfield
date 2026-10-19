# AE 适配层

当前候选与安装版本见 [当前状态](../docs/current-state.md)；构建/发布规则见 [构建说明](../docs/build-matrix.md)。旧 Build31 引言和27控制/Appearance描述已移入历史记录。

| 文件 | 职责 |
| --- | --- |
| EffectMain、PluginFlags、PluginVersion、PiPL | selector、宣告能力与打包身份 |
| Parameters、GraphParameter | 参数注册/checkout、ARB graph 复制/保存/恢复 |
| GraphCarrier、NativeGraphCommit、NodeGraphSync | 数值提交收据、原生编译与作者事务 |
| NativeNodeGraph、NativeTemporalCache/UI、EmitterHistoryCapture | UI 绑定准备、render 数值采样与历史 |
| NodeEffects、NodeRecord、各 layout | 独立原生作者效果、磁盘身份与曲线/渐变布局 |
| TransformBinding/Layout | Transform 和 Null 资源/数值绑定；实际宿主验收开放 |
| Camera、MotionBlur、SmartRender、WorldBridge、GpuRender | 宿主几何/曝光、SmartFX、世界转换与设备协商 |
| CoreLoader、Diagnostics | 不可变 Core lease、选择文件及诊断 |
| StarfieldHost | UI idle 初始化 |

AE 类型与句柄只存在于此目录；C ABI 不传 C++/AE 对象。主 AEX 也包含图编解码及冻结历史所需的共享 Core 源码，不是所有求值都独占 DLL。/MT 模块内分配/释放，generation/result 生命周期由 CoreLoader/SmartRender 管理。
节点 AEX 是菜单隐藏的普通参数容器并支持 SmartFX 透传；Output 是主 renderer，Auxiliary 是 Emitter 的结构模式。Appearance 已退役。
原生 GPU F32 已声明并受设备/帧 gate 约束；MFR/Compute Cache 仍关闭。宿主观察见 [行为验收](../docs/compatibility-matrix.md)。

从根目录使用 BuildWindows.ps1 -NoDistPublish -NoRuntimePublish 编译候选。MSBuild/PiPL 使用该脚本的路径处理；C++ 测试现在使用相对响应文件而不创建临时盘符。所有输出在 artifacts，SDK 不入 Git。
