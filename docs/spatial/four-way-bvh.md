# Four-way AABB tree

**English** | [简体中文](four-way-bvh.zh-CN.md)

The BVH4 indexes finite, closed axis-aligned boxes in three dimensions. Jolt's broad phase groups up to four bounding boxes under a parent. This implementation uses that branching factor with double-precision bounds, balanced median partitions and immutable snapshots. Each leaf retains one caller-assigned ID. [Jolt QuadTree](https://github.com/jrouwe/JoltPhysics/blob/master/Jolt/Physics/Collision/BroadPhase/QuadTree.h)

To use the BVH4 with an existing overlap client, replace the registration in its configuration. The standalone factory is `MakeFourWaySpatialOverlapProvider()`. [Spatial capability contract](../geometry/spatial-index.md)

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

## Grouping and overlap traversal

A group of two to four entries becomes one branch with one leaf per entry. Larger groups split at the median on the axis with the widest range of box centers. Each half repeats the split on its own widest axis, giving the parent four children. Tied axis widths prefer X, then Y, then Z. Tied center coordinates use the stable ID. A branch bounds the union of its children. Empty inputs produce an empty index, and a singleton is a root leaf.

A box query descends only through intersecting bounds. For self-overlap, each branch compares all unordered pairs of its children. A disjoint pair prunes both subtrees from that comparison. An intersecting pair descends until two leaves produce a candidate. Each pair of leaves separates at exactly one lowest common ancestor. IDs are returned in ascending order, and pairs are ordered lexicographically with the smaller ID first. Contact at a face, edge or corner counts as overlap.

## Measuring traversal

Construct `FourWaySpatialIndex index(entries)` and pass a `std::size_t` by reference to `index.Overlaps(bounds, visits)` or `index.OverlapPairs(visits)`. For box queries the count includes each tested node, including a rejected root. For pair queries it counts each comparison of two distinct nodes' bounds, including failed comparisons. Empty queries report zero tests on an empty tree. Concurrent callers can use separate counters on one index.

For 1,024 disjoint point boxes spaced along Z, a query at one point tests 21 nodes across five levels of branches. Enumerating all pairs tests six sibling combinations at each of the 341 branches, yielding 2,046 comparisons and zero candidates. [Installed consumer checks](../../tests/install-consumer/FourWaySpatialIndex.cpp)
