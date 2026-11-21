# 圆锥、圆台与变半径胶囊

[English](extended-shapes.md) | **简体中文**

包含 `HibikiPhysics/Geometry.h`，通过 `GeometryLibraryPlugin()` 提供的 `GeometryLibrary` 查询形状。长度单位为米，对称轴为局部 Z 轴。三种实体均包含边界。

| 构造函数 | 几何定义 |
| --- | --- |
| `Cone(radius, halfHeight)` | 底面位于 −halfHeight，顶点位于 +halfHeight。半径和半高均须为正。 |
| `Frustum(bottomRadius, topRadius, halfHeight)` | 两端圆盘分别位于 −halfHeight 和 +halfHeight，截面半径随 Z 线性变化。半高须为正。允许一端半径为零，另一端须为正。两端等半径时得到圆柱。 |
| `TaperedCapsule(bottomRadius, topRadius, halfSpineLength)` | 两个闭球的凸包。球心分别位于 −halfSpineLength 和 +halfSpineLength，两端球半径均须为正。半轴段长度允许为零。 |

变半径胶囊的侧面与两个端球相切。两端等半径时得到普通胶囊。当一个端球包含另一个端球时（包括球心重合），凸包即为较大的球。

`Contains(shape, localPoint)` 检查局部点。`Contains(shape, worldFromLocal, worldPoint)` 先将世界点逆变换到局部坐标系。`LocalBounds(shape)` 和 `WorldBounds(shape, worldFromLocal)` 分别沿查询坐标系的三个轴投影端面圆盘或端球，求出轴对齐包围盒。两端半径不同时，包围盒可能不关于原点对称。

所有尺寸和查询坐标都须有限。参数无效时抛出 `std::invalid_argument`。包围盒或逆变换后的坐标超出有限范围时抛出 `std::overflow_error`。
