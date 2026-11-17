# 原生节点选择控件审计 — P-09 / native105 — 2026-10-24

## 104宿主失败与105修正

Owner于2026-10-23提供AE截图，确认104的Shape和Inherit Motion选择整项消失。对照正常显示的Particle Color、Up Axis及纹理Layer/Dark Side，发现公共add_checked_parameter仍为定位线注入PF_PUI_TOPIC与自定义尺寸。原生popup/layer的行内值就在标题区，仅移除PF_PUI_CONTROL没有恢复完整原生行；上次关于“定位标题与选择器独立”的结论错误。

105移除公共标题标记/尺寸注入和对应自绘路由，沿用其他正常控件的现有注册：PF_PUI_NONE、ui_width/ui_height均为0，标题和控制区全部交给AE。没有新增替代控件或参数。定位线暂停占用原生参数行，Host原有reveal和会话定位查询保留；曲线、渐变和文字专用编辑器继续保留。参数ID/类型/顺序/数量、Core ABI9和渲染均不变；Create Null仍为104追加的原生按钮。

严格实际AEX审计在104产生240项检查中的6项确定失败（native104-before.log），相同测试在105为240项、0失败：覆盖八种实际模块的23个普通选择器，完整注册标志、零自定义尺寸、标题/控制区DRAW/CLICK/DRAG事件均验证。共享注册/透传回归Particle642、Emitter204、Force303、Transform202项，原生Shape81和Transform Null338项通过。四类共享测试还验证所有已注册参数不注入TOPIC。完整May2023 SDK/x64 Release使用两个NoPublish开关构建通过；恢复工作前另核验211个adapter输入哈希，保证当前源码与候选构建一致。

源码/构建/针对性测试完成于10月23日；自动审批服务用量限制中断了随后日志读取和文档更新。Owner10月24日要求继续后，读取完整成功日志、完成八类AEX审计与新AE0检查，再按standing授权完整发布native105/panel103。安装、旧备份、Junction、26个前端文件和Core selector哈希核验及Restore只读预检通过。Core DLL与104相同。精确收据在artifacts/p09r-native-titles-20261023/，冻结候选artifacts/prepared/p09r-native105-panel103/；一步回滚见current-state.md。

尚未启动AE实测。owner截图确定104显示失败；105的Shape/Null选择、Create Null、撤销/重做和保存重开仍需实际AE2023确认。以下保留104审计流水与普通选择器清单，其离线通过不构成真实显示验收。


Owner要求普通选择优先使用AE原生控件。审计当前节点AEX源码，并在隔离测试EXE加载完整构建的八个实际候选模块，调用真实PARAMS_SETUP和每个选择器的DRAW/DO_CLICK/DRAG控制区事件。不是AE宿主实测。

| 节点 | 普通选择项 | 结果 |
| --- | --- | --- |
| Emitter / Auxiliary（共享AEX） | Type、Emitting、Direction，共3项 | AE原生popup；Auxiliary继续沿用既有源模式显隐 |
| Particle | Shape、Particle Color、Up Axis、Orient To、Random Limit、Transfer Mode、Layer、Dark Side、Texture Time Sample、Texture Color Use，共10项 | Shape恢复原生PF_POPUP；两项PF_LAYER及其他选择均为AE原生 |
| Force | 无普通选择项 | Wind and Spin曲线保留专用编辑器 |
| Transform | Inherit Motion (Null Layer)，1项 | 恢复原生PF_LAYER；Create Null另用原生PF_BUTTON |
| Model | Source，1项 | 原生popup；Import OBJ已是原生按钮 |
| Material | Color Source，1项 | 原生popup |
| Motion | Motion Type、Origin Type、After Path，共3项 | 原生popup；Starting With文字输入和曲线为专用编辑器 |
| Turbulence | Turbulence Type、Choose axis、Turbulence affect、Affected Properties，共4项 | 原生popup；曲线为专用编辑器 |
| Output | 无独立AEX或普通选择栏 | 主效果的图片/面板/预设入口是已有启动区域；本轮选择器替换未改该区域 |

共23个普通popup/layer选择器，实际八类候选AEX的注册及不拦截原生事件审计171项通过。该轮只排除PF_PUI_CONTROL，遗漏了PF_PUI_TOPIC对原生行内值的接管；owner后续截图确认104的Shape/Transform选择消失，171项通过不能视为完整原生行验证。105取消普通参数的标题接管，定位线暂停，不再覆盖原生参数行。曲线和渐变涉及点/色标编辑，保留专用UI及其内部操作。Shape和TransformLayerPicker已退出生产NodeEffect构建；Transform/纹理自绘事件代码已移除，旧Shape文件仅保留历史源码，不再链接。

Shape仍保留disk213和原值1..4、Model6；尚未实现的Face5位置使用AE文档定义的“(-”不可选分隔项，拒绝伪造5，避免用自绘菜单做单项禁用。不得通过压缩枚举把旧Model6解释为5。

Create Null在Transform既有metadata之后追加disk1415/runtime91，旧控件1..14、UUID82..89、guard90不动，总num_params92；按钮没有图值。按钮位于现有可见数值控件之后，避免插入导致旧索引迁移。迁移/回滚及空参数交付细节见ADR0032；Shape选择契约见ADR0038。本轮未改Core数值实现，构建后的Core哈希与native103完全相同。

## native104验证与发布（宿主后续确认选择框消失）

May2023 SDK完整Release/x64构建使用两个NoPublish开关通过。新增审计入口：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunNativeSelectorAuditTests.ps1 -Run
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunParticleShapeUiTests.ps1 -Run
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunTransformNullTests.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tests/RunTextureSelectorTests.ps1 -Run
```

结果：Shape81、Transform Null338、纹理239、Emitter97/Particle109/Force90/Transform114、真实native sync20,972及Camera12、native record710、Transform panel95项通过。证据在artifacts/p09-native-controls-20261023/；保留初次缺少隔离Host的旧诊断夹具失败、缺STARFIELD_PANEL_ROOT的record启动失败及部署报告基线路径笔误日志，最终成功日志使用-final或独立文件。不把这些初次失败写成历史通过。

Owner确认AE退出并要求部署后，AE0检查、冻结native104/panel103、完整配对发布及11个native/Core文件、26个前端文件、Junction与Core selector哈希核验、Restore只读预检通过。当前安装/一步回滚见current-state.md。没有启动/停止AE、修改注册表或共享缓存。

待owner在AE2023用旧工程验证：选中效果图层及各节点、原生Shape/Null选择、Create Null创建与绑定、已有动画、撤销/重做、保存重开。P-08R崩溃门和定位刷新等此前开放宿主门保持开放。
