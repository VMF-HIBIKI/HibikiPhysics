# ADR-0008：Linux 与 macOS 工具链版本

[English](0008-linux-and-macos-toolchains.md) | **简体中文**

状态：Accepted（已接受）。

日期：2026-10-31。

验证状态：版本选择已接受，发行身份、包解析与构建兼容性待验证。

## 背景与决策

[ADR-0007](0007-platform-and-ci-validation.zh-CN.md)将 Linux x64 和 macOS arm64 列为首期验证平台。各平台使用对应标准库，以下版本构成环境准备阶段的候选组合。

## Linux x64

| 组件 | 选定基线 |
| --- | --- |
| 系统系列 | Ubuntu 24.04 LTS，x64 |
| 主编译器 | Clang 22.1.6 |
| 主链接器 | LLD 22.1.6 |
| 第二编译器 | GCC 13.3.0 |
| GCC 13 与标准库开发包 | 13.3.0-6ubuntu2~24.04.1 |
| libstdc++6 运行库包 | 14.2.0-4ubuntu2~24.04.1 |
| glibc | 2.39-0ubuntu8.9 |
| CMake | 4.4.4 |
| Ninja | 1.13.2 |

标准库头文件和实际加载的 libstdc++ 动态库分别记录。在发行版依赖模型中，两者的包版本可以不同。准备阶段验证选定组合的完整依赖解析、来源可获取性和哈希。

Linux 完整环境身份包括构建容器 digest、系统包清单及 LLVM 发行包哈希。基础镜像身份和包获取输入予以保留，支持后续重建。宿主内核与容器用户态包分别记录。

Clang 显式选择预期的 GCC 开发安装。GCC 验证使用独立构建目录，其链接器与 binutils 修订版本仍需补入包清单。经过验证的包集合与最终镜像 digest 共同完成锁定。

## macOS arm64

| 组件 | 选定基线 |
| --- | --- |
| 验证系统 | macOS 15，Apple Silicon |
| Xcode | 26.3 |
| Xcode 构建号 | 17C529 |
| 编译器 | 选定 Xcode 附带的 Apple Clang |
| 标准库头文件 | 选定 Xcode 工具链附带的 libc++ |
| SDK | macOS 26.2 SDK |
| 链接器 | 选定 Xcode 附带的 Apple linker |
| 初期部署目标 | macOS 15.0 |
| CMake | 4.4.4 |
| Ninja | 1.13.2 |

Xcode 26.3 要求受支持的宿主系统从 macOS 15.6 起。部署目标描述预期应用运行下限，SDK 选择构建时可用的 API。较新 macOS 15 宿主上的验证提供对应已测试版本的证据，最低运行版本验证单独列为验收项。

环境显式选择 Xcode 路径。准备阶段采集编译器和链接器完整版本字符串、SDK 构建身份及操作系统构建号。libc++ 头文件来自选定工具链，系统运行库随实际操作系统提供，两类身份均保留记录。

macOS 完整身份包含 Xcode 版本及构建号、SDK 身份、OS 构建号和部署目标。每次托管运行记录镜像身份，环境检查报告工具可用性及镜像变化。

## 共享工具与配置

CMake、Ninja、uv、Python、Ruff、basedpyright 和依赖源码版本沿用已接受的共享基线。各平台使用对应架构发行包并记录哈希。C++ 格式化在各平台统一使用 clang-format 22.1.6。macOS 编译采用选定 Apple Clang，语言服务与分析工具通过其编译数据库验证兼容性。

构建与数值行为遵循 [ADR-0004](0004-build-and-numerical-policy.zh-CN.md)，Sanitizer 覆盖遵循 [ADR-0005](0005-sanitizers-memory-and-runtime-diagnostics.zh-CN.md)，初始化与重建遵循 [ADR-0006](0006-environment-initialization-and-reproduction.zh-CN.md)。

## 影响与验证

各平台标准库实现保持版本化管理。Linux 包修订和 macOS 工具链包具有明确来源记录。版本选择在以下证据齐备后成为已验证基线：

- [ ] Linux 包文件、依赖解析及哈希通过验证。
- [ ] Linux 基础及最终容器 digest 与完整包清单已记录。
- [ ] Clang 使用选定头文件、运行库与链接器。
- [ ] GCC 及其链接器/binutils 身份已记录并独立测试。
- [ ] macOS 主机上的 Xcode 构建号、编译器、链接器、SDK 和 OS 身份通过验证。
- [ ] 部署设置和最低运行版本验证覆盖已说明。
- [ ] 两个平台的构建、测试和安装消费程序通过。
- [ ] 适用的 Sanitizer 与工具集成检查通过。

安装、镜像准备及环境配置仍待执行。由可获取性或兼容性发现引起的版本调整，作为明确的基线修订记录。

## 参考资料

- [Ubuntu GCC 13](https://packages.ubuntu.com/noble/gcc-13)
- [Ubuntu libstdc++ 开发包](https://packages.ubuntu.com/noble-updates/libstdc%2B%2B-13-dev)
- [Ubuntu libstdc++ 运行库](https://packages.ubuntu.com/noble-updates/libstdc%2B%2B6)
- [Ubuntu glibc](https://packages.ubuntu.com/noble-updates/libc6)
- [Apple Xcode 系统要求](https://developer.apple.com/xcode/system-requirements)
- [macOS 15 arm64 运行器清单](https://github.com/actions/runner-images/blob/main/images/macos/macos-15-arm64-Readme.md)
