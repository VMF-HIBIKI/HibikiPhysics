# Through-hole plate SDF accuracy

**English** | [简体中文](plate-accuracy.zh-CN.md)

A sphere of radius 4.8 mm travels along the Z axis through a centred 10 × 10 mm square hole in a 40 × 40 × 10 mm plate. The exact minimum clearance is 0.2 mm. The experiment measures how mesh baking, trilinear interpolation, voxel spacing and grid alignment affect the clearance inferred from the centre distance.

## Measured results

On the half-cell grids, only the 0.25 mm spacing preserves positive clearance for this sphere. In the middle of the bore the exact distance is `5 − max(|x|, |y|)` mm. With the axis on a node its value is 5 mm. With the axis at the centre of an XY cell all four neighbours are at distance `5 − h/2` mm. Interpolation therefore reduces clearance by half a cell and changes its sign when `h > 0.4 mm`. Even with accurately baked nodes, the 1 mm shifted grid predicts 0.3 mm of interference along a path with 0.2 mm of physical clearance.

Distances, absolute errors and displacements below are in millimetres. On aligned grids the measured wall offsets are within the bisection uncertainty of 4.55e−12 mm.

| h | XY phase | Node max | Probe mean | Probe max | Wall max | Centre | Minimum clearance |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 0 | 4.34e-15 | 0.02617 | 0.26089 | 4.55e-12 | 5.000 | 0.200 |
| 1 | 0.5 | 5.42e-15 | 0.04599 | 0.32359 | 0.38889 | 4.500 | -0.300 |
| 0.5 | 0 | 7.59e-15 | 0.00792 | 0.12925 | 4.55e-12 | 5.000 | 0.200 |
| 0.5 | 0.5 | 4.01e-15 | 0.01327 | 0.13788 | 0.15000 | 4.750 | -0.050 |
| 0.25 | 0 | 7.59e-15 | 0.00231 | 0.05970 | 4.55e-12 | 5.000 | 0.200 |
| 0.25 | 0.5 | 5.85e-15 | 0.00355 | 0.06925 | 0.04167 | 4.875 | 0.075 |

Serial CPU measurements on 2026-11-21 used the baker at `8355194`, Windows 11 x64 build 26200, AMD Ryzen 7 9700X and clang-cl 22.1.6. The C++20 Release library used `/O2`, x86-64, disabled fast math and disabled floating-point contraction. Times and process peaks are single observations per configuration. The [raw JSON](plate-accuracy-results.json) includes exact byte counts, both mean and maximum errors, and the number of comparisons.

| h (mm) | XY phase | Grid shape | Nodes | Grid (MiB) | Peak process (MiB) | Bake (s) |
| ---: | ---: | --- | ---: | ---: | ---: | ---: |
| 1 | 0 | 51 × 51 × 21 | 54,621 | 0.417 | 5.211 | 0.0384 |
| 1 | 0.5 | 52 × 52 × 21 | 56,784 | 0.433 | 4.699 | 0.0352 |
| 0.5 | 0 | 101 × 101 × 41 | 418,241 | 3.191 | 7.457 | 0.2636 |
| 0.5 | 0.5 | 102 × 102 × 41 | 426,564 | 3.254 | 7.535 | 0.2483 |
| 0.25 | 0 | 201 × 201 × 81 | 3,272,481 | 24.967 | 29.262 | 1.8985 |
| 0.25 | 0.5 | 202 × 202 × 81 | 3,305,124 | 25.216 | 29.496 | 1.8830 |

## Reproduce

From a prepared Windows checkout, build the Release library and its installed-package consumers. Run the six configurations from the repository root.

```powershell
./scripts/environment/VerifyCpp.ps1 -Presets windows-release
python experiments/sdf/measure_accuracy.py build/installed-tests-windows-release/hibiki_sdf_accuracy.exe --output plate-accuracy.json
```

The driver runs each configuration once in a fresh process. Omitting `--output` prints the JSON report. For a single measurement, run `hibiki_sdf_accuracy 1 0.5` with the spacing in millimetres and XY phase in cells. The installed CTest suite uses a 2.5 mm half-cell smoke case. The [runner](../../experiments/sdf/measure_accuracy.py) and [measurement code](../../tests/install-consumer/SdfAccuracy.cpp) contain the complete inputs.

## Reference and probes

The [mesh fixture](../../tests/fixtures/ThroughHolePlate.h) is centred at the origin. The independent reference uses only the stated dimensions. In a plane, write the signed distance to the square of half-width `a` as `Q(a) = hypot(max(u,0),max(v,0)) + min(max(u,v),0)`, with `u = |x| − a` and `v = |y| − a`. In millimetres, the signed distance to the square annulus is `P = max(Q(20), −Q(5))`. Its nested boundaries let this select the distance to the hole from its interior, to the outer square from outside, and to the nearest boundary from material. For the extrusion, let `Z = |z| − 5`. The exact distance is

```text
hypot(max(P,0), max(Z,0)) + min(max(P,Z),0)
```

Outside the solid, the horizontal and vertical offsets to its closest point are perpendicular. Inside, the shorter escape to a vertical wall or a horizontal face gives the depth. Positive values denote free space. All mesh, grid and query coordinates are in metres in code. Reported distances and displacements are in millimetres.

| Measurement | Definition |
| --- | --- |
| Bake and alignment | CPU `BakeDenseSdf` on the same mesh with XYZ padding (5, 5, 5) mm. Spacing is 1, 0.5 or 0.25 mm. Shift the origin by (0, 0, 0) or (−h/2, −h/2, 0) from the aligned padded minimum. |
| Node error | Maximum and mean absolute difference between stored nodes and the analytic reference within the common region [−21, 21]² × [−10, 10] mm. All grid nodes, including padding, count toward storage. |
| Interpolated distance error | Maximum and mean absolute error at the same 4,096 physical probes. The first 2,048 Halton triples (bases 2, 3, 5, starting at index 1) each map to one point in the common region and one in [4, 6]³ mm near a hole corner and lip. These means weight the two regions equally. |
| Hole-wall displacement | For each of four walls, probe the Cartesian product of five tangential positions (−4.875, −2.375, 0, 2.375, 4.875) mm and three heights (−4.875, 0, 4.875) mm. Bisect the sampled SDF along the outward horizontal ray from the hole, from radius 0 to 10 mm, for 40 iterations. Each exact crossing is at 5 mm. Aggregate the 60 absolute radial offsets. |
| Axial sphere clearance | Sample the centre on (0, 0, z) at 0.125 mm intervals over [−10, 10] mm and subtract the 4.8 mm radius. This includes the Z cell boundaries for every experimental spacing. The interpolant on each axial segment is linear, so the minimum over those samples captures the path minimum to roundoff. A negative value predicts interference. |
| Cost | Time only the `BakeDenseSdf` call using a steady clock. `grid_bytes` is eight bytes per stored double. Immediately after the call, read the process high-water resident memory, including runtime and bake allocations (Windows peak working set or POSIX maximum RSS). The reference and probes run afterward. |
