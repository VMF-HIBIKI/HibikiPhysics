# Validating a closed triangle mesh

**English** | [简体中文](solid-mesh-validation.zh-CN.md)

Before using a triangle mesh to define a solid, call `ValidateClosedSolid(mesh, faceIndex)` from `HibikiPhysics/SolidMeshValidation.h`. A successful call accepts one connected, closed shell. The shell can have through-holes. Intersecting surfaces, separate solids and nested shells representing enclosed cavities are rejected. Triangle winding can vary from face to face. Validation leaves the original mesh, indices and face order intact.

Build the index once from the mesh's local triangle bounds. Assign each entry the zero-based position of its face in `GetFaces()`. Every face must have an entry. The validator requests overlapping pairs from the supplied index, so subsequent distance and ray queries can share the same hierarchy.

```cpp
#include <HibikiPhysics/Geometry.h>
#include <HibikiPhysics/SolidMeshValidation.h>
#include <HibikiPhysics/SpatialIndex.h>

using namespace HibikiPhysics;

TriangleMesh tetrahedron({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}},
                         {{0, 1, 2}, {0, 3, 1}, {0, 2, 3}, {1, 3, 2}});
GeometryLibrary geometry;
const auto &vertices = tetrahedron.GetVertices();
std::vector<SpatialEntry> entries;
for (const auto &[a, b, c] : tetrahedron.GetFaces())
    entries.push_back({entries.size(), geometry.LocalBounds(Triangle(vertices[a], vertices[b], vertices[c]))});
auto index = MakeCpuSpatialQueryProvider()->Build(entries);
ValidateClosedSolid(tetrahedron, *index);
```

For connectivity, all three coordinates must compare equal. An imported box with separate per-face vertex records can therefore pass. Even a one-ULP separation creates a real gap. Unreferenced vertices do not participate. Each undirected edge must join exactly two faces. The faces at each vertex must form a single cycle when followed across incident edges.

For each pair of candidate triangles, the intersection must consist solely of their shared vertex or shared edge, or be empty. The narrow phase tests edge crossings, endpoint contacts and coplanar overlaps using orientation predicates. When the floating-point computation cannot resolve a sign, an integer determinant resolves it exactly for the stored coordinates.

A failure throws `std::invalid_argument`. Edge errors report a source face and its original endpoint indices. Pinched vertices identify a source vertex and incident face. Duplicate faces, intersections and disconnected shells identify source faces. All indices in messages are zero-based.
