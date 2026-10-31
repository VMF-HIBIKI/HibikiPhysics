# ADR-0007：跨平台与 CI 验证

[English](0007-platform-and-ci-validation.md) | **简体中文**

状态：Accepted（已接受）。

日期：2026-10-31。

验证状态：平台镜像、Linux/macOS 精确工具链及工作流待准备与验证。

## 背景与决策

首期平台范围为 Windows x64、Linux x64 和 macOS arm64。Windows 是主开发环境，Linux 提供跨平台和 Sanitizer 覆盖，macOS 提供 Apple Silicon 源码、安装及基本运行验证。

评估安排包括每个 PR 执行全部编译器与诊断配置，以及将主平台检查和较重的里程碑检查分开。选定方案在相关 PR 中运行三个主构建，第二编译器和重型检查由主动验证及里程碑验收执行。

## 平台职责

| 平台 | 工具链方向 | 职责 |
| --- | --- | --- |
| Windows x64 | 已接受的 clang-cl 基线、MSVC STL 和 Windows SDK | 完整开发验证与 ASan |
| Linux x64 | Ubuntu 24.04、Clang 22.1.6 和 libstdc++ | 跨平台测试、ASan/UBSan 和泄漏检查 |
| macOS arm64 | macOS 15、Apple Clang 和 libc++ | 构建、安装与基本运行验证 |
| Windows 第二编译器 | 选定工具集中的 MSVC | 编译器兼容性 |
| Linux 第二编译器 | 单独固定版本的 GCC | 里程碑编译器兼容性 |

[ADR-0008](0008-linux-and-macos-toolchains.zh-CN.md)记录已接受的 Linux 包及 macOS Xcode/SDK 选择，完整发行身份与兼容性验证仍待完成。本记录定义平台范围和验证职责。

## PR 检查

| 检查 | 触发范围 |
| --- | --- |
| 标题与 Issue 对应 | PR 创建、相关元数据变化及验证 |
| 双语文档、链接和配置语法 | 文档与配置变更 |
| Ruff 和 basedpyright | Python 及相关配置变更 |
| Windows clang-cl 构建与测试 | C++、依赖和构建变更 |
| Linux Clang 构建与测试 | C++、依赖和构建变更 |
| macOS 构建与基本测试 | C++、依赖和构建变更 |
| Sanitizer | 内存及算法变更；初期保守触发可以覆盖全部 C++ 变更 |

公共头文件、依赖和构建规则变化触发完整平台矩阵。稳定的汇总检查评估必需任务，对省略项记录明确原因。适用工作全部成功后，必需检查通过。

## 完整与里程碑验证

第二编译器构建、完整物理回归、长时间稳定性测试、安装消费程序、干净重建和离线重建归入完整或里程碑验证。TSan、GPU 诊断和基准测试随功能及资源要求安排。

正式性能对照使用固定硬件，记录负载和环境。托管运行器计时描述 CI 成本与吞吐。GPU 验证初期在 RTX 5070 Ti 上受控执行指定提交，并生成可复现报告。后续自动 GPU 执行单独定义环境及外部贡献代码的运行边界。

## CI 环境身份

GitHub 托管运行器标签用于选择平台。每次运行记录实际镜像身份，并显式选择项目工具。候选标签为 ubuntu-24.04、windows-2025-vs2026 和 arm64 的 macos-15，实施时核对可用性。

Linux 使用固定 digest 的构建容器控制用户态依赖，报告另行记录宿主内核。Windows 检查 MSVC 和 SDK 精确身份，组件缺失报告为环境失败。macOS 显式选择 Xcode 路径，记录 Xcode、SDK 和操作系统构建身份。外部 GitHub Actions 固定到完整提交 SHA。

CI 使用 [ADR-0006](0006-environment-initialization-and-reproduction.zh-CN.md)规定的项目入口。缓存身份包含相关平台、工具链、配置和依赖输入。产物保留环境报告、日志及诊断证据。

## 验证与实施边界

- [ ] Linux 和 macOS 精确工具链及包身份已选定并记录。
- [ ] 三个主平台构建及安装消费检查通过。
- [ ] 变更分类和汇总检查正确处理适用与省略任务。
- [ ] 标题与实现 Issue 对应检查遵循 ADR-0003。
- [ ] 第二编译器及诊断运行产生可识别报告。
- [ ] 镜像漂移和固定工具缺失提供明确诊断。
- [ ] 本地 GPU 报告标明测试提交与配置。

本决策确认验证范围和安排。远程仓库创建、工作流启用、工具安装及环境配置属于后续工作。

## 参考资料

- [GitHub 托管运行器](https://docs.github.com/en/actions/reference/runners/github-hosted-runners)
- [运行器镜像](https://github.com/actions/runner-images)
- [工具链基线](0001-development-toolchain-and-dependencies.zh-CN.md)
- [提交检查](0003-commit-hooks-and-validation.zh-CN.md)
- [Sanitizer 策略](0005-sanitizers-memory-and-runtime-diagnostics.zh-CN.md)
