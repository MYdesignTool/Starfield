# 连接与断开

连线让节点参与同一个流程。连接时要同时考虑方向、输入类型和最终输出，不能把所有端口都理解为通用接口。

## 最基本的粒子流程

**Emitter → Particle → Output**。Emitter 需要直接连接 Particle，Particle 再把粒子交给后续节点或 Output。不能用 Emitter → Force → Output 来替代 Particle 的职责。

## 粒子与模型是两类输入

Particle 的发射来源输入和模型输入用途不同。Model 或 Material 提供的是几何相关内容，应该接入相容的模型路径；它们不能作为 Emitter 使用。

## 吸附与连接检查

靠近合适端口时的吸附帮助你完成连接，但最终仍以实际连线和画面结果为准。看起来靠得很近的两个节点，不一定已经接通。

一般流程应避免循环连接。Auxiliary 有特殊的父粒子关系，不能据此认为任意节点都能接回上游；相关操作单独见[辅助发射](/nodes/auxiliary)。

## 断开后的结果

断开会改变流程，不只是隐藏线条。某个分支不再通向 Output 后，可能不再影响画面。需要临时比较时，先记住原连接，完成后检查是否已恢复。具体拖线和断线手势将在后续补充。

<div class="doc-pagination">

[← 添加与选择节点](/canvas/add-select)

[在线上插入节点 →](/canvas/insert)

</div>
