# Starfield 架构

当前版本与部署证据见 [current-state.md](current-state.md)；这里维护职责和契约，历史构建叙述见 [历史记录](history/README.md)。

## 模块和数据流

```text
CEP 节点画布 / 属性编辑 / 预设
  -> ExtendScript 普通参数流、UUID、数值 revision/checksum 收据
  -> 原生节点 AEX + StarfieldParticle.aex 图提交与持久化
  -> SmartFX 参数/历史/相机采样，不可变预渲染快照
  -> 当前 Core generation 的 C ABI / CPU 或准备后的 GPU 场景
  -> AE 输出世界
```

| 模块 | 职责 |
| --- | --- |
| StarfieldParticle.aex | 主渲染效果、ARB 图持久化、参数注册、SmartFX、Core 加载与输出转换 |
| Emitter/ParticleNode/Force AEX | 菜单隐藏的独立原生作者参数；节点效果 SmartFX 透传输入 |
| Transform AEX | Transform 控件与 Null 资源作者记录；CEP 资源与属性集成已实现，AE 宿主验收开放 |
| StarfieldHost.aex | UI idle 上有界的初始化/原生绑定准备 |
| StarfieldCore.dll | 纯数值图求值、确定性粒子模拟、CPU 栅格化、GPU 场景准备 |
| cep_panel/ | 图 UI、曲线/渐变/预设、守卫事务与自适应检查 |

Output 是主渲染效果在画布上的固定终端，没有独立 Output AEX。Auxiliary 使用 Emitter 原生效果的结构选择。Appearance 已退出当前表面。

## 核心与宿主边界

Settings.hpp 拥有纯 Core 值类型；Render.hpp 定义宿主无关渲染边界。AE 类型、suite、句柄和世界指针停在 ae_plugin/。
C ABI 使用固定宽度数值、字节跨度与不透明结果句柄；创建和释放在同一 generation/CRT 中完成，C++ 对象、异常和分配器不跨 DLL 边界。

主 AEX 也编译图编解码、构建与部分求值/历史代码，因为它需要制作持久化记录和冻结快照；DLL 使用这些共享实现进行渲染。不能把所有 GraphEvaluation 调用都描述为 DLL 独占。
CoreOnly 的 adapter-input 指纹会检查共享输入，runtime 发布还核对已安装的原生配对哈希；共享输入或 ABI 变化必须先完整成对构建。

## 图、模拟和样式

图 envelope1 使用独立 UUID、类型 key、强类型参数、端口与有界校验。主图存于隐藏 ARB 参数；sequence data 不存放可变模拟。
参数磁盘 ID、流索引、图 key 和 UI ID 各自独立；具体版本/索引由 schema 与对应头文件拥有。

粒子按 seed、稳定身份和绝对时间求值，支持乱序帧。Emitter 决定出生与初始速度；Particle 决定 lifetime、形状、基础样式和 Over Life；Force 按图路径作用；Output 对全图实行共享 population cap。
辅助发射采样父粒子在子出生时的状态。动画/积分语义见 ADR0023/0024/0025/0026；曲线、渐变和旋转见 ADR0027/0028/0030。

Transform 分别合成中心与精灵基底，保留 shear/reflection；Force 使用下游 Transform suffix。共享基底表最多4096项，snapshot4 保留200字节的显式粒子记录，仍能读取 snapshot3；内存对象 sizeof 不定义 wire stride。Null 作者选择与数值采样契约见 ADR0032。

M3-17 Model候选的纯数值几何类型由Settings.hpp拥有，ModelGeometry处理单位cube与有界OBJ角点/三角化/验证。ModelScene用显式model-to-layer矩阵制作裁剪三角形、四采样覆盖/深度和独立粒子叠加。ModelResources负责SFMG1网格codec和粒子pose；RenderRequest自有网格，Core ABI8追加有界数值数组并接受ABI7精确前缀。输入只有字节文本与数字，未执行文件IO或宿主操作。图/shape/快照/作者仍待接入；snapshot7未变。迁移和完整Model gate由ADR0038管理，现有安装保持native60/ABI7/CEP61。

## 渲染、资源与线程

SmartFX 构造不可变图/历史、几何和 Core lease；相同 generation 保持到对应 render/result 释放。内容身份参与 cache GUID。渲染输出为透明背景上的粒子 RGBA，不合成输入像素。
Core 验证时间、尺寸、格式、图、数值和工作预算；adapter 尊重 rowbytes、PAR、downsample、ROI、AE 16-bpc 的32768刻度和8/16/32-bpc 输出。内部积累为 premultiplied，编码 alpha 由请求决定；32-bpc 可以保留 HDR RGB。

Camera.cpp 内仍有相机/效果层 suite 查询；原生 sibling 作者编译与 Null 绑定准备由 UI 路径承担。不要将“禁用 MFR”等同于所有 render 都在主线程；各 suite 的 selector/thread 允许范围仍属宿主审计事项。
渲染请求不从 CEP 获取状态；host suite 或 checkout 不在持锁区内调用。UI 缓存不持有超出回调有效期的 host 对象。

GPU 通过声明后的设备协商、场景准备和独立核函数执行；CPU 是数值对照路径。Motion Blur 有 endpoint Linear 与实际 Subframe Sample 两种数值策略；前者是有界近似，详见 ADR0031。MFR/Compute Cache 的启用仍需要独立验收。

## 开发规则

- 发布过的身份、ID、schema、版本变化须带迁移计划和 ADR。
- 构建候选与发布/宿主支持分开记录；只有相应 host family/build 的实际行为才能关闭宿主 gate。
- 构建及清理输出在 artifacts/；dist 经 AE Junction 使用，清理工具不触碰它。
- ADR0011 管理所有宿主改动；注册表不由任何仓库脚本写入。
- 新测试及证据入口见 [testing.md](testing.md)，当前任务所有权见 [agent-backlog.md](agent-backlog.md)。
