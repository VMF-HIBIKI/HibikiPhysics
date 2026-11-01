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
| [ADR-0009：默认刚体研究配置](0009-incremental-potential-rigid-contact.zh-CN.md) | 已接受 | 待实现与数值实验 |
| [ADR-0010：插件化架构与生命周期](0010-plugin-architecture-and-lifecycle.zh-CN.md) | 已接受 | 待架构原型与扩展验证 |
| [ADR-0011：多 World 批量仿真](0011-multi-world-batch-simulation.zh-CN.md) | 已接受 | 待实现、隔离与批量验证 |
| [ADR-0012：使用方式与运行模式](0012-usage-and-runtime-modes.zh-CN.md) | 已接受 | 待实现与工作流验收 |
| [ADR-0013：场景与资产](0013-scenes-and-assets.zh-CN.md) | 已接受 | 待实现与导入／扩展验收 |
| [ADR-0014：可视化与诊断](0014-visualization-and-diagnostics.zh-CN.md) | 已接受 | 待渲染接入与诊断验收 |
| [ADR-0015：官方研究实验与评价](0015-experiments-and-acceptance.zh-CN.md) | 已接受 | 待实验基础设施与证据 |
| [ADR-0016：时间推进](0016-time-advancement.zh-CN.md) | 已接受 | 待实现与时间／状态验证 |
| [ADR-0017：状态快照与恢复](0017-snapshots-and-restoration.zh-CN.md) | 已接受 | 待实现与恢复验证 |
| [ADR-0018：CPU 与 Warp 后端职责](0018-cpu-and-warp-backends.zh-CN.md) | 已接受 | 待实现与后端实验 |
| [ADR-0019：World 配置与运行输入](0019-world-configuration-and-runtime-inputs.zh-CN.md) | 已接受 | 待实现 |
