#pragma once

#include <HibikiPhysics/AnalyticShapes.h>
#include <HibikiPhysics/Plugin.h>

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
};

[[nodiscard]] const CapabilitySlot<GeometryLibrary> &GeometryLibrarySlot();
[[nodiscard]] CapabilityRegistration				 GeometryLibraryPlugin();
} // namespace HibikiPhysics
