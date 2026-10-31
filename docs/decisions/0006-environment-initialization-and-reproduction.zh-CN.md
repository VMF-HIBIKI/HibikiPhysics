# ADR-0006：环境初始化与复现

[English](0006-environment-initialization-and-reproduction.md) | **简体中文**

状态：Accepted（已接受）。

日期：2026-10-31。

验证状态：Windows 环境准备、诊断和离线重建通过。
当前 Windows 证据见[验收记录](../environment/acceptance.zh-CN.md)。

## 背景与决策

已接受的工具链和依赖需要明确选择、可恢复准备及可重复验证。工具按版本保存在用户目录，每个工作副本独立维护虚拟环境和构建产物。仓库声明表达要求，本地报告记录实际环境。

## 存放位置

Windows 默认根目录为 %LOCALAPPDATA%/HibikiPhysics，包含 tools/<工具>/<版本>、保存托管解释器的 python、按内容寻址的 cache/downloads，以及由 uv 管理的 cache/uv。本地配置可以覆盖根目录。

MSVC 和 Windows SDK 保存在官方安装位置，按版本和组件身份选择。显式路径覆盖可以在身份校验后复用已有工具。CI 和首次复现优先使用清单指定的发行包。

| 工作副本目录 | 职责 |
| --- | --- |
| .local/ | 解析后的路径、本机配置和环境身份 |
| .venv/ | 工作副本独立 Python 环境 |
| build/<preset>/ | 独立构建配置与依赖构建树 |
| install/<preset>/ | 安装验收产物 |
| .cache/warp/<profile>/ | 按配置区分的 Warp 内核缓存 |
| artifacts/ | 环境报告、测试日志和诊断 |

生成目录加入 Git 忽略规则。源码、已提交配置和手写本地记录分别维护生命周期。

## 初始化入口

| 入口 | 职责 | 影响 |
| --- | --- | --- |
| doctor | 检查版本、路径和环境变量 | 默认终端报告，显式输出选项生成报告文件 |
| prepare | 获取校验工具、准备 Python/依赖并生成本地配置 | 用户工具目录与工作副本生成文件 |
| verify | 构建、测试、安装消费验证及工具集成检查 | 构建产物与验收报告 |

初始 Windows 入口采用 PowerShell，各平台入口保持相同语义。准备过程依次验证清单必需字段、定位组件、校验下载归档、使用固定 uv 准备指定 Python 发行、同步 uv.lock、获取已验证 C++ 归档，并生成本地路径及 CMake User Presets。

Git 钩子启用、编辑器 Profile 准备及系统组件安装提供独立显式选项。MSVC/SDK 使用官方安装流程。会话启动入口和子进程环境提供选定 PATH 与环境变量。

准备过程可重复执行并复用匹配组件。临时目录保存下载与解压中间结果，校验通过后进入版本目录。各步骤记录输入、结果和产物，以支持恢复。

## 失败处理

| 情况 | 处理 |
| --- | --- |
| 下载中断 | 已完成步骤继续复用，重试当前获取操作 |
| 哈希偏差 | 隔离文件，该组件准备失败并报告来源 |
| 版本偏差 | 报告实际版本、要求版本及路径 |
| MSVC/SDK 缺失 | 报告必需组件和准备步骤 |
| Python 同步失败 | 保留日志，使用同一锁文件重试 |
| 构建配置变化 | 选择独立构建目录或明确重新配置 |
| 验收失败 | 保留诊断产物，验证状态记录失败 |

环境状态包括待准备、准备完成、验证通过和验证失败。准备完成表示组件已就位，验证通过表示指定验收实际执行成功。

## 版本记录

| 记录 | 职责 |
| --- | --- |
| tools/toolchain-lock.json | 平台、版本、发行包、URL、SHA-256 和组件身份 |
| cmake/Dependencies.cmake | 读取工具链清单中的 C++ 源码身份及哈希，声明构建选项 |
| .python-version | 解释器版本 |
| pyproject.toml 与 uv.lock | Python 声明及解析结果 |
| CMakePresets.json | 共享构建配置 |
| CMakeUserPresets.json | 本机解析路径及本地配置 |
| .local/environment.json | 实际路径、检测结果和基线摘要 |

各字段具有权威来源，解释器版本等重复字段接受一致性检查。正式准备前，必需哈希、解释器构建身份、SDK 修订信息和 VSIX 身份需要完整。

## 缓存策略

下载归档按 SHA-256 共享。uv 使用官方接口管理缓存，已安装解释器属于持久工具。FetchContent 解压和依赖构建保存在各构建树中，已校验归档可以共享。C++ 产物按平台、工具链、配置和依赖身份隔离。

Warp 使用 WARP_CACHE_PATH 或 kernel_cache_dir，指向项目中按配置划分的目录。Warp 自身增加版本子目录，普通和诊断内核配置保持可识别。

清理范围包括单个 Preset、项目 Warp 缓存、可回收 uv 缓存或指定哈希的归档。清理前展示解析目标并校验范围。源码和手写本地记录位于生成产物清理范围之外。

## 日常入口与网络边界

规划中的项目入口包括 doctor、prepare、configure <preset>、build <preset>、test <preset>、check、format、verify 和 clean <scope>。包装脚本解析选定工具，日志保留实际命令。VS Code 任务和 CI 使用相同入口。

网络获取集中在 prepare。日常构建、测试和检查使用已准备输入，缺失内容时报告准备步骤。离线模式先检查归档及包的完整性。CI 按恢复缓存、准备环境、验证、保存报告与产物的顺序执行。

工具链升级更新版本、来源及哈希，重建受影响依赖，复核编辑器和 CLI 路径，并执行构建、调试与数值回归。旧版本工具目录可以保留，用于复现旧提交。每个提交中的声明决定对应基线。

## 验收

| 层级 | 证据 |
| --- | --- |
| 环境身份 | 版本、路径、哈希、架构、SDK 和运行库符合声明 |
| 工具协同 | CMake/Ninja 构建、clangd 索引、格式化与分析协同工作 |
| 库使用 | 构建、测试及独立安装消费程序通过 |
| 调试与计算 | C++/Python 断点及调用栈有效，要求的 Warp CPU/GPU 冒烟检查通过 |
| 重建复现 | 新工作目录、干净构建及基于已准备输入的离线重建通过 |

报告同时提供机器可读记录和阅读摘要，包含提交身份、基线摘要、工具路径、命令、结果及耗时。CPU/GPU 验收配置明确必需检查，并记录省略的可选检查。

初期复现约定覆盖工具与依赖身份、可重复构建步骤和规定测试结果。二进制逐字节一致需要进一步处理路径、时间戳、调试信息和归档顺序，作为后续目标。

ENV-001 记录选定身份，ENV-002 实现[只读诊断](../environment/doctor.zh-CN.md)，验证范围在对应说明中记录。环境准备、机器配置及验收执行按后续阶段推进。

## 参考资料

- [工具链与依赖](0001-development-toolchain-and-dependencies.zh-CN.md)
- [uv 存储](https://docs.astral.sh/uv/reference/storage/)
- [uv 缓存](https://docs.astral.sh/uv/concepts/cache/)
- [CMake FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html)
- [Warp 缓存配置](https://github.com/NVIDIA/warp/blob/v1.18.0/warp/config.py)
