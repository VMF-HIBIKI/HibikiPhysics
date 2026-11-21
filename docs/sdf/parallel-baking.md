# Parallel CPU baking measurements

**English** | [简体中文](parallel-baking.zh-CN.md)

A four-worker bake of the half-cell-shifted plate at 0.25 mm took 0.5375 s versus 1.8949 s for one worker on an AMD Ryzen 7 9700X (Windows x64, clang-cl 22.1.6 Release). The 3,305,124 output nodes had the same full-grid fingerprint across all six runs of that configuration. The predicted axial clearance stayed at +0.075 mm. [Accuracy experiment](plate-accuracy.md) · [Raw measurements](parallel-plate-results.json)

| Spacing (mm) | XY phase (cells) | 1 worker (s) | 4 workers (s) | Speedup | Peak process MiB (1 / 4 workers) |
|---:|---:|---:|---:|---:|---:|
| 1 | 0 | 0.0389 | 0.0132 | 2.95× | 5.21 / 5.12 |
| 1 | 0.5 | 0.0355 | 0.0110 | 3.22× | 4.72 / 5.14 |
| 0.5 | 0 | 0.2581 | 0.0740 | 3.49× | 7.49 / 7.90 |
| 0.5 | 0.5 | 0.2517 | 0.0706 | 3.57× | 7.54 / 7.97 |
| 0.25 | 0 | 1.9020 | 0.5550 | 3.43× | 29.27 / 29.68 |
| 0.25 | 0.5 | 1.8949 | 0.5375 | 3.53× | 29.50 / 29.93 |

Each worker count ran three times in fresh processes for each spacing and phase. The runner alternates the order of worker counts. Times are medians of the timed bake call. Speedup is the ratio of the two medians. Memory is the largest process peak working set observed for that configuration. Machine load, grid dimensions and the number of available slices affect these results. [Worker selection and failure propagation](serial-baking.md)

## Reproduction

```powershell
./scripts/environment/VerifyCpp.ps1 -Presets windows-release
python experiments/sdf/measure_accuracy.py build/installed-tests-windows-release/hibiki_sdf_accuracy.exe --workers 1 4 --repeats 3 --output parallel-plate-results.json
```

The runner verifies equal fingerprints over every binary64 node, including padding, across counts and repetitions. Installed-package regressions additionally compare nodes bit for bit under uneven slice counts and check that background and caller errors propagate after workers join.
