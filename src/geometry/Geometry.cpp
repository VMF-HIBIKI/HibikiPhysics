#include <HibikiPhysics/Geometry.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace HibikiPhysics
{
namespace
{
void RequirePositive(double inSize, const char *inParameter)
{
	if (!std::isfinite(inSize) || inSize <= 0)
		throw std::invalid_argument(inParameter);
}

void ValidatePoint(const std::array<double, 3> &inPoint)
{
	for (double coordinate : inPoint)
		if (!std::isfinite(coordinate))
			throw std::invalid_argument("Expected a finite local point");
}

constexpr double Infinity = std::numeric_limits<double>::infinity();

AABB CenteredBounds(const std::array<double, 3> &inHalf)
{
	return {{-inHalf[0], -inHalf[1], -inHalf[2]}, inHalf};
}

// Normalize only after checking the axis ranges. This keeps squares bounded
// for radii near either end of the double range.
template <std::size_t N>
bool InClosedEllipsoid(const std::array<double, N> &inPoint, const std::array<double, N> &inAxes)
{
	double squared = 0;
	for (std::size_t i = 0; i < N; ++i)
	{
		const double magnitude = std::abs(inPoint[i]);
		if (magnitude > inAxes[i])
			return false;
		if (magnitude == inAxes[i])
		{
			// At an axis tip, every other coordinate must be exactly zero.
			return std::count(inPoint.begin(), inPoint.end(), 0.0) == static_cast<std::ptrdiff_t>(N - 1);
		}
		const double unit = magnitude / inAxes[i];
		squared += unit * unit;
	}
	return squared <= 1;
}
} // namespace

AABB::AABB(std::array<double, 3> inMin, std::array<double, 3> inMax) : mMin(inMin), mMax(inMax)
{
	for (std::size_t i = 0; i < mMin.size(); ++i)
		if (!(mMin[i] <= mMax[i]) || mMin[i] == Infinity || mMax[i] == -Infinity)
			throw std::invalid_argument("Expected ordered AABB endpoints enclosing finite coordinates");
}

bool AABB::Contains(const std::array<double, 3> &inPoint) const
{
	ValidatePoint(inPoint);
	for (std::size_t i = 0; i < inPoint.size(); ++i)
		if (inPoint[i] < mMin[i] || inPoint[i] > mMax[i])
			return false;
	return true;
}

RectangularPatch::RectangularPatch(std::array<double, 2> inHalfExtents) : mHalfExtents(inHalfExtents)
{
	for (double extent : mHalfExtents)
		RequirePositive(extent, "Expected positive finite patch half extents");
}

Sphere::Sphere(double inRadius) : mRadius(inRadius)
{
	RequirePositive(mRadius, "Expected a positive finite sphere radius");
}

Box::Box(std::array<double, 3> inHalfExtents) : mHalfExtents(inHalfExtents)
{
	for (double extent : mHalfExtents)
		RequirePositive(extent, "Expected positive finite box half extents");
}

Capsule::Capsule(double inRadius, double inHalfCylinderLength) : mRadius(inRadius), mHalfCylinderLength(inHalfCylinderLength)
{
	RequirePositive(mRadius, "Expected a positive finite capsule radius");
	if (!std::isfinite(mHalfCylinderLength) || mHalfCylinderLength < 0)
		throw std::invalid_argument("Expected a nonnegative finite capsule half cylinder length");
}

std::array<double, 3> Capsule::GetHalfExtents() const
{
	const double tip = mHalfCylinderLength + mRadius;
	if (!std::isfinite(tip))
		throw std::overflow_error("Capsule axial half extent exceeds finite range");
	return {mRadius, mRadius, tip};
}

Cylinder::Cylinder(double inRadius, double inHalfHeight) : mRadius(inRadius), mHalfHeight(inHalfHeight)
{
	RequirePositive(mRadius, "Expected a positive finite cylinder radius");
	RequirePositive(mHalfHeight, "Expected a positive finite cylinder half height");
}

Ellipsoid::Ellipsoid(std::array<double, 3> inSemiAxes) : mSemiAxes(inSemiAxes)
{
	for (double axis : mSemiAxes)
		RequirePositive(axis, "Expected positive finite ellipsoid semi-axes");
}

double GeometryLibrary::SignedDistance(Plane, const std::array<double, 3> &inPoint) const
{
	ValidatePoint(inPoint);
	return inPoint[2];
}

bool GeometryLibrary::Contains(HalfSpace inHalfSpace, const std::array<double, 3> &inPoint) const
{
	return SignedDistance(inHalfSpace.GetBoundary(), inPoint) <= 0;
}

bool GeometryLibrary::Contains(const Sphere &inShape, const std::array<double, 3> &inPoint) const
{
	ValidatePoint(inPoint);
	const double r = inShape.GetRadius();
	return InClosedEllipsoid<3>(inPoint, {r, r, r});
}

bool GeometryLibrary::Contains(const Box &inShape, const std::array<double, 3> &inPoint) const
{
	return LocalBounds(inShape).Contains(inPoint);
}

bool GeometryLibrary::Contains(const Capsule &inShape, const std::array<double, 3> &inPoint) const
{
	ValidatePoint(inPoint);
	const double dz = std::max(std::abs(inPoint[2]) - inShape.GetHalfCylinderLength(), 0.0);
	const double r = inShape.GetRadius();
	return InClosedEllipsoid<3>({inPoint[0], inPoint[1], dz}, {r, r, r});
}

bool GeometryLibrary::Contains(const Cylinder &inShape, const std::array<double, 3> &inPoint) const
{
	ValidatePoint(inPoint);
	const double r = inShape.GetRadius();
	return std::abs(inPoint[2]) <= inShape.GetHalfHeight() && InClosedEllipsoid<2>({inPoint[0], inPoint[1]}, {r, r});
}

bool GeometryLibrary::Contains(const Ellipsoid &inShape, const std::array<double, 3> &inPoint) const
{
	ValidatePoint(inPoint);
	return InClosedEllipsoid(inPoint, inShape.GetSemiAxes());
}

AABB GeometryLibrary::LocalBounds(Plane) const
{
	return {{-Infinity, -Infinity, 0}, {Infinity, Infinity, 0}};
}

AABB GeometryLibrary::LocalBounds(HalfSpace) const
{
	return {{-Infinity, -Infinity, -Infinity}, {Infinity, Infinity, 0}};
}

AABB GeometryLibrary::LocalBounds(const RectangularPatch &inShape) const
{
	const auto half = inShape.GetHalfExtents();
	return CenteredBounds({half[0], half[1], 0});
}

AABB GeometryLibrary::LocalBounds(const Sphere &inShape) const
{
	const double r = inShape.GetRadius();
	return CenteredBounds({r, r, r});
}

AABB GeometryLibrary::LocalBounds(const Box &inShape) const
{
	return CenteredBounds(inShape.GetHalfExtents());
}

AABB GeometryLibrary::LocalBounds(const Capsule &inShape) const
{
	auto		 half = inShape.GetHalfExtents();
	const double r = inShape.GetRadius(), h = inShape.GetHalfCylinderLength();
	// The subtraction from the larger addend is exact. Round the bound outward
	// when the representable sum fell short of the mathematical tip.
	if (half[2] - std::max(r, h) < std::min(r, h))
	{
		half[2] = std::nextafter(half[2], Infinity);
		if (!std::isfinite(half[2]))
			throw std::overflow_error("Capsule AABB exceeds finite range");
	}
	return CenteredBounds(half);
}

AABB GeometryLibrary::LocalBounds(const Cylinder &inShape) const
{
	return CenteredBounds(inShape.GetHalfExtents());
}

AABB GeometryLibrary::LocalBounds(const Ellipsoid &inShape) const
{
	return CenteredBounds(inShape.GetSemiAxes());
}

std::array<double, 3> GeometryLibrary::ClosestPoint(const RectangularPatch &inPatch, const std::array<double, 3> &inPoint) const
{
	ValidatePoint(inPoint);
	const auto half = inPatch.GetHalfExtents();
	return {std::clamp(inPoint[0], -half[0], half[0]), std::clamp(inPoint[1], -half[1], half[1]), 0};
}

const CapabilitySlot<GeometryLibrary> &GeometryLibrarySlot()
{
	static const CapabilitySlot<GeometryLibrary> slot("geometry.library");
	return slot;
}

CapabilityRegistration GeometryLibraryPlugin()
{
	return GeometryLibrarySlot().Factory("hibiki.geometry", []
										 { return std::make_unique<GeometryLibrary>(); });
}
} // namespace HibikiPhysics
