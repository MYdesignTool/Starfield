# 开发路线图

当前实现和部署以 [current-state.md](current-state.md) 为准；这里不重复旧版本/哈希。

| 范围 | 当前处理原则 |
| --- | --- |
| P-04 容器迁移 | owner 已接受画布、预设修复及模态退出复测；同意源目录改为 web_panel、重新构建并退役旧注册。实际注册状态见当前状态 |
| 已实现节点 | 保留现有行为，不因迁移扩大功能或恢复浮动属性编辑器；参数通过 AE Effect Controls 编辑 |
| Motion Light Path | 大小写敏感 Starting With 规则保留；修正前缀后的实际粒子轨迹没有独立专项记录，不从容器验收推断数值 parity |
| 硬件、性能和生命周期扩展 | GPU 实际设备/CPU 像素对照、在途 Core 切换、IME/workspace、故障/headless 等按明确场景补证据；没有新增用户需求时不自行扩展功能 |
| 后续行为族 | 参考行为清单只描述候选范围，体积、新材质/网格和其他 Motion/Turbulence 行为须单独立卡 |
| 支持范围 | 当前仅 AE2023；MFR、Compute Cache 和新版宿主需独立任务与 qualification |

旧的 native54/panel54 等“当前未完成”清单、逐次部署和撤回流水已归档，见 [历史路线图](history/2026-10-19/pre-web-panel/docs/roadmap.md)。历史开放项并未自动逐条测试通过；也不能把已被后续实现替代的候选状态继续当作当前缺失功能。
