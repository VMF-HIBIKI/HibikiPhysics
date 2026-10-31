# ADR-0004：构建与数值策略

[English](0004-build-and-numerical-policy.md) | **简体中文**

状态：Accepted（已接受）。

日期：2026-10-31。

验证状态：Windows 工具链数值用例通过，引擎专项检查随实现推进。
当前 Windows 证据见[验收记录](../environment/acceptance.zh-CN.md)。

## 背景

[ADR-0001](0001-development-toolchain-and-dependencies.zh-CN.md)定义工具链。引擎需要明确的优化、浮点和指令集策略，使正确性与性能测量具有可识别的运行条件。

## 候选方案与决策

评估方案包括随构建类型改变浮点行为、默认启用宿主特定指令，以及采用明确的通用基线并单独开展优化实验。选定方案在各构建类型中保持一致的基础浮点语义。FMA、AVX2 和 LTO 实验使用独立标识的配置与测量记录。

## 构建配置

| 配置 | Windows clang-cl 优化 | 调试符号 | 引擎内部断言 | 用途 |
| --- | --- | --- | --- | --- |
| Debug | /Od | 开启 | 开启 | 单步调试与局部正确性检查 |
| RelWithDebInfo | /O2 | 开启 | 开启 | 日常运行与优化后调试 |
| Release | /O2 | 开启并保存 | 关闭 | 性能测量与发布验证 |

Release 符号可以作为独立产物保存，并与可执行文件关联。初始基线保持 LTO 关闭。后续 LTO 配置记录链接耗时、产物大小、运行性能和数值回归结果。

这些优化参数适用于 Windows 基线。Linux 和 macOS 的映射随对应工具链确定。项目断言采用独立开关，与第三方 NDEBUG 行为分别管理，各配置保持公共数据布局兼容。

## 基础浮点策略

三种基础配置均保留 NaN、Inf 和带符号零的语义，保持浮点表达式运算次序约束，并采用常规除法与平方根精度要求。fast-math 和自动乘加融合关闭。

实施时显式设置并验证实际生效的 clang-cl 选项及其顺序。精确浮点模式与融合设置分别控制。Release 优化遵循相同的数值策略。

## 融合乘加

FMA 在乘加中执行一次舍入，分开的乘法和加法通过两次舍入可能产生不同结果。基础配置关闭自动融合，建立明确的对照条件。

显式 std::fma 调用和 FMA intrinsic 属于算法行为约定，配套专项测试。后续 FMA 优化配置比较运行耗时、残差与稳定性。Warp 对照单独记录其融合设置。

跨平台逐位一致还需要控制算法顺序、并行归约、数学函数和运行状态。本决策定义数值设置与对照条件，复现保证通过独立验证证据明确。

## CPU 指令集

Windows/Linux x64 基线采用 SSE2。后续 AVX2 配置明确标识额外 CPU 要求。ARM64 要求随对应平台基线确定。

发布产物和可比较基准使用显式指令集设置。native 构建用于本机调查，记录 CPU 及实际编译参数。标量源码可以在选定目标指令集范围内被自动向量化。

## 断言与错误处理

| 检查 | Debug | RelWithDebInfo | Release |
| --- | --- | --- | --- |
| 内部不变量 | 开启 | 开启 | 关闭 |
| 公共 API 输入验证 | 保留 | 保留 | 保留 |
| 昂贵的完整状态检查 | 按需开启 | 按需开启 | 专项验证配置 |
| 浮点异常捕获 | 专项诊断配置 | 专项诊断配置 | 常规运行模式 |

非法句柄、无效参数和资源失败采用明确的错误处理。断言表达式保持无副作用，使必要操作在不同配置中一致执行。具体错误报告 API 由后续接口决策确定。

浮点异常捕获需要编译器和平台验证，与常规数值基线分别配置。

## 线程浮点状态

已接受基线的实施方向为舍入到最近值、中点取偶及渐进下溢。x86 平台包含 FTZ/DAZ 状态检查。相关引擎入口与工作线程初始化验证所需浮点状态，作用域内的临时调整在结束后恢复调用方状态。

平台层设计明确检查与恢复的具体边界。CPU 与 GPU 的运行状态行为分别验证。

[ADR-0005](0005-sanitizers-memory-and-runtime-diagnostics.zh-CN.md)规定插桩构建、分配器检查及 GPU 诊断流程。

## 验证

- [x] 实际编译与链接命令符合选定配置。
- [ ] 选定工具链支持符号、单步和优化后崩溃诊断。
- [x] 数值用例覆盖 NaN、Inf、带符号零、下溢和乘加舍入。
- [ ] 代表性生成代码验证自动融合设置。
- [ ] 基础产物符合声明的指令集要求。
- [ ] 内部断言符合各配置约定，公共输入验证保持启用。
- [ ] 调用方和工作线程浮点状态遵循检查与恢复约定。
- [ ] Debug 与优化配置均满足场景数值验收标准。
- [ ] 基准报告记录优化、浮点、指令集、断言和运行状态设置。

Windows 环境基线通过构建兼容性及工具链数值检查，引擎数值行为随实现验证。

## 参考资料

- [Jolt 构建配置](https://github.com/jrouwe/JoltPhysics/blob/master/Build/CMakeLists.txt)
- [Jolt 目标配置](https://github.com/jrouwe/JoltPhysics/blob/master/Jolt/Jolt.cmake)
- [Genesis 初始化](https://github.com/Genesis-Embodied-AI/genesis-world/blob/main/genesis/__init__.py)
- [Quadrants 宿主编译配置](https://github.com/Genesis-Embodied-AI/quadrants/blob/main/cmake/QuadrantsCXXFlags.cmake)
- [Clang 浮点行为](https://clang.llvm.org/docs/UsersManual.html#controlling-floating-point-behavior)
