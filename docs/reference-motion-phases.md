# Motion 分阶段参考

日期：2026-10-09。owner要求优先完善粒子节点，Motion首批只实现以下三种模式。截图是可见控件证据；具体方程和资源采样行为尚未测量，不能从标签推断为参考轨迹一致。

| 模式 | owner截图的可见控件及默认值 |
| --- | --- |
| Light Path | Starting With；Speed100；Speed Random0；Max Delay (Seconds)0.50；Delay Random0；Orient To Path关闭；Motion Over Life |
| Circle | Origin Type=Point；Origin XY1920,1080/Z0；Angle XYZ0；Offset XYZ0；Speed100；Speed Random0；Motion Over Life |
| Look At | Starting With=All...；Motion Over Life |

截图文件（用户临时文件，未纳入构建或复制资源）：codex-clipboard-951e5971-226e-4585-95a1-8052b790a205.png、codex-clipboard-0897907c-aecf-432b-aff0-948217d59b1e.png、codex-clipboard-2a4ad3c7-cc61-41f7-8c79-2d0fd95c0166.png。

本地参考库存artifacts/reference/stardust_effect_parameters.txt的Motion节提供完整标签与观测默认值，但只记录菜单当前数值；未列出下拉全部选项。三模式首批不需要注册其他模式。Starting With目标筛选、Light Path光源轨迹、Circle半径来源/速度单位、Look At目标/方向须在契约中区分观察与实现选择；公开指南和实机案例用于补充。

共用Motion Over Life采用项目已有Linear/Hold/Bezier/Draw曲线编辑与保存契约。实际执行须维持乱序帧、出生身份、Force/Transform/Auxiliary路径、快门采样及资源预算。实现和配对构建完成后，实际AE2023行为/撤销/保存重开单独验收。

[官方指南的Motion Node段落](https://superluminal.tv/user-guide)说明圆周运动与沿光源路径运动/朝向；2026-10-09复查未找到三种模式完整菜单、Circle半径或Look At方程。Particle自身的Orient To段落不能当作Motion Look At的精确语义证据。

2026-10-10补充：[官方Staff的Orbit Sphere答复](https://superluminal.tv/question/orbit-sphere)给出零发射速度粒子配合Circle绕球旋转的使用案例。它支持“在已有发射分布上增加旋转”的实现假设，但没有定义半径方程、默认轴、角度时钟或Speed单位；这几项仍不能视为实测一致。已向owner请求Speed0/100对照及Origin Type完整菜单；该请求仍待回复。
