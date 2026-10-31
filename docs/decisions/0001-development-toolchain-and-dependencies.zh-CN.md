# ADR-0001：开发工具链与依赖

[English](0001-development-toolchain-and-dependencies.md) | **简体中文**

状态：Accepted（已接受）。

日期：2026-10-31。

验证状态：Windows 工具链及 CPU／GPU 运行检查通过。
当前 Windows 证据见[验收记录](../environment/acceptance.zh-CN.md)。

关联工作：HP-003、HP-004、HP-011、HP-012。

## 背景

HibikiPhysics 面向 C++20 系统开发与物理算法实现。开发环境需要可复现的工具选择、明确的依赖版本、有效的编辑器诊断，以及 Windows、Linux、macOS 源码兼容性。

初始开发环境是 Windows x64。各平台标准库属于对应平台的版本化工具链。Windows 使用 MSVC STL，Linux 和 macOS 的工具链基线后续单独确定。

## 候选方案

| 范围 | 候选方案 | 选定方向 |
| --- | --- | --- |
| Windows 编译器 | MSVC、clang-cl | clang-cl 主编译，MSVC 兼容性检查 |
| 标准库 | 平台标准库、三平台自建 libc++ | 各平台标准库及明确版本记录 |
| C++ 依赖获取 | Conan 2、vcpkg Manifest、CMake FetchContent | 当前小规模依赖采用 FetchContent |
| 数学 | Eigen 核心、自有小型类型与 Eigen 对照 | 自有小型类型与 Eigen 验证 |
| C++ 测试 | doctest、GoogleTest | GoogleTest 配合 CTest |
| 日志 | 应用回调、直接接入日志库 | 核心回调，工具使用 spdlog |
| 性能测量 | 库级微基准、应用场景 | Google Benchmark 配合场景运行器 |

FetchContent 使依赖在项目配置的环境中构建，依赖选项和源码身份均明确记录。未来依赖复杂度增加时，通过独立决策评估包管理方案。

## Windows 工具链基线

以下版本作为已接受的实施候选。构建和运行检查通过后形成已验证基线。

| 组件 | 版本或配置 |
| --- | --- |
| 目标 | Windows x64 |
| 语言 | C++20 |
| clang-cl | 22.1.6 |
| clangd | 22.1.6 |
| clang-format | 22.1.6 |
| clang-tidy | 22.1.6 |
| 链接器 | lld-link 22.1.6 |
| MSVC 工具集 | 14.51.36231 |
| C++ 标准库 | 工具集 14.51.36231 附带的 MSVC STL |
| Windows SDK 选择 | 10.0.26100.0 |
| CMake | 4.4.4，上游发行版 |
| Ninja | 1.13.2，上游发行版 |
| 生成器 | Ninja，单配置 |
| Release / RelWithDebInfo CRT | 动态，/MD |
| Debug CRT | 动态，/MDd |
| 初始 LTO | 关闭 |
| 初始异常与 RTTI | 开启 |

ENV-001 已记录 SDK DisplayVersion 10.1.26100.8249、目录包版本 10.0.26100.16，以及 MSVC 编译器文件版本 19.51.36248.0。工具集目录、安装包修订和关键文件哈希分别保存在[Windows 身份记录](../environment/windows-identities.zh-CN.md)及其机器可读清单中。

每种构建配置使用独立目录。CMake 为当前配置生成编译数据库。clangd 使用选定的开发数据库，clang-tidy 使用对应检查配置的数据库。

工具链选择解析明确的可执行文件、头文件与库路径。机器专属绝对路径保存在本地配置中。后续环境检查脚本比较实际工具与基线，在进入配置前报告版本偏差。

[ADR-0004](0004-build-and-numerical-policy.zh-CN.md)定义优化等级、浮点行为、指令集要求和断言策略。

## Python 与 Warp 基线

| 组件 | 版本或配置 |
| --- | --- |
| uv | 0.13.6 |
| 解释器 | CPython 3.12.16，常规 GIL 构建，Windows x64 |
| 初期 Python 范围 | >=3.12,<3.13 |
| Warp | warp-lang 1.18.0，官方 Windows x64 wheel |
| Python 测试 | pytest 9.2.0 |

uv 管理项目解释器和虚拟环境。解释器发行标识、URL 和校验和补充 Python 版本信息。项目使用 Python 版本文件、依赖声明及提交到仓库的 uv 锁文件。环境同步和命令执行采用锁定模式。

Warp 环境记录选定 wheel、GPU 和实际驱动版本。已盘点的机器使用 RTX 5070 Ti 与 610.88 驱动。发布元数据及对应版本安装文档提供兼容性要求，CPU／CUDA／Graph 本地用例已通过。

Warp 源码构建需要单独确定 CUDA Toolkit 与宿主编译器兼容方案。当前起点采用已发布 wheel 及其附带的 CUDA 组件。

## 依赖基线与边界

| 依赖 | 版本 | 范围 | 许可证记录 |
| --- | --- | --- | --- |
| Eigen | 5.0.1 | 初期用于数学参考测试 | 主要为 MPL-2.0，同时保留随附第三方声明 |
| GoogleTest | 1.18.0 | C++ 测试程序 | BSD-3-Clause |
| spdlog | 1.17.0 | 工具程序及日志适配器 | MIT |
| 随附 fmt | spdlog 内的 12.1.0 | spdlog 格式化 | MIT |
| Google Benchmark | 1.10.0 | 微基准程序 | Apache-2.0 |
| pytest | 9.2.0 | Python 与 Warp 测试 | MIT |

核心数学类型初期覆盖 Vec3、Mat3、Quat 和 Transform。标量实现建立行为和布局基线，Eigen 提供独立参考计算。更复杂的数值依赖与 SIMD 工作依据具体需求和测量推进。

核心暴露项目自有日志回调，工具将回调接入 spdlog。Python 使用标准 logging。场景基准同时报告数值质量、耗时和工作量统计。

### 依赖构建配置

| 依赖 | 初始配置 |
| --- | --- |
| Eigen | 头文件参考计算；上游测试、演示、文档、BLAS 和 LAPACK 构建关闭 |
| GoogleTest | GoogleTest 开启；GoogleMock、Abseil/RE2 集成和安装关闭；Windows CRT 动态链接 |
| spdlog | 静态库、随附 fmt、动态 Windows CRT；上游测试、示例、基准和安装关闭 |
| Google Benchmark | 库开启；上游测试、依赖下载、Python 绑定、libpfm 和安装关闭 |
| pytest | 基础包；插件通过明确的依赖变更加入 |

选项作用于对应目标或依赖范围。项目测试与依赖自身测试分别控制，引擎目标显式要求 C++20。第三方诊断设置与项目警告策略分别维护。

## 锁定与来源记录

规划中的记录各自承担以下职责：

| 记录 | 职责 |
| --- | --- |
| tools/toolchain-lock.json | 工具版本、发行标识、URL、哈希、架构及运行库配置 |
| cmake/Dependencies.cmake | 读取工具链清单中的固定 C++ 源码身份与哈希，声明构建选项 |
| .python-version | 项目解释器版本 |
| pyproject.toml | Python 范围、直接依赖版本和工具配置 |
| uv.lock | Python 依赖解析结果、来源及发行文件哈希 |
| third_party/licenses/ | 选定源码发行包的许可证与来源声明 |
| 验证报告 | 实际工具路径、版本、构建结果、GPU 和驱动信息 |

源码归档使用 FetchContent URL_HASH 校验。版本标签便于审查，解析后的提交与归档哈希标识选定内容。初始配置从声明的位置获取源码。依赖开发使用的源码覆盖显式设置，并记录在验证输出中。

源码缓存以内容身份区分，构建目录和编译缓存按平台、工具链及配置隔离。首次校验下载后，源码缓存可以用于离线复用。

依赖升级在同一次可审查变更中更新源码身份、选项、许可证记录和验证结果。标准库与编译器升级触发相关 C++ 依赖重建。

## 验证与剩余工作

当前证据包括本地版本盘点和上游发布、配置审查。本 ADR 的接受状态表示方向与版本选择已经确认。

- [x] 发行 URL、源码提交和归档哈希已获取并核验。
- [x] Python 发行身份和 Windows SDK 服务修订信息已记录。
- [x] 干净的 Debug/Release 构建使用选定编译器、链接器、STL 和 CRT。
- [x] 消费程序可以链接已安装的库目标。
- [x] clangd 与 clang-tidy 通过编译数据库解析项目头文件。
- [x] 调试符号、断点、调用栈和变量观察正常。
- [x] GoogleTest 通过 CTest 在选定运行库配置下执行。
- [x] Eigen 参考计算及 spdlog 终端、文件输出正常。
- [x] Google Benchmark 输出机器可读报告。
- [x] 锁定 Python 环境运行 pytest 及最小 Warp CPU/GPU 内核。
- [x] 二进制依赖检查确认预期运行库链接。
- [x] 源码缓存复用和干净环境重建通过验证。

ENV-001 记录已核验的 C++ 源码身份及随附声明。Python 锁文件生成和安装按实施计划推进。[ADR-0008](0008-linux-and-macos-toolchains.zh-CN.md)定义 Linux/macOS 基线。

[ADR-0002](0002-editor-debugging-and-code-quality.zh-CN.md)记录已接受的 VS Code、调试和质量工具配置，项目集成验证已通过。

## 参考资料

- [CMake 4.4.4](https://github.com/Kitware/CMake/releases/tag/v4.4.4)
- [Ninja 1.13.2](https://github.com/ninja-build/ninja/releases/tag/v1.13.2)
- [uv 0.13.6](https://github.com/astral-sh/uv/releases/tag/0.13.6)
- [Warp 1.18.0 安装说明](https://github.com/NVIDIA/warp/blob/v1.18.0/docs/user_guide/installation.rst)
- [Eigen 5.0.1 构建配置](https://gitlab.com/libeigen/eigen/-/blob/5.0.1/CMakeLists.txt)
- [Eigen 许可信息](https://gitlab.com/libeigen/eigen/-/blob/5.0.1/COPYING.README)
- [GoogleTest 1.18.0](https://github.com/google/googletest/releases/tag/v1.18.0)
- [spdlog 1.17.0](https://github.com/gabime/spdlog/releases/tag/v1.17.0)
- [Google Benchmark 1.10.0](https://github.com/google/benchmark/releases/tag/v1.10.0)
- [pytest 9.2.0](https://pypi.org/project/pytest/9.2.0/)
