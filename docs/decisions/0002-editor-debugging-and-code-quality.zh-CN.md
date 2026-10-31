# ADR-0002：编辑器、调试与代码质量

[English](0002-editor-debugging-and-code-quality.md) | **简体中文**

状态：Accepted（已接受）。

日期：2026-10-31。

已确认偏好：VS Code 是主编辑器。

验证状态：Windows 编辑器、CLI 和调试集成通过。
当前 Windows 证据见[验收记录](../environment/acceptance.zh-CN.md)。

## 背景

项目需要一致的编辑器与命令行诊断、可复现格式化以及有效的 Windows 调试。[ADR-0001](0001-development-toolchain-and-dependencies.zh-CN.md)定义编译器和依赖基线。

## 候选方案

Windows 原生调试可以使用 cppvsdbg 提供的 Visual Studio Windows Debugger，或基于 LLDB 的适配器。本决策选择 cppvsdbg，匹配当前 MSVC ABI 和 PDB 流程。其他平台单独配置调试器。

Python 语言服务可以使用 Pyright/Pylance、basedpyright 或其他 LSP 实现。本决策选择 basedpyright，其 CLI 和语言服务由同一个 Python 包提供，适合通过 uv 固定版本。Ruff 的原生语言服务与 CLI 负责 lint 和格式化。

## 选定版本与职责

| 组件 | 选定版本 | 职责与来源 |
| --- | --- | --- |
| VS Code | 1.143.0，Windows x64 | 本机编辑器参考版本，记录用于复现 |
| C/C++ 扩展 | ms-vscode.cpptools 1.34.4，win32-x64 | 本机已有基线，cppvsdbg 提供 Windows 原生调试 |
| clangd 客户端 | llvm-vs-code-extensions.vscode-clangd 0.6.0 | 本机已有客户端，启动指定 clangd 22.1.6 |
| Python 扩展 | ms-python.python 2026.10.0，win32-x64 | 本机已有基线，用于解释器选择和测试集成 |
| Python Debugger 扩展 | ms-python.debugpy 2026.6.0，win32-x64 | 本机已有调试客户端基线 |
| debugpy 包 | 1.8.22 | 项目 Python 调试服务，由 uv 固定 |
| basedpyright | 1.40.2 | Python 补全、导航和类型检查 |
| Ruff | 0.17.3 | Python 格式化、lint 和导入排序 |
| C++ 质量工具 | clang-format、clang-tidy 22.1.6 | 已确定的 LLVM 基线 |

ENV-001 选定 detachhead.basedpyright 1.40.2 universal 和 charliermarsh.ruff 2026.92.0 win32-x64，VSIX 哈希记录在 tools/toolchain-lock.json 中。服务路径显式指向项目管理的可执行文件。basedpyright 的传递依赖包含 nodejs-wheel-binaries，uv.lock 记录解析后的版本和发行文件。

环境记录包含扩展 ID、版本、平台包和哈希。独立的项目编辑器 Profile 将扩展变更纳入明确管理。推荐扩展列表提供发现入口，环境检查器负责版本验证。

## 调试

Windows C++ 调试使用 cppvsdbg，加载选定 CMake 配置产生的程序和 PDB。Debug 配置生成完整调试信息并关闭优化；RelWithDebInfo 提供带优化的诊断配置。验收覆盖源码断点、单步、调用栈、STL 变量、项目类型及崩溃位置。项目类型布局稳定后，可以补充 Natvis 可视化描述。

clangd 负责 C++ 补全和诊断。C/C++ 扩展负责调试，其 IntelliSense 引擎及格式化功能在项目 Profile 中关闭。格式化显式使用固定版本的 clang-format。

Python 调试通过 uv 启动项目锁定的 debugpy，在本机回环地址监听，再由 VS Code attach。调试服务版本因此具备明确来源。解释器选择指向项目 .venv。debugpy 覆盖 Python 宿主代码，Warp 内核级调试由独立流程处理；初期内核检查采用 Warp 诊断和 CPU/参考结果对照。

## Python 语言服务

basedpyright 负责类型诊断、导航、补全和悬浮信息。Ruff 负责 lint、格式化和导入整理。每项能力配置一个明确的提供者。

类型检查采用 Python 3.12 和 standard 模式。普通脚本和宿主工具在类型接口建立后启用 strict。Warp 内核及适配代码初期采用 standard。动态 API 使用小型类型适配器，或附带原因的局部规则抑制。这些设置通过代表性 Warp 内核和宿主脚本验证。

## 格式化

| 设置 | C++ | Python |
| --- | --- | --- |
| 工具 | clang-format 22.1.6 | Ruff 0.17.3 |
| 基础风格 | 项目 C++ 规则 | Ruff 稳定格式化器 |
| 缩进 | Tab，显示宽度 4 | 4 空格 |
| 行宽 | 按语义换行 | 120 列 |
| 换行 | LF | LF |
| 其他选择 | Allman 花括号换行、指针和引用符号靠变量名、保留 include 顺序及注释换行 | 双引号、Python 3.12 目标、预览功能关闭 |

共享默认约定为 UTF-8、文件末尾换行及清理行尾空格。Markdown 保留有意设置的硬换行空格。PowerShell 文件沿用仓库 CRLF 约定。外部源码、生成代码、构建目录和虚拟环境位于项目格式化范围之外。

clang-format 设置为 IndentWidth=4、TabWidth=4、UseTab=Always、ColumnLimit=0、BreakBeforeBraces=Allman、PointerAlignment=Right、ReferenceAlignment=Right、DerivePointerAlignment=false、SortIncludes=Never、ReflowComments=Never、AlignConsecutiveDeclarations=Consecutive。声明列保持对齐，简短访问器可以保留单行形式。

C++ 类型和普通函数使用 PascalCase，成员使用 m 前缀，常量使用 c 前缀，静态函数使用 s 前缀。参数使用 in、out 或 io 前缀，分别表达输入、输出或输入输出角色。局部变量使用 lowerCamelCase。标准要求的名称及第三方重写接口保持规定拼写。Python 函数和变量使用 snake_case，类使用 PascalCase，常量使用 UPPER_SNAKE_CASE。

## 静态检查

### C++

项目目标在 clang-cl 和 MSVC 下使用 /W4。初始基线建立后，CI 将项目警告视为错误。Clang 的 conversion 和 sign-conversion 诊断初期作为提示，数值代码的转换策略单独审查。第三方目标的警告设置限定在对应依赖范围。

clang-tidy 使用当前编译数据库，并将诊断范围限定到项目代码。初始检查项如下：

```text
clang-analyzer-*
bugprone-assert-side-effect
bugprone-dangling-handle
bugprone-sizeof-expression
bugprone-use-after-move
performance-for-range-copy
performance-unnecessary-copy-initialization
modernize-use-nullptr
modernize-use-override
misc-definitions-in-headers
bugprone-unused-raii
```

每处抑制记录规则和原因。首次扫描建立阻断基线，新规则通过审查变更加入。数值约定、分配行为及数据竞争在后续验证中配合专项测试和运行时分析。

### Python

Ruff 基础规则集为 I、E、F、W，覆盖导入排序、代码风格和常见错误。忽略项为 E111、E114、E117、E741，其中 E741 的豁免允许数学代码采用简短符号名称。

文档字符串中的代码格式化开启（docstring-code-format=true），诊断输出使用 concise。第一方包名在 Python 包结构确定后显式配置。实际承担重导出的 __init__.py 文件按路径配置 F401、F403 豁免。生成文件按生成目录精确排除，手写 .pyi 文件继续接受检查。

编辑器、命令行、CI 及未来提交钩子均使用 uv 管理的 Ruff 0.17.3。VS Code 显式保存时执行 Ruff 格式化、安全修复和导入整理。提交钩子执行工具选定为 prek 0.5.9，由 uv 管理。[ADR-0003](0003-commit-hooks-and-validation.zh-CN.md)定义已接受的流程，钩子启用留待环境准备阶段。

项目指定 required-version=0.17.3、target-version=py312、line-length=120、preview=false。自动修复采用 safe-fix 策略，影响行为的变更经过明确审查。basedpyright 负责类型错误。CI 使用只检查命令，本地格式化生成可审查的修改。

## 配置与验证

当前实现使用 .clang-format 保存格式、.clang-tidy 保存 C++ 分析规则、pyproject.toml 保存 Python 工具配置、.editorconfig 保存通用空白规则。编辑器模板保存在受版本管理的配置目录，本地编辑器文件使用解析后的机器路径。共享编辑器配置落地时，明确调整现有 .vscode 忽略规则。

工具和源码准备完成后，预期检查命令如下：

```text
clang-format --dry-run --Werror <项目 C++ 文件>
clang-tidy -p <选定构建目录> <项目翻译单元>
uv run --locked ruff format --check .
uv run --locked ruff check .
uv run --locked basedpyright
```

验收包含实际服务版本、编辑器与 CLI 诊断一致性、格式化重复执行结果、代表性 C++/Python 调试，以及 Warp 注解的限定处理。Windows 安装、集成和运行验证已通过。

## 参考资料

- [VS Code C++ 调试](https://code.visualstudio.com/docs/cpp/cpp-debug)
- [debugpy](https://github.com/microsoft/debugpy)
- [basedpyright 安装](https://docs.basedpyright.com/latest/installation/command-line-and-language-server/)
- [basedpyright 配置](https://docs.basedpyright.com/latest/configuration/config-files/)
- [Ruff 编辑器集成](https://docs.astral.sh/ruff/editors/)
- [Ruff 设置](https://docs.astral.sh/ruff/settings/)
- [clang-tidy 检查列表](https://clang.llvm.org/extra/clang-tidy/checks/list.html)
