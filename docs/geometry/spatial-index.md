# CPU spatial index

**English** | [简体中文](spatial-index.zh-CN.md)

A `SpatialEntry` combines a caller-assigned `std::size_t` ID with a closed `AABB` enclosing one primitive. Use one coordinate frame for the entries and every query. IDs must be unique within a build, and both ends of every interval must be finite. Zero-width intervals are valid. Building an empty set produces an empty index. Each index owns its bounds and hierarchy and can serve concurrent read-only queries after the provider and input span have gone out of scope. The caller owns the geometry, group filtering and exact primitive tests.

## Choosing a capability

An overlap client declares `SpatialOverlapSlot()` and receives a `SpatialOverlapProvider`. Register `CpuSpatialOverlapPlugin()` for the default builder. A provider implementing `SpatialOverlapIndex` only needs to supply the two overlap operations. A mesh or SDF client declares `SpatialQuerySlot()` and registers `CpuSpatialQueryPlugin()` to request ray and nearest queries as well. One call to the selected provider's `Build` returns a `SpatialQueryIndex` usable by all four operations. An overlap consumer can take this same index by `const SpatialOverlapIndex&`.

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
    const auto pairs = index->OverlapPairs();  // One pair (4, 12).
    const auto hits = index->RayCandidates({0, 0, 0}, {1, 1, 1});  // Both IDs.
    return pairs.size() == 1 && hits.size() == 2 ? 0 : 1;
}
```

`CapabilityConfig::Replace` accepts a `Borrow` or `Factory` registration for either slot. Replacing one slot affects the clients that request that slot. A standalone application can construct the full CPU provider with `MakeCpuSpatialQueryProvider()`. [Installed-package tests and replacement implementations](../../tests/install-consumer/SpatialIndex.cpp)

## Query semantics

| Operation | Result |
| --- | --- |
| `Overlaps(bounds)` | IDs of candidate primitives for a finite closed query box. Touching at a corner, edge or face counts as overlap. |
| `OverlapPairs()` | All candidate pairs within this set. Each unordered pair of distinct IDs appears once with the smaller ID first. Coincident boxes with different IDs can form a pair. |
| `RayCandidates(origin, direction)` | Candidates along the closed half-line for parameter t ≥ 0. Finite directions can have any length. Each zero component means the ray stays in the corresponding slab coordinate. An all-zero direction tests the origin. The CPU slab calculation rounds outward near grazing intersections. |
| `Nearest(point, exactDistance)` | A `SpatialNearest` with ID and linear Euclidean distance. For each visited leaf, the callback receives its ID and computes distance from the query point to its enclosed primitive. A callback may return positive infinity to exclude that leaf. Equal finite distances select the smallest ID. An empty index or all-excluded set yields `std::nullopt`. |

The CPU builder splits the longest axis of each node's bounds at the median of the primitive box centers. Equal axis lengths prefer X, then Y, then Z. Equal centers on the split axis use the ID as a tie-breaker. The CPU implementation returns candidate IDs and pairs in ascending order. Other providers may use their own traversal and output order.

Nearest traversal visits the closer child first and uses the Euclidean point-to-box distance as a lower bound. Only an exact finite leaf result tightens the best distance. A node is pruned when its lower bound exceeds that distance. The callback's geometry must lie inside its entry bounds for pruning to be valid. Each simultaneous query needs a callback safe for its use on that thread.

Nonfinite entry or query coordinates, duplicate IDs, an empty distance callback, or a visited callback returning a negative value or NaN raise `std::invalid_argument`.
