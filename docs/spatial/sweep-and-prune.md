# CPU sweep-and-prune

**English** | [简体中文](sweep-and-prune.zh-CN.md)

To reuse endpoint ordering between frames, select the [incremental update provider](incremental-updates.md).

Include `<HibikiPhysics/SweepAndPrune.h>` and call `MakeSweepAndPruneProvider()` to build a CPU overlap index. To select it through the plugin system, add `SweepAndPrunePlugin()` to a `CapabilityConfig`, or pass it to `Replace` if another provider already occupies `SpatialOverlapSlot()`. Its implementation ID is `hibiki.sweep_and_prune`. [Spatial index contract](../geometry/spatial-index.md)

Each entry has a unique caller ID and finite, closed bounds. The index owns its data, and both queries can run concurrently. Face, edge and corner contact count as overlap, including for boxes with zero width on any axis. Nonfinite endpoints in entries or queries and duplicate entry IDs raise `std::invalid_argument`.

## Endpoints and queries

The builder sorts entries by ID and stores the minimum and maximum X endpoint of each box. At equal X coordinates, starts precede ends, with equal-kind ties ordered by ID. A point on the right face of one box and the left face of another therefore meets both during the sweep.

`OverlapPairs()` visits the endpoints from left to right. At a start it compares the entering box against every active box on Y and Z. Each pair receives exactly one test, at the later of the two starts, if the boxes overlap on X. At an end the sweep removes the box from the active set in constant time. Results use the smaller ID first and are sorted lexicographically.

`Overlaps(bounds)` uses two binary searches. Starts to the right of the query maximum are excluded. A prefix maximum over the right endpoints of boxes opened so far skips earlier starts whose boxes all end left of the query. The index checks the remaining starts against the query and returns IDs in ascending order.

Building an index with `n` boxes takes O(n log n) time and O(n) storage. For `c` overlapping X interval pairs and `k` reported 3D pairs, pair enumeration takes O(n + c + k log(k + 1)) time and O(n + k) working and result storage. X-separated boxes keep the active set small. A common X projection can require quadratic pair testing even when Y or Z separates every box. A single-box query visiting `v` endpoints and reporting `h` hits takes O(log(n + 1) + v + h log(h + 1)) time.
