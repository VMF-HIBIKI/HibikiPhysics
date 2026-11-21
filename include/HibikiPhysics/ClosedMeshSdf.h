#pragma once

#include <HibikiPhysics/TriangleMeshQueries.h>

namespace HibikiPhysics
{
enum class SolidPointLocation
{
	Interior,
	Boundary,
	Exterior
};

enum class RayAxis
{
	X,
	Y,
	Z
};

// Mesh-local queries on one validated closed shell. Copies share the prepared data.
class ClosedMeshSdf
{
  public:
	explicit ClosedMeshSdf(TriangleMesh inMesh);
	ClosedMeshSdf(TriangleMesh inMesh, const SpatialQueryProvider &inProvider);
	ClosedMeshSdf(const ClosedMeshSdf &) noexcept = default;
	ClosedMeshSdf					 &operator=(const ClosedMeshSdf &) noexcept = default;
	[[nodiscard]] const TriangleMesh &GetMesh() const noexcept;
	[[nodiscard]] SolidPointLocation  Classify(Vec3 inPoint, RayAxis inAxis = RayAxis::X) const;
	// Euclidean distance in metres, negative inside and positive outside.
	[[nodiscard]] double SignedDistance(Vec3 inPoint) const;

  private:
	struct Data;
	std::shared_ptr<const Data> mData;
};
} // namespace HibikiPhysics
