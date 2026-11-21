# Baking a dense CPU distance field

**English** | [简体中文](serial-baking.zh-CN.md)

`BakeDenseSdf(mesh, settings)` bakes a mesh-local `DenseSdfGrid` with the full Morton LBVH. One index over referenced faces serves boundary validation, nearest-distance queries and ray-based parity classification. For a median BVH comparison, pass `*MakeMedianSpatialQueryProvider()` as the third argument. The same overload accepts a caller-supplied `SpatialQueryProvider`. The completed grid is immutable. Distances are in metres, positive outside, negative inside and zero on the boundary.

The lattice is anchored at the referenced mesh minimum minus XYZ padding. An optional phase in [0,1) shifts the first node farther towards the negative axis by that fraction of the spacing. Each axis extends to the first representable node covering its padded maximum, counting both endpoints and at least two nodes. Spacing and padding use metres. The output uses X-fastest order. The 40 × 40 × 10 mm plate with 5 mm padding and 1 mm spacing has 51 × 51 × 21 nodes.

`SdfBakeSettings::mWorkerCount` defaults to 1. The caller then visits Z slices in order. A positive count includes the calling thread, and 0 selects `std::thread::hardware_concurrency()` with a fallback of 1 if the hardware count is unavailable. The baker caps the count at the number of Z slices. With three workers, for example, the caller fills slices 0, 3, 6 and each of the other workers starts one slice later and advances by the same stride.

Workers share the validated mesh and its immutable index. Each query uses its own temporary storage, and each worker writes directly to its assigned slices in the X-fastest output buffer. On the same build, the CPU provider produces bitwise-identical nodes across worker counts. A custom provider's index must support concurrent read-only queries.

Invalid geometry and settings raise diagnostics. Oversized dimensions and coordinate ranges report overflow. Sample-buffer allocation failures identify the requested count. If thread creation or a query throws, workers finish their current slices and stop. The caller joins all started threads and propagates the exception. Publication takes place after all signed queries succeed.

[Parallel bake measurements and reproduction](parallel-baking.md).
