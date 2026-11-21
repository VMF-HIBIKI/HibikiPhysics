# CPU spatial index

**English** | [简体中文](spatial-index.zh-CN.md)

A `SpatialEntry` combines a caller-assigned `std::size_t` ID with a closed `AABB` enclosing one primitive. Use one coordinate frame for the entries and every query. IDs must be unique within a build, and both ends of every interval must be finite. Zero-width intervals are valid. Building an empty set produces an empty index. Each index owns its bounds and hierarchy and can serve concurrent read-only queries after the provider and input span have gone out of scope. The caller owns the geometry, group filtering and exact primitive tests.

## Choosing a capability

`CpuSpatialOverlapPlugin()` registers the stackless Morton LBVH (`hibiki.stackless_lbvh`) in `SpatialOverlapSlot()` for CPU broad-phase box queries and candidate pairs. `CpuSpatialQueryPlugin()` registers the full Morton LBVH (`hibiki.morton_lbvh`) in `SpatialQuerySlot()`. Its index also supports ray candidates and nearest primitives. The default SDF baker uses this full provider.

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

`MakeCpuSpatialQueryProvider()` constructs the full Morton provider directly. To run the median-split BVH as a reference, call `config.Replace(MedianSpatialOverlapPlugin())` for broad phase or `config.Replace(MedianSpatialQueryPlugin())` for full queries. Both median registrations report `hibiki.cpu_bvh`. A standalone caller or SDF bake can use `MakeMedianSpatialQueryProvider()`. [Installed-package registration tests](../../tests/install-consumer/MortonSpatialIndex.cpp)

## Query semantics

| Operation | Result |
| --- | --- |
| `Overlaps(bounds)` | IDs of candidate primitives for a finite closed query box. Touching at a corner, edge or face counts as overlap. |
| `OverlapPairs()` | All candidate pairs within this set. Each unordered pair of distinct IDs appears once with the smaller ID first. Coincident boxes with different IDs can form a pair. |
| `RayCandidates(origin, direction)` | Candidates along the closed half-line for parameter t ≥ 0. Finite directions can have any length. Each zero component means the ray stays in the corresponding slab coordinate. An all-zero direction tests the origin. The CPU slab calculation rounds outward near grazing intersections. |
| `Nearest(point, exactDistance)` | A `SpatialNearest` with ID and linear Euclidean distance. For each visited leaf, the callback receives its ID and computes distance from the query point to its enclosed primitive. A callback may return positive infinity to exclude that leaf. Equal finite distances select the smallest ID. An empty index or all-excluded set yields `std::nullopt`. |

The stackless broad phase emits hits in traversal order. Each pair places the smaller ID first. Callers that require a globally sorted list can sort the returned IDs or pairs. The full Morton and median providers return sorted IDs and pairs. The median builder splits the longest axis at the median of the box centers, breaks axis ties in X, Y, Z order and breaks center ties by ID.

Nearest traversal visits the closer child first and uses the Euclidean point-to-box distance as a lower bound. Only an exact finite leaf result tightens the best distance. A node is pruned when its lower bound exceeds that distance. The callback's geometry must lie inside its entry bounds for pruning to be valid. Each simultaneous query needs a callback safe for its use on that thread.

Nonfinite entry or query coordinates, duplicate IDs, an empty distance callback, or a visited callback returning a negative value or NaN raise `std::invalid_argument`.

Compare build, query, update and SDF baking costs across the CPU providers, with raw measurements and reproduction commands in the [CPU spatial benchmark](../spatial/cpu-benchmark.md).
