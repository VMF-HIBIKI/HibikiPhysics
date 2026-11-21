# 离散几何体

[English](discrete-shapes.md) | **简体中文**

`GeometryLibrary` 支持三种局部坐标系下的不可变几何体。构造后对象持有传入的坐标和索引，同一份几何数据可以配合多个刚体位姿查询。

| 几何体 | 构造输入 | 点归属语义 |
| --- | --- | --- |
| `Triangle(a, b, c)` | 三个互异且不共线的 `Vec3` 顶点 | 双面三角形表面，包含三条边和三个顶点 |
| `ConvexHull(points)` | 无序三维点云，至少四个点仿射无关 | 凸实体的内部和边界 |
| `TriangleMesh(vertices, faces)` | 共享顶点数组，以及一个或多个从零开始的索引三元组 | 各三角面及其边界的并集 |

凸包以四面体为起点，逐点扩展朝外的边界面。点云允许重复点及内部点，`GetVertices()` 保留原始点云。网格对绕序、连通性和水密性均无限制。即便网格构成闭合四面体，严格位于其内部的点也返回 false。未被面引用的顶点仍须有限，计算网格包围盒时跳过这些顶点。

```cpp
#include <HibikiPhysics/Geometry.h>

using namespace HibikiPhysics;

GeometryLibrary geometry;
std::vector<Vec3> points{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
ConvexHull solid(points);
TriangleMesh shell(points, {{0, 1, 2}, {0, 3, 1}, {0, 2, 3}, {1, 3, 2}});
bool insideSolid = geometry.Contains(solid, {0.1, 0.1, 0.1}); // true
bool onShell = geometry.Contains(shell, {0.1, 0.1, 0.1});    // false
```

`WorldBounds(shape, worldFromLocal)` 将实际顶点投影到世界坐标系的三个坐标轴，从而得到紧致的轴对齐包围盒，适用于倾斜或不对称的几何体。`LocalBounds(shape)` 使用单位位姿。`Contains(shape, worldFromLocal, worldPoint)` 将查询点逆变换到局部坐标系，然后使用相同的归属判定。

## 数值约定

三角形以首顶点为原点，取其余顶点相对坐标分量的最大绝对值作为归一化尺度。凸包对整个点云采用相同的尺度定义。在归一化坐标系中，面和边的距离判定允许 64 倍双精度机器 epsilon 的误差，以处理边界处的舍入。局部与世界查询先剔除严格位于各自查询坐标系包围盒外的点，再在局部面判定中使用上述容差。小几何体经过大幅平移时，世界坐标的有效精度会限制逆变换的精度。

非有限坐标、无效索引、空网格、单个面内顶点重复或共线以及低维凸包都会抛出 `std::invalid_argument`。归一化后的面边向量叉积长度或初始四面体高度不超过 64 倍机器 epsilon 时，同样按数值退化处理。归一化尺度或包围盒投影超出有限浮点范围时抛出 `std::overflow_error`。

PhysX 将三角网格的重叠测试定义在表面上。MotrixSim 的近似凸分解从源网格生成运行时使用的凸碰撞几何体。这两项约定是区分网格表面和凸实体的依据。[PhysX 几何查询](https://nvidia-omniverse.github.io/PhysX/physx/5.3.0/docs/GeometryQueries.html) · [MotrixSim 近似凸分解](https://motrixsim.readthedocs.io/en/latest/user_guide/main_function/acd.html)
