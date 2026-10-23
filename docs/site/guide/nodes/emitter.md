# Emitter · 发射

Emitter 决定粒子的来源和发射方式。把它理解为系统中的“出生入口”，有助于与 Particle 的外观设置区分。

## 它负责什么

发射节奏、发射位置与范围，以及粒子离开发射器时的初始运动，都应从 Emitter 的设置开始寻找。改变初始速度，与在后续流程中施加重力不是同一种调整。

## 它不负责什么

粒子的寿命、大小、颜色和透明度主要由 Particle 定义；全系统的粒子数量上限属于输出设置。不要只增加发射量，却忽略寿命和上限对画面中实际数量的影响。

## 如何进入粒子流程

基础关系是 **Emitter → Particle → Output**。Emitter 应直接提供给 Particle，之后再由 Particle 接入运动处理或 Output。没有连接到有效粒子流程的发射器，不会仅因为存在于画布中就显示效果。

## 数量、寿命和密度

在持续发射、设置不变且未受到其他限制的简单场景中，更长的寿命通常会让更多粒子同时留在画面里。密度还会受到发射方式、观察时间和输出上限影响，不能只看一个数值。

逐项参数、单位和发射模式操作将在后续补充。接着阅读 [Particle](/nodes/particle) 或 [Force](/nodes/force)。

<div class="doc-pagination">

[← 撤销与保存](/canvas/undo-save)

[Auxiliary · 辅助发射 →](/nodes/auxiliary)

</div>
