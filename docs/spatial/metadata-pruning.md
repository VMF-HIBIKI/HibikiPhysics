# Metadata-aware LBVH traversal

**English** | [简体中文](metadata-pruning.zh-CN.md)

`FilteredMortonPlugin()` registers an optional filtered overlap provider. Each leaf carries two opaque caller tags, owner and category. A node records a uniform summary only when all leaves below it carry the same tags. The query predicate takes two metadata values and must be deterministic and side-effect free. Returning false for a uniform subtree skips every leaf in it. Mixed nodes descend and leaves receive the exact policy.

An individual box query uses the supplied source ID's metadata and includes its own ID when the box and predicate permit. Self-pairs appear once, in increasing ID order, with the lower ID's metadata as the first argument. The unfiltered methods ignore tags. The baseline loads query metadata at each predicate evaluation. Per-call counters report visits, query metadata reads, leaf tests and pruned internal nodes. Each query owns its counters and temporary storage.

## Caching the source tags

`CachedFilteredMortonPlugin()` selects `hibiki.cached_filtered_morton` in `FilteredSpatialSlot()`. Use `config.Replace(CachedFilteredMortonPlugin())` when the baseline already occupies that slot. A standalone caller can use `MakeCachedFilteredMortonProvider()` from `<HibikiPhysics/FilteredSpatialIndex.h>`.

The cached provider copies the source's two tags into a traversal-local value before testing the root. A box query reuses the source ID lookup that validates the call. Pair enumeration copies each source leaf's tags once and reuses them through that leaf's traversal. `mQueryReads` counts these logical source-record reads. A cached box query therefore reports one read even when it misses the root, and a pair query over N leaves reports N. Both providers use the same uniform summaries, node walk, predicate and result ordering.

### Release measurements

The experiment builds 512 boxes on a 32 × 16 grid. Each box spans two grid intervals in X and Y. The mixed scene gives each 4 × 4 block a common owner/category combination. The predicate accepts a lower source owner or a different category. The outside-root workload runs one disjoint box query for each source ID. Construction and comparison of the complete ordered candidate sets take place before timing. The timed calls include result allocation and sorting. [Experiment source](../../experiments/spatial/query-metadata/QueryMetadata.cpp)

| Workload | Candidates | Visits | Leaf tests | Pruned subtrees | Source reads, baseline / cached | Time, baseline / cached (µs) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Uniform, all pairs | 0 | 512 | 0 | 512 | 512 / 512 | 3.22 / 3.45 |
| Mixed, all pairs | 1,806 | 23,552 | 3,948 | 1,472 | 10,892 / 512 | 95.84 / 92.60 |
| Outside-root boxes | 0 | 512 | 0 | 0 | 0 / 512 | 6.22 / 5.44 |

On Windows x64, Ryzen 7 9700X, clang-cl 22.1.6 Release, caching cut the mixed pair time by 3.4%. Uniform root rejection used the same number of reads and took 0.23 µs longer. The outside-root workload added one logical read per box and still ran faster in this measurement. These times are medians of five fresh process medians. Each process warms both providers and alternates their order across 21 samples of ten complete workloads per provider.

With the prepared Windows toolchain, build the Release consumer and run the experiment five times.

```powershell
& ./scripts/environment/VerifyCpp.ps1 -Presets windows-release
1..5 | ForEach-Object { & ./build/installed-tests-windows-release/hibiki_query_metadata.exe }
```
