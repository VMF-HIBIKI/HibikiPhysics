# ADR-0021：四元数与刚体位姿

[English](0021-quaternion-and-rigid-transform.md) | **简体中文**

状态：已接受

日期：2026-11-21

相关工作见 [Issue #21](https://github.com/VMF-HIBIKI/HibikiPhysics/issues/21)、[数学依赖选择](0001-development-toolchain-and-dependencies.zh-CN.md)和[局部几何](0020-geometry-library-plugin.zh-CN.md)。

## 背景

几何插件和后续动力学插件需要共用朝向与相对位姿的语义。Jolt 的分量构造函数与 MotrixSim 运行时位姿采用 XYZW，MuJoCo 模型采用 WXYZ。Isaac Sim 的 Core API 采用 WXYZ，底层 PhysX 采用 XYZW。转换入口的名称直接标明顺序，便于检查适配器的输入输出。[Jolt](https://github.com/jrouwe/JoltPhysics/blob/master/Jolt/Math/Quat.h) · [MotrixSim](https://motrixsim.readthedocs.io/en/latest/user_guide/kinematics/sensor/frame.html) · [MuJoCo](https://mujoco.readthedocs.io/en/stable/modeling.html#frame-orientations) · [Isaac Sim](https://docs.isaacsim.omniverse.nvidia.com/latest/reference_material/reference_conventions.html)

## 决策

公开头文件 `HibikiPhysics/Math.h` 提供双精度标量值类型。`Vec3` 默认为零向量，支持加减、标量乘法、点积、右手叉积和欧氏长度。Eigen 在测试目标中提供独立参考计算。

`Quat` 表示单位旋转。默认值为单位旋转，XYZW 分量为 (0, 0, 0, 1)。`FromXYZW`、`FromWXYZ`、`ToXYZW` 和 `ToWXYZ` 显式标明分量顺序。输入分量均须有限，且至少一个分量非零，构造时归一化。轴角构造采用有限非零轴、右手旋转和弧度。组合采用 Hamilton 积并归一化结果，求逆取共轭。四元数整体变号表示相同朝向。

`Transform` 从局部坐标系映射到父坐标系。位置 t 表示局部原点在父系中的有限坐标，单位为米。旋转 R 描述局部坐标轴在父系中的朝向。默认位姿为零平移和单位旋转。`TransformPoint` 计算 R p + t，`TransformDirection` 计算 R v。`Inverse` 返回反向映射，旋转为 R⁻¹，平移为 −R⁻¹ t。

组合时右操作数先作用。若 A 表示连杆到世界，B 表示盒子到连杆，A * B 就表示盒子到世界。组合旋转为 qA qB，平移为 tA + RA tB。子物体的偏移和局部坐标轴都会随父物体一起转动。

## 影响与验证

对于半尺寸 (1, 0.5, 0.2) 米的盒子，先绕 +Z 旋转 +π/2，再平移 (3, 2, 0) 米，局部 +X 面中心 (1, 0, 0) 到达 (3, 3, 0)，+X 方向变为 +Y。施加逆变换可还原局部面中心。若先做同一平移再旋转，该点到达 (−2, 4, 0)。

对照测试使用 Eigen 轴角和刚体变换，验证上述盒子，以及父子旋转次序会影响结果时的八个盒角。测试还覆盖缩放输入、四元数符号等价、连续组合和无效输入。独立安装包示例变换盒子的面中心。带位姿的形状查询作为后续几何任务。[参考测试](../../tests/math/RigidMathTests.cpp) · [盒子位姿示例](../../examples/install-consumer/BoxPose.cpp)
