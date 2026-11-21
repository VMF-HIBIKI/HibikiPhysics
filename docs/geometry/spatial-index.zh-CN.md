# CPU 空间索引

[English](spatial-index.md) | **简体中文**

`SpatialEntry` 将调用方分配的 `std::size_t` ID 与包围单个图元的闭区间 `AABB` 关联起来。输入和查询使用同一坐标系。同一次构建中的 ID 必须唯一，各轴区间的两端均须有限，区间宽度可以为零。空输入生成空索引。索引持有包围盒和层次结构，提供者及输入 span 生命周期结束后仍可并发执行只读查询。几何数据、碰撞分组过滤和图元精确判定由调用方管理。

## 选择查询能力

`CpuSpatialOverlapPlugin()` 向 `SpatialOverlapSlot()` 注册无栈 Morton LBVH（`hibiki.stackless_lbvh`），用于 CPU 粗筛的盒查询和候选对枚举。`CpuSpatialQueryPlugin()` 向 `SpatialQuerySlot()` 注册完整 Morton LBVH（`hibiki.morton_lbvh`），其索引还支持射线候选和最近图元查询。SDF 烘焙默认使用这份完整查询实现。

```cpp
#include <HibikiPhysics/SpatialIndex.h>
#include <array>

using namespace HibikiPhysics;

int main()
{
    CapabilityConfig config;
    config.Add(CpuSpatialQueryPlugin());
    Dependencies requirements;
    SpatialQuerySlot().Declare(requirements);
    const auto scope = config.Build(requirements);

    const std::array<SpatialEntry, 2> entries{{
        {12, {{0, 0, 0}, {1, 1, 1}}},
        {4, {{1, 1, 1}, {2, 2, 2}}}
    }};
    const auto index = SpatialQuerySlot().Get(scope).Build(entries);
    const auto pairs = index->OverlapPairs();  // 唯一的候选对为 (4, 12)。
    const auto hits = index->RayCandidates({0, 0, 0}, {1, 1, 1});  // 包含两个 ID。
    return pairs.size() == 1 && hits.size() == 2 ? 0 : 1;
}
```

`MakeCpuSpatialQueryProvider()` 直接构造完整 Morton 提供者。做中位数 BVH 对照时，粗筛使用 `config.Replace(MedianSpatialOverlapPlugin())`，完整查询使用 `config.Replace(MedianSpatialQueryPlugin())`。两项中位数注册的实现名均为 `hibiki.cpu_bvh`。独立查询和 SDF 烘焙也可通过 `MakeMedianSpatialQueryProvider()` 获取对照实现。[安装包注册测试](../../tests/install-consumer/MortonSpatialIndex.cpp)

## 查询语义

| 操作 | 结果 |
| --- | --- |
| `Overlaps(bounds)` | 与有限闭包围盒重叠的候选图元 ID。面、棱和角上的接触均算重叠。 |
| `OverlapPairs()` | 当前集合内部的全部候选对。不同 ID 组成的无序对只出现一次，较小的 ID 在前。重合包围盒上的不同 ID 可以配对。 |
| `RayCandidates(origin, direction)` | 返回参数 t ≥ 0 的闭射线经过的候选图元。有限方向向量可以取任意长度。某个分量为零时，射线对应坐标保持不变。方向全零时查询原点位置。CPU 实现对接近相切的射线区间向外舍入。 |
| `Nearest(point, exactDistance)` | 返回包含 ID 和欧氏距离的 `SpatialNearest`。回调接收已访问叶子的 ID，计算查询点到该叶子所包围图元的实际距离。回调可返回正无穷以排除该图元。距离相同时选择较小的 ID。空索引及所有图元均被排除的情况返回 `std::nullopt`。 |

无栈粗筛按遍历顺序输出命中项，每对内部的较小 ID 在前。需要全局升序的调用方可对返回的 ID 或候选对排序。完整 Morton 和中位数提供者均返回已排序的 ID 和候选对。中位数构建器沿最长轴按包围盒中心的中位数划分，轴等长时依次优先选 X、Y、Z，中心坐标相同时按 ID 划分。

最近遍历先访问较近的子节点，以查询点到节点包围盒的欧氏距离为下界。叶子回调返回有限的精确距离后才会收紧当前最优距离。节点下界大于当前最优距离时可剪枝。调用方须保证图元处于输入包围盒内，以保证剪枝正确。并发查询使用的回调需要能在各自的线程中安全执行。

输入或查询坐标非有限、ID 重复、缺少距离回调，或者已访问的回调返回负数或 NaN 时，均抛出 `std::invalid_argument`。

各 CPU 实现的构建、查询、更新与 SDF 烘焙成本，以及原始测量结果和复现命令，见 [CPU 空间索引基准](../spatial/cpu-benchmark.zh-CN.md)。
