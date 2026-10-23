# Force · 力

Force 用于给已有粒子增加力的影响。它不产生新的粒子，也不代替 Particle 的外观定义。

## 初始运动与持续影响

Emitter 中的初始运动描述粒子离开发射来源时的状态；Force 中的重力、风和空气阻力等设置，关注粒子随后如何运动。两者共同参与结果，不宜当成同一个速度开关。

## 放在粒子流程中理解

一个常见阅读顺序是 **Emitter → Particle → Force → Output**。先确认基本粒子已可见，再加入 Force，更容易比较力对轨迹的影响。

## 先判断想改变什么

想让粒子持续向一个方向偏移，可以查看重力或风相关设置；想改变运动衰减，了解阻力相关设置。具体方向、单位和数值建议会在参数说明核对后补充，不应直接套用其他软件的数值。

## 从单个节点开始比较

多个力同时存在时，更难判断某一项调整的影响。学习时可以先使用一个 Force，观察多个时间点，再逐步增加其他节点。Force 不是刚体碰撞系统，也不会自动生成碰撞地面。

相关内容：[Emitter](/nodes/emitter) · [Motion](/nodes/motion) · [Turbulence](/nodes/turbulence)。

<div class="doc-pagination">

[← Particle · 粒子](/nodes/particle)

[Transform · 变换 →](/nodes/transform)

</div>
