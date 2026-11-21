# Morton linear BVH

**English** | [简体中文](morton-lbvh.zh-CN.md)

The CPU Morton provider builds an immutable hierarchy over finite, closed AABBs. It supports overlap candidates, self-overlap pairs, ray candidates and nearest-primitive queries through the existing spatial capability slots. Each result refers to the caller's original ID. [Spatial query contracts](../geometry/spatial-index.md)

## Selecting the provider

A client can replace the default median BVH for a particular capability scope. This example requests the full query provider and checks a shared corner.

```cpp
#include <HibikiPhysics/MortonSpatialIndex.h>
#include <array>

int main()
{
    using namespace HibikiPhysics;
    CapabilityConfig config;
    config.Add(CpuSpatialQueryPlugin());
    config.Replace(MortonSpatialQueryPlugin());
    Dependencies requirements;
    SpatialQuerySlot().Declare(requirements);
    const auto scope = config.Build(requirements);
    const auto &provider = SpatialQuerySlot().Get(scope);

    const std::array<SpatialEntry, 2> entries{{
        {7, {{0, 0, 0}, {1, 1, 1}}},
        {3, {{1, 1, 1}, {2, 2, 2}}}
    }};
    const auto index = provider.Build(entries);
    return index->OverlapPairs() == std::vector<std::pair<std::size_t, std::size_t>>{{3, 7}} ? 0 : 1;
}
```

Overlap-only clients can register `MortonSpatialOverlapPlugin()` in `SpatialOverlapSlot()`. Each Morton registration reports `hibiki.morton_lbvh` as its implementation. Standalone clients can call `MakeMortonSpatialQueryProvider()` or `MakeMortonSpatialOverlapProvider()`. The SDF baker accepts the selected query provider as the third argument to `BakeDenseSdf`. [Installed consumer test](../../tests/install-consumer/MortonSpatialIndex.cpp)

## Building the hierarchy

The builder computes each box center with a floating-point midpoint. It normalizes each center coordinate to the range of centers on that axis, maps it into one of 2²¹ bins and clamps the upper endpoint to the last bin. A constant axis maps to bin zero. When the interval width overflows, halving the endpoints and value before taking differences preserves their ratio.

Interleaving the three bin indices gives a 63-bit Morton code. The builder orders leaves by code, then by unsigned caller ID. Each range splits at the highest bit at which its first and last codes differ. If both codes match, the split uses the highest differing ID bit. This defines the same tree under input permutation and also handles fully coincident centers. Parents enclose the original child bounds, so quantization only influences grouping. The tree stores one leaf per entry and one internal node per split in preorder.

Both CPU builders share the closed-box intersection, conservative ray and nearest-distance traversal. They return sorted candidate IDs and sorted canonical pairs. For nearest queries, only a finite exact leaf distance updates the result, ties select the smallest ID, and a positive-infinite callback result excludes that primitive.
