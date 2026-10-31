# 构建与安装消费

[English](build.md) | **简体中文**

根 CMake 项目构建 C++20 静态目标 `hibiki::physics`。构建信息 API 提供可链接符号，用于安装和调试器检查。工具链测试覆盖 GoogleTest、Eigen、spdlog、Google Benchmark 和浮点行为。

`CMakePresets.json` 定义 Windows Debug、RelWithDebInfo、Release、ASan 和 MSVC 兼容性配置。各配置采用独立构建及安装目录。Windows 工具链从 `.local/environment.json` 读取选定路径，从已提交组件清单解析 SDK 修订。C 和 C++ 编译路径选择同一个编译驱动。

环境路径准备完成后，以下命令构建并测试 Debug：

```powershell
$environment = Get-Content .local/environment.json -Raw | ConvertFrom-Json
& $environment.paths.cmake --preset windows-debug
& $environment.paths.cmake --build --preset windows-debug
$ctest = Join-Path (Split-Path $environment.paths.cmake) ctest.exe
& $ctest --preset windows-debug
```

`scripts/environment/VerifyCpp.ps1` 构建配置矩阵、执行 CTest、安装静态库，并针对各安装目录构建独立的 `examples/install-consumer` 项目。ASan 配置还要求受控子进程失败，并产生带符号的堆越界报告。日志和 JUnit 结果写入 `artifacts/environment/`。

```powershell
& ./scripts/environment/VerifyCpp.ps1
& ./scripts/environment/VerifyCpp.ps1 -Presets windows-debug
```

消费项目使用 `find_package(HibikiPhysics CONFIG REQUIRED)`，链接导出的 `hibiki::physics` 目标。C++ 依赖归档按清单中的固定身份选择，从本地目录读取。项目安装包导出自身静态库和头文件。

Windows Debug 使用调试动态 CRT，优化配置使用发布动态 CRT。ASan 使用发布动态 CRT，并关闭优化。LLVM 构建显式关闭 fast math 和乘加融合，采用 x86-64 基线。MSVC 兼容性构建使用严格浮点模式。内部断言配置在构建信息 API 中独立表达。

当前证据：五种配置分别通过全部 11 个 CTest 用例及独立安装消费程序的编译与执行。ASan 子进程产生预期的带符号堆越界报告。MSVC 干净重建和头文件依赖记录已验证。完整环境准备、编辑器与调试集成及重建复现均已通过，证据见[验收记录](acceptance.zh-CN.md)。
