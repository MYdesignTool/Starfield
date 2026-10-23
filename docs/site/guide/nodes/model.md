# Model · 模型

Model 为流程提供几何形体。模型和发射来源用途不同：一个几何体不会仅因为连接了 Particle 的模型输入就自动产生发射数量。

## 作为粒子外观

基本粒子流程仍需要 Emitter 和 Particle。Model 提供给 Particle 的模型输入后，几何体用于粒子表现；粒子的出生、寿命和数量仍由相应流程控制。

## 作为独立几何输出

Model 也可以直接通向 Output。这是独立模型的输出路径，不能把它当成已经创建了一组按寿命发射的粒子。

## 与材质配合

当前新建流程的几何方向是 Model → Material，再进入 Output 或 Particle 的模型输入。Material 为几何添加基础表面设置，不是反过来生成模型的发射器。

## 素材与变换

模型的局部位置、旋转和缩放，与粒子系统中的后续变换要分开检查。使用外部模型时保留原素材，并记录来源；正式的导入步骤、格式限制和素材管理说明后续补充。

继续阅读：[Material](/nodes/material) · [纹理与模型](/looks/textures-models)。

<div class="doc-pagination">

[← Turbulence · 扰动](/nodes/turbulence)

[Material · 材质 →](/nodes/material)

</div>
