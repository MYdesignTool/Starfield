# Starfield WebView2 前端

此目录由 StarfieldHost 的 AEGP Panel + WebView2 加载，提供节点画布和独立预设窗口。它由历史 `cep_panel/` 改名而来，属于正在使用的产品源码。不要当作旧 CEP 残留删除，也不要重新安装 CEP。

## 当前行为

保留 Emitter/Auxiliary/Particle/Force/Transform/Motion/Turbulence/Model/Material/Output 的现有画布、选择、拓扑和预设工作流。Output 是固定终端，Auxiliary 复用 Emitter 原生作者。参数、动画、曲线及渐变在 AE Effect Controls 中编辑；旧浮动属性编辑器和 Appearance 不恢复。

`js/webview_host.js` 适配既有前端传输 API；原生回调仅排队，AE 脚本在安全 UI idle 上执行。请求保留大小、顺序、期限、身份、版本和回滚校验；AE 模态保存/退出期间暂停有效队列。共享 JSX/native 事务仍负责图持久化和预设 Add/Replace。

## 安装与恢复

当前入口是 AE Window 菜单的 Starfield Particle Controls (WebView2) 和 Starfield Presets (WebView2)。前端和原生配对由仓库部署工具维护，不运行旧 CEP 安装器。`CSXS/manifest.xml` 仅保留协议版本校验和显式历史兼容用途，不表示 CEP 仍注册，也不扩大宿主支持范围。

旧注册的精确清理状态和恢复命令见 [当前状态](../docs/current-state.md)。`Install.ps1` 现在只报告退役说明。任何宿主注册恢复都要独立授权；脚本不写注册表。

```powershell
node tests/RunPanelTests.cjs
```

测试支持 `STARFIELD_PANEL_ROOT` 选择隔离候选；实际 AE 验收与离线测试分别记录在 [测试说明](../docs/testing.md) 和 [行为验收](../docs/compatibility-matrix.md)。
