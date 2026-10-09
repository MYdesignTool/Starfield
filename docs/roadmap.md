# 开发路线图

范围仍是 AE2023。已实现功能和部署版本见 [当前状态](current-state.md)；旧版本修复经过留在 [历史记录](history/README.md)。

## 当前未完成工作

| 任务 | 下一步 | 关闭条件 |
| --- | --- | --- |
| M3-13 Texture | native59 原生选择器显示已确认；添加仍拒绝合法 ID44，CEP60 显式范围分支与细化拒绝原因候选 | 正反面选择、节点编辑、真实方向/时间采样/阶段依赖与撤销/重开 |
| M3-15 Cloud | native57/CEP57 作者已部署，等待参考外观与范围确认 | Circles/Aspect/Density 外观、动画、预算与宿主持久化 |
| M3-16 Birth controls | 纯核心策略已提交；图/历史/Auxiliary 源码保留，待 M3-13 方向维护完成后继续作者接入 | seed影响完整发射源、概率筛选/身份/预算、配对发布与实际 AE 验收 |
| M3-11 Transform | 原生/CEP 实现与 native54/panel54 配对部署已完成 | 原生/CEP 往返、带动画的 Null、不同图路径、几何、撤销/重开与 shutter 在 AE2023 验收 |
| P-02L 节点 palette | 验收 panel54 的拖入、坐标、取消与交互 | owner 的实际 AE 外观/拖动/撤销证据 |
| P-02K 性能 | 量测 idle、seek、多发射器及 Add/Replace | 宿主延迟/光标观察与正确性；源码调用减少不能替代计时 |
| P-03 预设管理 | 验收原生普通参数/曲线/渐变以及 Add/Replace | 版本、失败回滚、资源、重开和单次撤销确认 |
| M3-07/08/09 编辑器与旋转 | 补齐模式切换、曲线/渐变与旋转的宿主证据 | owner 观察与持久化/undo gates；已有定性接受保留 |
| M3-10 Motion Blur | 验收模式/相机/格式/短生命周期/成本 | AE2023 渲染与持久化；PTF 按 owner 要求暂缓 |
| H-01 Core reload | 渲染中切换、取消、反向帧、重复切换 | 实际 AE 在途 render lease 与输出一致性；已有三帧对比不推广为全场景通过 |

## 后续行为族

Texture/Layer 来源、Transfer、网格/材质/光照、体积等按照 [参考行为清单](reference-inventory.md) 和 [截图库存](reference-texture-transfer-transform.md)逐族建立任务与可观察案例。不要把菜单库存当作已实现功能，也不要复制旧实现。
MFR 审计和 qualification、GPU 实机/CPU 对照、安装与工程迁移分别推进。新的 host family 暂不纳入 owner 范围。

## 维护规则

一张任务卡一次只处理一个范围。架构变更更新 ADR，验证结果写带日期的证据；当前状态不从旧检查数复制“全绿”。保留开放 gate 和已有 owner 观察，不以编译通过替代实际宿主验收。
