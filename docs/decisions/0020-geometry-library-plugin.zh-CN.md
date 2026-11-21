# ADR-0020：几何库插件

[English](0020-geometry-library-plugin.md) | **简体中文**

状态：已接受

日期：2026-11-21

相关工作见 [Issue #17](https://github.com/VMF-HIBIKI/HibikiPhysics/issues/17) 和 [ADR-0010](0010-plugin-architecture-and-lifecycle.zh-CN.md)。

## 背景

HibikiPhysics 需要让研究插件共用几何定义。Jolt 提供复合形状与变截面几何，MuJoCo 定义模型使用的 geom 类型，MotrixSim 区分源几何与生成的碰撞体，Isaac Sim 允许同一网格选择多种碰撞表示。项目据此分别记录几何表达、格式导入和碰撞支持范围。[Jolt 形状类型](https://github.com/jrouwe/JoltPhysics/blob/master/Jolt/Physics/Collision/Shape/Shape.h) · [MuJoCo 几何](https://mujoco.readthedocs.io/en/stable/XMLreference.html#body-geom) · [MotrixSim 几何 API](https://motrixsim.readthedocs.io/en/v0.10.0/api_reference/core/motrixsim.html) · [Isaac Sim 碰撞表示](https://docs.isaacsim.omniverse.nvidia.com/latest/physics/simulation_fundamentals.html)

## 关键选择

一个官方几何库插件可以提供多种几何。插件负责形状定义、局部坐标和尺寸约定。格式适配器负责源数据转换。碰撞插件负责形状配对与接触算法。插件核心沿用 ADR-0010 的能力装配机制。

下表是已接受的候选清单。[几何文档](../geometry/)按已实现的类别记录形状参数、查询语义与验证方式。

| 类别 | 候选几何 |
| --- | --- |
| 基础解析几何 | 平面、闭半空间、有限矩形面片、球、盒、胶囊、圆柱、椭球 |
| 扩展解析几何 | 圆锥、圆台、两端半径不同的胶囊 |
| 离散与凸几何 | 三角形、凸包、三角网格 |
| 地形与场 | 高度场、有符号距离场（SDF） |
| 实例与组合 | 带局部位姿的几何实例、复合几何、各形状的缩放规则 |

## 局部解析几何

八种基础形状统一使用米和形状局部坐标，位姿由后续实例承载。首版 SDF 资产使用 CPU 从封闭三角网格烘焙局部稠密距离场。稀疏存储和设备后端留待研究。[基础几何](https://github.com/VMF-HIBIKI/HibikiPhysics/issues/19) · [CPU SDF 预处理](../sdf/serial-baking.zh-CN.md)。

| 形状 | 定义与构造尺寸 |
| --- | --- |
| `Plane` | 过原点的无限 XY 有向平面，单位法向为 +Z。有符号距离等于查询点的 Z 坐标。 |
| `HalfSpace` | 以该平面为边界的闭区域。Z ≤ 0 的点均属于半空间。 |
| `RectangularPatch` | 中心位于原点的零厚度闭矩形，位于 XY 平面。两个半尺寸确定四条边。最近点将 X、Y 分别限制在对应区间内，并取 Z = 0。 |
| `Sphere` | 球心位于原点，尺寸取半径。 |
| `Box` | 中心位于原点，沿局部 X、Y、Z 存三个半尺寸。半尺寸 (1, 0.5, 0.2) 对应完整边长 (2, 1, 0.4)。 |
| `Capsule` | 参数为半径 r 和圆柱段半长 h，两个端帽球心位于 z = ±h，总长为 2(h + r)。h = 0 时胶囊占据半径为 r 的球形区域。 |
| `Cylinder` | 参数为半径和半高 h，两端面位于 z = ±h。 |
| `Ellipsoid` | 三个半轴长度按局部 X、Y、Z 排列。 |

尺寸取有限正值，胶囊的圆柱段半长还允许为零。构造和点查询通过 `std::invalid_argument` 报告无效输入。胶囊轴向半尺寸超过可表示范围时，查询抛出 `std::overflow_error`。接口采用双精度坐标。半径 0.2 米、圆柱段半长 0.5 米的胶囊，其中心线段长 1 米，端到端总长 1.4 米。

## 注册与验证

`Contains(shape, worldFromLocal, worldPoint)` 以 `Vec3` 接收单位为米的世界点，适用于球、盒、胶囊、圆柱、椭球和闭半空间。查询先减去形状原点，再施加逆旋转，沿用局部查询的零接触裕量。

`WorldBounds(shape, worldFromLocal)` 将每根世界轴换到局部坐标系，取得形状沿该方向的投影区间，再加上该轴的平移分量。八种形状各自提供投影。球在任意朝向下沿各轴的半宽都等于半径。盒子投影三根局部半轴。投影方向在局部 X、Y 上的计算分量均恰好为零时，平面沿该世界轴只有一个坐标，半空间则由局部 Z 分量的符号决定哪一侧有界。任一切向分量非零时，两种形状沿该世界轴的投影都向两侧无界。坐标换算或有限边界的计算溢出时抛出 `std::overflow_error`。

`GeometryLibrary::Contains` 对五种闭实体和闭半空间执行局部点包含判断。边界计入内部，双精度比较采用零接触裕量。半径为 0.2 米、轴向半长为 0.5 米时，胶囊包含点 (0, 0, 0.6)，圆柱的上端面位于 z = 0.5 米。中心线段长为零的胶囊采用与球相同的包含判据。

`LocalBounds` 为八种形状分别返回局部轴对齐包围盒 `AABB`，最小和最大端点定义闭区间。无限平面的 X、Y 覆盖整个实轴，Z 固定为零。闭半空间的 X、Y 同样无界，Z 从负无穷延伸到零。有限面片在 XY 上有界，厚度为零。无界区间端点以 IEEE 无穷表示。胶囊的轴向包围边界按需向外舍入，有限边界超出可表示范围时抛出 `std::overflow_error`。`AABB::Contains` 判断点是否在包围盒内，形状对应的重载判断点是否在实际实体内。

应用链接 `hibiki::physics`，包含 `<HibikiPhysics/Geometry.h>`，通过 `CapabilityConfig::Add` 注册 `GeometryLibraryPlugin()`。消费方通过 `GeometryLibrarySlot()` 声明依赖。官方工厂为每个请求该能力的作用域创建一个 `GeometryLibrary` 实例，提供平面有符号距离、半空间包含判断和有限面片最近点查询。对于半尺寸 (1, 0.5) 的面片，点 (2, 0, 0) 位于无限平面上，面片上的最近点为 (1, 0, 0)。[安装包示例](../../examples/install-consumer/AnalyticGeometry.cpp) · [几何测试](../../tests/install-consumer/AnalyticShapes.cpp)。

安装包测试验证显式注册、选定对象的注入、各作用域的独立工厂实例，以及当前配置中的实验替换。[几何插件测试](../../tests/install-consumer/GeometryPlugin.cpp)
