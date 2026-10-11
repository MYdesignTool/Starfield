# Model 提交链路整体排查 — 2026-10-11

M3-17。CEP70出现另一 UUID 的 schema7/3 错误，owner要求停止逐个报错验收。范围包括 Model/普通图提交、预设、资产传输、脚本生命周期、备份回滚和发布。AE为什么返回3、新UUID的节点类型均未证实。

## 确认缺口及 CEP71 修正

| 位置 | 已核对缺口 | 修正 |
| --- | --- | --- |
| browser / Model transport | 浏览器投影五类布局，最终提交仅处理Particle | 同一 ES3 factory 嵌入 browser/gateway，全部五类共用 API规则 |
| nativeSnapshot | schema版本另有嵌套选择表达式 | 同一 type/version 表；没有证据认定AE解析表达式错误 |
| native callback 后 | receipt/ordinary submit/sync/Model snapshot 可能复用旧引用 | plain token 重新解析 renderer、receipt和snapshot |
| script boundary | Array instanceof 依赖调用方prototype | 中性Array tag，覆盖向量/曲线/资产bounds跨realm |
| diagnostics | UUID无法确定type或实际contract | 逐字段错误增加type/build/contract，信息有界 |
| 旧DOM夹具 | 共用宽泛参数表，只测试Particle旧label | actual PF_Cmd_PARAMS_SETUP 导出五模块属性，按真实matchName/disk/index组装DOM |

投影前检查有界整数label、所有必需/已知可选key、唯一canonical key、精确wire type及有限有界值。raw node/schema/stamp不重写，不补必需字段，不放宽用户值、布局或拓扑比较；prepared raw label仍严格。disk IDs、持久化graph/sequence schemas、Core ABI8、native65字节保持。

## 离线回归证据

入口 `tests/RunModelPanelAudit.ps1 -Run -PanelRoot <artifacts/prepared/.../cep_panel>`，默认report。日志 `artifacts/m3-17-full-panel-audit-run5.log`，详细目录 `artifacts/m3-17-panel-audit-84328947399c42dcb915a63a375eab46`。

- 五类factory/browser/gateway、各label、逐字段坏wire、重复/未知key与跨realm：458。
- actual planner/client/JSX混合图、各类prepared/published失败和模拟完整恢复、重试/取消/重载：2340。
- actual gateway/实际注册表18 DOM模式：每组44，旧API重载45。覆盖两个Emitter/两个Particle/Force/Transform/Model、正背面Texture IDs44/66、两种wrapper身份、发布失效、排序和legacy label、提交中gateway重载。
- actual注册：Emitter86/Particle91/Force82/Transform102；Model controls452/binding9433。补齐旧Shape event测试stub、六形状及Transform binding bound断言；native实现未改。
- 页面36、asset600含foreign bounds、author74、preset583、普通gateway全套、三完整JSX ES3 18。
- actual native backup/SFMG1 executor标准/ASAN各636209；Host标准9593769/ASAN9594294；实际Windows message pump exclusion各470；asset/import标准和ASAN也通过。SDK调用仍为替身。
- nested Prepare19、native65/CEP70→CEP71发布/整套恢复dummy64。

首次catalog运行发现旧测试链接/断言缺口及Windows PowerShell UTF16读入问题，失败日志保留，修正后整套通过。通过数不能代替AE资格。

## 自动AE验收及剩余gate

`tools/prepare_model_acceptance.cjs --prepare <isolated panel> <artifact output>` 使用真实browser planner、preset/mesh codec生成8例：默认Add、混合图/已有Texture、默认复制、SFMG1资源/pose恢复、资源复制、资源预设Add/Replace、删除Model。

`tools/Run-ModelAcceptance.jsx` 执行真实native idle事务、等待retained completion、重读snapshot，并逐字节native export回读资源。只创建临时文件夹/合成/solid，结束删除自身项目项并恢复合成选择，不保存工程；执行中超时保留测试合成，不能拆除native正在持有的对象。结果仅写artifacts。单独运行默认report。生成/CRC及引用重写、成功/失败/忙碌/超时清理和两个JSX ES3自检135通过，仍为替身。

`tools/Run-ModelAcceptance.ps1 -Run -PreparedName model-acceptance-panel71` 启动指定AfterFX.exe，短时抑制脚本错误弹窗后恢复，保留AE运行；ADR0011要求单次明确process-start授权。当前尚无实际AE自动结果。撤销/保存重开、OBJ文件窗口、像素/时间行为、模态时序及GPU硬件gate仍开放，禁止称为可向客户发布。
