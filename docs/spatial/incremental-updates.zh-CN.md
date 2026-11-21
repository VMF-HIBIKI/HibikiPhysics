# 增量重叠快照

[English](incremental-updates.md) | **简体中文**

物体在相邻帧沿用同一 ID 时，可选择 `spatial.incremental_overlap` 能力。注册 `SweepAndPruneUpdatePlugin()` 或 `FourWayUpdatePlugin()`，在依赖中声明 `IncrementalSpatialSlot()`，调用选中提供者的 `Build` 构建首帧。独立调用方也可以直接使用工厂。

```cpp
#include <HibikiPhysics/IncrementalSpatialIndex.h>
#include <array>

using namespace HibikiPhysics;

int main()
{
    const auto provider = MakeFourWayUpdateProvider();
    const std::array entries{SpatialEntry{7, {{0, 0, 0}, {1, 1, 1}}},
                             SpatialEntry{9, {{4, 0, 0}, {5, 1, 1}}}};
    const auto first = provider->Build(entries);
    const std::array moved{SpatialEntry{9, {{1, 0, 0}, {2, 1, 1}}}};
    SpatialUpdateStats stats;
    const auto second = first->Updated({{}, moved, {}}, stats);
    return first->OverlapPairs().empty() && second->OverlapPairs().size() == 1 ? 0 : 1;
}
```

批次的三个字段依次是新增条目、携带六个新边界坐标的移动条目，以及待删除的 ID。每个 ID 在整个批次中最多出现一次。新增要求该 ID 在原帧中不存在，移动或删除要求该 ID 已存在。各边界坐标必须有限，区间宽度可以为零。非法输入抛出 `std::invalid_argument`，大小无法表示时抛出 `std::length_error`。失败时，原帧和输出计数保留调用前的值。已删除的 ID 可在后续帧重新插入。

`Updated` 返回拥有新快照的指针，由调用方决定何时切换读者看到的帧。多线程应用可用 `std::atomic<std::shared_ptr<const IncrementalSpatialIndex>>` 发布共享指针。读者在一次查询中持有同一个快照。多个写者可从同一帧分别生成后继帧。空批次与前一帧共享存储。

| 提供者 | 移动物体 | 新增或删除物体 |
| --- | --- | --- |
| 扫描剪枝（SAP） | 根据 ID 和槽位找到变化的 X 端点，仅跨过恢复顺序所需的相邻端点。局部重算前缀最大值，直到可以继续使用旧后缀。只有 Y/Z 改变时沿用全部 X 端点顺序。 | 压紧保留的槽位并保持其端点顺序，对新增端点排序后归并，再重建槽位映射和前缀最大值。 |
| 四叉 BVH | 根据 ID 定位叶子，对相关祖先路径取并集，再从子到父重算包围盒。多个移动叶子共享的祖先只重算一次。连续移动的帧共享树拓扑和 ID 映射。 | 根据最终成员和更新后的包围盒调用一次构建器，重新生成平衡拓扑。 |

为使读者能继续查询旧版本，两种实现都会为有改动的帧复制紧凑的查询数组。每个保留的改动帧都需要线性存储和复制开销。SAP 中的大幅跳变可能跨越大量端点。BVH4 的多次远距离移动可能扩大兄弟分支间的重叠，此时可重新调用 `Build` 来改善剪枝效果。

`mEndpointsChanged` 统计移动条目中 X 坐标变化的端点，`mEndpointSwaps` 统计它们跨过的相邻端点数。`mRefitNodes` 统计移动时重算的不同 BVH4 分支，`mTopologyRebuilds` 统计成员变化引发的 BVH4 拓扑重建。这些计数仅针对当次调用，数组复制和查询另计。[安装包逐帧测试](../../tests/install-consumer/IncrementalSpatialIndex.cpp)
