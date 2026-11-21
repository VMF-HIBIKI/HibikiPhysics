# 四分支 AABB 树

[English](four-way-bvh.md) | **简体中文**

BVH4 为三维空间中的有限闭区间轴对齐包围盒建立索引。Jolt 的宽相位树在每个父节点下保存至多四个子包围盒。本实现采用相同的分支数，以双精度边界和中位数划分构建只读快照。每个叶节点保留一个调用方分配的 ID。[Jolt QuadTree](https://github.com/jrouwe/JoltPhysics/blob/master/Jolt/Physics/Collision/BroadPhase/QuadTree.h)

已有重叠查询客户端可以通过配置替换提供者。独立构造提供者使用 `MakeFourWaySpatialOverlapProvider()`。[空间查询能力约定](../geometry/spatial-index.zh-CN.md)

```cpp
#include <HibikiPhysics/FourWaySpatialIndex.h>
#include <array>

using namespace HibikiPhysics;

int main()
{
    CapabilityConfig config;
    config.Add(CpuSpatialOverlapPlugin());
    config.Replace(FourWaySpatialOverlapPlugin());
    Dependencies requirements;
    SpatialOverlapSlot().Declare(requirements);
    const auto scope = config.Build(requirements);

    const std::array<SpatialEntry, 2> entries{{
        {11, {{0, 0, 0}, {1, 1, 1}}},
        {3, {{1, 0, 0}, {2, 1, 1}}}
    }};
    const auto index = SpatialOverlapSlot().Get(scope).Build(entries);
    return index->OverlapPairs() == std::vector{std::pair<std::size_t, std::size_t>{3, 11}} ? 0 : 1;
}
```

## 分组与重叠遍历

一组中有二至四个条目时，各条目直接成为同一分支的叶节点。较大的组选择盒中心坐标跨度最大的轴，沿中位数一分为二。两半分别重新选轴，再各自分成两组，形成四个子节点。跨度相同时依次优先选 X、Y、Z 轴。选定轴上的中心坐标相同时按稳定 ID 排序。父包围盒包住所有子包围盒。空输入生成空树，单个条目直接成为根叶节点。

单盒查询仅沿相交的包围盒向下遍历。自重叠查询枚举每个分支下的所有无序子节点对。两盒分离时可结束这一对子树的比较。相交时继续向下遍历，直至两个叶节点产生候选对。任意两个叶节点只在其最低公共祖先处首次分入不同的子树。单盒命中的 ID 升序排列，候选对按字典序排列且小 ID 在前。面、边或顶点处接触均计为重叠。

## 测量遍历开销

构造 `FourWaySpatialIndex index(entries)`，然后向 `index.Overlaps(bounds, visits)` 或 `index.OverlapPairs(visits)` 传入 `std::size_t` 计数变量的引用。单盒查询统计测试过的节点数，根节点即使因分离而被剪枝也计入。自重叠查询统计两个不同节点之间的包围盒测试次数，包括分离的情况。查询空树时计数为零。多个线程同时查询同一索引时可以各自使用独立的计数变量。

对于沿 Z 轴分布的 1,024 个互不相交的点包围盒，查询其中一个点经过五层分支，共测试 21 个节点。枚举全部候选对时，341 个分支各比较六对子节点，共比较 2,046 次，结果为空。[安装包消费者测试](../../tests/install-consumer/FourWaySpatialIndex.cpp)
