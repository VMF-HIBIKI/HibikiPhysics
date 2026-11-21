#pragma once

namespace HibikiPhysics
{
class TriangleMesh;
class SpatialOverlapIndex;

// Requires a complete overlap index of this mesh's local face bounds, keyed by source face index.
// Throws std::invalid_argument for surfaces that do not bound a single embedded closed solid.
void ValidateClosedSolid(const TriangleMesh &inMesh, const SpatialOverlapIndex &inFaceIndex);
} // namespace HibikiPhysics
