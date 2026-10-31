# 架构决策记录

[English](README.md) | **简体中文**

架构决策记录保留重要选择背后的依据。

记录以[模板](0000-template.zh-CN.md)为基础，英文文件采用 `NNNN-short-title.md`，中文文件采用 `NNNN-short-title.zh-CN.md`。编号从 `0001` 顺序递增。状态使用 Proposed（提议）、Accepted（已接受）和 Superseded（已替代）。替代记录链接原决策并说明新证据。

每条记录描述背景、候选方案、决策、影响和验证。相关 Issue 与 PR 提供实现追溯入口。

初始决策主题包括 C++/Warp 边界、所有权与句柄、数值约定及首个求解器方案。

## 记录索引

| 记录 | 决策状态 | 验证状态 |
| --- | --- | --- |
| [ADR-0001：开发工具链与依赖](0001-development-toolchain-and-dependencies.zh-CN.md) | 已接受 | Windows 工具链及 CPU／GPU 运行检查通过。 |
| [ADR-0002：编辑器、调试与代码质量](0002-editor-debugging-and-code-quality.zh-CN.md) | 已接受 | Windows 编辑器、CLI 和调试集成通过。 |
| [ADR-0003：提交钩子与验证流程](0003-commit-hooks-and-validation.zh-CN.md) | 已接受 | 本地钩子与检查通过，远程 PR CI 属于后续工作。 |
| [ADR-0004：构建与数值策略](0004-build-and-numerical-policy.zh-CN.md) | 已接受 | Windows 工具链数值用例通过，引擎专项检查随实现推进。 |
| [ADR-0005：Sanitizer、内存检查与运行时诊断](0005-sanitizers-memory-and-runtime-diagnostics.zh-CN.md) | 已接受 | Windows ASan 及 Warp CPU／CUDA／Graph 检查通过，附加平台诊断按对应里程碑推进。 |
| [ADR-0006：环境初始化与复现](0006-environment-initialization-and-reproduction.zh-CN.md) | 已接受 | Windows 环境准备、诊断和离线重建通过。 |
| [ADR-0007：跨平台与 CI 验证](0007-platform-and-ci-validation.zh-CN.md) | 已接受 | 平台准备与工作流待实施 |
| [ADR-0008：Linux 与 macOS 工具链版本](0008-linux-and-macos-toolchains.zh-CN.md) | 已接受 | 包身份与构建验证待完成 |
