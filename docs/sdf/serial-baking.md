# Baking a dense CPU distance field

**English** | [简体中文](serial-baking.zh-CN.md)

`BakeDenseSdf(mesh, settings)` publishes a complete immutable mesh-local `DenseSdfGrid`. The overload taking a `SpatialQueryProvider` uses the caller's implementation. One index over referenced faces serves boundary validation, nearest-distance queries and parity classification. Samples carry unsigned distance to the surface outside, negative distance inside and zero on the boundary, in metres.

The lattice is anchored at the referenced mesh minimum minus XYZ padding. An optional phase in [0,1) shifts the first node farther towards the negative axis by that fraction of the spacing. Each axis extends to the first representable node covering its padded maximum, counting both endpoints and at least two nodes. Spacing and padding use metres. The output uses X-fastest order. The 40 × 40 × 10 mm plate with 5 mm padding and 1 mm spacing has 51 × 51 × 21 nodes.

The serial reference visits Z slices in order. Invalid geometry and settings raise diagnostics. Oversized dimensions and coordinate ranges report overflow; sample-buffer allocation failures identify the requested count. Publication takes place after all signed queries succeed.
