#pragma once

#include <HibikiPhysics/AnalyticShapes.h>
#include <HibikiPhysics/Math.h>
#include <HibikiPhysics/Plugin.h>
#include <HibikiPhysics/HeightField.h>

namespace HibikiPhysics
{
class GeometryLibrary
{
  public:
	[[nodiscard]] double				SignedDistance(Plane inPlane, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool					Contains(HalfSpace inHalfSpace, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool					Contains(const Sphere &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool					Contains(const Box &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool					Contains(const Capsule &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool					Contains(const Cylinder &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] bool					Contains(const Ellipsoid &inShape, const std::array<double, 3> &inPoint) const;
	[[nodiscard]] std::array<double, 3> ClosestPoint(const RectangularPatch &inPatch, const std::array<double, 3> &inPoint) const;

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
