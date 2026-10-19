# AE2023 行为证据

范围为 After Effects 2023.5.0 Build 52；不从编译推断更新宿主支持。当前配对和 Windows 注册状态见 [current-state.md](current-state.md)。

| 范围 | 证据边界 |
| --- | --- |
| 新画布 | owner 确认无可见故障 |
| 预设 | owner 接受 panel100 的复测；原先 Particle8 发布读回错误不再作为当前失败记录 |
| 保存/退出模态 | native98 增加宿主 HWND/脚本可用性门槛及单次 quiet-error 配对，owner 随后确认验证无误并授权退役 |
| web_panel 目录迁移 | owner 明确授权；原生内容发现、资源路径、前端和回滚由离线/构建验证；若没有新的实际启动记录，不捏造 native99 宿主运行证据 |
| 逐节点排列、IME/workspace、资源极限和故障/headless | 没有逐项实测记录；保留为后续针对性回归场景，owner 版本接受不等于整张专项矩阵测试通过 |
| GPU/性能/Core 在途切换 | 需硬件/时序/像素等实际证据；离线测试计数不代替测量 |

Motion 的大小写敏感前缀行为保持；修正前缀后实际轨迹的专项记录仍待补充。渲染模式、参数和保存格式不因容器/目录变化而迁移。MFR 和 Compute Cache 继续关闭。

原先重复的“CEP 保持启用”“panel54 palette 待验收”和历史安装版本已从活跃入口移到 [归档](history/2026-10-19/pre-web-panel/docs/compatibility-matrix.md)。历史观察仅覆盖各自日期、版本与场景。新记录应包含准确 host/build、bundle 身份、步骤、实际输出与失败证据。
