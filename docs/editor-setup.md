# 编辑器配置

打开 tools/Starfield.code-workspace 或仓库根目录；父 forensic 工作区不会应用本工程配置。MSVC 路径和头文件由 .vscode/c_cpp_properties.json 管理，C++20、May2023 AE SDK 和固定 WebView2 SDK 与构建保持一致。工具链更新后修改 starfieldCompiler。

所有配置默认加载 OpenClIntelliSense.h，只供 IDE 声明 get_global_id/内核限定符，不参与 CUDA/OpenCL 运行编译。默认配置也能解析 SpriteKernel.h，无需为它切换。查看 WebPanelHost 时选择 WebView2 Host；节点 variant 选择对应类型，详见 [.vscode 说明](../.vscode/README.md)。

缓存位于 artifacts/editor。若配置更新后旧红线仍在，可执行 Developer: Reload Window 或 C/C++: Reset IntelliSense Database。不要修改全局 IDE 配置或用户缓存。编辑器提示与实际编译结果分别判断，不能以红线或编译通过推断 AE 行为。
