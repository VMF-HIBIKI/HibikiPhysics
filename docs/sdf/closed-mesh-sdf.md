# Signed distance to a closed triangle mesh

**English** | [简体中文](closed-mesh-sdf.zh-CN.md)

`ClosedMeshSdf` accepts a triangle mesh in local coordinates (metres). Construction builds one face index and validates the shell, including its edge and vertex connectivity and triangle intersections. A through-hole can belong to the same shell. Invalid surfaces throw `std::invalid_argument`. Pass a `SpatialQueryProvider` as the second argument to choose the index implementation. Copies share the immutable mesh and index and support concurrent read-only queries. [Solid validation](../geometry/solid-mesh-validation.md)

`Classify(point)` returns `SolidPointLocation::Interior`, `Boundary` or `Exterior`. The boundary consists of points exactly on a triangle, including its edges and vertices, at the stored binary64 coordinates. `SignedDistance(point)` returns negative Euclidean distance in material, positive distance in empty space and zero on the boundary. Its magnitude uses the indexed closest-triangle query. Point coordinates must be finite. [Unsigned mesh queries](triangle-queries.md)

For example, a 40 × 40 × 10 mm plate centred on the origin with a centred 10 × 10 mm square through-hole along Z gives these distances along X.

| Local point (m) | Location | Signed distance (m) |
| --- | --- | --- |
| (0, 0, 0) | Hole centre | +0.005 |
| (0.005, 0, 0) | Hole wall | 0 |
| (0.007, 0, 0) | Plate material | −0.002 |

The classifier counts forward crossings along +X by default. `Classify(point, RayAxis::Y)` and `RayAxis::Z` select the other cardinal rays. In the plane perpendicular to the ray, each projected triangle uses counterclockwise winding. It owns its upward and leftward horizontal edges. This half-open rule handles shared edges, vertices and tangencies consistently across face order and original winding. A face parallel to the ray contributes zero crossings and can still contain the query point. Filtered orientation predicates use exact integer determinants when double precision cannot resolve the sign. `GeometryLibrary::Contains(TriangleMesh, ...)` continues to query the mesh surface.
