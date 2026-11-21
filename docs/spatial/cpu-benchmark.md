# CPU spatial index measurements

**English** | [简体中文](cpu-benchmark.zh-CN.md)

## Measured results

On an AMD Ryzen 7 9700X running Windows 11 x64 and clang-cl 22.1.6 Release, the Morton tree had the shortest build time on each static scene. The stackless tree had the shortest median times for both the 128 box queries and all self-pairs across these four scenes. Each timing is the median of five fresh processes. Peak memory figures are the largest of the five process peaks.

**Static geometry.** Each timing cell lists build / box batch / self-pairs in milliseconds. The first two numeric columns count returned IDs and unordered pairs.

| Scene | Box IDs | Pairs | Median tree | Morton | BVH4 | SAP | Stackless |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Sparse | 173 | 348 | 0.279 / 0.035 / 0.072 | 0.140 / 0.033 / 0.066 | 0.186 / 0.039 / 0.084 | 0.164 / 0.085 / 0.178 | 0.144 / 0.032 / 0.058 |
| Dense | 42,127 | 211,532 | 0.271 / 1.489 / 13.580 | 0.129 / 1.569 / 14.062 | 0.187 / 1.486 / 13.224 | 0.148 / 1.442 / 12.269 | 0.145 / 0.634 / 3.224 |
| Clustered | 2,473 | 11,798 | 0.261 / 0.140 / 0.780 | 0.137 / 0.140 / 0.767 | 0.187 / 0.142 / 0.765 | 0.153 / 0.311 / 1.254 | 0.140 / 0.110 / 0.341 |
| Thin | 829 | 3,400 | 0.279 / 0.120 / 0.478 | 0.127 / 0.074 / 0.285 | 0.183 / 0.128 / 0.515 | 0.156 / 0.258 / 0.723 | 0.149 / 0.066 / 0.175 |

**Pair collection.** On the dense scene, serial Morton traversal took 14.06 ms. Batched atomic emission took 32.15 ms with one worker and 13.37 ms with four. Peak working sets were 13.58, 8.86 and 9.67 MiB, respectively. Both batched cases exceeded the initial capacity of 256 pairs and completed in two passes. On the sparse scene, native traversal needed 0.066 ms and the four-worker batch needed 0.834 ms for 348 pairs.

**Metadata filtering.** Counts below cover all self-pairs. Under dense overlap, caching preserved all 1,060,976 node visits and 32,346 pruned subtrees while reducing source metadata reads from 402,840 to 1,024. The box batch on the same scene went from 39,936 reads to 128.

| Scene | Accepted pairs | Source reads, uncached / cached | Pair time (ms), uncached / cached |
| --- | ---: | ---: | ---: |
| Sparse | 41 | 1,709 / 1,024 | 0.154 / 0.147 |
| Dense | 32,002 | 402,840 / 1,024 | 6.039 / 5.334 |
| Clustered | 1,274 | 23,514 / 1,024 | 0.592 / 0.562 |
| Thin | 608 | 7,671 / 1,024 | 0.378 / 0.365 |

**Coherent motion.** Across eight updates, SAP adjusted 4,096 X endpoints in 74,034 neighbour swaps. BVH4 recomputed 2,136 branch bounds with zero topology rebuilds. The rebuild and update variants returned exactly the same candidate sets at every snapshot.

| Method | Build (ms) | 8 updates (ms) | 9 box batches (ms) | 9 pair sets (ms) | Peak MiB |
| --- | ---: | ---: | ---: | ---: | ---: |
| SAP rebuild | 0.158 | 1.237 | 2.701 | 10.569 | 5.64 |
| SAP update | 0.206 | 0.656 | 2.683 | 10.599 | 5.70 |
| BVH4 rebuild | 0.192 | 1.475 | 1.079 | 6.740 | 5.61 |
| BVH4 refit | 0.228 | 0.536 | 1.096 | 6.828 | 5.65 |

**SDF baking.** The median tree took 285.23 ms and Morton took 232.70 ms (1.23× speedup) for the same 102 × 102 × 41 grid. Both peaked at 8.22 MiB and produced the same fingerprint, `44b393a77a0a32d6`, across all ten bakes.

## Workloads and measurements

All five geometry methods receive identical boxes. [Median CPU tree](../geometry/spatial-index.md) · [Morton LBVH](morton-lbvh.md) · [Four-way BVH](four-way-bvh.md) · [Sweep-and-prune](sweep-and-prune.md) · [Stackless traversal](stackless-lbvh.md)

The emission, filtering and snapshot cases reuse these scenes to measure each alternative under its own query contract. [Batched pair emission](batched-overlap.md) · [Metadata filtering](metadata-pruning.md) · [Incremental updates](incremental-updates.md)

Each static scene has 1,024 boxes and 128 queries. The sparse scene places box centres on an 81-unit grid along each axis. The dense scene packs larger boxes into a 13-unit grid. The clustered scene has eight separated groups. The thin scene includes flat boxes on 13 closely spaced Z layers. Each query batch contains 96 entry boxes, 16 corner points and 16 exterior boxes. An MT19937 with seed 20261121 supplies integer coordinates, and the driver assigns distinct, descending IDs. [Scene generator](../../tests/fixtures/SpatialBenchmarkScenes.h)

The moving scene starts with the clustered layout. The same 256 bodies move by +0.25 or −0.125 unit on each axis for eight successive frames. The initial build and the eight replacement snapshots are timed separately. Query times and candidate counts cover all nine snapshots. SAP and BVH4 each have a full rebuild and an incremental update case. Filtered builds tag the current entries before the build timer starts.

The SDF experiment bakes a 40 × 40 × 10 mm plate with a 10 × 10 mm through-hole. Both providers use a single worker, 0.5 mm spacing, 5 mm padding and a half-cell shift in X and Y. The timer surrounds the complete `BakeDenseSdf` call, including its mesh queries and grid allocation. [Plate fixture](../../tests/fixtures/ThroughHolePlate.h)

On library revision `3e385bf`, the two grids differed at 14 of 426,564 nodes, by up to 7 ULP (8.67 × 10⁻¹⁹ m), with zero sign changes. At one plate corner, differently scaled triangle frames gave different distances to the same mesh vertex and changed which face the nearest traversal visited. The fix evaluates vertex hits at their original mesh coordinates and refines a fast distance that falls below its triangle's conservative box-distance bound. Both trees now select face 2 at that node, return its analytic vertex distance of 0.00093541434669348292 m and agree bit for bit across the whole grid. The regression also checks smallest-ID ties at a shared vertex and a shared edge of triangles with different scales. [Closest-point regressions](../../tests/install-consumer/TriangleMeshQueries.cpp)

Box times cover one batch per snapshot. Pair times cover all distinct closed-box intersections, including result allocation and any provider sorting or overflow retries. For every timed call the driver checks the actual IDs and canonical pair sets against an exhaustive closed-interval test. It performs that verification after reading the timer and the process memory counter. Filtered runs additionally require different owners and equal categories. The uncached and cached variants receive identical tags and predicates. [Benchmark driver](../../tests/install-consumer/SpatialBenchmark.cpp)

Each sample runs in a fresh process. The runner rotates and reverses the case order between repeats. Memory is the peak process working set on Windows or peak resident set on Unix, sampled after query output materialization or grid baking. It includes the runtime, inputs, index, result buffers and allocations from earlier frames. The value cannot isolate bytes owned by a provider. The JSON preserves every sample in milliseconds and bytes, candidate counts for each frame, work counters, compiler and executable identity. [Machine-readable measurements](cpu-results.json)

## Reproduction

From a prepared Windows checkout, run the installed-package checks and then the benchmark.

```powershell
./scripts/environment/VerifyCpp.ps1 -Presets windows-release
python experiments/spatial/measure.py build/installed-tests-windows-release/hibiki_spatial_benchmark.exe --library-revision (git rev-parse HEAD) --output cpu-results.json
```

The installed CTest case `spatial.benchmark_smoke` exercises every method on a small scene and directly compares both baked grids bit for bit. The full runner requires a Release build and verifies matching grid dimensions and fingerprints across both full-query providers and all repetitions.
