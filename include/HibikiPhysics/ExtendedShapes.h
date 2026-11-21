#pragma once

namespace HibikiPhysics
{
// The base and top lie on local Z at minus and plus the half-height.
// For the tapered capsule these positions are the centres of its endpoint balls.
class Cone
{
  public:
	Cone(double inRadius, double inHalfHeight);
	[[nodiscard]] double GetRadius() const noexcept { return mRadius; }
	[[nodiscard]] double GetHalfHeight() const noexcept { return mHalfHeight; }

  private:
	double mRadius;
	double mHalfHeight;
};

class Frustum
{
  public:
	Frustum(double inBottomRadius, double inTopRadius, double inHalfHeight);
	[[nodiscard]] double GetBottomRadius() const noexcept { return mBottomRadius; }
	[[nodiscard]] double GetTopRadius() const noexcept { return mTopRadius; }
	[[nodiscard]] double GetHalfHeight() const noexcept { return mHalfHeight; }

  private:
	double mBottomRadius;
	double mTopRadius;
	double mHalfHeight;
};

class TaperedCapsule
{
  public:
	TaperedCapsule(double inBottomRadius, double inTopRadius, double inHalfSpineLength);
	[[nodiscard]] double GetBottomRadius() const noexcept { return mBottomRadius; }
	[[nodiscard]] double GetTopRadius() const noexcept { return mTopRadius; }
	[[nodiscard]] double GetHalfSpineLength() const noexcept { return mHalfSpineLength; }

  private:
	double mBottomRadius;
	double mTopRadius;
	double mHalfSpineLength;
};
} // namespace HibikiPhysics
