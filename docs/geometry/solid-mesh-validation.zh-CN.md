# 验证闭合三角网格

[English](solid-mesh-validation.md) | **简体中文**

使用三角网格定义实体之前，可调用 `HibikiPhysics/SolidMeshValidation.h` 中的 `ValidateClosedSolid(mesh, faceIndex)`。验证通过表示输入由一个连通的闭合壳体组成。壳体可以带有贯穿孔。相交表面、多个独立实体、用嵌套壳体表示的封闭空腔都会被拒绝。各三角面的顶点绕序可以不同。验证保留原始网格、索引和面顺序。

使用每个三角面的局部包围盒构建一次索引。条目 ID 取该面在 `GetFaces()` 中从零开始的下标，每个面都必须拥有一个条目。验证器通过传入的索引找到包围盒重叠的面，后续距离和射线查询可复用同一棵树。

```cpp
#include <HibikiPhysics/Geometry.h>
#include <HibikiPhysics/SolidMeshValidation.h>
#include <HibikiPhysics/SpatialIndex.h>

using namespace HibikiPhysics;

TriangleMesh tetrahedron({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}},
                         {{0, 1, 2}, {0, 3, 1}, {0, 2, 3}, {1, 3, 2}});
GeometryLibrary geometry;
const auto &vertices = tetrahedron.GetVertices();
std::vector<SpatialEntry> entries;
for (const auto &[a, b, c] : tetrahedron.GetFaces())
    entries.push_back({entries.size(), geometry.LocalBounds(Triangle(vertices[a], vertices[b], vertices[c]))});
auto index = MakeCpuSpatialQueryProvider()->Build(entries);
ValidateClosedSolid(tetrahedron, *index);
```

判断连通性时，两个顶点的三个坐标必须分别相等。导入的盒子可以为各个面分别保存顶点。相差一个浮点步长的顶点仍会形成真实裂缝。未被引用的顶点不参与计算。每条无向边必须恰好连接两个面。沿顶点周围的边依次访问相邻面，必须能在一个环内遍历所有关联面。

对于每对候选三角形，交集可以为空，也可以恰好是两者共有的顶点或边。精确相交检查使用方向谓词判断边穿过三角形、端点接触和共面重叠。当浮点运算无法确定符号时，使用整数行列式求出输入浮点坐标对应的精确符号。

验证失败时抛出 `std::invalid_argument`。边错误给出源面及该面中两个端点的原始索引。顶点周围出现多个面环时，诊断指出源顶点和关联面。重复面、面间相交和不连通壳体的诊断给出相关源面。消息中的索引都从零开始。
