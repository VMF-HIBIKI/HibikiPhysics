# Discrete geometry

**English** | [简体中文](discrete-shapes.zh-CN.md)

`GeometryLibrary` accepts three immutable local-space shape values. Construction owns the supplied coordinates and indices. A single value can be queried with different rigid poses.

| Shape | Construction | Closed-set membership |
| --- | --- | --- |
| `Triangle(a, b, c)` | Three distinct, noncollinear `Vec3` vertices | The triangular surface, including its edges and vertices, from either side |
| `ConvexHull(points)` | Unordered 3D point cloud with at least four affinely independent points | Interior and boundary of the convex solid |
| `TriangleMesh(vertices, faces)` | One or more zero-based index triples into a shared vertex array | Union of the referenced triangle surfaces, including boundaries |

A hull starts from a tetrahedron and incrementally extends its outward-facing facets. Duplicate and interior points are allowed. `GetVertices()` retains the supplied point cloud, including those points. For meshes, winding, connectivity and watertightness are unrestricted. A closed tetrahedral mesh still returns false for a strictly interior point. Unused vertices must be finite and do not affect mesh bounds.

```cpp
#include <HibikiPhysics/Geometry.h>

using namespace HibikiPhysics;

GeometryLibrary geometry;
std::vector<Vec3> points{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
ConvexHull solid(points);
TriangleMesh shell(points, {{0, 1, 2}, {0, 3, 1}, {0, 2, 3}, {1, 3, 2}});
bool insideSolid = geometry.Contains(solid, {0.1, 0.1, 0.1}); // true
bool onShell = geometry.Contains(shell, {0.1, 0.1, 0.1});    // false
```

`WorldBounds(shape, worldFromLocal)` projects the actual vertices onto the three world axes. An oblique or asymmetric shape therefore has a tight world AABB. `LocalBounds(shape)` uses the identity pose. `Contains(shape, worldFromLocal, worldPoint)` inverse-transforms the point and applies the same local predicate.

## Numerical contract

Triangle arithmetic is scaled by the maximum absolute component of the vertex differences from its first vertex. Hull arithmetic uses the analogous scale for the full cloud. In this normalized frame, face and edge tests allow 64 double-precision machine epsilons of distance for roundoff. Local and posed queries reject points strictly outside the AABB in the query coordinate frame before applying the tolerance to the local face predicates. Precision can be lost when adding a small shape to a distant world origin.

Nonfinite coordinates, invalid indices, empty meshes, repeated or collinear vertices within a face, and lower-dimensional hulls throw `std::invalid_argument`. Numerically unresolved geometry also throws this exception when a normalized face cross-product length or initial tetrahedron altitude is at most 64 epsilons. An unrepresentable normalization extent or bounds projection throws `std::overflow_error`.

PhysX describes overlaps with triangle meshes as surface queries. MotrixSim’s approximate convex decomposition builds runtime convex collision geometry from source meshes. These conventions motivate the separate surface and convex-solid types here. [PhysX Geometry Queries](https://nvidia-omniverse.github.io/PhysX/physx/5.3.0/docs/GeometryQueries.html) · [MotrixSim approximate convex decomposition](https://motrixsim.readthedocs.io/en/latest/user_guide/main_function/acd.html)
