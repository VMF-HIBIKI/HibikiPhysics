#pragma once

#include <HibikiPhysics/ExtendedShapes.h>
#include <HibikiPhysics/AnalyticShapes.h>
#include <HibikiPhysics/DiscreteShapes.h>
#include <HibikiPhysics/Math.h>
#include <HibikiPhysics/Plugin.h>
#include <HibikiPhysics/HeightField.h>

namespace HibikiPhysics
{
class GeometryLibrary
{
  public:
	[[nodiscard]] bool Contains(const Cone &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool Contains(const Cone &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const;
	[[nodiscard]] AABB LocalBounds(const Cone &inShape) const;
	[[nodiscard]] AABB WorldBounds(const Cone &inShape, const Transform &inWorldFromLocal) const;
	[[nodiscard]] bool Contains(const Frustum &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool Contains(const Frustum &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const;
	[[nodiscard]] AABB LocalBounds(const Frustum &inShape) const;
	[[nodiscard]] AABB WorldBounds(const Frustum &inShape, const Transform &inWorldFromLocal) const;
	[[nodiscard]] bool Contains(const TaperedCapsule &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool Contains(const TaperedCapsule &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const;
	[[nodiscard]] AABB LocalBounds(const TaperedCapsule &inShape) const;
	[[nodiscard]] AABB WorldBounds(const TaperedCapsule &inShape, const Transform &inWorldFromLocal) const;

	[[nodiscard]] double				SignedDistance(Plane inPlane, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool					Contains(HalfSpace inHalfSpace, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool					Contains(const Sphere &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool					Contains(const Box &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool					Contains(const Capsule &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool					Contains(const Cylinder &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool					Contains(const Ellipsoid &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] std::array<double, 3> ClosestPoint(const RectangularPatch &inPatch, const std::array<double, 3> &inPoint) const;

	[[nodiscard]] bool Contains(const Triangle &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool Contains(const Triangle &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const;
	[[nodiscard]] AABB LocalBounds(const Triangle &inShape) const;
	[[nodiscard]] AABB WorldBounds(const Triangle &inShape, const Transform &inWorldFromLocal) const;
	[[nodiscard]] bool Contains(const ConvexHull &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool Contains(const ConvexHull &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const;
	[[nodiscard]] AABB LocalBounds(const ConvexHull &inShape) const;
	[[nodiscard]] AABB WorldBounds(const ConvexHull &inShape, const Transform &inWorldFromLocal) const;
	[[nodiscard]] bool Contains(const TriangleMesh &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool Contains(const TriangleMesh &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const;
	[[nodiscard]] AABB LocalBounds(const TriangleMesh &inShape) const;
	[[nodiscard]] AABB WorldBounds(const TriangleMesh &inShape, const Transform &inWorldFromLocal) const;

	[[nodiscard]] AABB LocalBounds(Plane) const;
	[[nodiscard]] AABB LocalBounds(HalfSpace) const;
	[[nodiscard]] AABB LocalBounds(const RectangularPatch &inShape) const;
	[[nodiscard]] AABB LocalBounds(const Sphere &inShape) const;
	[[nodiscard]] AABB LocalBounds(const Box &inShape) const;
	[[nodiscard]] AABB LocalBounds(const Capsule &inShape) const;
	[[nodiscard]] AABB LocalBounds(const Cylinder &inShape) const;
	[[nodiscard]] AABB LocalBounds(const Ellipsoid &inShape) const;

	// The pose maps shape-local coordinates to world coordinates.
	[[nodiscard]] bool Contains(HalfSpace inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const;
	[[nodiscard]] bool Contains(const Sphere &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const;
	[[nodiscard]] bool Contains(const Box &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const;
	[[nodiscard]] bool Contains(const Capsule &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const;
	[[nodiscard]] bool Contains(const Cylinder &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const;
	[[nodiscard]] bool Contains(const Ellipsoid &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const;

	[[nodiscard]] bool Contains(const HeightField &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool Contains(const HeightField &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const;
	[[nodiscard]] AABB LocalBounds(const HeightField &inShape) const;
	[[nodiscard]] AABB WorldBounds(const HeightField &inShape, const Transform &inWorldFromLocal) const;

	[[nodiscard]] AABB WorldBounds(Plane inShape, const Transform &inWorldFromLocal) const;
	[[nodiscard]] AABB WorldBounds(HalfSpace inShape, const Transform &inWorldFromLocal) const;
	[[nodiscard]] AABB WorldBounds(const RectangularPatch &inShape, const Transform &inWorldFromLocal) const;
	[[nodiscard]] AABB WorldBounds(const Sphere &inShape, const Transform &inWorldFromLocal) const;
	[[nodiscard]] AABB WorldBounds(const Box &inShape, const Transform &inWorldFromLocal) const;
	[[nodiscard]] AABB WorldBounds(const Capsule &inShape, const Transform &inWorldFromLocal) const;
	[[nodiscard]] AABB WorldBounds(const Cylinder &inShape, const Transform &inWorldFromLocal) const;
	[[nodiscard]] AABB WorldBounds(const Ellipsoid &inShape, const Transform &inWorldFromLocal) const;
};

[[nodiscard]] const CapabilitySlot<GeometryLibrary> &GeometryLibrarySlot();
[[nodiscard]] CapabilityRegistration				 GeometryLibraryPlugin();
} // namespace HibikiPhysics
