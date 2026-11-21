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

bool IsFinite(Vec3 inVector)
{
	return std::isfinite(inVector.mX) && std::isfinite(inVector.mY) && std::isfinite(inVector.mZ);
}

std::array<double, 3> LocalPoint(const Transform &inWorldFromLocal, Vec3 inWorldPoint)
{
	if (!IsFinite(inWorldPoint))
		throw std::invalid_argument("Expected a finite world point");
	const Vec3 local = inWorldFromLocal.GetRotation().Inverse().Rotate(inWorldPoint - inWorldFromLocal.GetPosition());
	if (!IsFinite(local))
		throw std::overflow_error("World-to-local point conversion exceeds finite range");
	return {local.mX, local.mY, local.mZ};
}

AABB CenteredBounds(const std::array<double, 3> &inHalf)
{
	return {{-inHalf[0], -inHalf[1], -inHalf[2]}, inHalf};
}

struct Interval
{
	double mMin, mMax;
};

Interval SymmetricInterval(double inExtent)
{
	if (!std::isfinite(inExtent))
		throw std::overflow_error("Projected half extent exceeds finite range");
	return {-inExtent, inExtent};
}

// The projection of a capsule adds the radius to the projected half spine.
// Round outward when the representable sum falls short of the tip.
double CapsuleExtent(double inRadius, double inHalfSpine)
{
	double extent = inRadius + inHalfSpine;
	if (extent - std::max(inRadius, inHalfSpine) < std::min(inRadius, inHalfSpine))
		extent = std::nextafter(extent, Infinity);
	if (!std::isfinite(extent))
		throw std::overflow_error("Capsule AABB exceeds finite range");
	return extent;
}

double RadiusBetweenEnds(double bottom, double top, double h, double z)
{
	const double fraction = ((h - std::abs(z)) / h) * 0.5;
	return z < 0 ? std::lerp(bottom, top, fraction) : std::lerp(top, bottom, fraction);
}

// Enclose the two endpoint disks (or balls) along the projected axis.
Interval EndBounds(double inOffset, double inBottomExtent, double inTopExtent)
{
	auto upper = [](double center, double extent)
	{
		if (center >= 0)
			return CapsuleExtent(extent, center);
		const double sum = center + extent;
		const double residual = extent >= -center ? center - (sum - extent) : extent - (sum - center);
		return residual > 0 ? std::nextafter(sum, Infinity) : sum;
	};
	return {-std::max(upper(inOffset, inBottomExtent), upper(-inOffset, inTopExtent)),
			std::max(upper(-inOffset, inBottomExtent), upper(inOffset, inTopExtent))};
}

Interval ProjectLocal(const Cone &inShape, Vec3 d)
{
	return EndBounds(inShape.GetHalfHeight() * d.mZ, inShape.GetRadius() * std::hypot(d.mX, d.mY), 0);
}

Interval ProjectLocal(const Frustum &inShape, Vec3 d)
{
	const double radial = std::hypot(d.mX, d.mY);
	return EndBounds(inShape.GetHalfHeight() * d.mZ, inShape.GetBottomRadius() * radial, inShape.GetTopRadius() * radial);
}

Interval ProjectLocal(const TaperedCapsule &inShape, Vec3 d)
{
	return EndBounds(inShape.GetHalfSpineLength() * d.mZ, inShape.GetBottomRadius(), inShape.GetTopRadius());
}

// Each direction is a world unit axis expressed in the shape's local frame.
Interval ProjectLocal(Plane, Vec3 d)
{
	if (d.mX == 0 && d.mY == 0)
		return {0, 0};
	return {-Infinity, Infinity};
}

Interval ProjectLocal(HalfSpace inShape, Vec3 d)
{
	Interval interval = ProjectLocal(inShape.GetBoundary(), d);
	if (d.mZ > 0)
		interval.mMin = -Infinity;
	else
		interval.mMax = Infinity;
	return interval;
}

Interval ProjectLocal(const RectangularPatch &inShape, Vec3 d)
{
	const auto [hx, hy] = inShape.GetHalfExtents();
	return SymmetricInterval(hx * std::abs(d.mX) + hy * std::abs(d.mY));
}

Interval ProjectLocal(const Sphere &inShape, Vec3)
{
	return SymmetricInterval(inShape.GetRadius());
}

Interval ProjectLocal(const Box &inShape, Vec3 d)
{
	const auto [hx, hy, hz] = inShape.GetHalfExtents();
	return SymmetricInterval(hx * std::abs(d.mX) + hy * std::abs(d.mY) + hz * std::abs(d.mZ));
}

Interval ProjectLocal(const Capsule &inShape, Vec3 d)
{
	return SymmetricInterval(CapsuleExtent(inShape.GetRadius(), inShape.GetHalfCylinderLength() * std::abs(d.mZ)));
}

Interval ProjectLocal(const Cylinder &inShape, Vec3 d)
{
	const double disk = inShape.GetRadius() * std::hypot(d.mX, d.mY);
	return SymmetricInterval(disk + inShape.GetHalfHeight() * std::abs(d.mZ));
}

Interval ProjectLocal(const Ellipsoid &inShape, Vec3 d)
{
	const auto [a, b, c] = inShape.GetSemiAxes();
	return SymmetricInterval(std::hypot(a * d.mX, b * d.mY, c * d.mZ));
}

double TranslateEndpoint(double inEndpoint, double inOffset)
{
	if (std::isinf(inEndpoint))
		return inEndpoint;
	const double result = inEndpoint + inOffset;
	if (!std::isfinite(result))
		throw std::overflow_error("World AABB exceeds finite range");
	return result;
}

AABB ProjectedBounds(const auto &inShape, const Transform &inWorldFromLocal)
{
	constexpr std::array<Vec3, 3> worldAxes = {Vec3{1, 0, 0}, Vec3{0, 1, 0}, Vec3{0, 0, 1}};
	const Quat					  worldToLocal = inWorldFromLocal.GetRotation().Inverse();
	const auto [x, y, z] = inWorldFromLocal.GetPosition();
	const std::array	  offset{x, y, z};
	std::array<double, 3> low{}, high{};
	for (std::size_t i = 0; i < worldAxes.size(); ++i)
	{
		const Interval interval = ProjectLocal(inShape, worldToLocal.Rotate(worldAxes[i]));
		low[i] = TranslateEndpoint(interval.mMin, offset[i]);
		high[i] = TranslateEndpoint(interval.mMax, offset[i]);
	}
	return {low, high};
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

Cone::Cone(double inRadius, double inHalfHeight) : mRadius(inRadius), mHalfHeight(inHalfHeight)
{
	RequirePositive(mRadius, "Expected a positive finite cone radius");
	RequirePositive(mHalfHeight, "Expected a positive finite cone half-height");
}

Frustum::Frustum(double inBottomRadius, double inTopRadius, double inHalfHeight)
	: mBottomRadius(inBottomRadius), mTopRadius(inTopRadius), mHalfHeight(inHalfHeight)
{
	if (!std::isfinite(mBottomRadius) || !std::isfinite(mTopRadius) || mBottomRadius < 0 || mTopRadius < 0 || std::max(mBottomRadius, mTopRadius) == 0)
		throw std::invalid_argument("Expected nonnegative finite frustum radii with at least one positive radius");
	RequirePositive(mHalfHeight, "Expected a positive finite frustum half-height");
}

TaperedCapsule::TaperedCapsule(double inBottomRadius, double inTopRadius, double inHalfSpineLength)
	: mBottomRadius(inBottomRadius), mTopRadius(inTopRadius), mHalfSpineLength(inHalfSpineLength)
{
	RequirePositive(mBottomRadius, "Expected a positive finite bottom ball radius");
	RequirePositive(mTopRadius, "Expected a positive finite top ball radius");
	if (!std::isfinite(mHalfSpineLength) || mHalfSpineLength < 0)
		throw std::invalid_argument("Expected a nonnegative finite half spine length");
}

bool GeometryLibrary::Contains(const Cone &inShape, const std::array<double, 3> &inPoint) const
{
	return Contains(Frustum(inShape.GetRadius(), 0, inShape.GetHalfHeight()), inPoint);
}

bool GeometryLibrary::Contains(const Frustum &inShape, const std::array<double, 3> &inPoint) const
{
	ValidatePoint(inPoint);
	const auto [x, y, z] = inPoint;
	const double h = inShape.GetHalfHeight();
	if (std::abs(z) > h)
		return false;
	const double bottom = inShape.GetBottomRadius(), top = inShape.GetTopRadius();
	const double radius = RadiusBetweenEnds(bottom, top, h, z);
	return radius == 0 ? x == 0 && y == 0 : InClosedEllipsoid<2>({x, y}, {radius, radius});
}

bool GeometryLibrary::Contains(const TaperedCapsule &inShape, const std::array<double, 3> &inPoint) const
{
	ValidatePoint(inPoint);
	const auto [x, y, z] = inPoint;
	const double h = inShape.GetHalfSpineLength(), bottom = inShape.GetBottomRadius(), top = inShape.GetTopRadius();
	if (bottom == top)
		return Contains(Capsule(bottom, h), inPoint);
	const double bottomZ = z + h, topZ = z - h;
	if (InClosedEllipsoid<3>({x, y, bottomZ}, {bottom, bottom, bottom}) || InClosedEllipsoid<3>({x, y, topZ}, {top, top, top}))
		return true;
	if (h == 0)
		return false;
	// The common tangent normal in a meridian section is (cosine, -sine).
	const double delta = top - bottom;
	const double sine = h <= std::numeric_limits<double>::max() / 2 ? delta / (2 * h) : (delta / h) * 0.5;
	if (std::abs(sine) >= 1 || bottomZ < -sine * bottom || topZ > -sine * top)
		return false;
	const double cosine = std::sqrt((1 - sine) * (1 + sine));
	// Interpolation retains the radius change even when the slope underflows.
	const double distance = std::abs(z) <= h ? RadiusBetweenEnds(bottom, top, h, z) : (z < 0 ? std::fma(sine, bottomZ, bottom) : std::fma(sine, topZ, top));
	return std::hypot(x * cosine, y * cosine) <= distance;
}

bool GeometryLibrary::Contains(const Cone &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const
{
	return Contains(inShape, LocalPoint(inWorldFromLocal, inWorldPoint));
}

AABB GeometryLibrary::LocalBounds(const Cone &inShape) const
{
	return ProjectedBounds(inShape, {});
}

AABB GeometryLibrary::WorldBounds(const Cone &inShape, const Transform &inWorldFromLocal) const
{
	return ProjectedBounds(inShape, inWorldFromLocal);
}

bool GeometryLibrary::Contains(const Frustum &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const
{
	return Contains(inShape, LocalPoint(inWorldFromLocal, inWorldPoint));
}

AABB GeometryLibrary::LocalBounds(const Frustum &inShape) const
{
	return ProjectedBounds(inShape, {});
}

AABB GeometryLibrary::WorldBounds(const Frustum &inShape, const Transform &inWorldFromLocal) const
{
	return ProjectedBounds(inShape, inWorldFromLocal);
}

bool GeometryLibrary::Contains(const TaperedCapsule &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const
{
	return Contains(inShape, LocalPoint(inWorldFromLocal, inWorldPoint));
}

AABB GeometryLibrary::LocalBounds(const TaperedCapsule &inShape) const
{
	return ProjectedBounds(inShape, {});
}

AABB GeometryLibrary::WorldBounds(const TaperedCapsule &inShape, const Transform &inWorldFromLocal) const
{
	return ProjectedBounds(inShape, inWorldFromLocal);
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
	const double r = inShape.GetRadius(), h = inShape.GetHalfCylinderLength();
	return CenteredBounds({r, r, CapsuleExtent(r, h)});
}

AABB GeometryLibrary::LocalBounds(const Cylinder &inShape) const
{
	return CenteredBounds(inShape.GetHalfExtents());
}

AABB GeometryLibrary::LocalBounds(const Ellipsoid &inShape) const
{
	return CenteredBounds(inShape.GetSemiAxes());
}

bool GeometryLibrary::Contains(HalfSpace inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const
{
	return Contains(inShape, LocalPoint(inWorldFromLocal, inWorldPoint));
}

bool GeometryLibrary::Contains(const Sphere &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const
{
	return Contains(inShape, LocalPoint(inWorldFromLocal, inWorldPoint));
}

bool GeometryLibrary::Contains(const Box &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const
{
	return Contains(inShape, LocalPoint(inWorldFromLocal, inWorldPoint));
}

bool GeometryLibrary::Contains(const Capsule &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const
{
	return Contains(inShape, LocalPoint(inWorldFromLocal, inWorldPoint));
}

bool GeometryLibrary::Contains(const Cylinder &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const
{
	return Contains(inShape, LocalPoint(inWorldFromLocal, inWorldPoint));
}

bool GeometryLibrary::Contains(const Ellipsoid &inShape, const Transform &inWorldFromLocal, Vec3 inWorldPoint) const
{
	return Contains(inShape, LocalPoint(inWorldFromLocal, inWorldPoint));
}

AABB GeometryLibrary::WorldBounds(Plane inShape, const Transform &inWorldFromLocal) const
{
	return ProjectedBounds(inShape, inWorldFromLocal);
}

AABB GeometryLibrary::WorldBounds(HalfSpace inShape, const Transform &inWorldFromLocal) const
{
	return ProjectedBounds(inShape, inWorldFromLocal);
}

AABB GeometryLibrary::WorldBounds(const RectangularPatch &inShape, const Transform &inWorldFromLocal) const
{
	return ProjectedBounds(inShape, inWorldFromLocal);
}

AABB GeometryLibrary::WorldBounds(const Sphere &inShape, const Transform &inWorldFromLocal) const
{
	return ProjectedBounds(inShape, inWorldFromLocal);
}

AABB GeometryLibrary::WorldBounds(const Box &inShape, const Transform &inWorldFromLocal) const
{
	return ProjectedBounds(inShape, inWorldFromLocal);
}

AABB GeometryLibrary::WorldBounds(const Capsule &inShape, const Transform &inWorldFromLocal) const
{
	return ProjectedBounds(inShape, inWorldFromLocal);
}

AABB GeometryLibrary::WorldBounds(const Cylinder &inShape, const Transform &inWorldFromLocal) const
{
	return ProjectedBounds(inShape, inWorldFromLocal);
}

AABB GeometryLibrary::WorldBounds(const Ellipsoid &inShape, const Transform &inWorldFromLocal) const
{
	return ProjectedBounds(inShape, inWorldFromLocal);
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
