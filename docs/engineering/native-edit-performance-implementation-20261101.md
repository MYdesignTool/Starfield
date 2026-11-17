# PERF12：原生编辑性能实施候选127/116

目标是在Owner确认删除故障已修的126/115基础上，减少宿主读取和退休槽写入。候选仍需最终独立实现审校；未据离线调用量承诺AE的5–10秒已消失。

## 修改范围

- NativeEdit已有UUID/参数/value delta入口。现在重复编辑时，对同帧、同几何/PAR/项目/renderer/epoch且SDK状态未变的非目标Emitter、Particle、Force、Turbulence，复用已接受图里的原始参数记录。当前目标节点完整读，连接/身份/guard/布局仍扫描，Core验证及完整不可变快照发布/ACK保持。按参数盲改旧快照不能检测外部脚本修改，因此本批不采用。
- 源私有只读selector必须有实际PF context，重新验证own effect match和UUID stream。PF_State仅由SDK比较；普通完整读取前后的原始receipt匹配才缓存。不保留AE句柄，失败/无能力自动完整读取并负缓存探测能力。Particle旧新两个贴图引用都0才复用；其他资源路径完整读取。
- 删除最终依赖刷新保留126全部live和退休集合，不按0/空文本判断未占用。退休槽以两种交替禁用无源标记之一替代value;→空双写；精确文本/状态/有限handoff检查、旧代交接/guard/回滚仍保留。旧126能识别标记前缀。代价是部分原空表达式带有标记，未压缩UTF16约增加75KB/三作者夹具；实际AEP大小未测。
- 删除请求先画pending反馈再发宿主请求，仍等正回执和一次展示快照；单击编辑保持。队列、捕获、发布、物理删除及最终刷新计时分别记录。源编辑在16条POD内存环记录query/full-node/compile/service耗时、命中数、miss bitmask和探测失败原因，只在安全idle排出，不在Clear/source callback写日志。

## 实际生产代码夹具调用量

3作者79绑定，同一操作条件：

| 操作 |126|127|限制 |
| --- | ---: | ---: | --- |
| 重复Particle Size修改的标量读 |241|196|127另有2次源generic state查询；当前Particle仍92次 |
| 最终force refresh表达式写 |1024|591|保留所有槽，未用sentinel控制冒充产品 |
| 最终force refresh表达式状态写 |591|158|退休原已禁用时省重复禁用 |
| 最终force refresh数值写 |512|79|退休基值精确等handoff时省重复数值写 |
| 最终force refresh表达式读 |945|1378|增加433次标记读回，需真实AE计时判断净收益 |

原生编辑仍发布完整immutable graph，这是Core的原子输入；没有因此重新从AE读取每个无变化节点。完整快照的CPU编码与AE参数调用是不同成本。缺PF context、状态不稳定或其他条件不足时仍完整读，不对不支持路径宣称命中。

## 独立反例与资格

reviewer-v2实际加载8个127 AEX，发现NodeEffects内部枚举不含Auxiliary占位，不能直接cast为图kind：Force/Turbulence被误判unsupported。该问题不会写错图，却会关闭常见图的复用。已改为显式逐项映射并把实际AEX selector回归提升到tests；旧失败AEX和62/4反例、完整actual-selector失败日志保留。v2还要求补齐query/full-node计时与miss原因，已补；须全新v3审校，v2不是通过资格。

稳定SDK、实际AEX/生产publisher/删除/备份/面板回归均属于离线资格。AE2023真实generic PF context、PF_State表达式依赖、marker依赖失效、Undo/save/reopen及输入到画面耗时仍由Owner验证。数据/命令见artifacts/perf-12-20261031及docs/testing.md。
