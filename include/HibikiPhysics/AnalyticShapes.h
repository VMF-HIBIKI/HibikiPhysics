#pragma once

#include <array>

namespace HibikiPhysics
{
// Closed intervals along the axes of the query frame. Individual ends may extend to infinity.
class AABB
{
  public:
	AABB(std::array<double, 3> inMin, std::array<double, 3> inMax);
	[[nodiscard]] std::array<double, 3> GetMin() const noexcept { return mMin; }
	[[nodiscard]] std::array<double, 3> GetMax() const noexcept { return mMax; }
	[[nodiscard]] bool					Contains(const std::array<double, 3> &inPoint) const;

  private:
	std::array<double, 3> mMin;
	std::array<double, 3> mMax;
};

// Lengths are in metres. Bounded shapes are centred at the local origin.
// The oriented infinite plane is z = 0 with unit normal +Z.
struct Plane
{
};

// The closed region z <= 0 shares the oriented plane as its boundary.
struct HalfSpace
{
	[[nodiscard]] constexpr Plane GetBoundary() const noexcept { return {}; }
};

class RectangularPatch
{
  public:
	explicit RectangularPatch(std::array<double, 2> inHalfExtents);
	[[nodiscard]] std::array<double, 2> GetHalfExtents() const noexcept { return mHalfExtents; }

  private:
	std::array<double, 2> mHalfExtents;
};

class Sphere
{
  public:
	explicit Sphere(double inRadius);
	[[nodiscard]] double GetRadius() const noexcept { return mRadius; }

  private:
	double mRadius;
};

class Box
{
  public:
	explicit Box(std::array<double, 3> inHalfExtents);
	[[nodiscard]] std::array<double, 3> GetHalfExtents() const noexcept { return mHalfExtents; }

  private:
	std::array<double, 3> mHalfExtents;
};

class Capsule
{
  public:
	Capsule(double inRadius, double inHalfCylinderLength);
	[[nodiscard]] double GetRadius() const noexcept { return mRadius; }
	[[nodiscard]] double GetHalfCylinderLength() const noexcept { return mHalfCylinderLength; }
	// The caps extend another radius beyond the spine endpoints on local Z.
	[[nodiscard]] std::array<double, 3> GetHalfExtents() const;

  private:
	double mRadius;
	double mHalfCylinderLength;
};

class Cylinder
{
  public:
	Cylinder(double inRadius, double inHalfHeight);
	[[nodiscard]] double				GetRadius() const noexcept { return mRadius; }
	[[nodiscard]] double				GetHalfHeight() const noexcept { return mHalfHeight; }
	[[nodiscard]] std::array<double, 3> GetHalfExtents() const noexcept { return {mRadius, mRadius, mHalfHeight}; }

  private:
	double mRadius;
	double mHalfHeight;
};

class Ellipsoid
{
  public:
	explicit Ellipsoid(std::array<double, 3> inSemiAxes);
	[[nodiscard]] std::array<double, 3> GetSemiAxes() const noexcept { return mSemiAxes; }

  private:
	std::array<double, 3> mSemiAxes;
};
} // namespace HibikiPhysics
