# Dense SDF grid

**English** | [简体中文](dense-sdf-grid.zh-CN.md)

`DenseSdfGrid` stores CPU distance samples at nodes in mesh-local coordinates. Include `<HibikiPhysics/DenseSdfGrid.h>` and link `hibiki::physics`. Coordinates, uniform spacing, stored values and interpolation results use double precision (64-bit) and metres.

`DenseSdfGrid(origin, spacing, {nx, ny, nz}, values)` places node `(0, 0, 0)` at `origin`. Node `(x, y, z)` lies at `origin + spacing * (x, y, z)`. Each count is a number of nodes, so an axis with four nodes spans three intervals. The contiguous buffer holds X neighbors next to one another, followed by Y rows and then Z planes. The offset of a node is `x + nx * (y + ny * z)`. The constructor takes ownership of a moved vector. Copies of the grid share the immutable values, available through `GetValues()`.

`GetNode(x, y, z)` reads an individual value and throws `std::out_of_range` if any index exceeds its axis bounds. `Sample(point)` interpolates the eight corners of the enclosing cell along X, Y and Z. The first and last planes on every axis belong to the domain. A finite point outside any plane returns `std::nullopt`. A nonfinite point throws `std::invalid_argument`.

For the plane at local X = 1 m, the signed distance is `x - 1`. This grid samples that distance on two X planes spaced 2 m apart.

```cpp
HibikiPhysics::DenseSdfGrid grid({0, 0, 0}, 2, {2, 2, 2}, {-1, 1, -1, 1, -1, 1, -1, 1});
const auto inside = grid.Sample({0.5, 1, 2}); // -0.5 m on the last Z plane
const auto outside = grid.Sample({0.5, 1, 3}); // std::nullopt
```

Construction requires a finite origin, positive finite spacing, at least two nodes per axis and exactly one finite value per node. Adjacent coordinates must remain distinct in double precision. Invalid inputs produce `std::invalid_argument` with the failed condition. A node count product beyond the addressable value buffer or an axis coordinate or span beyond finite range produces `std::overflow_error`.
