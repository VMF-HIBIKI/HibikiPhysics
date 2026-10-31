# ADR-0005：Sanitizer、内存检查与运行时诊断

[English](0005-sanitizers-memory-and-runtime-diagnostics.md) | **简体中文**

状态：Accepted（已接受）。

日期：2026-10-31。

验证状态：Windows ASan 及 Warp CPU／CUDA／Graph 检查通过，附加平台诊断按对应里程碑推进。
当前 Windows 证据见[验收记录](../environment/acceptance.zh-CN.md)。

## 背景

引擎需要内存安全检查、数值故障定位和可复现的错误报告。[ADR-0001](0001-development-toolchain-and-dependencies.zh-CN.md)定义工具链，[ADR-0003](0003-commit-hooks-and-validation.zh-CN.md)定义验证分层，[ADR-0004](0004-build-and-numerical-policy.zh-CN.md)定义数值基线。

## 候选方案与决策

评估方案包括常规调试断言、跨平台统一诊断配置，以及平台专用插桩构建配合独立 GPU 诊断。选定方案以 Windows ASan 作为首个内存检查入口，由 Linux 补充未定义行为、泄漏和并发检查，Warp 使用独立诊断流程。

诊断配置与普通 Debug/Release 使用独立构建目录及报告，执行安排依据受影响变更和里程碑要求确定。

## C++ 检查矩阵

| 配置 | 平台安排 | 主要检查 | 执行范围 |
| --- | --- | --- | --- |
| ASan | Windows x64 优先，Linux 补充 | 越界、释放后使用、重复释放及支持的生命周期错误 | 内存与生命周期变更 |
| ASan + UBSan | Linux | 内存错误、对齐、有符号整数溢出、非法移位及其他已启用 UB 检查 | 核心算法变更和完整验证 |
| LeakSanitizer | Linux，结合 ASan | 内存泄漏 | 生命周期测试和里程碑验收 |
| TSan | Linux，独立配置 | CPU 数据竞争 | 并行功能引入后 |
| MemorySanitizer | 后续专项评估 | 未初始化内存读取 | 选定疑难问题调查 |

ThreadSanitizer 的平台支持和运行库可用性决定部署方式。MemorySanitizer 需要专门的插桩依赖环境。实施时核对支持平台列表和选定工具链。

## 插桩构建策略

- 引擎内部断言和调试符号开启。
- LTO 关闭，浮点与指令集基线沿用 ADR-0004。
- Windows ASan 初期采用 /Od 和动态 CRT /MD，通过最小程序验证运行库兼容性。
- Linux 诊断配置使用 -O1、调试符号和帧指针。
- ASan 与 TSan 分别构建和执行。
- 诊断报告错误时，对应测试进程以失败状态结束。

ASan 运行库及符号化工具跟随 LLVM 22.1.6，验证记录实际运行库路径。引擎目标、测试程序及适用的源码依赖采用对应插桩配置。报告注明插桩覆盖及外部组件。

Linux 承担规划中的泄漏验收，Windows ASan 覆盖以该配置验证的能力为准。UBSan 检查项和失败行为在配置阶段明确，额外数值检查分别说明其含义。

## 引擎内存诊断

| 能力 | 用途 |
| --- | --- |
| 活跃分配数和字节数 | 创建与销毁过程中的资源记账 |
| 峰值和每步分配统计 | 发现增长及分析热路径分配 |
| 调试分配器模式 | 子块边界保护 |
| 临时内存生命周期标记 | 检测跨仿真步持有临时数据 |
| 带代数的句柄检查 | 检测对已销毁对象的引用 |
| 重复创建与销毁测试 | 验证 World、场景和资源的完整生命周期 |

内存池使用较大的底层分配块，内部子块边界需要分配器专用检查。分配器设计阶段选择 ASan poisoning 或调试分配模式，明确对齐、标记和复用行为。资源记账区分活跃分配与有意保留的缓存。

## Warp 诊断模式

选定 Warp 版本为 1.18.0，诊断控制包括 debug 编译模式、行号信息、verify_fp 和 verify_cuda。各项能力通过对应发行版及代表性内核验证。

| 模式 | 行为 |
| --- | --- |
| 普通诊断 | 行号信息、设备/内核/启动维度记录，以及明确阶段边界的结果检查 |
| 故障定位 | 小规模场景、调试内核、适用的浮点检查和分阶段同步 |

verify_cuda 用于普通 launch 路径。该选项具有已记录的 capture 限制，CUDA Graph capture 使用单独复现的诊断路径。CPU/参考对照和显式状态检查补充 Warp 诊断。debugpy 按 ADR-0002 负责 Python 宿主执行。

## GPU 内存与同步检查

NVIDIA Compute Sanitizer 提供规划中的 GPU 检查：

| 工具 | 用途 |
| --- | --- |
| memcheck | 设备内存越界及未对齐访问 |
| initcheck | 选定地址空间的未初始化设备内存读取 |
| racecheck | 共享内存访问冲突 |
| synccheck | 同步原语使用问题 |

全局内存并发正确性同时通过算法级测试和审查验证。初始化检查结果的分析以 memcheck 通过为前提。

Compute Sanitizer 属于额外开发工具，其精确版本及与驱动、GPU、Warp 生成代码的兼容性随 GPU 诊断环境选定。Warp wheel 运行要求和诊断工具安装要求分别记录。诊断错误通过明确的退出码处理传递为测试失败。

## 可复现故障记录

故障产物包含以下内容：

- 提交 SHA、构建配置、工具链和 Sanitizer 版本。
- 场景设置、随机种子和仿真步号。
- 相关刚体、形状和约束的稳定标识。
- 当前仿真阶段、残差和首次检测到的异常数值。
- 调用栈、日志及相关复现输入。
- GPU 场景的设备、驱动、内核和同步位置。

阶段检查可以覆盖积分后的状态有限性、接触生成后的法线与深度约定，以及求解后的残差。诊断与性能运行分别输出报告，使插桩开销保持可识别。

## 实施顺序与验收

实施顺序为 Windows ASan、Linux ASan/UBSan 与泄漏检查、随并行功能引入的 TSan，以及随 Warp 工作引入的 Compute Sanitizer。每种配置先在隔离测试进程中运行受控错误程序，确认检测与失败传递，再执行正常对照和真实项目测试。

- [x] 选定插桩运行库可用且版本明确。
- [x] 受控内存错误产生可定位、带符号的报告和失败结果。
- [x] 正常对照用例通过各项已启用配置。
- [x] Windows /MD、clang-cl 与 lld-link 在 ASan 构建中配合正常。
- [ ] Linux UB 和泄漏检查覆盖代表性项目行为。
- [ ] 分配器子块及生命周期检查符合约定。
- [ ] 并发检查覆盖已引入的并行代码。
- [x] Warp launch 和 graph 诊断路径遵循支持的设置。
- [ ] GPU 检查失败传递至测试执行器。
- [x] 报告保存复现输入并标明插桩覆盖范围。

本决策记录已接受的诊断策略，Windows ASan 执行已通过，附加平台及引擎诊断按对应里程碑推进。

## 参考资料

- [Jolt 构建配置](https://github.com/jrouwe/JoltPhysics/blob/master/Build/CMakeLists.txt)
- [Quadrants ASan 配置](https://github.com/Genesis-Embodied-AI/quadrants/blob/main/cmake/QuadrantsCXXFlags.cmake)
- [Genesis 调试初始化](https://github.com/Genesis-Embodied-AI/genesis-world/blob/main/genesis/__init__.py)
- [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html)
- [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
- [LeakSanitizer](https://clang.llvm.org/docs/LeakSanitizer.html)
- [ThreadSanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html)
- [MemorySanitizer](https://clang.llvm.org/docs/MemorySanitizer.html)
- [Warp 1.18.0 诊断设置](https://github.com/NVIDIA/warp/blob/v1.18.0/warp/config.py)
- [NVIDIA Compute Sanitizer](https://docs.nvidia.com/compute-sanitizer/ComputeSanitizer/index.html)
