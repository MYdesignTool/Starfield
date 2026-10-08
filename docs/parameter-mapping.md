# 参数来源与映射

当前版本由 [current-state.md](current-state.md) 统一记录。旧索引/manifest 的演变在 [历史映射](history/2026-10-07/parameter-mapping.md)，不要用历史表直接访问当前 AE 流。

## 权威来源

| 信息 | 权威文件 |
| --- | --- |
| 主效果 public ID、key、label、topic、diskId、默认值 | schema/parameters.json 与 Settings.hpp |
| 原生节点磁盘 ID、curve banks、作者 layout | schema/node-parameters.json 与 NodeRecord.hpp/ParticleLayout.hpp/TransformLayout.hpp |
| 图节点 schema、key、端口、值类型/校验 | Graph.hpp、Graph.cpp 与各 ADR |
| Native ordinary properties 到图的转换 | NativeNodeGraph.cpp 与 starfield_gateway.jsx |
| AE 世界/时间/像素转换 | Parameters.cpp、WorldBridge.cpp、Camera.cpp |

流索引是数组位置，磁盘 ID 是保存/脚本属性身份，图 ParameterKey 是数值契约；三者不可混用。PF_ARB 回调使用注册的磁盘 ID。native topic 是展示标记，不能假设它是脚本 PropertyGroup；曲线等使用精确 matchName/disk ID 查询。

## 数值与几何

Core 使用规范化世界坐标：+X 向右，+Y 向上，+Z 离开观察者（ADR0003）。AE layer 的 Y 向下；边界转换应用 layer height、pixel aspect 和 downsample。CameraView 的前方向不是世界 +Z 注释的替代定义。
Box/Sphere 的 Size XYZ 是完整分辨率 layer 像素；Sphere 可形成 ellipsoid；Point/Disc 不消费这些轴维度，Disc 另有尺寸参数。Velocity/Force 的 UI 像素单位由 geometry context 转换。
Particle base size 是完整分辨率像素；基础 opacity 转为0..1。Over Life 曲线使用固定0..100百分比乘子；random 衰减由 seed/粒子身份派生。HDR RGB 可以超过1，核心当前有限上界64；输出16-bpc 的 AE 刻度为32768。

## 时间、人口与动画

时间在 adapter/Core 保持有理值，模拟边界再换成秒。出生/死亡窗口为半开区间，稳定身份与输出 cap 不依赖帧请求顺序；按最新存活 slot 裁剪，负时间没有存活粒子。Emitter PPS 决定出生，Particle 拥有 lifetime，Output 拥有总人口上限。准确调度/边界由 Time、ParticleSimulation、EmissionTimeline 与测试定义。
当前原生数值动画通过主效果 numeric alias checkout；PPS 积分，出生控件按出生时采样，Force 历史使用约定格点。常量 bank/结构选择与动态参数分开处理，见 ADR0023/25/26/30。
Transform 的 Position/Anchor、角度符号与 Null 局部/世界转换由 ADR0032 定义；没有隐式 reference-pose capture。Linear Motion Blur 使用端点近似，Subframe Sample 按实际样本求值，见 ADR0031。

## 诊断与验证

Options 的 last geometry/history 是进程级最近状态，不是当前 effect 的保证。不要凭该文本的硬编码 build 标签判断安装版本，核对 dist 哈希/manifest/PiPL。
schema 唯一性、连续流索引、曲线/渐变边界、ordinary property 往返以及 pixel/time 映射由 [测试入口](testing.md) 验证；源测试通过不替代 AE 保存/undo/渲染验收。
