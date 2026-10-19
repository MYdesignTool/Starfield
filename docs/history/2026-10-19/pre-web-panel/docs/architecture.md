# P-04 source candidate — native97 AEGP/WebView2 — 2026-10-19

Native98 additionally gates pending panel work on a valid enabled AE main HWND
and scripting availability. AE save/quit modal loops can invoke idle hooks, so
plug-in-owned UI exclusion alone is insufficient. Preserve request deadlines
while deferred; scope SDK quiet errors only around ExecuteScript and restore
the prior state before returning. Actual save-confirmation lifecycle acceptance
is tracked separately from offline queue tests in current-state.md/ADR0046.

ADR0046 adds a dockable native graph panel and modeless WebView2 preset window
owned by StarfieldHost. Shared HTML/JS and the generation99 JSX/native author
transactions remain authoritative. Web callbacks enqueue bounded owned messages;
only the resident UI idle hook executes AE scripts. Closed/navigation-stale,
replayed or expired requests never execute. Browser initialization is lazy and
headless-safe; development profiles stay under checkout artifacts. Core ABI9,
node IDs, animation, schemas and rendering are unchanged. CEP retirement is gated
on actual AE2023 qualification, not offline tests. Installed pairing and evidence
remain in current-state.md; c43e293 is the pre-migration functional baseline.

# Current M3-20 candidate — native Motion prefix authoring — 2026-10-18

ADR0039/0045 restore Starting With using its existing constant numeric bank.
The custom control opens a native UTF-8 prefix editor with owned modal
snapshots; the AE parameter label supplies the title, and the control draws
only its prompt/value to avoid duplicate text. MotionAuthorTransaction persists
and verifies the bank jointly with graph publication. Static prefix overrides
never enter numeric animation bindings. Historical Motion sampling retains
frame-scoped Light Path and Look At resources instead of dropping them while
rebuilding numeric controls. No render boundary, public ID, persistent schema
or Core ABI change.

# Previous M3-20 candidate — explicit native repair and owned button input — 2026-10-18

ADR0040 separates normal live graph handoff from an explicit, backed-up repair
of a verified disabled renderer. Only the latter can replace unavailable old
bindings with finite record values while rebuilding derived state. New active
bindings still require actual author evaluation. Resident Host dispatch targets
the selected layer under its UI exclusion; no idle recovery or persisted-lock
adoption. No public IDs, sequence schema, Core ABI or render contract change.
ADR0038 makes Model button delivery optional: checked identity and owned saved
state cross the dialog boundary; raw callback arrays and SDK handles do not.
Current source/build/deployment/host gates belong to current-state.md.

# Previous M3-20 investigation — missing active Model — 2026-10-18

Actual saved CRC29ece35f matches host revision44: stream237 references an
Output-connected Model absent from the current native inventory. The bounded
republish recovery fails strictly at its old animation handoff. Do not apply
parked-orphan rules or substitute a recorded constant for this active source.
ADR0040 defines a separately authorized disabled-copy rebuild with a fresh
renderer and surviving native author effects. Normal transactions/IDs/ABI and
installed90/99 remain unchanged; deletion origin and host recovery remain open.

# Installed M3-20 checkpoint — native90 / CEP99 — 2026-10-17

Project-wide explicit command selection treats receipt=nonce and an exact
receipt=-nonce for a valid request as terminal. Only pending requests participate
in uniqueness. A rejected renderer may stay guarded after failed rollback;
skipping it is isolation, never authorization to retry or unlock it. Actual owner
evidence and regression reproduce its interference with fresh initialization.
Failed canonical initialization shows no legacy fixed graph; a previously
confirmed same-target canvas remains visible after transient read failure.
ADR0040 and current-state.md retain original-lock/Model host gates and evidence.

# Previous M3-20 checkpoint — native89 / CEP98 installed — 2026-10-17

Script-owned renderer guards use zero-time pre-expression saved samples,
matching EffectGraphBackup::scalar for this non-animating hidden stream.
No ordinary getter fallback. Busy diagnostics retain the original decision
and compare fresh named/indexed samples without writing. The CEP97 owner
failure persists immediately after successful deletion; reader divergence
is a hypothesis, not a confirmed host cause. ADR0040 records the contract;
current-state.md owns validation, deployment and remaining Model/AE gates.

# Previous M3-20 checkpoint — native89 / CEP97 installed — 2026-10-16

Guard ownership is request-scoped plain data. Reacquire the pinned renderer
guard after enumeration/publication; verify acquisition, release and zero after
undo closure. Ordinary edits/bootstrap share cleanup; only confirmed ownership
authorizes rollback. Failed rollback retains its guard; unknown guards are not
automatically cleared. ADR0040 records this contract and its host uncertainty.
Owner confirms Motion addition on96; canvas editing and Model chooser gates
remain open. Native89/Core ABI9 and the accepted Motion mapping remain.

# Previous M3-20 checkpoint — native89 / CEP96 installed — 2026-10-16

The owner trace raw1/Core0 identifies the Motion mapping expression, matching
ExtendScript's reported conditional association. Motion uses explicit enum
branches; all other direct nested conditionals in shipped JSX are explicitly
grouped. ES3 syntax gates also reject ungrouped conditional branches. Normal
Node parsing alone cannot qualify ExtendScript semantics. Native89/Core ABI9
and strict publication/rollback remain. ADR0045 records evidence and host gates.

# Previous M3-20 checkpoint — native89 / CEP95 installed — 2026-10-16

Owner reproduced the readback discrepancy on94. CEP95 requires direct sampling
for Motion mode, removes the scalar fallback and consumes the fresh Property
without another host getter. Request-local plain raw/Core evidence accompanies
failure diagnostics; saved values are never replaced by the desired mode or a
later probe. ADR0045 and current-state.md record the unresolved host gate.

# Previous M3-20 checkpoint — native89 / CEP94 installed — 2026-10-16

CEP94 adds failure-only pre-rollback Motion diagnostics under ADR0045. Strict
comparison remains authoritative; diagnostic values never replace the snapshot.
Successful comparisons add no host reads. Motion host resolution is still open;
current-state.md owns the pairing, evidence and undo.

# Previous M3-20 repair — native89 / CEP93 installed — 2026-10-15

AE23.5x52's failure trace places current Motion failure in gateway readback:
native1/guard0 versus cached snapshot Core0. Cache only layout locators; reacquire
children and unique full-UUID sources after enumeration/UI mutations. Explicit
OBJ publication owns its existing UI exclusion capability while unrelated
callbacks remain deferred. Model checkpoints/rollback resolve current full UUIDs;
owned mesh/numeric values survive modal/publication phases, SDK refs do not.
ADR0009/0038/0045 describe these unchanged-ID/ABI9 contracts. Exact deployment
and open post-install host gates are recorded in current-state.md.

# Starfield 架构

M3-20 deployed CEP92 (2026-10-15) keeps native88/Core ABI9. Native
publication is the sole Motion selector writer; scripts stage guard intent only.
Matching-generation gateways reuse code with request-local caches cleared.
Asset-free Cube add/duplicate uses ordinary publication; resource changes and
all deletions keep full backup. Optional Model transport loads on demand.
Common graph/receipt codecs remain eager for whole-graph validation; AE manages
AEX lifetime and Output-ancestor sampling remains unchanged. Tests and receipts
live in artifacts/m3-20-add-path-20261014/; actual AE correction/latency is open.


P-02M (2026-10-14) retires the CEP floating parameter inspector. The canvas and
preset manager remain; clicking a node focuses its native AE Effect Controls,
where all node values, popups, gradients and curves are edited. CEP91 removes
editor DOM, styles, event handlers and editor helper script loads. Graph snapshots,
read-only summaries and guarded topology/layout/preset transactions remain.
Native88/Core ABI9 is unchanged; previous Motion/deletion host gates stay open.


M3-20 resumed2026-10-14: native88 / Core ABI9 / CEP90 is the deployed Motion lifetime
repair; receipts and installed status are authoritative in current-state.md. New
features remain paused. ADR0045 now separates numeric transaction state from
operation-local SDK ownership. Prepare/accept/rollback resolve unique Motion
UUIDs, reacquire streams after writes and release them before the owning effect.
Graph/binding publication cannot carry source handles into its next phase.
Rollback returns an explicit status, verifies restoration and retains a guard on
failure where identity remains safe. Duplicate/missing identities never authorize
a numeric-slot write. This addresses the observed native87 crash path without
claiming AE's precise invalidation trigger has been proven.

Author intent is skipped for unrelated native edits; inactive sources hold no
transaction entries. Changed-source lookup uses one sorted UUID inventory per
phase, with linear SDK effect opens. Native graph publication obtains its CRC
from the same encoded bytes used to allocate the ARB value, removing duplicate
validation/serialization. IDs, schemas, Core ABI9, Output ancestor scope and
append reuse are preserved. Failure injection and microbenchmarks are recorded
in testing.md; AE persistence, deletion and responsiveness remain manual gates.

The rejected pairing was native87 / Core ABI9 / CEP90, deployed2026-10-14 after
all applicable offline gates and a zero-AE process check, under ADR0045. The
native UI publisher consumes bounded Motion author intent, persists/verifies the
source selector and its guard release, and preserves rollback. Source callbacks
require complete delivery, resolve current kind+UUID rather than a borrowed PF
parameter binding, and own exclusion with a distinct source-purpose query. All
current offline gates pass; actual AE mode/deletion cause remains unproven. The
installed state and deployment receipt are authoritative in current-state.md.

Previous test pairing was native86 / Core ABI9 / CEP89, deployed
2026-10-13 with UI transaction callback fencing and Motion authoring repair
(ADR0044), preserving Output scope/orphan retirement/append reuse (ADR0043).
CEP88 unifies the three-mode record reader, keeps the same-target confirmed
canvas on failed reads, requires a fresh receipt before editing, and gates
deployment/preparation on exact embedded contract and cache generations.
CEP89 explicitly authors Motion selectors, verifies fresh UUID-scoped reads
and published modes inside the transaction. Native86 supervises nonanimated
Motion Type/After Path. Ordinary rollback restores graph/bindings before removing
retired appended additions and holds the guard on failed restore.46 panel suites
and18 transaction variants pass offline; actual AE qualification remains open.
The owner confirms84/86 performance improvement. Manual AE2023
qualification remains open. The independent node exposes Normal, Axis and One
Side. The owner deferred Spherical and Spherical 2. Deployment hashes and
rollback are recorded in `docs/current-state.md`.
The owner has reopened stability investigation and paused new features. A
disconnected deleted source in the saved project is confirmed; remaining random
active-binding failures and actual host latency are unqualified.
Historical rollback and rejected-pair records below do not describe the current
installed pair; see `docs/current-state.md` for receipts and rollback.

## Previous implementation attempts (historical)

M3-17 native78 was rejected for add-node timeouts and exact77/CEP81 restored.
Deployed79/82 removes CRC expression reads and adds two bounded repairs: keep
polling a single transaction after a transport read timeout (original5min deadline,
1s backoff, no mutation replay); rebind restored effect instances under guard1 before
render release on failed/canceled graph edits. First materialization may restore AE
Controls, which requires no native aliases. Failed rebind retains guard and reports
rollback error. UI journal timestamps identify where slow/failed commands exit.
This closes confirmed code gaps; AE cause/qualification remains unproven.

M3-17 deployed native78 preserves accepted CEP81 and adds graph generation
coherence: explicit UI publication uses CRC-stamped alias programs and validates
all publication stream types before mutation. Programs check their renderer's
receipt before sibling lookup; PF validates graph/receipt and samples only matching
generations. Stale-generation reads cancel, including temporal Core propagation;
matching-generation unavailable/nonfinite data still fail. UI/PF journal provenance
includes CRC/count and failed UUID. No public IDs/schema/ABI change or render-thread
AEGP access. Native77 deletion failed on owner AE; exact AE caching remains unknown.

M3-17 deployed77/CEP81 (20:44+08, frozen352562d) adds final active/retired dependency refresh after source and
backup removals, before renderer guard release. A separate private version1 SFR1
UI request preserves the existing graph commit wire. Saved graph/nonce/IDs/schema/
CoreABI9 remain unchanged; unchanged Model/Texture resources are not copied or
parsed again. Errors remain strict and finalization failures use cleanup diagnostics.
CEP immediately projects validated edits, paints before host submission, reuses
the cached planning receipt and adopts the confirmed receipt before background
refresh. Failed drafts restore the prior canvas. Viewport interaction stays local;
host writes are serialized. Owner declines automatic AE acceptance; continue work
with manual host gates open. Reference-cache causality remains a hypothesis.

M3-17 deployed76/CEP80 (15:27+08) follows the owner's failed75/79 deletion retest. Renderer
guard0 remains strict; guard1 (edit) and2 (private backup) cancel render checkout
before graph, aliases and resources. Guard checkout records the release dependency.
This covers original and disabled backup renderers throughout physical deletion
and rollback. Full backup/render lifecycle reproduces old75 missing-source errors
and passes the new entry; actual AE scheduling/cancellation remains unqualified.
Optional confined development render journal supports automated host evidence.
ADR0040 owns this contract; IDs/schema/CoreABI9 unchanged, Motion work preserved.

M3-17 native75/CEP79 deployed13:54+08 repairs the separate alias mutation window left
by74/78. Keep obsolete expressions until saved graph verification, then detach
before physical removal. Validate the old evaluated alias and install a finite
base beneath its enabled expression before replacement. Rollback rebuilds the
old dependency before restoring its exact base. In-flight old graph sampling
does not see a sentinel during these mutations; unavailable/nonfinite sources
and final bindings still reject. Seven-kind actual native mutation-hook fixtures
pass and the74 algorithm fails. ADR0040 owns the full pairing; frozen ten-module
SDK/panel/hash/rollback gates pass. Actual AE2023 correction remains open.
Motion work is preserved; full goal active.

M3-17 deletion stage74/78 deployed11:18+08 uses full native effect-graph backup for every
removal, including non-Model graphs. Pending original guard3 is excluded from
authored inventories while the physical source stays alive until publication;
backup2 retains rollback identity. Verify new graph/aliases before physical
removal, and restore full state on any failure. Ordinary submit rejects deletion
before mutation. No ID/type/schema/CoreABI change; ADR0040 defines full pairing.
Frozen SDK/panel/ASAN/hash/rollback gates pass; actual AE correction remains open.

M3-18 unpublished mapped-time point sampler reads source world matrices at each
explicit composition time against the current owner frame. Numeric normalized
times leave the adapter with points; per-call caches key both identity and time.
The old current-time API remains. This is sampling infrastructure, not an author
or Core resource/ABI addition; native history/shutter/dependency integration follows.

M3-18 unpublished MotionLightTiming seam reads a point light's position keyframe
range, with separated follower timing and owned rational metadata. Only adapter
code sees Stream/Keyframe/Light suites; callback-local refs release before atomic
output publication. This is not timed path sampling or a resource/ABI addition.

M3-18 native73/CEP77 deployed stage implements Force blocks after Motion in graph edge
order, including static/history evaluation and Auxiliary inheritance. Deferred
Force is excluded from prefix simulation so it is applied once. Consecutive Force
fields combine in the current stream frame; ADR0039 defines drag's trajectory
mapping. No public ID, persistent schema or CoreABI change. Different-chain merges
and processors before Particle remain implementation work. Frozen ten-module
May2023 build, panel audit and paired hash/rollback gates passed; AE2023 acceptance
remains open (current-state.md).

2026-10-12 owner confirms edge order (upstream→downstream), independent of canvas
placement/effect list. Compatible processing modules must compose in arbitrary
order. Distinct-chain rejection remains implementation
debt, not the target architecture. Light Path author must use AE point-light
position keyframe timing/interpolation at Speed100%=original duration, with
Stop/Continue and fixed Max Delay at Delay Random0. Circle100% retains one turn
per second. ADR0039 records the updated contract and staged migration gates.

M3-18's next unpublished seam inventories light names/IDs/order/activity as
owned AE-adapter values. Prefix selection returns IDs for the existing numeric
point capture; target/path/activity policies remain at the author boundary.
Temporal Light Path adds bounded backwards fixed-distance geometry velocity
to its clock velocity, including Auxiliary birth inheritance. Resource snapshots,
native binding/history/shutter/cache GUID dependencies and the remaining two
authors are still required. No SDK object enters Core or Render.hpp (ADR0039).

M3-18 native72/CEP76 deployed stage adds an independent Motion Circle effect, kind7
and binding record9, preserving old record readers and CoreABI9. Origin contexts
are evaluator-local owned numbers; ParticleInstance/Render/snapshot layout is
unchanged. Circle frame velocity includes analytic pivot/axis derivative terms
with bounded backwards temporal rates. UI converts owner Speed100 to2*pi rad/s.
Force precedes Motion in this first author stage; ordered downstream Force and
different-chain merges remain full M3-18 completion gates (ADR0039).

M3-17's 2026-10-12 deployed ADR0041 slice uses Model → Material → Output and supports
Material → Particle model input. Material forwards geometry with branch-local
shading. Legacy Material → Model remains readable. Existing UUID/field alias
spans stay stable; structural edits rebuild bindings and retire removed aliases.
Mesh-preserving parameter edits use ordinary guarded commits. Native71/CEP75
passed full paired publication; AE2023 acceptance remains open.

The native70 legacy ADR0041 slice adds Material metadata → Model and direct Model →
Output. Numeric material values and captured AE lights stay owned by Core render
staging; AE suites and borrowed light buffers remain inside ae_plugin. GGX/Smith/
Schlick shading uses transformed corner normals, perspective interpolation and
bounded CPU sample/light work. Core ABI9 / snapshot11 / binding8 retain old
readers. Material is a separate node module, disk1601..1606, with a nine-module
native70/CEP73 pair. No Material keeps the previous unlit model behavior.

M3-17 native69/CEP72 deployment changes CEP graph submission per ADR0040: ordinary
request nonce plus explicit synchronous resident AEGP command, guarded unique-owner
resolution, versioned generic receiver and atomic saved-stream graph/receipt
publication. Render still reads owned graph/aliases; no script or idle queue in
this command. Core ABI8 / persistent schemas / parameter IDs unchanged. Installed
69/72 passed frozen release/hash gates; AE2023 acceptance remains open. See current-state.md.

## native68／CEP71 deployment — 2026-10-11 18:45+08

Installed native68/packed32836/CoreABI8/CEP71 from frozen source
61942b06b9e1171efc15aad9023f3ed637044c8a. Full May2023 x64 Release /MT eight-target
build session81508 finished exit0. Frozen standard/ASAN native9048/camera12,
Transform60, generated mixed JS643/Transform168, complete panel audit and
scene150946/Model particle2695 ASAN gates finished exit0. Whole-graph backup
standard/ASAN810845 and dummy67/71→68/71 deploy/full restore61 also passed.
Fresh AE0 permitted standing-authorized deployment. Independent26 pair hashes,
all27 tested CEP files and selector/runtime match. Installed before/after state:
artifacts/m3-17-native68-deploy-{before,after}.json; hash report:
artifacts/m3-17-native68-installed-hashes.json; wrapper log:
artifacts/m3-17-native68-panel71-deploy.log. A wrapper log-name collision occurred
before native publication; original67/71 hashes were rechecked, staging was
restored, saved baselines/logs retained, then the normal paired tool succeeded.

Literal source resolution eliminates heterogeneous effect iteration at alias
evaluation; backup names cannot shadow author sources. Owner-mesh workload
68,952,940→16,889,395 succeeds under unchanged caps, with independent coverage
oracle. These are verified offline properties; the exact AE stream mismatch
root cause and installed host correction remain unconfirmed. Existing-project
rebind, manual effect rename rewriting, Model editing and render performance
remain AE gates. No AE launch/dispatch occurred. Eight owner-mesh acceptance
cases are ready in artifacts/m3-17-native68-owner-acceptance; starting AE requires
the separate ADR0011 authorization. No new particle modes or Motion author
surface added. IDs/matchNames/schemas/ABI unchanged. Foreign Motion/MNT edits
preserved, local source commits89e8507/61942b0, Git push still pending.


## M3-17 native68 candidate — literal sources and triangle spans (2026-10-11)

Owner reports native67/CEP71 still produces Particle/Emitter stream matchName
mismatch on Model addition, and Model sample visit cap exceeded on OBJ import.
The expression host's reuse of a call site across heterogeneous effects remains
a hypothesis; no AE root cause or fix is claimed from mock tests. native68 emits
one literal Effect(name) reference per alias after a fresh native kind/full-UUID
inventory. Every UTF-16 name unit is escaped. Source identity still checks all
eight immutable UUID words before numeric control access; errors/sentinels retain
rollback. No expression enumerates the effect parade. Transaction duplicates
receive unique private names and original names are restored on rollback.
Missing/ambiguous author names reject before alias writes; this deliberately
replaces the old assumption that duplicate display names were irrelevant.
Custom author renames rebind at the next native graph commit; host automatic
rename rewriting and old-project migration remain AE acceptance gates.

Read-only owner OBJ (2858 positions/5600 triangles) reproduces the old cap at
size133, angle30, Center enabled: 68,952,940 bounding-box candidates. Scanline
half-plane spans preserve final edge/top-left/depth tests and reduce charged
work to16,889,395 including span calculations, succeeding under the unchanged
64,000,000 group/512,000,000 frame caps. Twelve owner-mesh poses were exercised;
this is an offline render workload result, not a claim about the user's exact
frame or AE performance. Independent whole-ROI oracle compares fractional,
thin, reflected, clipped-ROI and overlapping triangles, coverage/depth/source
identity and exact work-budget boundary. Row/span work is charged even if empty,
preflight runs before depth allocation and cancellation remains bounded.

Evidence so far: scene150946 standard/ASAN, Model particle2695 standard,
whole-graph transaction810845 standard/ASAN, mixed native9048/camera12/Transform60
ASAN, actual generated mixed JS643/Transform168 and animation/gateway checks,
dummy67/71→68/71 full deploy/rollback61. Initial test harness visibility/stage and
backup-name expectations were corrected; ASAN scene exe now runs inside the
vcvars process to retain its runtime search path. Failure logs are retained.
Build68/packed32836/CoreABI8/CEP71; persistent IDs/matchNames/schemas unchanged.
Full frozen SDK/panel verification and deployment pending. Installed67/71
retained until release receipts say otherwise. No AE process started or script
dispatched; owner chose manual testing. Motion/MNT work preserved; push pending.


2026-10-11 14:48+08 native67/packed32835/CoreABI8/CEP71已部署。源码816eb83fc0da5a9757cbf8c1e483bcfb32104380经git archive冻结，完整May2023 x64 Release /MT八目标会话75911实际exit0；冻结ASAN native8802/camera12/Transform60、真实生成JS混合644/Transform168及整套PanelAudit均exit0，dummy66/71→67/71发布与整套恢复61通过。fresh AE0后经Deploy-ModelTestBuild→Deploy-TestBuild -IncludeModel发布；26配对安装hash、全部27已测CEP文件和selector/runtime独立核对通过。首次Emitter Type绑定覆盖Effect计数缺失/0/999/throw和八UUID不匹配；删除native66未经证实的计数契约假设，改为固定UUID参数名称查找，未找到源与非有限值分开诊断且均回滚。实机stream98失败是否消失仍等待owner自行复测；没有启动AE或执行验收脚本，不能视为客户发布资格。IDs/schema/ABI8保持，Motion/MNT改动保留；Git push仍待已有独立授权回复，部署交接后按owner既有要求暂停。

2026-10-11 M3-17继续修复owner新建图层首次添加效果失败（stream98/parameter1/verify finite bound value/516）。native66增加的Effect.numProperties布局计数没有AE契约证据，旧JS桩从生成表达式反推正确计数，漏掉缺失/不同/抛错计数。native67候选改用Effect.param("Node UUID 0"…"Node UUID 7")固定常量名称，八字全部匹配后才访问源节点数字参数；主效果自身、其他renderer、不同kind及同名不同UUID均不会打开其动画别名。完整UUID唯一性仍由原生编译保证，source求值错误不吞掉，sentinel与非有限值分别报verify source binding resolved和verify finite bound value，失败仍完整恢复表达式。无IDs/matchName/schema/ABI8迁移，CEP71保持。首个Emitter Type真实生成表达式覆盖计数缺失/0/999/throw，native66 gate在同一夹具返回sentinel，新表达式返回1；不是AE实机根因或验收声明。原生/ASAN及冻结完整构建继续；owner选择自行测试，本轮无AE启动或脚本调度。

2026-10-11 14:27+08 native66/packed32834/CoreABI8/CEP71已部署。源码186cbf3e1a99f989019f040fcb74846ff342213f git archive冻结，May2023 x64 Release /MT完整八目标会话57534实际exit0，冻结mixed-ASAN/native8796/camera12/JS644及整套PanelAudit实际exit0。完整OBJ/Core4145标准-ASAN、控件453/导入20632标准-ASAN、Model模块9434、dummy65/71→66/71整套回滚61通过；small172/owner271自动验收自检通过。fresh AE0后Deploy-ModelTestBuild→Deploy-TestBuild -IncludeModel发布；26配对安装hash及全部27冻结面板字节、selector/runtime核对通过。owner选择自行测试并稍后反馈，本轮未启动或向AE调度验收。OBJ误拒绝已离线确认修复；Model/Transform内部错误根因及速度改善未据夹具关闭。 具体日志及回滚见current-state.md。

2026-10-11 13:38+08 M3-17 native65/CEP71已部署：同一ES3 ordinary control contract嵌入browser规划和AE gateway，用于五类完整布局投影和preflight；Model最终核对调用匹配build/contract API。提交后按plain token重取receipt/snapshot。源码9b77643冻结/27字节匹配、fresh AE0、26hash/selector/runtime/配对回滚通过；schemas/IDs/ABI8及native65保持。实际注册矩阵和标准/ASAN通过，首次获准AE启动未产生结果、owner延后现有实例自动调度；native66源码候选修正OBJ小数导出容差并在数字子流访问前检查完整effect布局，实机Model/Transform错误仍待确认，详见docs/model-transaction-audit.md。

2026-10-10当前配对native62/CEP64修正Model project root ID0：CEP/原生事务、资产传输及OBJ checkpoint允许非负root，0仍须与实际SDK根项匹配；comp/layer保持正值。完整May2023八目标构建及26安装hash/rollback通过，真实AE Model gate保持开放。该源码包含M3-18数字计算层与snapshot10读取/写入能力，但没有Motion原生/CEP作者入口；owner允许未来按可操作模式分批部署，不要求三个作者一起完成。

当前版本与部署证据见 [current-state.md](current-state.md)；这里维护职责和契约，历史构建叙述见 [历史记录](history/README.md)。

## 模块和数据流

```text
CEP 节点画布 / AE 效果定位 / 预设
  -> ExtendScript 普通参数流、UUID、数值 revision/checksum 收据
  -> 原生节点 AEX + StarfieldParticle.aex 图提交与持久化
  -> SmartFX 参数/历史/相机采样，不可变预渲染快照
  -> 当前 Core generation 的 C ABI / CPU 或准备后的 GPU 场景
  -> AE 输出世界
```

| 模块 | 职责 |
| --- | --- |
| StarfieldParticle.aex | 主渲染效果、ARB 图持久化、参数注册、SmartFX、Core 加载与输出转换 |
| Emitter/ParticleNode/Force AEX | 菜单隐藏的独立原生作者参数；节点效果 SmartFX 透传输入 |
| Transform AEX | Transform 控件与 Null 资源作者记录；CEP 资源与属性集成已实现，AE 宿主验收开放 |
| Model AEX | 静态 OBJ 数值网格与变换作者、导入检查点；资源传输和预设已接入，AE 宿主验收开放 |
| StarfieldHost.aex | UI idle 上有界初始化/绑定、Model资产与整笔事务排队；原生文件窗口与UI范围互斥 |
| StarfieldCore.dll | 纯数值图求值、确定性粒子模拟、CPU 栅格化、GPU 场景准备 |
| cep_panel/ | 图 UI、AE 效果定位、预设、守卫事务与自适应检查；参数/曲线/渐变在 AE 编辑 |

Output 是主渲染效果在画布上的固定终端，没有独立 Output AEX。Auxiliary 使用 Emitter 原生效果的结构选择。Appearance 已退出当前表面。

## 核心与宿主边界

Settings.hpp 拥有纯 Core 值类型；Render.hpp 定义宿主无关渲染边界。AE 类型、suite、句柄和世界指针停在 ae_plugin/。
C ABI 使用固定宽度数值、字节跨度与不透明结果句柄；创建和释放在同一 generation/CRT 中完成，C++ 对象、异常和分配器不跨 DLL 边界。

主 AEX 也编译图编解码、构建与部分求值/历史代码，因为它需要制作持久化记录和冻结快照；DLL 使用这些共享实现进行渲染。不能把所有 GraphEvaluation 调用都描述为 DLL 独占。
CoreOnly 的 adapter-input 指纹会检查共享输入，runtime 发布还核对已安装的原生配对哈希；共享输入或 ABI 变化必须先完整成对构建。

## 图、模拟和样式

图 envelope1 使用独立 UUID、类型 key、强类型参数、端口与有界校验。主图存于隐藏 ARB 参数；sequence data 不存放可变模拟。
参数磁盘 ID、流索引、图 key 和 UI ID 各自独立；具体版本/索引由 schema 与对应头文件拥有。

粒子按 seed、稳定身份和绝对时间求值，支持乱序帧。Emitter 决定出生与初始速度；Particle 决定 lifetime、形状、基础样式和 Over Life；Force 按图路径作用；Output 对全图实行共享 population cap。
辅助发射采样父粒子在子出生时的状态。动画/积分语义见 ADR0023/0024/0025/0026；曲线、渐变和旋转见 ADR0027/0028/0030。

Transform 分别合成中心与精灵基底，保留 shear/reflection；Force 使用下游 Transform suffix。共享基底表最多4096项，snapshot4 保留200字节的显式粒子记录，仍能读取 snapshot3；内存对象 sizeof 不定义 wire stride。Null 作者选择与数值采样契约见 ADR0032。

M3-18未发布Core候选新增Motion schema1的Circle与数字Look At实际graph求值：Circle显式角速度及Over Life积分、历史值与metadata proof、有界数字lease及轨道瞬时速度；Look At当前goal/明确forward和加权最短弧。逐粒子单位四元数在Euler/共享仿射之后施加于CPU/GPU场景准备/Model，保留剪切和镜像；snapshot9显式72B header/232B记录，单位朝向仍保留旧3..8格式，Linear shutter用SLERP。C ABI8/sequence1/AE IDs不变，共享adapter/Core输入须完整成对构建。当前支持Force/Transform前置、Circle→Look At及各自串联，反序/后置frame及不同链合并明确拒绝；三模式作者、外部Light资源/完整frame/动画轴原点导数及公开参考政策仍未完成，见ADR0039。该Core能力没有对应已部署原生/CEP入口，不能由registry条目宣称首批Motion交付。

Light Path的数字候选已接入mode0普通/历史graph和Auxiliary：SFMP1有界点包、本地key9..12、实际age曲线及出生时delay、动画speed/random的绝对midpoint或显式metadata常量积分。当前几何按node/time不可变lease缓存，历史clock缓存4096项；20M全请求work含自适应编译/曲线，32MiB累计capacity路径预算跨Auxiliary共用，不因淘汰退款。位移/速度在style前，切线朝向在Euler/共享affine后；保留发射分布与剪切镜像，snapshot9已有pose契约不变。外部光源依赖/GUID、完整有序frame与作者尚未接入，clamp及明确数字delay不认定参考末端/Max Delay/Delay Random，公开控件前必须确认。

后续M3-18有序frame候选已替代上述分离位置/朝向的执行方式：Particle样式/Euler先建立，Motion和后置Transform逐段按图顺序作用；普通/历史/Auxiliary共用Q*A*B*Euler。逐粒子row-major仿射A保持剪切/镜像，不占共享4096表；后置L以A'=L*Q*A、Q'=identity合成，前置仍用共享B。snapshot10显式80B header/304B记录，仅非单位A写10，原3..9保持；CPU/GPU准备/Model及Linear affine插值同步。反序Motion、后置Transform与交错串联已支持，后置Force/不同链合并及动画几何导数仍开放。共享输入须完整冻结配对构建；安装61/63没有该Motion作者入口。

M3-17 Model实现的纯数值几何类型由Settings.hpp拥有，ModelGeometry处理单位cube与有界OBJ角点/三角化/验证及一次验证的mesh lease。ModelScene用显式model-to-layer矩阵制作裁剪三角形、四采样覆盖/深度和独立粒子叠加。ModelResources负责SFMG1网格codec和粒子pose；RenderRequest自有网格，Core ABI8追加有界数值数组并接受ABI7精确前缀。snapshot8保留200B粒子并共享模型组，旧3..7保留读取；CPU绘制shape4，多个成员合并覆盖/深度再应用一次opacity。live图已接入Model元数据端口；完整依赖用于拓扑规划，粒子流用于发射/Force/Transform/Auxiliary，保持粒子身份与共享组。Temporal每帧采样一次活动Model；Linear shutter迁移组索引、对相同资源插值仿射矩阵。原生/CEP作者和有界资源传输已接入。Core输入只有字节文本与数字，文件IO和宿主操作在AE适配器内。迁移和完整Model gate由ADR0038管理，现有测试安装native61/ABI8/CEP63；AE2023实际验收开放。

## 渲染、资源与线程

ModelGeometryParameter在ae_plugin内管理有界SFMG1任意参数句柄；保存数值网格，UUID/revision由Model作者管理。复制拥有独立存储，读取先复制并解锁，再解码/取消。Model参数已注册，所有ARB selectors已有独立fake-host覆盖；未增加宿主资格。

SmartFX 构造不可变图/历史、几何和 Core lease；相同 generation 保持到对应 render/result 释放。内容身份参与 cache GUID。渲染输出为透明背景上的粒子 RGBA，不合成输入像素。
Core 验证时间、尺寸、格式、图、数值和工作预算；adapter 尊重 rowbytes、PAR、downsample、ROI、AE 16-bpc 的32768刻度和8/16/32-bpc 输出。内部积累为 premultiplied，编码 alpha 由请求决定；32-bpc 可以保留 HDR RGB。

Camera.cpp 内仍有相机/效果层 suite 查询；原生 sibling 作者编译与 Null 绑定准备由 UI 路径承担。不要将“禁用 MFR”等同于所有 render 都在主线程；各 suite 的 selector/thread 允许范围仍属宿主审计事项。
渲染请求不从 CEP 获取状态；host suite 或 checkout 不在持锁区内调用。UI 缓存不持有超出回调有效期的 host 对象。

GPU 通过声明后的设备协商、场景准备和独立核函数执行；CPU 是数值对照路径。Motion Blur 有 endpoint Linear 与实际 Subframe Sample 两种数值策略；前者是有界近似，详见 ADR0031。MFR/Compute Cache 的启用仍需要独立验收。

## 开发规则

- 发布过的身份、ID、schema、版本变化须带迁移计划和 ADR。
- 构建候选与发布/宿主支持分开记录；只有相应 host family/build 的实际行为才能关闭宿主 gate。
- 构建及清理输出在 artifacts/；dist 经 AE Junction 使用，清理工具不触碰它。
- ADR0011 管理所有宿主改动；注册表不由任何仓库脚本写入。
- 新测试及证据入口见 [testing.md](testing.md)，当前任务所有权见 [agent-backlog.md](agent-backlog.md)。
