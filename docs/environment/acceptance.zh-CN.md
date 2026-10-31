# Windows 验收记录

[English](acceptance.md) | **简体中文**

日期：2026-10-31。结果：通过。

范围：Windows x64 开发环境与静态库基础设施。验收使用 PowerShell 7.6.5、LLVM 22.1.6、MSVC 编译器 19.51.36248.0，以及选择版本为 10.0.26100.0、服务修订为 10.1.26100.8249 的 SDK。

| 检查 | 证据 |
| --- | --- |
| 工具与依赖身份 | 22 个归档身份、固定 C++ 提交、安装文件核验记录及已审查的 `uv.lock` |
| 只读 doctor | 要求 Warp 时 76 项检查匹配，28 项回归检查通过 |
| C++ 配置 | Debug、RelWithDebInfo、Release、ASan、MSVC 五种配置，各通过 11 个 CTest 用例 |
| 失败传递 | 五次受控 GoogleTest 失败经 CTest 正确传递 |
| 安装包 | 五个独立消费程序针对安装目录完成编译和执行 |
| 内存诊断 | LLVM ASan 产生预期堆越界报告，包含源文件与行号 |
| Python 与环境检查 | 九项测试通过，覆盖钩子、部分暂存、归档恢复和 Warp |
| 编辑器 | C++／Python 定义跳转、补全、诊断、Ruff 格式化、断点、单步、调用栈和变量求值通过 |
| GPU | Warp 1.18.0 的 CPU、CUDA 和 Graph 结果与参考值一致，设备为 RTX 5070 Ti，驱动为 610.88 |
| 重建复现 | 带空格路径的独立副本完成离线准备、五种配置重建、安装消费及 Warp 检查 |
| 源码保持 | 验收与重建过程保持输入源码文件哈希一致 |

环境测试共用一个 Python 模块。测试目录保留四个 C++ 源文件、三个 Python 源文件和一个 PowerShell 回归脚本，安装消费程序作为示例项目保留。

干净重建暴露了 CMake 特性探测阶段的 ASan 库选择及 DLL 搜索顺序差异。工具链现已显式选择 LLVM 导入库，并优先使用对应运行库目录。MSVC 包含文件前缀采用显式 UTF-8 解码，Ninja 头文件依赖在干净构建后得到验证。

## 重复验收

```powershell
pwsh -NoProfile -File scripts/dev.ps1 prepare -RequireWarp -WithEditor -EnableHooks
pwsh -NoProfile -File scripts/dev.ps1 verify -RequireWarp -WithEditor -Reproduce
```

编辑器检查打开项目验收窗口。重建检查在 `artifacts/repro/` 下创建新副本，结束后恢复原项目编辑器设置。

本地证据保存于 `artifacts/environment/verification.json`、`doctor-final.json`、`editor-acceptance.json`、`reproduction.json`、JUnit 文件及命令日志。验收记录包含工具链摘要、Python 锁文件摘要和输入文件哈希，缓存与配置布局见[环境准备说明](prepare.zh-CN.md)。

Windows PowerShell 5.1 已完成语法检查，其运行仍受宿主执行策略约束；实际验收宿主为 PowerShell 7.6.5。Linux／macOS 及其附加 Sanitizer 配置属于后续平台工作。
