# 准备 Windows 开发环境

[English](prepare.md) | **简体中文**

准备入口使用仓库中的工具与 Python 依赖身份，检查选定的 Visual Studio 和 SDK 安装，获取已校验归档，准备版本化工具，同步锁定 Python 环境，并记录安装文件身份。

```powershell
pwsh -NoProfile -File scripts/dev.ps1 prepare -RequireWarp -WithEditor -EnableHooks
pwsh -NoProfile -File scripts/dev.ps1 doctor -RequireWarp
```

`-RequireWarp` 包含 Warp 依赖组，`-WithEditor` 准备独立 VS Code Profile 和项目设置，`-EnableHooks` 安装固定版本 prek Git 钩子。这些开关在自动化中明确相应变更。已有系统编译器和 SDK 组件通过身份检查后复用，缺失或偏差项通过诊断列出，并通过官方安装流程处理。

## 存放与重复执行

| 位置 | 内容 |
| --- | --- |
| `%LOCALAPPDATA%/HibikiPhysics/tools/<tool>/<version>` | 已验证的工具发行包 |
| `%LOCALAPPDATA%/HibikiPhysics/cache/downloads/<sha256>` | 按内容哈希索引的归档缓存 |
| `%LOCALAPPDATA%/HibikiPhysics/cache/uv` | uv 包缓存 |
| `%LOCALAPPDATA%/HibikiPhysics/vscode` | 独立编辑器数据和扩展目录 |
| `.local/archives` | 工作副本归档视图，在支持时采用硬链接 |
| `.local/receipts` | 归档与安装文件的核验记录 |
| `.local/environment.json` | 选定路径和准备输入 |
| `.local/python-environment.json` | 锁文件身份、包版本和安装文件哈希 |
| `.venv` | 工作副本 Python 环境 |
| `.vscode` | 渲染后的本地编辑器设置、任务和调试配置 |
| `CMakeUserPresets.json` | 本地 Preset 别名和归档路径 |

已有工具经过文件内容检查后复用。归档核验将安装文件与选定归档内容逐一比较。Python 字节码缓存作为生成文件处理，源码、解释器和库文件保持检查。VS Code 为扩展清单增加安装元数据，归档比较时将其与原始包清单分别处理，核验记录保存最终安装文件的哈希。

`uv.lock` 使用选定 uv 显式生成，经审查后保存在仓库。常规准备执行 `uv sync --locked`。已安装发行包与选定依赖图及 RECORD 哈希核对，Python 核验记录将这些文件绑定到锁文件摘要。可选的生成字节码位于不可变文件清单之外。

```powershell
pwsh -NoProfile -File scripts/dev.ps1 prepare -Offline -RequireWarp -WithEditor -EnableHooks
pwsh -NoProfile -File scripts/dev.ps1 prepare -ArchiveDirectory artifacts/env001/downloads -RequireWarp
```

`-Offline` 要求归档和 Python 包缓存齐备，`-ArchiveDirectory` 在核验哈希后导入已下载归档。下载中断时保留 `.part` 文件，后续请求可以继续下载。哈希失败时保留证据并终止准备，已完成核验的缓存条目可以继续复用。

## 编辑器与质量工具

`HibikiPhysics` Profile 使用选定的 VS Code 发行版及独立用户数据、扩展目录，首次初始化时打开项目窗口。模板位于 `tools/vscode/`，渲染器将机器路径写入 `.vscode/`。Profile 固定扩展版本，并关闭编辑器和扩展自动更新。

clangd 使用 Windows Debug 编译数据库，C/C++ 扩展提供 cppvsdbg。项目 Python 服务通过 basedpyright 和 Ruff 使用选定环境。调试模板包含 C++ 验证程序配置及本机回环地址的 Python debugpy attach 配置，LSP 和调试器运行检查属于验收阶段。

```powershell
pwsh -NoProfile -File scripts/dev.ps1 configure -Preset windows-debug
pwsh -NoProfile -File scripts/dev.ps1 build -Preset windows-debug
pwsh -NoProfile -File scripts/dev.ps1 test -Preset windows-debug
pwsh -NoProfile -File scripts/dev.ps1 check
pwsh -NoProfile -File scripts/dev.ps1 format
```

`check` 验证格式、Python 类型、项目 C++ 诊断、文件语法、文本规范及双语链接；`format` 执行选定格式化工具和 Python 安全 lint 修复。C++ 构建预检查采用独立的 C++ 诊断范围，其身份检查可以独立于 Python 包和编辑器集成运行。

钩子配置使用固定本地工具及 [prek 内置文件检查](https://prek.j178.dev/reference/built-in-hooks/)，快速钩子针对暂存文件集合执行。独立 Git 测试覆盖部分暂存、未暂存修改保留、lint 失败传递和格式化工具版本偏差。第三方许可证字节内容保留在项目格式化范围之外。

## 验收证据

初始锁定环境包含 13 个已安装包，包括 Warp 1.18.0。归档文件核验和 Python 安装清单核验通过，完整 doctor 报告已达到 `ready: true`。项目文件钩子、三项独立 Git 行为测试和三项 Warp CPU／CUDA／Graph 测试通过。LSP、调试器和重建复现已通过，结果见[验收记录](acceptance.zh-CN.md)。
