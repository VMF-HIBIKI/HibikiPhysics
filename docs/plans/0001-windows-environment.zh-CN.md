# Windows 开发环境实施计划

[English](0001-windows-environment.md) | **简体中文**

状态：Windows 范围的 ENV-001 至 ENV-006 已完成。

日期：2026-10-31。

## 目标与范围

首期交付 Windows x64 开发环境、最小可安装 C++ 静态库及独立工具链验收用例。Windows 验证优先完成，随后推进 Linux 和 macOS。物理算法、World/刚体 API 和仿真场景归入后续计划。

初始库提供公共 CMake 目标 hibiki::physics。最小可调用接口返回项目或构建标识，使真实目标文件参与编译、链接和外部消费，该接口作为基础设施 API 审查。数值行为及诊断用例放在独立工具链验证目标中。

静态库沿用动态 CRT 策略：Release 和 RelWithDebInfo 使用 /MD，Debug 使用 /MDd，Windows ASan 诊断配置使用 /MD。共享库支持留作后续扩展。

ENV-001 交付身份声明、归档核验和双语记录。工具安装、环境准备、钩子启用和物理实现按后续阶段推进。当前证据见 [Windows 身份记录](../environment/windows-identities.zh-CN.md)。

验收证据见 [Windows 验收记录](../environment/acceptance.zh-CN.md)。

## 决策依据

- [ADR-0001：工具链与依赖](../decisions/0001-development-toolchain-and-dependencies.zh-CN.md)
- [ADR-0002：编辑器、调试与代码质量](../decisions/0002-editor-debugging-and-code-quality.zh-CN.md)
- [ADR-0003：提交钩子与验证](../decisions/0003-commit-hooks-and-validation.zh-CN.md)
- [ADR-0004：构建与数值策略](../decisions/0004-build-and-numerical-policy.zh-CN.md)
- [ADR-0005：内存与运行时诊断](../decisions/0005-sanitizers-memory-and-runtime-diagnostics.zh-CN.md)
- [ADR-0006：初始化与复现](../decisions/0006-environment-initialization-and-reproduction.zh-CN.md)
- [ADR-0007：跨平台与 CI 范围](../decisions/0007-platform-and-ci-validation.zh-CN.md)
- [ADR-0008：Linux/macOS 后续基线](../decisions/0008-linux-and-macos-toolchains.zh-CN.md)

ADR 负责工具版本和规则，本计划负责实施顺序、文件职责及验收证据。兼容性发现导致版本调整时，对应 ADR 与机器可读基线同步更新。

## 任务与执行顺序

以下编号属于本地规划引用。未来实现 Issue 与 PR 使用表中完全一致的英文标题，当前规划在本地进行。

| 编号 | Issue 与 PR 标题 | 前置条件 | 交付物 |
| --- | --- | --- | --- |
| ENV-001 | build: Record Windows toolchain and dependency identities | 已接受 ADR | 来源记录、完整发行身份及校验结构 |
| ENV-002 | build: Add read-only Windows environment diagnostics | ENV-001 | doctor 入口和结构化诊断 |
| ENV-003 | build: Add the static library and acceptance projects | ENV-001 | 构建文件、安装导出及小型验证用例 |
| ENV-004 | build: Add reproducible Windows environment preparation | ENV-001、ENV-002 及 ENV-003 输入 | prepare、本地配置和依赖准备 |
| ENV-005 | build: Integrate VS Code and project quality tools | ENV-003、ENV-004 | 编辑器模板、质量检查入口和钩子配置 |
| ENV-006 | test: Verify Windows environment reconstruction | ENV-002 至 ENV-005 | 验收执行和双语报告 |

清单约定建立后，ENV-002 和 ENV-003 可以独立编写。可执行验证安排在环境准备完成后。每阶段先记录证据，再更新完成复选框。

## 阶段一：身份与声明

规划文件包括：

- tools/toolchain-lock.json 及其 schema：工具精确版本、平台包、URL、哈希、组件身份和完整性校验。
- cmake/Dependencies.cmake：固定 C++ 源码引用、归档哈希及依赖选项。
- pyproject.toml 与 .python-version：选定 Python 工具、运行依赖及解释器。
- third_party/licenses/：选定发行包的许可证文本及来源。
- docs/ 下的双语环境说明。

必需来源记录包括 LLVM、CMake、Ninja、uv 归档，托管 Python 构建，MSVC 安装组件与工具集文件，SDK 服务修订，VSIX 包和 C++ 源码归档。每条记录注明核验状态，必需身份尚待确认时，准备就绪检查报告失败。

Python 声明区分质量/开发工具与可选 Warp 用途，直接依赖精确版本沿用 ADR。首份 uv.lock 在阶段四选定 uv 可用后显式生成，解析版本、索引、条件标记和哈希经过审查，再执行锁定同步。后续准备读取已提交锁文件。

- [x] ENV-001 完成 Windows 范围的声明和来源核验。
- [x] 源码归档及随附声明与记录身份一致。
- [x] Schema 校验能够区分完整记录与待补齐信息。

## 阶段二：只读 doctor

已实现入口为 scripts/dev.ps1 doctor，使用方式和验证范围见[环境诊断说明](../environment/doctor.zh-CN.md)。启动检查使用 Windows PowerShell 能力，可诊断项目 Python 或 uv 缺失。共享辅助代码位于 scripts/environment/。

doctor 检查可执行文件身份、Visual Studio 安装、选定 MSVC 头文件与库、SDK、架构、本地覆盖、解释器和扩展路径，检测重复版本字段的一致性，并报告拟使用的工具链解析结果。

默认输出终端报告，显式输出参数可以在 artifacts/environment/ 下生成 JSON。结果区分匹配、缺失、偏差和身份待确认。必需检查失败时进程返回失败，并提供准备步骤。

- [x] ENV-002 准确报告必需工具和实际路径。
- [x] 工具缺失及版本错误的测试输入产生明确失败。
- [x] 默认执行保持环境和文件系统原样。
- [x] 正确处理带空格路径及显式路径覆盖。

## 阶段三：静态库与验收源码

规划文件包括根 CMakeLists.txt、CMakePresets.json、cmake/HibikiPhysicsConfig.cmake.in、Windows toolchain 模块、最小公共构建信息头文件与实现，以及 tests/toolchain/ 下的验证项目。

静态目标导出 hibiki::physics。安装规则将头文件、库和 CMake 包元数据放入 install/<preset>/。独立消费程序使用 find_package(HibikiPhysics CONFIG REQUIRED) 并链接安装目标，检查头文件与库均从安装树解析。

Presets 覆盖 Windows clang-cl Debug、RelWithDebInfo、Release 和 ASan，另外准备独立 MSVC 兼容性配置。各配置使用独立构建目录与编译数据库。

核心库的测试、基准和工具通过明确选项控制。Eigen/GoogleTest 属于测试目标，Google Benchmark 属于基准目标，spdlog 属于工具目标。外部依赖使用已校验的本地归档，内容缺失时报告准备诊断。独立 C++ 消费程序通过已准备的 C++ 包运行，其使用路径独立于 Python 环境。

- [x] ENV-003 源码定义真实静态归档和可安装的 CMake 目标。
- [x] Presets 表达已接受的编译器、CRT、优化和数值规则。
- [x] 第三方构建选项和安装边界明确。
- [x] 工具链用例与后续物理实现分别组织。

## 阶段四：环境准备

已实现入口为 scripts/dev.ps1 prepare，使用方式见[环境准备说明](../environment/prepare.zh-CN.md)。准备过程解析清单、获取归档、检查哈希、解压到临时目录并将已验证产物移入版本目录，复用已匹配组件。MSVC/SDK 安装通过显式系统组件选项调用官方流程。

选定 uv 准备精确解释器发行。首次锁文件生成遵循阶段一流程，常规准备采用 uv sync --locked。验收配置区分 C++/质量检查与 Warp 要求。

生成文件包括 .local/environment.json 和 CMakeUserPresets.json。会话及子进程环境解析选定工具。清理在修改前验证解析路径和范围。离线准备预先检查内容完整性，安装与钩子/编辑器启用分别记录。

- [x] ENV-004 仅准备声明中的产物和设置。
- [x] 审查后的 uv.lock 在常规锁定同步前提交。
- [x] 重复准备复用匹配组件。
- [x] 中断下载、错误哈希及离线缺失具有可恢复结果。
- [x] 构建和测试入口使用已准备输入，日志保留实际命令。

## 阶段五：编辑器与检查集成

规划文件包括 .clang-format、.clang-tidy、.editorconfig、.pre-commit-config.yaml、pyproject.toml 中的 Python 工具配置、tools/vscode/ 下的版本化编辑器模板，以及 check/format/verify 脚本。

根 .gitignore 明确覆盖生成缓存和报告。共享 VS Code 模板进入版本管理，本地生成设置解析机器路径，独立 Profile 使用固定扩展身份。钩子启用保持为 prepare 的显式选项。

编辑器从选定 LLVM 启动 clangd，并读取匹配的编译数据库。cppvsdbg 负责 C++ 调试，Python 使用项目 basedpyright/Ruff 服务及锁定 debugpy 的 attach 流程。格式、lint 和钩子命令共享固定工具及已接受规则。

- [x] ENV-005 将编辑器能力连接到已验证项目工具。
- [x] 格式化与检查模式遵循 ADR-0002、ADR-0003。
- [x] 钩子测试覆盖部分暂存、安全修复和失败报告。
- [x] 手写 Python 类型声明与生成文件排除遵循约定范围。

## 阶段六：独立小用例验收

| 用例 | 预期证据 |
| --- | --- |
| 静态库与安装消费 | 真实符号进入静态归档，独立消费程序查找并调用成功 |
| GoogleTest/CTest | 小型参数化算术或容器测试通过，受控失败验证错误传递 |
| Eigen | 固定小矩阵运算与显式预期值一致 |
| spdlog | 小型工具中的终端、文件输出及 flush 正常 |
| Google Benchmark | 有限规模合成工作负载产生机器可读输出 |
| 数值规则 | 带符号零、NaN/Inf、下溢和乘加用例检验选定配置 |
| Windows ASan | 隔离受控错误产生带符号报告和子进程非零退出，正常对照通过 |
| Python 质量工具 | 受控 lint/类型问题产生预期诊断，正常对照通过 |
| Warp CPU/GPU | 小型逐元素内核在要求的设备配置下与参考值一致 |
| 编辑器与调试 | C++/Python 断点、调用栈、变量观察和源码导航有效 |
| 重建复现 | 新构建目录或工作副本，以及已校验缓存的离线使用复现必需结果 |

受控错误程序通过显式启用的子进程运行，验收执行器将预期检测视为成功，并检查诊断类别，使无关崩溃能够区分。普通冒烟用例保持有效程序行为。编辑器检查保留人工清单及证据，命令行检查记录自动结果。

最终报告记录工作副本/提交身份、本地变更、清单摘要、工具、命令、配置、必需/可选检查及结果。阅读摘要提供中英文两份，结构化证据使用稳定字段名。必需 GPU 检查缺失时，GPU 验收状态保持待执行或失败。

- [x] ENV-006 验证必需 Windows 配置及用例。
- [x] 报告区分通过、失败、省略和待执行。
- [x] 干净重建与离线重建通过。
- [x] 安装消费和诊断证据已保存。

## 完成与后续阶段

Windows 环境完成需要完整的选定身份清单、脚本和设置、必需验收用例通过，以及双语复现说明。待完成要求在报告中保持可见。

Windows 里程碑完成后，Linux 容器准备和 macOS 验证按 ADR-0008 推进。远程 CI 在托管建立后启用。物理实现遵循独立的范围与设计工作，API、ABI 和共享库扩展由后续明确决策管理。
