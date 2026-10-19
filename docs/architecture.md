# Starfield 架构

## 数据流

```text
AEGP Panel + WebView2 (web_panel)
  -> 有界消息队列 -> 安全 UI idle -> 既有 JSX/native 作者事务
  -> 原生节点参数、UUID/revision/checksum 与主效果图持久化
  -> SmartFX 不可变采样/历史/资源快照
  -> Core generation 的 C ABI / CPU 或 GPU 数值场景
  -> AE 输出世界
```

| 模块 | 职责 |
| --- | --- |
| StarfieldHost.aex | AEGP 面板、WebView2 生命周期、UI 排队、Model/预设文件服务及互斥 |
| StarfieldParticle.aex | 主渲染效果、ARB 图持久化、SmartFX、Core 加载和像素转换 |
| 各节点 AEX | 隐藏菜单的原生作者参数、持久化/动画绑定，SmartFX 透传输入 |
| StarfieldCore.dll | 宿主无关图求值、确定性模拟、CPU 栅格化和 GPU 场景准备 |
| web_panel/ | 画布/预设、选择/拓扑、守卫事务、自适应检查及传输适配 |

Output 无独立 AEX；Auxiliary 使用 Emitter 的结构模式。参数、曲线和渐变使用 AE Effect Controls；旧浮动编辑器及 Appearance 不恢复。

## 所有权与执行

Settings.hpp 拥有 Core 值类型，Render.hpp 是宿主无关渲染边界。AE suite、句柄、世界指针停在 ae_plugin/。主 AEX 也编译构图和冻结快照所需的共享 Core 代码，不能把所有求值描述为 DLL 独占。C ABI 不传 C++/AE 对象，分配与释放在同一 generation/CRT 内完成。

WebView2 回调只保存有界自有数据；只允许本地映射的两个页面发送请求。UI idle 检查线程、重入、插件互斥、有效且启用的 AE 主 HWND 和脚本可用性。模态期间保持 FIFO 与原截止时间，过期或旧页面请求不执行；单次 ExecuteScript 的 quiet-error 状态成对恢复，不持久修改全局偏好。关闭、导航和重放保护见 ADR0046。

图与节点身份、磁盘参数 ID、schema、动画、读回/回滚及 Core ABI 不因容器迁移而变化。按绝对时间与稳定身份求值，图/历史/几何及 Core lease 在 render 完成前保持有效。Core 输入为数值和拥有明确生命周期的资源；文件和宿主操作在适配层。

SmartFX 尊重 rowbytes、PAR、downsample、ROI 与 AE 8/16/32-bpc 语义。GPU 按设备/帧协商；MFR 和 Compute Cache 仍关闭，不从编译推断真实设备或新版宿主资格。格式、Motion/Transform/Model 的具体契约以 Settings/Render/PluginApi/schema 和对应 ADR 为准，不复制历史 ABI/snapshot 数字。

## 当前与历史

当前配对与证据仅见 [current-state.md](current-state.md)。原先混入的部署流水、旧安装版本和未发布候选叙述已移入 [2026-10-19 归档](history/2026-10-19/pre-web-panel/docs/architecture.md)，不能据此恢复旧 CEP 或旧包。变更范围与授权以 [任务卡](agent-backlog.md) 和 ADR0011 为准。
