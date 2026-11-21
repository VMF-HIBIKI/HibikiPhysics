# 高度场地形

[English](height-field.md) | **简体中文**

`HeightField(rows, columns, {dx, dy}, base, samples)` 持有一个矩形高程网格，长度单位为米。首个采样点位于局部 XY 原点。第 `row` 行、第 `col` 列的高程存于 `samples[row * columns + col]`，平面坐标为 `(col * dx, row * dy)`，高度沿 +Z 方向。行数和列数都至少为 2。两个方向的间距必须为有限正数，网格跨度必须有限。底面高程和每个采样值都必须有限，且底面不得高于最低采样点。

地形是多个闭合三棱柱的并集，所有三棱柱共用一个底面高程。MuJoCo 也采用三棱柱和有限深度的底面表示高度场碰撞几何。HibikiPhysics 直接使用以米表示的高程，并采用本文约定的原点、布局和三角剖分。[MuJoCo 高度场参考](https://mujoco.readthedocs.io/en/3.1.0/XMLreference.html#asset-hfield)

每个网格单元都沿 X、Y 同时最小的角到 X、Y 同时最大的角划分为两个三角形，每个三角形内线性插值。例如，一个间距为 1 米的 2 × 2 网格，按行存储的高程为 `{0, 8, 4, 0}`，中心点高程为 0，`(0.75, 0.25)` 处高程为 4 米。`SurfaceHeight(x, y)` 可查询最外侧边缘上的高程，查询范围外的有限坐标时返回空的 optional。

`GeometryLibrary::Contains` 包含顶面、侧面和底面，顶面与底面重合处也属于地形。局部包围盒从 `(0, 0, base)` 延伸到 `((columns - 1) * dx, (rows - 1) * dy, max(samples))`。世界坐标查询接受从局部到世界的刚体位姿。世界包围盒通过将所有顶面采样点和四个底角投影到各个世界坐标轴得到。
