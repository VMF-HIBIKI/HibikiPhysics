#pragma once

#include <HibikiPhysics/DiscreteShapes.h>
#include <HibikiPhysics/SpatialIndex.h>
#include <memory>
#include <vector>

namespace HibikiPhysics
{
struct MeshClosestPoint
{
	Vec3		mPoint;
	double		mDistance;
	std::size_t mFace;
};

enum class TriangleRayFeature
{
	Face,
	Edge,
	Vertex,
	Coplanar
};

struct MeshRayHit
{
	std::size_t		   mFace;
	double			   mParameter;
	TriangleRayFeature mFeature;
};

// Immutable mesh-local queries. Primitive IDs equal source face indices.
class TriangleMeshQueries
{
  public:
	explicit TriangleMeshQueries(TriangleMesh inMesh);
	TriangleMeshQueries(TriangleMesh inMesh, const SpatialQueryProvider &inProvider);
	TriangleMeshQueries(const TriangleMeshQueries &) noexcept = default;
	TriangleMeshQueries					  &operator=(const TriangleMeshQueries &) noexcept = default;
	[[nodiscard]] const TriangleMesh	  &GetMesh() const noexcept;
	[[nodiscard]] const SpatialQueryIndex &GetIndex() const noexcept;
	[[nodiscard]] MeshClosestPoint		   Closest(Vec3 inPoint) const;
	// Hits are ordered by forward ray parameter, then source face ID.
	[[nodiscard]] std::vector<MeshRayHit> RayHits(Vec3 inOrigin, Vec3 inDirection) const;

  private:
	struct Data;
	std::shared_ptr<const Data> mData;
};
} // namespace HibikiPhysics
