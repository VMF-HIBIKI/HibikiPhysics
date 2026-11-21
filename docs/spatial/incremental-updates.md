# Incremental overlap snapshots

**English** | [简体中文](incremental-updates.zh-CN.md)

Choose the `spatial.incremental_overlap` capability when objects keep their IDs between frames. Register `SweepAndPruneUpdatePlugin()` or `FourWayUpdatePlugin()`, declare `IncrementalSpatialSlot()` as a dependency, and call the selected provider's `Build` for the first frame. Standalone callers can use either factory directly.

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

Each batch supplies inserted entries, moved entries with all six new bounds, and removed IDs, in that order. IDs must be unique across the whole batch. An insert requires an absent ID. A move or removal requires an existing ID. Bounds must be finite and may have zero width. Invalid input throws `std::invalid_argument`. An unrepresentable size throws `std::length_error`. The current frame and output counters retain their values if an update fails. A removed ID can be inserted again in a later frame.

`Updated` returns an owning pointer to an immutable successor. The caller controls when readers switch frames. A threaded application can publish a `std::shared_ptr<const IncrementalSpatialIndex>` using `std::atomic<std::shared_ptr<const IncrementalSpatialIndex>>`. Each reader keeps its loaded handle for the whole query. Independent writers may derive different successors from the same frame. An empty batch shares its predecessor's storage.

| Provider | Moving objects | Adding or removing objects |
| --- | --- | --- |
| Sweep-and-prune | Locates the changed X endpoints by ID and slot. Each endpoint crosses only the neighbors needed to restore order. Prefix maxima are repaired until the old suffix is valid again. Y/Z-only changes preserve the entire X ordering. | Compacts retained slots without disturbing their endpoint order, sorts the new endpoints and merges them into the sweep. Rebuilds slot lookups and prefix maxima. |
| Four-way BVH | Locates leaves by ID and refits the union of their ancestor paths, from children to parents. Several moved leaves under the same branch cause one refit of that branch. The topology and ID lookup are shared across these frames. | Rebuilds a balanced topology for the final membership and new bounds in one pass through the builder. |

Both providers copy their packed query arrays for a changed frame so readers may retain the old version. Budget linear storage and copy work per retained changed frame. For SAP, a large teleport can cross many endpoints. For BVH4, repeated long moves can increase overlap among sibling branches, and a fresh `Build` can restore pruning quality.

`mEndpointsChanged` counts moved X endpoints whose coordinate changed, and `mEndpointSwaps` counts the neighboring endpoints they crossed. `mRefitNodes` counts the distinct BVH4 branches recomputed on a motion update. `mTopologyRebuilds` counts BVH4 membership rebuilds. These per-call counters exclude array copies and query work. [Installed frame-by-frame tests](../../tests/install-consumer/IncrementalSpatialIndex.cpp)
