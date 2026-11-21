# Cone, frustum and tapered capsule

**English** | [简体中文](extended-shapes.zh-CN.md)

Include `HibikiPhysics/Geometry.h` and use the `GeometryLibrary` supplied by `GeometryLibraryPlugin()`. Dimensions are in metres, and the symmetry axis is local Z. All three solids include their boundary.

| Constructor | Geometry |
| --- | --- |
| `Cone(radius, halfHeight)` | Base disk at −halfHeight, apex at +halfHeight. Both dimensions must be positive. |
| `Frustum(bottomRadius, topRadius, halfHeight)` | End disks at −halfHeight and +halfHeight. The cross-section radius varies linearly with Z. Half-height must be positive. Either radius may be zero, provided the other is positive. Equal radii give a cylinder. |
| `TaperedCapsule(bottomRadius, topRadius, halfSpineLength)` | Convex hull of two closed balls with positive radii. Their centres lie at −halfSpineLength and +halfSpineLength. Half spine length may be zero. |

The tapered capsule's straight side is tangent to both balls. Equal radii give an ordinary capsule. When one ball contains the other (including coincident centres), the hull equals the larger ball.

`Contains(shape, localPoint)` tests a local coordinate. `Contains(shape, worldFromLocal, worldPoint)` applies the inverse rigid pose. `LocalBounds(shape)` and `WorldBounds(shape, worldFromLocal)` bound the endpoint disks or balls projected on each query axis. With unequal radii the bounds can be asymmetric about the origin.

All dimensions and query coordinates must be finite. Invalid input throws `std::invalid_argument`. Bounds or world-to-local conversions that exceed the finite range throw `std::overflow_error`.
