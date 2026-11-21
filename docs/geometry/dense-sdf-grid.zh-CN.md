# 稠密 SDF 网格

[English](dense-sdf-grid.md) | **简体中文**

`DenseSdfGrid` 在 CPU 内存中保存三角网格局部坐标系下各节点的距离值。使用时包含 `<HibikiPhysics/DenseSdfGrid.h>` 并链接 `hibiki::physics`。坐标、统一间距、存储值和插值结果均采用双精度（64 位），单位为米。

`DenseSdfGrid(origin, spacing, {nx, ny, nz}, values)` 将节点 `(0, 0, 0)` 放在 `origin` 处。节点 `(x, y, z)` 的坐标为 `origin + spacing * (x, y, z)`。三个维度均按节点数计，例如某轴上的四个节点跨越三个间隔。数据在内存中连续排列，X 方向的相邻节点连续存储，然后沿 Y 换行、沿 Z 换层。节点在缓冲区中的偏移为 `x + nx * (y + ny * z)`。向构造函数移入向量即可转移缓冲区所有权，网格的副本共享这份只读数据，可通过 `GetValues()` 读取。

`GetNode(x, y, z)` 读取单个节点的值，任一轴的索引越界时抛出 `std::out_of_range`。`Sample(point)` 取所在单元的八个角点，沿 X、Y、Z 做三线性插值。每个轴的首末节点平面都属于采样域。有限查询点只要超出任一边界就返回 `std::nullopt`，含有非有限坐标的查询点会触发 `std::invalid_argument`。

对于局部坐标 X = 1 米处的平面，有符号距离为 `x - 1`。下例沿 X 方向在相隔 2 米的两个节点平面上存储该距离。

```cpp
HibikiPhysics::DenseSdfGrid grid({0, 0, 0}, 2, {2, 2, 2}, {-1, 1, -1, 1, -1, 1, -1, 1});
const auto inside = grid.Sample({0.5, 1, 2}); // 位于最后一个 Z 平面，距离为 -0.5 米
const auto outside = grid.Sample({0.5, 1, 3}); // std::nullopt
```

构造时要求原点坐标有限、间距为有限正数、每个轴至少有两个节点，且每个节点恰好对应一个有限的距离值。相邻节点的坐标在双精度表示下也必须彼此可区分。上述条件未满足时抛出 `std::invalid_argument` 并指出具体原因。节点总数超出缓冲区可寻址容量，或某轴的坐标或跨度超出有限数范围时，抛出 `std::overflow_error`。
