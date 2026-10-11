# Model 提交链路整体排查 — 2026-10-11

native65/CEP71于13:38+08部署：源码9b77643冻结27文件逐字节匹配并重跑关键回归，fresh AE0，26hash/selector/runtime和paired Restore通过；native9200c6b/packed32833/ABI8保持，SDK未重建。baseline71重现27安装面板字节。备份m3-17-native65-panel71-record-contract-20261011恢复65/70。首次授权启动已执行但无entry/result；owner正在自行测试，后续现有实例调度未获授权，保持离线排查。

## Model编辑及真实OBJ反馈 — 2026-10-11

owner确认Model可添加但慢，编辑时报Model/Transform stream matchName内部校验失败；OBJ时报invalid/oversized。后者已用owner只读文件 `D:\Project\Code\test\test.obj` 离线复现：385660字节，第6680行quad normalized non-planarity3.2092e-8，整体quad最大5.5953e-7，原1e-8阈值误拒绝小数导出误差。候选1e-6容差保留原XYZ、不扩大容量限制、继续拒绝明显扭曲/退化/自交。完整文件修后2858positions/5600triangles，编码SFMG1为384480字节；Core几何4145、控件453及导入20632标准/ASAN通过。错误消息包含静态解析原因与行号，失败仍不写控件。

表达式扫描原先先在任意effect上打开数字UUID参数，再检查name/UUID；Model100、Transform90、Emitter110等真实布局不同。候选在param(index)之前检查完整布局计数，后续name/八UUID字不变。实际mixed Model/Transform native edit及生成表达式执行通过：native8796/camera12标准及ASAN，JS混合644/既有动画通过。该证据证明候选避免跨kind数字子流访问，**不能证明AE内部校验错误已消失**，仍为宿主根因假设。ASAN聚合fixture首次默认栈溢出，测试exe单独增栈16MiB后通过；生产DLL标志不变。

新入口 `tests/RunMixedNodeBindingTests.ps1 -Run [-Sanitize]` 补齐Model链接依赖并运行实际生成JS；旧RunCoreTests的NativeSync范围缺少Model helper链接，本轮失败日志保留，未混入其他工作区测试修改。验收生成器可接收artifacts内的实际SFMG1输出，同一8例覆盖真实模型资源/复制/预设/删除。分页逐页核对，并记录prepare、queue/commit、readback及总耗时；计时含250ms轮询/调度，不能作为native单函数耗时。小mesh172、owner mesh271模拟/ES3检查通过；12页资源回读准备完成，没有真实AE结果或性能改善声明。

native66/packed32834/CoreABI8/CEP71完整源码候选正在冻结构建，未部署。原生参数IDs、effect match names、schema及C ABI保持；只修改开发build号。owner仍在使用65/71，继续原工程测试；不自动启动、关闭、写入其AE或安装路径。

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
