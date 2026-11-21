# Batched overlap pairs

**English** | [简体中文](batched-overlap.zh-CN.md)

`CollectOverlapPairs(index, entries, settings, stats)` queries an immutable overlap index using its original entries. Stable IDs select the lower-to-higher direction of each pair. Workers collect up to 64 pairs locally, reserve each batch with an atomic cursor, and write disjoint output ranges. A short buffer still counts the complete result. A second pass allocates exactly that count. The published output has sorted canonical pairs. A query failure joins workers and propagates to the caller. Statistics are published on success.

`BatchedMortonOverlapPlugin(settings)` registers this policy using a CPU Morton tree. Settings specify the total workers (zero selects hardware concurrency) and initial capacity in pairs. The caller participates and the worker count is capped at the input size.

## Dense fixture

The Release installed consumer `hibiki_batch_overlap` builds one Morton tree from 132 coincident boxes (8,646 unordered pairs). On a Ryzen 7 9700X, Windows x64, clang-cl 22.1.6, the median of seven process runs was 0.3449 ms for the tree's serial pair traversal and 0.9222 ms for four workers starting with two slots. The latter performs two passes. Both return the same pairs. Thread startup and the deliberate retry are included in the four-worker time.
