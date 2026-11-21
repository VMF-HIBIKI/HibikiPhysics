# Height field terrain

**English** | [简体中文](height-field.zh-CN.md)

A `HeightField(rows, columns, {dx, dy}, base, samples)` owns a rectangular grid of elevations in metres. The first sample is at the local XY origin. Sample `samples[row * columns + col]` lies at `(col * dx, row * dy)`, with height along +Z. Rows and columns each require at least two samples. Both spacings must be positive and finite, and the grid must have finite extents. The base and every elevation must be finite, with the base at or below the lowest sample.

The terrain is a union of closed triangular prisms with a common base elevation. MuJoCo uses triangular prisms and a finite base for collision geometry. HibikiPhysics takes elevations directly in local length units and fixes the grid origin, layout and triangulation as specified here. [MuJoCo height field reference](https://mujoco.readthedocs.io/en/3.1.0/XMLreference.html#asset-hfield)

Every cell uses the diagonal from its lower-X, lower-Y corner to its higher-X, higher-Y corner. The top varies linearly on each triangle. For example, a 2 × 2 unit-spaced grid with row-major heights `{0, 8, 4, 0}` has height zero at its center and height four at `(0.75, 0.25)`. `SurfaceHeight(x, y)` includes the outer edges and returns an empty optional outside the footprint.

`GeometryLibrary::Contains` includes the terrain top, sides and base. Where the top meets the base, the coincident surface also belongs to the shape. The local bounds extend from `(0, 0, base)` to `((columns - 1) * dx, (rows - 1) * dy, max(samples))`. World queries take a local-to-world rigid pose. World bounds project the sampled surface vertices and four base corners onto each world axis.
